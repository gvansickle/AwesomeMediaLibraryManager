/*
 * Copyright 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/**
* @file treetest.cpp
* Adapted from test file of same name from KDenLive.
*/

// Google Test
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Ours
#include "AbstractTreeModel.h"
#include "AbstractTreeModelItem.h"
#include "ColumnSpec.h"

TEST(BasicTreeTests, ItemCreationTests)
{
    SCOPED_TRACE("Item creation Test");

    auto model = AbstractTreeModel::create({ColumnSpec(SectionID::Filename, "Column0")});
    ASSERT_EQ(model->checkConsistency(), true);
    ASSERT_EQ(model->rowCount(), 0);


    auto item = AbstractTreeModelItem::create(std::vector<QVariant>{QStringLiteral("test")}, model, false);
    UUIncD id = item->getId();
    ASSERT_EQ(item->depth(), 0);
    ASSERT_EQ(model->checkConsistency(), true);

    // check that a valid Id has been assigned
    ASSERT_NE(id, UUIncD::null());

    // check that the item is not yet registered (not valid parent)
    ASSERT_EQ(model->get_total_model_node_count(), 1);

    // Assign this to a parent
    ASSERT_TRUE(model->getRootItem()->appendChild(item));
    ASSERT_EQ(model->checkConsistency(), true);
    // Now the item should be registered, we query it
    ASSERT_EQ(/*KdenliveTests::modelSize(model)*/ model->get_total_model_node_count(), 2);
    ASSERT_EQ(model->getItemById(id), item);
    ASSERT_EQ(item->depth(), 1);
    ASSERT_EQ(model->rowCount(), 1);
    ASSERT_EQ(model->rowCount(model->getIndexFromItem(item)),  0);

    // Retrieve data member
    auto index2 = model->getIndexFromItem(item);
    EXPECT_EQ(model->data(index2, 0).toString(),  QStringLiteral("test"));

    // Try joint creation / assignation
    auto item2 = item->appendChild(std::vector<QVariant>{QStringLiteral("test2")});
    auto state = [&]() {
        ASSERT_EQ(model->checkConsistency(), true);
        ASSERT_EQ(item->depth(),  1);
        ASSERT_EQ(item2->depth(),  2);
        ASSERT_EQ(model->rowCount(),  1);
        ASSERT_EQ(item->childNumber(),  0);
        ASSERT_EQ(item2->childNumber(),  0);
        EXPECT_EQ(model->data(model->getIndexFromItem(item2), 0).toString(),  QStringLiteral("test2"));
        ASSERT_EQ(model->rowCount(model->getIndexFromItem(item2)),  0);
    };
    state();
    ASSERT_EQ(model->rowCount(model->getIndexFromItem(item)),  1);
    ASSERT_EQ(model->get_total_model_node_count(), 3);

    // Add a second child to item to check if everything collapses
    auto item3 = item->appendChild(std::vector<QVariant>{QStringLiteral("test3")});
    state();
    ASSERT_EQ(model->rowCount(model->getIndexFromItem(item3)),  0);
    ASSERT_EQ(model->rowCount(model->getIndexFromItem(item)),  2);
    ASSERT_EQ(model->get_total_model_node_count(), 4);
    ASSERT_EQ(item3->depth(),  2);
    ASSERT_EQ(item3->childNumber(),  1);
    EXPECT_EQ(model->data(model->getIndexFromItem(item3), 0).toString(),  QStringLiteral("test3"));
}

