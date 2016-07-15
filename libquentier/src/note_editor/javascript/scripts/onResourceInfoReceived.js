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

function onResourceInfoReceived(resourceHash, filePath, displayName, displaySize) {
    console.log("onResourceInfoReceived: hash = " + resourceHash +
                ", file path = " + filePath + ", name = " + displayName +
                ", size = " + displaySize);

    var resources = document.querySelectorAll("[hash=\"" + resourceHash + "\"]");
    if (!resources) {
        return;
    }

    var resource = resources[0];
    if (!resource) {
        return;
    }

    var escapedPath = filePath.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');

    observer.stop();

    try {
        resource.setAttribute("src", escapedPath);
        resizableImageManager.setResizable(resource);

        var resourceName = resource.getElementsByClassName("resource-name");
        if (resourceName && resourceName[0]) {
            resourceName[0].textContent = displayName;
            console.log("Set resource display name to " + displayName);
        }
        else {
            console.log("Can't find child element for resource display name");
        }

        var resourceSize = resource.getElementsByClassName("resource-size");
        if (resourceSize && resourceSize[0]) {
            resourceSize[0].textContent = displaySize;
            console.log("Set resource display size to " + displaySize);
        }
        else {
            console.log("Can't find child element for resource display size");
        }
    }
    finally {
        observer.start();
    }
}
