/**
 * This function implements the JavaScript side control over resource html insertion/removal
 * and corresponding undo/redo commands
 */

function ResourceManager() {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var lastError;

    this.addResource = function(resourceHtml)
    {
        console.log("ResourceManager::addResource");

        var selection = window.getSelection();
        if (!selection) {
            lastError = "selection is null";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var anchorNode = selection.anchorNode;
        if (!anchorNode) {
            lastError = "selection.anchorNode is null";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var parentNode = anchorNode.parentNode;
        if (!parentNode) {
            lastError = "selection.anchorNode.parentNode is null";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        undoNodes.push(parentNode);
        undoNodeInnerHtmls.push(parentNode.innerHTML);

        observer.stop();
        try {
            document.execCommand('insertHtml', false, resourceHtml);
        }
        finally {
            observer.start();
        }

        return { status:true, error:"" };
    }

    this.removeResource = function(resourceHash) {
        console.log("ResourceManager::removeResource: " + resourceHash);

        var resource = document.querySelector("[hash='" + resourceHash + "']");
        if (!resource) {
            lastError = "can't find the resource to be removed: hash = " + resourceHash;
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var parentNode = resource.parentNode;
        if (!parentNode) {
            lastError = "the resource element has no parent: hash = " + resourceHash;
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        undoNodes.push(parentNode);
        undoNodeInnerHtmls.push(parentNode.innerHTML);

        observer.stop();
        try {
            parentNode.removeChild(resource);
        }
        finally {
            observer.start();
        }

        return { status:true, error:"" };
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
        console.log("ResourceManager::undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceNodes) {
            lastError = "can't " + actionString + " the resource action: no source nodes helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        if (!sourceNodeInnerHtmls) {
            lastError = "can't " + actionString + " the resource action: no source node inner html helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNode = sourceNodes.pop();
        if (!sourceNode) {
            lastError = "can't " + actionString + " the resource action: no source node";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNodeInnerHtml = sourceNodeInnerHtmls.pop();
        if (!sourceNodeInnerHtml) {
            lastError = "can't " + actionString + " the resource action: no source node's inner html";
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
    window.resourceManager = new ResourceManager;
})();
