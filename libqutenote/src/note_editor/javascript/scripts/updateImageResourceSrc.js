function updateImageResourceSrc(hash, newSrc) {
    console.log("updateImageResourceSrc: hash = " + hash + ", new src = " + newSrc);

    var resource = document.querySelector("[hash=\"" + hash + "\"]");
    if (!resource) {
        console.warn("can't find the image resource to update the src for");
        return;
    }

    var escapedPath = newSrc.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');

    observer.stop();

    try {
        resizableImageManager.destroyResizable(resource);
        resource.setAttribute("height", resource.naturalHeight);
        resource.setAttribute("width", resource.naturalWidth);
        resource.setAttribute("src", escapedPath);
        resizableImageManager.setResizable(resource);
    }
    finally {
        observer.start();
    }
}
