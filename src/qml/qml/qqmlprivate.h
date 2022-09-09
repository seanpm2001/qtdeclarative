/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLPRIVATE_H
#define QQMLPRIVATE_H

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

#include <functional>
#include <type_traits>

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqmlpropertyvaluesource.h>

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/qurl.h>
#include <QtCore/qpointer.h>

#include <QtCore/qmetaobject.h>
#include <QtCore/qdebug.h>

#define QML_GETTYPENAMES \
    const char *className = T::staticMetaObject.className(); \
    const int nameLen = int(strlen(className)); \
    QVarLengthArray<char,48> pointerName(nameLen+2); \
    memcpy(pointerName.data(), className, size_t(nameLen)); \
    pointerName[nameLen] = '*'; \
    pointerName[nameLen+1] = '\0'; \
    const int listLen = int(strlen("QQmlListProperty<")); \
    QVarLengthArray<char,64> listName(listLen + nameLen + 2); \
    memcpy(listName.data(), "QQmlListProperty<", size_t(listLen)); \
    memcpy(listName.data()+listLen, className, size_t(nameLen)); \
    listName[listLen+nameLen] = '>'; \
    listName[listLen+nameLen+1] = '\0';

QT_BEGIN_NAMESPACE

class QQmlPropertyValueInterceptor;

namespace QQmlPrivate {
struct CachedQmlUnit;
template<typename A>
using QQmlAttachedPropertiesFunc = A *(*)(QObject *);
}

namespace QV4 {
struct ExecutionEngine;
namespace CompiledData {
struct Unit;
struct CompilationUnit;
}
}
namespace QmlIR {
struct Document;
typedef void (*IRLoaderFunction)(Document *, const QQmlPrivate::CachedQmlUnit *);
}

using QQmlAttachedPropertiesFunc = QQmlPrivate::QQmlAttachedPropertiesFunc<QObject>;

inline uint qHash(QQmlAttachedPropertiesFunc func, uint seed = 0)
{
    return qHash(quintptr(func), seed);
}

template <typename TYPE>
class QQmlTypeInfo
{
public:
    enum {
        hasAttachedProperties = 0
    };
};


class QJSValue;
class QJSEngine;
class QQmlEngine;
class QQmlCustomParser;
class QQmlTypeNotAvailable;

template<class T>
QQmlCustomParser *qmlCreateCustomParser()
{
    return nullptr;
}

namespace QQmlPrivate
{
    void Q_QML_EXPORT qdeclarativeelement_destructor(QObject *);
    template<typename T>
    class QQmlElement final : public T
    {
    public:
        ~QQmlElement() override {
            QQmlPrivate::qdeclarativeelement_destructor(this);
        }
        static void operator delete(void *ptr) {
            // We allocate memory from this class in QQmlType::create
            // along with some additional memory.
            // So we override the operator delete in order to avoid the
            // sized operator delete to be called with a different size than
            // the size that was allocated.
            ::operator delete (ptr);
        }
        static void operator delete(void *, void *) {
            // Deliberately empty placement delete operator.
            // Silences MSVC warning C4291: no matching operator delete found
        }
    };

    template<typename T>
    constexpr bool isConstructible()
    {
        return std::is_default_constructible<T>::value && std::is_base_of<QObject, T>::value;
    }

    template<typename T>
    void createInto(void *memory) { new (memory) QQmlElement<T>; }

    template<typename T>
    QObject *createSingletonInstance(QQmlEngine *, QJSEngine *) { return new T; }

    template<typename T>
    QObject *createParent(QObject *p) { return new T(p); }

    using CreateIntoFunction = void (*)(void *);
    using CreateSingletonFunction = QObject *(*)(QQmlEngine *, QJSEngine *);
    using CreateParentFunction = QObject *(*)(QObject *);

    template<typename T, bool Constructible = isConstructible<T>()>
    struct Constructors;

