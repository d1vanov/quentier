function findAndReplace(textToReplace, replacementText, matchCase) {
    console.log("findAndReplace: text to replace = " + textToReplace +
                "; replacement text = " + replacementText + "; match case = " + matchCase);

    // TODO: replace it with getter for currently selected plain text
    var currentlySelectedText = getSelectionHtml();

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
            return;
        }
    }

    replaceSelectionWithHtml(replacementText);
}
