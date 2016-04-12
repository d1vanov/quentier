function updateImageResourceSrc(hash, newSrc) {
    console.log("updateImageResourceSrc: hash = " + hash + ", new src = " + newSrc);

    var resource = document.querySelector("[hash=\"" + hash + "\"]");
    if (!resource) {
        console.warn("can't find the image resource to update the src for");
        return;
    }

    var isImageResource = ((' ' + resource.className + ' ').indexOf(' en-media-image ') > -1);

    var escapedPath = newSrc.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');

    observer.stop();

    try {
        if (isImageResource) {
            try {
                resizableImageManager.destroyResizable(resource);
            }
            catch(e) {
                console.warn("ResourceImageManager::destroyResizable failed for image resource: " +
                             e.name + ":" + e.message + "\n" + e.stack);
            }
        }

        resource.setAttribute("height", resource.naturalHeight);
        resource.setAttribute("width", resource.naturalWidth);
        resource.setAttribute("src", escapedPath);

        if (isImageResource) {
            try {
                resizableImageManager.setResizable(resource);
            }
            catch(e) {
                console.warn("ResourceImageManager::setResizable failed for image resource: " +
                             e.name + ":" + e.message + "\n" + e.stack);
            }
        }
    }
    finally {
        observer.start();
    }
}
