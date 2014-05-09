#ifndef __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H
#define __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H

#include <QtGlobal>
QT_FORWARD_DECLARE_CLASS(QString)

namespace qute_note {
namespace test {

bool TestNoteAttributesSerialization(QString & errorDescription);

bool TestResourceAttributesSerialization(QString & errorDescription);

}
}

#endif // __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H
