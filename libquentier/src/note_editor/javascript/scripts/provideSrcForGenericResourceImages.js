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

function provideSrcForGenericResourceImages() {
    console.log("provideSrcForGenericResourceImages");
    if (!window.hasOwnProperty('genericResourceImageHandler')) {
        console.log("genericResourceImageHandler global variable is not defined");
        return;
    }

    var genericResources = document.querySelectorAll(".en-media-generic");
    var numGenericResources = genericResources.length;
    console.log("Found " + numGenericResources + " generic resource tags");

    for(var index = 0; index < numGenericResources; ++index) {
        var resource = genericResources[index];

        var hash = resource.getAttribute("hash");
        if (!hash) {
            console.log("Skipping resource without hash attribute");
            continue;
        }

        genericResourceImageHandler.findGenericResourceImage(hash);
        console.log("Requested generic resource image for generic resource with hash " + hash);
    }
}
