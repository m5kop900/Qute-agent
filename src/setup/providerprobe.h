#pragma once
#include <QString>
#include <QStringList>

// Talks to an OpenAI-compatible endpoint (llama.cpp server, vLLM, LM Studio, Ollama /v1, OpenRouter, ...).
// Blocking helpers for the CLI (they spin a local event loop). Need a QCoreApplication.
struct ProbeResult {
    bool ok = false;
    int httpStatus = 0;
    QString error;
    QStringList models;     // ids from GET {base}/models
};

struct ChatTestResult {
    bool ok = false;
    QString error;
    QString reply;
};

namespace ProviderProbe {
QString joinUrl(const QString &baseUrl, const QString &path);
ProbeResult listModels(const QString &baseUrl, const QString &apiKey, int timeoutMs = 8000);
ChatTestResult testChat(const QString &baseUrl, const QString &apiKey, const QString &model, int timeoutMs = 90000);
}
