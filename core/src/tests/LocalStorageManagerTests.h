#ifndef __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_MANAGER_TESTS_H
#define __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_MANAGER_TESTS_H

#include <QtGlobal>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(LocalStorageManager)
QT_FORWARD_DECLARE_CLASS(SavedSearch)

namespace test {

bool TestSavedSearchAddFindInLocalStorage(const SavedSearch & search,
                                          LocalStorageManager & localStorageManager,
                                          QString & errorDescription);

}
}

#endif // __QUTE_NOTE__CORE__TESTS__LOCAL_STORAGE_MANAGER_TESTS_H
