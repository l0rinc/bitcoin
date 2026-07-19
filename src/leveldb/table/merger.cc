// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "table/merger.h"

#include "leveldb/comparator.h"
#include "leveldb/iterator.h"
#include "table/iterator_wrapper.h"

#include <utility>
#include <vector>

namespace leveldb {

namespace {
class MergingIterator : public Iterator {
 public:
  MergingIterator(const Comparator* comparator, Iterator** children, int n)
      : comparator_(comparator),
        children_(new IteratorWrapper[n]),
        n_(n),
        current_(nullptr),
        direction_(kForward) {
    min_heap_.reserve(n);
    for (int i = 0; i < n; i++) {
      children_[i].Set(children[i]);
    }
  }

  ~MergingIterator() override { delete[] children_; }

  bool Valid() const override { return (current_ != nullptr); }

  void SeekToFirst() override {
    for (int i = 0; i < n_; i++) {
      children_[i].SeekToFirst();
    }
    BuildMinHeap();
    direction_ = kForward;
  }

  void SeekToLast() override {
    for (int i = 0; i < n_; i++) {
      children_[i].SeekToLast();
    }
    min_heap_.clear();
    FindLargest();
    direction_ = kReverse;
  }

  void Seek(const Slice& target) override {
    for (int i = 0; i < n_; i++) {
      children_[i].Seek(target);
    }
    BuildMinHeap();
    direction_ = kForward;
  }

  void Next() override {
    assert(Valid());

    // Ensure that all children are positioned after key().
    // If we are moving in the forward direction, it is already
    // true for all of the non-current_ children since current_ is
    // the smallest child and key() == current_->key().  Otherwise,
    // we explicitly position the non-current_ children.
    if (direction_ != kForward) {
      for (int i = 0; i < n_; i++) {
        IteratorWrapper* child = &children_[i];
        if (child != current_) {
          child->Seek(key());
          if (child->Valid() &&
              comparator_->Compare(key(), child->key()) == 0) {
            child->Next();
          }
        }
      }
      direction_ = kForward;
      current_->Next();
      BuildMinHeap();
      return;
    }

    current_->Next();
    ReplaceMinHeapTop();
  }

  void Prev() override {
    assert(Valid());
    min_heap_.clear();

    // Ensure that all children are positioned before key().
    // If we are moving in the reverse direction, it is already
    // true for all of the non-current_ children since current_ is
    // the largest child and key() == current_->key().  Otherwise,
    // we explicitly position the non-current_ children.
    if (direction_ != kReverse) {
      for (int i = 0; i < n_; i++) {
        IteratorWrapper* child = &children_[i];
        if (child != current_) {
          child->Seek(key());
          if (child->Valid()) {
            // Child is at first entry >= key().  Step back one to be < key()
            child->Prev();
          } else {
            // Child has no entries >= key().  Position at last entry.
            child->SeekToLast();
          }
        }
      }
      direction_ = kReverse;
    }

    current_->Prev();
    FindLargest();
  }

  Slice key() const override {
    assert(Valid());
    return current_->key();
  }

  Slice value() const override {
    assert(Valid());
    return current_->value();
  }

  Status status() const override {
    Status status;
    for (int i = 0; i < n_; i++) {
      status = children_[i].status();
      if (!status.ok()) {
        break;
      }
    }
    return status;
  }

 private:
  // Which direction is the iterator moving?
  enum Direction { kForward, kReverse };

  void FindLargest();

  bool MinHeapLess(const IteratorWrapper* lhs,
                   const IteratorWrapper* rhs) const;
  void BuildMinHeap();
  void MinHeapify(size_t root);
  void ReplaceMinHeapTop();

  const Comparator* comparator_;
  IteratorWrapper* children_;
  int n_;
  IteratorWrapper* current_;
  Direction direction_;
  std::vector<IteratorWrapper*> min_heap_;
};

bool MergingIterator::MinHeapLess(const IteratorWrapper* lhs,
                                  const IteratorWrapper* rhs) const {
  const int cmp = comparator_->Compare(lhs->key(), rhs->key());
  // Preserve FindSmallest()'s lowest-index tie order.
  return cmp < 0 || (cmp == 0 && lhs < rhs);
}

void MergingIterator::BuildMinHeap() {
  min_heap_.clear();
  for (int i = 0; i < n_; ++i) {
    if (children_[i].Valid()) {
      min_heap_.push_back(&children_[i]);
    }
  }
  for (size_t i = min_heap_.size() / 2; i > 0; --i) {
    MinHeapify(i - 1);
  }
  current_ = min_heap_.empty() ? nullptr : min_heap_.front();
}

void MergingIterator::MinHeapify(size_t root) {
  while (true) {
    size_t smallest = root;
    const size_t left = root * 2 + 1;
    const size_t right = left + 1;
    if (left < min_heap_.size() &&
        MinHeapLess(min_heap_[left], min_heap_[smallest])) {
      smallest = left;
    }
    if (right < min_heap_.size() &&
        MinHeapLess(min_heap_[right], min_heap_[smallest])) {
      smallest = right;
    }
    if (smallest == root) break;
    std::swap(min_heap_[root], min_heap_[smallest]);
    root = smallest;
  }
}

void MergingIterator::ReplaceMinHeapTop() {
  assert(!min_heap_.empty() && min_heap_.front() == current_);
  if (current_->Valid()) {
    MinHeapify(0);
  } else {
    min_heap_.front() = min_heap_.back();
    min_heap_.pop_back();
    if (!min_heap_.empty()) MinHeapify(0);
  }
  current_ = min_heap_.empty() ? nullptr : min_heap_.front();
}

void MergingIterator::FindLargest() {
  IteratorWrapper* largest = nullptr;
  for (int i = n_ - 1; i >= 0; i--) {
    IteratorWrapper* child = &children_[i];
    if (child->Valid()) {
      if (largest == nullptr) {
        largest = child;
      } else if (comparator_->Compare(child->key(), largest->key()) > 0) {
        largest = child;
      }
    }
  }
  current_ = largest;
}
}  // namespace

Iterator* NewMergingIterator(const Comparator* comparator, Iterator** children,
                             int n) {
  assert(n >= 0);
  if (n == 0) {
    return NewEmptyIterator();
  } else if (n == 1) {
    return children[0];
  } else {
    return new MergingIterator(comparator, children, n);
  }
}

}  // namespace leveldb
