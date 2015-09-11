function provideGenericResourceDisplayNameAndSize() {
    console.log("provideGenericResourceDisplayNameAndSize()");
    if (!window.hasOwnProperty('resourceCache')) {
        console.log("resourceCache global variable is not defined");
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

        resourceCache.findResourceInfo(hash);
        console.log("Requested resource info for hash " + hash);
    }
}
