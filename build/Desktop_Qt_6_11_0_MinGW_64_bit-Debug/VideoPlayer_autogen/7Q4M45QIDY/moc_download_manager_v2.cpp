/****************************************************************************
** Meta object code from reading C++ file 'download_manager_v2.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/download/download_manager_v2.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'download_manager_v2.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
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
struct qt_meta_tag_ZN17DownloadManagerV2E_t {};
} // unnamed namespace

template <> constexpr inline auto DownloadManagerV2::qt_create_metaobjectdata<qt_meta_tag_ZN17DownloadManagerV2E_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DownloadManagerV2",
        "taskAdded",
        "",
        "taskId",
        "taskProgress",
        "percent",
        "taskFinished",
        "success",
        "errorMsg",
        "taskRemoved",
        "taskSpeed",
        "speedKBps",
        "onReadyRead",
        "onFinished",
        "onProgress",
        "received",
        "total",
        "startNextTask"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'taskAdded'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'taskProgress'
        QtMocHelpers::SignalData<void(int, int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 5 },
        }}),
        // Signal 'taskFinished'
        QtMocHelpers::SignalData<void(int, bool, const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Bool, 7 }, { QMetaType::QString, 8 },
        }}),
        // Signal 'taskRemoved'
        QtMocHelpers::SignalData<void(int)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Signal 'taskSpeed'
        QtMocHelpers::SignalData<void(int, int)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 11 },
        }}),
        // Slot 'onReadyRead'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFinished'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProgress'
        QtMocHelpers::SlotData<void(qint64, qint64)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 15 }, { QMetaType::LongLong, 16 },
        }}),
        // Slot 'startNextTask'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DownloadManagerV2, qt_meta_tag_ZN17DownloadManagerV2E_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DownloadManagerV2::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17DownloadManagerV2E_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17DownloadManagerV2E_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN17DownloadManagerV2E_t>.metaTypes,
    nullptr
} };

void DownloadManagerV2::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DownloadManagerV2 *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->taskAdded((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->taskProgress((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->taskFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 3: _t->taskRemoved((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->taskSpeed((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->onReadyRead(); break;
        case 6: _t->onFinished(); break;
        case 7: _t->onProgress((*reinterpret_cast<std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[2]))); break;
        case 8: _t->startNextTask(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DownloadManagerV2::*)(int )>(_a, &DownloadManagerV2::taskAdded, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DownloadManagerV2::*)(int , int )>(_a, &DownloadManagerV2::taskProgress, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DownloadManagerV2::*)(int , bool , const QString & )>(_a, &DownloadManagerV2::taskFinished, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DownloadManagerV2::*)(int )>(_a, &DownloadManagerV2::taskRemoved, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DownloadManagerV2::*)(int , int )>(_a, &DownloadManagerV2::taskSpeed, 4))
            return;
    }
}

const QMetaObject *DownloadManagerV2::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DownloadManagerV2::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17DownloadManagerV2E_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DownloadManagerV2::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void DownloadManagerV2::taskAdded(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void DownloadManagerV2::taskProgress(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void DownloadManagerV2::taskFinished(int _t1, bool _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2, _t3);
}

// SIGNAL 3
void DownloadManagerV2::taskRemoved(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void DownloadManagerV2::taskSpeed(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}
QT_WARNING_POP
