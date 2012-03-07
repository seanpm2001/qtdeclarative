/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmldebugserver_p.h"
#include "qqmldebugservice_p.h"
#include "qqmldebugservice_p_p.h"
#include <private/qqmlengine_p.h>
#include <private/qqmlglobal_p.h>

#include <QtCore/QDir>
#include <QtCore/QPluginLoader>
#include <QtCore/QStringList>
#include <QtCore/qwaitcondition.h>

#include <private/qobject_p.h>
#include <private/qcoreapplication_p.h>

QT_BEGIN_NAMESPACE

/*
  QQmlDebug Protocol (Version 1):

  handshake:
    1. Client sends
         "QDeclarativeDebugServer" 0 version pluginNames
       version: an int representing the highest protocol version the client knows
       pluginNames: plugins available on client side
    2. Server sends
         "QQmlDebugClient" 0 version pluginNames pluginVersions
       version: an int representing the highest protocol version the client & server know
       pluginNames: plugins available on server side. plugins both in the client and server message are enabled.
  client plugin advertisement
    1. Client sends
         "QDeclarativeDebugServer" 1 pluginNames
  server plugin advertisement
    1. Server sends
         "QQmlDebugClient" 1 pluginNames pluginVersions
  plugin communication:
       Everything send with a header different to "QDeclarativeDebugServer" is sent to the appropriate plugin.
  */

const int protocolVersion = 1;

// print detailed information about loading of plugins
DEFINE_BOOL_CONFIG_OPTION(qmlDebugVerbose, QML_DEBUGGER_VERBOSE)

class QQmlDebugServerThread;

class QQmlDebugServerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlDebugServer)
public:
    QQmlDebugServerPrivate();

    void advertisePlugins();
    QQmlDebugServerConnection *loadConnectionPlugin(const QString &pluginName);

    QQmlDebugServerConnection *connection;
    QHash<QString, QQmlDebugService *> plugins;
    mutable QReadWriteLock pluginsLock;
    QStringList clientPlugins;
    bool gotHello;

    QMutex messageArrivedMutex;
    QWaitCondition messageArrivedCondition;
    QStringList waitingForMessageNames;
    QQmlDebugServerThread *thread;
    QPluginLoader loader;

private:
    // private slot
    void _q_sendMessages(const QList<QByteArray> &messages);
};

class QQmlDebugServerThread : public QThread
{
public:
    void setPluginName(const QString &pluginName) {
        m_pluginName = pluginName;
    }

    void setPort(int port, bool block) {
        m_port = port;
        m_block = block;
    }

    void run();

private:
    QString m_pluginName;
    int m_port;
    bool m_block;
};

QQmlDebugServerPrivate::QQmlDebugServerPrivate() :
    connection(0),
    gotHello(false),
    thread(0)
{
    // used in _q_sendMessages
    qRegisterMetaType<QList<QByteArray> >("QList<QByteArray>");
}

void QQmlDebugServerPrivate::advertisePlugins()
{
    Q_Q(QQmlDebugServer);

    if (!gotHello)
        return;

    QByteArray message;
    {
        QDataStream out(&message, QIODevice::WriteOnly);
        QStringList pluginNames;
        QList<float> pluginVersions;
        foreach (QQmlDebugService *service, plugins.values()) {
            pluginNames << service->name();
            pluginVersions << service->version();
        }
        out << QString(QLatin1String("QDeclarativeDebugClient")) << 1 << pluginNames << pluginVersions;
    }

    QMetaObject::invokeMethod(q, "_q_sendMessages", Qt::QueuedConnection, Q_ARG(QList<QByteArray>, QList<QByteArray>() << message));
}

