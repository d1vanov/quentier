/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#include "EditNoteDialog.h"
#include "ui_EditNoteDialog.h"

#include <lib/model/NotebookModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QStringListModel>
#include <QCompleter>
#include <QModelIndex>
#include <QToolTip>
#include <QDateTime>

#include <algorithm>
#include <iterator>

namespace quentier {

EditNoteDialog::EditNoteDialog(
        const Note & note,
        NotebookModel * pNotebookModel,
        QWidget * parent, const bool readOnlyMode) :
    QDialog(parent),
    m_pUi(new Ui::EditNoteDialog),
    m_note(note),
    m_pNotebookModel(pNotebookModel),
    m_pNotebookNamesModel(new QStringListModel(this)),
    m_stringUtils(),
    m_creationDateTimeEdited(false),
    m_modificationDateTimeEdited(false),
    m_deletionDateTimeEdited(false),
    m_subjectDateTimeEdited(false),
    m_latitudeEdited(false),
    m_longitudeEdited(false),
    m_altitudeEdited(false),
    m_readOnlyMode(readOnlyMode)
{
    m_pUi->setupUi(this);

    setWindowTitle(tr("Note"));

    fillNotebookNames();
    m_pUi->notebookComboBox->setModel(m_pNotebookNamesModel);

    QCompleter * pCompleter = m_pUi->notebookComboBox->completer();
    if (pCompleter) {
        pCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    }

    fillDialogContent();

    if (m_readOnlyMode)
    {
        m_pUi->titleLineEdit->setReadOnly(true);
        m_pUi->notebookComboBox->setEditable(false);
        m_pUi->creationDateTimeEdit->setReadOnly(true);
        m_pUi->modificationDateTimeEdit->setReadOnly(true);
        m_pUi->deletionDateTimeEdit->setReadOnly(true);
        m_pUi->subjectDateTimeEdit->setReadOnly(true);
        m_pUi->authorLineEdit->setReadOnly(true);
        m_pUi->sourceLineEdit->setReadOnly(true);
        m_pUi->sourceURLLineEdit->setReadOnly(true);
        m_pUi->sourceApplicationLineEdit->setReadOnly(true);
        m_pUi->latitudeSpinBox->setReadOnly(true);
        m_pUi->longitudeSpinBox->setReadOnly(true);
        m_pUi->altitudeSpinBox->setReadOnly(true);

        m_pUi->buttonBox->setStandardButtons(
            QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok));

        m_pUi->favoritedCheckBox->setAttribute(Qt::WA_TransparentForMouseEvents,
                                               true);
        m_pUi->favoritedCheckBox->setFocusPolicy(Qt::NoFocus);
    }

    // Synchronizable and dirty checkboxes are read-only all the time
    m_pUi->synchronizableCheckBox->setAttribute(Qt::WA_TransparentForMouseEvents,
                                                true);
    m_pUi->synchronizableCheckBox->setFocusPolicy(Qt::NoFocus);

    m_pUi->dirtyCheckBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_pUi->dirtyCheckBox->setFocusPolicy(Qt::NoFocus);

    createConnections();
}

EditNoteDialog::~EditNoteDialog()
{
    delete m_pUi;
}

