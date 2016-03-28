#include "NotebookModelItem.h"

namespace qute_note {

NotebookModelItem::NotebookModelItem(const Type::type type,
                                     const NotebookItem * notebookItem,
                                     const NotebookStackItem * notebookStackItem) :
    m_type(type),
    m_notebookItem(notebookItem),
    m_notebookStackItem(notebookStackItem)
{}

QTextStream & NotebookModelItem::Print(QTextStream & strm) const
{
    strm << "Notebook model item (" << (m_type == Type::Notebook ? "notebook" : "stack")
         << "): " << (m_type == Type::Notebook
                      ? (m_notebookItem ? m_notebookItem->ToQString() : QString("<null>"))
                      : (m_notebookStackItem ? m_notebookStackItem->ToQString() : QString("<null>")));
    return strm;
}

} // namespace qute_note
