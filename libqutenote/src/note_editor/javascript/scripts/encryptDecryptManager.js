/**
 * This function implements the JavaScript side control over text encryption/decryption and corresponding undo/redo commands
 */
function EncryptDecryptManager() {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    var lastError;

    this.replaceSelectionWithDecryptedText = function(id, decryptedText, encryptedText, hint, cipher, keyLength) {
        console.log("EncryptDecryptManager::replaceSelectionWithDecryptedText");

        var selection = window.getSelection();
        if (!selection || !selection.anchorNode || !selection.anchorNode.parentNode) {
            lastError = "can't encrypt the selected text: selection or its anchor node or its parent node is empty";
            return { status:false, error:lastError };
        }

        if (selection.isCollapsed) {
            lastError = "can't encrypt the selected text: the selection is collapsed";
            return { status:false, error:lastError };
        }

        if (!selection.rangeCount) {
            lastError = "can't encrypt the selected text: the selection has no ranges";
            return { status:false, error:lastError };
        }

        undoNodes.push(selection.anchorNode.parentNode);
        undoNodeInnerHtmls.push(selection.anchorNode.parentNode.innerHTML);

        observer.stop();

        try {
            var decryptedTextElement = document.createElement("div");
            decryptedTextElement.setAttribute("en-tag", "en-decrypted");
            decryptedTextElement.setAttribute("encrypted_text", encryptedText);
            decryptedTextElement.setAttribute("en-decrypted-id", id);
            decryptedTextElement.className = "en-decrypted hvr-border-color";

            if (cipher) {
                decryptedTextElement.setAttribute("cipher", cipher);
            }

            if (keyLength) {
                decryptedTextElement.setAttribute("length", keyLength);
            }

            if (hint) {
                decryptedTextElement.setAttribute("hint", hint);
            }

            decryptedTextElement.innerHTML = decryptedText;

            var range = selection.getRangeAt(0);
            range.deleteContents();
            range.insertNode(decryptedTextElement);
        }
        finally {
            observer.start();
        }

        return { status:true, error:"" };
    }

    this.encryptSelectedText = function(encryptedTextHtml) {
        console.log("EncryptDecryptManager::encryptSelectedText");

        var selection = window.getSelection();
        if (!selection || !selection.anchorNode || !selection.anchorNode.parentNode) {
            lastError = "can't encrypt the selected text: selection or its anchor node or its parent node is empty";
            return { status:false, error:lastError };
        }

        undoNodes.push(selection.anchorNode.parentNode);
        undoNodeInnerHtmls.push(selection.anchorNode.parentNode.innerHTML);

        observer.stop();

        try {
            replaceSelectionWithHtml(encryptedTextHtml);
        }
        finally {
            observer.start();
        }

        return { status:true, error:"" };
    }

    this.decryptEncryptedText = function(encryptedTextId, decryptedTextHtml) {
        console.log("EncryptDecryptManager::decryptEncryptedText: encrypted text id = " + encryptedTextId);

        var element = document.querySelector("[en-crypt-id='" + encryptedTextId + "']");
        if (!element) {
            lastError = "can't decrypt the encrypted text: can't find the encrypted text to decrypt";
            return { status:false, error:lastError };
        }

        if (!element.parentNode) {
            lastError = "can't decrypt encrypted text: the found encrypted text element has no parent node";
            return { status:false, error:lastError };
        }

        undoNodes.push(element.parentNode);
        undoNodeInnerHtmls.push(element.parentNode.innerHTML);

        observer.stop();

        try {
            element.outerHTML = decryptedTextHtml;
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
        console.log("EncryptDecryptManager::undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceNodes) {
            lastError = "can't " + actionString + " the encryption/decryption: no source nodes helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        if (!sourceNodeInnerHtmls) {
            lastError = "can't " + actionString + " the encryption/decryption: no source node inner html helper array";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNode = sourceNodes.pop();
        if (!sourceNode) {
            lastError = "can't " + actionString + " the encryption/decryption: no source node";
            console.warn(lastError);
            return { status:false, error:lastError };
        }

        var sourceNodeInnerHtml = sourceNodeInnerHtmls.pop();
        if (!sourceNodeInnerHtml) {
            lastError = "can't " + actionString + " the encryption/decryption: no source node's inner html";
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
    window.encryptDecryptManager = new EncryptDecryptManager;
})();
