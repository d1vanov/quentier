#ifndef __QUTE_NOTE__CORE__TESTS__CORE_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__CORE_TESTER_H

#include <QObject>

namespace qute_note {
namespace test {

class CoreTester: public QObject
{
    Q_OBJECT
public:
    explicit CoreTester(QObject * parent = nullptr);
    virtual ~CoreTester();

private slots:
    void serializationTestBusinessUserInfo();
    void serializationTestPremiumInfo();
    void serializationTestAccounting();
    void serializationTestUserAttributes();
    void serializationTestNoteAttributes();
    void serializationTestResourceAttributes();

    void localStorageManagerIndividualSavedSearchTest();
    void localStorageManagerIndividualLinkedNotebookTest();
    void localStorageManagerIndividualTagTest();
    void localStorageManagerIndividualResourceTest();
    void localStorageManagedIndividualNoteTest();
    void localStorageManagerIndividualNotebookTest();

    void localStorageManagerListAllSavedSearchesTest();
    void localStorageManagerListAllTagsTest();
    void localStorageManagerListAllLinkedNotebooksTest();
    void localStorageManagerListAllSharedNotebooksTest();
    void localStorageManagerListAllTagsPerNoteTest();
    void localStorageManagerListAllNotesPerNotebookTest();

    // TODO: add test for LocalStorageManager::ListAllNotebooks
    // TODO: add test for LocalStorageManager::ExpungeUser
    // TODO: add test for LocalStorageManager::DeleteUser
    // TODO: add test for LocalStorageManager::FindBusinessUserInfo
    // TODO: add test for LocalStorageManager::FindPremiumInfo
    // TODO: add test for LocalStorageManager::FindAccounting
    // TODO: add test for LocalStorageManager::FindUserAttributes
    // TODO: add test for LocalStorageManager::FindUser
    // TODO: add test for LocalStorageManager::UpdateUser
    // TODO: add test for LocalStorageManager::AddUser
    // TODO: add test for LocalStorageManager::SwitchUser

private:
    CoreTester(const CoreTester & other) = delete;
    CoreTester & operator=(const CoreTester & other) = delete;

};

}
}

#endif // __QUTE_NOTE__CORE__TESTS__CORE_TESTER_H
