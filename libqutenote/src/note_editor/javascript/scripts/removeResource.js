function removeResource(resourceHash) {
    console.log("removeResource: " + resourceHash);

    var element = document.querySelector("[hash='" + resourceHash + "']");
    if (!element) {
        console.warn("Can't find the resource to be removed from the note content: hash = " + resourceHash);
        return;
    }

    $(element).remove();
}


