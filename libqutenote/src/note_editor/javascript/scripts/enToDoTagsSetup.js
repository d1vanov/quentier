function onEnToDoTagClick() {
    if (this.className === 'checkbox_unchecked') {
        console.log("Click on unchecked checkbox, converting to checked");
        this.src='qrc:/checkbox_icons/checkbox_yes.png';
        this.className='checkbox_checked';
    }
    else {
        console.log("Click on checked checkbox, converting to unchecked");
        this.src='qrc:/checkbox_icons/checkbox_no.png';
        this.className='checkbox_unchecked';
    }
}

(function(){
    var toDoTags = document.querySelectorAll(".checkbox_checked,.checkbox_unchecked");
    console.log("Found " + toDoTags.length.toString() + " todo tags");
    for(var i = 0; i < toDoTags.length; ++i) {
        toDoTags[i].onclick = onEnToDoTagClick;
    }
})();
