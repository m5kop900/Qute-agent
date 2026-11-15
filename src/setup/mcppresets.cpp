#include "mcppresets.h"

namespace McpPresets {

QList<McpPreset> all()
{
    QList<McpPreset> p;

    McpPreset ctx7;
    ctx7.id = QStringLiteral("context7");
    ctx7.title = QStringLiteral("Context7");
    ctx7.description = QStringLiteral("Up-to-date library docs for the model (remote, free; API key optional for higher limits)");
    ctx7.remote = true;
    ctx7.url = QStringLiteral("https://mcp.context7.com/mcp");
    ctx7.secretHeader = QStringLiteral("CONTEXT7_API_KEY");
    ctx7.secretEnvName = QStringLiteral("CONTEXT7_API_KEY");
    ctx7.recommended = true;
    p << ctx7;

    McpPreset pw;
    pw.id = QStringLiteral("playwright");
    pw.title = QStringLiteral("Playwright");
    pw.description = QStringLiteral("Lets the agent drive a real browser: open pages, click, screenshot (needs Node.js / npx)");
    pw.command = {QStringLiteral("npx"), QStringLiteral("@playwright/mcp@latest")};
    pw.requiresTool = QStringLiteral("npx");
    pw.recommended = true;
    pw.postInstallNote = QStringLiteral("Browsers may need a one-time download: npx playwright install chromium");
    p << pw;

    McpPreset seq;
    seq.id = QStringLiteral("sequential-thinking");
    seq.title = QStringLiteral("Sequential Thinking");
    seq.description = QStringLiteral("Structured step-by-step reasoning tool (needs Node.js / npx)");
    seq.command = {QStringLiteral("npx"), QStringLiteral("-y"), QStringLiteral("@modelcontextprotocol/server-sequential-thinking")};
    seq.requiresTool = QStringLiteral("npx");
    p << seq;

    McpPreset gh;
    gh.id = QStringLiteral("github");
    gh.title = QStringLiteral("GitHub");
    gh.description = QStringLiteral("Issues, PRs, code search (remote; needs a personal access token)");
    gh.remote = true;
    gh.url = QStringLiteral("https://api.githubcopilot.com/mcp/");
    gh.secretHeader = QStringLiteral("Authorization");
    gh.secretHeaderPrefix = QStringLiteral("Bearer ");
    gh.secretEnvName = QStringLiteral("GITHUB_TOKEN");
    gh.secretRequired = true;
    p << gh;

    return p;
}

const McpPreset *find(const QString &id)
{
    static const QList<McpPreset> list = all();
    for (const McpPreset &m : list)
        if (m.id == id)
            return &m;
    return nullptr;
}

} // namespace McpPresets
