function TableManager
{
    var undoNodes = [];
    var undoNodeInnerHtmls = [];

    var redoNodes = [];
    var redoNodeInnerHtmls = [];

    this.insertRow = function() {
        var currentRow = getElementUnderCursor("tr");

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
            previousRow = currentRow.previousSibling;
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

        if (!table.parentNode) {
            console.log("Found table has no parent");
            return;
        }

        undoNodes.push(table.parentNode);
        undoNodeInnerHtmls.push(table.parentNode.innerHTML);

        table.insertRow(rowIndex + 1);
        // TODO: also insert the columns with appropriate styles and widths
    }

    this.deleteRow = function() {
        var element = getElementUnderCursor("tr");

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

    this.deleteColumn = function() {
        var element = getElementUnderCursor("td");

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
            previousColumn = element.parentNode.previousSibling;
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
}
