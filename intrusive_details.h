#ifndef BIMAP_INTRUSIVE_DETAILS_H
#define BIMAP_INTRUSIVE_DETAILS_H

#include <cstddef>
#include <iterator>

namespace intrusive_details {
struct Left_tag;
struct Right_tag;

struct base_node {
  base_node() = default;

  void swap(base_node& other) {
    std::swap(other.left, left);
    std::swap(other.right, right);
    std::swap(other.parent, parent);
  }

  base_node* parent{nullptr};
  base_node* left{nullptr};
  base_node* right{nullptr};
};

template <typename T, typename Tag>
struct node : base_node {
  explicit node(T const& value_) : value(value_){};
  explicit node(T&& value_) : value(std::move(value_)){};
  T value;
};

template <typename Left, typename Right>
struct node_t : node<Left, Left_tag>, node<Right, Right_tag> {
  template <typename L, typename R>
  node_t(L&& left, R&& right)
      : node<Left, Left_tag>(std::forward<L>(left)), node<Right, Right_tag>(
                                                         std::forward<R>(
                                                             right)) {}
};

template <typename T, typename Tag, typename Compare = std::less<T>>
struct intrusive_tree : Compare {
private:
  using tree_node = base_node;

public:
  intrusive_tree() = default;
  explicit intrusive_tree(Compare&& comp) : Compare(std::move(comp)) {}
  intrusive_tree(intrusive_tree&& other) {}
  void reset_fake(base_node* other_fake) {
    fake_.swap(*other_fake);
    other_fake->parent = other_fake->left = other_fake->right = nullptr;
  }

  tree_node* begin() const {
    if (fake_.left == nullptr) {
      return end();
    }
    auto cur = fake_.left;
    while (cur->left != nullptr) {
      cur = cur->left;
    }
    return cur;
  }

  tree_node* end() const {
    return const_cast<tree_node*>(&fake_);
  }

  void insert(tree_node* x) {
    tree_node* cur = &fake_;
    while (cur != nullptr) {
      if (!compare(x, cur)) {
        if (cur->right != nullptr) {
          cur = cur->right;
        } else {
          x->parent = cur;
          cur->right = x;
          break;
        }
      } else {
        if (cur->left != nullptr) {
          cur = cur->left;
        } else {
          x->parent = cur;
          cur->left = x;
          break;
        }
      }
    }
  }

  void erase(tree_node* x) {
    tree_node* pr = x->parent;
    tree_node* tmp;
    if (x->left == nullptr && x->right == nullptr) {
      tmp = nullptr;
    } else if (x->left != nullptr && x->right == nullptr) {
      tmp = x->left;
      x->left->parent = pr;
    } else if (x->right != nullptr && x->left == nullptr) {
      tmp = x->right;
      x->right->parent = pr;
    } else {
      tmp = next(x);
      if (tmp->parent->left == tmp) {
        tmp->parent->left = tmp->right;
        if (tmp->right != nullptr) {
          tmp->right->parent = tmp->parent;
        }
      } else {
        tmp->parent->right = tmp->right;
        if (tmp->right != nullptr) {
          tmp->right->parent = tmp->parent;
        }
      }
      tmp->parent = pr;
      tmp->left = x->left;
      tmp->right = x->right;
      if (tmp->right != nullptr) {
        tmp->right->parent = tmp;
      }
      if (tmp->left != nullptr) {
        tmp->left->parent = tmp;
      }
    }
    if (pr->left == x) {
      pr->left = tmp;
    } else {
      pr->right = tmp;
    }
  }

  tree_node* find(T const& x) const {
    tree_node* lb = lower_bound(x);
    if (lb != &fake_ && lb != nullptr &&
        (!cmp(get_node(lb)->value, x) && !cmp(x, get_node(lb)->value))) {
      return lb;
    } else {
      return nullptr;
    }
  }

  tree_node* lower_bound(const T& x) const {
    tree_node* answer = nullptr;
    tree_node* cur = fake_.left;
    while (cur != nullptr) {
      if (!Compare::operator()(get_node(cur)->value, x)) {
        answer = cur;
        cur = cur->left;
      } else {
        cur = cur->right;
      }
    }
    if (answer == nullptr) {
      return const_cast<tree_node*>(&fake_);
    }
    return answer;
  }

  tree_node* upper_bound(T const& x) const {
    tree_node* lb = lower_bound(x);
    if (lb != &fake_ && lb != nullptr &&
        Compare::operator()(x, get_node(lb)->value)) {
      return lb;
    }
    tree_node* answer = next(lb);
    if (answer == nullptr) {
      return const_cast<tree_node*>(&fake_);
    }
    return answer;
  }

  static tree_node* next(tree_node* x) {
    if (x == nullptr || x->parent == nullptr) {
      return x;
    }
    if (x->right != nullptr) {
      auto cur = x->right;
      while (cur->left != nullptr) {
        cur = cur->left;
      }
      return cur;
    }
    auto* pr = x->parent;
    while (pr != nullptr && x == pr->right) {
      x = pr;
      pr = pr->parent;
    }
    return pr;
  }

  static tree_node* prev(tree_node* x) {
    if (x == nullptr) {
      return nullptr;
    }
    if (x->left != nullptr) {
      auto cur = x->left;
      while (cur->right != nullptr) {
        cur = cur->right;
      }
      return cur;
    }
    auto* pr = x->parent;
    while (pr != nullptr && x == pr->left) {
      x = pr;
      pr = pr->parent;
    }
    return pr;
  }

  void swap(intrusive_tree& other) {
    if (other.fake_.left != nullptr) {
      other.fake_.left->parent = &fake_;
    }

    if (fake_.left != nullptr) {
      fake_.left->parent = &other.fake_;
    }

    fake_.swap(other.fake_);
  }

  bool cmp(const T& a, const T& b) const {
    return Compare::operator()(a, b);
  }

  static node<T, Tag>* get_node(base_node* v) {
    return static_cast<node<T, Tag>*>(v);
  }

  void set_fake_right(tree_node* other_tree_fake) {
    fake_.right = other_tree_fake;
  }

  tree_node* get_fake() {
    return &fake_;
  }

private:
  bool compare(tree_node* first, tree_node* second) const {
    if (first == &fake_) {
      return false;
    } else if (second == &fake_) {
      return true;
    }
    return (
        Compare::operator()(get_node(first)->value, get_node(second)->value));
  }

  tree_node fake_;
};
} // namespace intrusive_details

#endif // BIMAP_INTRUSIVE_DETAILS_H
