#include "ENMLConverter.h"
#include "../note_editor/QuteNoteTextEdit.h"
#include "../Note.h"
#include "../Resource.h"
#include "../tools/QuteNoteCheckPtr.h"
#include <QTextEdit>
#include <QString>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QDomDocument>
#include <QDebug>

namespace enml_private {

void EncodeFragment(const QTextFragment & fragment, QString & encodedFragment);

}

namespace qute_note {

// TODO: adopt some simple xml node-constructing instead of plain text insertions
void RichTextToENML(const Note & note, const QuteNoteTextEdit & noteEditor, QString & ENML)
{
    ENML.clear();

    const QTextDocument * pNoteDoc = noteEditor.document();
    QUTE_NOTE_CHECK_PTR(pNoteDoc, QObject::tr("Null QTextDocument pointer received from QuteNoteTextEdit"));

    const Note * pNote = noteEditor.getNotePtr();
    QUTE_NOTE_CHECK_PTR(pNote, QObject::tr("Null pointer to Note received from QuteNoteTextEdit"));

    size_t resourceIndex = 0;

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
            else if (currentFragmentCharFormatObjectType == QuteNoteTextEdit::MEDIA_RESOURCE_TXT_FORMAT)
            {
                const Resource * pCurrentResource = note.getResourceByIndex(resourceIndex);
                if (pCurrentResource == nullptr)
                {
                    qWarning() << "Internal error! Found char format corresponding to resource "
                               << "but haven't found the corresponding resource object for index "
                               << resourceIndex;
                }
                else
                {
                    QString hash = pCurrentResource->dataHash();
                    size_t width = pCurrentResource->width();
                    size_t height = pCurrentResource->height();
                    QString type = pCurrentResource->mimeType();
                    ENML.append("<en-media width=\"");
                    ENML.append(QString::number(width));
                    ENML.append("\" height=\"");
                    ENML.append(QString::number(height));
                    ENML.append("\" type=\"");
                    ENML.append(type);
                    ENML.append("\" hash=\"");
                    ENML.append(hash);
                    ENML.append("/>");
                }

                ++resourceIndex;
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

bool ENMLToRichText(const Note & /* note */, const QString & ENML,
                    QuteNoteTextEdit & noteEditor, QString & errorMessage)
{
    std::unique_ptr<QuteNoteTextEdit> pFakeNoteEditor(new QuteNoteTextEdit());
    QTextDocument * pNoteEditorDoc = noteEditor.document();
    pFakeNoteEditor.get()->setDocument(pNoteEditorDoc);
    // QTextCursor cursor(pNoteEditorDoc);

    // Now we have fake note editor object which we can use to iterate through
    // document using QTextCursor

    QDomDocument enXmlDomDoc;
    int errorLine = -1, errorColumn = -1;
    bool res = enXmlDomDoc.setContent(ENML, &errorMessage, &errorLine,
                                      &errorColumn);
    if (!res) {
        errorMessage.append(QString(". Error happened at line ") +
                            QString::number(errorLine) + QString(", at column ") +
                            QString::number(errorColumn));
        return false;
    }

    QDomElement docElem = enXmlDomDoc.documentElement();
    QString rootTag = docElem.tagName();
    if (rootTag != QString("en-note")) {
        errorMessage = "Wrong root tag, should be \"en-note\", instead: ";
        errorMessage.append(rootTag);
        return false;
    }

    QDomNode nextNode = docElem.firstChild();
    while(!nextNode.isNull())
    {
        QDomElement element = nextNode.toElement();
        if (!element.isNull())
        {
            // TODO: here the piece of HTML content should be formed to be inserted
            // into the document via QTextCursor
        }
        else
        {
            errorMessage = "Found QDomNode not convertable to QDomElement";
            return false;
        }
    }

    return true;
}

}

namespace enml_private {

void EncodeFragment(const QTextFragment &, QString &)
{
    // TODO: implement
}

}
