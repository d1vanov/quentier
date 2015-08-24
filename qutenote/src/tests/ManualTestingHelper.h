#ifndef __QUTE_NOTE__TESTS__MANUAL_TESTING_HELPER_H
#define __QUTE_NOTE__TESTS__MANUAL_TESTING_HELPER_H

#include <QString>

namespace qute_note {
namespace test {

// Collection of various stuff useful for manual testing
class ManualTestingHelper
{
public:
    static QString noteContentWithEncryption();
    static QString noteContentWithResources();
};

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__TESTS__MANUAL_TESTING_HELPER_H

