/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include <lib/model/notebook/NotebookModel.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Validation.h>

#include <QCompleter>
#include <QDateTime>
#include <QModelIndex>
#include <QStringListModel>
#include <QToolTip>

#include <algorithm>
#include <iterator>

namespace quentier {

EditNoteDialog::EditNoteDialog(
    qevercloud::Note note, NotebookModel * notebookModel, QWidget * parent,
    const bool readOnlyMode) :
    QDialog{parent},
    m_ui{new Ui::EditNoteDialog}, m_note{std::move(note)},
    m_notebookModel{notebookModel},
    m_notebookNamesModel{new QStringListModel(this)},
    m_readOnlyMode{readOnlyMode}
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Note"));

    fillNotebookNames();
    m_ui->notebookComboBox->setModel(m_notebookNamesModel);

    auto * completer = m_ui->notebookComboBox->completer();
    if (completer) {
        completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    }

    fillDialogContent();

    if (m_readOnlyMode) {
        m_ui->titleLineEdit->setReadOnly(true);
        m_ui->notebookComboBox->setEditable(false);
        m_ui->creationDateTimeEdit->setReadOnly(true);
        m_ui->modificationDateTimeEdit->setReadOnly(true);
        m_ui->deletionDateTimeEdit->setReadOnly(true);
        m_ui->subjectDateTimeEdit->setReadOnly(true);
        m_ui->authorLineEdit->setReadOnly(true);
        m_ui->sourceLineEdit->setReadOnly(true);
        m_ui->sourceURLLineEdit->setReadOnly(true);
        m_ui->sourceApplicationLineEdit->setReadOnly(true);
        m_ui->latitudeSpinBox->setReadOnly(true);
        m_ui->longitudeSpinBox->setReadOnly(true);
        m_ui->altitudeSpinBox->setReadOnly(true);

        m_ui->buttonBox->setStandardButtons(
            QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok));

        m_ui->favoritedCheckBox->setAttribute(
            Qt::WA_TransparentForMouseEvents, true);

        m_ui->favoritedCheckBox->setFocusPolicy(Qt::NoFocus);
    }

    // Synchronizable and dirty checkboxes are read-only all the time
    m_ui->synchronizableCheckBox->setAttribute(
        Qt::WA_TransparentForMouseEvents, true);

    m_ui->synchronizableCheckBox->setFocusPolicy(Qt::NoFocus);

    m_ui->dirtyCheckBox->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_ui->dirtyCheckBox->setFocusPolicy(Qt::NoFocus);

    createConnections();
}

EditNoteDialog::~EditNoteDialog()
{
    delete m_ui;
}

