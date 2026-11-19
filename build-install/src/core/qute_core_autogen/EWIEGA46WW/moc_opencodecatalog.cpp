/****************************************************************************
** Meta object code from reading C++ file 'opencodecatalog.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/core/opencodecatalog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'opencodecatalog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN15OpenCodeCatalogE_t {};
} // unnamed namespace

template <> constexpr inline auto OpenCodeCatalog::qt_create_metaobjectdata<qt_meta_tag_ZN15OpenCodeCatalogE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "OpenCodeCatalog",
        "commandsChanged",
        "",
        "modelsChanged",
        "agentsChanged",
        "filesChanged",
        "defaultModelChanged",
        "onEvent",
        "type",
        "QJsonObject",
        "properties",
        "refresh",
        "searchFiles",
        "query",
        "modelByRef",
        "QVariantMap",
        "providerID",
        "modelID",
        "agentById",
        "id",
        "commands",
        "QVariantList",
        "slashItems",
        "models",
        "agents",
        "primaryAgents",
        "files",
        "defaultModel"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'commandsChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'modelsChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'agentsChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'filesChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'defaultModelChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onEvent'
        QtMocHelpers::SlotData<void(const QString &, const QJsonObject &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 }, { 0x80000000 | 9, 10 },
        }}),
        // Method 'refresh'
        QtMocHelpers::MethodData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'searchFiles'
        QtMocHelpers::MethodData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Method 'modelByRef'
        QtMocHelpers::MethodData<QVariantMap(const QString &, const QString &) const>(14, 2, QMC::AccessPublic, 0x80000000 | 15, {{
            { QMetaType::QString, 16 }, { QMetaType::QString, 17 },
        }}),
        // Method 'agentById'
        QtMocHelpers::MethodData<QVariantMap(const QString &) const>(18, 2, QMC::AccessPublic, 0x80000000 | 15, {{
            { QMetaType::QString, 19 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'commands'
        QtMocHelpers::PropertyData<QVariantList>(20, 0x80000000 | 21, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'slashItems'
        QtMocHelpers::PropertyData<QVariantList>(22, 0x80000000 | 21, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'models'
        QtMocHelpers::PropertyData<QVariantList>(23, 0x80000000 | 21, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 1),
        // property 'agents'
        QtMocHelpers::PropertyData<QVariantList>(24, 0x80000000 | 21, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 2),
        // property 'primaryAgents'
        QtMocHelpers::PropertyData<QVariantList>(25, 0x80000000 | 21, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 2),
        // property 'files'
        QtMocHelpers::PropertyData<QVariantList>(26, 0x80000000 | 21, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 3),
        // property 'defaultModel'
        QtMocHelpers::PropertyData<QVariantMap>(27, 0x80000000 | 15, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 4),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<OpenCodeCatalog, qt_meta_tag_ZN15OpenCodeCatalogE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject OpenCodeCatalog::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15OpenCodeCatalogE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15OpenCodeCatalogE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15OpenCodeCatalogE_t>.metaTypes,
    nullptr
} };

void OpenCodeCatalog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<OpenCodeCatalog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->commandsChanged(); break;
        case 1: _t->modelsChanged(); break;
        case 2: _t->agentsChanged(); break;
        case 3: _t->filesChanged(); break;
        case 4: _t->defaultModelChanged(); break;
        case 5: _t->onEvent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 6: _t->refresh(); break;
        case 7: _t->searchFiles((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: { QVariantMap _r = _t->modelByRef((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 9: { QVariantMap _r = _t->agentById((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (OpenCodeCatalog::*)()>(_a, &OpenCodeCatalog::commandsChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeCatalog::*)()>(_a, &OpenCodeCatalog::modelsChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeCatalog::*)()>(_a, &OpenCodeCatalog::agentsChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeCatalog::*)()>(_a, &OpenCodeCatalog::filesChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (OpenCodeCatalog::*)()>(_a, &OpenCodeCatalog::defaultModelChanged, 4))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QVariantList*>(_v) = _t->commands(); break;
        case 1: *reinterpret_cast<QVariantList*>(_v) = _t->slashItems(); break;
        case 2: *reinterpret_cast<QVariantList*>(_v) = _t->models(); break;
        case 3: *reinterpret_cast<QVariantList*>(_v) = _t->agents(); break;
        case 4: *reinterpret_cast<QVariantList*>(_v) = _t->primaryAgents(); break;
        case 5: *reinterpret_cast<QVariantList*>(_v) = _t->files(); break;
        case 6: *reinterpret_cast<QVariantMap*>(_v) = _t->defaultModel(); break;
        default: break;
        }
    }
}

const QMetaObject *OpenCodeCatalog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OpenCodeCatalog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15OpenCodeCatalogE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int OpenCodeCatalog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void OpenCodeCatalog::commandsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void OpenCodeCatalog::modelsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void OpenCodeCatalog::agentsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void OpenCodeCatalog::filesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void OpenCodeCatalog::defaultModelChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
