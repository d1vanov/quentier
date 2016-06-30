(function clickInterceptor() {
    console.log("clickInterceptor");
    window.onclick = function(event) {
        var element = event.target;
        if (!element) {
            console.log("Click target is not an element");
            return;
        }

        if (element.nodeName && (element.nodeName === "A")) {
            console.log("Click target has node name \"a\"");
            var href = element.getAttribute("href");
            if (!href) {
                console.log("Click target has no href attribute");
                return;
            }

            if (window.hasOwnProperty('hyperlinkClickHandler')) {
                hyperlinkClickHandler.onHyperlinkClicked(href);
            }
        }
    }
})();