QQmlDebugServerConnection *QQmlDebugServerPrivate::loadConnectionPlugin(
        const QString &pluginName)
{
#ifndef QT_NO_LIBRARY
    QStringList pluginCandidates;
    const QStringList paths = QCoreApplication::libraryPaths();
    foreach (const QString &libPath, paths) {
        const QDir dir(libPath + QLatin1String("/qmltooling"));
        if (dir.exists()) {
            QStringList plugins(dir.entryList(QDir::Files));
            foreach (const QString &pluginPath, plugins) {
                if (QFileInfo(pluginPath).fileName().contains(pluginName))
                    pluginCandidates << dir.absoluteFilePath(pluginPath);
            }
        }
    }

    foreach (const QString &pluginPath, pluginCandidates) {
        if (qmlDebugVerbose())
            qDebug() << "QQmlDebugServer: Trying to load plugin " << pluginPath << "...";

        loader.setFileName(pluginPath);
        if (!loader.load()) {
            if (qmlDebugVerbose())
                qDebug() << "QQmlDebugServer: Error while loading: " << loader.errorString();
            continue;
        }
        if (QObject *instance = loader.instance())
            connection = qobject_cast<QQmlDebugServerConnection*>(instance);

        if (connection) {
            if (qmlDebugVerbose())
                qDebug() << "QQmlDebugServer: Plugin successfully loaded.";

            return connection;
        }

        if (qmlDebugVerbose())
            qDebug() << "QQmlDebugServer: Plugin does not implement interface QQmlDebugServerConnection.";

        loader.unload();
    }
#endif
    return 0;
}

void QQmlDebugServerThread::run()
{
    QQmlDebugServer *server = QQmlDebugServer::instance();
    QQmlDebugServerConnection *connection
            = server->d_func()->loadConnectionPlugin(m_pluginName);
    if (connection) {
        connection->setServer(QQmlDebugServer::instance());
        connection->setPort(m_port, m_block);
    } else {
        QCoreApplicationPrivate *appD = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));
        qWarning() << QString::fromAscii("QQmlDebugServer: Ignoring \"-qmljsdebugger=%1\". "
                                         "Remote debugger plugin has not been found.").arg(appD->qmljsDebugArgumentsString());
    }

    exec();

    // make sure events still waiting are processed
    QEventLoop eventLoop;
    eventLoop.processEvents(QEventLoop::AllEvents);
}

bool QQmlDebugServer::hasDebuggingClient() const
{
    Q_D(const QQmlDebugServer);
    return d->connection
            && d->connection->isConnected()
            && d->gotHello;
}

static QQmlDebugServer *qQmlDebugServer = 0;


static void cleanup()
{
    delete qQmlDebugServer;
    qQmlDebugServer = 0;
}

QQmlDebugServer *QQmlDebugServer::instance()
{
    static bool commandLineTested = false;

    if (!commandLineTested) {
        commandLineTested = true;

        QCoreApplicationPrivate *appD = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));
#ifndef QQML_NO_DEBUG_PROTOCOL
        // ### remove port definition when protocol is changed
        int port = 0;
        bool block = false;
        bool ok = false;

        // format: qmljsdebugger=port:3768[,block] OR qmljsdebugger=ost[,block]
        if (!appD->qmljsDebugArgumentsString().isEmpty()) {
            if (!QQmlEnginePrivate::qml_debugging_enabled) {
                qWarning() << QString::fromLatin1(
                                  "QQmlDebugServer: Ignoring \"-qmljsdebugger=%1\". "
                                  "Debugging has not been enabled.").arg(
                                  appD->qmljsDebugArgumentsString());
                return 0;
            }

            QString pluginName;
            if (appD->qmljsDebugArgumentsString().indexOf(QLatin1String("port:")) == 0) {
                int separatorIndex = appD->qmljsDebugArgumentsString().indexOf(QLatin1Char(','));
                port = appD->qmljsDebugArgumentsString().mid(5, separatorIndex - 5).toInt(&ok);
                pluginName = QLatin1String("qmldbg_tcp");
            } else if (appD->qmljsDebugArgumentsString().contains(QLatin1String("ost"))) {
                pluginName = QLatin1String("qmldbg_ost");
                ok = true;
            }

            block = appD->qmljsDebugArgumentsString().contains(QLatin1String("block"));

            if (ok) {
                qQmlDebugServer = new QQmlDebugServer();
                QQmlDebugServerThread *thread = new QQmlDebugServerThread;
                qQmlDebugServer->d_func()->thread = thread;
                qQmlDebugServer->moveToThread(thread);
                thread->setPluginName(pluginName);
                thread->setPort(port, block);
                thread->start();

                if (block) {
                    QQmlDebugServerPrivate *d = qQmlDebugServer->d_func();
                    d->messageArrivedMutex.lock();
                    d->messageArrivedCondition.wait(&d->messageArrivedMutex);
                    d->messageArrivedMutex.unlock();
                }

            } else {
                qWarning() << QString::fromLatin1(
                                  "QQmlDebugServer: Ignoring \"-qmljsdebugger=%1\". "
                                  "Format is -qmljsdebugger=port:<port>[,block]").arg(
                                  appD->qmljsDebugArgumentsString());
            }
        }
