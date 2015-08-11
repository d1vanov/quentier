MutationObserver = window.MutationObserver || window.WebKitMutationObserver;

var observer = new MutationObserver(function(mutations, observer) {
    pageMutationHandler.onPageMutation();
});

observer.observe(document, {
  subtree: true,
  attributes: true,
  childList: true,
  characterData: true
});