    template<typename T>
    struct Constructors<T, true>
    {
        static constexpr CreateIntoFunction createInto
                = QQmlPrivate::createInto<T>;
        static constexpr CreateSingletonFunction createSingletonInstance
                = QQmlPrivate::createSingletonInstance<T>;
    };

    // from https://en.cppreference.com/w/cpp/language/static:
    // If a const non-inline (since C++17) static data member or a constexpr
    // static data member (since C++11)(until C++17) is odr-used, a definition
    // at namespace scope is still required, but it cannot have an initializer.
    //
    // If a static data member is declared constexpr, it is implicitly inline
    // and does not need to be redeclared at namespace scope. This redeclaration
    // without an initializer (formerly required as shown above) is still
    // permitted, but is deprecated.
    //
    // TL;DR: redundant definitions for static constexpr are required in c++11
    // but deprecated in c++17.
    template<typename T>
    constexpr CreateIntoFunction Constructors<T, true>::createInto;
    template<typename T>
    constexpr CreateSingletonFunction Constructors<T, true>::createSingletonInstance;

    template<typename T>
    struct Constructors<T, false>
    {
        static constexpr CreateIntoFunction createInto = nullptr;
        static constexpr CreateSingletonFunction createSingletonInstance = nullptr;
    };

    // see comment above over the Constructors<T, true> definitions.
    template<typename T>
    constexpr CreateIntoFunction Constructors<T, false>::createInto;
    template<typename T>
    constexpr CreateSingletonFunction Constructors<T, false>::createSingletonInstance;

    template<typename T, bool IsVoid = std::is_void<T>::value>
    struct ExtendedType;

    // void means "not an extended type"
    template<typename T>
    struct ExtendedType<T, true>
    {
        static constexpr const CreateParentFunction createParent = nullptr;
        static constexpr const QMetaObject *staticMetaObject = nullptr;
    };

    // see comment above over the Constructors<T, true> definitions.
    template<typename T>
    constexpr const CreateParentFunction ExtendedType<T, true>::createParent;
    template<typename T>
    constexpr const QMetaObject* ExtendedType<T, true>::staticMetaObject;

    // If it's not void, we actually want an error if the ctor or the metaobject is missing.
    template<typename T>
    struct ExtendedType<T, false>
    {
        static constexpr const CreateParentFunction createParent = QQmlPrivate::createParent<T>;
        static constexpr const QMetaObject *staticMetaObject = &T::staticMetaObject;
    };

    // see comment above over the Constructors<T, true> definitions.
    template<typename T>
    constexpr const CreateParentFunction ExtendedType<T, false>::createParent;
    template<typename T>
    constexpr const QMetaObject* ExtendedType<T, false>::staticMetaObject;

    template<class From, class To, int N>
    struct StaticCastSelectorClass
    {
        static inline int cast() { return -1; }
    };

    template<class From, class To>
    struct StaticCastSelectorClass<From, To, sizeof(int)>
    {
        static inline int cast() { return int(reinterpret_cast<quintptr>(static_cast<To *>(reinterpret_cast<From *>(0x10000000)))) - 0x10000000; }
    };

    template<class From, class To>
    struct StaticCastSelector
    {
        typedef int yes_type;
        typedef char no_type;

        static yes_type checkType(To *);
        static no_type checkType(...);

        static inline int cast()
        {
            return StaticCastSelectorClass<From, To, sizeof(checkType(reinterpret_cast<From *>(0)))>::cast();
        }
    };

    template<typename...>
    using QmlVoidT = void;

    // You can prevent subclasses from using the same attached type by specialzing this.
    // This is reserved for internal types, though.
    template<class T, class A>
    struct OverridableAttachedType
    {
        using Type = A;
    };

    template<class T, class = QmlVoidT<>, bool OldStyle = QQmlTypeInfo<T>::hasAttachedProperties>
    struct QmlAttached
    {
        using Type = void;
        using Func = QQmlAttachedPropertiesFunc<QObject>;
        static const QMetaObject *staticMetaObject() { return nullptr; }
        static Func attachedPropertiesFunc() { return nullptr; }
    };

