function TextEditingUndoRedoManager() {
    var undoNodes = [];
    var undoNodeValues = [];
    var undoMutationCounters = [];

    var redoNodes = [];
    var redoNodeValues = [];
    var redoMutationCounters = [];

    var lastError = "";

    this.pushNode = function(node, value) {
        console.log("TextEditingUndoRedoManager::pushNode: type = " + node.nodeType + ", name = " + node.nodeName +
                    ", value = " + value + ", node's own value = " + (node.nodeType === 3 ? node.nodeValue : node.innerHTML));
        undoNodes.push(node);
        undoNodeValues.push(value || (node.nodeType === 3 ? node.nodeValue : node.innerHTML));
    }

    this.pushNumMutations = function(num) {
        console.log("TextEditingUndoRedoManager::pushNumMutations: " + num);
        undoMutationCounters.push(num);
    }

    this.undo = function() {
        return this.undoRedoImpl(undoNodes, undoNodeValues, undoMutationCounters,
                                 redoNodes, redoNodeValues, redoMutationCounters, true);
    }

    this.redo = function() {
        return this.undoRedoImpl(redoNodes, redoNodeValues, redoMutationCounters,
                                 undoNodes, undoNodeValues, undoMutationCounters, false);
    }

    this.undoRedoImpl = function(sourceNodes, sourceNodeValues, sourceMutationCounters,
                                 destNodes, destNodeValues, destMutationCounters, performingUndo) {
        console.log("TextEditingUndoRedoManager::undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceNodes) {
            lastError = "can't " + actionString + " the text editing: no source nodes helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        if (!sourceNodeValues) {
            lastError = "can't " + actionString + " the text editing: no source node inner html helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        if (!sourceMutationCounters) {
            lastError = "can't " + actionString + " the text editing: no source mutation counters helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var numMutations = sourceMutationCounters.pop();
        console.log("Num mutations = " + numMutations);

        observer.stop();
        try {
            lastError = null;
            for(var i = 0; i < numMutations; ++i) {
                console.log("Processing mutation " + i);

                var sourceNode = sourceNodes.pop();
                if (!sourceNode) {
                    lastError = "can't " + actionString + " the text editing: no source node";
                    break;
                }

                var sourceNodeInnerHtml = sourceNodeValues.pop();
                if (!sourceNodeInnerHtml) {
                    lastError = "can't " + actionString + " the text editing: no source node's inner html";
                    break;
                }

                var previousValue;
                if (sourceNode.nodeType === 3) {
                    previousValue = sourceNode.nodeValue;
                }
                else {
                    previousValue = sourceNode.innerHTML;
                }

                destNodes.push(sourceNode);
                destNodeValues.push(previousValue);

                console.log("Previous value: " + previousValue + "; value to paste: " + sourceNodeInnerHtml +
                            ", source node's type = " + sourceNode.nodeType + ", source node's name = " + sourceNode.nodeName);

                if (sourceNode.nodeType === 3) {
                    sourceNode.nodeValue = sourceNodeInnerHtml;
                }
                else {
                    sourceNode.innerHTML = sourceNodeInnerHtml;
                }
            }
        }
        finally {
            observer.start();
        }

        if (lastError) {
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        destMutationCounters.push(numMutations);

        return { status:true, error:"" };
    }
}

(function() {
    window.textEditingUndoRedoManager = new TextEditingUndoRedoManager;
})();
