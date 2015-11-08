MutationObserver = window.MutationObserver || window.WebKitMutationObserver;

var observer = new MutationObserver(function(mutations, observer) {
    var numMutations = mutations.length;
    for(var index = 0; index < numMutations; ++index) {
        var mutation = mutations[index];
        console.log("Mutation type: " + mutation.type);
        if (mutation.type === "attributes") {
            var attrName = mutation.attributeName;
            var attrNewValue = mutation.target.getAttribute(attrName);
            var attrOldValue = mutation.oldValue;
            if ((attrName === "style") && !attrOldValue && (attrNewValue === "cursor: default;")) {
                console.log("Ignoring the mutation of cursor style");
                return;
            }

            console.log("Changed attribute " + attrName + " from " + attrOldValue + " to " + attrNewValue);
        }
    }

    if (window.pageMutationObserver) {
        window.pageMutationObserver.onPageMutation();
    }
});

observer.observe(document, {
  subtree: true,
  attributes: true,
  attributeOldValue: true,
  childList: true,
  characterData: true
});
