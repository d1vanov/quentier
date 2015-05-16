#ifndef __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H
#define __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H

#include "Linkage.h"
#include <QString>
#include <QStyle>
#include <QFileDialog>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace qute_note {

// Convenience functions for some paths important for the application
const QString QUTE_NOTE_EXPORT applicationPersistentStoragePath();
const QString QUTE_NOTE_EXPORT applicationTemporaryStoragePath();
const QString QUTE_NOTE_EXPORT homePath();

QUTE_NOTE_EXPORT QStyle * applicationStyle();
const QString QUTE_NOTE_EXPORT humanReadableSize(const int bytes);

// Convenience functions for message boxes with proper modality on Mac OS X
void QUTE_NOTE_EXPORT genericMessageBox(QWidget * parent, const QString & title,
                                        const QString & briefText, const QString & detailedText = QString());
void QUTE_NOTE_EXPORT informationMessageBox(QWidget * parent, const QString & title,
                                            const QString & briefText, const QString & detailedText = QString());
void QUTE_NOTE_EXPORT warningMessageBox(QWidget * parent, const QString & title,
                                        const QString & briefText, const QString & detailedText = QString());
void QUTE_NOTE_EXPORT criticalMessageBox(QWidget * parent, const QString & title,
                                         const QString & briefText, const QString & detailedText = QString());
void QUTE_NOTE_EXPORT questionMessageBox(QWidget * parent, const QString & title,
                                         const QString & briefText, const QString & detailedText = QString());

// Convenience function for critical message box due to internal error, has built-in title
// ("Internal error") and brief text so the caller only needs to provide the detailed text
void QUTE_NOTE_EXPORT internalErrorMessageBox(QWidget * parent, QString detailedText = QString());

// Convenience functions for file dialogues with proper modality on Mac OS X
const QString QUTE_NOTE_EXPORT getExistingFolderDialog(QWidget * parent, const QString & title,
                                                       const QString & initialFolder,
                                                       QFileDialog::Options options = QFileDialog::ShowDirsOnly);

const QString QUTE_NOTE_EXPORT unsignedCharToQString(const unsigned char * str, const int size = -1);
void QUTE_NOTE_EXPORT appendUnsignedCharToQString(QString & result, const unsigned char * str, const int size = -1);

} // namespace qute_note

#endif // __QUTE__NOTE__CORE__TOOLS__APPLICATION_STORAGE_PERSISTENCE_PATH_H
