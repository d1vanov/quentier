#ifndef __QUTE_NOTE__CORE__TESTS__CORE_TESTER_H
#define __QUTE_NOTE__CORE__TESTS__CORE_TESTER_H

#include <tools/qt4helper.h>
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
    void initTestCase();

    void resourceRecognitionIndicesTest();
    void noteContainsToDoTest();
    void noteContainsEncryptionTest();

    void encryptDecryptNoteTest();
    void decryptNoteAesTest();
    void decryptNoteRc2Test();

    void enmlConverterSimpleTest();
    void enmlConverterToDoTest();

    void noteSearchQueryTest();
    void localStorageManagerNoteSearchQueryTest();

    void localStorageManagerIndividualSavedSearchTest();
    void localStorageManagerIndividualLinkedNotebookTest();
    void localStorageManagerIndividualTagTest();
    void localStorageManagerIndividualResourceTest();
    void localStorageManagedIndividualNoteTest();
    void localStorageManagerIndividualNotebookTest();
    void localStorageManagedIndividualUserTest();

    void localStorageManagerSequentialUpdatesTest();

    void localStorageManagerListSavedSearchesTest();
    void localStorageManagerListLinkedNotebooksTest();
    void localStorageManagerListTagsTest();
    void localStorageManagerListAllSharedNotebooksTest();
    void localStorageManagerListAllTagsPerNoteTest();
    void localStorageManagerListNotesTest();
    void localStorageManagerListNotebooksTest();

    void localStorageManagerExpungeNotelessTagsFromLinkedNotebooksTest();

    void localStorageManagerAsyncSavedSearchesTest();
    void localStorageManagerAsyncLinkedNotebooksTest();
    void localStorageManagerAsyncTagsTest();
    void localStorageManagerAsyncUsersTest();
    void localStorageManagerAsyncNotebooksTest();
    void localStorageManagerAsyncNotesTest();
    void localStorageManagerAsyncResourceTest();

    void localStorageCacheManagerTest();

private:
    CoreTester(const CoreTester & other) Q_DECL_DELETE;
    CoreTester & operator=(const CoreTester & other) Q_DECL_DELETE;

};

}
}

#endif // __QUTE_NOTE__CORE__TESTS__CORE_TESTER_H
