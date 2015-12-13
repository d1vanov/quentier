function removeResource(hash) {
    console.log("removeResource: " + hash);

    var element = document.querySelector("[hash='" + hash + "']");
    if (!element) {
        console.warn("Can't find the resource to be removed from the note content: hash = " + hash);
        return;
    }

    $(element).remove();
}