void EditNoteDialog::accept()
{
    QNDEBUG("dialog::EditNoteDialog", "EditNoteDialog::accept");

    if (m_readOnlyMode) {
        QDialog::accept();
        return;
    }

    if (Q_UNLIKELY(m_notebookModel.isNull())) {
        ErrorString error{
            QT_TR_NOOP("Can't edit note: no notebook model is set")};
        QNINFO("dialog::EditNoteDialog", error);

        QToolTip::showText(
            m_ui->notebookComboBox->geometry().bottomLeft(),
            error.localizedString());

        return;
    }

    auto modifiedNote = m_note;

    QString title = m_ui->titleLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(title);

    ErrorString error;
    if (!validateNoteTitle(title, &error)) {
        QNINFO("dialog::EditNoteDialog", error);

        QToolTip::showText(
            m_ui->titleLineEdit->geometry().bottomLeft(),
            error.localizedString());

        return;
    }

    modifiedNote.setTitle(title);

    const QString notebookName = m_ui->notebookComboBox->currentText();
    const QString notebookLocalId = m_notebookModel->localIdForItemName(
        notebookName,
        /* linked notebook guid = */ QString());

    if (notebookLocalId.isEmpty()) {
        ErrorString error{
            QT_TR_NOOP("Can't edit note: can't find notebook local "
                       "id for current notebook name")};
        QNINFO("dialog::EditNoteDialog", error);

        QToolTip::showText(
            m_ui->notebookComboBox->geometry().bottomLeft(),
            error.localizedString());
        return;
    }

    if (modifiedNote.notebookLocalId() != notebookLocalId)
    {
        QNTRACE(
            "dialog::EditNoteDialog",
            "Notebook local id " << notebookLocalId
                                 << " is different from that within the note: "
                                 << modifiedNote.notebookLocalId());

        const auto notebookIndex =
            m_notebookModel->indexForLocalId(notebookLocalId);

        const auto * notebookModelItem =
            m_notebookModel->itemForIndex(notebookIndex);

        if (Q_UNLIKELY(!notebookModelItem)) {
            ErrorString error(
                QT_TR_NOOP("Can't edit note: can't find the notebook "
                           "model item for notebook local id"));
            QNINFO("dialog::EditNoteDialog", error);

            QToolTip::showText(
                m_ui->notebookComboBox->geometry().bottomLeft(),
                error.localizedString());

            return;
        }

        const auto * notebookItem = notebookModelItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!notebookItem)) {
            ErrorString error{
                QT_TR_NOOP("Can't edit note: internal error, the notebook "
                           "model item corresponding to the chosen notebook "
                           "has wrong type")};
            QNINFO("dialog::EditNoteDialog", error);

            QToolTip::showText(
                m_ui->notebookComboBox->geometry().bottomLeft(),
                error.localizedString());

            return;
        }

        if (Q_UNLIKELY(!notebookItem->canCreateNotes())) {
            ErrorString error{
                QT_TR_NOOP("Can't edit note: the target notebook "
                           "doesn't allow the note creation in it")};
            QNINFO("dialog::EditNoteDialog", error);

            QToolTip::showText(
                m_ui->notebookComboBox->geometry().bottomLeft(),
                error.localizedString());

            return;
        }

        modifiedNote.setNotebookLocalId(notebookLocalId);
        modifiedNote.setNotebookGuid(notebookItem->guid());

        QNTRACE(
            "dialog::EditNoteDialog",
            "Successfully set the note's notebook to "
                << notebookName << " (" << notebookLocalId << ")");
    }

    if (m_creationDateTimeEdited) {
        QDateTime creationDateTime = m_ui->creationDateTimeEdit->dateTime();
        modifiedNote.setCreated(creationDateTime.toMSecsSinceEpoch());
    }

    if (m_modificationDateTimeEdited) {
        QDateTime modificationDateTime =
            m_ui->modificationDateTimeEdit->dateTime();

        modifiedNote.setUpdated(
            modificationDateTime.toMSecsSinceEpoch());
    }

    if (m_deletionDateTimeEdited) {
        QDateTime deletionDateTime = m_ui->deletionDateTimeEdit->dateTime();
        modifiedNote.setDeleted(deletionDateTime.toMSecsSinceEpoch());
    }

    modifiedNote.setLocallyFavorited(m_ui->favoritedCheckBox->isChecked());

    const QDateTime subjectDateTime = m_ui->subjectDateTimeEdit->dateTime();

    QString author = m_ui->authorLineEdit->text();
    m_stringUtils.removeNewlines(author);

    QString source = m_ui->sourceLineEdit->text();
    m_stringUtils.removeNewlines(source);

    QString sourceURL = m_ui->sourceURLLineEdit->text();
    m_stringUtils.removeNewlines(sourceURL);

    QString sourceApplication = m_ui->sourceApplicationLineEdit->text();
    m_stringUtils.removeNewlines(sourceApplication);

    QString placeName = m_ui->placeNameLineEdit->text();
    m_stringUtils.removeNewlines(placeName);

    const double latitude = m_ui->latitudeSpinBox->value();
    const double longitude = m_ui->longitudeSpinBox->value();
    const double altitude = m_ui->altitudeSpinBox->value();

    if ((!m_subjectDateTimeEdited || !subjectDateTime.isValid()) &&
        author.isEmpty() && source.isEmpty() && sourceURL.isEmpty() &&
        sourceApplication.isEmpty() && placeName.isEmpty() &&
        !m_latitudeEdited && !m_longitudeEdited && !m_altitudeEdited)
    {
        QNTRACE(
            "dialog::EditNoteDialog",
            "All note attributes parameters editable via "
                << "EditNoteDialog are empty");

        if (modifiedNote.attributes()) {
            modifiedNote.setAttributes(std::nullopt);
        }
    }
    else {
        if (modifiedNote.attributes()) {
            modifiedNote.setAttributes(qevercloud::NoteAttributes{});
        }

        auto & noteAttributes = *modifiedNote.mutableAttributes();

        if (m_subjectDateTimeEdited && subjectDateTime.isValid()) {
            noteAttributes.setSubjectDate(subjectDateTime.toMSecsSinceEpoch());
        }
        else {
            noteAttributes.setSubjectDate(std::nullopt);
        }

#define CHECK_ATTRIBUTE(attr)                                                  \
    int attr##Len = attr.size();                                               \
    if (attr##Len < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) {                      \
        ErrorString error{QT_TR_NOOP("Attribute length too small")};           \
        error.details() = QStringLiteral("min ");                              \
        error.details() +=                                                     \
            QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MIN);               \
        QNINFO("dialog::EditNoteDialog", error);                               \
        QToolTip::showText(                                                    \
            m_ui->attr##LineEdit->geometry().bottomLeft(),                     \
            error.localizedString());                                          \
        return;                                                                \
    }                                                                          \
    else if (attr##Len > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) {                 \
        ErrorString error{QT_TR_NOOP("Attribute length too large")};           \
        error.details() = QStringLiteral("max ");                              \
        error.details() +=                                                     \
            QString::number(qevercloud::EDAM_ATTRIBUTE_LEN_MAX);               \
        QNINFO("dialog::EditNoteDialog", error);                               \
        QToolTip::showText(                                                    \
            m_ui->attr##LineEdit->geometry().bottomLeft(),                     \
            error.localizedString());                                          \
        return;                                                                \
    }

        if (!author.isEmpty()) {
            CHECK_ATTRIBUTE(author)
            noteAttributes.setAuthor(std::move(author));
        }
        else {
            noteAttributes.setAuthor(std::nullopt);
        }

        if (!source.isEmpty()) {
            CHECK_ATTRIBUTE(source)
            noteAttributes.setSource(std::move(source));
        }
        else {
            noteAttributes.setSource(std::nullopt);
        }

        if (!sourceURL.isEmpty()) {
            CHECK_ATTRIBUTE(sourceURL)
            noteAttributes.setSourceURL(std::move(sourceURL));
        }
        else {
            noteAttributes.setSourceURL(std::nullopt);
        }

        if (!sourceApplication.isEmpty()) {
            CHECK_ATTRIBUTE(sourceApplication)
            noteAttributes.setSourceApplication(std::move(sourceApplication));
        }
        else {
            noteAttributes.setSourceApplication(std::nullopt);
        }

        if (!placeName.isEmpty()) {
            CHECK_ATTRIBUTE(placeName)
            noteAttributes.setPlaceName(std::move(placeName));
        }
        else {
            noteAttributes.setPlaceName(std::nullopt);
        }

