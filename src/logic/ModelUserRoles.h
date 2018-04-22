/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MODELUSERROLES_H
#define MODELUSERROLES_H

#include <QObject>
#include <QMetaEnum>

/**
 * @todo write docs
 */
class ModelUserRoles
{
    Q_GADGET
    
public:
    
	enum UserRoles
    {
		/// Used in a model to store a pointer to the underlying item.
        PointerToItemRole = Qt::UserRole + 1,
		/// Used in a model's headerData to give a ~UUID to each column in a model.
		HeaderViewSectionID,
		HeaderViewSectionShouldFitWidthToContents
    };
    
	Q_ENUM(UserRoles)

	static QMetaEnum metaEnum() { return QMetaEnum::fromType<ModelUserRoles::UserRoles>(); }
	static const char* valueToKey(UserRoles val) { return metaEnum().valueToKey(val); }
	static int keyCount() { return metaEnum().keyCount(); }
	static const char * key(int index) { return metaEnum().key(index); }
	static UserRoles value(int index) { return static_cast<UserRoles>(metaEnum().value(index)); }
	static const char* name() { return metaEnum().name(); }
	static const char* scope() { return metaEnum().scope(); }

};

Q_DECLARE_METATYPE(ModelUserRoles)


#endif // MODELUSERROLES_H
