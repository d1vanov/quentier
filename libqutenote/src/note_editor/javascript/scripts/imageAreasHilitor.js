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
            $(div).css("position: absolute; left: " + rectX + "; top: " + rectY +
                "; width: " + rectWidth + "; height: " + rectHeight + ";");
            div.innerHTML = " ";
            image.parentNode.appendChild(div);
            console.log("Successfully inserted the child image-area-hilitor div to the parent of the image");
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

