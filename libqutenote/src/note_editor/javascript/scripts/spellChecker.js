function SpellChecker() {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var lastError;

    var matchRegex = "";

    this.highlightMisSpelledWords = function() {
        console.log("SpellChecker::highlightMisSpelledWords");

        // Convert args to a long string with words separated by spaces
        var input = "";
        var numArgs = arguments.length;
        for(var index = 0; index < numArgs; ++index) {
            input += arguments[i];
            input += " ";
        }

        this.setRegex(input);
        this.apply(document.body);
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

    this.apply = function(node) {
        if (!this.matchRegex) {
            return;
        }

        if (node === undefined || !node) {
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
                // TODO: implement this section
            }
        }
    }
}

(function(){
    window.spellChecker = new SpellChecker;
})();

