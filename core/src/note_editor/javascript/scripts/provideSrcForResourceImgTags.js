function provideSrcForResourceImgTags() {
    var imgElements = [];
    imgElements = document.getElementsByTagName("img");
    for(var index = 0; index < imgElements.length; ++index) {
        var element = imgElements[index];
        if (!element.hasAttribute("en-tag")) {
            continue;
        }
        if (element.getAttribute("en-tag") != "en-media") {
            continue;
        }
        if (!element.hasAttribute("hash")) {
            continue;
        }
        if (element.hasAttribute("src")) {
            var srcAttr = element.getAttribute("src");
            if ((srcAttr != null) && (srcAttr != "")) {
                continue;
            }
        }
        var hash = element.getAttribute("hash");
        var resourceLocalFilePath = resourceCache.getResourceLocalFilePath(hash);
        if ((resourceLocalFilePath == null) || (resourceLocalFilePath == "")) {
            continue;
        }
        // automatically escape special characters in the path
        resourceLocalFilePath = resourceLocalFilePath.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
        element.setAttribute("src", resourceLocalFilePath);
        console.log("Set src to", resourceLocalFilePath, "for resource with hash", hash);
    }
}
