/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of AwesomeMediaLibraryManager.
 *
 * AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEBUGHELPERS_H
#define DEBUGHELPERS_H

/// Std C++
#include <type_traits>

/// Qt5
#include <QObject>
#include <QMetaMethod>
#include <QString>
#include <QDebug>
#include <QThread>
#include <QModelIndex>

/// Ours
#include "StringHelpers.h"



/// @name General Qt5-specific debug helpers.
/// @todo Move these to their own file.
/// @{

/**
 * Streaming operator for qDebug() << std::string.
 *
 * @note Seriously.
 */
inline static QDebug& operator<<(QDebug& d, const std::string& s)
{
	return d << toqstr(s);
}

/**
 * Shorter qDebug() etc. replacements.
 */
#define qDb() qDebug()
#define qIn() qInfo()
#define qWr() qWarning()
#define qCr() qCritical()

/// Stream out a warning of @a cond holds true.
#define AMLM_WARNIF(cond) if((cond)) { qWr() << #cond << cond; }

#define AMLM_ASSERT_IN_GUITHREAD() do { Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread()); } while(0)

/// From -> To
//#define AMLM_ASSERT_PTR_IS_CONVERTIBLE(a, b) if(dynamic_cast<std::remove_pointer_t<decltype(b)>>(a) == 0) \
//    { qCr() << "pointers are not dynamic_cast<> convertible:" << #a ":" << a << #b ":" << b;  Q_ASSERT(0); }
//#define AMLM_ASSERT_IS_CONVERTIBLE(a, b) if((a) == (b)) { qCr() << "Pointers not convertible:" << #a ":" << a << #b ":" << b; Q_ASSERT(0); }

inline static void dump_qobject(QObject* qobj)
{
#define out() qDebug()
    out() << "Dumping ObjectInfo for QObject:" << qobj;
    // No known control on where this goes other than "to debug output".
    qobj->dumpObjectInfo();
    out() << "Dumping ObjectTree for QObject:" << qobj;
    qobj->dumpObjectTree();
#undef out
}

///**
// * QObject property dumper.
// */
//Q_DECLARE_METATYPE(QModelIndex)
//inline static void dump_properties(QModelIndex* obj, QDebug dbg_stream = qDebug())
//{
//#define out() dbg_stream << M_THREADNAME()

//    auto dynamic_prop_names = obj->dynamicPropertyNames();

//    out() << toqstr("Dynamic properties of QObject:") << obj << toqstr(":");
//    for(auto prop_name : dynamic_prop_names)
//    {
//        out() << prop_name << ":" << obj->property(prop_name);
//    }

//#undef out
//}

class SignalHook : public QObject
{
    Q_OBJECT

public:
    SignalHook(QObject* parent = nullptr) : QObject(parent) { hook_all_signals(this); }

    void hook_all_signals(QObject* object)
    {
        m_object = object;
        const QMetaObject *me = object->metaObject();
        int methodCount = me->methodCount();
        for(int i = 0; i < methodCount; i++)
        {
            QMetaMethod method = me->method(i);
            if(method.methodType() == QMetaMethod::Signal)
            {
            	// Found a signal, hook it up to the single snoop handler.
                qDb() << "Hooking signal:" << method.methodSignature() << "of QObject:" << object;
                QObject::connect(object, "2"+method.methodSignature(), this, SLOT(signalFired()));
            }
        }
    }

protected:
    void connectNotify(const QMetaMethod &signal) override
    {
        qDb() << "Signal connected:" << signal.access() << signal.methodSignature();
    }

    void disconnectNotify(const QMetaMethod &signal) override
    {
        qDb() << "Signal disconnected:" << signal.access() << signal.methodSignature();
    }

public Q_SLOTS:
    void signalFired()
	{
        // Determine as many details about the sent signal as we can.
        QObject* sender = this;
        const QMetaObject* sender_metaobject = sender->metaObject();
        int sender_signal_index = senderSignalIndex();

        QMetaMethod sender_method = sender_metaobject->method(sender_signal_index);

        QString signal_signature = sender_method.methodSignature();
        qDb() << "SIGNAL FIRED FROM OBJECT:" << m_object << ", SIGNAL:" << signal_signature;
	}

private:
	QObject* m_object {nullptr};
};

/// @}


/// @name Helpers for the M_IDSTR() macro below.
/// @{

/// SFINAE version for T already convertible to std::string.
template <typename T>
static inline auto idstr(const char *id_as_c_str, T id) -> std::enable_if_t<std::is_convertible<T, std::string>::value == true, std::string>
{
	return std::string(id_as_c_str) + "(" + id + ")";
}

/// SFINAE version for T not convertible to std::string.
template <typename T>
static inline
std::enable_if_t<std::is_convertible<T, std::string>::value == false, std::string>
idstr(const char *id_as_c_str, T id)
{
	return std::string(id_as_c_str) + "(" + std::to_string(id) + ")";
}

/// @}

#define M_IDSTR(id) idstr(#id ": ", id) + ", " +


/// Attempts to get the compiler to print a human-readable type name at compile time.
/// @note In the 21st century, this should be a solved problem.  It isn't.
/// @see https://stackoverflow.com/a/46339450, https://stackoverflow.com/a/30276785
//		typedef typename ft_test2_class::something_made_up X;
//		bool x = decltype(&ft_test2_class::ft_test2_class)::nothing;
template<typename T>
void print_type_in_compilation_error(T&&)
{
	static_assert(std::is_same<T, int>::value && !std::is_same<T, int>::value, "Compilation failed because you wanted to read the type. See below");
}
/// Usage: use this anywhere to print a type name.
/// Prints as a side-effect of failing the compile.
/// @note Only works if t is a variable.
//#define M_PRINT_TYPE_IN_ERROR(t) void dummy(void) { print_type_in_compilation_error((t)); }
//#define M_PRINT_TYPE_IN_ERROR(T) typedef typename T::something_made_up X;
#define M_PRINT_TYPEOF_VAR_IN_ERROR(v) bool x = decltype((v))::no_such_member_so_you_can_see_the_type_name;
#define M_PRINT_TYPEOF_TEMPLATE_PARAM_T(T) typedef typename T::no_such_member_so_you_can_see_the_type_name X;

/// @name Preprocessor helpers for M_WARNING().
/// @{
#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)
#define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) "): "
/// @}

#define M_NAME_VAL(id) STRINGISE_IMPL(id) ":" << id


/**
 * Portable compile-time warning message.
 * Use: M_WARNING("My message")
 */
#if defined(_MSC_VER)
#   define M_WARNING(exp) __pragma(message(FILE_LINE_LINK "warning C2660: " exp))
#elif defined(__clang__)
#   define DEFER(M, ...) M(__VA_ARGS__)
#   define M_WARNING(X) _Pragma(STRINGISE_IMPL(GCC warning(X " at line " DEFER(STRINGISE_IMPL, __LINE__))))
#elif defined(__GNUC__) || defined(__GNUCXX__)
#	define DO_PRAGMA(x) _Pragma(#x)
#   define M_WARNING(exp) DO_PRAGMA(message FILE_LINE_LINK "warning: " exp)
#endif

#endif // DEBUGHELPERS_H
