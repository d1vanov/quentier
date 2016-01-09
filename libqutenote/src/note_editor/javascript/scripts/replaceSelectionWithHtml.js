function replaceSelectionWithHtml(html) {
    console.log("replaceSelectionWithHtml: " + html);
    var range, html;
    if (window.getSelection && window.getSelection().getRangeAt) {
        var selection = window.getSelection();
        if (selection.rangeCount) {
            range = window.getSelection().getRangeAt(0);
            range.deleteContents();
        }
        else {
            range = new Range();
            selection.addRange(range);
        }
        var div = document.createElement("div");
        div.innerHTML = html;
        var frag = document.createDocumentFragment(), child;
        while ( (child = div.firstChild) ) {
            frag.appendChild(child);
        }
        range.insertNode(frag);
    } else if (document.selection && document.selection.createRange) {
        range = document.selection.createRange();
        range.pasteHTML(html);
    }
}
