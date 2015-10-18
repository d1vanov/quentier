function changeFontSizeForSelection(newFontSize) {
    console.log("changeFontSizeForSelection: new font size = " + newFontSize);

    var selection = window.getSelection();
    if (!selection) {
        console.log("selection is null");
        return;
    }

    console.log("selection.rangeCount = " + selection.rangeCount);

    // Wrap selection into span
    var element;
    var span = document.createElement("span");
    if (selection.rangeCount) {
        // clone the Range object
        var range = selection.getRangeAt(0).cloneRange();
        // get the node at the start of the range
        element = range.startContainer;
        // find the first parent that is a real HTML tag and not a text node
        while (element.nodeType != 1) element = element.parentNode;
        // place the marker before the node
        element.parentNode.insertBefore(span, element);
        console.log("inserted span before the element");
        // restore the selection
        selection.removeAllRanges();
        selection.addRange(range);
    }

    if (!element) {
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
    }

    var computedStyle = window.getComputedStyle(element);

    var dpi = window.logicalDpiY || 96;
    var currentFontSizeInPt = parseInt((parseFloat(computedStyle.fontSize * 72.0 / dpi).toFixed(2)));
    if (currentFontSizeInPt === newFontSize) {
        console.log("current font size is already equal to the new applied one");
        return;
    }

    $(element).css("font-size", "" + newFontSize + "pt");
    console.log("Set font size to: " + newFontSize + "pt");
}
