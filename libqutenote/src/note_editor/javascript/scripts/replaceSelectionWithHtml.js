function replaceSelectionWithHtml(html) {
    console.log("replaceSelectionWithHtml: " + html);
    var selection = window.getSelection();
    if (!selection.rangeCount) {
        console.log("replaceSelectionWithHtml: no selection to replace");
        return false;
    }

    console.log("replaceSelectionWithHtml: rangeCount = " + selection.rangeCount);

    if (selection.isCollapsed) {
        console.log("replaceSelectionWithHtml: the selection is collapsed");
        return false;
    }

    document.execCommand("insertHTML", false, html);
    return true;
}
