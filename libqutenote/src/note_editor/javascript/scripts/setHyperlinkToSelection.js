function setHyperlinkToSelection(link) {
    console.log("setHyperlinkToSelection: " + link);

    var element;
    var selection = window.getSelection();
    if (selection && selection.anchorNode && selection.anchorNode.parentNode) {
        element = selection.anchorNode.parentNode;
    }

    var foundLink = false;

    while(element) {
        if (Object.prototype.toString.call( element ) === '[object Array]') {
            console.log("Found array of elements");
            element = element[0];
            if (!element) {
                console.log("First element of the array is null");
                break;
            }
        }

        console.log("element.nodeType = " + element.nodeType);
        if (element.nodeType === 1) {
            console.log("Found element with nodeType == 1");

            if (element.nodeName === "A") {
                foundLink = true;
                console.log("Found link");
                break;
            }
        }

        element = element.parentNode;
        console.log("Checking the next parent");
    }

    var selectedText;
    if (foundLink) {
        console.log("Found link node within the selection, " +
                    "will use its text as link text");
        element.href = link;
        return;
    }
    else {
        console.log("Found no link node within the selection, will use " +
                    "selection converted to string as link text");
        selectedText = window.getSelection().toString();
        if (!selectedText) {
            console.log("Selection converted to string yields empty string, " +
                        "will use the link itself as link text");
            selectedText = link;
        }
    }

    replaceSelectionWithHtml('<a href="' + link + '">' + selectedText + "</a>");
}