void EditNoteDialog::accept()
{
    QNDEBUG("EditNoteDialog::accept");

    if (m_readOnlyMode) {
        QDialog::accept();
        return;
    }

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        ErrorString error(QT_TR_NOOP("Can't edit note: no notebook model is set"));
        QNINFO(error);
        QToolTip::showText(m_pUi->notebookComboBox->geometry().bottomLeft(),
                           error.localizedString());
        return;
    }

    Note modifiedNote = m_note;

    QString title = m_pUi->titleLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(title);

    ErrorString error;
    if (!Note::validateTitle(title, &error)) {
        QNINFO(error);
        QToolTip::showText(m_pUi->titleLineEdit->geometry().bottomLeft(),
                           error.localizedString());
        return;
    }

    modifiedNote.setTitle(title);

    QString notebookName = m_pUi->notebookComboBox->currentText();
    QString notebookLocalUid =
        m_pNotebookModel->localUidForItemName(notebookName,
                                              /* linked notebook guid = */ QString());
    if (notebookLocalUid.isEmpty()) {
        ErrorString error(QT_TR_NOOP("Can't edit note: can't find notebook local "
                                     "uid for current notebook name"));
        QNINFO(error);
        QToolTip::showText(m_pUi->notebookComboBox->geometry().bottomLeft(),
                           error.localizedString());
        return;
    }

    if (!modifiedNote.hasNotebookLocalUid() ||
        (modifiedNote.notebookLocalUid() != notebookLocalUid))
    {
        QNTRACE("Notebook local uid " << notebookLocalUid
                << " is different from that within the note: "
                << (modifiedNote.hasNotebookLocalUid()
                    ? modifiedNote.notebookLocalUid()
                    : QStringLiteral("<empty>")));

        QModelIndex notebookIndex =
            m_pNotebookModel->indexForLocalUid(notebookLocalUid);
        const NotebookModelItem * pNotebookModelItem =
            m_pNotebookModel->itemForIndex(notebookIndex);
        if (Q_UNLIKELY(!pNotebookModelItem))
        {
            ErrorString error(QT_TR_NOOP("Can't edit note: can't find the notebook "
                                         "model item for notebook local uid"));
            QNINFO(error);
            QToolTip::showText(m_pUi->notebookComboBox->geometry().bottomLeft(),
                               error.localizedString());
            return;
        }

        if (Q_UNLIKELY(pNotebookModelItem->type() != NotebookModelItem::Type::Notebook))
        {
            ErrorString error(QT_TR_NOOP("Can't edit note: internal error, "
                                         "the notebook model item corresponding "
                                         "to the chosen notebook has wrong type"));
            QNINFO(error);
            QToolTip::showText(m_pUi->notebookComboBox->geometry().bottomLeft(),
                               error.localizedString());
            return;
        }

        const NotebookItem * pNotebookItem = pNotebookModelItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem))
        {
            ErrorString error(QT_TR_NOOP("Can't edit note: internal error, "
                                         "the notebook model item corresponding "
                                         "to the chosen notebook has null pointer "
                                         "to the notebook item"));
            QNINFO(error);
            QToolTip::showText(m_pUi->notebookComboBox->geometry().bottomLeft(),
                               error.localizedString());
            return;
        }

        if (Q_UNLIKELY(!pNotebookItem->canCreateNotes()))
        {
            ErrorString error(QT_TR_NOOP("Can't edit note: the target notebook "
                                         "doesn't allow the note creation in it"));
            QNINFO(error);
            QToolTip::showText(m_pUi->notebookComboBox->geometry().bottomLeft(),
                               error.localizedString());
            return;
        }

        modifiedNote.setNotebookLocalUid(notebookLocalUid);
        modifiedNote.setNotebookGuid(pNotebookItem->guid());
        QNTRACE("Successfully set the note's notebook to "
                << notebookName << " ("
                << notebookLocalUid << ")");
    }

    if (m_creationDateTimeEdited) {
        QDateTime creationDateTime = m_pUi->creationDateTimeEdit->dateTime();
        modifiedNote.setCreationTimestamp(creationDateTime.toMSecsSinceEpoch());
    }

    if (m_modificationDateTimeEdited) {
        QDateTime modificationDateTime = m_pUi->modificationDateTimeEdit->dateTime();
        modifiedNote.setModificationTimestamp(modificationDateTime.toMSecsSinceEpoch());
    }

    if (m_deletionDateTimeEdited) {
        QDateTime deletionDateTime = m_pUi->deletionDateTimeEdit->dateTime();
        modifiedNote.setDeletionTimestamp(deletionDateTime.toMSecsSinceEpoch());
    }

    modifiedNote.setFavorited(m_pUi->favoritedCheckBox->isChecked());

    QDateTime subjectDateTime = m_pUi->subjectDateTimeEdit->dateTime();

    QString author = m_pUi->authorLineEdit->text();
    m_stringUtils.removeNewlines(author);

    QString source = m_pUi->sourceLineEdit->text();
    m_stringUtils.removeNewlines(source);

    QString sourceURL = m_pUi->sourceURLLineEdit->text();
    m_stringUtils.removeNewlines(sourceURL);

    QString sourceApplication = m_pUi->sourceApplicationLineEdit->text();
    m_stringUtils.removeNewlines(sourceApplication);

    QString placeName = m_pUi->placeNameLineEdit->text();
    m_stringUtils.removeNewlines(placeName);

    double latitude = m_pUi->latitudeSpinBox->value();
    double longitude = m_pUi->longitudeSpinBox->value();
    double altitude = m_pUi->altitudeSpinBox->value();

    if ((!m_subjectDateTimeEdited || !subjectDateTime.isValid()) &&
        author.isEmpty() && source.isEmpty() && sourceURL.isEmpty() &&
        sourceApplication.isEmpty() && placeName.isEmpty() &&
        !m_latitudeEdited && !m_longitudeEdited && !m_altitudeEdited)
    {
        QNTRACE("All note attributes parameters editable via "
                "EditNoteDialog are empty");
        if (modifiedNote.hasNoteAttributes()) {
            modifiedNote.clearNoteAttributes();
        }
    }
    else
    {
        qevercloud::NoteAttributes & noteAttributes = modifiedNote.noteAttributes();

        if (m_subjectDateTimeEdited && subjectDateTime.isValid()) {
            noteAttributes.subjectDate = subjectDateTime.toMSecsSinceEpoch();
        }
        else {
            noteAttributes.subjectDate.clear();
        }

#define CHECK_ATTRIBUTE(attr)                                                  \
    int attr##Len = attr.size();                                               \
    if (attr##Len < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) {                      \
        ErrorString error(QT_TR_NOOP("Attribute length too small"));           \
        error.details() = QStringLiteral("min ");                              \
        error.details() += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);\
        QNINFO(error);                                                         \
        QToolTip::showText(m_pUi->attr##LineEdit->geometry().bottomLeft(),     \
                           error.localizedString());                           \
        return;                                                                \
    }                                                                          \
    else if (attr##Len > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) {                 \
        ErrorString error(QT_TR_NOOP("Attribute length too large"));           \
        error.details() = QStringLiteral("max ");                              \
        error.details() += QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);\
        QNINFO(error);                                                         \
        QToolTip::showText(m_pUi->attr##LineEdit->geometry().bottomLeft(),     \
                           error.localizedString());                           \
        return;                                                                \
    }                                                                          \
