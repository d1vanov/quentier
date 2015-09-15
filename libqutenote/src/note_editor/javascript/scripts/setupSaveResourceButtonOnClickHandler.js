function setupSaveResourceButtonOnClickHandler() {
    console.log("setupSaveResourceButtonOnClickHandler");
    if (!window.hasOwnProperty("openAndSaveResourceButtonsHandler")) {
        console.log("openAndSaveResourceButtonsHandler global variable is not defined");
        return;
    }

    var saveResourceButtons = document.querySelectorAll(".save-resource-button");
    var numSaveResourceButtons = saveResourceButtons.length;
    console.log("Found " + numSaveResourceButtons + " save resource button tags");
    for(var index = 0; index < numSaveResourceButtons; ++index) {
        var currentSaveResourceButton = saveResourceButtons[index];

        currentSaveResourceButton.setAttribute("onclick", "console.log(\"Save resource button has been clicked\"); " +
                                               "var parentResourceElement = this.parentElement; " +
                                               "if (!parentResourceElement) { " +
                                               "console.warn(\"Found save resource button without parent resourceElement\"); " +
                                               "return; } " +
                                               "if (!parentResourceElement.hasAttribute(\'hash\')) { " +
                                               "console.warn(\"Found resource element without hash attribute\"); " +
                                               "return; } " +
                                               "window.openAndSaveResourceButtonsHandler.onSaveResourceButtonPressed(parentResourceElement.getAttribute(\'hash\'));");
    }
}
