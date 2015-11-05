function decryptEncryptedTextPermanently(encryptedText, decryptedText) {
    console.log("decryptEncryptedTextPermanently: encrypted text = " + encryptedText);

    var elements = document.querySelectorAll("[encrypted_text='" + encryptedText + "']");
    var numElements = elements.length;
    console.log("Found " + numElements + " matching encrypted text entries");

    for(var index = 0; index < numElements; ++index) {
        var element = elements[index];
        if (!element.parentNode) {
            continue;
        }

        // FIXME: for some reason this doesn't seem to work properly
        var replacementElement = document.createElement("div");
        replacementElement.outerHtml = decryptedText;
        element.parentNode.replaceChild(replacementElement, element);
    }
}
