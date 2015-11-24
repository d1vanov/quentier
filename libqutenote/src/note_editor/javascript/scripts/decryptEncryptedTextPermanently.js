function decryptEncryptedTextPermanently(encryptedText, decryptedText, enCryptIndex) {
    console.log("decryptEncryptedTextPermanently: encrypted text = " + encryptedText);

    if (enCryptIndex) {
        console.log("Removing encrypted text with index " + enCryptIndex);
        var elements = document.querySelectorAll("[encrypted_text='" + encryptedText +
                                                 "',en-crypt-id=" + enCryptIndex + "]");
        var numElements = elements.length;
        if (numElements !== 1) {
            console.error("Unexpected number of found en-crypt tags: " + numElements);
            return;
        }

        var element = elements[0];
        element.outerHTML = decryptedText;
        return;
    }

    elements = document.querySelectorAll("[encrypted_text='" + encryptedText + "']");
    numElements = elements.length;
    console.log("Found " + numElements + " matching encrypted text entries");

    for(var index = 0; index < numElements; ++index) {
        element = elements[index];
        if (!element.parentNode) {
            continue;
        }

        element.outerHTML = decryptedText;
    }
}
