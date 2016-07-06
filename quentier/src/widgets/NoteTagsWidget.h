#ifndef QUENTIER_WIDGETS_NOTE_TAGS_WIDGET_H
#define QUENTIER_WIDGETS_NOTE_TAGS_WIDGET_H

#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
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

    const Note & currentNote() const { return m_currentNote; }
    void setCurrentNote(const Note & note);

    const QString & currentNotebookLocalUid() const { return m_currentNotebookLocalUid; }

    /**
     * @brief clear method clears out the tags displayed by the widget and
     * removes the association with the particular note
     */
    void clear();

    bool isActive() const;

Q_SIGNALS:
    void notifyError(QString errorDescription);
    void canUpdateNoteRestrictionChanged(bool canUpdateNote);

// private signals
    void findNotebook(Notebook notebook, QUuid requestId);
    void updateNote(Note note, bool updateResources, bool updateTags, QUuid requestId);

private Q_SLOTS:
    void onTagRemoved(QString tagName);

private Q_SLOTS:
    // Slots for response to events from local storage

    // Slots for notes events: finding, updating & expunging
    void onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            QString errorDescription, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);

    // Slots for notebook events: finding, updating and expunging
    // The notebooks are important  because they hold the information about the note restrictions
    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);
    void onUpdateNotebookComplete(Notebook notebook, QUuid requestId);
    void onExpungeNotebookComplete(Notebook notebook, QUuid requestId);

    // Slots for tag events: updating and expunging
    void onUpdateTagComplete(Tag tag, QUuid requestId);
    void onExpungeTagComplete(Tag tag, QUuid requestId);

private:
    void clearLayout(const bool skipNewTagWidget = false);
    void updateLayout();

    void addTagIconToLayout();
    void addNewTagWidgetToLayout();
    void removeNewTagWidgetFromLayout();

private:
    Note                    m_currentNote;
    QString                 m_currentNotebookLocalUid;

    QStringList             m_lastDisplayedTagLocalUids;

    typedef  boost::bimap<QString, QString>  TagLocalUidToNameBimap;
    TagLocalUidToNameBimap  m_currentNoteTagLocalUidToNameBimap;

    QPointer<TagModel>      m_pTagModel;

    QHash<QUuid, std::pair<QString, QString> >  m_updateNoteRequestIdToRemovedTagLocalUidAndGuid;
    QUuid                   m_findNotebookRequestId;


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

    FlowLayout *            m_pLayout;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NOTE_TAGS_WIDGET_H
