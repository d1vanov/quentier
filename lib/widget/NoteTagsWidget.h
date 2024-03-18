/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/StringUtils.h>
#include <quentier/utility/SuppressWarnings.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QWidget>

SAVE_WARNINGS

MSVC_SUPPRESS_WARNING(4834)

#include <boost/bimap.hpp>

RESTORE_WARNINGS

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

    void setLocalStorage(const local_storage::ILocalStorage & localStorage);
    void setTagModel(TagModel * tagModel);

    [[nodiscard]] const std::optional<qevercloud::Note> & currentNote()
        const noexcept
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
    void onTagRemoved(
        QString tagLocalId, QString tagName, QString linkedNotebookGuid,
        QString linkedNotebookUsername);

    void onNewTagNameEntered();
    void onAllTagsListed();

private:
    void clearLayout(const bool skipNewTagWidget = false);
    void updateLayout();

    void addTagIconToLayout();
    void addNewTagWidgetToLayout();
    void removeNewTagWidgetFromLayout();
    void removeTagWidgetFromLayout(const QString & tagLocalId);

    void setTagItemsRemovable(bool removable);

    void connectToLocalStorageEvents(
        local_storage::ILocalStorageNotifier * notifier);

    [[nodiscard]] NewListItemLineEdit * findNewItemWidget();

private:
    std::optional<qevercloud::Note> m_currentNote;
    QString m_currentNotebookLocalId;
    QString m_currentLinkedNotebookGuid;

    QStringList m_lastDisplayedTagLocalIds;

    using TagLocalIdToNameBimap = boost::bimap<QString, QString>;
    TagLocalIdToNameBimap m_currentNoteTagLocalIdToNameBimap;

    QPointer<TagModel> m_tagModel;

    struct Restrictions
    {
        bool m_canUpdateNote = false;
        bool m_canUpdateTags = false;
    };

    Restrictions m_tagRestrictions;

    StringUtils m_stringUtils;

    FlowLayout * m_layout;
};

} // namespace quentier
