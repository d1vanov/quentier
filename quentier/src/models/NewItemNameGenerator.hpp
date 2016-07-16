/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_NEW_ITEM_NAME_GENERATOR_HPP
#define QUENTIER_MODELS_NEW_ITEM_NAME_GENERATOR_HPP

#include <QString>

namespace quentier {

template <class NameIndexType>
QString newItemName(const NameIndexType & nameIndex, int & newItemCounter, QString baseName)
{
    if (newItemCounter != 0) {
        baseName += " (" + QString::number(newItemCounter) + ")";
    }

    while(true)
    {
        auto it = nameIndex.find(baseName.toUpper());
        if (it == nameIndex.end()) {
            return baseName;
        }

        if (newItemCounter != 0) {
            QString numPart = " (" + QString::number(newItemCounter) + ")";
            baseName.chop(numPart.length());
        }

        ++newItemCounter;
        baseName += " (" + QString::number(newItemCounter) + ")";
    }
}

} // namespace quentier

#endif // QUENTIER_MODELS_NEW_ITEM_NAME_GENERATOR_HPP
