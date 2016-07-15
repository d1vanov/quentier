/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

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
