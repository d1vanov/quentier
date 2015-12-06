function getHyperlinkFromSelection() {
    console.log("getHyperlinkFromSelection");

    var element = findSelectedHyperlinkElement();
    if (!element) {
        console.log("Haven't found hyperlink under selection");
        var text = getSelectionHtml();
        return [text, "", "0"];
    }

    var idNumber = element.getAttribute('en-hyperlink-id');
    if (!idNumber) {
        idNumber = 0;
    }

    console.log("Found hyperlink under selection, returning [" + element.innerHTML +
                ", " + element.href + ", " + idNumber + "]");
    return [element.innerHTML, element.href, idNumber.toString()];
}
