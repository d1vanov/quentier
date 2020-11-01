/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#ifndef QUENTIER_WIKI2ACCOUNT_WIKI_ARTICLES_FETCHING_TRACKER_H
#define QUENTIER_WIKI2ACCOUNT_WIKI_ARTICLES_FETCHING_TRACKER_H

#include <quentier/types/ErrorString.h>

#include <QObject>
#include <QTextStream>

namespace quentier {

class WikiArticlesFetchingTracker final: public QObject
{
    Q_OBJECT
public:
    explicit WikiArticlesFetchingTracker(QObject * parent = nullptr);

    virtual ~WikiArticlesFetchingTracker() override;

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);

public Q_SLOTS:
    void onWikiArticlesFetchingFinished();
    void onWikiArticlesFetchingFailed(ErrorString errorDescription);
    void onWikiArticlesFetchingProgressUpdate(double percentage);

private:
    QTextStream m_stdout;
    QTextStream m_stderr;
    quint32 m_lastReportedProgress = 0;
};

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_WIKI_ARTICLES_FETCHING_TRACKER_H
