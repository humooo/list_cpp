#pragma once

#include "iostream"

template <typename T, typename Allocator = std::allocator<T>>
class List {
  private:
  struct Node;

  public:
  using traits = std::allocator_traits<Allocator>;
  using node_allocator = typename traits::template rebind_alloc<Node>;
  using node_traits = std::allocator_traits<node_allocator>;
  using value_type = T;
  using allocator_type =
      typename std::allocator_traits<Allocator>::allocator_type;
  List(const Allocator& alloc = Allocator())
      : alloc_(static_cast<node_allocator>(alloc)) {
    init();
  }
  List(size_t count, const T& value, const Allocator& alloc = Allocator())
      : alloc_(static_cast<node_allocator>(alloc)) {
    init();
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back(value);
      }
    } catch (...) {
      destroy();
      throw;
    }
  }
  explicit List(size_t count, const Allocator& alloc = Allocator())
      : alloc_(static_cast<node_allocator>(alloc)) {
    init();
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back();
      }
    } catch (...) {
      destroy();
      throw;
    }
  }
  List(const List& other)
      : alloc_(traits::select_on_container_copy_construction(other.alloc_)) {
    init();
    Node* cur = other.root_->next;
    try {
      for (size_t i = 0; i < other.size_; ++i, cur = cur->next) {
        push_back(cur->value);
      }
    } catch (...) {
      destroy();
      throw;
    }
  }
  List(std::initializer_list<T> initializer,
       const Allocator& alloc = Allocator())
      : alloc_(static_cast<node_allocator>(alloc)) {
    init();
    try {
      for (auto it = initializer.begin(); it != initializer.end(); ++it) {
        push_back(*it);
      }
    } catch (...) {
      destroy();
      throw;
    }
  }
  List& operator=(const List& other) {
    if (this == &other) {
      return *this;
    }
    List tmp(other);
    if (node_traits::propagate_on_container_copy_assignment::value and
        tmp.alloc_ != other.alloc_) {
      tmp.alloc_ = other.alloc_;
    }
    std::swap(alloc_, tmp.alloc_);
    std::swap(root_, tmp.root_);
    std::swap(size_, tmp.size_);
    return *this;
  }
  node_allocator get_allocator() { return alloc_; }
  T& front() { return root_->next->value; }
  const T& front() const { return root_->next->value; }
  T& back() { return root_->prev->value; }
  const T& back() const { return root_->prev->value; }
  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  void push_back() { insert(root_->prev); }
  void push_back(const T& value) { insert(root_->prev, value); }
  void push_back(T&& value) { insert(root_->prev, std::forward<T>(value)); }
  void push_front(const T& value) { insert(root_, value); }
  void push_front(T&& value) { insert(root_, std::forward<T>(value)); }
  void pop_back() { del(root_->prev); }
  void pop_front() { del(root_->next); }
  ~List() { destroy(); }
  template <bool IsConst>
  class ListIterator {
    public:
    using difference_type = std::ptrdiff_t;
    using value_type = typename std::conditional<IsConst, const T, T>::type;
    using pointer = typename std::conditional<IsConst, const T*, T*>::type;
    using reference = typename std::conditional<IsConst, const T&, T&>::type;
    using iterator_category = std::bidirectional_iterator_tag;

    explicit operator ListIterator<true>() { return ListIterator<true>(node_); }
    explicit ListIterator(Node* node) : node_(node) {}
    ListIterator& operator++() {
      node_ = node_->next;
      return *this;
    }
    ListIterator operator++(int) {
      ListIterator res = *this;
      node_ = node_->next;
      return res;
    }
    ListIterator& operator--() {
      node_ = node_->prev;
      return *this;
    }
    ListIterator operator--(int) {
      ListIterator res = *this;
      node_ = node_->prev;
      return res;
    }
    bool operator==(const ListIterator& other) const {
      return node_ == other.node_;
    }
    bool operator!=(const ListIterator& other) const {
      return node_ != other.node_;
    }
    reference operator*() { return node_->value; }
    pointer operator->() { return &(node_->value); }

    private:
    Node* node_;
  };
  using iterator = ListIterator<false>;
  using const_iterator = ListIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(root_->next); }
  const_iterator begin() const { return const_iterator(root_->next); }
  const_iterator cbegin() const { return const_iterator(root_->next); }
  iterator end() { return iterator(root_); }
  const_iterator end() const { return const_iterator(root_); }
  const_iterator cend() const { return const_iterator(root_); }
  reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return std::make_reverse_iterator(end());
  }
  reverse_iterator rend() { return std::make_reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return std::make_reverse_iterator(begin());
  }
  const_reverse_iterator crbegin() const {
    return std::make_reverse_iterator(cend());
  }
  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(cbegin());
  }

  private:
  Node* root_;
  node_allocator alloc_;
  size_t size_;

  struct Node {
    T value;
    Node* next;
    Node* prev;
    explicit Node(Node* prev = nullptr, Node* next = nullptr)
        : value(T()), prev(prev), next(next) {}
    explicit Node(const T& value, Node* prev = nullptr, Node* next = nullptr)
        : value(value), prev(prev), next(next) {}
  };

  void init() {
    root_ = node_traits::allocate(alloc_, 1);
    root_->next = root_->prev = root_;
    size_ = 0;
  }

  void insert(Node* node) {
    Node* tmp = node_traits::allocate(alloc_, 1);
    try {
      node_traits::construct(alloc_, tmp, node, node->next);
    } catch (...) {
      node_traits::deallocate(alloc_, tmp, 1);
      throw;
    }
    node->next = tmp;
    tmp->next->prev = tmp;
    ++size_;
  }

  void insert(Node* node, const T& value) {
    Node* tmp = node_traits::allocate(alloc_, 1);
    try {
      node_traits::construct(alloc_, tmp, value, node, node->next);
    } catch (...) {
      node_traits::deallocate(alloc_, tmp, 1);
      throw;
    }
    node->next = tmp;
    tmp->next->prev = tmp;
    ++size_;
  }

  void insert(Node* node, Node* add) {
    add->next = node->next;
    node->next = add;
    add->next->prev = add;
    ++size_;
  }
  void del(Node* node) {
    if (node == root_) {
      return;
    }
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node_traits::destroy(alloc_, node);
    node_traits::deallocate(alloc_, node, 1);
    --size_;
  }
  void destroy() {
    Node* curr = root_->next;
    while (curr != root_) {
      Node* tmp = curr;
      curr = curr->next;
      node_traits::destroy(alloc_, tmp);
      node_traits::deallocate(alloc_, tmp, 1);
    }
    node_traits::deallocate(alloc_, root_, 1);
    size_ = 0;
  }
};