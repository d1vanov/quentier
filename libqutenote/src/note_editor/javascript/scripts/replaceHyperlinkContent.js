function replaceHyperlinkContent(hyperlinkId, link, text) {
    console.log("replaceHyperlinkContent: hyperlink id = " + hyperlinkId + ", link = " + link + ", text = " + text);

    var elements = document.querySelectorAll("[en-hyperlink-id='" + hyperlinkId + "']");
    var numElements = elements.length;
    if (numElements != 1) {
        console.error("Unexpected number of found hyperlink tags: " + numElements);
        return;
    }

    var element = elements[0];
    element.setAttribute("href", link);
    element.innerHTML = text;
}
