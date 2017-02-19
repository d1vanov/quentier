/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/utility/DesktopServices.h>
#include <quentier/logging/QuentierLogger.h>
#include <QStyleFactory>
#include <QApplication>
#include <QScopedPointer>
#include <QUrl>

#ifdef Q_OS_WIN

#include <qwindowdefs.h>
#include <QtGui/qwindowdefs_win.h>
#include <windows.h>
#include <Lmcons.h>

#define SECURITY_WIN32
#include <security.h>

#else

#if defined Q_OS_MAC
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QMacStyle>
#endif // QT_VERSION
#else // defined Q_OS_MAC
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QPlastiqueStyle>
#endif
#endif // defined Q_OS_MAX

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif

#endif // defined Q_OS_WIN

#include <QDesktopServices>
#include <QFile>

namespace quentier {

const QString applicationPersistentStoragePath()
{
#if defined(Q_OS_MAC) || defined (Q_OS_WIN)
    // FIXME: clarify in which version the enum item was actually renamed
    // Seriously, WTF is going on? Why the API gets changed within the major release?
    // Who is the moron who has authorized that?
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#endif
#else
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
#else // Linux, BSD-derivatives etc
    QString storagePath;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    storagePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#else
    storagePath = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
#endif
    storagePath += QStringLiteral("/.") + QApplication::applicationName().toLower();
    return storagePath;
#endif // Q_OS_<smth>
}

const QString applicationTemporaryStoragePath()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
    return QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif
}

const QString homePath()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#else
    return QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
#endif
}

QStyle * applicationStyle()
{
    QStyle * appStyle = QApplication::style();
    if (appStyle) {
        return appStyle;
    }

    QNINFO("Application style is empty, will try to deduce some default style");

#ifdef Q_OS_WIN
    // FIXME: figure out why QWindowsStyle doesn't compile
    return Q_NULLPTR;
#else

#if defined(Q_OS_MAC) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    return new QMacStyle;
#else

    const QStringList styleNames = QStyleFactory::keys();
#if !defined(Q_OS_MAC) && (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    if (styleNames.isEmpty()) {
        QNINFO(QStringLiteral("No valid styles were found in QStyleFactory! Fallback to the last resort of plastique style"));
        return new QPlastiqueStyle;
    }

    const QString & firstStyle = styleNames.first();
    return QStyleFactory::create(firstStyle);
#else
    return Q_NULLPTR;
#endif // !defined(Q_OS_MAC) && (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))

#endif // defined(Q_OS_MAC) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#endif // Q_OS_WIN
}

const QString humanReadableSize(const quint64 bytes)
{
    QStringList list;
    list << QStringLiteral("Kb") << QStringLiteral("Mb") << QStringLiteral("Gb") << QStringLiteral("Tb");

    QStringListIterator it(list);
    QString unit = QStringLiteral("bytes");

    double num = static_cast<double>(bytes);
    while(num >= 1024.0 && it.hasNext()) {
        unit = it.next();
        num /= 1024.0;
    }

    QString result = QString::number(num, 'f', 2);
    result += QStringLiteral(" ");
    result += unit;

    return result;
}

int messageBoxImplementation(const QMessageBox::Icon icon, QWidget * parent,
                             const QString & title, const QString & briefText,
                             const QString & detailedText, QMessageBox::StandardButtons buttons)
{
    QScopedPointer<QMessageBox> pMessageBox(new QMessageBox(parent));
    if (parent) {
        pMessageBox->setWindowModality(Qt::WindowModal);
    }

    pMessageBox->setWindowTitle(QApplication::applicationName() + " - " + title);
    pMessageBox->setText(briefText);
    if (!detailedText.isEmpty()) {
        pMessageBox->setInformativeText(detailedText);
    }

    pMessageBox->setIcon(icon);
    pMessageBox->setStandardButtons(buttons);
    return pMessageBox->exec();
}

int genericMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                       const QString & detailedText, const QMessageBox::StandardButtons buttons)
{
    return messageBoxImplementation(QMessageBox::NoIcon, parent, title, briefText,
                                    detailedText, buttons);
}

int informationMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                          const QString & detailedText, const QMessageBox::StandardButtons buttons)
{
    return messageBoxImplementation(QMessageBox::Information, parent, title, briefText,
                                    detailedText, buttons);
}

int warningMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                      const QString & detailedText, const QMessageBox::StandardButtons buttons)
{
    return messageBoxImplementation(QMessageBox::Warning, parent, title, briefText,
                                    detailedText, buttons);
}

int criticalMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                        const QString & detailedText, const QMessageBox::StandardButtons buttons)
{
    return messageBoxImplementation(QMessageBox::Critical, parent, title, briefText,
                                    detailedText, buttons);
}

int questionMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                        const QString & detailedText, const QMessageBox::StandardButtons buttons)
{
    return messageBoxImplementation(QMessageBox::Question, parent, title, briefText,
                                    detailedText, buttons);
}

void internalErrorMessageBox(QWidget * parent, QString detailedText)
{
    if (!detailedText.isEmpty()) {
        detailedText.prepend(QObject::tr("Technical details on the issue: "));
    }

    criticalMessageBox(parent, QObject::tr("Internal error"),
                       QObject::tr("Unfortunately, ") + QApplication::applicationName() + " " +
                       QObject::tr("encountered internal error. Please report the bug "
                                   "to the developers and try restarting the application"),
                       detailedText);
}

