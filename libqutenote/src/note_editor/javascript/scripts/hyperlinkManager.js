/**
 * This function implements the JavaScript side control over hyperlink insertion/removal/change etc
 * and corresponding undo/redo commands
 */
function HyperlinkManager() {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var lastError;

    this.removeHyperlink = function(hyperlinkIdNumber, completely) {
        console.log("HyperlinkManager::removeHyperlink: id number = " + hyperlinkIdNumber +
                    ", completely = " + (completely ? "true" : "false"));

        if (!hyperlinkIdNumber) {
            lastError = "no hyperlink id number provided";
            return { status:false, error:lastError };
        }

        var element = document.querySelector("[en-hyperlink-id='" + hyperlinkIdNumber + "']");
        if (!element) {
            lastError = "can't find hyperlink element with given hyperlink id number " + hyperlinkIdNumber.toString();
            return { status:false, error:lastError };
        }

        if (!element.parentNode) {
            lastError = "can't remove hyperlink: hyperlink element has no parent";
            return { status:false, error:lastError };
        }

        undoNodes.push(element.parentNode);
        undoNodeInnerHtmls.push(element.parentNode.innerHTML);

        observer.stop();

        try {
            if (completely) {
                element.parentNode.removeChild(element);
            }
            else {
                element.outerHTML = element.innerHTML;
            }
        }
        finally {
            observer.start();
        }

        return { status:true, error:"" };
    }

    this.setHyperlinkToSelection = function(text, link, hyperlinkIdNumber) {
        console.log("setHyperlinkToSelection: text = " + text + "; link = " + link +
                    "; id number = " + hyperlinkIdNumber);

        var element = this.findSelectedHyperlinkElement();
        if (element) {
            return this.setHyperlinkToElement(element, text, link);
        }

        console.log("Found no link node within the selection");
        if (!text) {
            text = window.getSelection().toString();
            if (!text) {
                console.log("Selection converted to string yields empty string, " +
                            "will use the link itself as link text");
                text = link;
            }
        }

        // TODO: undo stuff

        observer.stop();

        try {
            replaceSelectionWithHtml('<a href="' + link + '" en-hyperlink-id="' +
                                     hyperlinkIdNumber +'" >' + text + "</a>");
        }
        finally {
            observer.start();
        }
    }

    this.setHyperlinkToElement = function(element, text, link) {
        if (!element) {
            return { status:false, error:"Element to set hyperlink to is empty" };
        }

        if (!element.parentNode) {
            return { status:false, error:"Element to set hyperlink to has no parent" };
        }

        undoNodes.push(element.parentNode);
        undoNodeInnerHtmls.push(element.parentNode.innerHTML);

        observer.stop();

        try {
            element.href = link;
            if (text) {
                element.textContent = text;
            }
        }
        finally {
            observer.start();
        }
    }

    this.getSelectedHyperlinkData = function() {
        var element = this.findSelectedHyperlinkElement();
        if (!element) {
            console.log("Haven't found hyperlink under selection");
            var text = getSelectionHtml();
            return [text, "", "0"];
        }

        var idNumber = element.getAttribute('en-hyperlink-id');
        if (!idNumber) {
            idNumber = 0;
        }

        console.log("Found hyperlink under selection, returning [" + element.innerHTML +
                    ", " + element.href + ", " + idNumber + "]");
        return [element.innerHTML, element.href, idNumber.toString()];
    }

    this.findSelectedHyperlink = function() {
        console.log("findSelectedHyperlinkElement");

        var element;
        var selection = window.getSelection();
        if (selection && selection.anchorNode && selection.anchorNode.parentNode) {
            element = selection.anchorNode.parentNode;
        }

        while(element) {
            if (Object.prototype.toString.call( element ) === '[object Array]') {
                console.log("Found array of elements");
                element = element[0];
                if (!element) {
                    console.log("First element of the array is null");
                    break;
                }
            }

            console.log("element.nodeType = " + element.nodeType);
            if (element.nodeType === 1) {
                console.log("Found element with nodeType == 1");

                if (element.nodeName === "A") {
                    console.log("Found hyperlink within the selection");
                    break;
                }
            }

            element = element.parentNode;
        }

        return element;
    }

    this.undo = function() {
        return this.undoRedoImpl(undoNodes, undoNodeInnerHtmls,
                                 redoNodes, redoNodeInnerHtmls, true);

    }

    this.redo = function() {
        return this.undoRedoImpl(redoNodes, redoNodeInnerHtmls,
                                 undoNodes, undoNodeInnerHtmls, false);
    }

    this.undoRedoImpl = function(sourceNodes, sourceNodeInnerHtmls,
                                 destNodes, destNodeInnerHtmls, performingUndo) {
        console.log("HyperlinkManager::undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceNodes) {
            lastError = "can't " + actionString + " the hyperlink action: no source nodes helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        if (!sourceNodeInnerHtmls) {
            lastError = "can't " + actionString + " the hyperlink action: no source node inner html helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNode = sourceNodes.pop();
        if (!sourceNode) {
            lastError = "can't " + actionString + " the hyperlink action: no source node";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNodeInnerHtml = sourceNodeInnerHtmls.pop();
        if (!sourceNodeInnerHtml) {
            lastError = "can't " + actionString + " the hyperlink action: no source node's inner html";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        destNodes.push(sourceNode);
        destNodeInnerHtmls.push(sourceNode.innerHTML);

        console.log("Html before: " + sourceNode.innerHTML + "; html to paste: " + sourceNodeInnerHtml);

        observer.stop();
        try {
            sourceNode.innerHTML = sourceNodeInnerHtml;
        }
        finally {
            observer.start();
        }

        return { status:true, error:"" };
    }
}
