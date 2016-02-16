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
            div.style = "position: absolute; left: " + rectX + "; top: " + rectY +
                "; width: " + rectWidth + "; height: " + rectHeight + ";";
            image.appendChild(div);
        }
        finally {
            observer.start();
        }
    }

    this.clearImageHilitors = function() {
        console.log("ImageAreasHilitor::clearImageHilitors");

        observer.stop();

        try {
            var elements = document.querySelectorAll(".image-area-hilitor");
            var element;
            while(elements.length && (element = elements[0])) {
                var parent = element.parentNode;
                if (parent) {
                    parent.removeChild(element);
                }
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

