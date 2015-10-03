function determineStatesForCurrentTextCursorPosition(contextMenuSequenceNumber) {
    console.log("determineStatesForCurrentTextCursorPosition, context menu sequence number = " + 
                (contextMenuSequenceNumber ? contextMenuSequenceNumber.toString() : "null"));
    if (!window.hasOwnProperty('textCursorPositionHandler')) {
        console.log("textCursorPositionHandler global variable is not defined");
        return;
    }

    var selection = window.getSelection();
    if (!selection) {
        console.log("selection is null");
        return;
    }

    if (!selection.anchorNode) {
        console.log("selection.anchorNode is null");
        return;
    }

    var element = selection.anchorNode.parentNode;

    var foundBold = false;
    var foundItalic = false;
    var foundUnderline = false;
    var foundStrikethrough = false;
    
    var foundAlignLeft = false;
    var foundAlignCenter = false;
    var foundAlignRight = false;

    var foundOrderedList = false;
    var foundUnorderedList = false;

    var foundTable = false;

    var foundImageResource = false;
    var foundNonImageResource = false;
    var resourceHash = "";

    var foundEnCryptTag = false;
    var cipher = "";
    var encryptedText = "";
    var length = "";

    var textAlign;

    while(element) {
        if( Object.prototype.toString.call( element ) === '[object Array]' ) {
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
                else if (element.nodeName == "DIV") {
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

        if (element.nodeName == "B") {
            foundBold = true;
        }
        else if (element.nodeName == "I") {
            foundItalic = true;
        }
        else if (element.nodeName == "U") {
            foundUnderline = true;
        }
        else if ((element.nodeName == "S") ||
                 (element.nodeName == "DEL") || 
                 (element.nodeName == "STRIKE")) {
            foundStrikethrough = true;
        }
        else if ((element.nodeName == "OL") && !foundUnorderedList) {
            foundOrderedList = true;
        }
        else if ((element.nodeName == "UL") && !foundOrderedList) {
            foundUnorderedList = true;
        }
        else if (element.nodeName == "TBODY") {
            foundTable = true;
        }
 
        if (!foundAlignLeft && !foundAlignCenter && !foundAlignRight)
        {
            textAlign = element.style.textAlign;
            if (textAlign) {
                if (textAlign == "left") {
                    foundAlignLeft = true;
                }
                else if (textAlign == "center") {
                    foundAlignCenter = true;
                }
                else if (textAlign == "right") {
                    foundAlignRight = true;
                }
            }
            else {
                foundAlignLeft = true;
            }
        }

        element = element.parentNode;
        console.log("Checking the next parent");
    }

    textCursorPositionHandler.setOnImageResourceState(foundImageResource, resourceHash);
    textCursorPositionHandler.setOnNonImageResourceState(foundNonImageResource, resourceHash);
    textCursorPositionHandler.setOnEnCryptTagState(foundEnCryptTag, encryptedText, cipher, length);

    if (contextMenuSequenceNumber) {
        if (foundImageResource) {
            contextMenuEventHandler.setContextMenuContent("ImageResource", contextMenuSequenceNumber);
        }
        else if (foundNonImageResource) {
            contextMenuEventHandler.setContextMenuContent("NonImageResource", contextMenuSequenceNumber);
        }
        else if (foundEnCryptTag) {
            contextMenuEventHandler.setContextMenuContent("EncryptedText", contextMenuSequenceNumber);
        }
    }

    if (foundImageResource || foundNonImageResource || foundEnCryptTag) {
        console.log("foundImageResource = " + (foundImageResource ? "true" : "false") +
                    ", foundNonImageResource = " + (foundNonImageResource ? "true" : "false") +
                    ", foundEnCryptTag = " + (foundEnCryptTag ? "true" : "false"));
        return;
    }

    textCursorPositionHandler.setTextCursorPositionBoldState(foundBold);
    textCursorPositionHandler.setTextCursorPositionItalicState(foundItalic);
    textCursorPositionHandler.setTextCursorPositionUnderlineState(foundUnderline);
    textCursorPositionHandler.setTextCursorPositionStrikethroughState(foundStrikethrough);

    textCursorPositionHandler.setTextCursorPositionAlignLeftState(foundAlignLeft);
    textCursorPositionHandler.setTextCursorPositionAlignCenterState(foundAlignCenter);
    textCursorPositionHandler.setTextCursorPositionAlignRightState(foundAlignRight);

    textCursorPositionHandler.setTextCursorPositionInsideOrderedListState(foundOrderedList);
    textCursorPositionHandler.setTextCursorPositionInsideUnorderedListState(foundUnorderedList);

    textCursorPositionHandler.setTextCursorPositionInsideTableState(foundTable);

    if (contextMenuSequenceNumber) {
        contextMenuEventHandler.setContextMenuContent("GenericText", contextMenuSequenceNumber);
    }
}
