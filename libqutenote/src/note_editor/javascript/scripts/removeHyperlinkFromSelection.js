function removeHyperlinkFromSelection() {
    console.log("removeHyperlinkFromSelection");

    var element = findSelectedHyperlinkElement();
    if (!element) {
        return;
    }

    $(element).contents().unwrap();
}
