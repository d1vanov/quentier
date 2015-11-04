function snapSelectionToWord() {
    console.log("snapSelectionToWord");

    var sel = window.getSelection();
    if (!sel || !sel.isCollapsed) {
        return;
    }

    if (!sel.rangeCount) {
        return;
    }

    var rng = sel.getRangeAt(0);
    if (!rng || !rng.startContainer || !rng.startContainer.data) {
        return;
    }

    var startOffset = 0;
    for(var i = rng.startOffset; i >= 0; i--) {
        var str = rng.startContainer.data[i];
        if (!str) {
            return;
        }

        if (str.match(/\S/) != null) {
            startOffset++;
        } 
        else {
            break;
        }
    }
    
    if (startOffset > 0) {
        // Move one character forward from the leftmost whitespace
        startOffset--;
    }

    var endOffset = 0;
    for(var i = rng.endOffset; i < rng.endContainer.data.length; i++) {
        var str = rng.startContainer.data[i];
        if (!str) {
            return;
        }

        if (str.match(/\S/)) {
            endOffset++;
        } 
        else {
            break;
        }
    }

    startOffset = rng.startOffset - startOffset;
    startOffset = startOffset < 0 ? 0 : startOffset;

    endOffset = rng.endOffset + endOffset;
    endOffset = endOffset > rng.endContainer.data.length ? rng.endContainer.data.length : endOffset;

    rng.setStart(rng.startContainer, startOffset);
    rng.setEnd(rng.endContainer, endOffset);
    sel.removeAllRanges();
    sel.addRange(rng);
}
