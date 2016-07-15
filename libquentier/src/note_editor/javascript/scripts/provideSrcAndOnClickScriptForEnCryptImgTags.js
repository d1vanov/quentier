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

function provideSrcAndOnClickScriptForEnCryptImgTags(path) {
    var imgElements = [];
    imgElements = document.getElementsByTagName("img");
    console.log("found " + imgElements.length.toString() + " img elements in the document");
    for(var index = 0; index < imgElements.length; ++index) {
        var element = imgElements[index];
        if (!element.hasAttribute("en-tag")) {
            console.log("skipping element without en-tag");
            continue;
        }
        if (element.getAttribute("en-tag") != "en-crypt") {
            console.log("skipping non-en-crypt element");
            continue;
        }
        if (!element.hasAttribute("encrypted_text")) {
            console.log("skipping element without encrypted_text attribute");
            continue;
        }
        if (element.hasAttribute("src")) {
            var srcAttr = element.getAttribute("src");
            if ((srcAttr != null) && (srcAttr != "")) {
                console.log("skipping element with already set src: " + srcAttr);
                continue;
            }
        }
        // automatically escape special characters in the path
        path = path.replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
        element.setAttribute("src", path);
        element.setAttribute("onclick", "window.enCryptElementClickHandler.onEnCryptElementClicked(" +
                             "this.getAttribute(\"encrypted_text\"), " +
                             "this.getAttribute(\"cipher\") || \"\", " +
                             "this.getAttribute(\"length\") || \"\", " +
                             "this.getAttribute(\"hint\") || \"\", " +
                             "this.getAttribute(\"en-crypt-id\") || \"\"" +
                             ")");
        console.log("Set en-crypt tag's src to " + path + "; also set onclick script");
    }
}
