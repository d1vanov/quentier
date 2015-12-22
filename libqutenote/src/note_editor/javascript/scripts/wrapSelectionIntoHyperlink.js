function wrapSelectionIntoHyperlink(link, hyperlinkId) {
    console.log("wrapSelectionIntoHyperlink: link = " + link + ", hyperlink id = " + hyperlinkId);

    if (!link) {
        return;
    }

    var html = getSelectionHtml();
    if (!html) {
        html = link;
    }
    else {
        var tmp = document.createElement("DIV");
        tmp.innerHTML = html;
        html = tmp.textContent || text.innerText || "";
        $(tmp).remove();
    }

    replaceSelectionWithHtml("<a href=\"" + link + "\" title=\"" + link + "\" en-hyperlink-id=\"" + hyperlinkId + "\">" + html + "</a>");
}
