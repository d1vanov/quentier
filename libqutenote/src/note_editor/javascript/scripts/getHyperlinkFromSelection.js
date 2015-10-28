function getHyperlinkFromSelection() {
    console.log("getHyperlinkFromSelection");

    var element = findSelectedHyperlinkElement();
    if (!element) {
        console.log("Haven't found hyperlink under selection");
        var text = getSelectionHtml();
        return [text, ""];
    }

    console.log("Found hyperlink under selection, returning [" + element.innerHTML + ", " + element.href + "]");
    return [element.innerHTML, element.href];
}