#undef CHECK_ATTRIBUTE

        if (m_latitudeEdited) {
            noteAttributes.setLatitude(latitude);
        }
        else {
            noteAttributes.setLatitude(std::nullopt);
        }

        if (m_longitudeEdited) {
            noteAttributes.setLongitude(longitude);
        }
        else {
            noteAttributes.setLongitude(std::nullopt);
        }

        if (m_altitudeEdited) {
            noteAttributes.setAltitude(altitude);
        }
        else {
            noteAttributes.setAltitude(std::nullopt);
        }
    }

    m_note = modifiedNote;
    QNTRACE("dialog::EditNoteDialog", "Edited note: " << m_note);

    QDialog::accept();
}

void EditNoteDialog::dataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QNDEBUG("dialog::EditNoteDialog", "EditNoteDialog::dataChanged");

    Q_UNUSED(roles)

    if (!topLeft.isValid() || !bottomRight.isValid()) {
        QNDEBUG(
            "dialog::EditNoteDialog",
            "At least one of changed indexes is invalid");
        fillNotebookNames();
        return;
    }

    if ((topLeft.column() > static_cast<int>(NotebookModel::Column::Name)) ||
        (bottomRight.column() < static_cast<int>(NotebookModel::Column::Name)))
    {
        QNTRACE(
            "dialog::EditNoteDialog",
            "The updated indexed don't contain the notebook name");
        return;
    }

    fillNotebookNames();
}

