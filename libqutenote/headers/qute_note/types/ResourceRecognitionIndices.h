#ifndef __LIB_QUTE_NOTE__TYPES__RESOURCE_RECOGNITION_INDICES_H
#define __LIB_QUTE_NOTE__TYPES__RESOURCE_RECOGNITION_INDICES_H

#include <qute_note/types/ResourceRecognitionIndexItem.h>
#include <QByteArray>
#include <QSharedDataPointer>
#include <QVector>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ResourceRecognitionIndicesData)

class QUTE_NOTE_EXPORT ResourceRecognitionIndices: public Printable
{
public:
    ResourceRecognitionIndices();
    ResourceRecognitionIndices(const QByteArray & rawRecognitionIndicesData);

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

    void setData(const QByteArray & rawRecognitionIndicesData);

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<ResourceRecognitionIndicesData> d;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__RESOURCE_RECOGNITION_INDICES_H
