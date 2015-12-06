function getHyperlinkData(hyperlinkId) {
    console.log("getHyperlinkData: hyperlink id = " + hyperlinkId);

    var elements = document.querySelectorAll("[en-hyperlink-id='" + hyperlinkId + "']");
    var numElements = elements.length;
    if (numElements !== 1) {
        console.error("Unexpected number of found hyperlink tags: " + numElements);
        return;
    }

    var element = elements[0];

    console.log("Found hyperlink under selection, returning [" + element.innerHTML +
                ", " + element.href + "]");
    return [element.innerHTML, element.href];
}
