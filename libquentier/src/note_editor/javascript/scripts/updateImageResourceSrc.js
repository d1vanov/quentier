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

function updateImageResourceSrc(hash, newSrc) {
    console.log("updateImageResourceSrc: hash = " + hash + ", new src = " + newSrc);

    var resource = document.querySelector("[hash=\"" + hash + "\"]");
    if (!resource) {
        console.warn("can't find the image resource to update the src for");
        return;
    }

    var isImageResource = ((' ' + resource.className + ' ').indexOf(' en-media-image ') > -1);

    var escapedPath = newSrc.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');

    observer.stop();

    try {
        if (isImageResource) {
            try {
                resizableImageManager.destroyResizable(resource);
            }
            catch(e) {
                console.warn("ResourceImageManager::destroyResizable failed for image resource: " +
                             e.name + ":" + e.message + "\n" + e.stack);
            }
        }

        resource.setAttribute("height", resource.naturalHeight);
        resource.setAttribute("width", resource.naturalWidth);
        resource.setAttribute("src", escapedPath);

        if (isImageResource) {
            try {
                resizableImageManager.setResizable(resource);
            }
            catch(e) {
                console.warn("ResourceImageManager::setResizable failed for image resource: " +
                             e.name + ":" + e.message + "\n" + e.stack);
            }
        }
    }
    finally {
        observer.start();
    }
}
