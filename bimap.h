#pragma once

#include "intrusive_details.h"
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
private:
  using left_t = Left;
  using right_t = Right;
  using node_t = intrusive_details::node_t<Left, Right>;
  using left_tree_ =
      intrusive_details::intrusive_tree<Left, intrusive_details::Left_tag,
                                        CompareLeft>;
  using right_tree_ =
      intrusive_details::intrusive_tree<Right, intrusive_details::Right_tag,
                                        CompareRight>;
  using left_node = intrusive_details::node<Left, intrusive_details::Left_tag>;
  using right_node =
      intrusive_details::node<Right, intrusive_details::Right_tag>;

  left_tree_ left_tree;
  right_tree_ right_tree;
  size_t size_{0};

public:
  template <typename T, typename T_tag, typename Compare_T, typename T_flip,
            typename T_flip_tag, typename Compare_T_flip>
  struct bimap_iterator {
    using node_ptr = intrusive_details::base_node;
    using tree = intrusive_details::intrusive_tree<T, T_tag, Compare_T>;

    explicit bimap_iterator(node_ptr* ptr_) : ptr(ptr_) {}

    T const& operator*() const {
      return tree::get_node(ptr)->value;
    }

    right_t const* operator->() const {
      return &(tree::get_node(ptr)->value);
    }

    bimap_iterator& operator++() {
      ptr = tree::next(ptr);
      return *this;
    }

    bimap_iterator operator++(int) {
      bimap_iterator tmp(*this);
      ++(*this);
      return tmp;
    }

    bimap_iterator& operator--() {
      ptr = tree::prev(ptr);
      return *this;
    }

    bimap_iterator operator--(int) {
      bimap_iterator tmp(*this);
      --(*this);
      return tmp;
    }

    bimap_iterator<T_flip, T_flip_tag, Compare_T_flip, T, T_tag, Compare_T>
    flip() const {
      if (ptr->parent == nullptr) {
        return bimap_iterator<T_flip, T_flip_tag, Compare_T_flip, T, T_tag,
                              Compare_T>(ptr->right);
      } else {
        return bimap_iterator<T_flip, T_flip_tag, Compare_T_flip, T, T_tag,
                              Compare_T>(
            static_cast<intrusive_details::node<T_flip, T_flip_tag>*>(
                static_cast<node_t*>(tree::get_node(ptr))));
      }
    }

    friend bool operator==(bimap_iterator const& a,
                           bimap_iterator const& b) noexcept {
      return a.ptr == b.ptr;
    }

    friend bool operator!=(bimap_iterator const& a,
                           bimap_iterator const& b) noexcept {
      return !(a == b);
    }

    intrusive_details::node<T, T_tag>* get_ptr() {
      return tree::get_node(ptr);
    }

  private:
    friend bimap;
    node_ptr* ptr;
  };

  using left_iterator =
      bimap_iterator<left_t, intrusive_details::Left_tag, CompareLeft, right_t,
                     intrusive_details::Right_tag, CompareRight>;
  using right_iterator =
      bimap_iterator<right_t, intrusive_details::Right_tag, CompareRight,
                     left_t, intrusive_details::Left_tag, CompareLeft>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : left_tree(std::move(compare_left)),
        right_tree(std::move(compare_right)) {
    left_tree.set_fake_right(right_tree.get_fake());
    right_tree.set_fake_right(left_tree.get_fake());
  }

  // Конструкторы от других и присваивания
  bimap(bimap const& other) {
    try {
      for (auto it = other.begin_left(); it != other.end_left(); ++it) {
        insert(*it, *it.flip());
      }
      size_ = other.size_;
    } catch (...) {
      erase_left(begin_left(), end_left());
      throw;
    }
  }

  bimap(bimap&& other) noexcept
      : size_(other.size_), left_tree(std::move(other.left_tree)),
        right_tree(std::move(other.right_tree)) {}

  bimap& operator=(bimap const& other) {
    if (this != &other) {
      bimap(other).swap(*this);
    }
    return *this;
  }

  bimap& operator=(bimap&& other) noexcept {
    if (this != &other) {
      bimap(std::move(other)).swap(*this);
    }
    return *this;
  }

  void swap(bimap& other) {
    left_tree.swap(other.left_tree);
    right_tree.swap(other.right_tree);
    std::swap(size_, other.size_);
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    erase_left(begin_left(), end_left());
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_(left, right);
  }

  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_(left, std::move(right));
  }

  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_(std::move(left), right);
  }

  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_(std::move(left), std::move(right));
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    return erase_(it);
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    auto tmp = find_left(left);
    if (tmp != end_left()) {
      erase_(tmp);
      return true;
    } else {
      return false;
    }
  }

  right_iterator erase_right(right_iterator it) {
    return erase_(it);
  }

  bool erase_right(right_t const& right) {
    auto tmp = find_right(right);
    if (tmp != end_right()) {
      erase_(tmp);
      return true;
    } else {
      return false;
    }
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    return erase_first_last(first, last);
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    return erase_first_last(first, last);
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    auto tmp = left_tree.find(left);
    if (tmp != nullptr) {
      return left_iterator(tmp);
    } else {
      return end_left();
    }
  }
  right_iterator find_right(right_t const& right) const {
    auto tmp = right_tree.find(right);
    if (tmp != nullptr) {
      return right_iterator(tmp);
    } else {
      return end_right();
    }
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    auto it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range("out_of_range");
    }
    return *it.flip();
  }

  left_t const& at_right(right_t const& key) const {
    auto it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range("out_of_range");
    }
    return *it.flip();
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename T, typename = std::enable_if_t<
                            std::is_same_v<T, right_t> &&
                            std::is_default_constructible_v<left_t>>>
  right_t const& at_left_or_default(T const& key) {
    auto left_it = find_left(key);
    if (left_it != end_left()) {
      return *(left_it.flip());
    }
    auto right_it = find_right(right_t());
    if (right_it != end_right()) {
      left_tree.erase(get_left_node(get_node_t(right_it.get_ptr())));
      auto left_node_ = right_it.flip().get_ptr();
      left_node_->value = key;
      left_tree.insert(get_left_node(get_node_t(right_it.get_ptr())));
      return *right_it;
    }
    return *(insert(key, right_t()).flip());
  }

  template <typename T, typename = std::enable_if_t<
                            std::is_same_v<T, left_t> &&
                            std::is_default_constructible_v<right_t>>>
  left_t const& at_right_or_default(T const& key) {
    auto right_it = find_right(key);
    if (right_it != end_right()) {
      return *(right_it.flip());
    }
    auto left_it = find_left(left_t());
    if (left_it != end_left()) {
      right_tree.erase(get_right_node(get_node_t(left_it.get_ptr())));
      auto right_node_ = left_it.flip().get_ptr();
      right_node_->value = key;
      right_tree.insert(get_right_node(get_node_t(left_it.get_ptr())));
      return *left_it;
    }
    return *insert(left_t(), key);
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    return left_iterator(left_tree.lower_bound(left));
  }
  left_iterator upper_bound_left(const left_t& left) const {
    return left_iterator(left_tree.upper_bound(left));
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_iterator(right_tree.lower_bound(right));
  }
  right_iterator upper_bound_right(const right_t& right) const {
    return right_iterator(right_tree.upper_bound(right));
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return left_iterator(left_tree.begin());
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_iterator(left_tree.end());
  }
  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_iterator(right_tree.begin());
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_iterator(right_tree.end());
  }
  // Проверка на пустоту
  bool empty() const {
    return size_ == 0;
  }
  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return size_;
  }

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
    if (a.size_ != b.size_) {
      return false;
    }
    auto it_a = a.begin_left();
    auto it_b = b.begin_left();
    while (it_a != a.end_left()) {
      if (a.left_tree.cmp(*it_a, *it_b) || a.left_tree.cmp(*it_b, *it_a) ||
          a.right_tree.cmp(*it_a.flip(), *it_b.flip()) ||
          a.right_tree.cmp(*it_b.flip(), *it_a.flip())) {
        return false;
      }
      it_a++;
      it_b++;
    }
    return true;
  }

  friend bool operator!=(bimap const& a, bimap const& b) {
    return !(a == b);
  }

private:
  node_t* get_node_t(left_node* v) {
    return static_cast<node_t*>(v);
  }

  node_t* get_node_t(right_node* v) {
    return static_cast<node_t*>(v);
  }

  left_node* get_left_node(node_t* v) {
    return static_cast<left_node*>(v);
  }

  right_node* get_right_node(node_t* v) {
    return static_cast<right_node*>(v);
  }

  template <typename L, typename R>
  left_iterator insert_(L&& left, R&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    auto* new_node = new node_t(std::forward<L>(left), std::forward<R>(right));
    left_tree.insert(get_left_node(new_node));
    right_tree.insert(get_right_node(new_node));
    size_++;
    return left_iterator(get_left_node(new_node));
  }

  template <typename Iter>
  Iter erase_(Iter it) {
    if (it.ptr == nullptr) {
      return it;
    }
    Iter res(it.ptr);
    res++;
    node_t* tmp = get_node_t(it.get_ptr());
    left_tree.erase(get_left_node(tmp));
    right_tree.erase(get_right_node(tmp));
    delete tmp;
    size_--;
    return res;
  }

  template <typename T>
  T erase_first_last(T first, T last) {
    while (first != last) {
      first = erase_(first);
    }
    return last;
  }
};
