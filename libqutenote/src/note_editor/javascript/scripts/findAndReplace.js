function findAndReplace(textToReplace, replacementText, matchCase) {
    console.log("findAndReplace: text to replace = " + textToReplace +
                "; replacement text = " + replacementText + "; match case = " + matchCase);

    this.undo = function() {
        console.log("findAndReplace::undo");

        if (!this.replaceAnchorNodes) {
            return false;
        }

        if (!this.replaceSelectionCharacterRanges) {
            return false;
        }

        if (!this.replaceParams) {
            return false;
        }

        var anchorNode = this.replaceAnchorNodes.pop();
        var selectionCharacterRange = this.replaceSelectionCharacterRanges.pop();
        var replaceParams = this.replaceParams.pop();

        if ((anchorNode === undefined) || (selectionCharacterRange === undefined) || (replaceParams === undefined) ||
            (replaceParams.originalText === undefined) || (replaceParams.replacement === undefined) || (replaceParams.matchCaseOption === undefined)) {
            return false;
        }

        // 1) Restore the selection
        var rangySelection = rangy.getSelection();
        if (rangySelection === undefined) {
            return false;
        }
        rangySelection.restoreCharacterRanges(anchorNode, selectionCharacterRange);

        // 2) Find the replacement text (i.e. adjust the restored selection to match another word)
        var found = window.find(replaceParams.replacement, replaceParams.matchCaseOpton, false, false);
        if (!found) {
            return false;
        }

        // 3) Replace the text back to the original one in the adjusted selection
        replaceSelectionWithHtml(replaceParams.originalText);

        return true;
    }

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

    console.log("findAndReplace: current selection matches = " + currentSelectionMatches);

    if (!currentSelectionMatches) {
        var found = window.find(textToReplace, matchCase, false, true);
        if (!found) {
            console.log("findAndReplace: can't find the text to replace");
            return false;
        }
    }

    var rangySelection = rangy.getSelection();
    if (rangySelection === undefined) {
        return false;
    }

    var selection = window.getSelection();
    var anchorNode = selection.anchorNode;
    var selectionRanges = rangySelection.saveCharacterRanges(anchorNode);

    var res = replaceSelectionWithHtml(replacementText);
    if (!res) {
        return false;
    }

    if (!this.replaceAnchorNodes) {
        this.replaceAnchorNodes = [];
    }
    this.replaceAnchorNodes.push(anchorNode);

    if (!this.replaceSelectionCharacterRanges) {
        this.replaceSelectionCharacterRanges = [];
    }
    this.replaceSelectionCharacterRanges.push(selectionRanges);

    if (!this.replaceParams) {
        this.replaceParams = [];
    }
    this.replaceParams.push({ originalText: textToReplace, replacement: replacementText, matchCaseOption: matchCase });

    return true;
}
