#ifndef __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_MANAGER_TESTS_H
#define __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_MANAGER_TESTS_H

#include <QtGlobal>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManager)
QT_FORWARD_DECLARE_CLASS(SavedSearch)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)
QT_FORWARD_DECLARE_CLASS(Tag)

namespace test {

bool TestSavedSearchAddFindUpdateExpungeInLocalStorage(const SavedSearch & search,
                                                       LocalStorageManager & localStorageManager,
                                                       QString & errorDescription);

bool TestLinkedNotebookAddFindUpdateExpungeInLocalStorage(const LinkedNotebook & linkedNotebook,
                                                          LocalStorageManager & localStorageManager,
                                                          QString & errorDescription);

bool TestTagAddFindUpdateExpungeInLocalStorage(const Tag & tag, LocalStorageManager & localStorageManager,
                                               QString & errorDescription);

}
}

#endif // __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_MANAGER_TESTS_H
