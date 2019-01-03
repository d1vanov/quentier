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

#include <QObject>
#include <quentier/types/ErrorString.h>

namespace quentier {

/**
 * This function works around one of Qt's rather obscure limitations: when you
 * want to change the thread affinity of a QObject (i.e. when you want its slots
 * to be invoked in a particular thread), you can only change it from "current
 * thread" to "some other thread" but not otherwise. I.e. you can "push" the
 * object to some thread but you cannot "pull" it from some other thread
 * into the current thread (or some yet another thread). The workaround is to call
 * QObject::moveToThread from the thread which is currently assigned to the object.
 * This is what this function does using an intermediate QObject waiting in a blocking fashion
 * for the thread affinity change to happen before returning.
 *
 * @param object                The object which needs to be moved from its
 *                              assigned thread to the target thread; the
 *                              object's thread must exist and be running for
 *                              the function to succeed
 * @param targetThread          The thread to which the object needs to be moved
 * @param errorDescription      Textual description of the error if the object
 *                              could not be moved to the target thread
 * @return                      True if the object was successfully moved to
 *                              another thread, false otherwise
 */
bool moveObjectToThread(QObject & object, QThread & targetThread, ErrorString & errorDescription);

} // namespace quentier
