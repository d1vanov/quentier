function findSelectedHyperlinkId() {
    var element = findSelectedHyperlinkElement();
    if (!element) {
        return;
    }

    return element.getAttribute("en-hyperlink-id");
}
