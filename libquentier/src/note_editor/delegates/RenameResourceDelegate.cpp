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

#include "RenameResourceDelegate.h"
#include "../NoteEditor_p.h"
#include "../GenericResourceImageManager.h"
#include "../dialogs/RenameResourceDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <QScopedPointer>
#include <QBuffer>
#include <QImage>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't rename the attachment: no note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

RenameResourceDelegate::RenameResourceDelegate(const ResourceWrapper & resource, NoteEditorPrivate & noteEditor,
                                               GenericResourceImageManager * pGenericResourceImageManager,
                                               QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                                               const bool performingUndo) :
    m_noteEditor(noteEditor),
    m_pGenericResourceImageManager(pGenericResourceImageManager),
    m_genericResourceImageFilePathsByResourceHash(genericResourceImageFilePathsByResourceHash),
    m_resource(resource),
    m_oldResourceName(resource.displayName()),
    m_newResourceName(),
    m_shouldGetResourceNameFromDialog(true),
    m_performingUndo(performingUndo),
    m_pNote(noteEditor.notePtr())
#ifdef QUENTIER_USE_QT_WEB_ENGINE
    ,
    m_genericResourceImageWriterRequestId()
#endif
{}

void RenameResourceDelegate::start()
{
    QNDEBUG("RenameResourceDelegate::start");

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(RenameResourceDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        doStart();
    }
}

void RenameResourceDelegate::startWithPresetNames(const QString & oldResourceName, const QString & newResourceName)
{
    QNDEBUG("RenameResourceDelegate::startWithPresetNames: old resource name = " << oldResourceName
            << ", new resource name = " << newResourceName);

    m_oldResourceName = oldResourceName;
    m_newResourceName = newResourceName;
    m_shouldGetResourceNameFromDialog = false;

    start();
}

void RenameResourceDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("RenameResourceDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(RenameResourceDelegate,onOriginalPageConvertedToNote,Note));

    doStart();
}

#define CHECK_NOTE_ACTUALITY() \
    if (m_noteEditor.notePtr() != m_pNote) { \
        QNLocalizedString error = QT_TR_NOOP("the note set to the note editor was changed during the attachment renaming, " \
                                             "the action was not completed"); \
        QNDEBUG(error); \
        emit notifyError(error); \
        return; \
    }

void RenameResourceDelegate::doStart()
{
    QNDEBUG("RenameResourceDelegate::doStart");

    CHECK_NOTE_ACTUALITY()

    if (Q_UNLIKELY(!m_resource.hasDataHash())) {
        QNLocalizedString error = QT_TR_NOOP("can't rename the attachment: the data hash is missing");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    if (m_shouldGetResourceNameFromDialog)
    {
        raiseRenameResourceDialog();
    }
    else
    {
        m_resource.setDisplayName(m_newResourceName);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
        buildAndSaveGenericResourceImage();
#else
        emit finished(m_oldResourceName, m_newResourceName, m_resource, m_performingUndo);
#endif
    }
}

void RenameResourceDelegate::raiseRenameResourceDialog()
{
    QNDEBUG("RenameResourceDelegate::raiseRenameResourceDialog");

    QScopedPointer<RenameResourceDialog> pRenameResourceDialog(new RenameResourceDialog(m_oldResourceName, &m_noteEditor));
    pRenameResourceDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pRenameResourceDialog.data(), QNSIGNAL(RenameResourceDialog,accepted,QString),
                     this, QNSLOT(RenameResourceDelegate,onRenameResourceDialogFinished,QString));
    QNTRACE("Will exec rename resource dialog now");
    int res = pRenameResourceDialog->exec();
    if (res == QDialog::Rejected) {
        QNTRACE("Cancelled renaming the resource");
        emit cancelled();
    }
}

void RenameResourceDelegate::onRenameResourceDialogFinished(QString newResourceName)
{
    QNDEBUG("RenameResourceDelegate::onRenameResourceDialogFinished: new resource name = " << newResourceName);

    if (newResourceName.isEmpty()) {
        QNTRACE("New resource name is empty, treating it as cancellation");
        emit cancelled();
        return;
    }

    if (newResourceName == m_oldResourceName) {
        QNTRACE("The new resource name is equal to the old one, treating it as cancellation");
        emit cancelled();
        return;
    }

    m_newResourceName = newResourceName;
    m_resource.setDisplayName(m_newResourceName);
    m_noteEditor.replaceResourceInNote(m_resource);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    buildAndSaveGenericResourceImage();
#else
    emit finished(m_oldResourceName, m_newResourceName, m_resource, m_performingUndo);
#endif
}

