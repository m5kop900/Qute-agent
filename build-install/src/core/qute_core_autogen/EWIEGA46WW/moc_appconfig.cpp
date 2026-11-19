/****************************************************************************
** Meta object code from reading C++ file 'appconfig.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/core/appconfig.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'appconfig.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9AppConfigE_t {};
} // unnamed namespace

template <> constexpr inline auto AppConfig::qt_create_metaobjectdata<qt_meta_tag_ZN9AppConfigE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AppConfig",
        "changed",
        "",
        "load",
        "save",
        "configVersion",
        "configPath",
        "serverUrl",
        "serverUsername",
        "serverPassword",
        "autoStartServer",
        "serverCommand",
        "workDir",
        "modelProvider",
        "modelId",
        "apiFlavor",
        "lastSessionId",
        "packId",
        "variant",
        "themeMode",
        "accentColor",
        "scale",
        "overlayOpacity",
        "cornerRadius",
        "density",
        "spacingScale",
        "radiusScale",
        "animations",
        "fontFamily",
        "codeFontFamily",
        "controlStyle",
        "placeAtCursor",
        "hideOnFocusLoss",
        "maxAttachmentSizeMB",
        "autoCompactEnabled",
        "autoCompactThreshold"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'changed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'load'
        QtMocHelpers::MethodData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'save'
        QtMocHelpers::MethodData<bool()>(4, 2, QMC::AccessPublic, QMetaType::Bool),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'configVersion'
        QtMocHelpers::PropertyData<int>(5, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'configPath'
        QtMocHelpers::PropertyData<QString>(6, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'serverUrl'
        QtMocHelpers::PropertyData<QString>(7, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'serverUsername'
        QtMocHelpers::PropertyData<QString>(8, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'serverPassword'
        QtMocHelpers::PropertyData<QString>(9, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'autoStartServer'
        QtMocHelpers::PropertyData<bool>(10, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'serverCommand'
        QtMocHelpers::PropertyData<QString>(11, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'workDir'
        QtMocHelpers::PropertyData<QString>(12, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'modelProvider'
        QtMocHelpers::PropertyData<QString>(13, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'modelId'
        QtMocHelpers::PropertyData<QString>(14, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'apiFlavor'
        QtMocHelpers::PropertyData<QString>(15, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'lastSessionId'
        QtMocHelpers::PropertyData<QString>(16, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'packId'
        QtMocHelpers::PropertyData<QString>(17, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'variant'
        QtMocHelpers::PropertyData<QString>(18, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'themeMode'
        QtMocHelpers::PropertyData<QString>(19, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'accentColor'
        QtMocHelpers::PropertyData<QString>(20, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'scale'
        QtMocHelpers::PropertyData<double>(21, QMetaType::Double, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'overlayOpacity'
        QtMocHelpers::PropertyData<double>(22, QMetaType::Double, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'cornerRadius'
        QtMocHelpers::PropertyData<int>(23, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'density'
        QtMocHelpers::PropertyData<QString>(24, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'spacingScale'
        QtMocHelpers::PropertyData<double>(25, QMetaType::Double, QMC::DefaultPropertyFlags, 0),
        // property 'radiusScale'
        QtMocHelpers::PropertyData<double>(26, QMetaType::Double, QMC::DefaultPropertyFlags, 0),
        // property 'animations'
        QtMocHelpers::PropertyData<bool>(27, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'fontFamily'
        QtMocHelpers::PropertyData<QString>(28, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'codeFontFamily'
        QtMocHelpers::PropertyData<QString>(29, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'controlStyle'
        QtMocHelpers::PropertyData<QString>(30, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'placeAtCursor'
        QtMocHelpers::PropertyData<bool>(31, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'hideOnFocusLoss'
        QtMocHelpers::PropertyData<bool>(32, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'maxAttachmentSizeMB'
        QtMocHelpers::PropertyData<int>(33, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'autoCompactEnabled'
        QtMocHelpers::PropertyData<bool>(34, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'autoCompactThreshold'
        QtMocHelpers::PropertyData<int>(35, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AppConfig, qt_meta_tag_ZN9AppConfigE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AppConfig::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9AppConfigE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9AppConfigE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9AppConfigE_t>.metaTypes,
    nullptr
} };

void AppConfig::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AppConfig *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->changed(); break;
        case 1: _t->load(); break;
        case 2: { bool _r = _t->save();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AppConfig::*)()>(_a, &AppConfig::changed, 0))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<int*>(_v) = _t->configVersion(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->configPath(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->serverUrl(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->serverUsername(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->serverPassword(); break;
        case 5: *reinterpret_cast<bool*>(_v) = _t->autoStartServer(); break;
        case 6: *reinterpret_cast<QString*>(_v) = _t->serverCommand(); break;
        case 7: *reinterpret_cast<QString*>(_v) = _t->workDir(); break;
        case 8: *reinterpret_cast<QString*>(_v) = _t->modelProvider(); break;
        case 9: *reinterpret_cast<QString*>(_v) = _t->modelId(); break;
        case 10: *reinterpret_cast<QString*>(_v) = _t->apiFlavor(); break;
        case 11: *reinterpret_cast<QString*>(_v) = _t->lastSessionId(); break;
        case 12: *reinterpret_cast<QString*>(_v) = _t->packId(); break;
        case 13: *reinterpret_cast<QString*>(_v) = _t->variant(); break;
        case 14: *reinterpret_cast<QString*>(_v) = _t->themeMode(); break;
        case 15: *reinterpret_cast<QString*>(_v) = _t->accentColor(); break;
        case 16: *reinterpret_cast<double*>(_v) = _t->scale(); break;
        case 17: *reinterpret_cast<double*>(_v) = _t->overlayOpacity(); break;
        case 18: *reinterpret_cast<int*>(_v) = _t->cornerRadius(); break;
        case 19: *reinterpret_cast<QString*>(_v) = _t->density(); break;
        case 20: *reinterpret_cast<double*>(_v) = _t->spacingScale(); break;
        case 21: *reinterpret_cast<double*>(_v) = _t->radiusScale(); break;
        case 22: *reinterpret_cast<bool*>(_v) = _t->animations(); break;
        case 23: *reinterpret_cast<QString*>(_v) = _t->fontFamily(); break;
        case 24: *reinterpret_cast<QString*>(_v) = _t->codeFontFamily(); break;
        case 25: *reinterpret_cast<QString*>(_v) = _t->controlStyle(); break;
        case 26: *reinterpret_cast<bool*>(_v) = _t->placeAtCursor(); break;
        case 27: *reinterpret_cast<bool*>(_v) = _t->hideOnFocusLoss(); break;
        case 28: *reinterpret_cast<int*>(_v) = _t->maxAttachmentSizeMB(); break;
        case 29: *reinterpret_cast<bool*>(_v) = _t->autoCompactEnabled(); break;
        case 30: *reinterpret_cast<int*>(_v) = _t->autoCompactThreshold(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 2: _t->setServerUrl(*reinterpret_cast<QString*>(_v)); break;
        case 3: _t->setServerUsername(*reinterpret_cast<QString*>(_v)); break;
        case 4: _t->setServerPassword(*reinterpret_cast<QString*>(_v)); break;
        case 5: _t->setAutoStartServer(*reinterpret_cast<bool*>(_v)); break;
        case 6: _t->setServerCommand(*reinterpret_cast<QString*>(_v)); break;
        case 7: _t->setWorkDir(*reinterpret_cast<QString*>(_v)); break;
        case 8: _t->setModelProvider(*reinterpret_cast<QString*>(_v)); break;
        case 9: _t->setModelId(*reinterpret_cast<QString*>(_v)); break;
        case 10: _t->setApiFlavor(*reinterpret_cast<QString*>(_v)); break;
        case 11: _t->setLastSessionId(*reinterpret_cast<QString*>(_v)); break;
        case 12: _t->setPackId(*reinterpret_cast<QString*>(_v)); break;
        case 13: _t->setVariant(*reinterpret_cast<QString*>(_v)); break;
        case 14: _t->setThemeMode(*reinterpret_cast<QString*>(_v)); break;
        case 15: _t->setAccentColor(*reinterpret_cast<QString*>(_v)); break;
        case 16: _t->setScale(*reinterpret_cast<double*>(_v)); break;
        case 17: _t->setOverlayOpacity(*reinterpret_cast<double*>(_v)); break;
        case 18: _t->setCornerRadius(*reinterpret_cast<int*>(_v)); break;
        case 19: _t->setDensity(*reinterpret_cast<QString*>(_v)); break;
        case 22: _t->setAnimations(*reinterpret_cast<bool*>(_v)); break;
        case 23: _t->setFontFamily(*reinterpret_cast<QString*>(_v)); break;
        case 24: _t->setCodeFontFamily(*reinterpret_cast<QString*>(_v)); break;
        case 25: _t->setControlStyle(*reinterpret_cast<QString*>(_v)); break;
        case 26: _t->setPlaceAtCursor(*reinterpret_cast<bool*>(_v)); break;
        case 27: _t->setHideOnFocusLoss(*reinterpret_cast<bool*>(_v)); break;
        case 28: _t->setMaxAttachmentSizeMB(*reinterpret_cast<int*>(_v)); break;
        case 29: _t->setAutoCompactEnabled(*reinterpret_cast<bool*>(_v)); break;
        case 30: _t->setAutoCompactThreshold(*reinterpret_cast<int*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *AppConfig::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AppConfig::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9AppConfigE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AppConfig::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 31;
    }
    return _id;
}

// SIGNAL 0
void AppConfig::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
