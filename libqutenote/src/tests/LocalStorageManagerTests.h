#ifndef LIB_QUTE_NOTE_TESTS_LOCAL_STORAGE_MANAGER_TESTS_H
#define LIB_QUTE_NOTE_TESTS_LOCAL_STORAGE_MANAGER_TESTS_H

#include <QtGlobal>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManager)
QT_FORWARD_DECLARE_CLASS(SavedSearch)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(IUser)

namespace test {

bool TestSavedSearchAddFindUpdateExpungeInLocalStorage(SavedSearch & search,
                                                       LocalStorageManager & localStorageManager,
                                                       QString & errorDescription);

bool TestLinkedNotebookAddFindUpdateExpungeInLocalStorage(const LinkedNotebook & linkedNotebook,
                                                          LocalStorageManager & localStorageManager,
                                                          QString & errorDescription);

bool TestTagAddFindUpdateExpungeInLocalStorage(Tag & tag, LocalStorageManager & localStorageManager,
                                               QString & errorDescription);

bool TestResourceAddFindUpdateExpungeInLocalStorage(IResource & resource,
                                                    const Note & note,
                                                    LocalStorageManager & localStorageManager,
                                                    QString & errorDescription);

bool TestNoteFindUpdateDeleteExpungeInLocalStorage(Note & note, const Notebook & notebook,
                                                   LocalStorageManager & localStorageManager,
                                                   QString & errorDescription);

bool TestNotebookFindUpdateDeleteExpungeInLocalStorage(Notebook & notebook,
                                                       LocalStorageManager & localStorageManager,
                                                       QString & errorDescription);

bool TestUserAddFindUpdateDeleteExpungeInLocalStorage(const IUser & user,
                                                      LocalStorageManager & localStorageManager,
                                                      QString & errorDescription);

bool TestSequentialUpdatesInLocalStorage(QString & errorDescription);

} // namespace test
} // namespace qute_note

#endif // LIB_QUTE_NOTE_TESTS_LOCAL_STORAGE_MANAGER_TESTS_H
