// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Frame {
    id: control
    objectName: "frame-incomplete"

    contentItem: Item {
        objectName: "frame-contentItem-incomplete"
    }

    background: Item {
        objectName: "frame-background-incomplete"
    }
}