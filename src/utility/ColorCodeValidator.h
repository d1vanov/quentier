/*
 * Copyright 2018 Dmitry Ivanov
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

#ifndef QUENTIER_UTILITIES_COLOR_CODE_VALIDATOR_H
#define QUENTIER_UTILITIES_COLOR_CODE_VALIDATOR_H

#include <quentier/utility/Macros.h>
#include <QValidator>

namespace quentier {

/**
 * The ColorCodeValidator class represents the custom QValidator
 * subclass implementing validation for color code input of either
 * of the following forms:
 *
 * #RGB (each of R, G, and B is a single hex digit)
 * #RRGGBB
 * #RRRGGGBBB
 * #RRRRGGGGBBBB
 */
class ColorCodeValidator: public QValidator
{
    Q_OBJECT
public:
    explicit ColorCodeValidator(QObject * parent = Q_NULLPTR);

    virtual void fixup(QString & input) const Q_DECL_OVERRIDE;
    virtual State validate(QString & input, int & pos) const Q_DECL_OVERRIDE;

private:
    bool isHexDigit(const QChar & chr) const;
};

} // namespace quentier

#endif // QUENTIER_UTILITIES_COLOR_CODE_VALIDATOR_H
