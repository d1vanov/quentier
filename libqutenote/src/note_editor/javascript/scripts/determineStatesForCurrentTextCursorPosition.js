function determineStatesForCurrentTextCursorPosition() {
    console.log("determineStatesForCurrentTextCursorPosition");
    if (!window.hasOwnProperty('textCursorPositionHandler')) {
        console.log("textCursorPositionHandler global variable is not defined");
        return;
    }

    var selection = window.getSelection();
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
            element = element[0];
            if (!element) {
                break;
            }
        }

        if (element.nodeType == 1) {
            console.log("Testing for resource...");
            if (element.getAttribute("en-tag") == "en-media") {
                if (element.nodeName == "img") {
                    foundImageResource = true;
                    resourceHash = element.getAttribute("hash");
                    break;
                }
                else if (element.nodeName == "div") {
                    foundNonImageResource = true;
                    resourceHash = element.getAttribute("hash");
                    break;
                }
            }

            console.log("Testing for en-crypt tag...");
            if (element.getAttribute("en-tag") == "en-crypt") {
                foundEnCryptTag = true;
                cipher = element.getAttribute("cipher");
                length = element.getAttribute("length");
                encryptedText = element.getAttribute("encrypted_text");
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
    }

    textCursorPositionHandler.setOnImageResourceState(foundImageResource, resourceHash);
    textCursorPositionHandler.setOnNonImageResourceState(foundNonImageResource, resourceHash);
    textCursorPositionHandler.setOnEnCryptTagState(foundEnCryptTag, encryptedText, cipher, length);

    if (foundImageResource || foundNonImageResource || foundEnCryptTag) {
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
}
