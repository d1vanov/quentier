function provideSrcForEnCryptImgTags(path) {
    var imgElements = [];
    imgElements = document.getElementsByTagName("img");
    for(var index = 0; index < imgElements.length; ++index) {
        var element = imgElements[index];
        if (!element.hasAttribute("en-tag")) {
            continue;
        }
        if (element.getAttribute("en-tag") != "en-crypt") {
            continue;
        }
        if (!element.hasAttribute("encrypted_text")) {
            continue;
        }
        if (element.hasAttribute("src")) {
            var srcAttr = element.getAttribute("src");
            if ((srcAttr != null) && (srcAttr != "")) {
                continue;
            }
        }
        // automatically escape special characters in the path
        path = path.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
        element.setAttribute("src", path);
        console.log("Set en-crypt tag's src to ", path);
    }
}
