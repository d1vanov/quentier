#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_STACK_ITEM_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_STACK_ITEM_H

#include <qute_note/utility/Printable.h>

namespace qute_note {

class NotebookStackItem: public Printable
{
public:
    NotebookStackItem(const QString & name = QString()) :
        m_name(name)
    {}

    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString     m_name;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_STACK_ITEM_H
