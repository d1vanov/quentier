/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

function determineContextMenuEventTarget(contextMenuSequenceNumber, x, y) {
    console.log("determineContextMenuEventTarget: contextMenuSequenceNumber = " +
                contextMenuSequenceNumber + ", x = " + x + ", y = " + y);

    if (!window.hasOwnProperty('contextMenuEventHandler')) {
        console.log("contextMenuEventHandler global variable is not defined");
        return;
    }

    var foundImageResource = false;
    var foundNonImageResource = false;
    var foundTable = false;
    var resourceHash = "";

    var foundEnCryptTag = false;
    var cipher = "";
    var encryptedText = "";
    var length = "";
    var hint = "";
    var insideDecryptedTextFragment = false;

    var misSpelledWord;

    var extraData = [];

    // get context menu event target
    var element = document.elementFromPoint(x, y);
    console.log("Element from point: " + element.outerHTML);

    if (element.hasChildNodes()) {
        var deepest = findInnermostElement(element, x, y);
        if (deepest.deepestElem) {
            element = deepest.deepestElem;
        }
    }

    console.log("Context menu event target: " + element.outerHTML);

    while(element) {
        if (Object.prototype.toString.call(element) === '[object Array]') {
            console.log("Found array of elements");
            element = element[0];
            if (!element) {
                console.log("First element of the array is null");
                break;
            }
        }

        var elementClass = ' ' + element.className + ' ';
        console.log("Checking the element with class " + elementClass);

        if (elementClass.indexOf(' misspell ') > -1) {
            misSpelledWord = element.textContent;
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
                var enCryptId = element.getAttribute("en-crypt-id");
                extraData.push(cipher);
                extraData.push(length);
                extraData.push(encryptedText);
                extraData.push(hint);
                extraData.push(enCryptId);
                console.log("Found en-crypt tag: encryptedText = " + encryptedText +
                            ", cipher = " + cipher + ", length = " + length << ", hint = " + hint +
                            ", en-crypt-id = " + enCryptId);
                break;
            }
            else if (enTag == "en-decrypted") {
                insideDecryptedTextFragment = true;
                cipher = element.getAttribute("cipher");
                length = element.getAttribute("length");
                encryptedText = element.getAttribute("encrypted_text");
                hint = element.getAttribute("hint");
                var enDecryptedId = element.getAttribute("en-decrypted-id");
                extraData.push(cipher);
                extraData.push(length);
                extraData.push(encryptedText);
                extraData.push(hint);
                extraData.push(enDecryptedId);
                extraData.push(element.innerHTML);
                console.log("Found decrypted text fragment: encryptedText = " + encryptedText +
                            ", cipher = " + cipher + ", length = " + length << ", hint = " + hint +
                            ", en-decrypted-id = " + enDecryptedId);
                break;
            }

            if (element.nodeName == "TABLE") {
                foundTable = true;
                console.log("Found table");
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

        if (foundTable) {
            extraData.push("InsideTable");
        }

        if (misSpelledWord && !window.getSelection().isCollapsed) {
            extraData.push("MisSpelledWord_" + misSpelledWord);
        }

        contextMenuEventHandler.setContextMenuContent("GenericText", selectedHtml, insideDecryptedTextFragment, extraData, contextMenuSequenceNumber);
    }
}
