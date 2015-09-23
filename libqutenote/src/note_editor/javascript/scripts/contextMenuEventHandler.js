function contextMenuEventHandler(event) {
    console.log("contextMenuEventHandler");

    // Figure out on which element the cursor is
    var element = document.elementFromPoint(event.pageX, event.pageY);
    
    var onImageResource = false;
    var onNonImageResource = false;
    var onEnCryptTag = false;

    while(element) {
        if( Object.prototype.toString.call( element ) === '[object Array]' ) {
            element = element[0];
            if (!element) {
                break;
            }
        }

        if (element.nodeType != 1) {
            element = element.parent;
            continue;
        }

        var enTag = element.getAttribute("en-tag");
        console.log("en-tag attribute: " + enTag);

        if (enTag == "en-media") {
            console.log("Found en-media resource");
            if (element.nodeName == "IMG") {
                console.log("Found image resource under cursor");
                onImageResource = true;
                break;
            }
            else if (element.nodeName == "DIV") {
                console.log("Found non-image resource under cursor");
                foundNonImageResource = true;
                break;
            }
        }
        else if (enTag == "en-crypt") {
            console.log("Found en-crypt tag under cursor");
            onEnCryptTag = true;
            break;
        }
    }

    if (onImageResource || onNonImageResource || onEnCryptTag) {
        // These guys would have the event handlers of their own, won't handle them here
        console.log("Returning from generic context menu event handler");
        return;
    }

    event.preventDefault();

    var selectedHtml = getSelectionHtml();
    console.log("Selected html: " + selectedHtml);

    if (!selectedHtml) {
        // TODO: select the word under cursor and return it as the selection
    }

    // TODO: continue from here
}
