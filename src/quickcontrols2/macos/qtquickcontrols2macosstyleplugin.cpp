/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml/qqml.h>
#include <QtQuickControls2/private/qquickstyleplugin_p.h>
#include <QtQuickControls2/qquickstyle.h>

extern void qml_register_types_QtQuick_Controls_macOS();
Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Controls_macOS);

QT_BEGIN_NAMESPACE

class QtQuickControls2MacOSStylePlugin : public QQuickStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickControls2MacOSStylePlugin(QObject *parent = nullptr);
    QString name() const override;
    void initializeTheme(QQuickTheme *theme) override;
};


QtQuickControls2MacOSStylePlugin::QtQuickControls2MacOSStylePlugin(QObject *parent):
    QQuickStylePlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Controls_macOS;
    Q_UNUSED(registration);
}

QString QtQuickControls2MacOSStylePlugin::name() const
{
    return QStringLiteral("macOS");
}

void QtQuickControls2MacOSStylePlugin::initializeTheme(QQuickTheme */*theme*/)
{
}

QT_END_NAMESPACE

#include "qtquickcontrols2macosstyleplugin.moc"
