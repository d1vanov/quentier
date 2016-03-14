#ifndef __QUTE_NOTE__MODELS__SAVED_SEARCH_MODEL_ITEM_H
#define __QUTE_NOTE__MODELS__SAVED_SEARCH_MODEL_ITEM_H

#include <qute_note/utility/Printable.h>

namespace qute_note {

class SavedSearchModelItem: public Printable
{
public:
    explicit SavedSearchModelItem(const QString & localUid = QString(),
                                  const QString & name = QString(),
                                  const QString & query = QString(),
                                  const bool isSynchronizable = false,
                                  const bool isDirty = false);

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

    QString     m_localUid;
    QString     m_name;
    QString     m_query;
    bool        m_isSynchronizable;
    bool        m_isDirty;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__SAVED_SEARCH_MODEL_ITEM_H
