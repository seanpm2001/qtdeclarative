// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Item {
    id: root

    Keys.onPressed: {}

    MouseArea {
        onClicked: root.Keys.enabled = false
    }
}
