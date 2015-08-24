(function(){
    var toDoTags = document.querySelectorAll(".checkbox_checked,.checkbox_unchecked");
    toDoTags.addEventListener("click", function() {
        if (this.className == 'checkbox_unchecked') {
            this.src='qrc:/checkbox_icons/checkbox_yes.png';
            this.className = 'checkbox_checked';
        }
        else {
            this.src='qrc:/checkbox_icons/checkbox_no.png';
            this.className = 'checkbox_unchecked'
        }
    }, false);
})();
