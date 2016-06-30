#include "ParsePageScrollData.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

bool parsePageScrollData(const QVariant & data, int & pageXOffset, int & pageYOffset, QString & errorDescription)
{
    QStringList dataStrList = data.toStringList();
    const int numDataItems = dataStrList.size();

    if (Q_UNLIKELY(numDataItems != 2)) {
        errorDescription = QT_TR_NOOP("can't find note editor page's scroll: unexpected number of items "
                                      "received from JavaScript side, expected 2, got ") + QString::number(numDataItems);
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