// CHECK_ATTRIBUTE

        if (!author.isEmpty()) {
            CHECK_ATTRIBUTE(author)
            noteAttributes.author = author;
        }
        else {
            noteAttributes.author.clear();
        }

        if (!source.isEmpty()) {
            CHECK_ATTRIBUTE(source)
            noteAttributes.source = source;
        }
        else {
            noteAttributes.source.clear();
        }

        if (!sourceURL.isEmpty()) {
            CHECK_ATTRIBUTE(sourceURL)
            noteAttributes.sourceURL = sourceURL;
        }
        else {
            noteAttributes.sourceURL.clear();
        }

        if (!sourceApplication.isEmpty()) {
            CHECK_ATTRIBUTE(sourceApplication)
            noteAttributes.sourceApplication = sourceApplication;
        }
        else {
            noteAttributes.sourceApplication.clear();
        }

        if (!placeName.isEmpty()) {
            CHECK_ATTRIBUTE(placeName)
            noteAttributes.placeName = placeName;
        }
        else {
            noteAttributes.placeName.clear();
        }

#undef CHECK_ATTRIBUTE

        if (m_latitudeEdited) {
            noteAttributes.latitude = latitude;
        }
        else {
            noteAttributes.latitude.clear();
        }

        if (m_longitudeEdited) {
            noteAttributes.longitude = longitude;
        }
        else {
            noteAttributes.longitude.clear();
        }

        if (m_altitudeEdited) {
            noteAttributes.altitude = altitude;
        }
        else {
            noteAttributes.altitude.clear();
        }
    }

    m_note = modifiedNote;
    QNTRACE("Edited note: " << m_note);

    QDialog::accept();
}

