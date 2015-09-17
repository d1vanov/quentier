function notifyTextCursorPositionChanged() {
    console.log("notifyTextCursorPositionChanged");
    if (!window.hasOwnProperty('textCursorPositionHandler')) {
        console.log("No textCursorPositionHandler global variable is set");
        return;
    }

    textCursorPositionHandler.onTextCursorPositionChange();
}