#else
        if (!appD->qmljsDebugArgumentsString().isEmpty()) {
            qWarning() << QString::fromLatin1(
                         "QQmlDebugServer: Ignoring \"-qmljsdebugger=%1\". "
                         "QtQml is not configured for debugging.").arg(
                         appD->qmljsDebugArgumentsString());
        }
#endif
    }

    return qQmlDebugServer;
}

QQmlDebugServer::QQmlDebugServer()
    : QObject(*(new QQmlDebugServerPrivate))
{
    qAddPostRoutine(cleanup);
}

QQmlDebugServer::~QQmlDebugServer()
{
    Q_D(QQmlDebugServer);

    QReadLocker(&d->pluginsLock);
    {
        foreach (QQmlDebugService *service, d->plugins.values()) {
            service->stateAboutToBeChanged(QQmlDebugService::NotConnected);
            service->d_func()->server = 0;
            service->d_func()->state = QQmlDebugService::NotConnected;
            service->stateChanged(QQmlDebugService::NotConnected);
        }
    }

    if (d->thread) {
        d->thread->exit();
        d->thread->wait();
        delete d->thread;
    }
    delete d->connection;
}

void QQmlDebugServer::receiveMessage(const QByteArray &message)
{
    Q_D(QQmlDebugServer);

    QDataStream in(message);

    QString name;

    in >> name;
    if (name == QLatin1String("QDeclarativeDebugServer")) {
        int op = -1;
        in >> op;
        if (op == 0) {
            int version;
            in >> version >> d->clientPlugins;

            // Send the hello answer immediately, since it needs to arrive before
            // the plugins below start sending messages.
            QByteArray helloAnswer;
            {
                QDataStream out(&helloAnswer, QIODevice::WriteOnly);
                QStringList pluginNames;
                QList<float> pluginVersions;
                foreach (QQmlDebugService *service, d->plugins.values()) {
                    pluginNames << service->name();
                    pluginVersions << service->version();
                }

                out << QString(QLatin1String("QDeclarativeDebugClient")) << 0 << protocolVersion << pluginNames << pluginVersions;
            }
            d->connection->send(QList<QByteArray>() << helloAnswer);

            d->gotHello = true;

            QReadLocker(&d->pluginsLock);
            QHash<QString, QQmlDebugService*>::ConstIterator iter = d->plugins.constBegin();
            for (; iter != d->plugins.constEnd(); ++iter) {
                QQmlDebugService::State newState = QQmlDebugService::Unavailable;
                if (d->clientPlugins.contains(iter.key()))
                    newState = QQmlDebugService::Enabled;
                iter.value()->d_func()->state = newState;
                iter.value()->stateChanged(newState);
            }

            qWarning("QQmlDebugServer: Connection established");
            d->messageArrivedCondition.wakeAll();

        } else if (op == 1) {

            // Service Discovery
            QStringList oldClientPlugins = d->clientPlugins;
            in >> d->clientPlugins;

            QReadLocker(&d->pluginsLock);
            QHash<QString, QQmlDebugService*>::ConstIterator iter = d->plugins.constBegin();
            for (; iter != d->plugins.constEnd(); ++iter) {
                const QString pluginName = iter.key();
                QQmlDebugService::State newState = QQmlDebugService::Unavailable;
                if (d->clientPlugins.contains(pluginName))
                    newState = QQmlDebugService::Enabled;

                if (oldClientPlugins.contains(pluginName)
                        != d->clientPlugins.contains(pluginName)) {
                    iter.value()->d_func()->state = newState;
                    iter.value()->stateChanged(newState);
                }
            }

        } else {
            qWarning("QQmlDebugServer: Invalid control message %d", op);
            d->connection->disconnect();
            return;
        }

    } else {
        if (d->gotHello) {
            QByteArray message;
            in >> message;

            QReadLocker(&d->pluginsLock);
            QHash<QString, QQmlDebugService *>::Iterator iter = d->plugins.find(name);
            if (iter == d->plugins.end()) {
                qWarning() << "QQmlDebugServer: Message received for missing plugin" << name;
            } else {
                (*iter)->messageReceived(message);

                if (d->waitingForMessageNames.removeOne(name))
                    d->messageArrivedCondition.wakeAll();
            }
        } else {
            qWarning("QQmlDebugServer: Invalid hello message");
        }

    }
}

