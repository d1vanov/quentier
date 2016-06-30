/**************************************************************************
**
** This file is part of QMime
**
** Based on Qt Creator source code
**
** Qt Creator Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
**************************************************************************/

#ifndef QMIME_GLOBAL_H
#define QMIME_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QMIME_LIBRARY)
#  define QMIME_EXPORT Q_DECL_EXPORT
#else
#  define QMIME_EXPORT Q_DECL_IMPORT
#endif

#endif // QMIME_GLOBAL_H
