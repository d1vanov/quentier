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

function genericResourceOnClickHandler(event) {
    console.log("genericResourceOnClickHandler");
    if (!window.hasOwnProperty('openAndSaveResourceButtonsHandler')) {
        console.log("openAndSaveResourceButtonsHandler global variable is not defined");
        return true;
    }

    if (event.button != 0) {
        console.log("event.button = " + event.button);
        return true;
    }

    var element = event.target;
    if (!element) {
        console.log("no event target");
        return true;
    }

    if (!element.hasAttribute("hash")) {
        console.log("genericResourceOnClickHandler fired on element without hash attribute");
        return true;
    }

    var hash = element.getAttribute("hash");

    var offset = $(element).offset();
    var x = event.pageX - offset.left;
    var y = event.pageY - offset.top;
    console.log("Click coordinates within image: x = " + x + ", y = " + y);

    // We want to listen for clicks inside certain rectangular regions where the generic resource image
    // has open and save resource icons drawn
    if (x >= 174 && x <= 198 && y >= 4 && y <= 28) {
        console.log("Detected attempt to open resource");
        openAndSaveResourceButtonsHandler.onOpenResourceButtonPressed(hash);
        return false;
    }
    else if (x >= 202 && x <= 226 && y >= 4 && y <= 28) {
        console.log("Detected attempt to save resource");
        openAndSaveResourceButtonsHandler.onSaveResourceButtonPressed(hash);
        return false;
    }

    console.log("Doesn't look like a click meant to open or save the resource, skipping");
    return true;
}
