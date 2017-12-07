#include "SetupApplicationIcon.h"
#include <quentier/utility/QuentierApplication.h>
#include <quentier/utility/Macros.h>
#include <QIcon>

namespace quentier {

void setupApplicationIcon(QuentierApplication & app)
{
    QIcon icon;
    icon.addFile(QStringLiteral(":/app_icons/quentier_icon_512.png"), QSize(512, 512));
    icon.addFile(QStringLiteral(":/app_icons/quentier_icon_256.png"), QSize(256, 256));
    icon.addFile(QStringLiteral(":/app_icons/quentier_icon_128.png"), QSize(128, 128));
    icon.addFile(QStringLiteral(":/app_icons/quentier_icon_64.png"), QSize(64, 64));
    icon.addFile(QStringLiteral(":/app_icons/quentier_icon_48.png"), QSize(48, 48));
    icon.addFile(QStringLiteral(":/app_icons/quentier_icon_32.png"), QSize(32, 32));
    icon.addFile(QStringLiteral(":/app_icons/quentier_icon_16.png"), QSize(16, 16));

    app.setWindowIcon(icon);
}

}
