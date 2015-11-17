MutationObserver = window.MutationObserver || window.WebKitMutationObserver;

var observer = new MutationObserver(function(mutations, observer) {
    if (!pageMutationObserver) {
        console.warn("pageMutationObserver global variable is not defined yet");
        return;
    }

    var numMutations = mutations.length;
    if (!numMutations) {
        return;
    }

    console.log("Detected " + numMutations + " page mutations");

    var numApprovedMutations = 0;

    for(var index = 0; index < numMutations; ++index) {
        var mutation = mutations[index];

        console.log("Mutation[" + index + "]: type = " + mutation.type + ", target: " +
                    (mutation.target ? mutation.target.nodeName : "null") + ", num added nodes = " +
                    (mutation.addedNodes ? mutation.addedNodes.length : 0) + ", num removed nodes = " +
                    (mutation.removedNodes ? mutation.removedNodes.length : 0));

        if (mutation.type === "childList") {
            if (mutation.addedNodes && mutation.addedNodes.length &&
                mutation.removedNodes && mutation.removedNodes.length) {
                console.log("Skipping child mutation which contains both added and removed nodes: " +
                            "it doesn't look like manual note text editing");
                continue;
            }
            else if (mutation.target && mutation.target.nodeName === "HEAD") {
                console.log("Skipping the mutation of HEAD target");
                continue;
            }
        }
        else if (mutation.type !== "characterData") {
            console.log("Skipping non-characterData mutation");
            continue;
        }

        ++numApprovedMutations;
    }

    if (numApprovedMutations) {
        pageMutationObserver.onPageMutation();
    }
});

observer.observe(document, {
  subtree: true,
  attributes: true,
  childList: true,
  characterData: true
});
