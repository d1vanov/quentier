function provideSrcForResourceImgTags() {
    console.log("provideSrcForResourceImgTags()");
    if (!window.hasOwnProperty('resourceCache')) {
        console.log("resourceCache global variable is not defined");
        return;
    }

    var imgElements = document.getElementsByTagName("img");
    var numElements = imgElements.length;
    if (!numElements) {
        return;
    }

    console.log("Found " + numElements + " img tags");

    for(var index = 0; index < numElements; ++index) {
        var element = imgElements[index];
        if (element.getAttribute("en-tag") != "en-media") {
            console.log("Skipping non en-media tag: " + element.getAttribute("en-tag"));
            continue;
        }

        var srcAttr = element.getAttribute("src");
        if (srcAttr && srcAttr !== "") {
            console.log("Skipping tag with src attribute: " + srcAttr);
            continue;
        }

        var hash = element.getAttribute("hash");
        if (!hash) {
            console.log("Skipping img resource without hash attribute");
            continue;
        }

        resourceCache.findResourceInfo(hash);
        console.log("Requested resource info for hash " + hash);
    }
}
