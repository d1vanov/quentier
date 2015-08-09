#include <qute_note/utility/DesktopServices.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QStyleFactory>
#include <QApplication>
#include <QMessageBox>
#include <QScopedPointer>

#ifdef Q_OS_WIN
#include <qwindowdefs.h>
#include <QtGui/qwindowdefs_win.h>
#elif defined Q_OS_MAC
#include <QMacStyle>
#else

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#include <QPlastiqueStyle>
#endif

#endif

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

namespace qute_note {

const QString applicationPersistentStoragePath()
{
#if QT_VERSION >= 0x050000
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
}

const QString applicationTemporaryStoragePath()
{
#if QT_VERSION >= 0x50000
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
    return QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif
}

const QString homePath()
{
#if QT_VERSION >= 0x50000
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
    return nullptr;
#endif

#ifdef Q_OS_MAC
    return new QMacStyle;
#endif

    const QStringList styleNames = QStyleFactory::keys();
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC) && (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    if (styleNames.isEmpty()) {
        QNINFO("No valid styles were found in QStyleFactory! Fallback to the last resort of plastique style");
        return new QPlastiqueStyle;
    }
#elif QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    return nullptr;
#endif

    const QString & firstStyle = styleNames.first();
    return QStyleFactory::create(firstStyle);
}

const QString humanReadableSize(const int bytes)
{
    QStringList list;
    list << "Kb" << "Mb" << "Gb" << "Tb";

    QStringListIterator it(list);
    QString unit("bytes");

    double num = static_cast<double>(bytes);
    while(num >= 1024.0 && it.hasNext()) {
        unit = it.next();
        num /= 1024.0;
    }

    QString result = QString::number(num, 'f', 2);
    result += " ";
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

} // namespace qute_note

