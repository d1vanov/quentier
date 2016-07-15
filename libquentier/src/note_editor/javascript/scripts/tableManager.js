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

function TableManager() {
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    this.insertRow = function() {
        console.log("TableManager::insertRow");

        var currentRow = this.getElementUnderCursor("tr");

        if (!currentRow) {
            console.log("Can't find table row to insert the new row after");
            return;
        }

        if (!currentRow.parentNode) {
            console.log("Found table row has no parent");
            return;
        }

        var rowIndex = -1;
        var previousRow = currentRow;
        while(previousRow) {
            if (previousRow.nodeName === "TR") {
                ++rowIndex;
            }
            previousRow = previousRow.previousSibling;
        }

        console.log("The index of current row appears to be " + rowIndex);

        var table = currentRow;
        while(table)
        {
            if (table.nodeName === "TABLE") {
                break;
            }

            table = table.parentNode;
        }

        if (!table) {
            console.log("Can't find table to insert row into");
            return;
        }

        undoNodes.push(table);
        undoNodeInnerHtmls.push(table.innerHTML);

        observer.stop();

        try {
            table.insertRow(rowIndex + 1);
            var newRow = table.rows[rowIndex + 1];

            var existingRowChildren = currentRow.children;
            for(var i = 0; i < existingRowChildren.length; ++i) {
                var td = existingRowChildren[i];
                if (td.nodeName != "TD") {
                    continue;
                }

                var newTd = document.createElement("td");
                newTd.style.cssText = td.style.cssText;
                newTd.innerHTML = "\u00a0";
                newRow.appendChild(newTd);
            }
        }
        finally {
            observer.start();
        }
    }

    this.insertColumn = function() {
        console.log("TableManager::insertColumn");

        var currentColumn = this.getElementUnderCursor("td");

        if (!currentColumn) {
            console.log("Can't find table column to insert the new column after");
            return;
        }

        if (!currentColumn.parentNode) {
            console.log("Found table column has no parent");
            return;
        }

        console.log("source column: " + currentColumn.outerHTML);

        var columnIndex = -1;
        var previousColumn = currentColumn;
        while(previousColumn) {
            if (previousColumn.nodeName === "TD") {
                ++columnIndex;
            }
            previousColumn = previousColumn.previousSibling;
        }

        console.log("The index of column to add the new column after appears to be " + columnIndex);

        var table = currentColumn;
        while(table) {
            if (table.nodeName === "TABLE") {
                break;
            }

            table = table.parentNode;
        }

        if (!table) {
            console.log("Can't find table to insert column into");
            return;
        }

        undoNodes.push(table);
        undoNodeInnerHtmls.push(table.innerHTML);

        var cellIndex = columnIndex + 1;
        var numRows = table.rows.length;

        // Need to calculate the appropriate width for the new column
        var isRelativeWidthTable = false;   // Note that colResizable makes even relative width table have fixed size
                                            // when at least one column resizing is done
        var columnWidths = [];
        var rowChildren = table.rows[0].children;
        for(var i = 0; i < rowChildren.length; ++i) {
            var rowChild = rowChildren[i];
            if (rowChild.nodeName != "TD") {
                continue;
            }

            var currentColumnWidth = rowChild.style.width;
            if ((typeof currentColumnWidth === "string") && (currentColumnWidth.indexOf("%") >= 0)) {
                isRelativeWidthTable = true;
                console.log("The table appears to have width relative to the page's width");
                break;
            }

            currentColumnWidth = parseInt(currentColumnWidth, 10);
            if (!currentColumnWidth) {
                continue;
            }

            columnWidths.push(currentColumnWidth);
            console.log("Pushed column width " + currentColumnWidth);
        }

        var newAverageColumnWidth;
        var columnWidthProportion;
        if (!isRelativeWidthTable) {
            averageColumnWidth = 0;
            for(i = 0; i < columnWidths.length; ++i) {
                averageColumnWidth += columnWidths[i];
            }
            newAverageColumnWidth = Math.round(averageColumnWidth / (columnWidths.length + 1));
            columnWidthProportion = columnWidths.length / (columnWidths.length + 1);
            console.log("The average column width would become " + newAverageColumnWidth + " after the new column insertion; " +
                        "all the other columns would need their width multiplied by " + columnWidthProportion + " to compensate");
        }

        observer.stop();

        try {
            this.disableColumnHandles(table);

            for(i = 0; i < table.rows.length; ++i) {
                var row = table.rows[i];
                var cell = row.insertCell(cellIndex);
                cell.style.cssText = currentColumn.style.cssText;
                cell.innerHTML = "\u00a0";

                if (!isRelativeWidthTable) {
                    cell.style.width = "" + newAverageColumnWidth.toString() + "px";
                    console.log("Set new cell's width to " + newAverageColumnWidth + " after cell insertion at row " + i);

                    rowChildren = row.children;
                    var columnWidthIndex = 0;
                    for(var j = 0; j < rowChildren.length; ++j) {
                        if (j === cellIndex) {
                            continue;
                        }

                        rowChild = rowChildren[j];
                        if (rowChild.nodeName != "TD") {
                            continue;
                        }

                        console.log("Column width was " + rowChild.style.width);
                        rowChild.style.width = "" + Math.round(columnWidths[columnWidthIndex] * columnWidthProportion).toString() + "px";
                        console.log("Column width was assumed to be " + columnWidths[columnWidthIndex] +
                                    ", now it is " + rowChild.style.width);
                        ++columnWidthIndex;
                    }
                }
            }

            this.updateColumnHandles(table);
        }
        finally {
            observer.start();
        }
    }

    this.removeRow = function() {
        console.log("TableManager::removeRow");

        var currentRow = this.getElementUnderCursor("tr");

        if (!currentRow) {
            console.log("Can't find table row to remove");
            return;
        }

        if (!currentRow.parentNode) {
            console.log("Found table row has no parent");
            return;
        }

        var table = currentRow;
        while(table)
        {
            if (table.nodeName === "TABLE") {
                break;
            }

            table = table.parentNode;
        }

        if (!table) {
            console.log("Can't find table element to remove row from");
            return;
        }

        undoNodes.push(table);
        undoNodeInnerHtmls.push(table.innerHTML);

        observer.stop();

        try {
            currentRow.parentNode.removeChild(currentRow);
        }
        finally {
            observer.start();
        }
    }

    this.removeColumn = function() {
        console.log("TableManager::removeColumn");

        var currentColumn = this.getElementUnderCursor("td");

        if (!currentColumn) {
            console.log("Can't find table column to remove");
            return;
        }

        if (!currentColumn.parentNode) {
            console.log("Found table column has no parent");
            return;
        }

        var columnIndex = -1;
        var previousColumn = currentColumn;
        while(previousColumn) {
            if (previousColumn.nodeName === "TD") {
                ++columnIndex;
            }
            previousColumn = previousColumn.previousSibling;
        }

        console.log("The index of column to remove appears to be " + columnIndex);

        var table = currentColumn;
        while(table)
        {
            if (table.nodeName === "TABLE") {
                break;
            }

            table = table.parentNode;
        }

        if (!table) {
            console.log("Can't find table element to remove column from");
            return;
        }

        undoNodes.push(table);
        undoNodeInnerHtmls.push(table.innerHTML);

        observer.stop();

        try {
            this.disableColumnHandles(table);

            for(var i = 0; i < table.rows.length; ++i) {
                table.rows[i].deleteCell(columnIndex);
            }

            this.updateColumnHandles(table);
        }
        finally {
            observer.start();
        }
    }

    this.getElementUnderCursor = function(nodeName) {
        var selection = window.getSelection();
        if (!selection) {
            console.log("selection is null");
            return null;
        }

        var anchorNode = selection.anchorNode;
        if (!anchorNode) {
            console.log("selection.anchorNode is null");
            return null;
        }

        var element = anchorNode.parentNode;
        while(element) {
            if (Object.prototype.toString.call( element ) === '[object Array]') {
                console.log("Found array of elements");
                element = element[0];
                if (!element) {
                    console.log("First element of the array is null");
                    break;
                }
            }

            if (element.nodeName == nodeName.toUpperCase()) {
                break;
            }

            element = element.parentNode;
        }

        return element;
    }

    this.undo = function() {
        return this.undoRedoImpl(undoNodes, undoNodeInnerHtmls,
                                 redoNodes, redoNodeInnerHtmls, true);

    }

    this.redo = function() {
        return this.undoRedoImpl(redoNodes, redoNodeInnerHtmls,
                                 undoNodes, undoNodeInnerHtmls, false);
    }

    this.undoRedoImpl = function(sourceNodes, sourceNodeInnerHtmls,
                                 destNodes, destNodeInnerHtmls, performingUndo) {
        console.log("tableManager.undoRedoImpl: performingUndo = " + (performingUndo ? "true" : "false"));

        var actionString = (performingUndo ? "undo" : "redo");

        if (!sourceNodes) {
            console.warn("Can't " + actionString + " the table action: no source nodes helper array");
            return false;
        }

        if (!sourceNodeInnerHtmls) {
            console.warn("Can't " + actionString + " the table action: no source node inner html helper array");
            return false;
        }

        var sourceNode = sourceNodes.pop();
        if (!sourceNode) {
            console.warn("Can't " + actionString + " the table action: no source node");
            return false;
        }

        var sourceNodeInnerHtml = sourceNodeInnerHtmls.pop();
        if (!sourceNodeInnerHtml) {
            console.warn("Can't " + actionString + " the table action: no source node's inner html");
            return false;
        }

        destNodes.push(sourceNode);
        destNodeInnerHtmls.push(sourceNode.innerHTML);

        console.log("Html before: " + sourceNode.innerHTML + "; html to paste: " + sourceNodeInnerHtml);

        observer.stop();

        try {
            this.disableColumnHandles(sourceNode);
            sourceNode.innerHTML = sourceNodeInnerHtml;
            this.updateColumnHandles(sourceNode);
        }
        finally {
            observer.start();
        }
    }

    this.disableColumnHandles = function(table) {
        $(table).colResizable({
            disable:true
        });
    }

    this.updateColumnHandles = function(table) {
        $(table).each(function(index, element) {
            var foundRelativeWidthColumn = false;
            if (element.rows.length > 0) {
                var row = element.rows[0];
                var rowChildren = row.childNodes;
                if (rowChildren.length > 0) {
                    for(var columnIndex = 0; columnIndex < rowChildren.length; ++columnIndex) {
                        var column = rowChildren[columnIndex];
                        if (column.nodeName != "TD") {
                            continue;
                        }
                        var styleWidthStr = column.style.width;
                        if (styleWidthStr.indexOf("%") > -1) {
                            foundRelativeWidthColumn = true;
                            break;
                        }
                    }
                }
            }
            if (foundRelativeWidthColumn) {
                $(element).colResizable({
                    liveDrag:true,
                    draggingClass:"dragging",
                    onResize:onTableResize
                });
            }
            else {
                $(element).colResizable({
                    liveDrag:true,
                    fixed:false,
                    draggingClass:"dragging",
                    onResize:onTableResize
                });
            }
        });
    }
}

(function() {
    window.tableManager = new TableManager;
})();
