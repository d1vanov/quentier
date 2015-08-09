#include <qute_note/note_editor/NoteEditor.h>
#include "NoteEditor_p.h"

#ifndef USE_QT_WEB_ENGINE
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#endif

#include <qute_note/types/Note.h>
#include <qute_note/types/Notebook.h>
#include <QFont>
#include <QColor>

namespace qute_note {

NoteEditor::NoteEditor(QWidget * parent) :
#ifndef USE_QT_WEB_ENGINE
    QWebView(parent),
#else
    QWebEngineView(parent),
#endif
    d_ptr(new NoteEditorPrivate(*this))
{}

NoteEditor::~NoteEditor()
{}

void NoteEditor::setNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    Q_D(NoteEditor);
    d->setNoteAndNotebook(note, notebook);
}

const Note * NoteEditor::getNote()
{
    Q_D(NoteEditor);
    return d->getNote();
}

const Notebook * NoteEditor::getNotebook() const
{
    Q_D(const NoteEditor);
    return d->getNotebook();
}

bool NoteEditor::isModified() const
{
    Q_D(const NoteEditor);
    return d->isModified();
}

#ifndef USE_QT_WEB_ENGINE
const NoteEditorPluginFactory & NoteEditor::pluginFactory() const
{
    Q_D(const NoteEditor);
    return d->pluginFactory();
}

NoteEditorPluginFactory & NoteEditor::pluginFactory()
{
    Q_D(NoteEditor);
    return d->pluginFactory();
}
#endif

void NoteEditor::undo()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::Undo);
#else
    page()->triggerAction(QWebEnginePage::Undo);
#endif
}

void NoteEditor::redo()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::Redo);
#else
    page()->triggerAction(QWebEnginePage::Redo);
#endif
}

void NoteEditor::cut()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::Cut);
#else
    page()->triggerAction(QWebEnginePage::Cut);
#endif
}

void NoteEditor::copy()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::Copy);
#else
    page()->triggerAction(QWebEnginePage::Copy);
#endif
}

void NoteEditor::paste()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::Paste);
#else
    page()->triggerAction(QWebEnginePage::Paste);
#endif
}

void NoteEditor::textBold()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::ToggleBold);
#else
    // TODO: err... do somethind?
#endif
}

void NoteEditor::textItalic()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::ToggleItalic);
#else
    // TODO: err... do somethind?
#endif
}

void NoteEditor::textUnderline()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::ToggleUnderline);
#else
    // TODO: err... do somethind?
#endif
}

void NoteEditor::textStrikethrough()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::ToggleStrikethrough);
#else
    // TODO: err... do somethind?
#endif
}

void NoteEditor::alignLeft()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::AlignLeft);
#else
    // TODO: err... do somethind?
#endif
}

void NoteEditor::alignCenter()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::AlignCenter);
#else
    // TODO: err... do somethind?
#endif
}

void NoteEditor::alignRight()
{
#ifndef USE_QT_WEB_ENGINE
    page()->triggerAction(QWebPage::AlignRight);
#else
    // TODO: err... do somethind?
#endif
}

void NoteEditor::insertToDoCheckbox()
{
    Q_D(NoteEditor);
    d->insertToDoCheckbox();
}

void NoteEditor::setSpellcheck(const bool enabled)
{
    // TODO: implement
    Q_UNUSED(enabled)
}

void NoteEditor::setFont(const QFont & font)
{
    Q_D(NoteEditor);
    d->setFont(font);
}

void NoteEditor::setFontHeight(const int height)
{
    Q_D(NoteEditor);
    d->setFontHeight(height);
}

void NoteEditor::setFontColor(const QColor & color)
{
    Q_D(NoteEditor);
    d->setFontColor(color);
}

void NoteEditor::setBackgroundColor(const QColor & color)
{
    Q_D(NoteEditor);
    d->setBackgroundColor(color);
}

void NoteEditor::insertHorizontalLine()
{
    Q_D(NoteEditor);
    d->insertHorizontalLine();
}

void NoteEditor::changeIndentation(const bool increase)
{
    Q_D(NoteEditor);
    d->changeIndentation(increase);
}

void NoteEditor::insertBulletedList()
{
    Q_D(NoteEditor);
    d->insertBulletedList();
}

void NoteEditor::insertNumberedList()
{
    Q_D(NoteEditor);
    d->insertNumberedList();
}

void NoteEditor::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    Q_D(NoteEditor);
    d->insertFixedWidthTable(rows, columns, widthInPixels);
}

void NoteEditor::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    Q_D(NoteEditor);
    d->insertRelativeWidthTable(rows, columns, relativeWidth);
}

void NoteEditor::encryptSelectedText(const QString & passphrase, const QString & hint)
{
    Q_D(NoteEditor);
    d->encryptSelectedText(passphrase, hint);
}

void NoteEditor::onEncryptedAreaDecryption(QString encryptedText, QString decryptedText, bool rememberForSession)
{
    Q_D(NoteEditor);
    d->onEncryptedAreaDecryption(encryptedText, decryptedText, rememberForSession);
}

void NoteEditor::onNoteLoadCancelled()
{
    Q_D(NoteEditor);
    d->onNoteLoadCancelled();
}

void NoteEditor::dropEvent(QDropEvent * pEvent)
{
    Q_D(NoteEditor);
    d->onDropEvent(pEvent);
}

} // namespace qute_note