const QString getExistingFolderDialog(QWidget * parent, const QString & title,
                                      const QString & initialFolder,
                                      QFileDialog::Options options)
{
    QScopedPointer<QFileDialog> pFileDialog(new QFileDialog(parent));
    if (parent) {
        pFileDialog->setWindowModality(Qt::WindowModal);
    }

    pFileDialog->setWindowTitle(title);
    pFileDialog->setDirectory(initialFolder);
    pFileDialog->setOptions(options);
    int res = pFileDialog->exec();
    if (res == QDialog::Accepted) {
        return pFileDialog->directory().absolutePath();
    }
    else {
        return QString();
    }
}

const QString relativePathFromAbsolutePath(const QString & absolutePath, const QString & relativePathRootFolder)
{
    QNDEBUG(QStringLiteral("relativePathFromAbsolutePath: ") << absolutePath);

    int position = absolutePath.indexOf(relativePathRootFolder, 0,
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
                                        Qt::CaseInsensitive
#else
                                        Qt::CaseSensitive
#endif
                                        );
    if (position < 0) {
        QNINFO(QStringLiteral("Can't find folder ") << relativePathRootFolder << QStringLiteral(" within path ") << absolutePath);
        return QString();
    }

    return absolutePath.mid(position + relativePathRootFolder.length() + 1);   // NOTE: additional symbol for slash
}

const QString getCurrentUserName()
{
    QNDEBUG(QStringLiteral("getCurrentUserName"));

    QString userName;

#if defined(Q_OS_WIN)
    TCHAR acUserName[UNLEN+1];
    DWORD nUserName = sizeof(acUserName);
    if (GetUserName(acUserName, &nUserName)) {
        userName = static_cast<LPSTR>(acUserName);
    }
#else
    uid_t uid = geteuid();
    struct passwd * pw = getpwuid(uid);
    if (pw) {
        userName = QString::fromLocal8Bit(pw->pw_name);
    }
#endif

    if (userName.isEmpty())
    {
        QNTRACE(QStringLiteral("Native platform API failed to provide the username, trying environment variables fallback"));

        userName = qgetenv("USER");
        if (userName.isEmpty()) {
            userName = qgetenv("USERNAME");
        }
    }

    QNTRACE(QStringLiteral("Username = ") << userName);
    return userName;
}

const QString getCurrentUserFullName()
{
    QNDEBUG(QStringLiteral("getCurrentUserFullName"));

    QString userFullName;

#if defined(Q_OS_WIN)
    TCHAR acUserName[UNLEN+1];
    DWORD nUserName = sizeof(acUserName);
    bool res = GetUserNameEx(NameDisplay, acUserName, &nUserName);
    if (res) {
        userFullName = static_cast<LPSTR>(acUserName);
    }

    if (userFullName.isEmpty())
    {
        // GetUserNameEx with NameDisplay format doesn't work when the computer is offline.
        // It's serious. Take a look here: http://stackoverflow.com/a/2997257
        // I've never had any serious business with WinAPI but I nearly killed myself
        // with a facepalm when I figured this thing out. God help Microsoft - nothing else will.
        //
        // Falling back to the login name
        userFullName = getCurrentUserName();
     }
#else
    uid_t uid = geteuid();
    struct passwd * pw = getpwuid(uid);
    if (Q_LIKELY(pw))
    {
        struct passwd * pwf = getpwnam(pw->pw_name);
        if (Q_LIKELY(pwf)) {
            userFullName = QString::fromLocal8Bit(pwf->pw_gecos);
        }
    }

    // NOTE: some Unix systems put more than full user name into this field but also something about the location etc.
    // The convention is to use comma to split the values of different kind and the user's full name is the first one
    int commaIndex = userFullName.indexOf(QChar(','));
    if (commaIndex > 0) {   // NOTE: not >= but >
        userFullName.truncate(commaIndex);
    }
#endif

    return userFullName;
}

void openUrl(const QUrl url)
{
    QNDEBUG(QStringLiteral("openUrl: ") << url);
    QDesktopServices::openUrl(url);
}

bool removeFile(const QString & filePath)
{
    QNDEBUG(QStringLiteral("removeFile: ") << filePath);

    QFile file(filePath);
    file.close();   // NOTE: this line seems to be mandatory on Windows
    bool res = file.remove();
    if (res) {
        QNTRACE(QStringLiteral("Successfully removed file ") << filePath);
        return true;
    }

#ifdef Q_OS_WIN
    if (filePath.endsWith(QStringLiteral(".lnk"))) {
        // NOTE: there appears to be a bug in Qt for Windows, QFile::remove returns false
        // for any *.lnk files even though the files are actually getting removed
        QNTRACE(QStringLiteral("Skipping the reported failure at removing the .lnk file"));
        return true;
    }
#endif

    QNWARNING(QStringLiteral("Cannot remove file ") << filePath << QStringLiteral(": ") << file.errorString()
              << QStringLiteral(", error code ") << file.error());
    return false;
}

} // namespace quentier

