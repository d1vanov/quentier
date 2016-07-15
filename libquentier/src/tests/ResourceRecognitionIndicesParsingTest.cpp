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

#include "ResourceRecognitionIndicesParsingTest.h"
#include <quentier/types/ResourceRecognitionIndices.h>
#include <quentier/types/ResourceRecognitionIndexItem.h>
#include <quentier/logging/QuentierLogger.h>
#include <QFile>
#include <QByteArray>

namespace quentier {
namespace test {

bool parseResourceRecognitionIndicesAndItemsTest(QString & error)
{
    QNDEBUG("parseResourceRecognitionIndicesAndItemsTest");

    QFile resource(":/tests/recoIndex-all-in-one-example.xml");
    if (!resource.open(QIODevice::ReadOnly)) {
        error = "Can't open test file " + resource.fileName();
        QNWARNING(error << ", error: " << resource.errorString() << " (error code " << resource.error() << ")");
        return false;
    }

    QByteArray resourceData = resource.readAll();
    resource.close();

    ResourceRecognitionIndices recoIndices;
    bool res = recoIndices.setData(resourceData);
    if (!res) {
        error = "Failed to parse the recognition indices";
        return false;
    }

#define CHECK_INDICES_PROPERTY(accessor, expected, property) \
    if (recoIndices.accessor() != expected) { \
        error = "Incorrectly parsed reco indices " property ": expected \"" expected "\", got \"" + recoIndices.accessor() + "\""; \
        QNWARNING(error << "; reco indices: " << recoIndices); \
        return false; \
    }

    CHECK_INDICES_PROPERTY(docType, "picture", "doc type")
    CHECK_INDICES_PROPERTY(objectType, "ink", "object type")
    CHECK_INDICES_PROPERTY(objectId, "a284273e482578224145f2560b67bf45", "object id")
    CHECK_INDICES_PROPERTY(engineVersion, "3.0.17.14", "engine version")
    CHECK_INDICES_PROPERTY(recoType, "client", "recognition type")
    CHECK_INDICES_PROPERTY(lang, "en", "lang")

    if (recoIndices.objectHeight() != 2592) {
        error = "Incorrectly parsed reco indices object height: expected 2592, got " + QString::number(recoIndices.objectHeight());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    if (recoIndices.objectWidth() != 1936) {
        error = "Incorrectly parsed reco indices object width: expected 1936, got " + QString::number(recoIndices.objectWidth());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    QVector<ResourceRecognitionIndexItem> items = recoIndices.items();
    int numItems = items.size();
    if (numItems != 2) {
        error = "Incorrectly parsed reco indices items: expected 2 items, got " + QString::number(numItems);
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

#define CHECK_ITEM_PROPERTY(item, accessor, expected, property) \
    if (item.accessor() != expected) { \
        error = "Incorrectly parsed recognition item's " property ": expected " + QString::number(expected) + ", got " + QString::number(item.accessor()); \
        QNWARNING(error << "; reco indices: " << recoIndices); \
        return false; \
    }

    const ResourceRecognitionIndexItem & item0 = items[0];
    CHECK_ITEM_PROPERTY(item0, x, 853, "x")
    CHECK_ITEM_PROPERTY(item0, y, 1278, "y")
    CHECK_ITEM_PROPERTY(item0, w, 14, "w")
    CHECK_ITEM_PROPERTY(item0, h, 17, "h")

    QVector<ResourceRecognitionIndexItem::ObjectItem> objectItems0 = item0.objectItems();
    if (!objectItems0.empty()) {
        error = "Incorrectly parsed recognition item's object items: expected none of them, got " + QString::number(objectItems0.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    QVector<ResourceRecognitionIndexItem::ShapeItem> shapeItems0 = item0.shapeItems();
    if (!shapeItems0.empty()) {
        error = "Incorrectly parsed recognition item's shape items: expected none of them, got " + QString::number(shapeItems0.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    QVector<ResourceRecognitionIndexItem::BarcodeItem> barcodeItems0 = item0.barcodeItems();
    if (!barcodeItems0.empty()) {
        error = "Incorrectly parsed recognition item's barcode items: expected none of them, got " + QString::number(barcodeItems0.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    QVector<ResourceRecognitionIndexItem::TextItem> textItems0 = item0.textItems();
    if (textItems0.size() != 4) {
        error = "Incorrectly parsed recognition item's text items: expected 4 text items, got " + QString::number(textItems0.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

#define CHECK_TEXT_ITEM_TEXT(item, global_index, index, expected) \
    const ResourceRecognitionIndexItem::TextItem & item##_textItem##index = textItems##global_index[index]; \
    if (item##_textItem##index.m_text != expected) { \
        error = "Incorrectly parsed recognition item's text item's text: expected " + QString(expected) + QString(", got ") + item##_textItem##index.m_text; \
        QNWARNING(error << "; reco indices: " << recoIndices); \
        return false; \
    }

#define CHECK_TEXT_ITEM_WEIGHT(item, index, expected) \
    if (item##_textItem##index.m_weight != expected) { \
        error = "Incorrectly parsed recognition item's text item's text: expected " #expected ", got " + QString::number(item##_textItem##index.m_weight); \
        QNWARNING(error << "; reco indices: " << recoIndices); \
        return false; \
    }

    CHECK_TEXT_ITEM_TEXT(item0, 0, 0, "II")
    CHECK_TEXT_ITEM_WEIGHT(item0, 0, 31)

    CHECK_TEXT_ITEM_TEXT(item0, 0, 1, "11")
    CHECK_TEXT_ITEM_WEIGHT(item0, 1, 31)

    CHECK_TEXT_ITEM_TEXT(item0, 0, 2, "ll")
    CHECK_TEXT_ITEM_WEIGHT(item0, 2, 31)

    CHECK_TEXT_ITEM_TEXT(item0, 0, 3, "Il")
    CHECK_TEXT_ITEM_WEIGHT(item0, 3, 31)

    const ResourceRecognitionIndexItem & item1 = items[1];
    CHECK_ITEM_PROPERTY(item1, x, 501, "x")
    CHECK_ITEM_PROPERTY(item1, y, 635, "y")
    CHECK_ITEM_PROPERTY(item1, w, 770, "w")
    CHECK_ITEM_PROPERTY(item1, h, 254, "h")
    CHECK_ITEM_PROPERTY(item1, offset, 12, "offset")
    CHECK_ITEM_PROPERTY(item1, duration, 17, "duration")

    QVector<int> strokeList = item1.strokeList();
    if (strokeList.size() != 5) {
        error = "Incorrectly parsed recognition item's stroke list: expected 5 items in the list, got " + QString::number(strokeList.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    bool strokeListOk = strokeList.contains(14) && strokeList.contains(28) && strokeList.contains(19) &&
                        strokeList.contains(41) && strokeList.contains(54);
    if (!strokeListOk) {
        error = "Incorrectly parsed recognition item's stroke list: not all expected numbers are found within the list";
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    QVector<ResourceRecognitionIndexItem::ObjectItem> objectItems1 = item1.objectItems();
    if (objectItems1.size() != 4) {
        error = "Incorrectly parsed recognition item's object items: expected 4, got " + QString::number(objectItems1.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    QVector<ResourceRecognitionIndexItem::ShapeItem> shapeItems1 = item1.shapeItems();
    if (shapeItems1.size() != 4) {
        error = "Incorrectly parsed recognition item's shape items: expected 4, got " + QString::number(shapeItems1.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    QVector<ResourceRecognitionIndexItem::BarcodeItem> barcodeItems1 = item1.barcodeItems();
    if (barcodeItems1.size() != 3) {
        error = "Incorrectly parsed recognition item's barcode items: expected 3, got " + QString::number(barcodeItems1.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

    QVector<ResourceRecognitionIndexItem::TextItem> textItems1 = item1.textItems();
    if (textItems1.size() != 11) {
        error = "Incorrectly parsed recognition item's barcode items: expected 11, got " + QString::number(textItems1.size());
        QNWARNING(error << "; reco indices: " << recoIndices);
        return false;
    }

#define CHECK_SUB_ITEM_PROPERTY(subItemType, subItems, index, property, expected, ...) \
    { \
        const ResourceRecognitionIndexItem::subItemType & checkedItem = subItems[index]; \
        if (checkedItem.property != expected) { \
            error = "Incorrectly parsed recognition item's " #subItemType " " #property ": expected " + QString(__VA_ARGS__(expected)) + \
                    QString(", got ") + __VA_ARGS__(checkedItem.property); \
            QNWARNING(error << "; reco indices: " << recoIndices); \
            return false; \
        } \
    }

    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 0, m_text, "LONG")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 0, m_weight, 32, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 1, m_text, "LONG")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 1, m_weight, 25, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 2, m_text, "GOV")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 2, m_weight, 23, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 3, m_text, "NOV")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 3, m_weight, 23, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 4, m_text, "Lang")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 4, m_weight, 19, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 5, m_text, "lane")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 5, m_weight, 18, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 6, m_text, "CONN")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 6, m_weight, 18, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 7, m_text, "bono")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 7, m_weight, 17, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 8, m_text, "mono")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 8, m_weight, 17, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 9, m_text, "LONON")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 9, m_weight, 15, QString::number)
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 10, m_text, "LONGE")
    CHECK_SUB_ITEM_PROPERTY(TextItem, textItems1, 10, m_weight, 15, QString::number)

    CHECK_SUB_ITEM_PROPERTY(ObjectItem, objectItems1, 0, m_objectType, "face")
    CHECK_SUB_ITEM_PROPERTY(ObjectItem, objectItems1, 0, m_weight, 31, QString::number)
    CHECK_SUB_ITEM_PROPERTY(ObjectItem, objectItems1, 1, m_objectType, "lake")
    CHECK_SUB_ITEM_PROPERTY(ObjectItem, objectItems1, 1, m_weight, 30, QString::number)
    CHECK_SUB_ITEM_PROPERTY(ObjectItem, objectItems1, 2, m_objectType, "snow")
    CHECK_SUB_ITEM_PROPERTY(ObjectItem, objectItems1, 2, m_weight, 29, QString::number)
    CHECK_SUB_ITEM_PROPERTY(ObjectItem, objectItems1, 3, m_objectType, "road")
    CHECK_SUB_ITEM_PROPERTY(ObjectItem, objectItems1, 3, m_weight, 32, QString::number)

    CHECK_SUB_ITEM_PROPERTY(ShapeItem, shapeItems1, 0, m_shapeType, "circle")
    CHECK_SUB_ITEM_PROPERTY(ShapeItem, shapeItems1, 0, m_weight, 31, QString::number)
    CHECK_SUB_ITEM_PROPERTY(ShapeItem, shapeItems1, 1, m_shapeType, "oval")
    CHECK_SUB_ITEM_PROPERTY(ShapeItem, shapeItems1, 1, m_weight, 29, QString::number)
    CHECK_SUB_ITEM_PROPERTY(ShapeItem, shapeItems1, 2, m_shapeType, "rectangle")
    CHECK_SUB_ITEM_PROPERTY(ShapeItem, shapeItems1, 2, m_weight, 30, QString::number)
    CHECK_SUB_ITEM_PROPERTY(ShapeItem, shapeItems1, 3, m_shapeType, "triangle")
    CHECK_SUB_ITEM_PROPERTY(ShapeItem, shapeItems1, 3, m_weight, 32, QString::number)

    CHECK_SUB_ITEM_PROPERTY(BarcodeItem, barcodeItems1, 0, m_barcode, "5000600001")
    CHECK_SUB_ITEM_PROPERTY(BarcodeItem, barcodeItems1, 0, m_weight, 32, QString::number)
    CHECK_SUB_ITEM_PROPERTY(BarcodeItem, barcodeItems1, 1, m_barcode, "3000600001")
    CHECK_SUB_ITEM_PROPERTY(BarcodeItem, barcodeItems1, 1, m_weight, 25, QString::number)
    CHECK_SUB_ITEM_PROPERTY(BarcodeItem, barcodeItems1, 2, m_barcode, "2000600001")
    CHECK_SUB_ITEM_PROPERTY(BarcodeItem, barcodeItems1, 2, m_weight, 31, QString::number)

    return true;
}

} // namespace test
} // namespace quentier
