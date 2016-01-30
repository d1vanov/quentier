// Copyright (c) 2015, Axel Gembe <axel@gembe.net>
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or other
//   materials provided with the distribution.
// * The name of the contributors may not be used to endorse or promote products
//   derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef QSLOGDESTMODEL_H
#define QSLOGDESTMODEL_H

#include "QsLogDest.h"

#include <QAbstractTableModel>
#include <QReadWriteLock>

#include <limits>
#include <deque>

namespace QsLogging
{

class QSLOG_SHARED_OBJECT ModelDestination : public QAbstractTableModel, public Destination
{
    Q_OBJECT
public:
    static const char* const Type;

    enum Column
    {
        TimeColumn = 0,
        LevelNameColumn = 1,
        MessageColumn = 2,
        FormattedMessageColumn = 100
    };

    explicit ModelDestination(size_t max_items = std::numeric_limits<size_t>::max());
    virtual ~ModelDestination();

    void addEntry(const LogMessage& message);
    void clear();
    LogMessage at(size_t index);

    // Destination overrides
    virtual void write(const LogMessage& message);
    virtual bool isValid();
    virtual QString type() const;

    // QAbstractTableModel overrides
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    std::deque<LogMessage> mLogMessages;
    mutable QReadWriteLock mMessagesLock;
    size_t mMaxItems;
};

}

#endif // QSLOGDESTMODEL_H
