/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtScript module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-ONLY$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QJSCONVERTER_P_H
#define QJSCONVERTER_P_H

#include "qjsvalue.h"
#include <QtCore/qglobal.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qstring.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qregexp.h>
#include <QtCore/qdatetime.h>

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

extern char *qdtoa(double d, int mode, int ndigits, int *decpt, int *sign, char **rve, char **digits_str);
Q_CORE_EXPORT QString qt_regexp_toCanonical(const QString &, QRegExp::PatternSyntax);

/*
  \internal
  \class QJSConverter
  QJSValue and QJSEngine helper class. This class's responsibility is to convert values
  between JS values and Qt/C++ values.

  This is a nice way to inline these functions in both QJSValue and QJSEngine.
*/
class QJSConverter {
public:
    static quint32 toArrayIndex(const QString& string)
    {
        // FIXME this function should be exported by JSC C API.
        bool ok;
        quint32 idx = string.toUInt(&ok);
        if (!ok || toString(idx) != string)
            idx = 0xffffffff;

        return idx;
    }

    static QString toString(v8::Handle<v8::String> jsString)
    {
        if (jsString.IsEmpty())
            return QString();
        QString qstr;
        qstr.resize(jsString->Length());
        jsString->Write(reinterpret_cast<uint16_t*>(qstr.data()));
        return qstr;
    }

    static v8::Handle<v8::String> toString(const QString& string)
    {
        return v8::String::New(reinterpret_cast<const uint16_t*>(string.data()), string.size());
    }

    static QString toString(double value)
    {
        // FIXME this should be easier. The ideal fix is to create
        // a new function in V8 API which could cover the functionality.

        if (qIsNaN(value))
            return QString::fromLatin1("NaN");
        if (qIsInf(value))
            return QString::fromLatin1(value < 0 ? "-Infinity" : "Infinity");
        if (!value)
            return QString::fromLatin1("0");

        QVarLengthArray<char, 25> buf;
        int decpt;
        int sign;
        char* result = 0;
        char* endresult;
        (void)qdtoa(value, 0, 0, &decpt, &sign, &endresult, &result);

        if (!result)
            return QString();

        int resultLen = endresult - result;
        if (decpt <= 0 && decpt > -6) {
            buf.resize(-decpt + 2 + sign);
            qMemSet(buf.data(), '0', -decpt + 2 + sign);
            if (sign) // fix the sign.
                buf[0] = '-';
            buf[sign + 1] = '.';
            buf.append(result, resultLen);
        } else {
            if (sign)
                buf.append('-');
            int length = buf.size() - sign + resultLen;
            if (decpt <= 21 && decpt > 0) {
                if (length <= decpt) {
                    const char* zeros = "0000000000000000000000000";
                    buf.append(result, resultLen);
                    buf.append(zeros, decpt - length);
                } else {
                    buf.append(result, decpt);
                    buf.append('.');
                    buf.append(result + decpt, resultLen - decpt);
                }
            } else if (result[0] >= '0' && result[0] <= '9') {
                if (length > 1) {
                    buf.append(result, 1);
                    buf.append('.');
                    buf.append(result + 1, resultLen - 1);
                } else
                    buf.append(result, resultLen);
                buf.append('e');
                buf.append(decpt >= 0 ? '+' : '-');
                int e = qAbs(decpt - 1);
                if (e >= 100)
                    buf.append('0' + e / 100);
                if (e >= 10)
                    buf.append('0' + (e % 100) / 10);
                buf.append('0' + e % 10);
            }
        }
        free(result);
        buf.append(0);
        return QString::fromLatin1(buf.constData());
    }

    enum {
        PropertyAttributeMask = v8::ReadOnly | v8::DontDelete | v8::DontEnum,
    };

