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
    // void localStorageManagerIndividualNotebookTest();

private:
    CoreTester(const CoreTester & other) = delete;
    CoreTester & operator=(const CoreTester & other) = delete;

};

}
}

#endif // __QUTE_NOTE__CORE__TESTS__CORE_TESTER_H
