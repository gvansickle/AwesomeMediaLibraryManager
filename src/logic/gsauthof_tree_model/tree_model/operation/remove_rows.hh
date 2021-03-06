// Copyright 2016, Georg Sauthoff <mail@georg.so>

/* {{{ LGPLv3

    This file is part of tree-model.

    tree-model is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    tree-model is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with tree-model.  If not, see <http://www.gnu.org/licenses/>.

}}} */
#ifndef TREE_MODEL_OPERATION_REMOVE_ROWS_HH
#define TREE_MODEL_OPERATION_REMOVE_ROWS_HH

#include <tree_model/operation/base.hh>
#include <tree_model/deep_model_index.hh>

#include <memory>
#include <deque>

class QAbstractItemModel;
class QModelIndex;
class QMimeData;

namespace tree_model {
  namespace operation {

    class Remove_Rows : public Base {
      private:
        Deep_Model_Index parent_;
        int first_ {0};
        int last_  {0};
        std::deque<std::unique_ptr<QMimeData> > data_;
        int row_count_ {0};
      public:
        Remove_Rows(const QModelIndex &parent, int first, int last,
            QAbstractItemModel &model
            );
        ~Remove_Rows();

        void forward(QAbstractItemModel &model) override;
        void rewind(QAbstractItemModel &model) override;
    };

  }
}

#endif
