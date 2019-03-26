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

#include "WikiArticleFetcher.h"

#include <quentier/logging/QuentierLogger.h>

#include <QNetworkAccessManager>
#include <QTimer>

#define WALOG_IMPL(message, macro) \
    macro(QStringLiteral("<") << m_wikiArticleUrl << QStringLiteral(">: ") << message)

#define WATRACE(message) \
    WALOG_IMPL(message, QNTRACE)

#define WADEBUG(message) \
    WALOG_IMPL(message, QNDEBUG)

#define WAINFO(message) \
    WALOG_IMPL(message, QNINFO)

#define WAWARNING(message) \
    WALOG_IMPL(message, QNWARNING)

#define WACRITICAL(message) \
    WALOG_IMPL(message, QNCRITICAL)

#define WAFATAL(message) \
    WALOG_IMPL(message, QNFATAL)

namespace quentier {

WikiArticleFetcher::WikiArticleFetcher(QNetworkAccessManager * pNetworkAccessManager,
                                       const QUrl & articleUrl,
                                       const qint64 timeoutMsec,
                                       QObject * parent) :
    m_pNetworkAccessManager(pNetworkAccessManager),
    m_wikiArticleUrl(articleUrl),
    m_pNetworkReply(Q_NULLPTR),
    m_started(false),
    m_finished(false),
    m_timeoutMsec(timeoutMsec),
    m_lastNetworkTime(0),
    m_timeoutTimer(Q_NULLPTR),
    m_timedOut(false),
    m_fetchedData(),
    m_bytesFetched(0),
    m_bytesTotal(0),
    m_status(false),
    m_httpStatusCode(0)
{}

void WikiArticleFetcher::start()
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::start"));

    if (m_started) {
        WADEBUG(QStringLiteral("Already started"));
        return;
    }

    // TODO: implement
}

} // namespace quentier
