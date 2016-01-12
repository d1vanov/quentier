function replaceSelectionWithHtml(html, simpleString) {
    console.log("replaceSelectionWithHtml: " + html);
    var range, html;
    if (window.getSelection && window.getSelection().getRangeAt) {
        var selection = window.getSelection();
        if (!selection.rangeCount) {
            console.log("replaceSelectionWithHtml: no selection to replace");
            return;
        }
        range = selection.getRangeAt(0);
        range.deleteContents();

        if (!simpleString) {
            console.log("replaceSelectionWithHtml: inserting some html markup");
            var div = document.createElement("div");
            div.innerHTML = html;
            var frag = document.createDocumentFragment(), child;
            while ( (child = div.firstChild) ) {
                frag.appendChild(child);
            }
            range.insertNode(frag);
        }
        else {
            console.log("replaceSelectionWithHtml: inserting plain text");
            range.insertNode(document.createTextNode(html));
        }
    } else if (document.selection && document.selection.createRange) {
        range = document.selection.createRange();
        range.pasteHTML(html);
    }
}
