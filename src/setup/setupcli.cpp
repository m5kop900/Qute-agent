#include "setupcli.h"

#include "appconfig.h"
#include "installer.h"
#include "mcppresets.h"
#include "opencodeconfig.h"
#include "opencodelocator.h"
#include "providerprobe.h"
#include "secrets.h"
#include "tty.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace {

struct Opts {
    bool yes = false, dryRun = false, noInstall = false, noMcp = false, noProvider = false, noQute = false, force = false;
    QString channel, method, scope, projectDir, format, mcp, providerId, providerName, baseUrl, apiKeyEnv, models,
        defaultModel, configPath;
    int context = 0, output = 0;
    QStringList positional;
};

using Format = OpencodeConfig::Format;

void usage()
{
    Tty::plain(QStringLiteral(
        "Usage: qute opencode <command> [options]      (alias: qute setup)\n\n"
        "Commands:\n"
        "  setup      full wizard: detect/install opencode, MCP servers, custom OpenAI-compatible provider, Qute Agent link\n"
        "  install    only detect/install opencode (V2 or V1)\n"
        "  mcp        only the MCP step   (also: mcp list | mcp remove <name>)\n"
        "  provider   only the OpenAI-compatible provider step\n"
        "  doctor     check everything and print what is wrong\n\n"
        "Options (all optional; without them the wizard asks):\n"
        "  -y, --yes                 answer yes / use defaults, never prompt (for scripts and AI agents)\n"
        "      --dry-run             show what would be written, change nothing\n"
        "      --channel v2|v1       which OpenCode to install (default v2)\n"
        "      --method <id>         installer id, e.g. v2-pacman, v2-npm, v1-curl (see `qute opencode install`)\n"
        "      --no-install          never install opencode\n"
        "      --force               reinstall even if opencode is present\n"
        "      --scope global|project   where to write the opencode config (default global)\n"
        "      --project-dir <dir>   project folder for --scope project (default: current dir)\n"
        "      --config <file>       exact opencode config file to edit\n"
        "      --format auto|v1|v2   config dialect (default auto: keep the file's style, else V1 = readable by V1 and V2)\n"
        "      --mcp <list>          comma list: context7,playwright,sequential-thinking,github | recommended | none\n"
        "      --no-mcp / --no-provider / --no-qute   skip that step\n"
        "      --base-url <url>      OpenAI-compatible endpoint, e.g. http://127.0.0.1:8080/v1\n"
        "      --provider-id <id>    provider id (default: derived from the URL)\n"
        "      --provider-name <n>   display name\n"
        "      --api-key-env <NAME>  read the key from this environment variable (stored as {env:NAME})\n"
        "      --models <list>       comma list of model ids (default: asks / first from /models)\n"
        "      --context <n>  --output <n>   token limits written for the models\n"
        "      --default-model <ref> provider/model to set as default (default: first model of the new provider)\n"));
}