void EditNoteDialog::dataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QNDEBUG("EditNoteDialog::dataChanged");

    Q_UNUSED(roles)

    if (!topLeft.isValid() || !bottomRight.isValid()) {
        QNDEBUG("At least one of changed indexes is invalid");
        fillNotebookNames();
        return;
    }

    if ((topLeft.column() > NotebookModel::Columns::Name) ||
        (bottomRight.column() < NotebookModel::Columns::Name))
    {
        QNTRACE("The updated indexed don't contain the notebook name");
        return;
    }

    fillNotebookNames();
}

void EditNoteDialog::rowsInserted(const QModelIndex & parent, int start, int end)
{
    QNDEBUG("EditNoteDialog::rowsInserted");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    fillNotebookNames();
}

void EditNoteDialog::rowsAboutToBeRemoved(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("EditNoteDialog::rowsAboutToBeRemoved");

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        QNDEBUG("Notebook model is null, nothing to do");
        return;
    }

    QStringList currentNotebookNames = m_pNotebookNamesModel->stringList();

    for(int i = start; i <= end; ++i)
    {
        QModelIndex index =
            m_pNotebookModel->index(i, NotebookModel::Columns::Name, parent);
        QString removedNotebookName = m_pNotebookModel->data(index).toString();
        if (Q_UNLIKELY(removedNotebookName.isEmpty())) {
            continue;
        }

        auto it = std::lower_bound(currentNotebookNames.constBegin(),
                                   currentNotebookNames.constEnd(),
                                   removedNotebookName);
        if ((it != currentNotebookNames.constEnd()) &&
            (*it == removedNotebookName))
        {
            int offset =
                static_cast<int>(std::distance(currentNotebookNames.constBegin(), it));
            QStringList::iterator nit = currentNotebookNames.begin() + offset;
            Q_UNUSED(currentNotebookNames.erase(nit));
        }
    }

    m_pNotebookNamesModel->setStringList(currentNotebookNames);
}

void EditNoteDialog::onCreationDateTimeEdited(const QDateTime & dateTime)
{
    QNTRACE("EditNoteDialog::onCreationDateTimeEdited: " << dateTime);
    m_creationDateTimeEdited = true;
}

void EditNoteDialog::onModificationDateTimeEdited(const QDateTime & dateTime)
{
    QNTRACE("EditNoteDialog::onModificationDateTimeEdited: " << dateTime);
    m_modificationDateTimeEdited = true;
}

void EditNoteDialog::onDeletionDateTimeEdited(const QDateTime & dateTime)
{
    QNTRACE("EditNoteDialog::onDeletionDateTimeEdited: " << dateTime);
    m_deletionDateTimeEdited = true;
}

void EditNoteDialog::onSubjectDateTimeEdited(const QDateTime & dateTime)
{
    QNTRACE("EditNoteDialog::onSubjectDateTimeEdited: " << dateTime);
    m_subjectDateTimeEdited = true;
}

void EditNoteDialog::onLatitudeValueChanged(double value)
{
    QNTRACE("EditNoteDialog::onLatitudeValueChanged: " << value);
    m_latitudeEdited = true;
}

void EditNoteDialog::onLongitudeValueChanged(double value)
{
    QNTRACE("EditNoteDialog::onLongitudeValueChanged: " << value);
    m_longitudeEdited = true;
}

void EditNoteDialog::onAltitudeValueChanged(double value)
{
    QNTRACE("EditNoteDialog::onAltitudeValueChanged: " << value);
    m_altitudeEdited = true;
}

