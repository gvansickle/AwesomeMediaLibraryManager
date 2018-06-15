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

#ifndef AWESOMEMEDIALIBRARYMANAGER_REGISTERQTMETATYPES_H
#define AWESOMEMEDIALIBRARYMANAGER_REGISTERQTMETATYPES_H

#include <functional>
#include <vector>

void RegisterQtMetatypes();

int RegisterQTRegCallback(std::function<void(void)> f);

/**
 * Singleton class for static-init-time registering of callbacks to be called
 * immediately after the QApp has been created.
 *
 * Uses the Construct On First Use Idiom.
 * https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use
 */
class QtRegCallbackRegistry
{
public:
    QtRegCallbackRegistry() = default;

    void register_callback(std::function<void(void)> callback);
    static void static_append(std::function<void(void)> f);
    void call_registration_callbacks();

private:
    std::vector<std::function<void(void)>> m_registered_callbacks;
};

QtRegCallbackRegistry& reginstance();

#define AMLM_QREG_CALLBACK(f) static int dummy = (reginstance().register_callback(f), 0)

#endif //AWESOMEMEDIALIBRARYMANAGER_REGISTERQTMETATYPES_H
