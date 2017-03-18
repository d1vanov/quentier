#ifndef QUENTIER_DEFAULT_SETTINGS_H
#define QUENTIER_DEFAULT_SETTINGS_H

#include <QtGlobal>

#define DEFAULT_SHOW_SYSTEM_TRAY_ICON (true)
#define DEFAULT_CLOSE_TO_SYSTEM_TRAY (true)
#define DEFAULT_MINIMIZE_TO_SYSTEM_TRAY (false)

#define DEFAULT_START_MINIMIZED_TO_SYSTEM_TRAY (false)

#ifdef Q_WS_MAC
#define DEFAULT_TRAY_ICON_KIND QStringLiteral("dark")
#else
#define DEFAULT_TRAY_ICON_KIND QStringLiteral("colored")
#endif

#endif // QUENTIER_DEFAULT_SETTINGS_H
