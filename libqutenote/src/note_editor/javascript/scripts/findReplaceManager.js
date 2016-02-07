function FindReplaceManager() {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var undoReplaceAllCounters = [];
    var redoReplaceAllCounters = [];

    this.setSearchHighlight = function(textToHighlight, matchCase) {
        observer.stop();

        try {
            searchHilitor.remove();
            if (textToHighlight && (textToHighlight != "")) {
                searchHilitor.caseSensitive = (matchCase ? true : false);
                searchHilitor.openLeft = true;
                searchHilitor.openRight = true;
                searchHilitor.apply(textToHighlight);
            }
        }
        finally {
            observer.start();
        }
    }

    this.replace = function(textToReplace, replacementText, matchCase) {
        console.log("replace: text to replace = " + textToReplace +
                    "; replacement text = " + replacementText + "; match case = " + matchCase);

        var currentlySelectedHtml = getSelectionHtml();

        var tmp = document.createElement("div");
        tmp.innerHTML = currentlySelectedHtml;
        var currentlySelectedText = tmp.textContent || "";

        console.log("Currently selected text = " + currentlySelectedText);

        var currentSelectionMatches = false;
        if (matchCase) {
            currentSelectionMatches = (textToReplace === currentlySelectedText);
        }
        else {
            currentSelectionMatches = (textToReplace.toUpperCase() === currentlySelectedText.toUpperCase());
        }

        console.log("replace: current selection matches = " + currentSelectionMatches);

        if (!currentSelectionMatches)
        {
            var found = window.find(textToReplace, matchCase, false, true);
            if (!found) {
                console.log("replace: can't find the text to replace");
                return false;
            }
        }

        var selection = window.getSelection();
        if (selection.isCollapsed) {
            console.log("replace: the selection is collapsed");
            return false;
        }

        var anchorNode = selection.anchorNode;
        if (!anchorNode) {
            console.log("replace: no anchor node in the selection");
            return false;
        }

        while(anchorNode && ((anchorNode.nodeType === 3) || (anchorNode.className == "hilitorHelper"))) {
            anchorNode = anchorNode.parentNode;
        }

        if (!anchorNode) {
            console.log("replace: no anchor node after walking up to parents from the original text node");
        }

        var undoHtml = this.clearHighlightTags(anchorNode.innerHTML);

        undoNodes.push(anchorNode);
        undoNodeInnerHtmls.push(undoHtml);

        console.log("replace: pushed html to undo stack: " + undoHtml);

        observer.stop();

        try {
            var res = replaceSelectionWithHtml(replacementText);
            if (!res) {
                console.warn("replace: replaceSelectionWithHtml failed");
                return false;
            }
        }
        finally {
            observer.start();
        }

        return true;
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
        console.log("findAndReplace.undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceNodes) {
            console.warn("Can't " + actionString + " the replacement: no source nodes helper array");
            return false;
        }

        if (!sourceNodeInnerHtmls) {
            console.warn("Can't " + actionString + " the replacement: no source node inner html helper array");
            return false;
        }

        var anchorNode = sourceNodes.pop();
        if (!anchorNode) {
            console.warn("Can't " + actionString + " the replacement: no anchor node");
            return false;
        }

        var anchorNodeInnerHtml = sourceNodeInnerHtmls.pop();
        if (!anchorNodeInnerHtml) {
            console.warn("Can't " + actionString + " the replacement: no anchor node's inner html");
            return false;
        }

        observer.stop();

        try {
            var currentHtml = this.clearHighlightTags(anchorNode.innerHTML);

            destNodes.push(anchorNode);
            destNodeInnerHtmls.push(currentHtml);

            console.log("Html before: " + anchorNode.innerHTML + "; html to paste: " + anchorNodeInnerHtml + "; html before without highlighting tags: " + currentHtml);

            anchorNode.innerHTML = anchorNodeInnerHtml;
        }
        finally {
            observer.start();
        }
    }

    // TODO: should replace this with some more proper way
    this.clearHighlightTags = function(html) {
        var highlightOpenTagStart = 0;
        while(true) {
            highlightOpenTagStart = html.indexOf("<em", highlightOpenTagStart);
            if (highlightOpenTagStart < 0) {
                break;
            }

            var highlightOpenTagEnd = html.indexOf(">", highlightOpenTagStart);
            if (highlightOpenTagEnd < 0) {
                break;
            }

            var highlightCloseTagStart = html.indexOf("</em>", highlightOpenTagEnd);
            if (highlightCloseTagStart < 0) {
                break;
            }

            var classAttributeValueStart = html.indexOf("hilitorHelper");
            if ((classAttributeValueStart <= highlightOpenTagStart) && (classAttributeValueStart >= highlightOpenTagEnd)) {
                continue;
            }

            var highlightedText = html.substring(highlightOpenTagEnd + 1, highlightCloseTagStart);
            html = html.substring(0, highlightOpenTagStart) + highlightedText + html.substring(highlightCloseTagStart + 5, html.length);
        }

        return html;
    }

    this.replaceAll = function(textToReplace, replacementText, matchCase) {
        console.log("replaceAll: text to replace = " + textToReplace +
                    "; replacement text = " + replacementText + "; match case = " + matchCase);

        observer.stop();

        try {
            var counter = 0;
            while(this.replace(textToReplace, replacementText, matchCase)) {
                ++counter;
            }

            undoReplaceAllCounters.push(counter);
        }
        finally {
            observer.start();
        }
    }

    this.undoReplaceAll = function() {
        console.log("undoReplaceAll");

        observer.stop();

        try {
            var counter = undoReplaceAllCounters.pop();
            for(var i = 0; i < counter; ++i) {
                this.undo();
            }

            redoReplaceAllCounters.push(counter);
        }
        finally {
            observer.start();
        }
    }

    this.redoReplaceAll = function() {
        console.log("redoReplaceAll");

        observer.stop();

        try {
            var counter = redoReplaceAllCounters.pop();
            for(var i = 0; i < counter; ++i) {
                this.redo();
            }

            undoReplaceAllCounters.push(counter);
        }
        finally {
            observer.start();
        }
    }
}

(function() {
    window.findReplaceManager = new FindReplaceManager;
})();
