(function(){
    var baseUrl = "ws://127.0.0.1:" + window.websocketserverport.toString();
    console.log("Connecting to WebSocket server at " + baseUrl + ".");
    var socket = new WebSocket(baseUrl);

    socket.onclose = function() {
        console.error("web channel closed");
    };

    socket.onerror = function(error) {
        console.error("web channel error: " + error);
    };

    socket.onopen = function() {
        console.log("WebSocket connected, setting up QWebChannel.");
        new QWebChannel(socket, function(channel) {
            window.resourceCache = channel.objects.resourceCache;
            window.resourceCache.notifyResourceInfo.connect(onResourceInfoReceived);

            window.genericResourceImageHandler = channel.objects.genericResourceImageHandler;
            window.genericResourceImageHandler.genericResourceImageFound.connect(onGenericResourceImageReceived);

            window.pageMutationObserver = channel.objects.pageMutationObserver;
            window.enCryptElementClickHandler = channel.objects.enCryptElementClickHandler;
            window.openAndSaveResourceButtonsHandler = channel.objects.openAndSaveResourceButtonsHandler;
            window.textCursorPositionHandler = channel.objects.textCursorPositionHandler;
            window.contextMenuEventHandler = channel.objects.contextMenuEventHandler;
            window.hyperlinkClickHandler = channel.objects.hyperlinkClickHandler;
            window.toDoCheckboxClickHandler = channel.objects.toDoCheckboxClickHandler;
            window.tableResizeHandler = channel.objects.tableResizeHandler;

            console.log("Created window variables for objects exposed to JavaScript");
        });
        console.log("Connected to WebChannel, ready to send/receive messages!");
    }
})();
