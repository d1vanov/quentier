#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_ITEM_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_ITEM_H

#include "NotebookItem.h"
#include "NotebookStackItem.h"

namespace qute_note {

class NotebookModelItem: public Printable
{
public:
    struct Type
    {
        enum type {
            Stack = 0,
            Notebook
        };
    };

    NotebookModelItem(const Type::type type = Type::Notebook,
                      const NotebookItem * notebookItem = Q_NULLPTR,
                      const NotebookStackItem * notebookStackItem = Q_NULLPTR);
    ~NotebookModelItem();

    Type::type type() const { return m_type; }
    void setType(const Type::type type) { m_type = type; }

    const NotebookItem * notebookItem() const { return m_notebookItem; }
    void setNotebookItem(const NotebookItem * notebookItem) { m_notebookItem = notebookItem; }

    const NotebookStackItem * notebookStackItem() const { return m_notebookStackItem; }
    void setNotebookStackItem(const NotebookStackItem * notebookStackItem) { m_notebookStackItem = notebookStackItem; }

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    Type::type                  m_type;
    const NotebookItem *        m_notebookItem;
    const NotebookStackItem *   m_notebookStackItem;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_ITEM_H
