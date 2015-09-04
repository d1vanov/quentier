(function(){
    console.log("Running qWebKitSetup.js");
    window.resourceCache.resourceLocalFilePathForHash.connect(onResourceLocalFilePathForHashReceived);
})();
