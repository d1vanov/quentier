#ifndef LIB_QUENTIER_TESTS_ENCRYPTION_MANAGER_TESTS_H
#define LIB_QUENTIER_TESTS_ENCRYPTION_MANAGER_TESTS_H

#include <QString>

namespace quentier {
namespace test {

bool encryptDecryptTest(QString & error);
bool decryptAesTest(QString & error);
bool decryptRc2Test(QString & error);

} // namespace test
} // namespace quentier

#endif // LIB_QUENTIER_TESTS_ENCRYPTION_MANAGER_TESTS_H
