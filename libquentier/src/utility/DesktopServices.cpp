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
#include <QMessageBox>
#include <QScopedPointer>
#include <QUrl>

#ifdef Q_OS_WIN
#include <qwindowdefs.h>
#include <QtGui/qwindowdefs_win.h>
#elif defined Q_OS_MAC
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QMacStyle>
#endif
#else

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QPlastiqueStyle>
#endif

#endif

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif
#include <QDesktopServices>
#include <QFile>

namespace quentier {

const QString applicationPersistentStoragePath()
{
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
#endif

#ifdef Q_OS_MAC
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    return new QMacStyle;
#endif
#endif

    const QStringList styleNames = QStyleFactory::keys();
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC) && (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    if (styleNames.isEmpty()) {
        QNINFO(QStringLiteral("No valid styles were found in QStyleFactory! Fallback to the last resort of plastique style"));
        return new QPlastiqueStyle;
    }
#elif QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    return Q_NULLPTR;
#endif

    const QString & firstStyle = styleNames.first();
    return QStyleFactory::create(firstStyle);
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

void messageBoxImplementation(const QMessageBox::Icon icon, QWidget * parent,
                              const QString & title, const QString & briefText,
                              const QString & detailedText)
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
    pMessageBox->addButton(QMessageBox::Ok);
    pMessageBox->exec();
}

void genericMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                       const QString & detailedText)
{
    messageBoxImplementation(QMessageBox::NoIcon, parent, title, briefText, detailedText);
}

void informationMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                           const QString & detailedText)
{
    messageBoxImplementation(QMessageBox::Information, parent, title, briefText, detailedText);
}

void warningMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                       const QString & detailedText)
{
    messageBoxImplementation(QMessageBox::Warning, parent, title, briefText, detailedText);
}

void criticalMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                        const QString & detailedText)
{
    messageBoxImplementation(QMessageBox::Critical, parent, title, briefText, detailedText);
}

void questionMessageBox(QWidget * parent, const QString & title, const QString & briefText,
                        const QString & detailedText)
{
    messageBoxImplementation(QMessageBox::Question, parent, title, briefText, detailedText);
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

