#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H

#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(INoteStore)
QT_FORWARD_DECLARE_CLASS(NotebookPrivate)

class Notebook: public TypeWithError,
                public SynchronizedDataElement
{
public:
    static Notebook Create(const QString & name, INoteStore & noteStore);

    Notebook(const Notebook & other);
    Notebook & operator=(const Notebook & other);
    virtual ~Notebook();

    virtual bool isEmpty() const;

    /**
     * @return name of the notebook
     */
    const QString name() const;
    void setName(const QString & name);

    time_t createdTimestamp() const;
    time_t updatedTimestamp() const;

    bool isDefault() const;
    bool isLastUsed() const;

    virtual QTextStream & Print(QTextStream & strm) const;

private:
    Notebook() = delete;
    Notebook(const QString & name);

    const QScopedPointer<NotebookPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Notebook)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H
