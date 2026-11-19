/****************************************************************************
** Meta object code from reading C++ file 'theme.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/core/theme.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'theme.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN5ThemeE_t {};
} // unnamed namespace

template <> constexpr inline auto Theme::qt_create_metaobjectdata<qt_meta_tag_ZN5ThemeE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Theme",
        "changed",
        "",
        "dark",
        "accent",
        "QColor",
        "accentAlt",
        "panel",
        "input",
        "text",
        "textDim",
        "border",
        "windowColor",
        "baseColor",
        "alternateBase",
        "buttonColor",
        "buttonText",
        "highlightedText",
        "danger",
        "warning",
        "success",
        "codeBackground",
        "syntaxString",
        "syntaxNumber",
        "family",
        "codeFamily"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'changed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'dark'
        QtMocHelpers::PropertyData<bool>(3, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'accent'
        QtMocHelpers::PropertyData<QColor>(4, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'accentAlt'
        QtMocHelpers::PropertyData<QColor>(6, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'panel'
        QtMocHelpers::PropertyData<QColor>(7, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'input'
        QtMocHelpers::PropertyData<QColor>(8, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'text'
        QtMocHelpers::PropertyData<QColor>(9, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'textDim'
        QtMocHelpers::PropertyData<QColor>(10, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'border'
        QtMocHelpers::PropertyData<QColor>(11, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'windowColor'
        QtMocHelpers::PropertyData<QColor>(12, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'baseColor'
        QtMocHelpers::PropertyData<QColor>(13, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'alternateBase'
        QtMocHelpers::PropertyData<QColor>(14, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'buttonColor'
        QtMocHelpers::PropertyData<QColor>(15, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'buttonText'
        QtMocHelpers::PropertyData<QColor>(16, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'highlightedText'
        QtMocHelpers::PropertyData<QColor>(17, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'danger'
        QtMocHelpers::PropertyData<QColor>(18, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'warning'
        QtMocHelpers::PropertyData<QColor>(19, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'success'
        QtMocHelpers::PropertyData<QColor>(20, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'codeBackground'
        QtMocHelpers::PropertyData<QColor>(21, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'syntaxString'
        QtMocHelpers::PropertyData<QColor>(22, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'syntaxNumber'
        QtMocHelpers::PropertyData<QColor>(23, 0x80000000 | 5, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'family'
        QtMocHelpers::PropertyData<QString>(24, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'codeFamily'
        QtMocHelpers::PropertyData<QString>(25, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Theme, qt_meta_tag_ZN5ThemeE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Theme::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN5ThemeE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN5ThemeE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN5ThemeE_t>.metaTypes,
    nullptr
} };

void Theme::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Theme *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->changed(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Theme::*)()>(_a, &Theme::changed, 0))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->dark(); break;
        case 1: *reinterpret_cast<QColor*>(_v) = _t->accent(); break;
        case 2: *reinterpret_cast<QColor*>(_v) = _t->accentAlt(); break;
        case 3: *reinterpret_cast<QColor*>(_v) = _t->panel(); break;
        case 4: *reinterpret_cast<QColor*>(_v) = _t->input(); break;
        case 5: *reinterpret_cast<QColor*>(_v) = _t->text(); break;
        case 6: *reinterpret_cast<QColor*>(_v) = _t->textDim(); break;
        case 7: *reinterpret_cast<QColor*>(_v) = _t->border(); break;
        case 8: *reinterpret_cast<QColor*>(_v) = _t->windowColor(); break;
        case 9: *reinterpret_cast<QColor*>(_v) = _t->baseColor(); break;
        case 10: *reinterpret_cast<QColor*>(_v) = _t->alternateBase(); break;
        case 11: *reinterpret_cast<QColor*>(_v) = _t->buttonColor(); break;
        case 12: *reinterpret_cast<QColor*>(_v) = _t->buttonText(); break;
        case 13: *reinterpret_cast<QColor*>(_v) = _t->highlightedText(); break;
        case 14: *reinterpret_cast<QColor*>(_v) = _t->danger(); break;
        case 15: *reinterpret_cast<QColor*>(_v) = _t->warning(); break;
        case 16: *reinterpret_cast<QColor*>(_v) = _t->success(); break;
        case 17: *reinterpret_cast<QColor*>(_v) = _t->codeBackground(); break;
        case 18: *reinterpret_cast<QColor*>(_v) = _t->syntaxString(); break;
        case 19: *reinterpret_cast<QColor*>(_v) = _t->syntaxNumber(); break;
        case 20: *reinterpret_cast<QString*>(_v) = _t->family(); break;
        case 21: *reinterpret_cast<QString*>(_v) = _t->codeFamily(); break;
        default: break;
        }
    }
}

const QMetaObject *Theme::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Theme::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN5ThemeE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Theme::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    }
    return _id;
}

// SIGNAL 0
void Theme::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
