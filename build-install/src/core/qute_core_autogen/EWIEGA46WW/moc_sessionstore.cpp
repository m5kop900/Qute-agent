/****************************************************************************
** Meta object code from reading C++ file 'sessionstore.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/core/sessionstore.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sessionstore.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12SessionStoreE_t {};
} // unnamed namespace

template <> constexpr inline auto SessionStore::qt_create_metaobjectdata<qt_meta_tag_ZN12SessionStoreE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SessionStore",
        "busyChanged",
        "",
        "loadingHistoryChanged",
        "sessionChanged",
        "workDirChanged",
        "sessionDirectoryChanged",
        "activeAgentChanged",
        "activeModelChanged",
        "usageChanged",
        "contextChanged",
        "onEvent",
        "type",
        "QJsonObject",
        "properties",
        "onMessages",
        "QJsonArray",
        "messagesAscending",
        "onPromptSent",
        "text",
        "attachmentNames",
        "onConnectedChanged",
        "connected",
        "selectAgent",
        "agent",
        "cycleAgent",
        "selectModel",
        "providerID",
        "modelID",
        "variant",
        "requestCompaction",
        "conversation",
        "ConversationModel*",
        "busy",
        "loadingHistory",
        "sessionId",
        "workDir",
        "sessionDirectory",
        "activeAgent",
        "activeModel",
        "activeModelProvider",
        "activeModelId",
        "activeVariant",
        "usage",
        "QVariantMap",
        "contextTokens",
        "contextLimit",
        "contextPercent"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'busyChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'loadingHistoryChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'sessionChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'workDirChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'sessionDirectoryChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'activeAgentChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'activeModelChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'usageChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'contextChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onEvent'
        QtMocHelpers::SlotData<void(const QString &, const QJsonObject &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 }, { 0x80000000 | 13, 14 },
        }}),
        // Slot 'onMessages'
        QtMocHelpers::SlotData<void(const QJsonArray &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
        // Slot 'onPromptSent'
        QtMocHelpers::SlotData<void(const QString &, const QStringList &)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 19 }, { QMetaType::QStringList, 20 },
        }}),
        // Slot 'onPromptSent'
        QtMocHelpers::SlotData<void(const QString &)>(18, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::QString, 19 },
        }}),
        // Slot 'onConnectedChanged'
        QtMocHelpers::SlotData<void(bool)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 22 },
        }}),
        // Slot 'selectAgent'
        QtMocHelpers::SlotData<void(const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 24 },
        }}),
        // Slot 'cycleAgent'
        QtMocHelpers::SlotData<QString()>(25, 2, QMC::AccessPublic, QMetaType::QString),
        // Slot 'selectModel'
        QtMocHelpers::SlotData<void(const QString &, const QString &, const QString &)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 27 }, { QMetaType::QString, 28 }, { QMetaType::QString, 29 },
        }}),
        // Slot 'requestCompaction'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'conversation'
        QtMocHelpers::PropertyData<ConversationModel*>(31, 0x80000000 | 32, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'busy'
        QtMocHelpers::PropertyData<bool>(33, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'loadingHistory'
        QtMocHelpers::PropertyData<bool>(34, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'sessionId'
        QtMocHelpers::PropertyData<QString>(35, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'workDir'
        QtMocHelpers::PropertyData<QString>(36, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'sessionDirectory'
        QtMocHelpers::PropertyData<QString>(37, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'activeAgent'
        QtMocHelpers::PropertyData<QString>(38, QMetaType::QString, QMC::DefaultPropertyFlags, 5),
        // property 'activeModel'
        QtMocHelpers::PropertyData<QString>(39, QMetaType::QString, QMC::DefaultPropertyFlags, 6),
        // property 'activeModelProvider'
        QtMocHelpers::PropertyData<QString>(40, QMetaType::QString, QMC::DefaultPropertyFlags, 6),
        // property 'activeModelId'
        QtMocHelpers::PropertyData<QString>(41, QMetaType::QString, QMC::DefaultPropertyFlags, 6),
        // property 'activeVariant'
        QtMocHelpers::PropertyData<QString>(42, QMetaType::QString, QMC::DefaultPropertyFlags, 6),
        // property 'usage'
        QtMocHelpers::PropertyData<QVariantMap>(43, 0x80000000 | 44, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 7),
        // property 'contextTokens'
        QtMocHelpers::PropertyData<qint64>(45, QMetaType::LongLong, QMC::DefaultPropertyFlags, 8),
        // property 'contextLimit'
        QtMocHelpers::PropertyData<qint64>(46, QMetaType::LongLong, QMC::DefaultPropertyFlags, 8),
        // property 'contextPercent'
        QtMocHelpers::PropertyData<int>(47, QMetaType::Int, QMC::DefaultPropertyFlags, 8),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SessionStore, qt_meta_tag_ZN12SessionStoreE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SessionStore::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SessionStoreE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SessionStoreE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12SessionStoreE_t>.metaTypes,
    nullptr
} };

void SessionStore::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SessionStore *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->busyChanged(); break;
        case 1: _t->loadingHistoryChanged(); break;
        case 2: _t->sessionChanged(); break;
        case 3: _t->workDirChanged(); break;
        case 4: _t->sessionDirectoryChanged(); break;
        case 5: _t->activeAgentChanged(); break;
        case 6: _t->activeModelChanged(); break;
        case 7: _t->usageChanged(); break;
        case 8: _t->contextChanged(); break;
        case 9: _t->onEvent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 10: _t->onMessages((*reinterpret_cast<std::add_pointer_t<QJsonArray>>(_a[1]))); break;
        case 11: _t->onPromptSent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2]))); break;
        case 12: _t->onPromptSent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->onConnectedChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 14: _t->selectAgent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: { QString _r = _t->cycleAgent();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 16: _t->selectModel((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 17: _t->requestCompaction(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::busyChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::loadingHistoryChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::sessionChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::workDirChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::sessionDirectoryChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::activeAgentChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::activeModelChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::usageChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (SessionStore::*)()>(_a, &SessionStore::contextChanged, 8))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<ConversationModel**>(_v) = _t->conversation(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->busy(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->loadingHistory(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->sessionId(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->workDir(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->sessionDirectory(); break;
        case 6: *reinterpret_cast<QString*>(_v) = _t->activeAgent(); break;
        case 7: *reinterpret_cast<QString*>(_v) = _t->activeModel(); break;
        case 8: *reinterpret_cast<QString*>(_v) = _t->activeModelProvider(); break;
        case 9: *reinterpret_cast<QString*>(_v) = _t->activeModelId(); break;
        case 10: *reinterpret_cast<QString*>(_v) = _t->activeVariant(); break;
        case 11: *reinterpret_cast<QVariantMap*>(_v) = _t->usage(); break;
        case 12: *reinterpret_cast<qint64*>(_v) = _t->contextTokens(); break;
        case 13: *reinterpret_cast<qint64*>(_v) = _t->contextLimit(); break;
        case 14: *reinterpret_cast<int*>(_v) = _t->contextPercent(); break;
        default: break;
        }
    }
}

const QMetaObject *SessionStore::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SessionStore::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SessionStoreE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SessionStore::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 18;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void SessionStore::busyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SessionStore::loadingHistoryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SessionStore::sessionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SessionStore::workDirChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SessionStore::sessionDirectoryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SessionStore::activeAgentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SessionStore::activeModelChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SessionStore::usageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void SessionStore::contextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}
QT_WARNING_POP
