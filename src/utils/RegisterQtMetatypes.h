/*
 * Copyright 2017, 2018, 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef AWESOMEMEDIALIBRARYMANAGER_REGISTERQTMETATYPES_H
#define AWESOMEMEDIALIBRARYMANAGER_REGISTERQTMETATYPES_H

// Std C++
#include <functional>
#include <vector>
#include <iostream>
#include <cstdint>
#include <memory>
#include <deque>
#include <optional>

// Qt5
#include <QObject>

// Qt5 metatype declarations for some std containers, smart_ptr's, etc.
Q_DECLARE_METATYPE(std::string);
Q_DECLARE_SMART_POINTER_METATYPE(std::unique_ptr);
Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_SMART_POINTER_METATYPE(std::weak_ptr);
Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(std::deque);
// Qt5 metatype declarations for metatypes for <cstdint>.
Q_DECLARE_METATYPE(std::int32_t);
Q_DECLARE_METATYPE(std::uint32_t);
Q_DECLARE_METATYPE(std::int64_t);
Q_DECLARE_METATYPE(std::uint64_t);
//Q_DECLARE_METATYPE(std::size_t);
//Q_DECLARE_METATYPE(std::ssize_t);
// Some of ours.
Q_DECLARE_METATYPE(std::optional<bool>);


void RegisterQtMetatypes();

int RegisterQTRegCallback(std::function<void(void)> f);

/**
 * Singleton class for static-init-time registering of callbacks to be called
 * immediately after the QApp has been created.
 *
 * @note Welcome to the "static initialization order fiasco":
 * @link https://isocpp.org/wiki/faq/ctors#static-init-order
 *
 * Uses the Construct On First Use Idiom.
 * https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
 */
class QtRegCallbackRegistry
{
public:
    QtRegCallbackRegistry() = default;

    int* register_callback(std::function<void(void)> callback);
	int* register_callback(const char* name, std::function<void(void)> callback);

	static void static_append(std::function<void(void)> f);
    void call_registration_callbacks();

private:
    std::vector<std::function<void(void)>> m_registered_callbacks {};
};

/**
 * The QtRegCallbackRegistry singleton accessor.
 *
 * Uses the "Construct On First Use Idiom" to ensure that the singleton is:
 * a) Created once; after the first call, subsequent calls return the same object instance.
 * b) Created by the first call.
 * @link https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
 */
QtRegCallbackRegistry& reginstance();

/**
 * We're attempting to take advantage of Deferred dynamic initialization here.
 * @link https://en.cppreference.com/w/cpp/language/initialization#Non-local_variables
 * Wait, no we're not.  God this language.
 */
#define TOKEN_PASTE_HELPER(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_HELPER(x, y)

template <typename T>
struct StaticInitBaseBase
{
	enum
	{
		Defined = 0
	};
};

template <typename T>
struct StaticInitBase : public StaticInitBaseBase<T>
{
	int dummy_odr_use()
	{
		return rand();
	}
};

/**
 * This is me trying to avoid using Q_GLOBAL_STATIC().
 * @link https://doc.qt.io/qt-5/qglobalstatic.html#Q_GLOBAL_STATIC.
 */
#if 1
#define AMLM_QREG_CALLBACK(...) static auto TOKEN_PASTE(dummy, __COUNTER__) = (reginstance().register_callback(__VA_ARGS__), rand())

#else
template <class Lambda>
class QGSBaseClass
{
public:
	explicit QGSBaseClass(Lambda l)
	{
		m_l = l;
	}

	Lambda m_l;
};

#define AMLM_QREG_CALLBACK(...) /*class TOKEN_PASTE(dummy, __COUNTER__) {};*/ \
	Q_GLOBAL_STATIC_WITH_ARGS(QGSBaseClass, staticType, (__VA_ARGS__));\
	/*static auto TOKEN_PASTE(dummy, __COUNTER__) = (reginstance().register_callback(__VA_ARGS__), rand())*/
#endif

//#define AMLM_QREG_CALLBACK(...) AMLM_QREG_CALLBACK2(FIXME, __VA_ARGS__)


#define AMLM_QREG_CALLBACK2(classname, ...) \
	template <>\
	struct StaticInitBase< classname >  /*classname ## _AMLM_QREG_CALLBACK*/ \
	{\
		StaticInitBase< classname > /*classname ## _AMLM_QREG_CALLBACK*/ () \
        {\
	this->m_dummy_int = __LINE__;\
	std::cout << "TEST" << std::endl;\
	std::cerr << "TEST2" << std::endl;\
            reginstance().register_callback( # classname, __VA_ARGS__);\
        }\
	int m_dummy_int{};	\
	};\
	static StaticInitBase< classname > odr_use_THISISTOPREVENTACCIDENTALUSE_ ## classname ;

#define AMLM_QREG_CALLBACK_ODR_USE(classname) \
	static const auto* unused = std::addressof( odr_use_THISISTOPREVENTACCIDENTALUSE_ ## classname );


#endif //AWESOMEMEDIALIBRARYMANAGER_REGISTERQTMETATYPES_H
