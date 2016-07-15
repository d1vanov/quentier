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

function provideSrcForResourceImgTags() {
    console.log("provideSrcForResourceImgTags()");
    if (!window.hasOwnProperty('resourceCache')) {
        console.log("resourceCache global variable is not defined");
        return;
    }

    var imgElements = document.getElementsByTagName("img");
    var numElements = imgElements.length;
    if (!numElements) {
        return;
    }

    console.log("Found " + numElements + " img tags");

    for(var index = 0; index < numElements; ++index) {
        var element = imgElements[index];
        if (element.getAttribute("en-tag") != "en-media") {
            console.log("Skipping non en-media tag: " + element.getAttribute("en-tag"));
            continue;
        }

        var type = element.getAttribute("type");
        if (!type || (type.slice(0,5) !== "image")) {
            console.log("Skipping img tag which doesn't represent the image resource, type = " + type);
            continue;
        }

        var hash = element.getAttribute("hash");
        if (!hash) {
            console.log("Skipping img resource without hash attribute");
            continue;
        }

        resourceCache.findResourceInfo(hash);
        console.log("Requested resource info for hash " + hash);
    }
}
