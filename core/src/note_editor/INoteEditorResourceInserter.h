#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__I_NOTE_EDITOR_RESOURCE_INSERTER_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__I_NOTE_EDITOR_RESOURCE_INSERTER_H

#include "NoteEditor.h"
#include <QMimeType>

namespace qute_note {

class INoteEditorResourceInserter: public QObject
{
    Q_OBJECT
protected:
    explicit INoteEditorResourceInserter(NoteEditor & noteEditorView) :
        QObject(&noteEditorView)
    {}

public:
    virtual void insertResource(const QByteArray & resource, const QMimeType & mimeType) = 0;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__I_NOTE_EDITOR_RESOURCE_INSERTER_H