void EditNoteDialog::createConnections()
{
    QNDEBUG("EditNoteDialog::createConnections");

    if (!m_pNotebookModel.isNull())
    {
        QObject::connect(m_pNotebookModel.data(), &NotebookModel::dataChanged,
                         this, &EditNoteDialog::dataChanged);
        QObject::connect(m_pNotebookModel.data(),
                         QNSIGNAL(NotebookModel,rowsInserted,QModelIndex,int,int),
                         this,
                         QNSLOT(EditNoteDialog,rowsInserted,QModelIndex,int,int));
        QObject::connect(m_pNotebookModel.data(),
                         QNSIGNAL(NotebookModel,rowsAboutToBeRemoved,
                                  QModelIndex,int,int),
                         this,
                         QNSLOT(EditNoteDialog,rowsAboutToBeRemoved,
                                QModelIndex,int,int));
    }

    if (m_readOnlyMode) {
        return;
    }

    QObject::connect(m_pUi->creationDateTimeEdit,
                     QNSIGNAL(QDateTimeEdit,dateTimeChanged,QDateTime),
                     this,
                     QNSLOT(EditNoteDialog,onCreationDateTimeEdited,QDateTime));
    QObject::connect(m_pUi->modificationDateTimeEdit,
                     QNSIGNAL(QDateTimeEdit,dateTimeChanged,QDateTime),
                     this,
                     QNSLOT(EditNoteDialog,onModificationDateTimeEdited,QDateTime));
    QObject::connect(m_pUi->deletionDateTimeEdit,
                     QNSIGNAL(QDateTimeEdit,dateTimeChanged,QDateTime),
                     this,
                     QNSLOT(EditNoteDialog,onDeletionDateTimeEdited,QDateTime));
    QObject::connect(m_pUi->subjectDateTimeEdit,
                     QNSIGNAL(QDateTimeEdit,dateTimeChanged,QDateTime),
                     this,
                     QNSLOT(EditNoteDialog,onSubjectDateTimeEdited,QDateTime));
    QObject::connect(m_pUi->latitudeSpinBox,
                     SIGNAL(valueChanged(double)),
                     this,
                     SLOT(onLatitudeValueChanged(double)));
    QObject::connect(m_pUi->longitudeSpinBox,
                     SIGNAL(valueChanged(double)),
                     this,
                     SLOT(onLongitudeValueChanged(double)));
    QObject::connect(m_pUi->altitudeSpinBox,
                     SIGNAL(valueChanged(double)),
                     this,
                     SLOT(onAltitudeValueChanged(double)));
}

void EditNoteDialog::fillNotebookNames()
{
    QNDEBUG("EditNoteDialog::fillNotebookNames");

    QStringList notebookNames;

    if (!m_readOnlyMode)
    {
        if (!m_pNotebookModel.isNull())
        {
            NotebookModel::NotebookFilters filter(
                NotebookModel::NotebookFilter::CanCreateNotes);
            notebookNames = m_pNotebookModel->notebookNames(filter);
        }

        m_pNotebookNamesModel->setStringList(notebookNames);

        return;
    }

    QNTRACE("In read-only mode, will insert only the current "
            "note's notebook");

    if (!m_note.hasNotebookLocalUid()) {
        QNTRACE("The note has no notebook local uid");
        m_pNotebookNamesModel->setStringList(notebookNames);
        return;
    }

    if (m_pNotebookModel.isNull()) {
        QNTRACE("The notebook model is null");
        m_pNotebookNamesModel->setStringList(notebookNames);
        return;
    }

    QString notebookName =
        m_pNotebookModel->itemNameForLocalUid(m_note.notebookLocalUid());
    if (notebookName.isEmpty()) {
        QNTRACE("Found no notebook name for local uid "
                << m_note.notebookLocalUid());
        m_pNotebookNamesModel->setStringList(notebookNames);
        return;
    }

    notebookNames << notebookName;
    m_pNotebookNamesModel->setStringList(notebookNames);
}

