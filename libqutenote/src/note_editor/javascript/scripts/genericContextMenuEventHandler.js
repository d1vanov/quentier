function genericContextMenuEventHandler(event) {
    console.log("contextMenuEventHandler");

    // Figure out on which element the cursor is
    var x = event.pageX - window.pageXOffset;
    var y = event.pageY - window.pageYOffset;
    var element = document.elementFromPoint(x, y);
    if (!element) {
        console.log("Can't get element from point: (" + x + "; " + y + ")");
        return;
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

    // If nothing is selected currently, select the word under cursor
    var selectedHtml = getSelectionHtml();
    if (!selectedHtml) {
        snapSelectionToWord();
        selectedHtml = getSelectionHtml();
    }

    // Wrap the current selection to separate element to allow jQuery context menu run on it
    var selection = window.getSelection();
    if (!selection || !selection.rangeCount || !selection.getRangeAt) {
        console.log("Selection is missing");
        return;
    }

    var span = document.createElement("span");

    var range = selection.getRangeAt(0).cloneRange();
    range.surroundContents(span);
    selection.removeAllRanges();
    selection.addRange(range);

    var emptySelection = $(span).is(':empty');

    // Setup the list of menu items
    var textMenu = [
    { 'Cut':{
            onclick:function(menuItem, menu) { console.log("Clicked cut"); },
            disabled:emptySelection
        }
    },
    { 'Copy':{
            onclick:function(menuItem, menu) { console.log("Clicked copy"); },
            disabled:emptySelection
        }
    },
    { 'Paste':function(menuItem, menu) { console.log("Clicked paste"); } },
    { 'Paste as unformatted text':function(menuItem, menu) { console.log("Clicked paste as text"); } },
    $.contextMenu.separator,
    { 'Insert ToDo checkbox':function(menuItem, menu) { console.log("Clicked insert ToDo checkbox"); } },
    { 'Insert special symbol...':function(menuItem, menu) { console.log("Clicked insert special symbol"); } },
    { 'Insert table...':function(menuItem, menu) { console.log("Clicked insert table"); } },
    { 'Insert horizontal line':function(menuItem, menu) { console.log("Clicked insert horizontal line"); } },
    { 'Hyperlink...':function(menuItem, menu) { console.log("Clicked hyperlink"); } },
    $.contextMenu.separator,
    { 'Encrypt selected fragment':{
             onclick:function(menuItem, menu) { console.log("Clicked encrypt selected fragment"); },
             disabled:emptySelection
        }
    }
    ];

    var contextMenuTheme = "xp";
    if (navigator.appVersion.indexOf("Mac")!=-1) {
        contextMenuTheme = "osx";
    }

    $(span).triggerContextMenu(event,textMenu,{
        theme:contextMenuTheme,
        hideCallback:function(){$(span).contents().unwrap();}
    });
}
