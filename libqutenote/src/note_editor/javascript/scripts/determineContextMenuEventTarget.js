function determineContextMenuEventTarget(contextMenuSequenceNumber, x, y) {
    console.log("determineContextMenuEventTarget: contextMenuSequenceNumber = " + 
                contextMenuSequenceNumber + ", x = " + x + ", y = " + y);

    if (!window.hasOwnProperty('contextMenuEventHandler')) {
        console.log("contextMenuEventHandler global variable is not defined");
        return;
    }

    var foundImageResource = false;
    var foundNonImageResource = false;
    var resourceHash = "";

    var foundEnCryptTag = false;
    var cipher = "";
    var encryptedText = "";
    var length = "";

    // get context menu event target
    var element = document.elementFromPoint(x, y);
    console.log("Context menu event target: " + $(element).html());

    while(element) {
        if (Object.prototype.toString.call(element) === '[object Array]') {
            console.log("Found array of elements");
            element = element[0];
            if (!element) {
                console.log("First element of the array is null");
                break;
            }
        }

        if (element.nodeType == 1) {
            console.log("Found element with nodeType == 1");
            var enTag = element.getAttribute("en-tag");
            console.log("enTag = " + enTag + ", node name = " + element.nodeName);
            if (enTag == "en-media") {
                console.log("Found tag with en-tag = en-media");
                if (element.nodeName == "IMG") {
                    foundImageResource = true;
                    resourceHash = element.getAttribute("hash");
                    console.log("Found image resource with hash " + resourceHash);
                    break;
                }
                else if ((element.nodeName == "DIV") || (element.nodeName == "OBJECT")) {
                    foundNonImageResource = true;
                    resourceHash = element.getAttribute("hash");
                    console.log("Found non-image resource with hash " + resourceHash);
                    break;
                }
            }
            else if (enTag == "en-crypt") {
                foundEnCryptTag = true;
                cipher = element.getAttribute("cipher");
                length = element.getAttribute("length");
                encryptedText = element.getAttribute("encrypted_text");
                console.log("Found en-crypt tag: encryptedText = " + encryptedText + 
                            ", cipher = " + cipher + ", length = " + length);
                break;
            }
        }

        element = element.parentNode;
        console.log("Checking the next parent");
    }

    if (foundImageResource) {
        contextMenuEventHandler.setContextMenuContent("ImageResource", contextMenuSequenceNumber);
    }
    else if (foundNonImageResource) {
        contextMenuEventHandler.setContextMenuContent("NonImageResource", contextMenuSequenceNumber);
    }
    else if (foundEnCryptTag) {
        contextMenuEventHandler.setContextMenuContent("EncryptedText", contextMenuSequenceNumber);
    }
    else {
        contextMenuEventHandler.setContextMenuContent("GenericText", contextMenuSequenceNumber);
    }
}
