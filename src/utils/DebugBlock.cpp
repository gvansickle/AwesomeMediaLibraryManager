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

#include "DebugBlock.h"

static thread_local int ftl_indent_level = 0;
static thread_local std::string ftl_indent_str = "";

constexpr int c_indent_level_spaces = 4;

DebugBlock::DebugBlock(const char* function_name) : m_function_name(function_name)
{
    ftl_indent_level++;
    ftl_indent_str.assign(" ", ftl_indent_level*c_indent_level_spaces);
}

DebugBlock::~DebugBlock()
{
    ftl_indent_level--;
    ftl_indent_str.assign(" ", ftl_indent_level*c_indent_level_spaces);
}

std::string DebugBlock::get_indent()
{
    return ftl_indent_str;
}

