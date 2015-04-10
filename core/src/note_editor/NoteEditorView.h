#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_VIEW_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_VIEW_H

#include <tools/qt4helper.h>
#include <QWebView>
#include <QMimeType>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QByteArray)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorController)

class INoteEditorResourceInserter: public QObject
{
    Q_OBJECT
public:
    virtual void insertResource(const QByteArray & resource, const QMimeType & mimeType, QWebView & noteEditor) = 0;

protected:
    explicit INoteEditorResourceInserter(QObject * parent = nullptr) :
        QObject(parent)
    {}
};

class NoteEditorView: public QWebView
{
    Q_OBJECT
public:
    explicit NoteEditorView(QWidget * parent = nullptr);
    void setController(NoteEditorController * pController);

    void addResourceInserterForMimeType(const QString & mimeTypeName,
                                        INoteEditorResourceInserter * pResourceInserter);

    // returns true if resource inserter was found by mime type and removed
    bool removeResourceInserterForMimeType(const QString & mimeTypeName);

    bool hasResourceInsertedForMimeType(const QString & mimeTypeName) const;

private:
    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;
    void dropFile(QString & filepath);

private:
    NoteEditorController *                          m_pNoteEditorController;
    QHash<QString, INoteEditorResourceInserter*>    m_noteEditorResourceInserters;
};

} // namespace qute_note

#endif //__QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_VIEW_H
