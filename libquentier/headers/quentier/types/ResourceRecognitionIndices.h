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
