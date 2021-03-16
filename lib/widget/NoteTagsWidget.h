/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_NOTE_TAGS_WIDGET_H
#define QUENTIER_LIB_WIDGET_NOTE_TAGS_WIDGET_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/utility/StringUtils.h>

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QWidget>

#include <boost/bimap.hpp>

class FlowLayout;

namespace quentier {

class TagModel;
class NewListItemLineEdit;

/**
 * @brief The NoteTagsWidget class demonstrates the tags of a particular note
 *
 * It encapsulates the flow layout consisting of smaller widgets representing
 * each tag of the given note + a it listens to the updates of notes from
 * the local storage in order to track any updates of the given note
 */
class NoteTagsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoteTagsWidget(QWidget * parent = nullptr);

    ~NoteTagsWidget() override;

    void setLocalStorageManagerThreadWorker(
        LocalStorageManagerAsync & localStorageManagerAsync);

    void setTagModel(TagModel * pTagModel);

    [[nodiscard]] const qevercloud::Note & currentNote() const noexcept
    {
        return m_currentNote;
    }

    void setCurrentNoteAndNotebook(
        const qevercloud::Note & note, const qevercloud::Notebook & notebook);

    [[nodiscard]] const QString & currentNotebookLocalId() const noexcept
    {
        return m_currentNotebookLocalId;
    }

    [[nodiscard]] const QString & currentLinkedNotebookGuid() const noexcept
    {
        return m_currentLinkedNotebookGuid;
    }

    /**
     * @brief clear method clears out the tags displayed by the widget and
     * removes the association with the particular note
     */
    void clear();

    [[nodiscard]] bool isActive() const noexcept;

    /**
     * @brief tagNames - returns the list of current note's tag names
     */
    [[nodiscard]] QStringList tagNames() const;

Q_SIGNALS:
    void notifyError(ErrorString errorDescription);
    void canUpdateNoteRestrictionChanged(bool canUpdateNote);

    void noteTagsListChanged(qevercloud::Note note);

    void newTagLineEditReceivedFocusFromWindowSystem();

private Q_SLOTS:
    void onTagRemoved(QString tagName);
    void onNewTagNameEntered();
    void onAllTagsListed();

    // Slots for response to events from local storage

    // Slots for notes events: updating & expunging
    void onUpdateNoteComplete(
        qevercloud::Note note, LocalStorageManager::UpdateNoteOptions options,
        QUuid requestId);

    void onExpungeNoteComplete(qevercloud::Note note, QUuid requestId);

    // Slots for notebook events: updating and expunging
    // The notebooks are important  because they hold the information about
    // the note restrictions
    void onUpdateNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    void onExpungeNotebookComplete(
        qevercloud::Notebook notebook, QUuid requestId);

    // Slots for tag events: updating and expunging
    void onUpdateTagComplete(qevercloud::Tag tag, QUuid requestId);

    void onExpungeTagComplete(
        qevercloud::Tag tag, QStringList expungedChildTagLocalIds,
        QUuid requestId);

private:
    void clearLayout(bool skipNewTagWidget = false);
    void updateLayout();

    void addTagIconToLayout();
    void addNewTagWidgetToLayout();
    void removeNewTagWidgetFromLayout();
    void removeTagWidgetFromLayout(const QString & tagLocalId);

    void setTagItemsRemovable(bool removable);

    void createConnections(LocalStorageManagerAsync & localStorageManagerAsync);

    [[nodiscard]] NewListItemLineEdit * findNewItemWidget();

private:
    qevercloud::Note m_currentNote;
    QString m_currentNotebookLocalId;
    QString m_currentLinkedNotebookGuid;

    QStringList m_lastDisplayedTagLocalIds;

    using TagLocalIdToNameBimap = boost::bimap<QString, QString>;
    TagLocalIdToNameBimap m_currentNoteTagLocalIdToNameBimap;

    QPointer<TagModel> m_pTagModel;

    struct Restrictions
    {
        bool m_canUpdateNote = false;
        bool m_canUpdateTags = false;
    };

    Restrictions m_tagRestrictions;

    StringUtils m_stringUtils;

    FlowLayout * m_pLayout;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NOTE_TAGS_WIDGET_H
