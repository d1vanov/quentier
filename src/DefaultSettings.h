#ifndef QUENTIER_DEFAULT_SETTINGS_H
#define QUENTIER_DEFAULT_SETTINGS_H

#include "SystemTrayIconManager.h"
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

#define DEFAULT_SINGLE_CLICK_TRAY_ACTION (SystemTrayIconManager::TrayActionShowContextMenu)
#define DEFAULT_MIDDLE_CLICK_TRAY_ACTION (SystemTrayIconManager::TrayActionShowHide)
#define DEFAULT_DOUBLE_CLICK_TRAY_ACTION (SystemTrayIconManager::TrayActionDoNothing)

#endif // QUENTIER_DEFAULT_SETTINGS_H
