/****************************************************************************
** Meta object code from reading C++ file 'opencodeclient.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/core/opencodeclient.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'opencodeclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14OpenCodeClientE_t {};
} // unnamed namespace

template <> constexpr inline auto OpenCodeClient::qt_create_metaobjectdata<qt_meta_tag_ZN14OpenCodeClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "OpenCodeClient",
        "sessionsListed",
        "",
        "QVariantList",
        "sessions",
        "requestFailed",
        "message",
        "connectedChanged",
        "connected",
        "responseTextChanged",
        "sessionChanged",
        "lastErrorChanged",
        "opencodeMissingChanged",
        "promptSent",
        "text",
        "attachmentNames",
        "serverEvent",
        "type",
        "QJsonObject",
        "properties",
        "messageHistoryReceived",
        "QJsonArray",
        "messages",
        "historyFailed",
        "sessionsListFailed",
        "jsonFetched",
        "path",
        "QJsonDocument",
        "document",
        "status",
        "isV2Server",
        "start",
        "sendPrompt",
        "sendPromptWithFiles",
        "files",
        "agents",
        "abort",
        "answerPermission",
        "permissionId",
        "response",
        "cancelForm",
        "formId",
        "selectAgent",
        "agent",
        "selectModel",
        "providerID",
        "modelID",
        "variant",
        "newSession",
        "listSessions",
        "switchSession",
        "id",
        "runServerCommand",
        "name",
        "arguments",
        "compactSession",
        "requestMessageHistory",
        "limit",
        "responseText",
        "sessionId",
        "lastError",
        "opencodeMissing",
        "apiName"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'sessionsListed'
        QtMocHelpers::SignalData<void(const QVariantList &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'requestFailed'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'connectedChanged'
        QtMocHelpers::SignalData<void(bool)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 8 },
        }}),
        // Signal 'responseTextChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'sessionChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'lastErrorChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'opencodeMissingChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'promptSent'
        QtMocHelpers::SignalData<void(const QString &, const QStringList &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 }, { QMetaType::QStringList, 15 },
        }}),
        // Signal 'serverEvent'
        QtMocHelpers::SignalData<void(const QString &, const QJsonObject &)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 17 }, { 0x80000000 | 18, 19 },
        }}),
        // Signal 'messageHistoryReceived'
        QtMocHelpers::SignalData<void(const QJsonArray &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Signal 'historyFailed'
        QtMocHelpers::SignalData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'sessionsListFailed'
        QtMocHelpers::SignalData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'jsonFetched'
        QtMocHelpers::SignalData<void(const QString &, const QJsonDocument &, int)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 26 }, { 0x80000000 | 27, 28 }, { QMetaType::Int, 29 },
        }}),
        // Method 'isV2Server'
        QtMocHelpers::MethodData<bool() const>(30, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'start'
        QtMocHelpers::MethodData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'sendPrompt'
        QtMocHelpers::MethodData<void(const QString &)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 },
        }}),
        // Method 'sendPromptWithFiles'
        QtMocHelpers::MethodData<bool(const QString &, const QVariantList &, const QVariantList &)>(33, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 14 }, { 0x80000000 | 3, 34 }, { 0x80000000 | 3, 35 },
        }}),
        // Method 'abort'
        QtMocHelpers::MethodData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'answerPermission'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 38 }, { QMetaType::QString, 39 },
        }}),
        // Method 'cancelForm'
        QtMocHelpers::MethodData<void(const QString &)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 41 },
        }}),
        // Method 'selectAgent'
        QtMocHelpers::MethodData<void(const QString &)>(42, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 43 },
        }}),
        // Method 'selectModel'
        QtMocHelpers::MethodData<void(const QString &, const QString &, const QString &)>(44, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 45 }, { QMetaType::QString, 46 }, { QMetaType::QString, 47 },
        }}),
        // Method 'newSession'
        QtMocHelpers::MethodData<void()>(48, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'listSessions'
        QtMocHelpers::MethodData<void()>(49, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'switchSession'
        QtMocHelpers::MethodData<void(const QString &)>(50, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 51 },
        }}),
        // Method 'runServerCommand'
        QtMocHelpers::MethodData<void(const QString &, const QString &, const QVariantList &, const QVariantList &)>(52, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 53 }, { QMetaType::QString, 54 }, { 0x80000000 | 3, 34 }, { 0x80000000 | 3, 35 },
        }}),
        // Method 'compactSession'
        QtMocHelpers::MethodData<void()>(55, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'requestMessageHistory'
        QtMocHelpers::MethodData<void(int)>(56, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 57 },
        }}),
        // Method 'requestMessageHistory'
        QtMocHelpers::MethodData<void()>(56, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'connected'
        QtMocHelpers::PropertyData<bool>(8, QMetaType::Bool, QMC::DefaultPropertyFlags, 2),
        // property 'responseText'
        QtMocHelpers::PropertyData<QString>(58, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'sessionId'
        QtMocHelpers::PropertyData<QString>(59, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'lastError'
        QtMocHelpers::PropertyData<QString>(60, QMetaType::QString, QMC::DefaultPropertyFlags, 5),
        // property 'opencodeMissing'
        QtMocHelpers::PropertyData<bool>(61, QMetaType::Bool, QMC::DefaultPropertyFlags, 6),
        // property 'apiName'
        QtMocHelpers::PropertyData<QString>(62, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<OpenCodeClient, qt_meta_tag_ZN14OpenCodeClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject OpenCodeClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14OpenCodeClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14OpenCodeClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14OpenCodeClientE_t>.metaTypes,
    nullptr
} };

void OpenCodeClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<OpenCodeClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->sessionsListed((*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[1]))); break;
        case 1: _t->requestFailed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->connectedChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->responseTextChanged(); break;
        case 4: _t->sessionChanged(); break;
        case 5: _t->lastErrorChanged(); break;
        case 6: _t->opencodeMissingChanged(); break;
        case 7: _t->promptSent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2]))); break;
        case 8: _t->serverEvent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 9: _t->messageHistoryReceived((*reinterpret_cast<std::add_pointer_t<QJsonArray>>(_a[1]))); break;
        case 10: _t->historyFailed(); break;
        case 11: _t->sessionsListFailed(); break;
        case 12: _t->jsonFetched((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonDocument>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 13: { bool _r = _t->isV2Server();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 14: _t->start(); break;
        case 15: _t->sendPrompt((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: { bool _r = _t->sendPromptWithFiles((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[3])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 17: _t->abort(); break;
        case 18: _t->answerPermission((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 19: _t->cancelForm((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->selectAgent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 21: _t->selectModel((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 22: _t->newSession(); break;
        case 23: _t->listSessions(); break;
        case 24: _t->switchSession((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 25: _t->runServerCommand((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[4]))); break;
        case 26: _t->compactSession(); break;
        case 27: _t->requestMessageHistory((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 28: _t->requestMessageHistory(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)(const QVariantList & )>(_a, &OpenCodeClient::sessionsListed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)(const QString & )>(_a, &OpenCodeClient::requestFailed, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)(bool )>(_a, &OpenCodeClient::connectedChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)()>(_a, &OpenCodeClient::responseTextChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)()>(_a, &OpenCodeClient::sessionChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)()>(_a, &OpenCodeClient::lastErrorChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)()>(_a, &OpenCodeClient::opencodeMissingChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)(const QString & , const QStringList & )>(_a, &OpenCodeClient::promptSent, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)(const QString & , const QJsonObject & )>(_a, &OpenCodeClient::serverEvent, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)(const QJsonArray & )>(_a, &OpenCodeClient::messageHistoryReceived, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)()>(_a, &OpenCodeClient::historyFailed, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)()>(_a, &OpenCodeClient::sessionsListFailed, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeClient::*)(const QString & , const QJsonDocument & , int )>(_a, &OpenCodeClient::jsonFetched, 12))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->connected(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->responseText(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->sessionId(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->lastError(); break;
        case 4: *reinterpret_cast<bool*>(_v) = _t->opencodeMissing(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->apiName(); break;
        default: break;
        }
    }
}

const QMetaObject *OpenCodeClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OpenCodeClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14OpenCodeClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int OpenCodeClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 29)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 29;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void OpenCodeClient::sessionsListed(const QVariantList & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void OpenCodeClient::requestFailed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void OpenCodeClient::connectedChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void OpenCodeClient::responseTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void OpenCodeClient::sessionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void OpenCodeClient::lastErrorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void OpenCodeClient::opencodeMissingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void OpenCodeClient::promptSent(const QString & _t1, const QStringList & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2);
}

// SIGNAL 8
void OpenCodeClient::serverEvent(const QString & _t1, const QJsonObject & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2);
}

// SIGNAL 9
void OpenCodeClient::messageHistoryReceived(const QJsonArray & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void OpenCodeClient::historyFailed()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void OpenCodeClient::sessionsListFailed()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void OpenCodeClient::jsonFetched(const QString & _t1, const QJsonDocument & _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
