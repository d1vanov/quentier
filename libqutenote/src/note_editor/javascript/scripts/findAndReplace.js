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
        console.log("findAndReplace::undo");

        if (!undoReplaceAnchorNodes) {
            console.warn("Can't undo the replacement: no undoReplaceAnchorNodes helper array");
            return false;
        }

        if (!undoReplaceSelectionCharacterRanges) {
            console.warn("Can't undo the replacement: no undoReplaceSelectionCharacterRanges helper array");
            return false;
        }

        if (!undoReplaceParams) {
            console.warn("Can't undo the replacement: no undoReplaceParams helper array");
            return false;
        }

        var anchorNode = undoReplaceAnchorNodes.pop();
        var selectionCharacterRange = undoReplaceSelectionCharacterRanges.pop();
        var params = undoReplaceParams.pop();

        if ((anchorNode === undefined) || (selectionCharacterRange === undefined) || (params === undefined) ||
            (params.originalText === undefined) || (params.replacement === undefined) ||
            (params.matchCaseOption === undefined)) {
            console.warn("Can't undo the replacement: some required data is undefined");
            return false;
        }

        // 1) Restore the selection
        var rangySelection = rangy.getSelection();
        if (rangySelection === undefined) {
            console.warn("Can't undo the replacement: rangy selection is undefined");
            return false;
        }

        rangySelection.restoreCharacterRanges(anchorNode, selectionCharacterRange);

        // 2) Adjust the end position of the selection to match the replacement text's length
        var selection = window.getSelection();
        if (!selection.rangeCount) {
            console.warn("Can't undo the replacement: no selection after restoring the character ranges");
            return false;
        }

        var range = selection.getRangeAt(0).cloneRange();
        var rangeRefinedEndOffset = range.endOffset + params.replacement.length - params.originalText.length;
        console.log("Previous range's end offset: " + range.endOffset + ", original text length: " + params.originalText.length +
                    ", replacement text length: " + params.replacement.length + "; new range end offset: " + rangeRefinedEndOffset);

        range.setEnd(selection.focusNode, rangeRefinedEndOffset);
        selection.removeAllRanges();
        selection.addRange(range);

        // 3) Replace the text back to the original one in the adjusted selection
        replaceSelectionWithHtml(params.originalText);

        rangySelection = rangy.getSelection();
        if (rangySelection === undefined) {
            console.warn("Can't preserve some information for redo after undo the replacement: rangy selection is undefined");
            return false;
        }

        redoReplaceAnchorNodes.push(selection.anchorNode);
        redoReplaceSelectionCharacterRanges.push(rangy.saveCharacterRanges(selection.anchorNode));
        redoReplaceParams.push(params);

        return true;
    }
}

(function() {
    window.Replacer = new findAndReplace();
})();
