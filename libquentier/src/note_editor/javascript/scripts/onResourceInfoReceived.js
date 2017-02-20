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
        console.log("The resource with the given hash was not found, skipping");
        return;
    }

    var resource = resources[0];
    if (!resource) {
        console.log("The resource is null, skipping");
        return;
    }

    var escapedPath = filePath.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
    console.log("Escaped path to the resource image: " + escapedPath);

    if (window.hasOwnProperty("observer")) {
        observer.stop();
    }

    try {
        resource.setAttribute("src", escapedPath);
        console.log("Set the src attribute for resource with hash \"" + resourceHash + "\" to " + escapedPath);

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
    catch(e) {
        console.log("Caught exception while trying to set the src attribute to a resource image: " +
                    e.name + ": " + e.message + " - " + e.stack);
    }
    finally {
        if (window.hasOwnProperty("observer")) {
            observer.start();
        }
    }
}
