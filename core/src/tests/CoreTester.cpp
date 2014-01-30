#include "CoreTester.h"
#include "SerializationTests.h"
#include <QtTest/QTest>

namespace qute_note {
namespace test {

CoreTester::CoreTester(QObject * parent) :
    QObject(parent)
{}

CoreTester::~CoreTester()
{}

#define TEST(component) \
    void CoreTester::serializationTest##component() \
    { \
        QString error; \
        bool res = Test##component##Serialization(error); \
        QVERIFY2(res == true, error.toStdString().c_str()); \
    }

TEST(BusinessUserInfo)
TEST(PremiumInfo)
TEST(Accounting)
TEST(UserAttributes)

#undef TEST

}
}
