function SpellChecker(id, tag) {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var lastError;

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

        var nv = "";
        if (node.hasChildNodes()) {
            var childNode;
            for(var i = 0; i < node.childNodes.length; i++) {
                childNode = node.childNodes[i];
                if (!childNode) {
                    continue;
                }

                if (childNode.nodeType != 3) {
                    this.highlightMisSpelledWords(childNode);
                    continue;
                }

                nv += childNode.nodeValue;
            }
        }
        else {
            if (node.nodeType != 3) {
                console.log("Skipping non-text node: type " + node.nodeType + ", name " + node.nodeName + ", html: " + node.innerHTML);
                return;
            }

            nv = node.nodeValue;
        }

        if (!nv || nv == "") {
            console.log("No node value (cumulative or single) to highlight");
            return;
        }

        matchRegex.lastIndex = 0;
        var regs = matchRegex.exec(nv);
        if (!regs) {
            console.log("No match");
            return;
        }

        console.log("Found misspelled word(s) within the text: " + nv + "; regs.length = " + regs.length +
                    ", regs[0] = " + regs[0] + ", regs[1] = " + regs[1] + ", regs.index = " + regs.index +
                    ", matchRegex.lastIndex = " + matchRegex.lastIndex);

        var match = document.createElement(misspellTag);
        match.appendChild(document.createTextNode(regs[1]));
        match.className = misspellTagClassName;

        var matchIndex = regs.index;
        if(regs[0].match(/^\s/)) { // in case of leading whitespace
            ++matchIndex;
        }

        var targetNode;
        var nextNode;
        if (node.hasChildNodes()) {
            for(i = 0; i < node.childNodes.length; ++i) {
                childNode = node.childNodes[i];

                if (!childNode) {
                    continue;
                }

                if (childNode.nodeType != 3) {
                    continue;
                }

                if (matchIndex >= childNode.length) {
                    matchIndex -= childNode.length;
                    continue;
                }

                targetNode = childNode;
                console.log("Found target text node for match: name = " + targetNode.nodeName + ", node value: " +
                            targetNode.nodeValue + "; adjusted match index = " + matchIndex);

                for(var nextNodeIndex = i + 1; nextNodeIndex < node.childNodes.length; ++nextNodeIndex) {
                    childNode = node.childNodes[nextNodeIndex];
                    if (!childNode) {
                        continue;
                    }

                    if (childNode.nodeType != 3) {
                        continue;
                    }

                    nextNode = childNode;
                    console.log("Found target node's next text node: name = " + nextNode.nodeName + ", node value: " +
                                nextNode.nodeValue);
                    break;
                }

                break;
            }
        }
        else {
            targetNode = node;
        }

        if (matchIndex >= targetNode.length) {
            console.warn("Match index = " + matchIndex + " is larger than the target node's length: " + targetNode.length);
            return;
        }

        var after = targetNode.splitText(matchIndex);
        console.log("after: node value = " + after.nodeValue);

        var wordPartInNextNode = regs[1].length - after.length;
        if (wordPartInNextNode > 0) {
            after.nodeValue = "";
            console.log("Erased after's node value");
        }
        else {
            after.nodeValue = after.nodeValue.substring(regs[1].length);
            console.log("after + substring " + regs[1].length + ": node value = " + after.nodeValue);
        }

        // Part of the matched word might have got stuck in the next node, check for that
        if (nextNode && wordPartInNextNode > 0) {
            nextNode.nodeValue = nextNode.nodeValue.substring(wordPartInNextNode);
            console.log("Next node's node value after removing the part until the first whitespace: " + nextNode.nodeValue);
        }

        var parentNode = targetNode.parentNode;
        parentNode.insertBefore(match, after);
        parentNode.normalize();
        console.log("After misspelled word highlighting: inner HTML: " + parentNode.innerHTML);

        this.highlightMisSpelledWords(parentNode);
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

        var navigationKey = ( (keyCode == 37) ||
                              (keyCode == 38) ||
                              (keyCode == 39) ||
                              (keyCode == 40) );

        var insideMisSpelledWord = false;
        var selection = window.getSelection();
        if (!selection || !selection.anchorNode || !selection.rangeCount) {
            return;
        }

        var range = selection.getRangeAt(0);
        if (!range.collapsed) {
            return;
        }

        var node = selection.anchorNode;
        while(node) {
            console.log("Checking node: type = " + node.nodeType + ", name = " + node.nodeName +
                        ", class = " + node.className);
            if ((node.nodeName == misspellTag) && node.className) {
                var className = " " + node.className + " ";
                var classIndex = className.indexOf(" " + misspellTagClassName + " ");
                console.log("Extended class name = " + className + ", class index = " + classIndex);
                if (classIndex >= 0) {
                    insideMisSpelledWord = true;
                    break;
                }
            }
            node = node.parentNode;
        }

        console.log("navigationKey = " + (navigationKey ? "true" : "false") +
                    ", inside misspelled word = " + (insideMisSpelledWord ? "true" : "false"));

        if (navigationKey && insideMisSpelledWord) {
            return;
        }

        if ( !insideMisSpelledWord &&
             (keyCode != 8) &&
             (keyCode != 9) &&
             (keyCode != 13) &&
             (keyCode != 32) &&
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
            console.log("Not inside misspeled word and not non-character editing key code");
            return;
        }

        var words = [];

        // Need to take the parent node and compose the cumulative string of all its child text nodes
        // because otherwise rangy's hidden selection market breaks words into different "sub-words"
        // and the dynamic spell checking gets broken
        var text = "";

        var parentNode = selection.anchorNode.parentNode;
        if (!parentNode) {
            return;
        }

        if (parentNode.hasChildNodes()) {
            for(var index = 0; index < parentNode.childNodes.length; ++index) {
                var childNode = parentNode.childNodes[index];
                if (!childNode) {
                    continue;
                }

                if (childNode.nodeType != 3) {
                    continue;
                }

                text += childNode.textContent;
            }
        }
        else {
            text = range.startContainer.textContent.substring(0, range.startOffset + 1);
        }

        words = text.split(/\s/);
        spellCheckerDynamicHelper.setLastEnteredWords(words);
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

