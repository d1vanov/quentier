function getHyperlinkFromSelection() {
    console.log("getHyperlinkFromSelection");

    var element = findSelectedHyperlinkElement();
    if (!element) {
        return;
    }

    return element.href;
}
