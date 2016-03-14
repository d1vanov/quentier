#ifndef __QUTE_NOTE__MODELS__TAG_MODEL_ITEM_H
#define __QUTE_NOTE__MODELS__TAG_MODEL_ITEM_H

#include <qute_note/utility/Printable.h>

namespace qute_note {

class TagModelItem: public Printable
{
public:
    explicit TagModelItem(const QString & localUid = QString(),
                          const QString & name = QString(),
                          const QString & parentLocalUid = QString(),
                          const bool isSynchronizable = false,
                          const bool isDirty = false);

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

    QString     m_localUid;
    QString     m_name;
    QString     m_parentLocalUid;
    bool        m_isSynchronizable;
    bool        m_isDirty;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__TAG_MODEL_ITEM_H
