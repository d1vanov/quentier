function onIconFilePathForIconThemeNameReceived(iconThemeName, iconFilePath) {
    console.log("onIconFilePathForIconThemeNameReceived: icon theme name = " + iconThemeName +
                ", file path = " + iconFilePath);

    var affectedElements = [];
    if (iconThemeName === "document-open") {
        affectedElements = document.querySelectorAll(".open-resource-button");
    }
    else if (iconThemeName === "document-save-as") {
        affectedElements = document.querySelectorAll(".save-resource-button");
    }
    else {
        console.warn("Hmm, don't see for which elements the icon theme name " + iconThemeName + " was requested");
        return;
    }

    var numAffectedElements = affectedElements.length;
    console.log("Found " + numAffectedElements + " elements using icon with theme name " + iconThemeName);

    for(var index = 0; index < numAffectedElements; ++index) {
        affectedElements[index].setAttribute("src", iconFilePath);
    }
}