    // return a mask of v8::PropertyAttribute that may also contains QScriptValue::PropertyGetter or QScriptValue::PropertySetter
    static uint toPropertyAttributes(const QFlags<QJSValue::PropertyFlag>& flags)
    {
        uint attr = 0;
        if (flags.testFlag(QJSValue::ReadOnly))
            attr |= v8::ReadOnly;
        if (flags.testFlag(QJSValue::Undeletable))
            attr |= v8::DontDelete;
        if (flags.testFlag(QJSValue::SkipInEnumeration))
            attr |= v8::DontEnum;
//        if (flags.testFlag(QScriptValue::PropertyGetter))
//            attr |= QScriptValue::PropertyGetter;
//        if (flags.testFlag(QScriptValue::PropertySetter))
//            attr |= QScriptValue::PropertySetter;
        return attr;
    }

    // Converts a JS RegExp to a QRegExp.
    // The conversion is not 100% exact since ECMA regexp and QRegExp
    // have different semantics/flags, but we try to do our best.
    static QRegExp toRegExp(v8::Handle<v8::RegExp> jsRegExp)
    {
        QString pattern = QJSConverter::toString(jsRegExp->GetSource());
        Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
        if (jsRegExp->GetFlags() & v8::RegExp::kIgnoreCase)
            caseSensitivity = Qt::CaseInsensitive;
        return QRegExp(pattern, caseSensitivity, QRegExp::RegExp2);
    }

    // Converts a QRegExp to a JS RegExp.
    // The conversion is not 100% exact since ECMA regexp and QRegExp
    // have different semantics/flags, but we try to do our best.
    static v8::Handle<v8::RegExp> toRegExp(const QRegExp &re)
    {
        // Convert the pattern to a ECMAScript pattern.
        QString pattern = qt_regexp_toCanonical(re.pattern(), re.patternSyntax());
        if (re.isMinimal()) {
            QString ecmaPattern;
            int len = pattern.length();
            ecmaPattern.reserve(len);
            int i = 0;
            const QChar *wc = pattern.unicode();
            bool inBracket = false;
            while (i < len) {
                QChar c = wc[i++];
                ecmaPattern += c;
                switch (c.unicode()) {
                case '?':
                case '+':
                case '*':
                case '}':
                    if (!inBracket)
                        ecmaPattern += QLatin1Char('?');
                    break;
                case '\\':
                    if (i < len)
                        ecmaPattern += wc[i++];
                    break;
                case '[':
                    inBracket = true;
                    break;
                case ']':
                    inBracket = false;
                    break;
                default:
                    break;
                }
            }
            pattern = ecmaPattern;
        }

        int flags = v8::RegExp::kNone;
        if (re.caseSensitivity() == Qt::CaseInsensitive)
            flags |= v8::RegExp::kIgnoreCase;

        return v8::RegExp::New(QJSConverter::toString(pattern), static_cast<v8::RegExp::Flags>(flags));
    }

    // Converts a QStringList to JS.
    // The result is a new Array object with length equal to the length
    // of the QStringList, and the elements being the QStringList's
    // elements converted to JS Strings.
    static v8::Handle<v8::Array> toStringList(const QStringList &lst)
    {
        v8::Handle<v8::Array> result = v8::Array::New(lst.size());
        for (int i = 0; i < lst.size(); ++i)
            result->Set(i, toString(lst.at(i)));
        return result;
    }

    // Converts a JS Array object to a QStringList.
    // The result is a QStringList with length equal to the length
    // of the JS Array, and elements being the JS Array's elements
    // converted to QStrings.
    static QStringList toStringList(v8::Handle<v8::Array> jsArray)
    {
        QStringList result;
        uint32_t length = jsArray->Length();
        for (uint32_t i = 0; i < length; ++i)
            result.append(toString(jsArray->Get(i)->ToString()));
        return result;
    }


    // Converts a JS Date to a QDateTime.
    static QDateTime toDateTime(v8::Handle<v8::Date> jsDate)
    {
        return QDateTime::fromMSecsSinceEpoch(jsDate->NumberValue());
    }

    // Converts a QDateTime to a JS Date.
    static v8::Handle<v8::Value> toDateTime(const QDateTime &dt)
    {
        double date;
        if (!dt.isValid())
            date = qSNaN();
        else
            date = dt.toMSecsSinceEpoch();
        return v8::Date::New(date);
    }
};

QT_END_NAMESPACE

#endif
