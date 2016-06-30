(function(){
    console.log("Running qWebKitSetup.js");
    window.resourceCache.notifyResourceInfo.connect(onResourceInfoReceived);
})();