void QQmlDebugServerPrivate::_q_sendMessages(const QList<QByteArray> &messages)
{
    if (connection)
        connection->send(messages);
}

QList<QQmlDebugService*> QQmlDebugServer::services() const
{
    const Q_D(QQmlDebugServer);
    QReadLocker(&d->pluginsLock);
    return d->plugins.values();
}

QStringList QQmlDebugServer::serviceNames() const
{
    const Q_D(QQmlDebugServer);
    QReadLocker(&d->pluginsLock);
    return d->plugins.keys();
}

bool QQmlDebugServer::addService(QQmlDebugService *service)
{
    Q_D(QQmlDebugServer);
    {
        QWriteLocker(&d->pluginsLock);
        if (!service || d->plugins.contains(service->name()))
            return false;
        d->plugins.insert(service->name(), service);
    }
    {
        QReadLocker(&d->pluginsLock);
        d->advertisePlugins();
        QQmlDebugService::State newState = QQmlDebugService::Unavailable;
        if (d->clientPlugins.contains(service->name()))
            newState = QQmlDebugService::Enabled;
        service->d_func()->state = newState;
    }
    return true;
}

bool QQmlDebugServer::removeService(QQmlDebugService *service)
{
    Q_D(QQmlDebugServer);
    {
        QWriteLocker(&d->pluginsLock);
        if (!service || !d->plugins.contains(service->name()))
            return false;
        d->plugins.remove(service->name());
    }
    {
        QReadLocker(&d->pluginsLock);
        QQmlDebugService::State newState = QQmlDebugService::NotConnected;
        service->stateAboutToBeChanged(newState);
        d->advertisePlugins();
        service->d_func()->server = 0;
        service->d_func()->state = newState;
        service->stateChanged(newState);
    }

    return true;
}

void QQmlDebugServer::sendMessages(QQmlDebugService *service,
                                          const QList<QByteArray> &messages)
{
    QList<QByteArray> prefixedMessages;
    foreach (const QByteArray &message, messages) {
        QByteArray prefixed;
        QDataStream out(&prefixed, QIODevice::WriteOnly);
        out << service->name() << message;
        prefixedMessages << prefixed;
    }

    QMetaObject::invokeMethod(this, "_q_sendMessages", Qt::QueuedConnection, Q_ARG(QList<QByteArray>, prefixedMessages));
}

bool QQmlDebugServer::waitForMessage(QQmlDebugService *service)
{
    Q_D(QQmlDebugServer);
    QReadLocker(&d->pluginsLock);

    if (!service
            || !d->plugins.contains(service->name()))
        return false;

    d->messageArrivedMutex.lock();
    d->waitingForMessageNames << service->name();
    do {
        d->messageArrivedCondition.wait(&d->messageArrivedMutex);
    } while (d->waitingForMessageNames.contains(service->name()));
    d->messageArrivedMutex.unlock();
    return true;
}

QT_END_NAMESPACE

#include "moc_qqmldebugserver_p.cpp"
