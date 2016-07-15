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

#ifndef LIB_QUENTIER_TYPES_RESOURCE_RECOGNITION_INDICES_H
#define LIB_QUENTIER_TYPES_RESOURCE_RECOGNITION_INDICES_H

#include <quentier/types/ResourceRecognitionIndexItem.h>
#include <QByteArray>
#include <QSharedDataPointer>
#include <QVector>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ResourceRecognitionIndicesData)

class QUENTIER_EXPORT ResourceRecognitionIndices: public Printable
{
public:
    ResourceRecognitionIndices();
    ResourceRecognitionIndices(const ResourceRecognitionIndices & other);
    ResourceRecognitionIndices(const QByteArray & rawRecognitionIndicesData);
    ResourceRecognitionIndices & operator=(const ResourceRecognitionIndices & other);
    virtual ~ResourceRecognitionIndices();

    bool isNull() const;
    bool isValid() const;

    QString objectId() const;
    QString objectType() const;
    QString recoType() const;
    QString engineVersion() const;
    QString docType() const;
    QString lang() const;

    int objectHeight() const;
    int objectWidth() const;

    QVector<ResourceRecognitionIndexItem> items() const;

    bool setData(const QByteArray & rawRecognitionIndicesData);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<ResourceRecognitionIndicesData> d;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_RESOURCE_RECOGNITION_INDICES_H
