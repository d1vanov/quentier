function determineStatesForCurrentTextCursorPosition() {
    console.log("determineStatesForCurrentTextCursorPosition");

    if (!window.hasOwnProperty('textCursorPositionHandler')) {
        console.log("textCursorPositionHandler global variable is not defined");
        return;
    }

    var selection = window.getSelection();
    if (!selection) {
        console.log("selection is null");
        return;
    }

    var anchorNode = selection.anchorNode;
    if (!anchorNode) {
        console.log("selection.anchorNode is null");
        return;
    }

    var element = anchorNode.parentNode;

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
    var foundEnCryptTag = false;

    var textAlign;
    var firstElement = true;
    var style;

    while(element) {
        if (Object.prototype.toString.call( element ) === '[object Array]') {
            console.log("Found array of elements");
            element = element[0];
            if (!element) {
                console.log("First element of the array is null");
                break;
            }
        }

        if (firstElement) {
            var styleSource = (element.nodeType == 3 ? element.parentNode : element);
            style = window.getComputedStyle(styleSource);
            console.log("Got style for element " + $(styleSource).contents() + ": " + style);
        }

        if (element.nodeType == 1) {
            console.log("Found element with nodeType == 1");
            var enTag = element.getAttribute("en-tag");
            console.log("enTag = " + enTag + ", node name = " + element.nodeName);
            if (enTag == "en-media") {
                console.log("Found tag with en-tag = en-media");
                if (element.nodeName == "IMG") {
                    foundImageResource = true;
                    console.log("Found image resource");
                    break;
                }
                else if (element.nodeName == "DIV") {
                    foundNonImageResource = true;
                    console.log("Found non-image resource");
                    break;
                }
            }
            else if (enTag == "en-crypt") {
                foundEnCryptTag = true;
                console.log("Found en-crypt tag");
                break;
            }
        }

        if (element.nodeType == "B") {
            foundBold = true;
        }
        else if (element.nodeType == "I") {
            foundItalic = true;
        }
        else if (element.nodeType == "U") {
            foundUnderline = true;
        }
        else if ((element.nodeType == "S") ||
                 (element.nodeType == "DEL") ||
                 (element.nodeType == "STRIKE")) {
            foundStrikethrough = true;
        }
        else if ((element.nodeType == "OL") && !foundUnorderedList) {
            foundOrderedList = true;
        }
        else if ((element.nodeType == "UL") && !foundOrderedList) {
            foundUnorderedList = true;
        }
        else if (element.nodeType == "TBODY") {
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
        firstElement = false;
        console.log("Checking the next parent");
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

    if (style) {
        console.log("Notifying of font params change, style: " + style);
        textCursorPositionHandler.setTextCursorPositionFontName(style.fontFamily);
        textCursorPositionHandler.setTextCursorPositionFontSize(style.fontSize);
    }
    else {
        console.log("Computed style is null");
    }
}
