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

void CoreTester::serializationTests()
{
    QString error;
    bool res = TestBusinessUserInfoSerialization(error);
    QVERIFY2(res == true, error.toStdString().c_str());
}

}
}
