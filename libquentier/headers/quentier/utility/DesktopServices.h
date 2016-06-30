#ifndef LIB_QUENTIER_UTILITY_DESKTOP_SERVICES_H
#define LIB_QUENTIER_UTILITY_DESKTOP_SERVICES_H

#include <quentier/utility/Linkage.h>
#include <QString>
#include <QStyle>
#include <QFileDialog>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace quentier {

// Convenience functions for some paths important for the application
const QString QUENTIER_EXPORT applicationPersistentStoragePath();
const QString QUENTIER_EXPORT applicationTemporaryStoragePath();
const QString QUENTIER_EXPORT homePath();

QUENTIER_EXPORT QStyle * applicationStyle();
const QString QUENTIER_EXPORT humanReadableSize(const quint64 bytes);

// Convenience functions for message boxes with proper modality on Mac OS X
void QUENTIER_EXPORT genericMessageBox(QWidget * parent, const QString & title,
                                        const QString & briefText, const QString & detailedText = QString());
void QUENTIER_EXPORT informationMessageBox(QWidget * parent, const QString & title,
                                            const QString & briefText, const QString & detailedText = QString());
void QUENTIER_EXPORT warningMessageBox(QWidget * parent, const QString & title,
                                        const QString & briefText, const QString & detailedText = QString());
void QUENTIER_EXPORT criticalMessageBox(QWidget * parent, const QString & title,
                                         const QString & briefText, const QString & detailedText = QString());
void QUENTIER_EXPORT questionMessageBox(QWidget * parent, const QString & title,
                                         const QString & briefText, const QString & detailedText = QString());

// Convenience function for critical message box due to internal error, has built-in title
// ("Internal error") and brief text so the caller only needs to provide the detailed text
void QUENTIER_EXPORT internalErrorMessageBox(QWidget * parent, QString detailedText = QString());

// Convenience function for file dialogues with proper modality on Mac OS X
const QString QUENTIER_EXPORT getExistingFolderDialog(QWidget * parent, const QString & title,
                                                       const QString & initialFolder,
                                                       QFileDialog::Options options = QFileDialog::ShowDirsOnly);

// Convenience function to convert the absolute path to the relative one with respect to the given folder name
const QString QUENTIER_EXPORT relativePathFromAbsolutePath(const QString & absolutePath,
                                                            const QString & relativePathRootFolder);

// Convenience function to send URL opening request
void QUENTIER_EXPORT openUrl(const QUrl url);

// Convenience function to remove a file automatically printing the error to warning and workarounding some platform specific quirks
bool QUENTIER_EXPORT removeFile(const QString & filePath);

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_DESKTOP_SERVICES_H
