MutationObserver = window.MutationObserver || window.WebKitMutationObserver;

var observer = new MutationObserver(function(mutations, observer) {
    if (window.pageMutationObserver) {
        window.pageMutationObserver.onPageMutation();
    }
});

observer.observe(document, {
  subtree: true,
  attributes: true,
  childList: true,
  characterData: true
});
