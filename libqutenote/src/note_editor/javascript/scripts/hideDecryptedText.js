function hideDecryptedText(enDecryptedIndex, replacementHtml) {
    console.log("hideDecryptedText: " + enDecryptedIndex);

    var decryptedTextToHide = document.querySelector("[en-decrypted-id=\"" + enDecryptedIndex + "\"]");
    if (!decryptedTextToHide) {
        console.warn("Can't hide decrypted text: can't find decrypted text by en-decrypt-id");
        return;
    }

    $(decryptedTextToHide).replaceWith(replacementHtml);
}
