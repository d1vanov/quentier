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
    var hint = "";
    var insideDecryptedTextFragment = false;

    var extraData = [];

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
            var type = element.getAttribute("type");
            console.log("enTag = " + enTag + ", node name = " + element.nodeName);
            if (enTag == "en-media") {
                console.log("Found tag with en-tag = en-media");
                if (type && (type.slice(0,5) == "image")) {
                    foundImageResource = true;
                    resourceHash = element.getAttribute("hash");
                    extraData.push(resourceHash);
                    console.log("Found image resource with hash " + resourceHash);
                    break;
                }
                else if (type) {
                    foundNonImageResource = true;
                    resourceHash = element.getAttribute("hash");
                    extraData.push(resourceHash);
                    console.log("Found non-image resource with hash " + resourceHash);
                    break;
                }
            }
            else if (enTag == "en-crypt") {
                foundEnCryptTag = true;
                cipher = element.getAttribute("cipher");
                length = element.getAttribute("length");
                encryptedText = element.getAttribute("encrypted_text");
                hint = element.getAttribute("hint");
                extraData.push(cipher);
                extraData.push(length);
                extraData.push(encryptedText);
                extraData.push(hint);
                console.log("Found en-crypt tag: encryptedText = " + encryptedText +
                            ", cipher = " + cipher + ", length = " + length << ", hint = " + hint);
                break;
            }
            else if (enTag == "en-decrypted") {
                insideDecryptedTextFragment = true;
                console.log("Found decrypted text fragment");
                break;
            }
        }

        element = element.parentNode;
        console.log("Checking the next parent");
    }

    if (foundImageResource) {
        contextMenuEventHandler.setContextMenuContent("ImageResource", "", insideDecryptedTextFragment, extraData, contextMenuSequenceNumber);
    }
    else if (foundNonImageResource) {
        contextMenuEventHandler.setContextMenuContent("NonImageResource", "", insideDecryptedTextFragment, extraData, contextMenuSequenceNumber);
    }
    else if (foundEnCryptTag) {
        contextMenuEventHandler.setContextMenuContent("EncryptedText", "", insideDecryptedTextFragment, extraData, contextMenuSequenceNumber);
    }
    else {
        var selectedHtml = getSelectionHtml();
        if (!selectedHtml) {
            snapSelectionToWord();
            selectedHtml = getSelectionHtml();
        }

        contextMenuEventHandler.setContextMenuContent("GenericText", selectedHtml, insideDecryptedTextFragment, extraData, contextMenuSequenceNumber);
    }
}
