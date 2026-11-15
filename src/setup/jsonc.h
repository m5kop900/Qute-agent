#pragma once
#include <QByteArray>

namespace Jsonc {
// Turns JSONC (comments + trailing commas, as used by opencode.jsonc) into strict JSON.
// String contents are never touched. *hadComments is set when comments were removed.
QByteArray strip(const QByteArray &in, bool *hadComments = nullptr);
}
