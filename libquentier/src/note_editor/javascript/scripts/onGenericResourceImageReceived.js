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

function onGenericResourceImageReceived(resourceHash, imageFilePath) {
    console.log("onGenericResourceImageReceived: resource hash = " + resourceHash + ", image file path = " + imageFilePath);

    var resources = document.querySelectorAll(".en-media-generic[hash=\"" + resourceHash + "\"]");
    if (!resources) {
        return;
    }

    var resource = resources[0];
    if (!resource) {
        return;
    }

    var escapedPath = imageFilePath.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
    resource.setAttribute("src", escapedPath);
    console.log("Successfully set image file path " + escapedPath + " for generic resource with hash " + resourceHash);
}
