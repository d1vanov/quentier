#include "DesktopServices.h"
#include <logging/QuteNoteLogger.h>
#include <QStyleFactory>
#include <QApplication>

#ifdef Q_OS_WIN
#include <qwindowdefs.h>
#include <QtGui/qwindowdefs_win.h>
#elif defined Q_OS_MAC
#include <QMacStyle>
#else
#include <QPlastiqueStyle>
#endif

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace qute_note {

const QString GetApplicationPersistentStoragePath()
{
#if QT_VERSION >= 0x050000
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
}

const QString GetTemporaryStoragePath()
{
#if QT_VERSION >= 0x50000
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
    return QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif
}

QStyle * GetApplicationStyle()
{
    QStyle * appStyle = QApplication::style();
    if (appStyle) {
        return appStyle;
    }

    QNINFO("Application style is empty, will try to deduce some default style");

#ifdef Q_OS_WIN
    // FIXME: figure out why QWindowsStyle doesn't compile
    return nullptr;
#endif

#ifdef Q_OS_MAC
    return new QMacStyle;
#endif

    const QStringList styleNames = QStyleFactory::keys();
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
    if (styleNames.isEmpty()) {
        QNINFO("No valid styles were found in QStyleFactory! Fallback to the last resort of plastique style");
        return new QPlastiqueStyle;
    }
#endif

    const QString & firstStyle = styleNames.first();
    return QStyleFactory::create(firstStyle);
}

} // namespace qute_note

