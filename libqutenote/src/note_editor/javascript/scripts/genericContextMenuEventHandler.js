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
    { title: "Cut", uiIcon: "ui-icon-scissors", disabled:emptySelection,
      action: function(event, ui) { console.log("Clicked cut"); } },
    { title: "Copy", uiIcon: "ui-icon-copy", disabled:emptySelection,
      action: function(event, ui) { console.log("Clicked copy"); } },
    { title: "Paste", uiIcon: "ui-icon-clipboard",
      action: function(event, ui) { console.log("Clicked paste"); } },
    { title: "Paste as unformatted text", uiIcon: "ui-icon-clipboard",
      action: function(event, ui) { console.log("Clicked paste as unformatted text"); } },
    { title: "---" },
    { title: "Font...", disabled:emptySelection, action: function(event, ui) { console.log("Font menu clicked"); } },
    { title: "Style", disabled:emptySelection, children: [
        { title: "Bold", action: function(event, ui) { console.log("Clicked style bold"); } },
        { title: "Italic", action: function(event, ui) { console.log("Clicked style italic"); } },
        { title: "Underline", action: function(event, ui) { console.log("Clicked style underline"); } },
        { title: "Strikethrough", action: function(event, ui) { console.log("Clicked style strikethrough"); } }
      ]
    },
    { title: "---" },
    { title: "Insert ToDo checkbox", action: function(event, ui) { console.log("Clicked insert ToDo checkbox"); } },
    { title: "Insert special symbol...", action: function(event, ui) { console.log("Clicked insert special symbol"); } },
    { title: "Insert table...", action: function(event, ui) { console.log("Clicked insert table"); } },
    { title: "Insert horizontal line", action: function(event, ui) { console.log("Clicked insert horizontal line"); } },
    { title: "Hyperlink...", action: function(event, ui) { console.log("Clicked hyperlink"); } },
    { title: "---" },
    { title: "Encrypt selected fragment",
      action: function(event, ui) { console.log("Clicked encrypt selected fragment"); },
      disabled:emptySelection }
    ];

    $(span).contextmenu({
        delegate: document,
        menu: textMenu,
        close: function(event, ui) { $(span).contents().unwrap(); }
    });

    $(span).contextmenu("open", $(span), textMenu);
}
