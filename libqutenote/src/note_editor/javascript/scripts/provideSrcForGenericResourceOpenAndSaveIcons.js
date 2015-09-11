function provideSrcForGenericResourceOpenAndSaveIcons() {
    console.log("provideSrcForGenericResourceOpenAndSaveIcons()");
    if (!window.hasOwnProperty('iconThemeHandler')) {
        console.log("iconThemeHandler global variable is not defined");
        return;
    }

    iconThemeHandler.onIconFilePathForIconThemeNameRequest("document-open");
    iconThemeHandler.onIconFilePathForIconThemeNameRequest("document-save-as");
}
