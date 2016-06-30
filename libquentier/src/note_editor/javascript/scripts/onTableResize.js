function onTableResize(e)
{
    console.log("onTableResize");

    if (window.hasOwnProperty('tableResizeHandler')) {
        tableResizeHandler.onTableResize();
    }

    return true;
};
