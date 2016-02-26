function descend(element, depth, result) {
    if (element.nodeType != 1) {
        return;
    }

    if (depth > result.maxDepth) {
        result.maxDepth = depth;
        result.deepestElem = element;
    }

    for (var i = 0; i < element.childNodes.length; i++) {
        descend(element.childNodes[i], depth + 1, result);
    }
}

function findInnermostElement(element) {
    var result = { maxDepth:0, deepestElem:null };
    descend(element, 0, result);
    return result;
}
