function onResourceLocalFilePathForHashReceived(resourceHash, filePath) {
    console.log("Response to the event of resource file path update for given data hash: hash = " +
                resourceHash.toString() + ", file path = " + filePath.toString());
    var resources = document.querySelectorAll("[hash=\"" + resourceHash.toString() + "\"]");
    var numResources = resources.length;
    if (!numResources) {
        return;
    }

    console.log("Found " + numResources + " resources with given hash");
    for(var index = 0; index < numResources; ++index) {
        // automatically escape special characters in the path
        var resourceLocalFilePath = filePath.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
        resources[index].setAttribute("src", resourceLocalFilePath);
        console.log("Set src to", resourceLocalFilePath, "for resource with hash", resourceHash);
    }
}
