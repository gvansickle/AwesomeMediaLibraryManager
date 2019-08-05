/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TREEITEM_H
#define TREEITEM_H

#include <memory>

#include <QList>
#include <QVariant>
#include <QVector>

// Ours
#include <logic/serialization/ISerializable.h>
#include <logic/UUIncD.h>
#include <future/enable_shared_from_this_virtual.h>


class TreeItem : public virtual ISerializable, public enable_shared_from_this_virtual<TreeItem>
{
public:
	explicit TreeItem(const QVector<QVariant> &data, const std::shared_ptr<TreeItem>& parent = nullptr, UUIncD id = UUIncD::null());
    ~TreeItem() override;

	std::shared_ptr<TreeItem> child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;

    /**
     * @note This is where all(?) children are ultimately created.
     */
    std::vector<std::shared_ptr<TreeItem>> insertChildren(int position, int count, int columns);
	///AQP
	std::shared_ptr<TreeItem> insertChild(int row, std::shared_ptr<TreeItem> item);

    bool insertColumns(int position, int columns);
	std::shared_ptr<TreeItem> parent();
	std::weak_ptr<TreeItem> parent_item() const;
	bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    bool setData(int column, const QVariant &value);

	/// GRVS
//	void append_child(TreeItem* new_item);
	/**
	 * Returns the UUIncD assigned to this item on construction.
	 */
	UUIncD getId() const;

	/// @todo
	QVariant toVariant() const override { return QVariant(); };
	void fromVariant(const QVariant& variant) override {};

private:
	QList<std::shared_ptr<TreeItem>> m_child_items;
    QVector<QVariant> m_item_data;
	std::weak_ptr<TreeItem> parentItem;

	/// GRVS
	/// Our guaranteed-to-be unique-to-this-run-of-the-program numeric ID.
	UUIncD m_uuincid { UUIncD::null() };
};


#endif // TREEITEM_H
