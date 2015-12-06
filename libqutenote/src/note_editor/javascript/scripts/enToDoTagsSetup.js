function enToDoTagFlipState(element) {
    if (element.className === 'checkbox_unchecked') {
        console.log("Click on unchecked checkbox, converting to checked");
        element.src='qrc:/checkbox_icons/checkbox_yes.png';
        element.className='checkbox_checked';
    }
    else {
        console.log("Click on checked checkbox, converting to unchecked");
        element.src='qrc:/checkbox_icons/checkbox_no.png';
        element.className='checkbox_unchecked';
    }
}

function onEnToDoTagClick() {
    enToDoTagFlipState(this);
    var id = this.getAttribute("en-todo-id");
    toDoCheckboxClickHandler.onToDoCheckboxClicked(id);
}

(function(){
    var toDoTags = document.querySelectorAll(".checkbox_checked,.checkbox_unchecked");
    console.log("Found " + toDoTags.length.toString() + " todo tags");
    for(var index = 0; index < toDoTags.length; ++index) {
        toDoTags[index].onclick = onEnToDoTagClick;
    }
})();
