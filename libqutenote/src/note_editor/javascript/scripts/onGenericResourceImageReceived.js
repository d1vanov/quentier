function onGenericResourceImageReceived(resourceHash, imageFilePath) {
    console.log("onGenericResourceImageReceived: resource hash = " + resourceHash + ", image file path = " + imageFilePath);

    var resources = document.querySelectorAll(".en-media-generic[hash=\"" + resourceHash + "\"]");
    if (!resources) {
        return;
    }

    var resource = resources[0];
    if (!resource) {
        return;
    }

    var escapedPath = imageFilePath.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
    resource.setAttribute("src", escapedPath);
    console.log("Successfully set image file path " + escapedPath + " for generic resource with hash " + resourceHash);
}
