#ifndef QUENTIER_TESTS_MANUAL_TESTING_HELPER_H
#define QUENTIER_TESTS_MANUAL_TESTING_HELPER_H

#include <QString>

namespace quentier {
namespace test {

// Collection of various stuff useful for manual testing
class ManualTestingHelper
{
public:
    static QString noteContentWithEncryption();
    static QString noteContentWithResources();
};

} // namespace test
} // namespace quentier

#endif // QUENTIER_TESTS_MANUAL_TESTING_HELPER_H

