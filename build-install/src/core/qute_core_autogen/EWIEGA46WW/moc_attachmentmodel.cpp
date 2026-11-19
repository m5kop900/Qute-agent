/****************************************************************************
** Meta object code from reading C++ file 'attachmentmodel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/core/attachmentmodel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'attachmentmodel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN15AttachmentModelE_t {};
} // unnamed namespace

template <> constexpr inline auto AttachmentModel::qt_create_metaobjectdata<qt_meta_tag_ZN15AttachmentModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AttachmentModel",
        "countChanged",
        "",
        "addFile",
        "urlOrPath",
        "listDirectory",
        "QVariantList",
        "dir",
        "defaultDirectory",
        "addMention",
        "relativePath",
        "baseDir",
        "pasteClipboardImage",
        "clipboardHasImage",
        "clipboardHasText",
        "remove",
        "row",
        "clear",
        "promptFiles",
        "agentRefs",
        "names",
        "count"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'countChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'addFile'
        QtMocHelpers::MethodData<QString(const QString &)>(3, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 4 },
        }}),
        // Method 'listDirectory'
        QtMocHelpers::MethodData<QVariantList(const QString &) const>(5, 2, QMC::AccessPublic, 0x80000000 | 6, {{
            { QMetaType::QString, 7 },
        }}),
        // Method 'defaultDirectory'
        QtMocHelpers::MethodData<QString() const>(8, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'addMention'
        QtMocHelpers::MethodData<QString(const QString &, const QString &)>(9, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 10 }, { QMetaType::QString, 11 },
        }}),
        // Method 'pasteClipboardImage'
        QtMocHelpers::MethodData<QString()>(12, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'clipboardHasImage'
        QtMocHelpers::MethodData<bool() const>(13, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'clipboardHasText'
        QtMocHelpers::MethodData<bool() const>(14, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'remove'
        QtMocHelpers::MethodData<void(int)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 16 },
        }}),
        // Method 'clear'
        QtMocHelpers::MethodData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'promptFiles'
        QtMocHelpers::MethodData<QVariantList() const>(18, 2, QMC::AccessPublic, 0x80000000 | 6),
        // Method 'agentRefs'
        QtMocHelpers::MethodData<QVariantList(const QStringList &) const>(19, 2, QMC::AccessPublic, 0x80000000 | 6, {{
            { QMetaType::QStringList, 20 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'count'
        QtMocHelpers::PropertyData<int>(21, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AttachmentModel, qt_meta_tag_ZN15AttachmentModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AttachmentModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15AttachmentModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15AttachmentModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15AttachmentModelE_t>.metaTypes,
    nullptr
} };

void AttachmentModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AttachmentModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->countChanged(); break;
        case 1: { QString _r = _t->addFile((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 2: { QVariantList _r = _t->listDirectory((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 3: { QString _r = _t->defaultDirectory();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 4: { QString _r = _t->addMention((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 5: { QString _r = _t->pasteClipboardImage();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->clipboardHasImage();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->clipboardHasText();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->remove((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->clear(); break;
        case 10: { QVariantList _r = _t->promptFiles();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 11: { QVariantList _r = _t->agentRefs((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AttachmentModel::*)()>(_a, &AttachmentModel::countChanged, 0))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<int*>(_v) = _t->count(); break;
        default: break;
        }
    }
}

const QMetaObject *AttachmentModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AttachmentModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15AttachmentModelE_t>.strings))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int AttachmentModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void AttachmentModel::countChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
