#include "ENMLConverter.h"
#include "../note_editor/QuteNoteTextEdit.h"
#include "../Note.h"
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

    const Note * pNote = noteEditor.getNotePtr();
    Q_CHECK_PTR(pNote);

    ENML.append("<en-note>");

    QTextBlock noteDocEnd = pNoteDoc->end();
    for(QTextBlock block = pNoteDoc->begin(); block != noteDocEnd; block = block.next())
    {
        QString blockText = block.text();
        if (blockText.isEmpty()) {
            ENML.append("<div><br /></div>");
            continue;
        }

        for(QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it)
        {
            ENML.append("<div>");
            QTextFragment currentFragment = it.fragment();
            QTextCharFormat currentFragmentCharFormat = currentFragment.charFormat();
            int currentFragmentCharFormatObjectType = currentFragmentCharFormat.objectType();
            if (currentFragmentCharFormatObjectType == QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_CHECKED) {
                ENML.append("<en-todo checked=\"true\"/>");
            }
            else if (currentFragmentCharFormatObjectType == QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_UNCHECKED) {
                ENML.append("<en-todo checked=\"false\"/>");
            }
            else if (currentFragmentCharFormatObjectType == QuteNoteTextEdit::MEDIA_RESOURCE_TXT_FORMAT) {
                // TODO: process en-media tag properly: need to have resources metadata somewhere
            }
            else if (currentFragment.isValid()) {
                QString encodedCurrentFragment;
                enml_private::EncodeFragment(currentFragment, encodedCurrentFragment);
                ENML.append(encodedCurrentFragment);
            }
            else {
                qWarning() << "Found invalid QTextFragment during encoding note content to ENML! Ignoring it...";
            }
            ENML.append(("</div>"));
        }
    }

    ENML.append("</en_note>");
}

}

namespace enml_private {

void EncodeFragment(const QTextFragment &, QString &)
{
    // TODO: implement
}

}
