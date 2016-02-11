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

    this.replaceHyperlinkContent = function(hyperlinkId, link, text) {
        console.log("HyperlinkManager::replaceHyperlinkContent: hyperlink id = " + hyperlinkId +
                    ", link = " + link + ", text = " + text);

        var element = document.querySelector("[en-hyperlink-id='" + hyperlinkId + "']");
        if (!element) {
            lastError = "Can't replace hyperlink content: can't find hyperlink element by id number";
            return { status:false, error:lastError };
        }

        undoNodes.push(element);
        undoNodeInnerHtmls.push(element.innerHTML);

        observer.stop();

        try {
            element.setAttribute("href", link);
            element.innerHTML = text;
        }
        finally {
            observer.start();
        }
    }

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
        console.log("HyperlinkManager::setHyperlinkToSelection: text = " + text + "; link = " + link +
                    "; id number = " + hyperlinkIdNumber);

        var element = this.findSelectedHyperlink();
        if (element) {
            return this.setHyperlinkToElement(element, text, link);
        }

        var selection = window.getSelection();
        if (!selection || !selection.anchorNode || !selection.anchorNode.parentNode) {
            lastError = "can't set hyperlink to selection: selection or its anchor node or its parent node is empty";
            return { status:false, error:lastError };
        }

        console.log("Found no link node within the selection");
        if (!text) {
            text = selection.toString();
            if (!text) {
                console.log("Selection converted to string yields empty string, " +
                            "will use the link itself as link text");
                text = link;
            }
        }

        undoNodes.push(selection.anchorNode.parentNode);
        undoNodeInnerHtmls.push(selection.anchorNode.parentNode.innerHTML);

        observer.stop();

        try {
            replaceSelectionWithHtml('<a href="' + link + '" en-hyperlink-id="' +
                                     hyperlinkIdNumber +'" >' + text + "</a>");
        }
        finally {
            observer.start();
        }

        return { status:true, error:"" };
    }

    this.setHyperlinkToElement = function(element, text, link) {
        console.log("HyperlinkManager::setHyperlinkToElement");

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

    this.getHyperlinkData = function(hyperlinkId) {
        console.log("HyperlinkManager::getHyperlinkData: hyperlink id = " + hyperlinkId);

        var element = document.querySelector("[en-hyperlink-id='" + hyperlinkId + "']");
        if (!element) {
            lastError = "Can't get hyperlink data: can't find hyperlink element by id number";
            return { status:false, error:lastError };
        }

        console.log("Found hyperlink under selection, returning [" + element.innerHTML +
                    ", " + element.href + "]");
        return { status:true, error:"", data:[element.innerHTML, element.href] };
    }

    this.getSelectedHyperlinkData = function() {
        console.log("HyperlinkManager::getSelectedHyperlinkData");

        var element = this.findSelectedHyperlink();
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

    this.findSelectedHyperlinkId = function() {
        console.log("HyperlinkManager::findSelectedHyperlinkId");

        var element = this.findSelectedHyperlink();
        if (!element) {
            lastError = "can't find selected hyperlink by id number";
            return { status:false, error:lastError };
        }

        var hyperlinkId = element.getAttribute("en-hyperlink-id");
        return { status:true, error:"", data:hyperlinkId };
    }

    this.findSelectedHyperlink = function() {
        console.log("HyperlinkManager::findSelectedHyperlink");

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

(function() {
    window.hyperlinkManager = new HyperlinkManager;
})();