void EditNoteDialog::fillDialogContent()
{
    QNDEBUG("EditNoteDialog::fillDialogContent");

    m_pUi->titleLineEdit->setText(m_note.hasTitle() ? m_note.title() : QString());

    QStringList notebookNames = m_pNotebookNamesModel->stringList();

    bool setNotebookName = false;
    QString notebookName;
    if (m_note.hasNotebookLocalUid() && !m_pNotebookModel.isNull()) {
        notebookName = m_pNotebookModel->itemNameForLocalUid(m_note.notebookLocalUid());
        QNTRACE("Current note's notebook name: " << notebookName
                << ", notebook local uid = " << m_note.notebookLocalUid());
    }

    if (!notebookName.isEmpty())
    {
        auto it = std::lower_bound(notebookNames.constBegin(),
                                   notebookNames.constEnd(),
                                   notebookName);
        if ((it != notebookNames.constEnd()) && (*it == notebookName))
        {
            int index =
                static_cast<int>(std::distance(notebookNames.constBegin(), it));
            m_pUi->notebookComboBox->setCurrentIndex(index);
            QNTRACE("Set the current notebook name index: "
                    << index << ", notebook name: " << notebookName);
            setNotebookName = true;
        }
    }

    if (!setNotebookName) {
        QNTRACE("Wasn't able to find & set the notebook name, "
                "setting the empty name as a fallback...");
        notebookNames.prepend(QString());
        m_pNotebookNamesModel->setStringList(notebookNames);
        m_pUi->notebookComboBox->setCurrentIndex(0);
    }

    if (m_note.hasCreationTimestamp()) {
        m_pUi->creationDateTimeEdit->setDateTime(
            QDateTime::fromMSecsSinceEpoch(m_note.creationTimestamp()));
    }
    else {
        m_pUi->creationDateTimeEdit->setDateTime(QDateTime());
    }

    if (m_note.hasModificationTimestamp()) {
        m_pUi->modificationDateTimeEdit->setDateTime(
            QDateTime::fromMSecsSinceEpoch(m_note.modificationTimestamp()));
    }
    else {
        m_pUi->modificationDateTimeEdit->setDateTime(QDateTime());
    }

    if (m_note.hasDeletionTimestamp()) {
        m_pUi->deletionDateTimeEdit->setDateTime(
            QDateTime::fromMSecsSinceEpoch(m_note.deletionTimestamp()));
    }
    else {
        m_pUi->deletionDateTimeEdit->setDateTime(QDateTime());
    }

    m_pUi->synchronizableCheckBox->setChecked(!m_note.isLocal());
    m_pUi->favoritedCheckBox->setChecked(m_note.isFavorited());
    m_pUi->dirtyCheckBox->setChecked(m_note.isDirty());

    m_pUi->localUidLineEdit->setText(m_note.localUid());

    if (m_note.hasGuid()) {
        m_pUi->guidLineEdit->setText(m_note.guid());
    }

    if (!m_note.hasNoteAttributes())
    {
        QNTRACE("The note has no attributes");

        m_pUi->subjectDateTimeEdit->setDateTime(QDateTime());
        m_pUi->authorLineEdit->setText(QString());
        m_pUi->sourceLineEdit->setText(QString());
        m_pUi->sourceURLLineEdit->setText(QString());
        m_pUi->sourceApplicationLineEdit->setText(QString());
        m_pUi->placeNameLineEdit->setText(QString());
        m_pUi->latitudeSpinBox->clear();
        m_pUi->longitudeSpinBox->clear();
        m_pUi->altitudeSpinBox->clear();

        return;
    }

    const qevercloud::NoteAttributes & attributes = m_note.noteAttributes();

    if (attributes.subjectDate.isSet()) {
        m_pUi->subjectDateTimeEdit->setDateTime(
            QDateTime::fromMSecsSinceEpoch(attributes.subjectDate.ref()));
    }
    else {
        m_pUi->subjectDateTimeEdit->setDateTime(QDateTime());
    }

    m_pUi->authorLineEdit->setText(attributes.author.isSet()
                                   ? attributes.author.ref()
                                   : QString());
    m_pUi->sourceLineEdit->setText(attributes.source.isSet()
                                   ? attributes.source.ref()
                                   : QString());
    m_pUi->sourceURLLineEdit->setText(attributes.sourceURL.isSet()
                                      ? attributes.sourceURL.ref()
                                      : QString());
    m_pUi->sourceApplicationLineEdit->setText(attributes.sourceApplication.isSet()
                                              ? attributes.sourceApplication.ref()
                                              : QString());
    m_pUi->placeNameLineEdit->setText(attributes.placeName.isSet()
                                      ? attributes.placeName.ref()
                                      : QString());

    if (attributes.latitude.isSet()) {
        m_pUi->latitudeSpinBox->setValue(attributes.latitude.ref());
    }
    else {
        m_pUi->latitudeSpinBox->clear();
    }

    if (attributes.longitude.isSet()) {
        m_pUi->longitudeSpinBox->setValue(attributes.longitude.ref());
    }
    else {
        m_pUi->longitudeSpinBox->clear();
    }

    if (attributes.altitude.isSet()) {
        m_pUi->altitudeSpinBox->setValue(attributes.altitude.ref());
    }
    else {
        m_pUi->altitudeSpinBox->clear();
    }
}

} // namespace quentier
