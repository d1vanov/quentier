function removeHyperlink(enHyperlinkIndex) {
    console.log("removeHyperlink: " + enHyperlinkIndex);

    if (!enHyperlinkIndex) {
        console.warn("Can't remove hyperlink: invalid index");
        return;
    }

    var elements = document.querySelectorAll("[en-hyperlink-id='" + enHyperlinkIndex + "']");
    var numElements = elements.length;
    if (numElements !== 1) {
        console.error("Unexpected number of found hyperlink tags: " + numElements);
        return;
    }

    var element = elements[0];
    $(element).contents().unwrap();
}
