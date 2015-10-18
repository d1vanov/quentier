function provideDpiSettings(logicalDpiX, logicalDpiY, physicalDpiX, physicalDpiY) {
    console.log("provideDpiSettings: logical dpi x = " + logicalDpiX + ", logical dpi y = " +
                logicalDpiY + ", physical dpi x = " + physicalDpiX + ", physical dpi y = " +
                physicalDpiY);
    window.logicalDpiX = logicalDpiX;
    window.logicalDpiY = logicalDpiY;
    window.physicalDpiX = physicalDpiX;
    window.physicalDpiY = physicalDpiY;
}
