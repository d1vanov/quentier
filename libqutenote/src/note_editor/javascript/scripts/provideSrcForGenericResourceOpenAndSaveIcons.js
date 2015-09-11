function provideSrcForGenericResourceOpenAndSaveIcons() {
    console.log("provideSrcForGenericResourceOpenAndSaveIcons()");
    if (!window.hasOwnProperty('iconThemeHandler')) {
        console.log("iconThemeHandler global variable is not defined");
        return;
    }

    iconThemeHandler.onIconFilePathForIconThemeNameRequest("document-open", 24, 24);
    iconThemeHandler.onIconFilePathForIconThemeNameRequest("document-save-as", 24, 24);
}
