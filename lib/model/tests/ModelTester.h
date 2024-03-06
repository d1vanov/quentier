/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/local_storage/Fwd.h>

#include <QObject>
#include <QTemporaryDir>

class ModelTester : public QObject
{
    Q_OBJECT
public:
    ModelTester(QObject * parent = nullptr);

    ~ModelTester() override;

private Q_SLOTS:
    void testSavedSearchModel();
    void testTagModel();
    void testNotebookModel();
    void testNoteModel();
    /*
    void testFavoritesModel();
    void testTagModelItemSerialization();
    */

private:
    QTemporaryDir m_tempDir;
    quentier::local_storage::ILocalStoragePtr m_localStorage;
};
