function provideSrcForGenericResourceIcons() {
    console.log("provideSrcForGenericResourceIcons()");
    if (!window.hasOwnProperty('mimeTypeIconHandler')) {
        console.log("mimeTypeIconHandler global variable is not defined");
        return;
    }

    var genericResourceIcons = document.querySelectorAll(".resource-icon");
    console.log("Found " + genericResourceIcons.length.toString() + " generic resource icon tags");
    for(var index = 0; index < genericResourceIcons.length; ++index)
    {
        var currentResourceIcon = genericResourceIcons[index];
        var mimeType = currentResourceIcon.getAttribute("type");
        if (!mimeType) {
            console.warn("Detected resource with undetermined mime type");
            continue;
        }

        mimeTypeIconHandler.onIconFilePathForMimeTypeRequest(mimeType);
    }
}
