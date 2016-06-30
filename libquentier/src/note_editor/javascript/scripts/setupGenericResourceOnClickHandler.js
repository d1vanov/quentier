function setupGenericResourceOnClickHandler() {
    console.log("setupGenericResourceOnClickHandler");

    var genericResources = document.querySelectorAll(".en-media-generic");
    var numGenericResources = genericResources.length;
    console.log("Found " + numGenericResources + " generic resource tags");

    for(var index = 0; index < numGenericResources; ++index) {
        var resource = genericResources[index];
        resource.onclick = genericResourceOnClickHandler;
    }
}
