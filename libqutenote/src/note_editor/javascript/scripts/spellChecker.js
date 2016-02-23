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
            input += arguments[i];
            input += " ";
        }

        this.setRegex(input);

        observer.stop();

        try {
            this.highlightMisSpelledWords(document.body);
        }
        finally {
            observer.start();
        }
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
        if (!this.matchRegex) {
            return;
        }

        if (node === undefined || !node) {
            return;
        }

        if (this.skipTags.test(node.nodeName)) {
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
                var match = document.createElement(this.misspellTag);
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
}

(function(){
    window.spellChecker = new SpellChecker;
})();

