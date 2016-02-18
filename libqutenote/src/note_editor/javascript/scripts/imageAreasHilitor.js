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
            div.innerHTML = " ";
            document.body.appendChild(div);
            var imageOffset = $(image).offset();
            var top = imageOffset.top + rectY;
            var left = imageOffset.left + rectX;
            div.style.top = top + "px";
            div.style.left = left + "px";
            div.style.width = rectWidth + "px";
            div.style.height = rectHeight + "px";
            console.log("Successfully inserted the child image-area-hilitor div to the document body: top = " + div.style.top +
                        ", left = " + div.style.left + ", width = " + div.style.width + ", height = " + div.style.height);
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

