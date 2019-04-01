/*
 * Copyright 2019 Dmitry Ivanov
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

#include "WikiArticleToNote.h"

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/enml/ENMLConverter.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

WikiArticleToNote::WikiArticleToNote(
        ENMLConverter & enmlConverter,
        QObject * parent) :
    QObject(parent),
    m_enmlConverter(enmlConverter)
{}

void WikiArticleToNote::start(QByteArray wikiPageContent)
{
    QNDEBUG(QStringLiteral("WikiArticleToNote::start"));

    if (Q_UNLIKELY(m_started)) {
        QNDEBUG(QStringLiteral("Already started"));
        return;
    }

    m_started = true;
    m_finished = false;

    QString cleanedUpHtml;
    ErrorString errorDescription;
    bool res = m_enmlConverter.cleanupExternalHtml(
        QString::fromUtf8(wikiPageContent),
        cleanedUpHtml, errorDescription);
    if (!res) {
        finishWithError(errorDescription);
        return;
    }

    // TODO: continue from here
}

void WikiArticleToNote::onNetworkReplyFinished(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("WikiArticleToNote::onNetworkReplyFinished: ")
            << (status ? QStringLiteral("success") : QStringLiteral("failure"))
            << QStringLiteral(", error description: ") << errorDescription);

    Q_UNUSED(fetchedData)
    // TODO: implement
}

void WikiArticleToNote::finishWithError(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("WikiArticleToNote::finishWithError: ")
            << errorDescription);

    m_started = false;
    m_finished = false;

    m_note = Note();
    for(auto it = m_imageDataFetchersByResourceLocalUid.constBegin(),
        end = m_imageDataFetchersByResourceLocalUid.constEnd(); it != end; ++it)
    {
        NetworkReplyFetcher * pNetworkReplyFetcher = it.value();
        pNetworkReplyFetcher->disconnect(this);
        pNetworkReplyFetcher->deleteLater();
    }
    m_imageDataFetchersByResourceLocalUid.clear();

    Q_EMIT finished(false, errorDescription, Note());
}

} // namespace quentier
