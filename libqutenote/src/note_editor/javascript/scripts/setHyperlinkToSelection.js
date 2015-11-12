function setHyperlinkToSelection(text, link, hyperlinkIdNumber) {
    console.log("setHyperlinkToSelection: text = " + text + "; link = " + link +
                "; id number = " + hyperlinkIdNumber);

    var element = findSelectedHyperlinkElement();
    if (element) {
        console.log("Found link node within the selection");
        element.href = link;
        if (text) {
            element.innerHtml = text;
        }
        return;
    }

    console.log("Found no link node within the selection");
    if (!text) {
        text = window.getSelection().toString();
        if (!text) {
            console.log("Selection converted to string yields empty string, " +
                        "will use the link itself as link text");
            text = link;
        }
    }

    replaceSelectionWithHtml('<a href="' + link + '" en-hyperlink-id="' +
                             hyperlinkIdNumber +'" >' + text + "</a>");
}
