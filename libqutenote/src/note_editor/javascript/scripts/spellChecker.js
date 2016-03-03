function SpellChecker(id, tag) {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var lastError;
    var lastDynamicCheckTriggeredByBackspace = false;

    var dynamicModeOn = false;

    var targetNode = document.getElementById(id) || document.body;
    var misspellTag = tag || "EM";
    var skipTags = new RegExp("^(?:" + misspellTag + "|SCRIPT|FORM)$");
    var matchRegex = "";
    var misspellTagClassName = "misspell";

    this.apply = function() {
        console.log("SpellChecker::apply");

        this.buildRegex.apply(this, arguments);
        this.applyToNode(targetNode);
    }

    this.applyToSelection = function() {
        console.log("SpellChecker::applyToSelection");

        var selection = window.getSelection();
        if (!selection || !selection.anchorNode || !selection.anchorNode.parentNode || !selection.focusNode) {
            console.log("No valid selection");
            return;
        }

        this.buildRegex.apply(this, arguments);
        this.applyToNode(selection.anchorNode.parentNode.parentNode);
    }

    this.applyToNode = function(node) {
        console.log("SpellChecker::applyToNode");

        if (node) {
            console.log("Applying to node: value = " + node.nodeValue + ", text content = " + node.textContent +
                        ", outer HTML = " + node.outerHTML + ", inner HTML = " + node.innerHTML);
        }

        observer.stop();

        try {
            var savedSelection;
            try {
                savedSelection = rangy.saveSelection();
            }
            catch(error) {
                console.warn("Caught exception while trying to save the selection with rangy: " + error);
                savedSelection = null;
            }

            this.remove(node);
            this.highlightMisSpelledWords(node);
        }
        finally {
            if (savedSelection) {
                try {
                    rangy.restoreSelection(savedSelection);
                }
                catch(error) {
                    console.warn("Caught exception while trying to restore the selection with rangy: " + error);

                    try {
                        rangy.removeMarkers(savedSelection);
                    }
                    catch(error) {
                        console.warn("Caught exception while trying to remove the saved selection's markers: " + error);
                    }
                }
            }

            observer.start();
        }
    }

    this.buildRegex = function() {
        console.log("SpellChecker::buildRegex");

        // Convert args to a long string with words separated by spaces
        var input = "";
        var numArgs = arguments.length;
        for(var index = 0; index < numArgs; ++index) {
            input += arguments[index];
            input += " ";
        }

        this.setRegex(input);
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

    this.remove = function(node) {
        console.log("SpellChecker::remove");

        observer.stop();

        try {
            var elements = document.querySelectorAll("." + misspellTagClassName);
            for(var index = 0; index < elements.length; ++index) {
                console.log("Checking element " + elements[index].outerHTML);
                if (!node || node.contains(elements[index])) {
                    var parentNode = elements[index].parentNode;
                    $(elements[index]).contents().unwrap();
                    console.log("Updated element's outer HTML to " + parentNode.innerHTML);
                }
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
        input = input.trim();
        input = input.replace(/\s+/g, "|");
        var re = "(?:^|[\\b\\s.,;:-=+-_><])" + "(" + input + ")" + "(?:[\\b\\s.,;:-=+-_><]|$)";
        var flags = "gi";
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
                matchRegex.lastIndex = 0;
                console.log("match regex = " + matchRegex);
                while((regs = matchRegex.exec(nv)) !== null)
                {
                    console.log("Found misspelled word(s) within the text: " + nv + "; regs.length = " + regs.length +
                                ", regs[0] = " + regs[0] + ", regs[1] = " + regs[1] + ", regs.index = " + regs.index +
                                ", matchRegex.lastIndex = " + matchRegex.lastIndex + ", node length = " + nv.length);

                    if (regs.index >= node.length) {
                        console.log("RegExp result index " + regs.index + " is larger than the text node's length " + node.length);
                        break;
                    }

                    var match = document.createElement(misspellTag);
                    match.addEventListener("keyup", this.misspelledWordsDynamicCheck);
                    match.appendChild(document.createTextNode(regs[1]));
                    match.className = misspellTagClassName;

                    var after;
                    if(regs[0].match(/^\s/)) { // in case of leading whitespace
                        if (regs.index + 1 >= node.length) {
                            console.log("regs.index + 1 = " + (regs.index + 1) + " is larger than the text node's length: " + node.length);
                            break;
                        }
                        after = node.splitText(regs.index + 1);
                    }
                    else {
                        after = node.splitText(regs.index);
                    }

                    after.nodeValue = after.nodeValue.substring(regs[1].length);
                    var parentNode = node.parentNode;
                    parentNode.insertBefore(match, after);
                    parentNode.normalize();
                    if (parentNode) {
                        console.log("After misspelled word highlighting: inner HTML: " + parentNode.innerHTML);
                    }
                }
            }
        }
    }

    this.enableDynamic = function() {
        console.log("SpellChecker::enableDynamic");

        if (dynamicModeOn) {
            console.log("The dynamic mode is already enabled, nothing to do");
            return;
        }

        document.body.addEventListener("keyup", this.dynamicCheck);
        dynamicModeOn = true;
    }

    this.disableDynamic = function() {
        console.log("SpellChecker::disableDynamic");

        document.body.removeEventListener("keyup", this.dynamicCheck);
        dynamicModeOn = false;
    }

    this.dynamicCheck = function(event) {
        console.log("SpellChecker::dynamicCheck: key code = " + event.keyCode);

        var keyCode = event.keyCode;
        if ( (keyCode != 8) &&
             (keyCode != 9) &&
             (keyCode != 13) &&
             (keyCode != 32) &&
             (keyCode != 37) &&
             (keyCode != 38) &&
             (keyCode != 39) &&
             (keyCode != 40) &&
             (keyCode != 106) &&
             (keyCode != 107) &&
             (keyCode != 109) &&
             (keyCode != 110) &&
             (keyCode != 111) &&
             (keyCode != 186) &&
             (keyCode != 187) &&
             (keyCode != 188) &&
             (keyCode != 189) &&
             (keyCode != 190) &&
             (keyCode != 191) &&
             (keyCode != 192) &&
             (keyCode != 219) &&
             (keyCode != 220) &&
             (keyCode != 221) )
        {
            return;
        }

        lastDynamicCheckTriggeredByBackspace = (keyCode == 8);

        var range = window.getSelection().getRangeAt(0);
        if (!range.collapsed) {
            return;
        }

        var words = [];
        var text = range.startContainer.textContent; // .substring(0, range.startOffset + 1);
        words = text.split(/\s/);
        spellCheckerDynamicHelper.setLastEnteredWords(words);
    }

    this.misspelledWordsDynamicCheck = function(event) {
        console.log("SpellChecker::misspelledWordsDynamicCheck")

        var node = event.currentTarget;
        if (!node) {
            return;
        }

        var words = [];
        var text = node.textContent;
        words = text.split(/\s/);
        spellCheckerDynamicHelper.setLastEnteredWords(words);

        event.stopPropagation();
        event.preventDefault();
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