    // Defined inline via QML_ATTACHED
    template<class T>
    struct QmlAttached<T, QmlVoidT<typename OverridableAttachedType<T, typename T::QmlAttachedType>::Type>, false>
    {
        // Normal attached properties
        template <typename Parent, typename Attached>
        struct Properties
        {
            using Func = QQmlAttachedPropertiesFunc<Attached>;
            static const QMetaObject *staticMetaObject() { return &Attached::staticMetaObject; }
            static Func attachedPropertiesFunc() { return Parent::qmlAttachedProperties; }
        };

        // Disabled via OverridableAttachedType
        template<typename Parent>
        struct Properties<Parent, void>
        {
            using Func = QQmlAttachedPropertiesFunc<QObject>;
            static const QMetaObject *staticMetaObject() { return nullptr; };
            static Func attachedPropertiesFunc() { return nullptr; };
        };

        using Type = typename OverridableAttachedType<T, typename T::QmlAttachedType>::Type;
        using Func = typename Properties<T, Type>::Func;

        static const QMetaObject *staticMetaObject()
        {
            return Properties<T, Type>::staticMetaObject();
        }

        static Func attachedPropertiesFunc()
        {
            return Properties<T, Type>::attachedPropertiesFunc();
        }
    };

    // Separately defined via QQmlTypeInfo
    template<class T>
    struct QmlAttached<T, QmlVoidT<decltype(T::qmlAttachedProperties)>, true>
    {
        using Type = typename std::remove_pointer<decltype(T::qmlAttachedProperties(nullptr))>::type;
        using Func = QQmlAttachedPropertiesFunc<Type>;

        static const QMetaObject *staticMetaObject() { return &Type::staticMetaObject; }
        static Func attachedPropertiesFunc() { return T::qmlAttachedProperties; }
    };

    // This is necessary because both the type containing a default template parameter and the type
    // instantiating the template need to have access to the default template parameter type. In
    // this case that's T::QmlAttachedType. The QML_FOREIGN macro needs to befriend specific other
    // types. Therefore we need some kind of "accessor". Because of compiler bugs in gcc and clang,
    // we cannot befriend attachedPropertiesFunc() directly. Wrapping the actual access into another
    // struct "fixes" that. For convenience we still want the free standing functions in addition.
    template<class T>
    struct QmlAttachedAccessor
    {
        static QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunc()
        {
            return QQmlAttachedPropertiesFunc<QObject>(QmlAttached<T>::attachedPropertiesFunc());
        }

        static const QMetaObject *staticMetaObject()
        {
            return QmlAttached<T>::staticMetaObject();
        }
    };

    template<typename T>
    inline QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunc()
    {
        return QmlAttachedAccessor<T>::attachedPropertiesFunc();
    }

    template<typename T>
    inline const QMetaObject *attachedPropertiesMetaObject()
    {
        return QmlAttachedAccessor<T>::staticMetaObject();
    }

    enum AutoParentResult { Parented, IncompatibleObject, IncompatibleParent };
    typedef AutoParentResult (*AutoParentFunction)(QObject *object, QObject *parent);

    struct RegisterType {
        int version;

        int typeId;
        int listId;
        int objectSize;
        void (*create)(void *);
        QString noCreationReason;

        const char *uri;
        int versionMajor;
        int versionMinor;
        const char *elementName;
        const QMetaObject *metaObject;

        QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunction;
        const QMetaObject *attachedPropertiesMetaObject;

        int parserStatusCast;
        int valueSourceCast;
        int valueInterceptorCast;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QQmlCustomParser *customParser;

        int revision;
        // If this is extended ensure "version" is bumped!!!
    };

    struct RegisterTypeAndRevisions {
        int version;

        int typeId;
        int listId;
        int objectSize;
        void (*create)(void *);

        const char *uri;
        int versionMajor;

        const QMetaObject *metaObject;
        const QMetaObject *classInfoMetaObject;

        QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunction;
        const QMetaObject *attachedPropertiesMetaObject;

