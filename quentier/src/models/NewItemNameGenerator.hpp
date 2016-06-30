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
