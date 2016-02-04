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
            ++rowIndex;
            previousRow = previousRow.previousSibling;
        }

        console.log("The index of current row appears to be " + rowIndex);

        var table = currentRow;
        while(table)
        {
            if (table.nodeName == "TABLE") {
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

        table.insertRow(rowIndex + 1);
        var newRow = table.rows[rowIndex + 1];

        var existingRowChildren = currentRow.children;
        for(var i = 0; i < existingRowChildren.length; ++i) {
            var td = existingRowChildren[i];
            if (td.nodeName != "TD") {
                continue;
            }

            var newTd = document.createElement("td");

            var attr;
            var attributes = Array.prototype.slice.call(td.attributes);
            while(attr = attributes.pop()) {
                newTd.setAttribute(attr.nodeName, attr.nodeValue);
            }

            newRow.appendChild(newTd);
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

        var columnIndex = 0;
        var previousColumn = currentColumn;
        while(previousColumn) {
            if (previousColumn.nodeName == "TD") {
                ++columnIndex;
            }
            previousColumn = previousColumn.previousSibling;
        }

        console.log("The index of column to add the new column after appears to be " + columnIndex);

        var table = currentColumn;
        while(table) {
            if (table.nodeName == "TABLE") {
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

        var cellIndex = columnIndex;
        var numRows = table.rows.length;

        for(var i = 0; i < table.rows.length; ++i) {
            var row = table.rows[i];
            var cell = row.insertCell(cellIndex);
            cell.style.cssText = currentColumn.style.cssText;
            cell.innerHTML = "<div> </div>";
        }
    }

    this.removeRow = function() {
        console.log("TableManager::removeRow");

        var element = this.getElementUnderCursor("tr");

        if (!element) {
            console.log("Can't find table row to remove");
            return;
        }

        if (!element.parentNode) {
            console.log("Found table row has no parent");
            return;
        }

        undoNodes.push(element.parentNode);
        undoNodeInnerHtmls.push(element.parentNode.innerHTML);

        element.parentNode.removeChild(element);
    }

    this.removeColumn = function() {
        console.log("TableManager::removeColumn");

        var element = this.getElementUnderCursor("td");

        if (!element) {
            console.log("Can't find table column to remove");
            return;
        }

        if (!element.parentNode) {
            console.log("Found table column has no parent");
            return;
        }

        var columnIndex = -1;
        var previousColumn = element;
        while(previousColumn) {
            ++columnIndex;
            previousColumn = previousColumn.previousSibling;
        }

        console.log("The index of column to remove appears to be " + columnIndex);

        while(element)
        {
            if (element.nodeName == "TABLE") {
                break;
            }

            element = element.parentNode;
        }

        if (!element) {
            console.log("Can't find table element to remove column from");
            return;
        }

        if (!element.parentNode) {
            console.log("Found table has no parent");
            return;
        }

        undoNodes.push(element.parentNode);
        undoNodeInnerHtmls.push(element.parentNode.innerHTML);

        for(var i = 0; i < element.rows.length; ++i) {
            element.rows[i].deleteCell(columnIndex);
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
        destNodeInnerHtmls.push(sourceNodeInnerHtml);

        console.log("Html before: " + sourceNode.innerHTML + "; html to paste: " + sourceNodeInnerHtml);

        sourceNode.innerHTML = sourceNodeInnerHtml;
    }
}

(function() {
    window.tableManager = new TableManager;
})();
