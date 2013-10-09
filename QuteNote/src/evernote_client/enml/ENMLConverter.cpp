#include "ENMLConverter.h"
#include "../note_editor/QuteNoteTextEdit.h"
#include <QTextEdit>
#include <QString>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QDebug>

namespace enml_private {

void EncodeFragment(const QTextFragment & fragment, QString & encodedFragment);

}

namespace qute_note {

void RichTextToENML(const QuteNoteTextEdit & noteEditor, QString & ENML)
{
    ENML.clear();

    const QTextDocument * pNoteDoc = noteEditor.document();
    Q_CHECK_PTR(pNoteDoc);

    QTextBlock noteDocEnd = pNoteDoc->end();
    for(QTextBlock block = pNoteDoc->begin(); block != noteDocEnd; block = block.next())
    {
        for(QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it)
        {
            QTextFragment currentFragment = it.fragment();
            if (currentFragment.isValid()) {
                QString encodedCurrentFragment;
                enml_private::EncodeFragment(currentFragment, encodedCurrentFragment);
            }
            else {
                qWarning() << "Found invalid QTextFragment during encoding note content to ENML! Ignoring it...";
            }
        }
    }
}

}

namespace enml_private {

void EncodeFragment(const QTextFragment &, QString &)
{
    // TODO: implement
}

}
