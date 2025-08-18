/*
 * Copyright 2017, 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/// @file

// Std C++
#include <functional>
#include <vector>
#include <iostream>
#include <cstdint>
#include <memory>
#include <deque>
#include <optional>
#include <string>

// Qt
#include <QObject>
#include <QDebug>
#include <QMetaType>

// Qt metatype declarations for some std containers.
Q_DECLARE_METATYPE(std::string);
Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr);
Q_DECLARE_SMART_POINTER_METATYPE(std::weak_ptr);
Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(std::deque);
// Qt metatype declarations for metatypes for <cstdint>.
Q_DECLARE_METATYPE(std::int32_t);
Q_DECLARE_METATYPE(std::uint32_t);
Q_DECLARE_METATYPE(std::int64_t);
Q_DECLARE_METATYPE(std::uint64_t);
//Q_DECLARE_METATYPE(std::size_t);
//Q_DECLARE_METATYPE(std::ssize_t);

// One of ours.
inline QDebug operator<<(QDebug debug, const std::optional<bool>& optbool)
{
	if(optbool.has_value())
	{
		debug << (optbool.value() ? "true" : "false");
	}
	else
	{
		debug << "(unknown)";
	}
	return debug;
}
Q_DECLARE_METATYPE(std::optional<bool>);


void RegisterQtMetatypes();

int RegisterQTRegCallback(std::function<void(void)> f);

/**
 * Singleton class for static-init-time registering of callbacks to be called
 * immediately after the QApp has been created.
 *
 * @note Welcome to the "static initialization order fiasco":
 * https://isocpp.org/wiki/faq/ctors#static-init-order
 *
 * Uses the Construct On First Use Idiom.
 * https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
 */
class QtRegCallbackRegistry
{
public:
    QtRegCallbackRegistry() = default;

    void register_callback(std::function<void(void)> callback);
	void register_callback(const char* name, std::function<void(void)> callback);

	static void static_append(std::function<void(void)> f);
    void call_registration_callbacks();

private:
    std::vector<std::function<void(void)>> m_registered_callbacks {};
};

/**
 * The QtRegCallbackRegistry singleton accessor.
 *
 * Uses the "Construct On First Use Idiom" to ensure that the singleton is:
 * 1. Created once; after the first call, subsequent calls return the same object instance.
 * 2. Created by the first call.
 *
 * @see https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
 */
QtRegCallbackRegistry& reginstance();

/**
 * We're attempting to take advantage of Deferred dynamic initialization here.
 * https://en.cppreference.com/w/cpp/language/initialization#Non-local_variables
 * Wait, no we're not.  God this language.
 */
#define TOKEN_PASTE_HELPER(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_HELPER(x, y)


#if 1
/**
 * With this, we get an un-silenceable "-Wclazy-non-pod-global-static" warning at every point of use.  It works otherwise.
 */
// #define AMLM_QREG_CALLBACK(...) static auto TOKEN_PASTE(dummy, __COUNTER__) = (reginstance().register_callback(__VA_ARGS__), rand()) // clazy:exclude=non-pod-global-static
#define AMLM_QREG_CALLBACK(...) static volatile auto TOKEN_PASTE(dummy, __COUNTER__) = (reginstance().register_callback(__VA_ARGS__), 1) // clazy:exclude=non-pod-global-static
// #define AMLM_QREG_CALLBACK(...) static constinit auto TOKEN_PASTE(dummy, __COUNTER__) = (reginstance().register_callback(__VA_ARGS__), 1) // clazy:exclude=non-pod-global-static
// _Pragma("GCC diagnostic push") \
// _Pragma("GCC diagnostic ignored \"-Wunknown-pragmas\"") \
// _Pragma("GCC diagnostic ignored \"-Wunknown-warning-option\"") \
// _Pragma("GCC diagnostic ignored \"-Wclazy-non-pod-global-static\"") \
// _Pragma("GCC diagnostic pop")
#else
// This doesn't work, unclear why.
#define AMLM_QREG_CALLBACK(...) \
    struct TOKEN_PASTE(amlm_internal_pre_main_init, __COUNTER__) { \
		static bool init() { \
			reginstance().register_callback(__VA_ARGS__); \
			return true; \
		} \
        static inline bool initialized = init(); \
    }

// #define AMLM_QREG_CALLBACK(...) AMLM_QREG_CALLBACK_IMPL(__COUNTER__, __VA_ARGS__)
#endif



#endif //AWESOMEMEDIALIBRARYMANAGER_REGISTERQTMETATYPES_H
