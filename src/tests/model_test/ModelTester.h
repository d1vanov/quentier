/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_SRC_TESTS_MODEL_TEST_MODEL_TESTER_H
#define QUENTIER_SRC_TESTS_MODEL_TEST_MODEL_TESTER_H

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <QObject>

class ModelTester: public QObject
{
    Q_OBJECT
public:
    ModelTester(QObject * parent = Q_NULLPTR);
    virtual ~ModelTester();

private Q_SLOTS:
    void testSavedSearchModel();
    void testTagModel();
    void testNotebookModel();
    void testNoteModel();
    void testFavoritesModel();
    void testTagModelItemSerialization();

private:
    quentier::LocalStorageManagerAsync *    m_pLocalStorageManagerAsync;
};

#endif // QUENTIER_SRC_TESTS_MODEL_TEST_MODEL_TESTER_H
