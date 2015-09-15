function setupOpenResourceButtonOnClickHandler() {
    console.log("setupOpenResourceButtonOnClickHandler");
    if (!window.hasOwnProperty("openAndSaveResourceButtonsHandler")) {
        console.log("openAndSaveResourceButtonsHandler global variable is not defined");
        return;
    }

    var openResourceButtons = document.querySelectorAll(".open-resource-button");
    var numOpenResourceButtons = openResourceButtons.length;
    console.log("Found " + numOpenResourceButtons + " open resource button tags");
    for(var index = 0; index < numOpenResourceButtons; ++index) {
        var currentOpenResourceButton = openResourceButtons[index];

        currentOpenResourceButton.setAttribute("onclick", "console.log(\"Open resource button has been clicked\"); " +
                                               "var parentResourceElement = this.parentElement; " +
                                               "if (!parentResourceElement) { " +
                                               "console.warn(\"Found open resource button without parent resourceElement\"); " +
                                               "return; } " +
                                               "if (!parentResourceElement.hasAttribute(\'hash\')) { " +
                                               "console.warn(\"Found resource element without hash attribute\"); " +
                                               "return; } " +
                                               "window.openAndSaveResourceButtonsHandler.onOpenResourceButtonPressed(parentResourceElement.getAttribute(\'hash\'));");
    }
}
