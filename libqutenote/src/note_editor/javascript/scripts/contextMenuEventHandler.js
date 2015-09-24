function contextMenuEventHandler(event) {
    console.log("contextMenuEventHandler");

    // Figure out on which element the cursor is
    var x = event.pageX - window.pageXOffset;
    var y = event.pageY - window.pageYOffset;
    var element = document.elementFromPoint(x, y);
    if (!element) {
        console.log("Can't get element from point: (" + x + "; " + y + ")");
    }
    
    var onImageResource = false;
    var onNonImageResource = false;
    var onEnCryptTag = false;

    while(element) {
        if( Object.prototype.toString.call( element ) === '[object Array]' ) {
            console.log("Detected array of elements under the cursor");
            element = element[0];
            if (!element) {
                console.log("The first element is null, breaking away from the loop");
                break;
            }
        }

        if (element.nodeType != 1) {
            console.log("element.nodeType = " + element.nodeType);
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
                onNonImageResource = true;
                break;
            }
        }
        else if (enTag == "en-crypt") {
            console.log("Found en-crypt tag under cursor");
            onEnCryptTag = true;
            break;
        }

        element = element.parent;
    }

    if (onImageResource || onNonImageResource || onEnCryptTag) {
        // These guys would have the event handlers of their own, won't handle them here
        console.log("Returning from generic context menu event handler");
        return;
    }

    event.preventDefault();

    var selectedHtml = getSelectionHtml();

    if (!selectedHtml) {
        snapSelectionToWord();
        selectedHtml = getSelectionHtml();
    }

    // TODO: continue from here
}
