function ImageAreasHilitor() {
    this.hiliteImageArea = function(imageResourceHash, rectX, rectY, rectHeight, rectWidth) {
        console.log("ImageAreasHilitor::hiliteImageArea: image resource hash = " + imageResourceHash + ", rect x = " + rectX +
                    ", y = " + rectY + ", height = " + rectHeight + ", width = " + rectWidth);

        var image = document.querySelector(".en-media-image[hash='" + imageResourceHash + "']");
        if (!image) {
            console.log("Can't find the image to apply highlighting for");
            return;
        }

        observer.stop();

        try {
            var div = document.createElement("div");
            div.className = "image-area-hilitor";
            div.setAttribute("hash", imageResourceHash);
            div.innerHTML = " ";
            document.body.appendChild(div);
            var imageOffset = $(image).offset();
            var top = imageOffset.top + rectY;
            var left = imageOffset.left + rectX;
            div.style.top = top + "px";
            div.style.left = left + "px";
            div.style.width = rectWidth + "px";
            div.style.height = rectHeight + "px";

            // See if we need to scroll to the position of that image - only if the same text was not found within the note's text
            var firstTextHilitorHelper = document.querySelector(".hilitorHelper");
            if (!firstTextHilitorHelper) {
                div.scrollIntoView(/* scroll to top = */ false);
            }
        }
        finally {
            observer.start();
        }
    }

    this.clearImageHilitors = function(hash) {
        console.log("ImageAreasHilitor::clearImageHilitors: hash = " + (hash ? hash : "<all>"));

        observer.stop();

        try {
            var selector = "";
            if (hash) {
                selector += "[hash='" + hash + "']";
            }
            selector += ".image-area-hilitor";
            var elements = document.querySelectorAll(selector);
            for(var index = 0; index < elements.length; ++index) {
                elements[index].parentNode.removeChild(elements[index]);
            }
        }
        finally {
            observer.start();
        }
    }
}

(function() {
    window.imageAreasHilitor = new ImageAreasHilitor;
})();