        int parserStatusCast;
        int valueSourceCast;
        int valueInterceptorCast;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QQmlCustomParser *(*customParserFactory)();
    };

    struct RegisterInterface {
        int version;

        int typeId;
        int listId;

        const char *iid;

        const char *uri;
        int versionMajor;
    };

    struct RegisterAutoParent {
        int version;

        AutoParentFunction function;
    };

    struct RegisterSingletonType {
        int version;

        const char *uri;
        int versionMajor;
        int versionMinor;
        const char *typeName;

        QJSValue (*scriptApi)(QQmlEngine *, QJSEngine *);
        QObject *(*qobjectApi)(QQmlEngine *, QJSEngine *);
        const QMetaObject *instanceMetaObject; // new in version 1
        int typeId; // new in version 2
        int revision; // new in version 2
        std::function<QObject*(QQmlEngine *, QJSEngine *)> generalizedQobjectApi; // new in version 3
        // If this is extended ensure "version" is bumped!!!
    };

    struct RegisterSingletonTypeAndRevisions {
        int version;
        const char *uri;
        int versionMajor;

        QJSValue (*scriptApi)(QQmlEngine *, QJSEngine *);
        const QMetaObject *instanceMetaObject;
        const QMetaObject *classInfoMetaObject;

        int typeId;
        std::function<QObject*(QQmlEngine *, QJSEngine *)> generalizedQobjectApi; // new in version 3
    };

    struct RegisterCompositeType {
        QUrl url;
        const char *uri;
        int versionMajor;
        int versionMinor;
        const char *typeName;
    };

    struct RegisterCompositeSingletonType {
        QUrl url;
        const char *uri;
        int versionMajor;
        int versionMinor;
        const char *typeName;
    };

    struct CachedQmlUnit {
        const QV4::CompiledData::Unit *qmlData;
        void *unused1;
        void *unused2;
    };

    typedef const CachedQmlUnit *(*QmlUnitCacheLookupFunction)(const QUrl &url);
    struct RegisterQmlUnitCacheHook {
        int version;
        QmlUnitCacheLookupFunction lookupCachedQmlUnit;
    };

    enum RegistrationType {
        TypeRegistration       = 0,
        InterfaceRegistration  = 1,
        AutoParentRegistration = 2,
        SingletonRegistration  = 3,
        CompositeRegistration  = 4,
        CompositeSingletonRegistration = 5,
        QmlUnitCacheHookRegistration = 6,
        TypeAndRevisionsRegistration = 7,
        SingletonAndRevisionsRegistration = 8
    };

    int Q_QML_EXPORT qmlregister(RegistrationType, void *);
    void Q_QML_EXPORT qmlunregister(RegistrationType, quintptr);
    struct Q_QML_EXPORT RegisterSingletonFunctor
    {
        QObject *operator()(QQmlEngine *, QJSEngine *);

        QPointer<QObject> m_object;
        bool alreadyCalled = false;
    };

    static int indexOfOwnClassInfo(const QMetaObject *metaObject, const char *key)
    {
        if (!metaObject || !key)
            return -1;

        const int offset = metaObject->classInfoOffset();
        for (int i = metaObject->classInfoCount() + offset - 1; i >= offset; --i)
            if (qstrcmp(key, metaObject->classInfo(i).name()) == 0) {
                return i;
        }
        return -1;
    }

    inline const char *classInfo(const QMetaObject *metaObject, const char *key)
    {
        return metaObject->classInfo(indexOfOwnClassInfo(metaObject, key)).value();
    }

    inline int intClassInfo(const QMetaObject *metaObject, const char *key, int defaultValue = 0)
    {
        const int index = indexOfOwnClassInfo(metaObject, key);
        return (index == -1) ? defaultValue
                             : QByteArray(metaObject->classInfo(index).value()).toInt();
    }

    inline bool boolClassInfo(const QMetaObject *metaObject, const char *key,
                              bool defaultValue = false)
    {
        const int index = indexOfOwnClassInfo(metaObject, key);
        return (index == -1) ? defaultValue
                             : (QByteArray(metaObject->classInfo(index).value()) == "true");
    }

