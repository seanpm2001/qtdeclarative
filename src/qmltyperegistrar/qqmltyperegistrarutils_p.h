// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLTYPEREGISTRAR_UTILS_P_H
#define QQMLTYPEREGISTRAR_UTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

QTypeRevision handleInMinorVersion(QTypeRevision revision, int majorVersion);

QT_END_NAMESPACE

#endif // QQMLTYPEREGISTRAR_UTILS_P_H