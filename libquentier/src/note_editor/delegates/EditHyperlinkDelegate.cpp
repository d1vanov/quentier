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

#include "EditHyperlinkDelegate.h"
#include "../NoteEditor_p.h"
#include "../NoteEditorPage.h"
#include "../dialogs/EditHyperlinkDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <QScopedPointer>
#include <QStringList>

namespace quentier {

EditHyperlinkDelegate::EditHyperlinkDelegate(NoteEditorPrivate & noteEditor, const quint64 hyperlinkId) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_hyperlinkId(hyperlinkId)
{}

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't edit hyperlink: no note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

void EditHyperlinkDelegate::start()
{
    QNDEBUG(QStringLiteral("EditHyperlinkDelegate::start"));

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(EditHyperlinkDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        doStart();
    }
}

void EditHyperlinkDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG(QStringLiteral("EditHyperlinkDelegate::onOriginalPageConvertedToNote"));

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(EditHyperlinkDelegate,onOriginalPageConvertedToNote,Note));

    doStart();
}

void EditHyperlinkDelegate::onHyperlinkDataReceived(const QVariant & data)
{
    QNDEBUG(QStringLiteral("EditHyperlinkDelegate::onHyperlinkDataReceived: data = ") << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QNLocalizedString error = QT_TR_NOOP("can't parse the result of hyperlink data request from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QNLocalizedString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error = QT_TR_NOOP("can't parse the error of hyperlink data request from JavaScript");
        }
        else {
            error = QT_TR_NOOP("can't get hyperlink data from JavaScript");
            error += QStringLiteral(": ");
            error += errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    auto dataIt = resultMap.find(QStringLiteral("data"));
    if (Q_UNLIKELY(dataIt == resultMap.end())) {
        QNLocalizedString error = QT_TR_NOOP("no hyperlink data received from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QStringList hyperlinkDataList = dataIt.value().toStringList();
    if (hyperlinkDataList.isEmpty()) {
        QNLocalizedString error = QT_TR_NOOP("can't edit hyperlink: can't find hyperlink text and link");
        emit notifyError(error);
        return;
    }

    if (hyperlinkDataList.size() != 2) {
        QNLocalizedString error = QT_TR_NOOP("can't edit hyperlink: can't parse hyperlink text and link");
        QNWARNING(error << QStringLiteral("; hyperlink data: ") << hyperlinkDataList.join(QStringLiteral(",")));
        emit notifyError(error);
        return;
    }

    raiseEditHyperlinkDialog(hyperlinkDataList[0], hyperlinkDataList[1]);
}

void EditHyperlinkDelegate::doStart()
{
    QNDEBUG(QStringLiteral("EditHyperlinkDelegate::doStart"));

    QString javascript = QStringLiteral("hyperlinkManager.getHyperlinkData(") + QString::number(m_hyperlinkId) + QStringLiteral(");");

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &EditHyperlinkDelegate::onHyperlinkDataReceived));
}

void EditHyperlinkDelegate::raiseEditHyperlinkDialog(const QString & startupHyperlinkText, const QString & startupHyperlinkUrl)
{
    QNDEBUG(QStringLiteral("EditHyperlinkDelegate::raiseEditHyperlinkDialog: original text = ") << startupHyperlinkText
            << QStringLiteral(", original url: ") << startupHyperlinkUrl);

    QScopedPointer<EditHyperlinkDialog> pEditHyperlinkDialog(new EditHyperlinkDialog(&m_noteEditor, startupHyperlinkText, startupHyperlinkUrl));
    pEditHyperlinkDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pEditHyperlinkDialog.data(), QNSIGNAL(EditHyperlinkDialog,accepted,QString,QUrl,quint64,bool),
                     this, QNSLOT(EditHyperlinkDelegate,onHyperlinkDataEdited,QString,QUrl,quint64,bool));
    QNTRACE(QStringLiteral("Will exec edit hyperlink dialog now"));
    int res = pEditHyperlinkDialog->exec();
    if (res == QDialog::Rejected) {
        QNTRACE(QStringLiteral("Cancelled editing the hyperlink"));
        emit cancelled();
        return;
    }
}

void EditHyperlinkDelegate::onHyperlinkDataEdited(QString text, QUrl url, quint64 hyperlinkId, bool startupUrlWasEmpty)
{
    QNDEBUG(QStringLiteral("EditHyperlinkDelegate::onHyperlinkDataEdited: text = ") << text << QStringLiteral(", url = ")
            << url << QStringLiteral(", hyperlink id = ") << hyperlinkId);

    Q_UNUSED(hyperlinkId)
    Q_UNUSED(startupUrlWasEmpty)

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString urlString = url.toString(QUrl::FullyEncoded);
#else
    QString urlString = url.toString(QUrl::None);
#endif

    QString javascript = QStringLiteral("hyperlinkManager.setHyperlinkData('") + text + QStringLiteral("', '") + urlString +
                         QStringLiteral("', ") + QString::number(m_hyperlinkId) + QStringLiteral(");");

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &EditHyperlinkDelegate::onHyperlinkModified));
}

void EditHyperlinkDelegate::onHyperlinkModified(const QVariant & data)
{
    QNDEBUG(QStringLiteral("EditHyperlinkDelegate::onHyperlinkModified"));

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QNLocalizedString error = QT_TR_NOOP("can't parse the result of hyperlink edit from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QNLocalizedString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error = QT_TR_NOOP("can't parse the error of hyperlink editing from JavaScript");
        }
        else {
            error = QT_TR_NOOP("can't edit hyperlink: ");
            error += QStringLiteral(": ");
            error += errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit finished();
}

} // namespace quentier
