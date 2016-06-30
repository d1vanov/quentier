function updateResourceHash(oldHash, newHash) {
    console.log("updateResourceHash: old hash = " + oldHash + ", new hash = " + newHash);

    var resource = document.querySelector("[hash=\"" + oldHash + "\"]");
    if (!resource) {
        console.warn("can't find the resource with the old hash to update it");
        return;
    }

    resource.setAttribute("hash", newHash);

    if (window.hasOwnProperty('imageAreasHilitor')) {
        imageAreasHilitor.clearImageHilitors(oldHash);
    }
}
