function descend(element, depth, result, x, y) {
    if (element.nodeType != 1) {
        return;
    }

    var rect = element.getBoundingClientRect();
    console.log("descend: inspecting element: " + element.outerHTML + "; rect: left = " + rect.left +
                ", right = " + rect.right + ", top = " + rect.top + ", bottom = " + rect.bottom +
                "; x = " + x + ", y = " + y);

    if ((x < rect.left) || (x > rect.right) || (y < rect.top) || (y > rect.bottom)) {
        console.log("Point outside of rect");
        return;
    }

    if (depth > result.maxDepth) {
        result.maxDepth = depth;
        result.deepestElem = element;
    }

    for (var i = 0; i < element.childNodes.length; i++) {
        descend(element.childNodes[i], depth + 1, result, x, y);
    }
}

function findInnermostElement(element, x, y) {
    var result = { maxDepth:0, deepestElem:null };
    descend(element, 0, result, x, y);
    return result;
}
