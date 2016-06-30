function ResizableImageManager() {
    var undoNodes = [];
    var undoSizes = [];
    var redoNodes = [];
    var redoSizes = [];

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
    }

    this.onResizeStop = function(event, ui) {
        undoNodes.push(ui.originalElement[0]);
        undoSizes.push(ui.originalSize);
        observer.start();
        resizableImageHandler.notifyImageResourceResized(true);
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

    this.undo = function() {
        console.log("ResizableImageManager::undo");
        this.undoRedoImpl(undoNodes, undoSizes, redoNodes, redoSizes);
    }

    this.redo = function() {
        console.log("ResizableImageManager::redo");
        this.undoRedoImpl(redoNodes, redoSizes, undoNodes, undoSizes);
    }

    this.undoRedoImpl = function(sourceNodes, sourceSizes, destNodes, destSizes) {
        var node = sourceNodes.pop();
        var size = sourceSizes.pop();

        observer.stop();

        try {
            if (!node) {
                console.log("No node to undo/redo resizing for");
                return;
            }

            if (!size) {
                console.log("No size saved for undo/redo");
                return;
            }

            var $wrapper = $(node).resizable("widget");
            var $element = $wrapper.find(".ui-resizable");
            var dx = $element.width() - size.width;
            var dy = $element.height() - size.height;

            var previousHeight = $(node).height();
            var previousWidth = $(node).width();

            $element.width(size.width);
            $wrapper.width($wrapper.width() - dx);
            $element.height(size.height);
            $wrapper.height($wrapper.height() - dy);

            console.log("Set element width to " + size.width + ", height to " + size.height +
                        "; wrapper's width to " + $wrapper.width() + ", height to " + $wrapper.height());

            size.height = previousHeight;
            size.width = previousWidth;

            destNodes.push(node);
            destSizes.push(size);

            resizableImageHandler.notifyImageResourceResized(false);
        }
        finally {
            observer.start();
        }
    }
}

(function(){
    window.resizableImageManager = new ResizableImageManager;
})();
