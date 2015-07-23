#ifndef __QUTE_NOTE__CORE__TESTS__ENML_CONVERTER_TESTS_H
#define __QUTE_NOTE__CORE__TESTS__ENML_CONVERTER_TESTS_H

#include <QString>

namespace qute_note {
namespace test {

bool convertSimpleNoteToHtmlAndBack(QString & error);
bool convertNoteWithToDoTagsToHtmlAndBack(QString & error);
bool convertNoteWithEncryptionToHtmlAndBack(QString & error);
bool convertNoteWithResourcesToHtmlAndBack(QString & error);
bool convertComplexNoteToHtmlAndBack(QString & error);
bool convertComplexNote2ToHtmlAndBack(QString & error);
bool convertComplexNote3ToHtmlAndBack(QString & error);
bool convertComplexNote4ToHtmlAndBack(QString & error);

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TESTS__ENML_CONVERTER_TESTS_H
