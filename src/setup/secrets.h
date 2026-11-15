#pragma once
#include <QString>

// API keys never go into opencode.json in plain text. We store them in a 0600 file and reference it with
// opencode's `{file:...}` substitution. (Alternative: `{env:NAME}` with the variable exported by the user.)
namespace Secrets {
QString dir();                                                   // <AppConfigLocation>/secrets
QString writeKey(const QString &name, const QString &value);     // returns the file path or empty on error
QString fileRef(const QString &path);                            // "{file:/abs/path}"
QString envRef(const QString &envName);                          // "{env:NAME}"
}
