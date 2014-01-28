#include "CoreTester.h"
#include <QtTest/QtTest>

using namespace qute_note::test;

int main()
{
    QTest::qExec(new CoreTester);
    return 0;
}
