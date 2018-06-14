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

class QtRegCallbackRegistry
{
public:
    QtRegCallbackRegistry() {}

    static QtRegCallbackRegistry& instance();

    void register_callback(std::function<void(void)> callback);

private:
    static std::vector<std::function<void(void)>> m_registered_callbacks;
};

//#define AMLM_QREG_CALLBACK(f) static int xxxx_dummy_var ##__LINE__ = RegisterQTRegCallback(f);
//#define AMLM_QREG_CALLBACK(f) QtRegCallbackRegistry::instance()->register_callback(f)
//template <class T>
//void AMLM_QREG_CALLBACK(T f) static auto dummy = [](){ QtRegCallbackRegistry::instance()->register_callback(f); }();
#define AMLM_QREG_CALLBACK(f) static int dummy = [](){ QtRegCallbackRegistry::instance().register_callback(f); return 5; }();

#endif //AWESOMEMEDIALIBRARYMANAGER_REGISTERQTMETATYPES_H
