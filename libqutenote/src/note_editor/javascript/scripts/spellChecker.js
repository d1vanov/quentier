function SpellChecker() {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var lastError;

    var matchRegex = "";
    var misspellTag = "EM";
    var skipTags = new RegExp("^(?:" + misspellTag + "|SCRIPT|FORM)$");

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

        observer.stop();

        try {
            this.remove();
            this.highlightMisSpelledWords(document.body);
        }
        finally {
            observer.start();
        }
    }

    this.correctSpelling = function(correction) {
        console.log("SpellChecker::correctSpelling: " << correction);

        var selection = window.getSelection();
        if (!selection || !selection.anchorNode || selection.anchorNode.parentNode) {
            lastError = "Can't correct spelling: no proper selection";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        if (selection.isCollapsed) {
            lastError = "Can't correct spelling: the selection is collapsed";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        undoNodes.push(selection.anchorNode.parentNode);
        undoNodeInnerHtmls.push(selection.anchorNode.parentNode.innerHTML);

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
            var elements = document.getElementsByTagName(misspellTag);
            for(var index = 0; index < elements.length; ++index) {
                elements[index].parentNode.removeChild(elements[index]);
            }
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
        this.matchRegex = new RegExp(re, flags);
    }

    this.highlightMisSpelledWords = function(node) {
        if (!matchRegex) {
            return;
        }

        if (node === undefined || !node) {
            return;
        }

        if (skipTags.test(node.nodeName)) {
            return;
        }

        if (node.hasChildNodes()) {
            var numChildNodes = node.childNodes.length;
            for(var i = 0; i < numChildNodes; i++) {
                this.apply(node.childNodes[i]);
            }
        }

        if (node.nodeType == 3) { // NODE_TEXT
            var nv = node.nodeValue;
            var regs;
            if (nv && (regs = this.matchRegex.exec(nv))) {
                var match = document.createElement(misspellTag);
                match.appendChild(document.createTextNode(regs[1]));
                match.className = "misspell";

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
        console.log("tableManager.undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceNodes) {
            console.warn("Can't " + actionString + " the spell check correction action: no source nodes helper array");
            return false;
        }

        if (!sourceNodeInnerHtmls) {
            console.warn("Can't " + actionString + " the spell check correction action: no source node inner html helper array");
            return false;
        }

        var sourceNode = sourceNodes.pop();
        if (!sourceNode) {
            console.warn("Can't " + actionString + " the spell check correction action: no source node");
            return false;
        }

        var sourceNodeInnerHtml = sourceNodeInnerHtmls.pop();
        if (!sourceNodeInnerHtml) {
            console.warn("Can't " + actionString + " the table action: no source node's inner html");
            return false;
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
    }
}

(function(){
    window.spellChecker = new SpellChecker;
})();

