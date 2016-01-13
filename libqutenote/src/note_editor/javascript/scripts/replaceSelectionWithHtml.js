function replaceSelectionWithHtml(html) {
    console.log("replaceSelectionWithHtml: " + html);
    var selection = window.getSelection();
    if (!selection.rangeCount) {
        console.log("replaceSelectionWithHtml: no selection to replace");
        return;
    }

    console.log("replaceSelectionWithHtml: rangeCount = " + selection.rangeCount);

    if (selection.isCollapsed) {
        console.log("replaceSelectionWithHtml: the selection is collapsed");
        return;
    }

    document.execCommand("insertHTML", false, html);
}