// ---------- small helpers ----------
int httpStatus(const QString &url, int timeoutMs, QByteArray *body = nullptr)
{
    QNetworkAccessManager nam;
    QNetworkReply *r = nam.get(QNetworkRequest{QUrl(url)});
    QEventLoop loop;
    QTimer t;
    t.setSingleShot(true);
    QObject::connect(&t, &QTimer::timeout, &loop, [&] { r->abort(); });
    QObject::connect(r, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    t.start(timeoutMs);
    loop.exec();
    const int st = r->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (body) *body = r->readAll();
    r->deleteLater();
    return st;
}

QString slug(const QString &s)
{
    QString out = s.toLower();
    out.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")), QStringLiteral("-"));
    while (out.startsWith(QLatin1Char('-'))) out.remove(0, 1);
    while (out.endsWith(QLatin1Char('-'))) out.chop(1);
    return out.isEmpty() ? QStringLiteral("custom") : out;
}

QStringList splitList(const QString &s)
{
    return s.split(QRegularExpression(QStringLiteral("[,\\s]+")), Qt::SkipEmptyParts);
}

int runForwarded(const QString &program, const QStringList &args, int timeoutMs = -1)
{
    QProcess p;
    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.setInputChannelMode(QProcess::ForwardedInputChannel);
    p.start(program, args);
    if (!p.waitForStarted(10000)) return 127;
    if (!p.waitForFinished(timeoutMs)) { p.kill(); return 124; }
    return p.exitCode();
}

QString describe(const OpencodeBinary &b)
{
    return QStringLiteral("%1  (%2, %3)").arg(b.path, b.versionText.isEmpty() ? QStringLiteral("version unknown") : b.versionText,
                                              b.major >= 2 ? QStringLiteral("OpenCode 2") : (b.major == 1 ? QStringLiteral("OpenCode 1") : QStringLiteral("unknown generation")));
}

// ---------- step: detect ----------
OpencodeBinary detect(const QString &channel = QString())
{
    const QList<OpencodeBinary> all = OpencodeLocator::findAll();
    if (all.isEmpty()) {
        Tty::warn(QStringLiteral("opencode was not found in PATH or the usual install folders."));
        return {};
    }
    for (const OpencodeBinary &b : all)
        (b.runs ? Tty::ok(describe(b)) : Tty::warn(b.path + QStringLiteral("  (found, but `--version` failed)")));
    return OpencodeLocator::best(channel);
}

// ---------- step: install ----------
bool installFlow(const Opts &o, OpencodeBinary &bin)
{
    QString channel = o.channel;
    if (channel.isEmpty()) {
        const int c = Tty::choose(QStringLiteral("Which OpenCode do you want?"),
                                  {QStringLiteral("OpenCode 2 (current generation, recommended)"),
                                   QStringLiteral("OpenCode 1 (legacy)")}, 0);
        channel = c == 0 ? QStringLiteral("v2") : QStringLiteral("v1");
    }
    const QList<InstallOption> options = OpencodeInstaller::availableOptions(channel);
    if (options.isEmpty()) {
        Tty::error(QStringLiteral("No supported installer is available on this machine for %1.").arg(channel));
        Tty::info(QStringLiteral("Install it manually: https://opencode.ai/docs/ (V1) or https://opencode.ai/v2/docs/ (V2),"));
        Tty::info(QStringLiteral("then run `qute opencode setup` again. Common prerequisites: curl, or Node.js (npm), or an AUR helper."));
        return false;
    }

    int idx = 0;
    if (!o.method.isEmpty()) {
        idx = -1;
        for (int i = 0; i < options.size(); ++i)
            if (options[i].id == o.method) idx = i;
        if (idx < 0) {
            QStringList ids;
            for (const InstallOption &x : options) ids << x.id;
            Tty::error(QStringLiteral("Installer '%1' is not available here. Available: %2").arg(o.method, ids.join(QStringLiteral(", "))));
            return false;
        }
    } else {
        QStringList labels;
        for (const InstallOption &x : options)
            labels << QStringLiteral("%1   [%2]").arg(x.label, x.displayCommand());
        idx = Tty::choose(QStringLiteral("How should it be installed?"), labels, 0);
    }
    const InstallOption &opt = options[idx];
    if (!opt.note.isEmpty()) Tty::info(opt.note);
    Tty::info(QStringLiteral("Command that will run:  ") + opt.displayCommand());
    if (!Tty::confirm(QStringLiteral("Run it now?"), true)) {
        Tty::warn(QStringLiteral("Skipped. You can run the command yourself and then `qute opencode setup`."));
        return false;
    }
    const int code = OpencodeInstaller::run(opt);
    if (code != 0) {
        Tty::error(QStringLiteral("The installer exited with code %1.").arg(code));
        return false;
    }
    bin = OpencodeLocator::best(channel);
    if (bin.command.isEmpty())
        bin = OpencodeLocator::best();
    if (bin.command.isEmpty()) {
        Tty::warn(QStringLiteral("Installed, but the `%1` command cannot be started yet.").arg(opt.expectedCommand));
        Tty::info(QStringLiteral("Open a NEW terminal (PATH may have changed). Common locations: ~/.opencode/bin, ~/.local/bin, npm's global bin folder."));
        return false;
    }
    Tty::ok(QStringLiteral("Installed: ") + describe(bin));
    if (channel == QLatin1String("v2") && bin.major < 2)
        Tty::warn(QStringLiteral("You asked for OpenCode 2 but the installed binary reports version %1. Check with: %2 --version").arg(bin.versionText, bin.command));
    return true;
}

// ---------- step: config target ----------
bool openConfig(const Opts &o, OpencodeConfig &cfg, QString &scopeLabel)
{
    QString path;
    if (!o.configPath.isEmpty()) {
        path = o.configPath;
        scopeLabel = QStringLiteral("custom file");
    } else {
        QString scope = o.scope;
        const QString cwd = o.projectDir.isEmpty() ? QDir::currentPath() : o.projectDir;
        if (scope.isEmpty()) {
            const int c = Tty::choose(QStringLiteral("Where should the settings be stored?"),
                                      {QStringLiteral("Global: %1  (every project)").arg(OpencodeConfig::globalPath()),
                                       QStringLiteral("This project only: %1").arg(cwd)}, 0);
            scope = c == 0 ? QStringLiteral("global") : QStringLiteral("project");
        }
        path = scope == QLatin1String("project") ? OpencodeConfig::projectPath(cwd) : OpencodeConfig::globalPath();
        scopeLabel = scope;
    }
    QString err;
    if (!cfg.load(path, &err)) {
        Tty::error(QStringLiteral("Cannot use %1: %2").arg(path, err));
        Tty::info(QStringLiteral("Fix the file by hand or pass --config <other file>. Nothing was changed."));
        return false;
    }
    Tty::ok(QStringLiteral("%1 %2").arg(cfg.existed() ? QStringLiteral("Editing") : QStringLiteral("Creating"), path));
    if (cfg.hadComments())
        Tty::warn(QStringLiteral("This file contains comments. They will be lost when saving (a backup is written first)."));
    return true;
}

Format resolveFormat(const Opts &o, const OpencodeConfig &cfg, const OpencodeBinary &bin, QString *why)
{
    if (o.format == QLatin1String("v1")) { *why = QStringLiteral("forced by --format v1"); return Format::V1; }
    if (o.format == QLatin1String("v2")) { *why = QStringLiteral("forced by --format v2"); return Format::V2; }
    if (const auto d = cfg.detectFormat()) {
        *why = QStringLiteral("matches the style already used in the file");
        return *d;
    }
    bool v1Installed = false;
    for (const OpencodeBinary &b : OpencodeLocator::findAll())
        if (b.runs && b.major == 1) v1Installed = true;
    if (bin.major >= 2 && !v1Installed) {
        *why = QStringLiteral("only OpenCode 2 is installed, so the native V2 format is used");
        return Format::V2;
    }
    *why = QStringLiteral("V1 syntax: understood by both OpenCode 1 and OpenCode 2");
    return Format::V1;
}

// ---------- step: MCP ----------
bool askSecretRef(const McpPreset &p, const QString &name, QString *ref)
{
    const QString env = p.secretEnvName;
    const bool envSet = !qEnvironmentVariable(env.toUtf8().constData()).isEmpty();
    QStringList choices;
    choices << QStringLiteral("Paste the key now (stored in a private 0600 file)")
            << QStringLiteral("Use environment variable %1 (you export it yourself)%2").arg(env, envSet ? QStringLiteral(" - currently set") : QString());
    choices << (p.secretRequired ? QStringLiteral("Skip this server") : QStringLiteral("No key (use without)"));
    const int def = envSet ? 1 : (p.secretRequired ? 0 : 2);
    const int c = Tty::choose(QStringLiteral("%1 needs an API key / token%2").arg(p.title, p.secretRequired ? QString() : QStringLiteral(" (optional)")), choices, def);
    if (c == 1) { *ref = Secrets::envRef(env); return true; }
    if (c == 0) {
        const QString key = Tty::askSecret(QStringLiteral("%1 key").arg(p.title));
        if (key.isEmpty()) { *ref = QString(); return !p.secretRequired; }
        const QString path = Secrets::writeKey(name, key);
        if (path.isEmpty()) { Tty::error(QStringLiteral("Could not store the key.")); return false; }
        *ref = Secrets::fileRef(path);
        return true;
    }
    *ref = QString();
    return !p.secretRequired;
}

void mcpStep(const Opts &o, OpencodeConfig &cfg, Format f, QStringList &changes)
{
    const QList<McpPreset> presets = McpPresets::all();
    QStringList selected;
    if (!o.mcp.isEmpty()) {
        if (o.mcp == QLatin1String("none")) return;
        if (o.mcp == QLatin1String("recommended")) { for (const McpPreset &p : presets) if (p.recommended) selected << p.id; }
        else selected = splitList(o.mcp);
    } else {
        QStringList labels;
        QList<int> defs;
        for (int i = 0; i < presets.size(); ++i) {
            labels << QStringLiteral("%1 - %2%3").arg(presets[i].title, presets[i].description, cfg.hasMcp(presets[i].id) ? QStringLiteral("  [already configured]") : QString());
            if (presets[i].recommended && !cfg.hasMcp(presets[i].id)) defs << i;
        }
        for (int i : Tty::chooseMany(QStringLiteral("Which MCP servers do you want?"), labels, defs))
            selected << presets[i].id;
    }

    for (const QString &id : selected) {
        const McpPreset *p = McpPresets::find(id);
        if (!p) { Tty::warn(QStringLiteral("Unknown MCP preset '%1' (known: context7, playwright, sequential-thinking, github)").arg(id)); continue; }
        if (cfg.hasMcp(id) && !Tty::confirm(QStringLiteral("'%1' already exists. Overwrite it?").arg(id), false)) {
            Tty::info(QStringLiteral("Kept the existing '%1'.").arg(id));
            continue;
        }
        if (!p->requiresTool.isEmpty() && QStandardPaths::findExecutable(p->requiresTool, OpencodeLocator::searchPath()).isEmpty()) {
            Tty::warn(QStringLiteral("`%1` is not installed (needed by %2). Install Node.js 18+ from https://nodejs.org or your package manager.").arg(p->requiresTool, p->title));
            if (!Tty::confirm(QStringLiteral("Add %1 to the config anyway?").arg(p->title), true))
                continue;
        }
        OpencodeConfig::McpSpec s;
        s.name = p->id;
        s.remote = p->remote;
        s.command = p->command;
        s.url = p->url;
        if (!p->secretHeader.isEmpty()) {
            QString ref;
            if (!askSecretRef(*p, p->id, &ref)) { Tty::info(QStringLiteral("Skipped %1.").arg(p->title)); continue; }
            if (!ref.isEmpty()) {
                s.headers.insert(p->secretHeader, p->secretHeaderPrefix + ref);
                if (ref.startsWith(QLatin1String("{env:")))
                    Tty::info(QStringLiteral("Remember to export %1 in your shell profile.").arg(p->secretEnvName));
            }
        }
        cfg.setMcp(s, f);
        changes << QStringLiteral("MCP server '%1' (%2)").arg(p->id, p->remote ? p->url : p->command.join(QLatin1Char(' ')));
        Tty::ok(QStringLiteral("Added MCP server %1").arg(p->title));
        if (p->id == QLatin1String("playwright") && !QStandardPaths::findExecutable(QStringLiteral("npx"), OpencodeLocator::searchPath()).isEmpty()
            && Tty::confirm(QStringLiteral("Download the Chromium browser for Playwright now? (npx playwright install chromium)"), false)) {
            runForwarded(QStandardPaths::findExecutable(QStringLiteral("npx"), OpencodeLocator::searchPath()),
                         {QStringLiteral("playwright"), QStringLiteral("install"), QStringLiteral("chromium")});
        }
    }
}

// ---------- step: custom provider ----------
void providerStep(const Opts &o, OpencodeConfig &cfg, Format f, QStringList &changes)
{
    if (o.noProvider) return;
    if (o.baseUrl.isEmpty()) {
        if (!Tty::interactive()) return;
        Tty::info(QStringLiteral("opencode has no built-in menu for custom OpenAI-compatible endpoints."));
        Tty::info(QStringLiteral("This adds one (llama.cpp server, vLLM, LM Studio, Ollama /v1, OpenRouter, any /v1 API)."));
        if (!Tty::confirm(QStringLiteral("Add an OpenAI-compatible provider?"), false)) return;
    }
    for (;;) {
        QString base = Tty::ask(QStringLiteral("Base URL"), o.baseUrl.isEmpty() ? QStringLiteral("http://127.0.0.1:8080/v1") : o.baseUrl);
        while (base.endsWith(QLatin1Char('/'))) base.chop(1);
        const QUrl url(base);
        if (!url.isValid() || url.host().isEmpty()) { Tty::error(QStringLiteral("That is not a valid URL.")); if (!Tty::interactive()) return; continue; }
        const bool isLocal = url.host() == QLatin1String("127.0.0.1") || url.host() == QLatin1String("localhost") || url.host() == QLatin1String("::1");

        OpencodeConfig::ProviderSpec p;
        p.baseUrl = base;
        p.id = slug(Tty::ask(QStringLiteral("Provider id (letters, digits, dashes)"), o.providerId.isEmpty() ? slug(url.host() + (url.port() > 0 ? QStringLiteral("-%1").arg(url.port()) : QString())) : o.providerId));
        p.name = Tty::ask(QStringLiteral("Display name"), o.providerName.isEmpty() ? p.id : o.providerName);
        if (cfg.hasProvider(p.id) && !Tty::confirm(QStringLiteral("Provider '%1' exists. Overwrite it?").arg(p.id), false)) return;

        QString keyValue;                                   // only used for probing, never written to the config
        if (!o.apiKeyEnv.isEmpty()) {
            p.apiKeyRef = Secrets::envRef(o.apiKeyEnv);
            keyValue = qEnvironmentVariable(o.apiKeyEnv.toUtf8().constData());
        } else {
            const int c = Tty::choose(QStringLiteral("API key"),
                                      {QStringLiteral("No key needed (typical for local servers)"),
                                       QStringLiteral("Paste a key (stored in a private 0600 file)"),
                                       QStringLiteral("Read it from an environment variable")}, isLocal ? 0 : 1);
            if (c == 1) {
                keyValue = Tty::askSecret(QStringLiteral("API key"));
                if (!keyValue.isEmpty()) {
                    const QString path = Secrets::writeKey(p.id, keyValue);
                    if (path.isEmpty()) Tty::error(QStringLiteral("Could not store the key; continuing without one."));
                    else p.apiKeyRef = Secrets::fileRef(path);
                }
            } else if (c == 2) {
                const QString env = Tty::ask(QStringLiteral("Variable name"), p.id.toUpper().replace(QLatin1Char('-'), QLatin1Char('_')) + QStringLiteral("_API_KEY"));
                p.apiKeyRef = Secrets::envRef(env);
                keyValue = qEnvironmentVariable(env.toUtf8().constData());
                Tty::info(QStringLiteral("Remember to export %1 before starting opencode.").arg(env));
            }
        }

        // models
        QStringList modelIds = splitList(o.models);
        if (modelIds.isEmpty()) {
            Tty::info(QStringLiteral("Asking %1/models ...").arg(base));
            const ProbeResult pr = ProviderProbe::listModels(base, keyValue);
            if (pr.ok && !pr.models.isEmpty()) {
                Tty::ok(QStringLiteral("Server answered, %1 model(s).").arg(pr.models.size()));
                const QStringList shown = pr.models.mid(0, 40);
                for (int i : Tty::chooseMany(QStringLiteral("Which models should opencode offer?"), shown, {0}))
                    modelIds << shown[i];
            } else {
                Tty::warn(QStringLiteral("Could not list models (%1%2).").arg(pr.error.isEmpty() ? QStringLiteral("empty list") : pr.error,
                                                                           pr.httpStatus ? QStringLiteral(", HTTP %1").arg(pr.httpStatus) : QString()));
                if (isLocal) Tty::info(QStringLiteral("Is the server running? For llama.cpp: llama-server -m <model.gguf> --jinja -c <context>"));
                modelIds = splitList(Tty::ask(QStringLiteral("Model id(s), comma separated"), QString()));
            }
        }
        if (modelIds.isEmpty()) { Tty::warn(QStringLiteral("No models selected; provider not added.")); return; }

        if (isLocal) {
            Tty::info(QStringLiteral("Agents need tool calling: llama.cpp needs --jinja; the context below should not exceed the server's context (-c)."));
        }
        const int ctx = o.context > 0 ? o.context : Tty::ask(QStringLiteral("Context window (tokens)"), QStringLiteral("32768")).toInt();
        const int outTok = o.output > 0 ? o.output : Tty::ask(QStringLiteral("Max output tokens"), QStringLiteral("8192")).toInt();
        for (const QString &id : modelIds) {
            OpencodeConfig::ModelSpec m;
            m.id = id; m.name = id; m.context = ctx > 0 ? ctx : 32768; m.output = outTok > 0 ? outTok : 8192;
            p.models << m;
        }

        if (Tty::confirm(QStringLiteral("Send a tiny test request to '%1'?").arg(modelIds.first()), true)) {
            Tty::info(QStringLiteral("Testing (a local model may need a moment to load) ..."));
            const ChatTestResult t = ProviderProbe::testChat(base, keyValue, modelIds.first());
            if (t.ok) Tty::ok(QStringLiteral("Model replied: \"%1\"").arg(t.reply));
            else Tty::warn(QStringLiteral("Test failed: %1 - the provider is still added; fix the server and retry.").arg(t.error));
        }

        cfg.setProvider(p, f);
        changes << QStringLiteral("Provider '%1' -> %2 (%3 model%4)").arg(p.id, base).arg(p.models.size()).arg(p.models.size() == 1 ? QString() : QStringLiteral("s"));
        Tty::ok(QStringLiteral("Added provider %1").arg(p.id));

        const QString ref = p.id + QLatin1Char('/') + modelIds.first();
        const QString dm = o.defaultModel.isEmpty() ? ref : o.defaultModel;
        if (Tty::confirm(QStringLiteral("Use %1 as the default model?").arg(dm), true)) {
            cfg.setDefaultModel(dm);
            changes << QStringLiteral("Default model = %1").arg(dm);
        }
        if (!o.baseUrl.isEmpty() || !Tty::interactive()) return;
        if (!Tty::confirm(QStringLiteral("Add another provider?"), false)) return;
    }
}

// ---------- step: link Qute Agent ----------
void quteStep(const Opts &o, const OpencodeBinary &bin)
{
    if (o.noQute || bin.command.isEmpty()) return;
    AppConfig wc;
    const bool inPath = !QStandardPaths::findExecutable(bin.command).isEmpty();
    const QString value = inPath ? bin.command : bin.path;       // full path when PATH does not contain it
    if (wc.serverCommand() == value) { Tty::ok(QStringLiteral("Qute Agent already starts opencode with '%1'.").arg(value)); return; }
    if (!Tty::confirm(QStringLiteral("Let Qute Agent start the server with '%1'?").arg(value), true)) return;
    if (o.dryRun) { Tty::info(QStringLiteral("(dry run) would set Qute Agent serverCommand = %1").arg(value)); return; }
    wc.setServerCommand(value);
    wc.setAutoStartServer(true);
    if (wc.save()) Tty::ok(QStringLiteral("Qute Agent config updated (serverCommand = %1).").arg(value));
    else Tty::warn(QStringLiteral("Could not write the Qute Agent config."));
}

// ---------- commands ----------
int cmdDoctor(const Opts &)
{
    Tty::heading(QStringLiteral("Qute Agent / opencode doctor"));
    Tty::step(1, 5, QStringLiteral("opencode binaries"));
    const OpencodeBinary best = detect();
    if (best.command.isEmpty()) Tty::info(QStringLiteral("Fix: qute opencode install"));

    Tty::step(2, 5, QStringLiteral("Node.js (needed by Playwright and most MCP servers)"));
    const QString npx = QStandardPaths::findExecutable(QStringLiteral("npx"), OpencodeLocator::searchPath());
    npx.isEmpty() ? Tty::warn(QStringLiteral("npx not found: Playwright / sequential-thinking MCP servers will not start.")) : Tty::ok(npx);

    Tty::step(3, 5, QStringLiteral("opencode config files"));
    QStringList paths{OpencodeConfig::globalPath(), OpencodeConfig::projectPath(QDir::currentPath())};
    paths.removeDuplicates();
    for (const QString &path : paths) {
        OpencodeConfig c;
        QString err;
        if (!QFileInfo::exists(path)) { Tty::info(path + QStringLiteral("  (does not exist)")); continue; }
        if (!c.load(path, &err)) { Tty::error(path + QStringLiteral(": ") + err); continue; }
        const auto fmt = c.detectFormat();
        Tty::ok(QStringLiteral("%1  style: %2").arg(path, fmt ? (*fmt == Format::V2 ? QStringLiteral("native V2") : QStringLiteral("V1 / compatible")) : QStringLiteral("neutral")));
        Tty::info(QStringLiteral("  MCP: %1").arg(c.mcpNames().isEmpty() ? QStringLiteral("none") : c.mcpNames().join(QStringLiteral(", "))));
        Tty::info(QStringLiteral("  providers: %1").arg(c.providerIds().isEmpty() ? QStringLiteral("none") : c.providerIds().join(QStringLiteral(", "))));
    }

    Tty::step(4, 5, QStringLiteral("opencode server"));
    AppConfig wc;
    const QString base = wc.serverUrl();
    QByteArray body;
    int st = httpStatus(base + QStringLiteral("/global/health"), 2500, &body);
    if (st == 200) Tty::ok(QStringLiteral("%1/global/health -> %2").arg(base, QString::fromUtf8(body).trimmed()));
    else {
        st = httpStatus(base + QStringLiteral("/api/info"), 2500, &body);
        if (st == 200) Tty::ok(QStringLiteral("%1/api/info reachable (OpenCode 2 API)").arg(base));
        else Tty::warn(QStringLiteral("No server at %1 (Qute Agent starts one automatically when needed; or run: opencode serve)").arg(base));
    }

    Tty::step(5, 5, QStringLiteral("Qute Agent"));
    Tty::info(QStringLiteral("config: ") + AppConfig::defaultPath());
    Tty::info(QStringLiteral("serverCommand: ") + wc.serverCommand());
    return 0;
}

int cmdInstall(const Opts &o)
{
    Tty::heading(QStringLiteral("Install opencode"));
    OpencodeBinary bin = detect();
    if (!bin.command.isEmpty() && !o.force) {
        Tty::ok(QStringLiteral("Already installed. Use --force to install anyway."));
        return 0;
    }
    return installFlow(o, bin) ? 0 : 1;
}

int cmdMcpSub(const Opts &o)
{
    if (o.positional.value(0) == QLatin1String("list") || o.positional.value(0) == QLatin1String("remove")) {
        OpencodeConfig cfg;
        QString scope;
        if (!openConfig(o, cfg, scope)) return 1;
        if (o.positional.value(0) == QLatin1String("list")) {
            Tty::info(cfg.mcpNames().isEmpty() ? QStringLiteral("(no MCP servers)") : cfg.mcpNames().join(QLatin1Char('\n') + QStringLiteral("  ")));
            return 0;
        }
        const QString name = o.positional.value(1);
        if (name.isEmpty() || !cfg.removeMcp(name)) { Tty::error(QStringLiteral("No such MCP server: ") + name); return 1; }
        if (o.dryRun) { Tty::plain(QString::fromUtf8(cfg.toJson())); return 0; }
        QString err, bak;
        if (!cfg.save(&err, &bak)) { Tty::error(err); return 1; }
        Tty::ok(QStringLiteral("Removed '%1'. Backup: %2").arg(name, bak.isEmpty() ? QStringLiteral("(none, new file)") : bak));
        return 0;
    }
    return -1;   // not a sub-command: run the normal MCP step
}

int cmdSetup(const Opts &o, const QString &only)
{
    if (!Tty::interactive() && !Tty::assumeYes && !o.dryRun) {
        Tty::error(QStringLiteral("Not running in a terminal. Run it in one, or pass --yes together with the options you want (see --help)."));
        return 2;
    }
    const bool full = only.isEmpty();
    const int total = full ? 8 : 4;
    int n = 0;

    Tty::heading(full ? QStringLiteral("Qute Agent setup wizard") : QStringLiteral("Qute Agent setup: ") + only);
    if (full) {
        Tty::info(QStringLiteral("This will: find or install opencode, add MCP servers (Playwright, Context7, ...),"));
        Tty::info(QStringLiteral("optionally add an OpenAI-compatible model endpoint, and connect Qute Agent to it."));
        Tty::info(QStringLiteral("Nothing is written before you confirm; existing files are backed up first."));
    }

    // 1. detect
    Tty::step(++n, total, QStringLiteral("Looking for opencode"));
    OpencodeBinary bin = detect(o.channel);
    if (bin.command.isEmpty() && !o.channel.isEmpty()) bin = detect();

    // 2. install
    if (full) {
        Tty::step(++n, total, QStringLiteral("Installing opencode"));
        if (!bin.command.isEmpty() && !o.force) Tty::ok(QStringLiteral("Not needed."));
        else if (o.noInstall) Tty::warn(QStringLiteral("opencode is missing and --no-install was given."));
        else if (!installFlow(o, bin)) Tty::warn(QStringLiteral("Continuing without a working opencode binary: the config can still be prepared."));
    }

    // 3. config target
    Tty::step(++n, total, QStringLiteral("Choosing the opencode config file"));
    OpencodeConfig cfg;
    QString scopeLabel;
    if (!openConfig(o, cfg, scopeLabel)) return 1;
    QString why;
    const Format fmt = resolveFormat(o, cfg, bin, &why);
    Tty::info(QStringLiteral("Config dialect: %1 (%2)").arg(fmt == Format::V2 ? QStringLiteral("native V2") : QStringLiteral("V1/compatible"), why));

    QStringList changes;
    if (only.isEmpty() || only == QLatin1String("mcp")) {
        Tty::step(++n, total, QStringLiteral("MCP servers"));
        if (o.noMcp) Tty::info(QStringLiteral("Skipped (--no-mcp)."));
        else mcpStep(o, cfg, fmt, changes);
    }
    if (only.isEmpty() || only == QLatin1String("provider")) {
        Tty::step(++n, total, QStringLiteral("OpenAI-compatible model provider"));
        providerStep(o, cfg, fmt, changes);
    }
    if (full) {
        Tty::step(++n, total, QStringLiteral("Connecting Qute Agent"));
        quteStep(o, bin);
    }

    // write
    Tty::step(++n, total, QStringLiteral("Review and write"));
    if (changes.isEmpty()) {
        Tty::info(QStringLiteral("No changes to the opencode config."));
    } else {
        for (const QString &c : changes) Tty::info(QStringLiteral("+ ") + c);
        if (o.dryRun) {
            Tty::info(QStringLiteral("--dry-run: the file would become:"));
            Tty::plain(QString::fromUtf8(cfg.toJson()));
        } else if (Tty::confirm(QStringLiteral("Write %1 ?").arg(cfg.path()), true)) {
            QString err, bak;
            if (!cfg.save(&err, &bak)) { Tty::error(err); return 1; }
            Tty::ok(QStringLiteral("Saved %1").arg(cfg.path()));
            if (!bak.isEmpty()) Tty::info(QStringLiteral("Backup: ") + bak);
        } else {
            Tty::warn(QStringLiteral("Cancelled. Nothing was written."));
            return 0;
        }
    }

    // verify + next steps
    if (full) {
        Tty::step(++n, total, QStringLiteral("Verify and next steps"));
        if (!bin.command.isEmpty() && !o.dryRun && !changes.isEmpty()
            && Tty::confirm(QStringLiteral("Run `%1 mcp list` to check the servers?").arg(bin.command), true))
            runForwarded(bin.path, {QStringLiteral("mcp"), QStringLiteral("list")}, 60000);
        Tty::plain(QString());
        Tty::info(QStringLiteral("Next:"));
        Tty::info(QStringLiteral("  1. Bind the command  qute toggle  to a keyboard shortcut in your desktop."));
        Tty::info(QStringLiteral("  2. Start the background process:  qute --daemon   (or add it to autostart)."));
        Tty::info(QStringLiteral("  3. Problems? run:  qute opencode doctor"));
    }
    return 0;
}

} // namespace

