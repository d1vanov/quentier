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

#ifndef LIB_QUENTIER_ENML_HTML_CLEANER_H
#define LIB_QUENTIER_ENML_HTML_CLEANER_H

#include <quentier/utility/Linkage.h>
#include <quentier/utility/Macros.h>
#include <QString>

namespace quentier {

class QUENTIER_EXPORT HTMLCleaner
{
public:
    HTMLCleaner();
    virtual ~HTMLCleaner();

    bool htmlToXml(const QString & html, QString & output, QString & errorDescription);
    bool htmlToXhtml(const QString & html, QString & output, QString & errorDescription);
    bool cleanupHtml(QString & html, QString & errorDescription);

private:
    Q_DISABLE_COPY(HTMLCleaner)

private:
    class Impl;
    Impl * m_impl;
};

} // namespace quentier

#endif // LIB_QUENTIER_ENML_HTML_CLEANER_H
