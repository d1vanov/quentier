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

#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_REMOVE_RESOURCE_DELEGATE_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_REMOVE_RESOURCE_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/types/Note.h>
#include <quentier/types/ResourceWrapper.h>
#include <QObject>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

class RemoveResourceDelegate: public QObject
{
    Q_OBJECT
public:
    explicit RemoveResourceDelegate(const ResourceWrapper & resourceToRemove, NoteEditorPrivate & noteEditor);

    void start();

Q_SIGNALS:
    void finished(ResourceWrapper removedResource);
    void notifyError(QNLocalizedString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);

    void onResourceReferenceRemovedFromNoteContent(const QVariant & data);

private:
    void doStart();

private:
    typedef JsResultCallbackFunctor<RemoveResourceDelegate> JsCallback;

private:
    NoteEditorPrivate &             m_noteEditor;
    ResourceWrapper                 m_resource;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_REMOVE_RESOURCE_DELEGATE_H
