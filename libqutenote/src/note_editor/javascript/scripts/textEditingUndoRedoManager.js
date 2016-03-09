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

        value = value || (node.nodeType === 3 ? node.nodeValue : node.innerHTML);
        if (node.nodeType === 3) {
            var element = node;
            while(element.nodeType === 3) {
                element = element.parentNode;
            }

            var html = this.collectHtmlWithOldTextNodeValue(element, node, value);
            console.log("Collected html with old text node value: " + html + "; the real current inner HTML of the element: " +
                        element.innerHTML);
            node = element;
            value = html;
        }

        undoNodes.push(node);
        undoNodeValues.push(value);
    }

    this.pushNumMutations = function(num) {
        console.log("TextEditingUndoRedoManager::pushNumMutations: " + num);
        undoMutationCounters.push(num);
    }

    this.collectHtmlWithOldTextNodeValue = function(element, textNode, value) {
        var html = "";
        for(var i = 0; i < element.childNodes.length; ++i) {
            var child = element.childNodes[i];
            if (child === textNode) {
                html += value;
            }
            else if (child.nodeType === 3) {
                html += child.nodeValue;
            }
            else {
                html += child.outerHTML;
            }

            if (child.hasChildNodes()) {
                for(var j = 0; j < child.childNodes.length; ++j) {
                    html += this.collectHtmlWithOldTextNodeValue(child.childNodes[j], textNode, value);
                }
            }
        }

        return html;
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

                if (i === (numMutations - 1)) {
                    var range = document.createRange();
                    range.selectNodeContents(sourceNode);
                    range.collapse(false);
                    var selection = window.getSelection();
                    selection.removeAllRanges();
                    selection.addRange(range);
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
