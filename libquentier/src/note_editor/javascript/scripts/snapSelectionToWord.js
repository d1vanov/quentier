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

function snapSelectionToWord() {
    console.log("snapSelectionToWord");

    var sel = window.getSelection();
    if (!sel || !sel.isCollapsed) {
        return;
    }

    if (!sel.rangeCount) {
        return;
    }

    var rng = sel.getRangeAt(0);
    if (!rng || !rng.startContainer || !rng.startContainer.data) {
        return;
    }

    var startOffset = 0;
    for(var i = rng.startOffset; i >= 0; i--) {
        var str = rng.startContainer.data[i];
        if (!str) {
            return;
        }

        if (str.match(/\S/) != null) {
            startOffset++;
        } 
        else {
            break;
        }
    }
    
    if (startOffset > 0) {
        // Move one character forward from the leftmost whitespace
        startOffset--;
    }

    var endOffset = 0;
    for(var i = rng.endOffset; i < rng.endContainer.data.length; i++) {
        var str = rng.startContainer.data[i];
        if (!str) {
            return;
        }

        if (str.match(/\S/)) {
            endOffset++;
        } 
        else {
            break;
        }
    }

    startOffset = rng.startOffset - startOffset;
    startOffset = startOffset < 0 ? 0 : startOffset;

    endOffset = rng.endOffset + endOffset;
    endOffset = endOffset > rng.endContainer.data.length ? rng.endContainer.data.length : endOffset;

    rng.setStart(rng.startContainer, startOffset);
    rng.setEnd(rng.endContainer, endOffset);
    sel.removeAllRanges();
    sel.addRange(rng);
}
