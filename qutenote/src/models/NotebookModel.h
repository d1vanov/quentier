#ifndef __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_H
#define __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_H

#include "NotebookModelItem.h"
#include <qute_note/types/Notebook.h>
#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>
#include <qute_note/utility/LRUCache.hpp>
#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#endif

#define NOTEBOOK_MODEL_MIME_TYPE "application/x-com.qute_note.notebookmodeldatalist"
#define NOTEBOOK_MODEL_DATA_MAX_COMPRESSION (9)

namespace qute_note {

class NotebookModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit NotebookModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                           QObject * parent = Q_NULLPTR);
    virtual ~NotebookModel();

    struct Columns
    {
        enum type {
            Name = 0,
            Synchronizable,
            Dirty,
            Default,
            Published,
            FromLinkedNotebook
        };
    };

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex parent(const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;
    virtual bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTEBOOK_MODEL_H
