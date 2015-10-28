function setHyperlinkToSelection(link) {
    console.log("setHyperlinkToSelection: " + link);

    var element = findSelectedHyperlinkElement();
    if (element) {
        console.log("Found link node within the selection, " +
                    "will use its text as link text");
        element.href = link;
        return;
    }

    console.log("Found no link node within the selection, will use " +
                "selection converted to string as link text");
    var selectedText = window.getSelection().toString();
    if (!selectedText) {
        console.log("Selection converted to string yields empty string, " +
                    "will use the link itself as link text");
        selectedText = link;
    }

    replaceSelectionWithHtml('<a href="' + link + '">' + selectedText + "</a>");
}
