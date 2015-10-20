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

    var scaledFontSize = parseFloat(computedStyle.fontSize) * 3.0 / 4;
    console.log("Scaled font size = " + scaledFontSize);
    var convertedFontSize = parseInt(Math.round(scaledFontSize));
    if (convertedFontSize === newFontSize) {
        console.log("current font size is already equal to the new applied one");
        return;
    }

    var selectedText = "";
    if (selection.rangeCount) {
        var container = document.createElement("div");
        for (var i = 0, len = selection.rangeCount; i < len; ++i) {
            container.appendChild(selection.getRangeAt(i).cloneContents());
        }
        selectedText = container.textContent;
    }

    document.execCommand('insertHtml', false, "<span style=\"font-size:" + newFontSize + "pt;\">" + selectedText + "</span>");
    console.log("Set font size to: " + newFontSize + "pt");
}