void EditNoteDialog::rowsInserted(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("dialog::EditNoteDialog", "EditNoteDialog::rowsInserted");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    fillNotebookNames();
}

void EditNoteDialog::rowsAboutToBeRemoved(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("dialog::EditNoteDialog", "EditNoteDialog::rowsAboutToBeRemoved");

    if (Q_UNLIKELY(m_notebookModel.isNull())) {
        QNDEBUG(
            "dialog::EditNoteDialog", "Notebook model is null, nothing to do");
        return;
    }

    auto currentNotebookNames = m_notebookNamesModel->stringList();

    for (int i = start; i <= end; ++i) {
        const auto index = m_notebookModel->index(
            i, static_cast<int>(NotebookModel::Column::Name), parent);

        const QString removedNotebookName =
            m_notebookModel->data(index).toString();
        if (Q_UNLIKELY(removedNotebookName.isEmpty())) {
            continue;
        }

        const auto it = std::lower_bound(
            currentNotebookNames.constBegin(), currentNotebookNames.constEnd(),
            removedNotebookName);

        if (it != currentNotebookNames.constEnd() &&
            *it == removedNotebookName) {
            const int offset = static_cast<int>(
                std::distance(currentNotebookNames.constBegin(), it));

            auto nit = currentNotebookNames.begin() + offset;
            Q_UNUSED(currentNotebookNames.erase(nit));
        }
    }

    m_notebookNamesModel->setStringList(currentNotebookNames);
}

void EditNoteDialog::onCreationDateTimeEdited(const QDateTime & dateTime)
{
    QNTRACE(
        "dialog::EditNoteDialog",
        "EditNoteDialog::onCreationDateTimeEdited: " << dateTime);

    m_creationDateTimeEdited = true;
}

void EditNoteDialog::onModificationDateTimeEdited(const QDateTime & dateTime)
{
    QNTRACE(
        "dialog::EditNoteDialog",
        "EditNoteDialog::onModificationDateTimeEdited: " << dateTime);

    m_modificationDateTimeEdited = true;
}

void EditNoteDialog::onDeletionDateTimeEdited(const QDateTime & dateTime)
{
    QNTRACE(
        "dialog::EditNoteDialog",
        "EditNoteDialog::onDeletionDateTimeEdited: " << dateTime);

    m_deletionDateTimeEdited = true;
}

void EditNoteDialog::onSubjectDateTimeEdited(const QDateTime & dateTime)
{
    QNTRACE(
        "dialog::EditNoteDialog",
        "EditNoteDialog::onSubjectDateTimeEdited: " << dateTime);

    m_subjectDateTimeEdited = true;
}

void EditNoteDialog::onLatitudeValueChanged(double value)
{
    QNTRACE(
        "dialog::EditNoteDialog",
        "EditNoteDialog::onLatitudeValueChanged: " << value);

    m_latitudeEdited = true;
}

void EditNoteDialog::onLongitudeValueChanged(double value)
{
    QNTRACE(
        "dialog::EditNoteDialog",
        "EditNoteDialog::onLongitudeValueChanged: " << value);

    m_longitudeEdited = true;
}

void EditNoteDialog::onAltitudeValueChanged(double value)
{
    QNTRACE(
        "dialog::EditNoteDialog",
        "EditNoteDialog::onAltitudeValueChanged: " << value);

    m_altitudeEdited = true;
}

