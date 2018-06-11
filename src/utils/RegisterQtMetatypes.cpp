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

#include "RegisterQtMetatypes.h"

/// Std C++
#include <vector>
#include <functional>

#include <ThreadWeaver/Job>
#include <KJob>

#include <logic/LibraryEntry.h>
#include <logic/PlaylistModelItem.h>
#include <utils/Fraction.h>
#include <logic/LibraryRescanner.h>
#include <logic/LibraryEntryMimeData.h>
#include <concurrency/AMLMJob.h>
#include <logic/LibraryRescannerMapItem.h>

/**
 * Why do we need this?  According to: https://woboq.com/blog/qmetatype-knows-your-types.html
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
 */


static std::vector<std::function<void(void)>> f_register_callbacks;


void RegisterQtMetatypes()
{

    // Call all the registration callbacks.
    for(std::function<void(void)>& f : f_register_callbacks)
    {
        qDb() << "Calling registration callback, total:" << f_register_callbacks.size();
        f();
    }

	// ThreadWeaver types.
    qRegisterMetaType<ThreadWeaver::Job*>( "ThreadWeaver::Job*" );
    // KJob types.
    qRegisterMetaType<KJob*>( "KJob*" );


	// Register the types we want to be able to use in Qt's queued signal and slot connections or in QObject's property system.
	qRegisterMetaType<LibraryEntry>();
	qRegisterMetaType<PlaylistModelItem>();
	qRegisterMetaType<std::shared_ptr<LibraryEntry>>();
	qRegisterMetaType<std::shared_ptr<PlaylistModelItem>>();
	
	// Cast std::shared_ptr<PlaylistModelItem> to std::shared_ptr<LibraryEntry>.
	auto PlaylistModelItemToLibraryEntry = [](const std::shared_ptr<PlaylistModelItem> plmi)
		{
			return std::dynamic_pointer_cast<LibraryEntry>(plmi);
		};
	QMetaType::registerConverter< std::shared_ptr<PlaylistModelItem>, std::shared_ptr<LibraryEntry> >(PlaylistModelItemToLibraryEntry);

	qRegisterMetaType<Fraction>();
	qRegisterMetaTypeStreamOperators<Fraction>("Fraction");

	// From #include <logic/LibraryRescanner.h>
	qRegisterMetaType<MetadataReturnVal>();
	qRegisterMetaType<QFuture<MetadataReturnVal>>();
	qRegisterMetaType<VecLibRescannerMapItems>("VecLibRescannerMapItems");

	// #include <logic/LibraryEntryMimeData.h>
	qRegisterMetaType<LibraryEntryMimeData*>();

	qRegisterMetaType<AMLMJobPtr>();

	qRegisterMetaType<LibraryRescannerMapItem>();
	qRegisterMetaType<VecLibRescannerMapItems>();
//	qRegisterMetaTypeStreamOperators<LibraryRescannerMapItem>("LibraryRescannerMapItem");
}

int RegisterQTRegCallback(const std::function<void(void)>& f)
{
    f_register_callbacks.push_back(f);

    std::cerr << "Registering callback:" << &f << ", size now:" << f_register_callbacks.size() << "\n";

    // Return a non-const but dummy value.
    return 2;//static_cast<int>(&f);
}
