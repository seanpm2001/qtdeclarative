/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#include "qquicklabsplatformfontdialog_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype FontDialog
    \inherits Dialog
//!     \instantiates QQuickLabsPlatformFontDialog
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native font dialog.

    The FontDialog type provides a QML API for native platform font dialogs.

    \image qtlabsplatform-fontdialog-gtk.png

    To show a font dialog, construct an instance of FontDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l currentFont
    property can be used to determine the currently selected font in the
    dialog. The \l font property is updated only after the final selection
    has been made by accepting the dialog.

    \code
    MenuItem {
        text: "Font"
        onTriggered: fontDialog.open()
    }

    FontDialog {
        id: fontDialog
        currentFont.family: document.font
    }

    MyDocument {
        id: document
        font: fontDialog.font
    }
    \endcode

    \section2 Availability

    A native platform font dialog is currently available on the following platforms:

    \list
    \li macOS
    \li Linux (when running with the GTK+ platform theme)
    \endlist

    \input includes/widgets.qdocinc 1

    \labs
*/

QQuickLabsPlatformFontDialog::QQuickLabsPlatformFontDialog(QObject *parent)
    : QQuickLabsPlatformDialog(QPlatformTheme::FontDialog, parent),
      m_options(QFontDialogOptions::create())
{
}

/*!
    \qmlproperty font Qt.labs.platform::FontDialog::font

    This property holds the final accepted font.

    Unlike the \l currentFont property, the \c font property is not updated
    while the user is selecting fonts in the dialog, but only after the final
    selection has been made. That is, when the user has clicked \uicontrol OK
    to accept a font. Alternatively, the \l {Dialog::}{accepted()} signal
    can be handled to get the final selection.

    \sa currentFont, {Dialog::}{accepted()}
*/
QFont QQuickLabsPlatformFontDialog::font() const
{
    return m_font;
}

void QQuickLabsPlatformFontDialog::setFont(const QFont &font)
{
    if (m_font == font)
        return;

    m_font = font;
    setCurrentFont(font);
    emit fontChanged();
}

/*!
    \qmlproperty font Qt.labs.platform::FontDialog::currentFont

    This property holds the currently selected font in the dialog.

    Unlike the \l font property, the \c currentFont property is updated
    while the user is selecting fonts in the dialog, even before the final
    selection has been made.

    \sa font
*/
QFont QQuickLabsPlatformFontDialog::currentFont() const
{
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(handle()))
        return fontDialog->currentFont();
    return m_currentFont;
}

void QQuickLabsPlatformFontDialog::setCurrentFont(const QFont &font)
{
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(handle()))
        fontDialog->setCurrentFont(font);
    m_currentFont = font;
}

/*!
    \qmlproperty flags Qt.labs.platform::FontDialog::options

    This property holds the various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the dialog is
    visible is not guaranteed to have an immediate effect on the dialog (depending on
    the option and on the platform).

    Available options:
    \value FontDialog.ScalableFonts Show scalable fonts.
    \value FontDialog.NonScalableFonts Show non-scalable fonts.
    \value FontDialog.MonospacedFonts Show monospaced fonts.
    \value FontDialog.ProportionalFonts Show proportional fonts.
    \value FontDialog.NoButtons Don't display \uicontrol OK and \uicontrol Cancel buttons (useful for "live dialogs").
*/
QFontDialogOptions::FontDialogOptions QQuickLabsPlatformFontDialog::options() const
{
    return m_options->options();
}

void QQuickLabsPlatformFontDialog::setOptions(QFontDialogOptions::FontDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

bool QQuickLabsPlatformFontDialog::useNativeDialog() const
{
    return QQuickLabsPlatformDialog::useNativeDialog()
            && !m_options->testOption(QFontDialogOptions::DontUseNativeDialog);
}

void QQuickLabsPlatformFontDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(dialog)) {
        connect(fontDialog, &QPlatformFontDialogHelper::currentFontChanged, this, &QQuickLabsPlatformFontDialog::currentFontChanged);
        fontDialog->setOptions(m_options);
    }
}

void QQuickLabsPlatformFontDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(dialog))
        fontDialog->setOptions(m_options);
}

void QQuickLabsPlatformFontDialog::accept()
{
    setFont(currentFont());
    QQuickLabsPlatformDialog::accept();
}

QT_END_NAMESPACE
