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
            // make resourceCache, pageMutationHandler and enCryptElementClickHandler objects accessible globally
            window.resourceCache = channel.objects.resourceCache;
            window.pageMutationHandler = channel.objects.pageMutationHandler;
            window.enCryptElementClickHandler = channel.enCryptElementClickHandler;
        });
        console.log("Connected to WebChannel, ready to send/receive messages!");
    }
})();