TEST(BasicTreeTests, InvalidMoves)
{
    SCOPED_TRACE("Invalid moves");

    auto model = AbstractTreeModel::create({ColumnSpec(SectionID::Filename, "Column0")});
    ASSERT_EQ(model->checkConsistency(), true);
    ASSERT_EQ(model->rowCount(), 0);

    auto item = model->getRootItem()->appendChild(std::vector<QVariant>{QStringLiteral("test")});
    auto state = [&]() {
        ASSERT_EQ(model->checkConsistency(), true);
        ASSERT_EQ(model->rowCount(), 1);
        ASSERT_EQ(item->depth(), 1);
        ASSERT_EQ(item->childNumber(), 0);
        ASSERT_EQ(model->data(model->getIndexFromItem(item), 0), QStringLiteral("test"));
    };
    state();
    ASSERT_EQ(model->rowCount(model->getIndexFromItem(item)), 0);

    // Try to move the root
    ASSERT_FALSE(item->appendChild(model->getRootItem()));
    state();
    ASSERT_EQ(model->rowCount(model->getIndexFromItem(item)), 0);

    auto item2 = item->appendChild(std::vector<QVariant>{QStringLiteral("test2")});
    auto item3 = item2->appendChild(std::vector<QVariant>{QStringLiteral("test3")});
    auto item4 = item3->appendChild(std::vector<QVariant>{QStringLiteral("test4")});
    auto state2 = [&]() {
        state();
        ASSERT_EQ(item2->depth(), 2);
        ASSERT_EQ(item2->childNumber(), 0);
        ASSERT_EQ(model->data(model->getIndexFromItem(item2), 0), QStringLiteral("test2"));
        ASSERT_EQ(item3->depth(), 3);
        ASSERT_EQ(item3->childNumber(), 0);
        ASSERT_EQ(model->data(model->getIndexFromItem(item3), 0), QStringLiteral("test3"));
        ASSERT_EQ(item4->depth(), 4);
        ASSERT_EQ(item4->childNumber(), 0);
        ASSERT_EQ(model->data(model->getIndexFromItem(item4), 0), QStringLiteral("test4"));
    };
    state2();

    // Try to make a loop
    ASSERT_FALSE(item->changeParent(item3));
    state2();
    ASSERT_FALSE(item->changeParent(item4));
    state2();

    // Try to append a child that already have a parent
    ASSERT_FALSE(item->appendChild(item4));
    state2();

    // valid move
    ASSERT_TRUE(item4->changeParent(item2));
    ASSERT_EQ(model->checkConsistency(), true);
}

TEST(BasicTreeTests, ItemsNotInModel)
{
    // SCOPED_TRACE("Add items to not-in-model item tests");

    auto model = AbstractTreeModel::create({ColumnSpec(SectionID::Filename, "Column0")});
    ASSERT_EQ(model->checkConsistency(), true);
    ASSERT_EQ(model->rowCount(), 0);

    auto item = AbstractTreeModelItem::create({QStringLiteral("test")}, model);
    auto item2 = AbstractTreeModelItem::create(std::vector<QVariant>{QStringLiteral("test2")}, model);
    ASSERT_FALSE(item->isInModel());
    ASSERT_FALSE(item2->isInModel());
    ASSERT_EQ(model->rowCount(QModelIndex()), 0);
    ASSERT_EQ(model->checkConsistency(), true);

    ASSERT_TRUE(item->appendChild(item2));
    ASSERT_FALSE(item->isInModel());
    ASSERT_TRUE(item2->isInModel()); /// @todo Should this really be false, since item2 is not actually in a model?
    ASSERT_EQ(model->rowCount(QModelIndex()), 0);
    ASSERT_EQ(model->checkConsistency(), true);

    /// "ASSERT failure in virtual void AbstractTreeModel::register_item(const std::shared_ptr<AbstractTreeModelItem>&): "Item was already in model.""
    /// This is expected.
    EXPECT_DEATH(model->getRootItem()->appendChild(item), "");
#if 0
    EXPECT_TRUE(item->isInModel());
    EXPECT_TRUE(item2->isInModel());
    ASSERT_EQ(model->rowCount(QModelIndex()), 1);
    ASSERT_EQ(model->checkConsistency(), true);
#endif
}

