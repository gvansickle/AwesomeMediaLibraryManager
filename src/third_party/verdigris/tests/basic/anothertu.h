/****************************************************************************
 *  Copyright (C) 2018 Woboq GmbH
 *  Olivier Goffart <contact at woboq.com>
 *  https://woboq.com/
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QObject>
#include <wobjectimpl.h>

namespace AnotherTU {
struct Gaga {
    W_GADGET(Gaga)
public:
    int m_foo;
    W_PROPERTY(int, foo MEMBER m_foo);
};


struct InlineGadget {
    W_GADGET(InlineGadget)
public:
    int m_foo;
    W_PROPERTY(int, foo MEMBER m_foo)
};

struct InlineObject : QObject {
    W_OBJECT(InlineObject)
public:
    int m_foo;
    W_PROPERTY(int, foo MEMBER m_foo)
};

}

#ifdef __cpp_inline_variables
W_GADGET_IMPL_INLINE(AnotherTU::InlineGadget)
W_OBJECT_IMPL_INLINE(AnotherTU::InlineObject)
#endif

