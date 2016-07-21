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

// The two inner functions below were adapted from Tim Down's answers on StackOverflow:
// https://stackoverflow.com/a/13950376 and https://stackoverflow.com/a/17694760/1217285
function SelectionManager() {
    // This function computes and returns the character offset from the start of document.body to the current selection's start & end
    this.saveSelection = function() {
        console.log("SelectionManager.saveSelection");

        var selection = window.getSelection();
        if (!selection) {
            console.log("Can't save selection: no selection exists");
            return;
        }

        var range = selection.getRangeAt(0);
        if (!range) {
            console.log("Can't save selection: no range");
            return;
        }

        var preSelectionRange = range.cloneRange();
        preSelectionRange.selectNodeContents(document.body);
        preSelectionRange.setEnd(range.startContainer, range.startOffset);
        var start = preSelectionRange.toString().length;
        var rangeStr = range.toString();
        var end = start + rangeStr.length;

        console.log("Computed selection offsets: start = " + start + ", end = " + end +
                    ", stringified range: " + rangeStr);
        return { start: start, end: end };
    }

    // This function tries to restore the previously saved selection
    this.restoreSelection = function(savedSelection) {
        if (!savedSelection) {
            console.log("Can't restore selection: saved selection is null");
            return;
        }

        console.log("Trying to restore the selections with offsets: start = " +
                    savedSelection.start + ", end = " + savedSelection.end);

        var charIndex = 0;

        var range = document.createRange();
        range.setStart(document.body, 0);
        range.collapse(true);

        var nodeStack = [document.body];
        var node;
        var foundStart = false;
        var stop = false;

        while (!stop && (node = nodeStack.pop())) {
            if (node.nodeType === 3) {
                var nextCharIndex = charIndex + node.length;

                if (!foundStart && (savedSelection.start >= charIndex) && (savedSelection.start <= nextCharIndex)) {
                    range.setStart(node, savedSelection.start - charIndex);
                    foundStart = true;
                }

                if (foundStart && (savedSelection.end >= charIndex) && (savedSelection.end <= nextCharIndex)) {
                    range.setEnd(node, savedSelection.end - charIndex);
                    stop = true;
                }

                charIndex = nextCharIndex;
            }
            else {
                var i = node.childNodes.length;
                while(i--) {
                    nodeStack.push(node.childNodes[i]);
                }
            }
        }

        var selection = window.getSelection();
        selection.removeAllRanges();
        selection.addRange(range);

        console.log("Restored stringified range: " + range.toString());
    }
}

(function() {
    window.selectionManager = new SelectionManager;
})();
