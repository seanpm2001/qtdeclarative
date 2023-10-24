// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Switch {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 6
    spacing: 6

    readonly property bool __notCustomizable: true
    readonly property Item __focusFrameTarget: indicator
    readonly property Item __focusFrameStyleItem: indicator

    indicator: Rectangle {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
                        : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: 40
        implicitHeight: 16
        radius: 3
        color: Qt.darker(control.palette.button, control.down ? 1.2 : 1.1)
        border.color: Qt.darker(control.palette.window, 1.4)

        readonly property bool __ignoreNotCustomizable: true
        readonly property real __focusFrameRadius: 2

        // Checked indicator.
        Rectangle {
            x: control.mirrored ? parent.children[1].x : 0
            width: control.mirrored ? parent.width - parent.children[1].x : parent.children[1].x + parent.children[1].width
            height: parent.height
            radius: 3
            color: Qt.darker(control.palette.highlight, control.down ? 1.1 : 1)
            border.color: Qt.darker(control.palette.highlight, 1.35)
            border.width: control.enabled ? 1 : 0
            opacity: control.checked ? 1 : 0

            Behavior on opacity {
                enabled: !control.down
                NumberAnimation { duration: 80 }
            }
        }

        // Handle.
        Rectangle {
            x: Math.max(0, Math.min(parent.width - width, control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2
            width: 20
            height: 16
            radius: 3
            color: Qt.lighter(control.palette.button, control.down ? 1 : (control.hovered ? 1.07 : 1.045))
            border.width: 1
            border.color: Qt.darker(control.palette.window, 1.4)

            Behavior on x {
                enabled: !control.down
                SmoothedAnimation { velocity: 200 }
            }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0
        text: control.text
        font: control.font
        color: control.palette.windowText
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter

        readonly property bool __ignoreNotCustomizable: true
    }
}
