#ifndef LIB_QUENTIER_TESTS_CORE_TESTER_H
#define LIB_QUENTIER_TESTS_CORE_TESTER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>

namespace quentier {
namespace test {

class CoreTester: public QObject
{
    Q_OBJECT
public:
    explicit CoreTester(QObject * parent = Q_NULLPTR);
    virtual ~CoreTester();

private slots:
    void initTestCase();

    void noteContainsToDoTest();
    void noteContainsEncryptionTest();

    void encryptDecryptNoteTest();
    void decryptNoteAesTest();
    void decryptNoteRc2Test();

    void enmlConverterSimpleTest();
    void enmlConverterToDoTest();
    void enmlConverterEnCryptTest();
    void enmlConverterEnCryptWithModifiedDecryptedTextTest();
    void enmlConverterEnMediaTest();
    void enmlConverterComplexTest();
    void enmlConverterComplexTest2();
    void enmlConverterComplexTest3();
    void enmlConverterComplexTest4();
    void enmlConverterHtmlWithTableHelperTags();
    void enmlConverterHtmlWithTableAndHilitorHelperTags();

    void resourceRecognitionIndicesParsingTest();

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

} // namespace test
} // namespace quentier

#endif // LIB_QUENTIER_TESTS_CORE_TESTER_H
