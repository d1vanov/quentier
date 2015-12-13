function removeHyperlink(enHyperlinkIndex) {
    console.log("removeHyperlink: " + enHyperlinkIndex);

    if (!enHyperlinkIndex) {
        console.warn("Can't remove hyperlink: invalid index");
        return;
    }

    var element = document.querySelector("[en-hyperlink-id='" + enHyperlinkIndex + "']");
    if (element) {
        $(element).contents().unwrap();
    }
}
