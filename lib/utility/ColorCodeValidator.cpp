/*
 * Copyright 2018-2021 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ColorCodeValidator.h"

namespace quentier {

namespace {

[[nodiscard]] bool isHexDigit(const QChar & chr) noexcept
{
    if (chr.isDigit()) {
        return true;
    }

    return (chr == QChar::fromLatin1('A')) || (chr == QChar::fromLatin1('B')) ||
        (chr == QChar::fromLatin1('C')) || (chr == QChar::fromLatin1('D')) ||
        (chr == QChar::fromLatin1('E')) || (chr == QChar::fromLatin1('F'));
}

const QChar sharp = QChar::fromLatin1('#');

} // namespace

ColorCodeValidator::ColorCodeValidator(QObject * parent) : QValidator(parent) {}

void ColorCodeValidator::fixup(QString & input) const
{
    if (input.isEmpty()) {
        return;
    }

    if (!input.startsWith(sharp)) {
        input.prepend(sharp);
    }

    for (int i = 1; i < input.size(); ++i) {
        if (!isHexDigit(input[i].toUpper())) {
            input.remove(i, 1);
        }
    }
}

QValidator::State ColorCodeValidator::validate(QString & input, int & pos) const
{
    Q_UNUSED(pos)

    if (input.isEmpty()) {
        return QValidator::Acceptable;
    }

    if (!input.startsWith(sharp)) {
        return QValidator::Invalid;
    }

    const int size = input.size();
    for (int i = 1; i < size; ++i) {
        if (!isHexDigit(input[i].toUpper())) {
            return QValidator::Invalid;
        }
    }

    if (size < 4) {
        return QValidator::Intermediate;
    }

    if (size == 4) {
        return QValidator::Acceptable;
    }

    if (size < 7) {
        return QValidator::Intermediate;
    }

    if (size == 7) {
        return QValidator::Acceptable;
    }

    if (size < 10) {
        return QValidator::Intermediate;
    }

    if (size == 10) {
        return QValidator::Acceptable;
    }

    if (size < 13) {
        return QValidator::Intermediate;
    }

    if (size == 13) {
        return QValidator::Acceptable;
    }

    return QValidator::Invalid;
}

} // namespace quentier
