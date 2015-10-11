function provideSrcForGenericResourceImages() {
    console.log("provideSrcForGenericResourceImages");
    if (!window.hasOwnProperty('genericResourceImageHandler')) {
        console.log("genericResourceImageHandler global variable is not defined");
        return;
    }

    var genericResources = document.querySelectorAll(".en-media-generic");
    var numGenericResources = genericResources.length;
    console.log("Found " + numGenericResources + " generic resource tags");

    for(var index = 0; index < numGenericResources; ++index) {
        var resource = genericResources[index];

        var hash = resource.getAttribute("hash");
        if (!hash) {
            console.log("Skipping resource without hash attribute");
            continue;
        }

        genericResourceImageHandler.findGenericResourceImage(hash);
        console.log("Requested generic resource image for generic resource with hash " + hash);
    }
}
