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
            console.log("Got style: font family = " + style.fontFamily + ", font size = " + style.fontSize);
        }

        console.log("element.nodeType = " + element.nodeType);
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

        if (element.nodeName == "B") {
            foundBold = true;
            console.log("Found bold");
        }
        else if (element.nodeName == "I") {
            foundItalic = true;
            console.log("Found italic");
        }
        else if (element.nodeName == "U") {
            foundUnderline = true;
            console.log("Fount underline");
        }
        else if ((element.nodeName == "S") ||
                 (element.nodeName == "DEL") ||
                 (element.nodeName == "STRIKE")) {
            foundStrikethrough = true;
            console.log("Found strikethrough");
        }
        else if ((element.nodeName == "OL") && !foundUnorderedList) {
            foundOrderedList = true;
            console.log("Found ordered list");
        }
        else if ((element.nodeName == "UL") && !foundOrderedList) {
            foundUnorderedList = true;
            console.log("Found unordered list");
        }
        else if (element.nodeName == "TBODY") {
            foundTable = true;
            console.log("Found table");
        }

        if (!foundAlignLeft && !foundAlignCenter && !foundAlignRight)
        {
            textAlign = element.style.textAlign;
            if (textAlign) {
                if (textAlign == "left") {
                    foundAlignLeft = true;
                    console.log("Found text align left");
                }
                else if (textAlign == "center") {
                    foundAlignCenter = true;
                    console.log("Found text align center");
                }
                else if (textAlign == "right") {
                    foundAlignRight = true;
                    console.log("Found text align right");
                }
            }
            else {
                foundAlignLeft = true;
                console.log("Assuming text align left");
            }
        }

        element = element.parentNode;
        firstElement = false;
        console.log("Checking the next parent");
    }

    console.log("End of the loop over target elements");

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
        var dpi = window.logicalDpiY || 96;
        var convertedFontSize = (parseFloat(parseInt(style.fontSize) * 72.0 / dpi)).toFixed(2);
        console.log("Notifying of font params change: font family = " + style.fontFamily +
                    ", font size = " + style.fontSize + ", converted font size in pt = " +
                    convertedFontSize);
        textCursorPositionHandler.setTextCursorPositionFontName(style.fontFamily);
        textCursorPositionHandler.setTextCursorPositionFontSize(Math.round(convertedFontSize));
    }
    else {
        console.log("Computed style is null");
    }
}
