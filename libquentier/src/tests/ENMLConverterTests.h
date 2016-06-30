#ifndef LIB_QUENTIER_TESTS_ENML_CONVERTER_TESTS_H
#define LIB_QUENTIER_TESTS_ENML_CONVERTER_TESTS_H

#include <QString>

namespace quentier {
namespace test {

bool convertSimpleNoteToHtmlAndBack(QString & error);
bool convertNoteWithToDoTagsToHtmlAndBack(QString & error);
bool convertNoteWithEncryptionToHtmlAndBack(QString & error);
bool convertNoteWithResourcesToHtmlAndBack(QString & error);
bool convertComplexNoteToHtmlAndBack(QString & error);
bool convertComplexNote2ToHtmlAndBack(QString & error);
bool convertComplexNote3ToHtmlAndBack(QString & error);
bool convertComplexNote4ToHtmlAndBack(QString & error);
bool convertHtmlWithModifiedDecryptedTextToEnml(QString & error);
bool convertHtmlWithTableHelperTagsToEnml(QString & error);
bool convertHtmlWithTableAndHilitorHelperTagsToEnml(QString & error);

} // namespace test
} // namespace quentier

#endif // LIB_QUENTIER_TESTS_ENML_CONVERTER_TESTS_H
