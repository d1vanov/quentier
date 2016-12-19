#include "AddOrEditTagDialog.h"
#include "ui_AddOrEditTagDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <QStringListModel>
#include <algorithm>
#include <iterator>

namespace quentier {

AddOrEditTagDialog::AddOrEditTagDialog(TagModel * pTagModel, QWidget * parent,
                                       const QString & editedTagLocalUid) :
    QDialog(parent),
    m_pUi(new Ui::AddOrEditTagDialog),
    m_pTagModel(pTagModel),
    m_pTagNamesModel(Q_NULLPTR),
    m_editedTagLocalUid(editedTagLocalUid),
    m_currentTagName()
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->setHidden(true);

    QStringList tagNames;
    if (!m_pTagModel.isNull()) {
        tagNames = m_pTagModel->tagNames();
    }

    if (!tagNames.isEmpty()) {
        qSort(tagNames);
        m_pTagNamesModel = new QStringListModel(this);
        m_pTagNamesModel->setStringList(tagNames);
        m_pUi->parentTagNameComboBox->setModel(m_pTagNamesModel);
    }

    createConnections();

    if (!m_editedTagLocalUid.isEmpty() && !m_pTagModel.isNull())
    {
        QModelIndex editedTagIndex = m_pTagModel->indexForLocalUid(m_editedTagLocalUid);
        const TagModelItem * pItem = m_pTagModel->itemForIndex(editedTagIndex);
        if (!pItem)
        {
            m_pUi->statusBar->setText(tr("Can't find the edited tag within the model"));
            m_pUi->statusBar->setHidden(false);
        }
        else
        {
            m_pUi->tagNameLineEdit->setText(pItem->name());

            if (!tagNames.isEmpty())
            {
                QModelIndex editedTagParentIndex = editedTagIndex.parent();
                const TagModelItem * pParentItem = pItem->parent();
                if (editedTagParentIndex.isValid() && pParentItem)
                {
                    const QString & parentTagName = pParentItem->name();
                    auto it = std::lower_bound(tagNames.constBegin(), tagNames.constEnd(), parentTagName);
                    if ((it != tagNames.constEnd()) && (parentTagName == *it)) {
                        int offset = static_cast<int>(std::distance(tagNames.constBegin(), it));
                        m_pUi->parentTagNameComboBox->setCurrentIndex(offset);
                    }
                }
            }
        }
    }

    m_pUi->tagNameLineEdit->setFocus();
}

AddOrEditTagDialog::~AddOrEditTagDialog()
{
    delete m_pUi;
}

void AddOrEditTagDialog::accept()
{
    QString tagName = m_pUi->tagNameLineEdit->text();
    QString parentTagName = m_pUi->parentTagNameComboBox->currentText();

    QNDEBUG(QStringLiteral("AddOrEditTagDialog::accept: tag name = ") << tagName
            << QStringLiteral(", parent tag name = ") << parentTagName);

#define REPORT_ERROR(error) \
    m_pUi->statusBar->setText(tr(error)); \
    QNWARNING(error); \
    m_pUi->statusBar->setHidden(false)

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        REPORT_ERROR("Can't accept new tag or edit the existing one: tag model is gone");
        return;
    }

    if (m_editedTagLocalUid.isEmpty())
    {
        QNDEBUG(QStringLiteral("Edited tag local uid is empty, adding new tag to the model"));

        // TODO: implement the proper helper method in the tag model and call it here
    }
    else
    {
        QNDEBUG(QStringLiteral("Edited tag local uid is not empty, editing "
                               "the existing tag within the model"));

        QModelIndex index = m_pTagModel->indexForLocalUid(m_editedTagLocalUid);
        const TagModelItem * pItem = m_pTagModel->itemForIndex(index);
        if (Q_UNLIKELY(!pItem)) {
            REPORT_ERROR("Can't edit tag: tag was not found in the model");
            return;
        }

        // If needed, update the tag name
        QModelIndex nameIndex = m_pTagModel->index(index.row(), TagModel::Columns::Name,
                                                   index.parent());
        if (pItem->name().toUpper() != tagName.toUpper())
        {
            bool res = m_pTagModel->setData(nameIndex, tagName);
            if (Q_UNLIKELY(!res))
            {
                // Probably the new name collides with some existing tag's name
                QModelIndex existingItemIndex = m_pTagModel->indexForTagName(tagName);
                if (existingItemIndex.isValid() &&
                    ((existingItemIndex.row() != nameIndex.row()) ||
                     (existingItemIndex.parent() != nameIndex.parent())))
                {
                    // The new name collides with some existing tag and now with the currently edited one
                    REPORT_ERROR("The tag name must be unique in case insensitive manner");
                }
                else
                {
                    // Don't really know what happened...
                    REPORT_ERROR("Can't set this name for the tag");
                }

                return;
            }
        }

        // TODO: if needed, set the new parent tag for the edited one
    }

    QDialog::accept();
}

