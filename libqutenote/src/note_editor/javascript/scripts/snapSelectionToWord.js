function snapSelectionToWord() {
    console.log("snapSelectionToWord");

    var sel = window.getSelection();
    if (!sel.isCollapsed) {
        return;
    }

    var rng2 = sel.getRangeAt(0);

    var startOffset = 0;
    for(var i = rng2.startOffset; i >= 0; i--) {
        if (rng2.startContainer.data[i].match(/\S/) != null) {
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
    for(var i = rng2.endOffset; i < rng2.endContainer.data.length; i++) {
        if (rng2.endContainer.data[i].match(/\S/)) {
            endOffset++;
        } 
        else {
            break;
        }
    }

    startOffset = rng2.startOffset - startOffset;
    startOffset = startOffset < 0 ? 0 : startOffset;

    endOffset = rng2.endOffset + endOffset;
    endOffset = endOffset >= rng2.endContainer.data.length ? rng2.endContainer.data.length - 1 : endOffset;

    rng2.setStart(rng2.startContainer, startOffset);
    rng2.setEnd(rng2.endContainer, endOffset);
    sel.removeAllRanges();
    sel.addRange(rng2);
}
