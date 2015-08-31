function provideSrcForResourceImgTags() {
    var imgElements = document.getElementsByTagName("img");
    var numElements = imgElements.length;
    console.log("Found " + numElements + " img tags");
    for(var index = 0; index < numElements; ++index) {
        var element = imgElements[index];
        if (element.getAttribute("en-tag") != "en-media") {
            console.log("Skipping non en-media tag: " + element.getAttribute("en-tag"));
            continue;
        }

        var srcAttr = element.getAttribute("src");
        if (srcAttr && srcAttr != "") {
            console.log("Skipping tag with src attribute: " + srcAttr);
            continue;
        }

        var hash = element.getAttribute("hash");
        if (!hash) {
            element.getAttribute("Skipping img resource without hash attribute");
            continue;
        }

        resourceCache.getResourceLocalFilePath(hash);
        console.log("Requested resource local file path for hash " + hash);
    }
}
