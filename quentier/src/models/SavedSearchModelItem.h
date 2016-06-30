#ifndef QUENTIER_MODELS_SAVED_SEARCH_MODEL_ITEM_H
#define QUENTIER_MODELS_SAVED_SEARCH_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

namespace quentier {

class SavedSearchModelItem: public Printable
{
public:
    explicit SavedSearchModelItem(const QString & localUid = QString(),
                                  const QString & name = QString(),
                                  const QString & query = QString(),
                                  const bool isSynchronizable = false,
                                  const bool isDirty = false);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    QString nameUpper() const { return m_name.toUpper(); }

    QString     m_localUid;
    QString     m_name;
    QString     m_query;
    bool        m_isSynchronizable;
    bool        m_isDirty;
};

} // namespace quentier

#endif // QUENTIER_MODELS_SAVED_SEARCH_MODEL_ITEM_H
