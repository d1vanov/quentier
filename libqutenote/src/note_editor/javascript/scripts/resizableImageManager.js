function ResizableImageManager() {
    this.disableResizable = function(resource) {
        console.log("ResizableImageManager::disableResizable");
        if (!resource) {
            return;
        }

        $(resource).resizable("disable");
    }

    this.enableResizable = function(resource) {
        console.log("ResizableImageManager::enableResizable");
        if (!resource) {
            return;
        }

        $(resource).resizable("enable");
    }

    this.destroyResizable = function(resource) {
        console.log("ResizableImageManager::destroyResizable");
        if (!resource) {
            return;
        }

        $(resource).resizable("destroy");
    }

    this.onResizeStart = function(event, ui) {
        observer.stop();
        resizableImageHandler.notifyImageResourceResized();
    }

    this.onResizeStop = function(event, ui) {
        observer.start();
    }

    this.onResize = function(event, ui) {
        ui.originalElement[0].setAttribute("height", ui.size.height + "px");
        ui.originalElement[0].setAttribute("width", ui.size.width + "px");
    }

    this.setResizable = function(resource) {
        console.log("ResizableImageManager::setResizable");

        if (!resource) {
            return;
        }

        if (document.body.contentEditable && resource.nodeName === "IMG") {
            $(resource).load(function() {
                var observerWasRunning = observer.running;
                console.log("observerWasRunning = " + observerWasRunning);

                if (observerWasRunning) {
                    observer.stop();
                    console.log("Stopped the observer");
                }

                try {
                    var height = this.naturalHeight;
                    var width = this.naturalWidth;
                    console.log("Applying resizable to image: " + this.outerHTML + "; height = " + height + ", width = " + width);
                    $(this).height(height);
                    $(this).width(width);
                    $(this).resizable({
                        maxHeight: height,
                        maxWidth: width,
                        minHeight: 20,
                        minWidth: 20,
                        start: resizableImageManager.onResizeStart,
                        stop: resizableImageManager.onResizeStop,
                        resize: resizableImageManager.onResize
                    });
                    $(this).resizable("enable");
                }
                finally {
                    if (observerWasRunning) {
                        observer.start();
                        console.log("Started the observer");
                    }
                }
            });
        }
    }
}

(function(){
    window.resizableImageManager = new ResizableImageManager;
})();
