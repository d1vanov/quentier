function setupTextCursorPositionTracking() {
    console.log("setupTextCursorPositionTracking");

    var $body = $('body');
    var debouncedFunc = function(event) {
        notifyTextCursorPositionChanged();
    }

    $body.bind('keyup', $.throttle(debouncedFunc, 1000));
    $body.bind('mouseup', $.throttle(debouncedFunc, 1000));
}
