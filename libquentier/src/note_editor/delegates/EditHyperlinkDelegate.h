/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/Note.h>
#include <QObject>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(NoteEditorPage)

class EditHyperlinkDelegate: public QObject
{
    Q_OBJECT
public:
    explicit EditHyperlinkDelegate(NoteEditorPrivate & noteEditor, const quint64 hyperlinkId);

    void start();

Q_SIGNALS:
    void finished();
    void cancelled();
    void notifyError(ErrorString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onHyperlinkDataReceived(const QVariant & data);
    void onHyperlinkDataEdited(QString text, QUrl url, quint64 hyperlinkId, bool startupUrlWasEmpty);
    void onHyperlinkModified(const QVariant & data);

private:
    void doStart();
    void raiseEditHyperlinkDialog(const QString & startupHyperlinkText, const QString & startupHyperlinkUrl);

private:
    typedef JsResultCallbackFunctor<EditHyperlinkDelegate> JsCallback;

private:
    NoteEditorPrivate &     m_noteEditor;
    const quint64           m_hyperlinkId;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H
