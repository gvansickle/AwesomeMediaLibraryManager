/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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



#include <logic/serialization/ExtEnum.h>


#if 0
///// @temp Testing

struct ExtEnum2
{
	ExtEnum2 EA, EB;
};

//DECL_EXTENUM(MyEvenBetterEnum, MyEnumerator1{"name", 0, 0}, MyEnumerator2(1, 1), MyE3("MyE3", 0x01, 3), MyE4(987, 2), MyOtherE3(0x01, 3));
//struct MyEvenBetterEnum : public ExtEnumerator {};
DECL_EXTENUM(MyEvenBetterEnum2, MyE3, 0); //, ({"MyE3", 0x01, 3}));//, MyE4(987, 2), MyOtherE3(0x01, 3));

//inline constexpr MyEvenBetterEnum MyEnumerator1(0, 0), MyEnumerator2(1, 1), MyE3("MyE3", 0x01, 3), MyE4(987, 2);

//struct MyEvenBetterEnum : public ExtEnum
//{
//	MyEnumerator1,
//	MyEnumerator2,
//	MyE3 = 0x01,
//	MyE4 = 2678
//};

//static_assert(MyEnumerator1 == 0);
//static_assert(MyEnumerator2.toInt() == 1);
//static_assert(MyE3 != 1);
//static_assert(MyE3 == 1);
//static_assert(MyE3 == MyOtherE3);
//static_assert(MyE3.c_str() == "MyE3", "");
//static_assert(MyE4.toString() == "MyE4", "");

#endif
