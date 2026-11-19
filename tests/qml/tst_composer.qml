import QtQuick
import QtTest

// UI logic of the prompt box (src/overlay/Composer.qml), loaded by file path. Uses the real composerTools and
// catalog from C++ (context properties set in tst_composer_ui.cpp). Covers F1.3 (@ list) and F1.4 (/ list).
TestCase {
    name: "Composer"
    width: 400
    height: 300

    property var comp: null
    property var pickedSpy: null

    function makeComposer() {
        const c = Qt.createComponent(Qt.resolvedUrl("../../src/overlay/Composer.qml"))
        if (c.status !== Component.Ready) {
            console.log(c.errorString())
            fail("Composer.qml did not load")
        }
        const obj = c.createObject(this, {
            theme: themeStub,
            width: 360,
            tools: composerTools,
            commandItems: catalog.slashItems,
        })
        verify(obj !== null)
        return obj
    }

    QtObject {
        id: themeStub
        readonly property color accent: "#6ea8ff"
        readonly property color text: "#1d2033"
        readonly property color textDim: "#8a8fa3"
        readonly property color input: "#ffffff"
        readonly property color border: "#dddddd"
        readonly property color panel: "#f2f7f8fc"
    }

    function init() {
        comp = makeComposer()
        pickedSpy = createTemporaryObject(signalSpy, null, { target: comp, signalName: "listPicked" })
    }

    function cleanup() {
        comp.destroy()
        comp = null
    }

    Component { id: signalSpy; SignalSpy {} }

    function test_slashOpensCommandListAtStart() {
        comp.setText("/comp")
        compare(comp.listKind, "commands")
        compare(comp.visibleItems[0].key, "compact")
        verify(comp.listOpen)
    }

    function test_slashIsOnlyAtStart() {
        comp.setText("hi /comp")
        compare(comp.listKind, "")
    }

    function test_pickingCompactRunsIt() {
        comp.setText("/comp")
        comp.pickIndex(0)
        compare(pickedSpy.count, 1)
        compare(pickedSpy.signalArguments[0][0], "commands")
        compare(pickedSpy.signalArguments[0][1], "compact")
        compare(comp.inputText, "")          // the command was taken out of the box
    }

    function test_escClosesTheCommandList() {
        comp.setText("/co")
        verify(comp.listOpen)
        comp.closeList()
        compare(comp.listKind, "")
        comp.setText("/com")                 // typing again opens it again
        compare(comp.listKind, "commands")
    }

    function test_mentionOpensWithQuery() {
        comp.mentionItems = [{key: "README.md", label: "@README.md", kind: "file", tag: "", sub: ""}]
        const q = createTemporaryObject(signalSpy, null, { target: comp, signalName: "mentionQueryChanged" })
        comp.setText("look at @RE")
        compare(comp.listKind, "mentions")
        compare(q.count, 1)
        compare(q.signalArguments[0][0], "RE")
    }

    function test_pickingAFileInsertsItAsMention() {
        comp.mentionItems = [{key: "README.md", label: "@README.md", kind: "file", tag: "", sub: ""}]
        comp.setText("look at @RE")
        comp.pickIndex(0)
        compare(pickedSpy.count, 1)
        compare(pickedSpy.signalArguments[0][0], "file")
        compare(pickedSpy.signalArguments[0][1], "README.md")
        compare(comp.inputText, "look at @README.md ")
    }

    function test_pickingAnAgentInsertsItsName() {
        comp.mentionItems = [{key: "build", label: "@build", kind: "agent", tag: "AGENT", sub: "Agent"}]
        comp.setText("@bu")
        comp.pickIndex(0)
        compare(pickedSpy.signalArguments[0][0], "agent")
        compare(comp.inputText, "@build ")
    }

    function test_emailIsNotAMention() {
        comp.setText("mail a@b.com")
        compare(comp.listKind, "")
    }

    function test_mentionInTheMiddleReplacesOnlyTheToken() {
        comp.mentionItems = [{key: "src/main.cpp", label: "@src/main.cpp", kind: "file", tag: "", sub: ""}]
        comp.setText("check @sr and tell me")
        comp.setCaret(9)
        comp.pickIndex(0)
        compare(comp.inputText, "check @src/main.cpp  and tell me")
    }
}