#ifdef QUENTIER_USE_QT_WEB_ENGINE
void RenameResourceDelegate::buildAndSaveGenericResourceImage()
{
    QNDEBUG("RenameResourceDelegate::buildAndSaveGenericResourceImage");

    CHECK_NOTE_ACTUALITY()

    QImage resourceImage = m_noteEditor.buildGenericResourceImage(m_resource);

    QByteArray imageData;
    QBuffer buffer(&imageData);
    Q_UNUSED(buffer.open(QIODevice::WriteOnly));
    resourceImage.save(&buffer, "PNG");

    m_genericResourceImageWriterRequestId = QUuid::createUuid();

    QNDEBUG("Emitting request to write generic resource image for resource with local uid "
            << m_resource.localUid() << ", request id " << m_genericResourceImageWriterRequestId
            << ", note local uid = " << m_pNote->localUid());

    QObject::connect(this, QNSIGNAL(RenameResourceDelegate,saveGenericResourceImageToFile,QString,QString,QByteArray,QString,QByteArray,QString,QUuid),
                     m_pGenericResourceImageManager, QNSLOT(GenericResourceImageManager,onGenericResourceImageWriteRequest,QString,QString,QByteArray,QString,QByteArray,QString,QUuid));
    QObject::connect(m_pGenericResourceImageManager, QNSIGNAL(GenericResourceImageManager,genericResourceImageWriteReply,bool,QByteArray,QString,QNLocalizedString,QUuid),
                     this, QNSLOT(RenameResourceDelegate,onGenericResourceImageWriterFinished,bool,QByteArray,QString,QNLocalizedString,QUuid));

    emit saveGenericResourceImageToFile(m_pNote->localUid(), m_resource.localUid(), imageData, "png", m_resource.dataHash(), m_resource.displayName(), m_genericResourceImageWriterRequestId);
}

void RenameResourceDelegate::onGenericResourceImageWriterFinished(bool success, QByteArray resourceHash, QString filePath,
                                                                  QNLocalizedString errorDescription, QUuid requestId)
{
    if (requestId != m_genericResourceImageWriterRequestId) {
        return;
    }

    QNDEBUG("RenameResourceDelegate::onGenericResourceImageWriterFinished: success = " << (success ? "true" : "false")
            << ", resource hash = " << resourceHash.toHex() << ", file path = " << filePath << ", error description = "
            << errorDescription << ", request id = " << requestId);

    QObject::disconnect(this, QNSIGNAL(RenameResourceDelegate,saveGenericResourceImageToFile,QString,QString,QByteArray,QString,QByteArray,QString,QUuid),
                        m_pGenericResourceImageManager, QNSLOT(GenericResourceImageManager,onGenericResourceImageWriteRequest,QString,QString,QByteArray,QString,QByteArray,QString,QUuid));
    QObject::disconnect(m_pGenericResourceImageManager, QNSIGNAL(GenericResourceImageManager,genericResourceImageWriteReply,bool,QByteArray,QString,QString,QUuid),
                        this, QNSLOT(RenameResourceDelegate,onGenericResourceImageWriterFinished,bool,QByteArray,QString,QString,QUuid));

    if (Q_UNLIKELY(!success)) {
        QNLocalizedString error = QT_TR_NOOP("can't rename generic resource: can't write generic resource image to file");
        error += ": ";
        error += errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    m_genericResourceImageFilePathsByResourceHash[resourceHash] = filePath;

    QString javascript = "updateImageResourceSrc('" + QString::fromLocal8Bit(resourceHash.toHex()) + "', '" + filePath + "');";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &RenameResourceDelegate::onGenericResourceImageUpdated));
}

void RenameResourceDelegate::onGenericResourceImageUpdated(const QVariant & data)
{
    QNDEBUG("RenameResourceDelegate::onGenericResourceImageUpdated");

    Q_UNUSED(data)

    emit finished(m_oldResourceName, m_newResourceName, m_resource, m_performingUndo);
}

#endif

} // namespace quentier