TEST(BasicTreeTests, Deregistration)
{
    SCOPED_TRACE("Deregistration tests");

    auto model = AbstractTreeModel::create({ColumnSpec(SectionID::Filename, "Column0")});
    ASSERT_EQ(model->checkConsistency(), true);
    ASSERT_EQ(model->rowCount(), 0);


    // we construct a non trivial structure
    auto item = model->getRootItem()->appendChild(std::vector<QVariant>{QStringLiteral("test")});
    auto item2 = item->appendChild(std::vector<QVariant>{QStringLiteral("test2")});
    auto item3 = item2->appendChild(std::vector<QVariant>{QStringLiteral("test3")});
    auto item4 = item3->appendChild(std::vector<QVariant>{QStringLiteral("test4")});
    auto item5 = item2->appendChild(std::vector<QVariant>{QStringLiteral("test5")});
    auto state = [&]() {
        ASSERT_EQ(model->checkConsistency(), true);
        ASSERT_EQ(model->rowCount(), 1);
        ASSERT_EQ(item->depth(), 1);
        ASSERT_EQ(item->childNumber(), 0);
        ASSERT_EQ(model->data(model->getIndexFromItem(item), 0), QStringLiteral("test"));
        ASSERT_EQ(model->rowCount(model->getIndexFromItem(item)), 1);
        ASSERT_EQ(item2->depth(), 2);
        ASSERT_EQ(item2->childNumber(), 0);
        ASSERT_EQ(model->data(model->getIndexFromItem(item2), 0), QStringLiteral("test2"));
        ASSERT_EQ(model->rowCount(model->getIndexFromItem(item2)), 2);
        ASSERT_EQ(item3->depth(), 3);
        ASSERT_EQ(item3->childNumber(), 0);
        ASSERT_EQ(model->data(model->getIndexFromItem(item3), 0), QStringLiteral("test3"));
        ASSERT_EQ(model->rowCount(model->getIndexFromItem(item3)), 1);
        ASSERT_EQ(item4->depth(), 4);
        ASSERT_EQ(item4->childNumber(), 0);
        ASSERT_EQ(model->data(model->getIndexFromItem(item4), 0), QStringLiteral("test4"));
        ASSERT_EQ(model->rowCount(model->getIndexFromItem(item4)), 0);
        ASSERT_EQ(item5->depth(), 3);
        ASSERT_EQ(item5->childNumber(), 1);
        ASSERT_EQ(model->data(model->getIndexFromItem(item5), 0), QStringLiteral("test5"));
        ASSERT_EQ(model->rowCount(model->getIndexFromItem(item5)), 0);
        ASSERT_EQ(model->get_total_model_node_count(), 6);
        ASSERT_TRUE(item->isInModel());
        ASSERT_TRUE(item2->isInModel());
        ASSERT_TRUE(item3->isInModel());
        ASSERT_TRUE(item4->isInModel());
        ASSERT_TRUE(item5->isInModel());
    };
    state();

    // deregister the topmost item, should also deregister its children
    item->changeParent(std::shared_ptr<AbstractTreeModelItem>());
    ASSERT_EQ(model->get_total_model_node_count(), 1);
    ASSERT_EQ(model->rowCount(), 0);
    ASSERT_TRUE(!item->isInModel());
    ASSERT_TRUE(!item2->isInModel());
    ASSERT_TRUE(!item3->isInModel());
    ASSERT_TRUE(!item4->isInModel());
    ASSERT_TRUE(!item5->isInModel());

    // reinsert
    ASSERT_TRUE(model->getRootItem()->appendChild(item));
    state();

    item2->removeChild(item5);
    ASSERT_TRUE(!item5->isInModel());
    ASSERT_EQ(model->rowCount(model->getIndexFromItem(item2)), 1);
    ASSERT_EQ(model->get_total_model_node_count(), 5);

    // reinsert
    ASSERT_TRUE(item5->changeParent(item2));
    state();
}


