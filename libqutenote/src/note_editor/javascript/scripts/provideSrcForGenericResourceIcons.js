(function(){
    var genericResourceIcons = document.querySelectorAll(".resource-icon");
    console.log("Found " + genericResourceIcons.length.toString() + " generic resource icon tags");
    for(var i = 0; i < genericResourceIcons.length; ++i)
    {
        var currentResourceIcon = genericResourceIcons[i];
        var mimeType = currentResourceIcon.getAttribute("type");
        if (!mimeType) {
            console.log("Detected resource with undetermined mime type");
            continue;
        }

        mimeTypeIconHandler.iconFilePathForMimeType(mimeType.toString());
    }
})();
