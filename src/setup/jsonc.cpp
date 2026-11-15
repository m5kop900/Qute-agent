#include "jsonc.h"

#include <cctype>

QByteArray Jsonc::strip(const QByteArray &in, bool *hadComments)
{
    bool had = false;

    // pass 1: remove // and /* */ comments
    QByteArray noComments;
    noComments.reserve(in.size());
    bool inStr = false, esc = false;
    for (qsizetype i = 0; i < in.size(); ++i) {
        const char c = in[i];
        if (inStr) {
            noComments += c;
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; noComments += c; continue; }
        if (c == '/' && i + 1 < in.size()) {
            if (in[i + 1] == '/') {
                had = true;
                while (i < in.size() && in[i] != '\n') ++i;
                if (i < in.size()) noComments += '\n';
                continue;
            }
            if (in[i + 1] == '*') {
                had = true;
                i += 2;
                while (i + 1 < in.size() && !(in[i] == '*' && in[i + 1] == '/')) ++i;
                ++i;
                continue;
            }
        }
        noComments += c;
    }

    // pass 2: remove trailing commas before } or ]
    QByteArray out;
    out.reserve(noComments.size());
    inStr = false; esc = false;
    for (qsizetype i = 0; i < noComments.size(); ++i) {
        const char c = noComments[i];
        if (inStr) {
            out += c;
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; out += c; continue; }
        if (c == ',') {
            qsizetype j = i + 1;
            while (j < noComments.size() && std::isspace(static_cast<unsigned char>(noComments[j]))) ++j;
            if (j < noComments.size() && (noComments[j] == '}' || noComments[j] == ']'))
                continue;
        }
        out += c;
    }

    if (hadComments)
        *hadComments = had;
    return out;
}
