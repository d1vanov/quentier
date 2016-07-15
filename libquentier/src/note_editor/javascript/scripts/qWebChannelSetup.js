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

(function(){
    var baseUrl = "ws://127.0.0.1:" + window.websocketserverport.toString();
    console.log("Connecting to WebSocket server at " + baseUrl + ".");
    var socket = new WebSocket(baseUrl);

    socket.onclose = function() {
        console.error("web channel closed");
    };

    socket.onerror = function(error) {
        console.error("web channel error: " + error);
    };

    socket.onopen = function() {
        console.log("WebSocket connected, setting up QWebChannel.");
        new QWebChannel(socket, function(channel) {
            window.resourceCache = channel.objects.resourceCache;
            window.resourceCache.notifyResourceInfo.connect(onResourceInfoReceived);

            window.genericResourceImageHandler = channel.objects.genericResourceImageHandler;
            window.resizableImageHandler = channel.objects.resizableImageHandler;
            window.genericResourceImageHandler.genericResourceImageFound.connect(onGenericResourceImageReceived);

            window.pageMutationObserver = channel.objects.pageMutationObserver;
            window.enCryptElementClickHandler = channel.objects.enCryptElementClickHandler;
            window.openAndSaveResourceButtonsHandler = channel.objects.openAndSaveResourceButtonsHandler;
            window.textCursorPositionHandler = channel.objects.textCursorPositionHandler;
            window.contextMenuEventHandler = channel.objects.contextMenuEventHandler;
            window.hyperlinkClickHandler = channel.objects.hyperlinkClickHandler;
            window.toDoCheckboxClickHandler = channel.objects.toDoCheckboxClickHandler;
            window.tableResizeHandler = channel.objects.tableResizeHandler;
            window.spellCheckerDynamicHelper = channel.objects.spellCheckerDynamicHelper;

            console.log("Created window variables for objects exposed to JavaScript");
        });
        console.log("Connected to WebChannel, ready to send/receive messages!");
    }
})();
