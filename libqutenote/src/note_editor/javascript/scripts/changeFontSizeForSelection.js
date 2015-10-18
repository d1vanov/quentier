function changeFontSizeForSelection(newFontSize) {
    console.log("changeFontSizeForSelection: new font size = " + newFontSize);

    var selection = window.getSelection();
    if (!selection) {
        console.log("selection is null");
        return;
    }

    var anchorNode = selection.anchorNode;
    if (!anchorNode) {
        console.log("selection.anchorNode is null");
        return;
    }

    var element = anchorNode.parentNode;

    if (Object.prototype.toString.call( element ) === '[object Array]') {
        console.log("Found array of elements");
        element = element[0];
        if (!element) {
            console.log("First element of the array is null");
            return;
        }
    }

    while (element.nodeType != 1) {
        element = element.parentNode;
    }

    var computedStyle = window.getComputedStyle(element);

    var dpi = window.logicalDpiY || 96;
    var currentFontSizeInPt = parseInt((parseFloat(computedStyle.fontSize * 72.0 / dpi).toFixed(2)));
    if (currentFontSizeInPt === newFontSize) {
        console.log("current font size is already equal to the new applied one");
        return;
    }

    var selectedHtml = getSelectionHtml();
    document.execCommand('insertHtml', false, "<span style=\"font-size:" + newFontSize + "pt;\">" + selectedHtml + "</span>");
    console.log("Set font size to: " + newFontSize + "pt");
}
