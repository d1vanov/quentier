/*
 * Copyright 2018 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_EXCEPTION_LOCAL_STORAGE_VERSION_TOO_HIGH_EXCEPTION_H
#define QUENTIER_LIB_EXCEPTION_LOCAL_STORAGE_VERSION_TOO_HIGH_EXCEPTION_H

#include <quentier/exception/IQuentierException.h>

namespace quentier {

class LocalStorageVersionTooHighException: public IQuentierException
{
public:
    explicit LocalStorageVersionTooHighException(const ErrorString & message);

protected:
    virtual const QString exceptionDisplayName() const override;
};

} // namespace quentier

#endif // QUENTIER_LIB_EXCEPTION_LOCAL_STORAGE_VERSION_TOO_HIGH_EXCEPTION_H