    inline const char *classElementName(const QMetaObject *metaObject)
    {
        const char *elementName = classInfo(metaObject, "QML.Element");
        if (qstrcmp(elementName, "auto") == 0)
            return metaObject->className();
        if (qstrcmp(elementName, "anonymous") == 0)
            return nullptr;

        if (!elementName || elementName[0] < 'A' || elementName[0] > 'Z') {
            qWarning() << "Missing or unusable QML.Element class info \"" << elementName << "\""
                       << "for" << metaObject->className();
        }

        return elementName;
    }

    template<class T, class = QmlVoidT<>>
    struct QmlExtended
    {
        using Type = void;
    };

    template<class T>
    struct QmlExtended<T, QmlVoidT<typename T::QmlExtendedType>>
    {
        using Type = typename T::QmlExtendedType;
    };

    template<class T, class = QmlVoidT<>>
    struct QmlResolved
    {
        using Type = T;
    };

    template<class T>
    struct QmlResolved<T, QmlVoidT<typename T::QmlForeignType>>
    {
        using Type = typename T::QmlForeignType;
    };

    template<class T, class = QmlVoidT<>>
    struct QmlSingleton
    {
        static constexpr bool Value = false;
    };

    // see comment above over the Constructors<T, true> definitions.
    template<typename T, class C>
    constexpr bool QmlSingleton<T, C>::Value;

    template<class T>
    struct QmlSingleton<T, QmlVoidT<typename T::QmlIsSingleton>>
    {
        static constexpr bool Value = bool(T::QmlIsSingleton::yes);
    };

    template<class T, class = QmlVoidT<>>
    struct QmlInterface
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlInterface<T, QmlVoidT<typename T::QmlIsInterface>>
    {
        static constexpr bool Value = bool(T::QmlIsInterface::yes);
    };

    template<typename T>
    void qmlRegisterSingletonAndRevisions(const char *uri, int versionMajor,
                                          const QMetaObject *classInfoMetaObject)
    {
        QML_GETTYPENAMES

        RegisterSingletonTypeAndRevisions api = {
            0,

            uri,
            versionMajor,

            nullptr,

            &T::staticMetaObject,
            classInfoMetaObject,

            qRegisterNormalizedMetaType<T *>(pointerName.constData()),
            Constructors<T>::createSingletonInstance
        };

        qmlregister(SingletonAndRevisionsRegistration, &api);
    }

    template<typename T, typename E>
    void qmlRegisterTypeAndRevisions(const char *uri, int versionMajor,
                                     const QMetaObject *classInfoMetaObject)
    {
        QML_GETTYPENAMES

        RegisterTypeAndRevisions type = {
            0,
            qRegisterNormalizedMetaType<T *>(pointerName.constData()),
            qRegisterNormalizedMetaType<QQmlListProperty<T> >(listName.constData()),
            int(sizeof(T)),
            Constructors<T>::createInto,

            uri,
            versionMajor,

            &T::staticMetaObject,
            classInfoMetaObject,

            attachedPropertiesFunc<T>(),
            attachedPropertiesMetaObject<T>(),

            StaticCastSelector<T, QQmlParserStatus>::cast(),
            StaticCastSelector<T, QQmlPropertyValueSource>::cast(),
            StaticCastSelector<T, QQmlPropertyValueInterceptor>::cast(),

            ExtendedType<E>::createParent,
            ExtendedType<E>::staticMetaObject,

            &qmlCreateCustomParser<T>
        };

        qmlregister(TypeAndRevisionsRegistration, &type);
    }

    template<>
    void Q_QML_EXPORT qmlRegisterTypeAndRevisions<QQmlTypeNotAvailable, void>(
            const char *uri, int versionMajor, const QMetaObject *classInfoMetaObject);

} // namespace QQmlPrivate

QT_END_NAMESPACE

#endif // QQMLPRIVATE_H