void EditNoteDialog::createConnections()
{
    QNDEBUG("dialog::EditNoteDialog", "EditNoteDialog::createConnections");

    if (!m_notebookModel.isNull()) {
        QObject::connect(
            m_notebookModel.data(), &NotebookModel::dataChanged, this,
            &EditNoteDialog::dataChanged);

        QObject::connect(
            m_notebookModel.data(), &NotebookModel::rowsInserted, this,
            &EditNoteDialog::rowsInserted);

        QObject::connect(
            m_notebookModel.data(), &NotebookModel::rowsAboutToBeRemoved, this,
            &EditNoteDialog::rowsAboutToBeRemoved);
    }

    if (m_readOnlyMode) {
        return;
    }

    QObject::connect(
        m_ui->creationDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this,
        &EditNoteDialog::onCreationDateTimeEdited);

    QObject::connect(
        m_ui->modificationDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this,
        &EditNoteDialog::onModificationDateTimeEdited);

    QObject::connect(
        m_ui->deletionDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this,
        &EditNoteDialog::onDeletionDateTimeEdited);

    QObject::connect(
        m_ui->subjectDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this,
        &EditNoteDialog::onSubjectDateTimeEdited);

    QObject::connect(
        m_ui->latitudeSpinBox,
        qOverload<double>(&QDoubleSpinBox::valueChanged), this,
        &EditNoteDialog::onLatitudeValueChanged);

    QObject::connect(
        m_ui->longitudeSpinBox,
        qOverload<double>(&QDoubleSpinBox::valueChanged), this,
        &EditNoteDialog::onLongitudeValueChanged);

    QObject::connect(
        m_ui->altitudeSpinBox,
        qOverload<double>(&QDoubleSpinBox::valueChanged), this,
        &EditNoteDialog::onAltitudeValueChanged);
}

void EditNoteDialog::fillNotebookNames()
{
    QNDEBUG("dialog::EditNoteDialog", "EditNoteDialog::fillNotebookNames");

    QStringList notebookNames;

    if (!m_readOnlyMode) {
        if (!m_notebookModel.isNull()) {
            NotebookModel::Filters filter =
                NotebookModel::Filter::CanCreateNotes;

            notebookNames = m_notebookModel->notebookNames(filter);
        }

        m_notebookNamesModel->setStringList(notebookNames);
        return;
    }

    QNTRACE(
        "dialog::EditNoteDialog",
        "In read-only mode, will insert only the current note's notebook");

    if (m_note.notebookLocalId().isEmpty()) {
        QNTRACE("dialog::EditNoteDialog", "The note has no notebook local id");
        m_notebookNamesModel->setStringList(notebookNames);
        return;
    }

    if (m_notebookModel.isNull()) {
        QNTRACE("dialog::EditNoteDialog", "The notebook model is null");
        m_notebookNamesModel->setStringList(notebookNames);
        return;
    }

    const QString notebookName =
        m_notebookModel->itemNameForLocalId(m_note.notebookLocalId());

    if (notebookName.isEmpty()) {
        QNTRACE(
            "dialog::EditNoteDialog",
            "Found no notebook name for local id "
                << m_note.notebookLocalId());
        m_notebookNamesModel->setStringList(notebookNames);
        return;
    }

    notebookNames << notebookName;
    m_notebookNamesModel->setStringList(notebookNames);
}

