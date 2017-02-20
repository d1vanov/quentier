/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

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
        if (window.hasOwnProperty("observer")) {
            observer.stop();
        }
    }

    this.onResizeStop = function(event, ui) {
        undoNodes.push(ui.originalElement[0]);
        undoSizes.push(ui.originalSize);

        if (window.hasOwnProperty("observer")) {
            observer.start();
        }

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
                var observerWasRunning = false;
                if (window.hasOwnProperty("observer")) {
                    observerWasRunning = observer.running;
                }
                console.log("observerWasRunning = " + observerWasRunning);

                if (observerWasRunning) {
                    observer.stop();
                    console.log("Stopped the observer");
                }

                try {
                    this.style.height = "";
                    this.style.width = "";
                    this.style.margin = "";
                    this.style.resize = "";
                    this.style.position = "";
                    this.style.zoom = "";
                    this.style.display = "";
                    console.log("Updated style: " + this.style);

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

                    height = this.getAttribute("height");
                    if (height) {
                        height = Number(height.replace(/[^\d\.\-]/g, ''));
                    }
                    else {
                        height = $(this).height();
                    }

                    width = this.getAttribute("width");
                    if (width) {
                        width = Number(width.replace(/[^\d\.\-]/g, ''));
                    }
                    else {
                        width = $(this).width();
                    }

                    var $wrapper = $(this).resizable("widget");
                    var $element = $wrapper.find(".ui-resizable");

                    $element.width(width);
                    $wrapper.width(width);
                    $element.height(height);
                    $wrapper.height(height);

                    console.log("Resized element and wrapper to height = " + $element.height() + ", width = " + $element.width());
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

        if (window.hasOwnProperty("observer")) {
            observer.stop();
        }

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
            if (window.hasOwnProperty("observer")) {
                observer.start();
            }
        }
    }
}

(function(){
    window.resizableImageManager = new ResizableImageManager;
})();
