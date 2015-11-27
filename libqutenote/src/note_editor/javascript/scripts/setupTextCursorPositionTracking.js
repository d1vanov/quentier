function setupTextCursorPositionTracking() {
    console.log("setupTextCursorPositionTracking");

    var $body = $('body');
    var debouncedFunc = function(event) {
        notifyTextCursorPositionChanged();
    }

    // TODO: figure out why $.debounce doesn't work and fix it
    $body.bind('keyup', debouncedFunc);
    $body.bind('mouseup', debouncedFunc);
}
