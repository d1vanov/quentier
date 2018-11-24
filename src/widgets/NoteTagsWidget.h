/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_WIDGETS_NOTE_TAGS_WIDGET_H
#define QUENTIER_WIDGETS_NOTE_TAGS_WIDGET_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/utility/StringUtils.h>
#include <QWidget>
#include <QPointer>
#include <QHash>
#include <QSet>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/bimap.hpp>
#endif

QT_FORWARD_DECLARE_CLASS(FlowLayout)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(NewListItemLineEdit)

/**
 * @brief The NoteTagsWidget class demonstrates the tags of a particular note
 *
 * It encapsulates the flow layout consisting of smaller widgets representing each tag
 * of the given note + a it listens to the updates of notes from the local storage
 * in order to track any updates of the given note
 */
class NoteTagsWidget: public QWidget
{
    Q_OBJECT
public:
    explicit NoteTagsWidget(QWidget * parent = Q_NULLPTR);

    void setLocalStorageManagerThreadWorker(LocalStorageManagerAsync & localStorageManagerAsync);

    void setTagModel(TagModel * pTagModel);

    const Note & currentNote() const { return m_currentNote; }
    void setCurrentNoteAndNotebook(const Note & note, const Notebook & notebook);

    const QString & currentNotebookLocalUid() const { return m_currentNotebookLocalUid; }

    const QString & currentLinkedNotebookGuid() const { return m_currentLinkedNotebookGuid; }

    /**
     * @brief clear method clears out the tags displayed by the widget and
     * removes the association with the particular note
     */
    void clear();

    bool isActive() const;

    /**
     * @brief tagNames - returns the list of current note's tag names
     */
    QStringList tagNames() const;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);
    void canUpdateNoteRestrictionChanged(bool canUpdateNote);

    void newTagLineEditReceivedFocusFromWindowSystem();

// private signals
    void updateNote(Note note, LocalStorageManager::UpdateNoteOptions options, QUuid requestId);

private Q_SLOTS:
    void onTagRemoved(QString tagName);
    void onNewTagNameEntered();
    void onAllTagsListed();

    // Slots for response to events from local storage

    // Slots for notes events: finding, updating & expunging
    void onUpdateNoteComplete(Note note, LocalStorageManager::UpdateNoteOptions options, QUuid requestId);
    void onUpdateNoteFailed(Note note, LocalStorageManager::UpdateNoteOptions options,
                            ErrorString errorDescription, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);

    // Slots for notebook events: updating and expunging
    // The notebooks are important  because they hold the information about the note restrictions
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    // Slots for tag events: updating and expunging
    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId);

private:
    void clearLayout(const bool skipNewTagWidget = false);
    void updateLayout();

    void addTagIconToLayout();
    void addNewTagWidgetToLayout();
    void removeNewTagWidgetFromLayout();
    void removeTagWidgetFromLayout(const QString & tagLocalUid);

    void setTagItemsRemovable(const bool removable);

    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);

    NewListItemLineEdit * findNewItemWidget();

private:
    Note                    m_currentNote;
    QString                 m_currentNotebookLocalUid;
    QString                 m_currentLinkedNotebookGuid;

    QStringList             m_lastDisplayedTagLocalUids;

    typedef  boost::bimap<QString, QString>  TagLocalUidToNameBimap;
    TagLocalUidToNameBimap  m_currentNoteTagLocalUidToNameBimap;

    QPointer<TagModel>      m_pTagModel;

    typedef QHash<QUuid, std::pair<QString, QString> > RequestIdToTagLocalUidAndGuid;
    RequestIdToTagLocalUidAndGuid   m_updateNoteRequestIdToRemovedTagLocalUidAndGuid;
    RequestIdToTagLocalUidAndGuid   m_updateNoteRequestIdToAddedTagLocalUidAndGuid;

    struct Restrictions
    {
        Restrictions() :
            m_canUpdateNote(false),
            m_canUpdateTags(false)
        {}

        bool    m_canUpdateNote;
        bool    m_canUpdateTags;
    };

    Restrictions            m_tagRestrictions;

    StringUtils             m_stringUtils;

    FlowLayout *            m_pLayout;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTE_TAGS_WIDGET_H
