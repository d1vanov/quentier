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
