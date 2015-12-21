function updateImageResourceSrc(hash, newSrc) {
    console.log("updateImageResourceSrc: hash = " + hash + ", new src = " + newSrc);

    var resource = document.querySelector("[hash=\"" + hash + "\"]");
    if (!resource) {
        console.warn("can't find the image resource to update the src for");
        return;
    }

    var escapedPath = newSrc.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
    resource.setAttribute("src", escapedPath);
}
