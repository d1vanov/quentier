function onResourceInfoReceived(resourceHash, filePath, displayName, displaySize) {
    console.log("onResourceInfoReceived: hash = " + resourceHash +
                ", file path = " + filePath + ", name = " + displayName +
                ", size = " + displaySize);

    var resources = document.querySelectorAll("[hash=\"" + resourceHash + "\"]");
    if (!resources) {
        return;
    }

    var escapedPath = filePath.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');

    var resource = resources[0];
    resource.setAttribute("src", escapedPath);

    var resourceName = resource.getElementsByClassName("resource-name");
    if (resourceName && resourceName[0]) {
        resourceName[0].innerHtml = displayName;
        console.log("Set display name to " + displayName + " for element " + resourceName[0]);
    }
    else {
        console.log("Can't find child element for resource display name");
    }

    var resourceSize = resource.getElementsByClassName("resource-size");
    if (resourceSize && resourceSize[0]) {
        resourceSize[0].innerHtml = displaySize;
        console.log("Set display size to " + displaySize + " for element " + resourceSize[0]);
    }
    else {
        console.log("Can't find child element for resource display size");
    }
}
