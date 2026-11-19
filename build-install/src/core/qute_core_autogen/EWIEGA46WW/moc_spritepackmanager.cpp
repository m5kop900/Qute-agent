/****************************************************************************
** Meta object code from reading C++ file 'spritepackmanager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/core/spritepackmanager.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'spritepackmanager.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN17SpritePackManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto SpritePackManager::qt_create_metaobjectdata<qt_meta_tag_ZN17SpritePackManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SpritePackManager",
        "loaded",
        "",
        "packsChanged",
        "rescan",
        "load",
        "packId",
        "variant",
        "variantsOf",
        "stateInfo",
        "QVariantMap",
        "stateName",
        "packsFolder",
        "openPacksFolder",
        "validatePack",
        "packIdOrPath",
        "validateUserPacks",
        "QVariantList",
        "frameWidth",
        "frameHeight",
        "packName",
        "revision",
        "availablePacks"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'loaded'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'packsChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'rescan'
        QtMocHelpers::MethodData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'load'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 6 }, { QMetaType::QString, 7 },
        }}),
        // Method 'variantsOf'
        QtMocHelpers::MethodData<QStringList(const QString &) const>(8, 2, QMC::AccessPublic, QMetaType::QStringList, {{
            { QMetaType::QString, 6 },
        }}),
        // Method 'stateInfo'
        QtMocHelpers::MethodData<QVariantMap(const QString &) const>(9, 2, QMC::AccessPublic, 0x80000000 | 10, {{
            { QMetaType::QString, 11 },
        }}),
        // Method 'packsFolder'
        QtMocHelpers::MethodData<QString() const>(12, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'openPacksFolder'
        QtMocHelpers::MethodData<bool()>(13, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'validatePack'
        QtMocHelpers::MethodData<QVariantMap(const QString &) const>(14, 2, QMC::AccessPublic, 0x80000000 | 10, {{
            { QMetaType::QString, 15 },
        }}),
        // Method 'validateUserPacks'
        QtMocHelpers::MethodData<QVariantList() const>(16, 2, QMC::AccessPublic, 0x80000000 | 17),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'frameWidth'
        QtMocHelpers::PropertyData<int>(18, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'frameHeight'
        QtMocHelpers::PropertyData<int>(19, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'packName'
        QtMocHelpers::PropertyData<QString>(20, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'revision'
        QtMocHelpers::PropertyData<int>(21, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'availablePacks'
        QtMocHelpers::PropertyData<QVariantList>(22, 0x80000000 | 17, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 1),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SpritePackManager, qt_meta_tag_ZN17SpritePackManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SpritePackManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17SpritePackManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17SpritePackManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN17SpritePackManagerE_t>.metaTypes,
    nullptr
} };

void SpritePackManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SpritePackManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->loaded(); break;
        case 1: _t->packsChanged(); break;
        case 2: _t->rescan(); break;
        case 3: { bool _r = _t->load((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 4: { QStringList _r = _t->variantsOf((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 5: { QVariantMap _r = _t->stateInfo((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 6: { QString _r = _t->packsFolder();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->openPacksFolder();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { QVariantMap _r = _t->validatePack((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 9: { QVariantList _r = _t->validateUserPacks();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SpritePackManager::*)()>(_a, &SpritePackManager::loaded, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SpritePackManager::*)()>(_a, &SpritePackManager::packsChanged, 1))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<int*>(_v) = _t->frameWidth(); break;
        case 1: *reinterpret_cast<int*>(_v) = _t->frameHeight(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->packName(); break;
        case 3: *reinterpret_cast<int*>(_v) = _t->revision(); break;
        case 4: *reinterpret_cast<QVariantList*>(_v) = _t->availablePacks(); break;
        default: break;
        }
    }
}

const QMetaObject *SpritePackManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SpritePackManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17SpritePackManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SpritePackManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SpritePackManager::loaded()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SpritePackManager::packsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
