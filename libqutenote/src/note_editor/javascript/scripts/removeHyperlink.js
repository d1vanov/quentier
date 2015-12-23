function removeHyperlink(enHyperlinkIndex, completely) {
    console.log("removeHyperlink: " + enHyperlinkIndex + ", completely = " + completely);

    if (!enHyperlinkIndex) {
        console.warn("Can't remove hyperlink: invalid index");
        return;
    }

    var element = document.querySelector("[en-hyperlink-id='" + enHyperlinkIndex + "']");
    if (element)
    {
        if (completely) {
            $(element).remove();
        }
        else {
            $(element).contents().unwrap();
        }
    }
}