void EditNoteDialog::fillDialogContent()
{
    QNDEBUG("dialog::EditNoteDialog", "EditNoteDialog::fillDialogContent");

    m_ui->titleLineEdit->setText(m_note.title().value_or(QString{}));

    QStringList notebookNames = m_notebookNamesModel->stringList();

    bool setNotebookName = false;
    QString notebookName;
    if (!m_note.notebookLocalId().isEmpty() && !m_notebookModel.isNull()) {
        notebookName =
            m_notebookModel->itemNameForLocalId(m_note.notebookLocalId());

        QNTRACE(
            "dialog::EditNoteDialog",
            "Current note's notebook name: " << notebookName
                                             << ", notebook local id = "
                                             << m_note.notebookLocalId());
    }

    if (!notebookName.isEmpty()) {
        const auto it = std::lower_bound(
            notebookNames.constBegin(), notebookNames.constEnd(), notebookName);

        if (it != notebookNames.constEnd() && *it == notebookName) {
            const int index =
                static_cast<int>(std::distance(notebookNames.constBegin(), it));

            m_ui->notebookComboBox->setCurrentIndex(index);
            QNTRACE(
                "dialog::EditNoteDialog",
                "Set the current notebook name index: "
                    << index << ", notebook name: " << notebookName);

            setNotebookName = true;
        }
    }

    if (!setNotebookName) {
        QNTRACE(
            "dialog::EditNoteDialog",
            "Wasn't able to find & set the notebook name, "
                << "setting the empty name as a fallback...");

        notebookNames.prepend(QString{});
        m_notebookNamesModel->setStringList(notebookNames);
        m_ui->notebookComboBox->setCurrentIndex(0);
    }

    if (m_note.created()) {
        m_ui->creationDateTimeEdit->setDateTime(
            QDateTime::fromMSecsSinceEpoch(*m_note.created()));
    }
    else {
        m_ui->creationDateTimeEdit->setDateTime(QDateTime{});
    }

    if (m_note.updated()) {
        m_ui->modificationDateTimeEdit->setDateTime(
            QDateTime::fromMSecsSinceEpoch(*m_note.updated()));
    }
    else {
        m_ui->modificationDateTimeEdit->setDateTime(QDateTime{});
    }

    if (m_note.deleted()) {
        m_ui->deletionDateTimeEdit->setDateTime(
            QDateTime::fromMSecsSinceEpoch(*m_note.deleted()));
    }
    else {
        m_ui->deletionDateTimeEdit->setDateTime(QDateTime{});
    }

    m_ui->synchronizableCheckBox->setChecked(!m_note.isLocalOnly());
    m_ui->favoritedCheckBox->setChecked(m_note.isLocallyFavorited());
    m_ui->dirtyCheckBox->setChecked(m_note.isLocallyModified());

    m_ui->localIdLineEdit->setText(m_note.localId());

    if (m_note.guid()) {
        m_ui->guidLineEdit->setText(*m_note.guid());
    }

    if (!m_note.attributes()) {
        QNTRACE("dialog::EditNoteDialog", "The note has no attributes");

        m_ui->subjectDateTimeEdit->setDateTime(QDateTime{});
        m_ui->authorLineEdit->setText(QString{});
        m_ui->sourceLineEdit->setText(QString{});
        m_ui->sourceURLLineEdit->setText(QString{});
        m_ui->sourceApplicationLineEdit->setText(QString{});
        m_ui->placeNameLineEdit->setText(QString{});
        m_ui->latitudeSpinBox->clear();
        m_ui->longitudeSpinBox->clear();
        m_ui->altitudeSpinBox->clear();

        return;
    }

    const auto & attributes = *m_note.attributes();

    if (attributes.subjectDate()) {
        m_ui->subjectDateTimeEdit->setDateTime(
            QDateTime::fromMSecsSinceEpoch(*attributes.subjectDate()));
    }
    else {
        m_ui->subjectDateTimeEdit->setDateTime(QDateTime{});
    }

    m_ui->authorLineEdit->setText(attributes.author().value_or(QString{}));
    m_ui->sourceLineEdit->setText(attributes.source().value_or(QString{}));

    m_ui->sourceURLLineEdit->setText(
        attributes.sourceURL().value_or(QString{}));

    m_ui->sourceApplicationLineEdit->setText(
        attributes.sourceApplication().value_or(QString{}));

    m_ui->placeNameLineEdit->setText(
        attributes.placeName().value_or(QString{}));

    if (attributes.latitude()) {
        m_ui->latitudeSpinBox->setValue(*attributes.latitude());
    }
    else {
        m_ui->latitudeSpinBox->clear();
    }

    if (attributes.longitude()) {
        m_ui->longitudeSpinBox->setValue(*attributes.longitude());
    }
    else {
        m_ui->longitudeSpinBox->clear();
    }

    if (attributes.altitude()) {
        m_ui->altitudeSpinBox->setValue(*attributes.altitude());
    }
    else {
        m_ui->altitudeSpinBox->clear();
    }
}

} // namespace quentier