void AddOrEditTagDialog::onTagNameEdited(const QString & tagName)
{
    QNDEBUG(QStringLiteral("AddOrEditTagDialog::onTagNameEdited: ") << tagName);

    if (Q_UNLIKELY(m_currentTagName == tagName)) {
        QNTRACE(QStringLiteral("Current tag name hasn't changed"));
        return;
    }

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        QNTRACE(QStringLiteral("Tag model is missing"));
        return;
    }

    m_pUi->statusBar->clear();
    m_pUi->statusBar->setHidden(true);

    QStringList tagNames;
    if (m_pTagNamesModel) {
        tagNames = m_pTagNamesModel->stringList();
    }

    bool changed = false;

    // 1) If previous tag name corresponds to a real tag and is missing within the tag names,
    //    need to insert it there
    QModelIndex previousTagNameIndex = m_pTagModel->indexForTagName(m_currentTagName);
    if (previousTagNameIndex.isValid())
    {
        auto it = std::lower_bound(tagNames.constBegin(), tagNames.constEnd(), m_currentTagName);
        if ((it != tagNames.constEnd()) && (m_currentTagName != *it))
        {
            // Found the tag name "larger" than the previous tag name
            auto offset = std::distance(tagNames.constBegin(), it);
            QStringList::iterator nonConstIt = tagNames.begin();
            std::advance(nonConstIt, offset);
            Q_UNUSED(tagNames.insert(nonConstIt, m_currentTagName))
            changed = true;
        }
        else if (it == tagNames.constEnd())
        {
            tagNames.append(m_currentTagName);
            changed = true;
        }
    }

    // 2) If the new edited tag name is within the list of tag names,
    //    need to remove it from there
    auto it = std::lower_bound(tagNames.constBegin(), tagNames.constEnd(), tagName);
    if ((it != tagNames.constEnd()) && (tagName == *it))
    {
        auto offset = std::distance(tagNames.constBegin(), it);
        QStringList::iterator nonConstIt = tagNames.begin();
        std::advance(nonConstIt, offset);
        Q_UNUSED(tagNames.erase(nonConstIt))
        changed = true;
    }

    m_currentTagName = tagName;

    if (!changed) {
        return;
    }

    if (!m_pTagNamesModel && !tagNames.isEmpty()) {
        m_pTagNamesModel = new QStringListModel(this);
        m_pUi->parentTagNameComboBox->setModel(m_pTagNamesModel);
    }

    if (m_pTagNamesModel) {
        m_pTagNamesModel->setStringList(tagNames);
    }
}

void AddOrEditTagDialog::createConnections()
{
    QObject::connect(m_pUi->tagNameLineEdit, QNSIGNAL(QLineEdit,textEdited,const QString &),
                     this, QNSLOT(AddOrEditTagDialog,onTagNameEdited,const QString &));
}

} // namespace quentier
