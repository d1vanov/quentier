function provideSrcForEnCryptImgTags(path) {
    var imgElements = [];
    imgElements = document.getElementsByTagName("img");
    console.warn("found " + imgElements.length.toString() + " img elements in the document");
    for(var index = 0; index < imgElements.length; ++index) {
        var element = imgElements[index];
        if (!element.hasAttribute("en-tag")) {
            console.warn("skipping element without en-tag");
            continue;
        }
        if (element.getAttribute("en-tag") != "en-crypt") {
            console.warn("skipping non-en-crypt element");
            continue;
        }
        if (!element.hasAttribute("encrypted_text")) {
            console.warn("skipping element without encrypted_text attribute");
            continue;
        }
        if (element.hasAttribute("src")) {
            var srcAttr = element.getAttribute("src");
            if ((srcAttr != null) && (srcAttr != "")) {
                console.warn("skipping element with already set src: " + srcAttr);
                continue;
            }
        }
        // automatically escape special characters in the path
        path = path.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
        element.setAttribute("src", path);
        console.warn("Set en-crypt tag's src to " + path);
    }
}
