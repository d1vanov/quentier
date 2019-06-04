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

#include "WikiArticlesFetchingTracker.h"

#include <iostream>
#include <cmath>

namespace quentier {

WikiArticlesFetchingTracker::WikiArticlesFetchingTracker(QObject * parent) :
    QObject(parent),
    m_stdout(stdout),
    m_stderr(stderr)
{}

WikiArticlesFetchingTracker::~WikiArticlesFetchingTracker()
{}

void WikiArticlesFetchingTracker::onWikiArticlesFetchingFinished()
{
    m_stdout << "Finished downloading notes\n";
    Q_EMIT finished();
}

void WikiArticlesFetchingTracker::onWikiArticlesFetchingFailed(
    ErrorString errorDescription)
{
    m_stderr << "Failed to download notes: "
        << errorDescription.nonLocalizedString() << "\n";
    Q_EMIT failure(errorDescription);
}

void WikiArticlesFetchingTracker::onWikiArticlesFetchingProgressUpdate(
    double percentage)
{
    percentage *= 10000.0;
    percentage = std::floor(percentage + 0.5);
    percentage *= 0.01;

    m_stdout << "Downloading notes: " << percentage << "\n";
}

} // namespace quentier
