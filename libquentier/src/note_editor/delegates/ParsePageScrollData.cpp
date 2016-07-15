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

#include "ParsePageScrollData.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

bool parsePageScrollData(const QVariant & data, int & pageXOffset, int & pageYOffset, QNLocalizedString & errorDescription)
{
    QStringList dataStrList = data.toStringList();
    const int numDataItems = dataStrList.size();

    if (Q_UNLIKELY(numDataItems != 2)) {
        errorDescription = QT_TR_NOOP("can't find note editor page's scroll: unexpected number of items "
                                      "received from JavaScript side, expected 2, got");
        errorDescription += " ";
        errorDescription += QString::number(numDataItems);
        return false;
    }

    bool conversionResult = 0;
    int x = dataStrList[0].toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        errorDescription = QT_TR_NOOP("can't find note editor page's scroll: can't convert x scroll coordinate to number");
        return false;
    }

    conversionResult = false;
    int y = dataStrList[1].toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        errorDescription = QT_TR_NOOP("can't find note editor page's scroll: can't convert y scroll coordinate to number");
        return false;
    }

    pageXOffset = x;
    pageYOffset = y;
    QNTRACE("Page scroll: x = " << pageXOffset << ", y = " << pageYOffset);

    return true;
}

} // namespace quentier
