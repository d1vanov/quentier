function findAndReplace(textToReplace, replacementText, matchCase) {
    console.log("findAndReplace: text to replace = " + textToReplace +
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