int SetupCli::run(const QStringList &argsIn)
{
    QStringList args = argsIn;
    if (!args.isEmpty() && args.first() == QLatin1String("opencode"))
        args.removeFirst();
    QString sub = QStringLiteral("setup");
    if (!args.isEmpty() && !args.first().startsWith(QLatin1Char('-')))
        sub = args.takeFirst();
    if (sub == QLatin1String("help")) { usage(); return 0; }

    QCommandLineParser p;
    p.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    p.addPositionalArgument(QStringLiteral("args"), QStringLiteral("sub-command arguments"), QStringLiteral("[args...]"));
    auto flag = [&](const char *name, const char *alias = nullptr) {
        QStringList names{QString::fromLatin1(name)};
        if (alias) names << QString::fromLatin1(alias);
        p.addOption(QCommandLineOption(names, QString()));
    };
    auto value = [&](const char *name) { p.addOption(QCommandLineOption(QString::fromLatin1(name), QString(), QStringLiteral("v"))); };
    flag("yes", "y"); flag("dry-run"); flag("no-install"); flag("no-mcp"); flag("no-provider"); flag("no-qute"); flag("force"); flag("help", "h");
    for (const char *v : {"channel", "method", "scope", "project-dir", "format", "mcp", "provider-id", "provider-name", "base-url",
                          "api-key-env", "models", "default-model", "config", "context", "output"})
        value(v);
    p.process(QStringList{QStringLiteral("qute")} + args);

    if (p.isSet(QStringLiteral("help"))) { usage(); return 0; }

    Opts o;
    o.yes = p.isSet(QStringLiteral("yes"));
    o.dryRun = p.isSet(QStringLiteral("dry-run"));
    o.noInstall = p.isSet(QStringLiteral("no-install"));
    o.noMcp = p.isSet(QStringLiteral("no-mcp"));
    o.noProvider = p.isSet(QStringLiteral("no-provider"));
    o.noQute = p.isSet(QStringLiteral("no-qute"));
    o.force = p.isSet(QStringLiteral("force"));
    o.channel = p.value(QStringLiteral("channel"));
    o.method = p.value(QStringLiteral("method"));
    o.scope = p.value(QStringLiteral("scope"));
    o.projectDir = p.value(QStringLiteral("project-dir"));
    o.format = p.value(QStringLiteral("format")).toLower();
    o.mcp = p.value(QStringLiteral("mcp"));
    o.providerId = p.value(QStringLiteral("provider-id"));
    o.providerName = p.value(QStringLiteral("provider-name"));
    o.baseUrl = p.value(QStringLiteral("base-url"));
    o.apiKeyEnv = p.value(QStringLiteral("api-key-env"));
    o.models = p.value(QStringLiteral("models"));
    o.defaultModel = p.value(QStringLiteral("default-model"));
    o.configPath = p.value(QStringLiteral("config"));
    o.context = p.value(QStringLiteral("context")).toInt();
    o.output = p.value(QStringLiteral("output")).toInt();
    o.positional = p.positionalArguments();
    Tty::assumeYes = o.yes;

    if (sub == QLatin1String("doctor")) return cmdDoctor(o);
    if (sub == QLatin1String("install")) return cmdInstall(o);
    if (sub == QLatin1String("setup")) return cmdSetup(o, QString());
    if (sub == QLatin1String("provider")) return cmdSetup(o, QStringLiteral("provider"));
    if (sub == QLatin1String("mcp")) {
        const int r = cmdMcpSub(o);
        return r >= 0 ? r : cmdSetup(o, QStringLiteral("mcp"));
    }
    Tty::error(QStringLiteral("Unknown command '%1'.").arg(sub));
    usage();
    return 2;
}
