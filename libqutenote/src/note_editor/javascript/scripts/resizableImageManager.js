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

    this.setResizable = function(resource) {
        console.log("ResizableImageManager::setResizable");

        if (!resource) {
            return;
        }

        if (document.body.contentEditable && resource.nodeName === "IMG") {
            $(resource).load(function() {
                var height = this.naturalHeight;
                var width = this.naturalWidth;
                console.log("Applying resizable to image: " + this.outerHTML + "; height = " + height + ", width = " + width);
                $(this).height(height);
                $(this).width(width);
                $(this).resizable({ maxHeight: height, maxWidth: width, minHeight: 20, minWidth: 20 });
                $(this).resizable("enable");
            });
        }
    }
}

(function(){
    window.resizableImageManager = new ResizableImageManager;
})();
