/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "RegisterQtMetatypes.h"

// Std C++
#include <vector>
#include <functional>
#include <string>
#include <cstdint>

// KF5
#include <KJob>

// Ours.
#include <future/preproc.h>
#include <logic/LibraryEntry.h>
#include <logic/PlaylistModelItem.h>
#include <utils/Fraction.h>
#include <logic/LibraryRescanner.h>
#include <logic/LibraryEntryMimeData.h>
//#include <concurrency/AMLMJob.h>
#include <logic/LibraryRescannerMapItem.h>
#include <logic/AudioFileType.h>

/**
 * Why do we need this qRegisterMetaType<> mechanism?  It appears that we really don't.
 *
 * According to: https://woboq.com/blog/qmetatype-knows-your-types.html
 *
 * "It enables things such as QVariant wrapping of custom types, copy of queued connection arguments, and more."
 *
 * However, he goes on to say this is almost unnecessary:
 *
 * "That was it for Q_DECLARE_METATYPE, but you would still need to call qRegisterMetaType to use these type
 * in a Q_PROPERTY or as a parameter in a signal/slot queued connection. Since Qt 5.x however, the code generated
 * by moc will call qRegisterMetaType for you if moc can determine that the type may be registered as a meta-type."
 *
 * There's also this from StackOverflow ca. 2013: https://stackoverflow.com/a/19380656
 *
 * "Foo and Foo* are different types and need to be registered separately. [...]
 * Normally, you would only register the pointer form if your class cannot be copied,
 * as in the case of QObject and derivatives, but this depends on your requirements."
 *
 * Docs:
 * http://doc.qt.io/qt-5/qmetatype.html#qRegisterMetaType-1
 *
 * And one more, from here: @link ftp://ftp.informatik.hu-berlin.de/pub/Linux/Qt/QT/videos/DevDays2011/TrainingDay/DevDays2011_-_Advanced_Qt_-_A_Deep_Dive.pdf
 *
 * Cross-Thread Signals/Slots with Custom Classes
 * - If you want to emit signals across threads that use custom
 * (your own) data types, those data types need to be made
 * known to Qt for serializing.
 * - Tell Qt about your types with Q_DECLARE_METATYPE:
 * class MyType { ... };
 * Q_DECLARE_METATYPE(MyType)
 * - In case Qt complains it does not know MyType, check that you have
 * Q_DECLARE_METATYPE(MyType) in the header file of the class, outside any namespace.
 *
 * If that still does not solve the problem, you have to call qRegisterMetaType<MyType>()
 * yourself.
 */


#define EXPAND(x) x
#define ADD_STD(base_type) /*TOKENPASTE*/std::base_type
#define ADD_U(base_type) TOKENPASTE2(u, base_type)
#define ADD_BITS(base_type, bits) TOKENPASTE2(base_type, bits)
#define ADD_T(base_type) TOKENPASTE2(base_type, _t)
#define DUP_NS_AND_NOT(X, base_type) X(base_type) X(std::base_type)

/// Qt5's take on construct-on-first-use.  Not sure we need it here, see reginstance() below.
/// @link https://doc.qt.io/qt-5/qglobalstatic.html
Q_GLOBAL_STATIC(QtRegCallbackRegistry, f_qt_reg_callback_registry);


/**
 * Register a number of general-purpose Qt5 converters etc.
 */
AMLM_QREG_CALLBACK([](){
	qIn() << "Registering std::string->QString converter";

	QMetaType::registerConverter<std::string, QString>([](const std::string& str){ return toqstr(str); });
	QMetaType::registerConverter<QString, std::string>([](const QString& str){ return tostdstr(str); });

	qIn() << "Registering <cstdint> metatypes";

#define RMT(full_type) qRegisterMetaType< full_type >( # full_type );
	DUP_NS_AND_NOT(RMT, int8_t);
	DUP_NS_AND_NOT(RMT, int32_t);
	DUP_NS_AND_NOT(RMT, uint32_t);
	DUP_NS_AND_NOT(RMT, int64_t);
	DUP_NS_AND_NOT(RMT, uint64_t);
});
/// @todo Compile breaks if this is in the lambda.
#undef RMT


void RegisterQtMetatypes()
{
    // Call all the registration callbacks which have been registered during static-init time.
    qIn() << "START Calling registered callbacks";
	reginstance().call_registration_callbacks();


	{
		auto a = qRegisterMetaType<AudioFileType>();
		qIn() << "Registering AudioFileType, QMetaType:" << a;
	}

	qIn() << "END Calling registered callbacks";

    // KJob types.
    qRegisterMetaType<KJob*>();


	// Register the types we want to be able to use in Qt's queued signal and slot connections or in QObject's property system.
	qRegisterMetaType<LibraryEntry>();
	qRegisterMetaType<PlaylistModelItem>();
	qRegisterMetaType<std::shared_ptr<LibraryEntry>>();
	qRegisterMetaType<std::shared_ptr<PlaylistModelItem>>();
	
	// Cast std::shared_ptr<PlaylistModelItem> to std::shared_ptr<LibraryEntry>.
	auto PlaylistModelItemToLibraryEntry = [](const std::shared_ptr<PlaylistModelItem> plmi)
		{
			// dynamic_pointer_cast<> takes care of keeping the shared ptr ref counts correct.
			return std::dynamic_pointer_cast<LibraryEntry>(plmi);
		};
	QMetaType::registerConverter< std::shared_ptr<PlaylistModelItem>, std::shared_ptr<LibraryEntry> >(PlaylistModelItemToLibraryEntry);

	// #include <logic/LibraryEntryMimeData.h>
	qRegisterMetaType<LibraryEntryMimeData*>();

	qRegisterMetaType<LibraryRescannerMapItem>();
	qRegisterMetaType<VecLibRescannerMapItems>();
}


int* QtRegCallbackRegistry::register_callback(std::function<void(void)> callback)
{
    m_registered_callbacks.push_back(callback);
    std::cerr << "Registering callback:" << &callback << ", size now:" << m_registered_callbacks.size() << "\n";
    // Return a dummy pointer.
	return reinterpret_cast<int*>(&callback);
}

int* QtRegCallbackRegistry::register_callback(const char* name, std::function<void(void)> callback)
{
	m_registered_callbacks.push_back(callback);
	std::cerr << "Registering callback, name:" << name << "address:" << &callback << ", size now:" << m_registered_callbacks.size() << "\n";
	// Return a dummy pointer.
	return reinterpret_cast<int*>(&callback);
}

void QtRegCallbackRegistry::call_registration_callbacks()
{
    // Call all the registration callbacks.
    for(std::function<void(void)>& f : m_registered_callbacks)
    {
        qDb() << "Calling registration callback:" << &f << ", total:" << m_registered_callbacks.size();
        f();
    }
}

QtRegCallbackRegistry& reginstance()
{
	// The callback registry.  static local so that it will be created only once, on first use,
	// Returning a ref to it so that it's accessible to all callers.
	// @link https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
    static QtRegCallbackRegistry* retval = new QtRegCallbackRegistry();
    return *retval;
}
