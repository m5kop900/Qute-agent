.pragma library
// Splits a message into prose and fenced code blocks, and highlights code with a small tokenizer.
// Pure functions: no QML types, colors are passed as hex strings.

function split(text) {
    const out = []
    const pushProse = function (chunk) {
        if (chunk.trim() !== "")
            out.push({code: false, lang: "", text: chunk})
    }
    // An unclosed fence (a message still streaming) runs to the end of the text as code.
    const re = /```[ \t]*([\w+#.-]*)[^\n]*\n([\s\S]*?)(```|$)/g
    let last = 0
    let m
    while ((m = re.exec(text)) !== null) {
        if (m.index > last)
            pushProse(text.slice(last, m.index))
        out.push({code: true, lang: m[1].toLowerCase(), text: m[2].replace(/\n$/, "")})
        last = re.lastIndex
    }
    if (last < text.length)
        pushProse(text.slice(last))
    return out
}

// Text content only: RichText collapses newlines and runs of spaces, so keep them explicitly here.
function escapeHtml(s) {
    return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
            .replace(/\t/g, "&nbsp;&nbsp;&nbsp;&nbsp;").replace(/ /g, "&nbsp;").replace(/\n/g, "<br>")
}

const KEYWORDS = {
    js: "abstract as async await break case catch class const continue debugger default delete do else enum export extends false finally for from function if import in instanceof let new null of return static super switch this throw true try typeof undefined var void while with yield",
    py: "and as assert async await break class continue def del elif else except False finally for from global if import in is lambda None nonlocal not or pass raise return True try while with yield",
    cpp: "alignas auto bool break case catch char class const constexpr continue default delete do double else enum explicit extern false float for friend if include define inline int long namespace new nullptr operator private protected public return short signed sizeof static struct switch template this throw true try typedef typename union unsigned using virtual void volatile while",
    sh: "case do done elif else esac export fi for function if in local return then while echo",
    json: "true false null",
    qml: "import property signal function return var let const if else for while switch case break default Item Rectangle Text Column Row component id"
}

function familyOf(lang) {
    if (["js", "javascript", "ts", "typescript", "jsx", "tsx"].indexOf(lang) >= 0) return "js"
    if (["py", "python"].indexOf(lang) >= 0) return "py"
    if (["c", "cc", "cpp", "c++", "h", "hpp"].indexOf(lang) >= 0) return "cpp"
    if (["sh", "bash", "zsh", "shell", "console"].indexOf(lang) >= 0) return "sh"
    if (["json", "jsonc"].indexOf(lang) >= 0) return "json"
    if (["qml"].indexOf(lang) >= 0) return "qml"
    return ""
}

// Returns RichText. colors: {text, dim, keyword, string, number, comment}, all "#rrggbb".
function highlight(code, lang, colors) {
    const family = familyOf(lang)
    const kw = family ? new Set(KEYWORDS[family].split(" ")) : new Set()
    const hashComments = family === "py" || family === "sh"
    const tokenRe = /(\/\/[^\n]*|\/\*[\s\S]*?\*\/|#[^\n]*)|("(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*'|`[\s\S]*?`)|(\b\d+(?:\.\d+)?\b)|([A-Za-z_][A-Za-z0-9_]*)/g
    let html = ""
    let last = 0
    let m
    const span = function (color, s) {
        return "<font color=\"" + color + "\">" + escapeHtml(s) + "</font>"
    }
    while ((m = tokenRe.exec(code)) !== null) {
        if (m.index > last)
            html += escapeHtml(code.slice(last, m.index))
        const word = m[0]
        if (m[1] !== undefined) {
            // "#" starts a comment only in languages that use it; in C/C++ it is a preprocessor line.
            if (word.charAt(0) === "#" && !hashComments && family !== "")
                html += span(colors.text, word)
            else
                html += span(colors.comment, word)
        } else if (m[2] !== undefined) {
            html += span(colors.string, word)
        } else if (m[3] !== undefined) {
            html += span(colors.number, word)
        } else if (kw.has(word)) {
            html += span(colors.keyword, word)
        } else {
            html += span(colors.text, word)
        }
        last = m.index + word.length
    }
    if (last < code.length)
        html += escapeHtml(code.slice(last))
    return html
}
