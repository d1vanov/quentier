function flipEnToDoCheckboxState(idNumber) {
    console.log("flipEnToDoCheckboxState: " + idNumber);

    var elements = document.querySelectorAll("[en-todo-id='" + idNumber + "']");
    if (!elements) {
        console.warn("Could not find en-todo checkbox with id number " + idNumber);
        return;
    }

    var element = elements[0];
    enToDoTagFlipState(element);
}
