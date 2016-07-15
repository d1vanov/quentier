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

function descend(element, depth, result, x, y) {
    if (element.nodeType != 1) {
        return;
    }

    var rect = element.getBoundingClientRect();
    console.log("descend: inspecting element: " + element.outerHTML + "; rect: left = " + rect.left +
                ", right = " + rect.right + ", top = " + rect.top + ", bottom = " + rect.bottom +
                "; x = " + x + ", y = " + y);

    if ((x < rect.left) || (x > rect.right) || (y < rect.top) || (y > rect.bottom)) {
        console.log("Point outside of rect");
        return;
    }

    if (depth > result.maxDepth) {
        result.maxDepth = depth;
        result.deepestElem = element;
    }

    for (var i = 0; i < element.childNodes.length; i++) {
        descend(element.childNodes[i], depth + 1, result, x, y);
    }
}

function findInnermostElement(element, x, y) {
    var result = { maxDepth:0, deepestElem:null };
    descend(element, 0, result, x, y);
    return result;
}
