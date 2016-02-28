function SpellChecker(id, tag) {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var lastError;

    var targetNode = document.getElementById(id) || document.body;
    var misspellTag = tag || "EM";
    var skipTags = new RegExp("^(?:" + misspellTag + "|SCRIPT|FORM)$");
    var matchRegex = "";
    var misspellTagClassName = "misspell";

    this.apply = function() {
        console.log("SpellChecker::apply");

        // Convert args to a long string with words separated by spaces
        var input = "";
        var numArgs = arguments.length;
        for(var index = 0; index < numArgs; ++index) {
            input += arguments[index];
            input += " ";
        }

        this.setRegex(input);
        this.remove();

        observer.stop();

        try {
            this.highlightMisSpelledWords(targetNode);
        }
        finally {
            observer.start();
        }
    }

    this.correctSpelling = function(correction) {
        console.log("SpellChecker::correctSpelling: " << correction);

        var selection = window.getSelection();
        if ( (!selection || !selection.anchorNode || !selection.anchorNode.parentNode) ||
             ((selection.anchorNode.parentNode !== document.body) && !selection.anchorNode.parentNode.parentNode) ) {
            lastError = "Can't correct spelling: no proper selection";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        if (selection.isCollapsed) {
            lastError = "Can't correct spelling: the selection is collapsed";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNode = selection.anchorNode.parentNode;
        if (sourceNode.parentNode) {
            sourceNode = sourceNode.parentNode;
        }

        undoNodes.push(sourceNode);
        undoNodeInnerHtmls.push(sourceNode.innerHTML);

        observer.stop();

        try {
            document.execCommand("insertText", false, correction);
        }
        finally {
            observer.start();
        }

        return { status:true, error:"" };
    }

    this.remove = function() {
        console.log("SpellChecker::remove");

        observer.stop();

        try {
            var elements = document.querySelectorAll("." + misspellTagClassName);
            for(var index = 0; index < elements.length; ++index) {
                elements[index].outerHTML = elements[index].innerHTML;
            }
        }
        catch(err) {
            console.warn("Error removing the misspelled words highlighting: " + err);
        }
        finally {
            observer.start();
        }
    }

    this.setRegex = function(input) {
        input = input.replace(/\\([^u]|$)/g, "$1");
        input = input.replace(/[^\w\\\s']+/g, "");
        input = input.replace(/\s+/g, "|");
        var re = "(?:^|[\\b\\s])" + "(" + input + ")" + "(?:[\\b\\s]|$)";
        var flags = "i";
        console.log("Search regex: " + re + "; flags: " + flags);
        matchRegex = new RegExp(re, flags);
    }

    this.highlightMisSpelledWords = function(node) {
        console.log("SpellChecker::highlightMisSpelledWords");
        if (!matchRegex) {
            console.log("Regex is empty");
            return;
        }

        if (node === undefined || !node) {
            console.log("Node is empty");
            return;
        }

        if (skipTags.test(node.nodeName)) {
            var nodeClasses = node.classList;
            if (nodeClasses && nodeClasses.contains(misspellTagClassName)) {
                console.log("Should skip node " + node.nodeName + "; skipTags = " + skipTags);
                return;
            }
        }

        if (node.hasChildNodes()) {
            for(var i = 0; i < node.childNodes.length; i++) {
                this.highlightMisSpelledWords(node.childNodes[i]);
            }
        }

        if (node.nodeType == 3) {
            var nv = node.nodeValue;
            console.log("Found text node to check: node value = " + nv);

            var trimmedNv = nv;
            trimmedNv = trimmedNv.replace(/\s+/g, "");
            trimmedNv = trimmedNv.replace(/(\r\n|\n|\r)/gm, "");
            if (trimmedNv == "") {
                console.log("Skipping string which is empty after removing the whitespace characters; original string: " + nv +
                            "; processed string: " + trimmedNv);
                return;
            }

            var regs;
            if (nv) {
                regs = matchRegex.exec(nv);
                console.log("Regs = " + regs + "; match regex = " + matchRegex);
            }

            if (nv && regs) {
                console.log("Found misspelled word: " + nv);

                var match = document.createElement(misspellTag);
                match.appendChild(document.createTextNode(regs[1]));
                match.className = misspellTagClassName;

                var after;
                if(regs[0].match(/^\s/)) { // in case of leading whitespace
                    after = node.splitText(regs.index + 1);
                }
                else {
                    after = node.splitText(regs.index);
                }

                after.nodeValue = after.nodeValue.substring(regs[1].length);
                node.parentNode.insertBefore(match, after);
            }
        }
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
        console.log("SpellChecker::undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceNodes) {
            lastError = "Can't " + actionString + " the spell check correction action: no source nodes helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        if (!sourceNodeInnerHtmls) {
            lastError = "Can't " + actionString + " the spell check correction action: no source node inner html helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNode = sourceNodes.pop();
        if (!sourceNode) {
            lastError = "Can't " + actionString + " the spell check correction action: no source node";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNodeInnerHtml = sourceNodeInnerHtmls.pop();
        if (!sourceNodeInnerHtml) {
            lastError = "Can't " + actionString + " the table action: no source node's inner html";
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

(function(){
    window.spellChecker = new SpellChecker;
})();

