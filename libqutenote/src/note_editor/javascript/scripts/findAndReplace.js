function findAndReplace() {
    var undoReplaceAnchorNodes = [];
    var undoReplaceSelectionCharacterRanges = [];
    var undoReplaceParams = [];

    var redoReplaceAnchorNodes = [];
    var redoReplaceSelectionCharacterRanges = [];
    var redoReplaceParams = [];

    this.replace = function(textToReplace, replacementText, matchCase) {
        console.log("replace: text to replace = " + textToReplace +
                    "; replacement text = " + replacementText + "; match case = " + matchCase);

        var currentlySelectedText = getSelectionHtml();

        var tmp = document.createElement("div");
        tmp.innerHTML = currentlySelectedText;
        currentlySelectedText = tmp.textContent || "";

        console.log("Currently selected text = " + currentlySelectedText);

        var currentSelectionMatches = false;
        if (matchCase) {
            currentSelectionMatches = (textToReplace === currentlySelectedText);
        }
        else {
            currentSelectionMatches = (textToReplace.toUpperCase() === currentlySelectedText.toUpperCase());
        }

        console.log("replace: current selection matches = " + currentSelectionMatches);

        if (!currentSelectionMatches) {
            var found = window.find(textToReplace, matchCase, false, true);
            if (!found) {
                console.log("replace: can't find the text to replace");
                return false;
            }
        }

        var rangySelection = rangy.getSelection();
        if (rangySelection === undefined) {
            console.warn("replace: can't get rangy selection");
            return false;
        }

        var selection = window.getSelection();
        var anchorNode = selection.anchorNode;
        var selectionRanges = rangySelection.saveCharacterRanges(anchorNode);

        var res = replaceSelectionWithHtml(replacementText);
        if (!res) {
            console.warn("replace: replaceSelectionWithHtml failed");
            return false;
        }

        undoReplaceAnchorNodes.push(anchorNode);
        undoReplaceSelectionCharacterRanges.push(selectionRanges);
        undoReplaceParams.push({ originalText: textToReplace, replacement: replacementText, matchCaseOption: matchCase });

        return true;
    }

    this.undo = function() {
        return this.undoRedoImpl(undoReplaceAnchorNodes, undoReplaceSelectionCharacterRanges, undoReplaceParams,
                                 redoReplaceAnchorNodes, redoReplaceSelectionCharacterRanges, redoReplaceParams, true);
    }

    this.redo = function() {
        return this.undoRedoImpl(redoReplaceAnchorNodes, redoReplaceSelectionCharacterRanges, redoReplaceParams,
                                 undoReplaceAnchorNodes, undoReplaceSelectionCharacterRanges, undoReplaceParams, false);
    }

    this.undoRedoImpl = function(sourceAnchorNodes, sourceSelectionCharacterRanges, sourceParams,
                                 destAnchorNodes, destSelectionCharacterRanges, destParams,
                                 performingUndo) {
        console.log("findAndReplace.undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceAnchorNodes) {
            console.warn("Can't " + actionString + " the replacement: no anchor nodes helper array");
            return false;
        }

        if (!sourceSelectionCharacterRanges) {
            console.warn("Can't " + actionString + " the replacement: no selection character ranges helper array");
            return false;
        }

        if (!sourceParams) {
            console.warn("Can't " + actionString + " the replacement: no params helper array");
            return false;
        }

        var anchorNode = sourceAnchorNodes.pop();
        var selectionCharacterRange = sourceSelectionCharacterRanges.pop();
        var params = sourceParams.pop();

        if ((anchorNode === undefined) || (selectionCharacterRange === undefined) || (params === undefined) ||
            (params.originalText === undefined) || (params.replacement === undefined) ||
            (params.matchCaseOption === undefined)) {
            console.warn("Can't " + actionString + " the replacement: some required data is undefined");
            return false;
        }

        // 1) Restore the selection
        var rangySelection = rangy.getSelection();
        if (rangySelection === undefined) {
            console.warn("Can't " + actionString + " the replacement: rangy selection is undefined");
            return false;
        }

        rangySelection.restoreCharacterRanges(anchorNode, selectionCharacterRange);

        // 2) Adjust the end position of the selection to match the replacement text's length
        var selection = window.getSelection();
        if (!selection.rangeCount) {
            console.warn("Can't " + actionString + " the replacement: no selection after restoring the character ranges");
            return false;
        }

        var range = selection.getRangeAt(0).cloneRange();
        var rangeRefinedEndOffset;
        if (performingUndo) {
            rangeRefinedEndOffset = range.endOffset + params.replacement.length - params.originalText.length;
        }
        else {
            rangeRefinedEndOffset = range.endOffset + params.originalText.length - params.replacement.length;
        }
        console.log("Previous range's end offset: " + range.endOffset + ", original text length: " + params.originalText.length +
                    ", replacement text length: " + params.replacement.length + "; new range end offset: " + rangeRefinedEndOffset);

        range.setEnd(selection.focusNode, rangeRefinedEndOffset);
        selection.removeAllRanges();
        selection.addRange(range);

        rangySelection = rangy.getSelection();
        if (rangySelection === undefined) {
            console.warn("Can't preserve some information after " + actionString + "ing the replacement: rangy selection is undefined");
            return false;
        }

        var savedCharacterRanges = rangySelection.saveCharacterRanges(selection.anchorNode);

        // 3) Replace the text in the adjusted selection
        replaceSelectionWithHtml((performingUndo ? params.originalText : params.replacement));

        destAnchorNodes.push(selection.anchorNode);
        destSelectionCharacterRanges.push(savedCharacterRanges);
        destParams.push(params);
    }
}

(function() {
    window.Replacer = new findAndReplace();
})();
