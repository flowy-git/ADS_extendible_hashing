#ifndef ADS_SET_H
#define ADS_SET_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 63> class ADS_set {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using const_iterator = Iterator;
  using iterator = const_iterator;
  using key_equal = std::equal_to<key_type>;
  using hasher = std::hash<key_type>;

private:
  //////////   BUCKET   //////////
  struct Bucket {
    size_type local_depth;
    size_type size;
    size_type count{0};
    key_type *elements;
    Bucket(size_type depth) : local_depth(depth), size(N), count(0) {
      elements = new key_type[N];
    }
    ~Bucket() { delete[] elements; }
    Bucket(const Bucket &other)
        : local_depth(other.local_depth), size(N), count(other.count) {
      elements = new key_type[N];
      for (size_t i{0}; i < count; ++i)
        elements[i] = other.elements[i];
    }
    Bucket &operator=(const Bucket &other) {
      if (this != &other) {
        local_depth = other.local_depth;
        size = N;
        count = other.count;
        delete[] elements;
        elements = new key_type[N];
        for (size_t i{0}; i < count; ++i)
          elements[i] = other.elements[i];
      }
      return *this;
    }
    inline bool split() const { return count >= (1 * size);}
    inline bool isFull() const { return count >= (size); }
    void insert(const key_type &key) { elements[count++] = key; }
  };
  //////////   DIRECTORY   //////////
  struct Directory {
    size_type global_depth;
    Bucket **buckets;
    Directory(size_type depth, size_t val = 1) : global_depth(depth) {
      size_type size = 1 << global_depth;
      buckets = new Bucket *[size];
      if (val == 1)
        for (size_t i{0}; i < size; ++i) {
          buckets[i] = new Bucket(global_depth);
        }
    }
    ~Directory() {
      size_type size = 1 << global_depth;
      bool *deleted = new bool[size]{false}; // Track which buckets have been deleted
      for (size_type i{0}; i < size; ++i) {
        if (!deleted[i]) {
          for (size_type j = i; j < size; j += (1 << buckets[i]->local_depth)) {
            deleted[j] = true; // Mark all references to this bucket as deleted
          }
          delete buckets[i];
        }
      }
      delete[] deleted;
      delete[] buckets;
    }
  };
  //////////   INSTANZ VARS   //////////
  Directory directory;
  void split_bucket(size_type hash);
  void double_catalog();
  size_t add_feed{0};
  size_type current_size{0};
  size_type h(const key_type &key) const {
    return hasher{}(key) & ((1 << directory.global_depth) - 1);
  }
  size_type hashed(const key_type &key, size_t mod) const {
    return hasher{}(key) & ((1 << mod) - 1);
  }

public:
  // constructors
  ADS_set();
  ADS_set(std::initializer_list<key_type> ilist);
  ADS_set(const ADS_set &other);

  template <typename InputIt> ADS_set(InputIt first, InputIt last);

  ~ADS_set() {}
  // assignment
  ADS_set &operator=(const ADS_set &other);
  ADS_set &operator=(std::initializer_list<key_type> ilist);
  // inlines
  inline size_type size() const { return current_size; };
  inline bool empty() const { return current_size == 0; };
  // inserts
  void insert(std::initializer_list<key_type> ilist);
  std::pair<iterator, bool> insert(const key_type &key);
  size_t add(const key_type &key);

  template <typename InputIt> void insert(InputIt first, InputIt last);
  // remove
  void clear();
  size_type erase(const key_type &key);
  // search
  size_type count(const key_type &key) const; // PH1
  iterator find(const key_type &key) const;
  // swap
  void swap(ADS_set &other);
  // iterator
  const_iterator begin() const;
  const_iterator end() const;
  //////////   COMPARISONS   //////////
  friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
    if (lhs.current_size != rhs.current_size) {
      return false;
    }
    size_type size = 1 << lhs.directory.global_depth;
    bool *visited = new bool[size]{false};
    for (size_t i{0}; i < size; ++i) {
      if (!visited[i]) {
        for (size_t j{0}; j < lhs.directory.buckets[i]->count; ++j) {
          if (rhs.count(lhs.directory.buckets[i]->elements[j]) == 0) {
            delete[] visited;
            return false;
          }
        }
        for (size_type j = i; j < size; j += (1 << lhs.directory.buckets[i]->local_depth)) {
          if (lhs.directory.buckets[i] == lhs.directory.buckets[j])
            visited[j] = true;
        }
      }
    }
    delete[] visited;
    return true;
  };
  friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {
    return !(lhs == rhs);
  };
  //////////   DUMP   //////////
  void dump(std::ostream &o = std::cerr) const {
    o << "ADS_set dump:\n";
    o << "Global depth: " << directory.global_depth << "\n";
    size_t num_buckets = 1 << directory.global_depth;
    for (size_t i = 0; i < num_buckets; ++i) {
      o << "Bucket " << i << " Address: " << directory.buckets[i]
        << " (local depth: " << directory.buckets[i]->local_depth
        << ", size: " << directory.buckets[i]->size
        << ", count: " << directory.buckets[i]->count << "): ";
      for (size_t j = 0; j < directory.buckets[i]->count; ++j) {
        o << directory.buckets[i]->elements[j] << " - ";
      }
      o << "\n";
    }
  }
};

//////////   CONSTR & ASS   ////////////////////   CONSTR & ASS   //////////

template <typename Key, size_t N> ADS_set<Key, N>::ADS_set() : directory(1) {}
template <typename Key, size_t N>
ADS_set<Key, N>::ADS_set(std::initializer_list<key_type> ilist) : ADS_set() {
  insert(ilist);
}
template <typename Key, size_t N>
template <typename InputIt>
ADS_set<Key, N>::ADS_set(InputIt first, InputIt last) : ADS_set() {
  insert(first, last);
}
template <typename Key, size_t N>
ADS_set<Key, N>::ADS_set(const ADS_set &other)
    : ADS_set{other.begin(), other.end()} {}
template <typename Key, size_t N>
ADS_set<Key, N> &ADS_set<Key, N>::operator=(const ADS_set<Key, N> &other) {
  if (this == &other) {
    return *this;
  }
  clear();
  insert(other.begin(), other.end());
  return *this;
}
template <typename Key, size_t N>
ADS_set<Key, N> &
ADS_set<Key, N>::operator=(std::initializer_list<key_type> ilist) {
  clear();
  insert(ilist);
  return *this;
}

//////////   BUCKET MANAGEMENT   ////////////////////   BUCKET MANAGEMENT
////////////////

template <typename Key, size_t N>
void ADS_set<Key, N>::split_bucket(size_type hash) {
  Bucket *old_bucket = directory.buckets[hash];
  size_type new_local = old_bucket->local_depth + 1;
  Bucket *new_bucket = new Bucket(new_local);
  size_type mask = (1 << old_bucket->local_depth);
  for (size_type i = 0; i < static_cast<size_type>(1 << directory.global_depth);
       ++i) {
    if ((i & (mask - 1)) == (hash & (mask - 1))) {
      if (i & mask) {
        directory.buckets[i] = new_bucket;
      } else {
        directory.buckets[i] = old_bucket;
      }
    }
  }
  size_type old_count = old_bucket->count;
  old_bucket->count = 0; // Reset count for reorganizing elements
  for (size_type i = 0; i < old_count; ++i) {
    size_type new_hash = h(old_bucket->elements[i]);
    if ((new_hash & mask) == 0) {
      old_bucket->insert(old_bucket->elements[i]);
    } else {
      new_bucket->insert(old_bucket->elements[i]);
    }
  }
  old_bucket->local_depth = new_local;
}

template <typename Key, size_t N> void ADS_set<Key, N>::double_catalog() {
  size_type size = 1 << directory.global_depth;
  size_type new_size = size << 1;
  Bucket **newBuckets = new Bucket *[new_size];
  for (size_t i{0}; i < size; ++i) {
    newBuckets[i] = directory.buckets[i];
    newBuckets[i + size] = directory.buckets[i];
  }
  delete[] directory.buckets;
  directory.buckets = newBuckets;
  ++directory.global_depth;
}

//////////   INSERTS   ////////////////////   INSERTS   //////////

template <typename Key, size_t N>
size_t ADS_set<Key, N>::add(const key_type &key) {
  size_type hash = h(key);
  Bucket *bucket = directory.buckets[hash];
  for (size_t i = 0; i < bucket->count; ++i) {
    if (key_equal{}(bucket->elements[i], key)) {
      add_feed = i; // mark second location for iterator constr
      return hash;  // Key already exists, return hash for iterator constr
    }
  }
  while (bucket->split()) {
    if (bucket->local_depth == directory.global_depth) {
      double_catalog();
      hash = h(key);
      bucket = directory.buckets[hash];
    }
    split_bucket(hash);
    hash = h(key);
    bucket = directory.buckets[hash];
  }
  bucket->insert(key);
  ++current_size;
  return hash; // key inserted, return hash for interator constr
}
template <typename Key, size_t N>
void ADS_set<Key, N>::insert(std::initializer_list<key_type> ilist) {
  insert(ilist.begin(), ilist.end());
}
template <typename Key, size_t N>
template <typename InputIt>
void ADS_set<Key, N>::insert(const InputIt first, InputIt last) {
  for (auto it{first}; it != last; ++it) {
    add(*it);
  }
}
template <typename Key, size_t N>
std::pair<typename ADS_set<Key, N>::iterator, bool>
ADS_set<Key, N>::insert(const typename ADS_set<Key, N>::key_type &key) {
  size_t curr = current_size;
  size_t hash = add(key);
  hash = hashed(key, directory.buckets[hash]->local_depth);
  if (curr == current_size) {
    return {iterator(this, hash, add_feed, true), false};
  }
  return {iterator(this, hash, (directory.buckets[hash]->count) - 1, true), true};
}

//////////   REMOVE   ////////////////////   REMOVE   //////////

template <typename Key, size_t N> void ADS_set<Key, N>::clear() {
  size_type size = 1 << directory.global_depth;
  bool *cleared =
      new bool[size]{false}; // Track which buckets have been deleted
  for (size_type i{0}; i < size; ++i) {
    if (!cleared[i]) {
      for (size_type j = i; j < size;
           j += (1 << directory.buckets[i]->local_depth)) {
        cleared[j] = true; // Mark all references to this bucket as deleted
      }
      delete directory.buckets[i];
    }
  }
  delete[] cleared;
  delete[] directory.buckets;
  directory.global_depth = 1;
  directory.buckets = new Bucket *[1 << directory.global_depth];
  for (size_t i{0}; i < static_cast<size_t>(1 << directory.global_depth); ++i) {
    directory.buckets[i] = new Bucket(directory.global_depth);
  }
  current_size = 0;
}
template <typename Key, size_t N>
typename ADS_set<Key, N>::size_type
ADS_set<Key, N>::erase(const key_type &key) {
  auto elem_ptr = find(key);
  if (elem_ptr == end())
    return 0;
  size_t bucket_index = elem_ptr.get_buck();
  size_t element_index = elem_ptr.get_ele();
  auto &bucket = directory.buckets[bucket_index];
  for (size_t i{element_index}; i < bucket->count - 1; ++i) {
    bucket->elements[i] = bucket->elements[i + 1];
  }
  bucket->count--;
  current_size--;
  return 1;
}

//////////   SEARCH   ////////////////////   SEARCH   //////////

template <typename Key, size_t N>
typename ADS_set<Key, N>::size_type
ADS_set<Key, N>::count(const key_type &key) const {
  size_type hash = h(key);
  Bucket *bucket = directory.buckets[hash];
  if (bucket->count != 0) {
    for (size_t i{0}; i < bucket->count; ++i) {
      if (key_equal{}(bucket->elements[i], key)) {
        return 1;
      }
    }
  }
  return 0;
}

template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator
ADS_set<Key, N>::find(const key_type &key) const {
  size_type hash = h(key);
  hash = hashed(key, directory.buckets[hash]->local_depth);
  Bucket *bucket = directory.buckets[hash];
  if (bucket->count == 0) {
    return end();
  }
  for (size_t i{0}; i < bucket->count; ++i) {
    if (key_equal{}(bucket->elements[i], key)) {
      return const_iterator(this, hash, i, true);
    }
  }
  return end();
}

//////////   SWAPS   ////////////////////   SWAPS   //////////

template <typename Key, size_t N>
void swap(ADS_set<Key, N> &lhs, ADS_set<Key, N> &rhs) {
  lhs.swap(rhs);
}
template <typename Key, size_t N> void ADS_set<Key, N>::swap(ADS_set &other) {
  std::swap(this->current_size, other.current_size);
  std::swap(this->directory.global_depth, other.directory.global_depth);
  std::swap(this->directory.buckets, other.directory.buckets);
}

//////////   ITERATOR   ////////////////////   ITERATOR   //////////

template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator ADS_set<Key, N>::begin() const {
  return const_iterator(this);
}
template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator ADS_set<Key, N>::end() const {
  return const_iterator(this, 1 << directory.global_depth, 0);
}

template <typename Key, size_t N> class ADS_set<Key, N>::Iterator {
private:
  const ADS_set *set;
  size_t bucket_index{0};
  size_t element_index{0};
  bool noskip{false};

public:
  size_t get_buck() const { return bucket_index; }
  size_t get_ele() const { return element_index; }
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;

  Iterator() : set(nullptr), bucket_index(0), element_index(0) {}
  Iterator(const ADS_set *set, size_t bucket_index = 0,
           size_t element_index = 0, bool noskip = false)
      : set(set), bucket_index(bucket_index), element_index(element_index) {
    if (!noskip) {
      skip();
    }
  }
  ~Iterator() {}
  reference operator*() const {
    return set->directory.buckets[bucket_index]->elements[element_index];
  }
  pointer operator->() const {
    return &(set->directory.buckets[bucket_index]->elements[element_index]);
  }
  Iterator &operator++() {
    if (isAtEnd()) {
      return *this;
    }
    ++element_index;
    skip();
    if (isAtEnd()) {
      bucket_index = static_cast<size_t>(1 << set->directory.global_depth);
      element_index = 0;
    }
    return *this;
  }
  Iterator operator++(int) {
    Iterator temp = *this;
    ++(*this);
    return temp;
  }
  friend bool operator==(const Iterator &lhs, const Iterator &rhs) {
    return (lhs.set == rhs.set) && (lhs.bucket_index == rhs.bucket_index) &&
           (lhs.element_index == rhs.element_index);
  }
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs) {
    return !(lhs == rhs);
  }
  bool isAtEnd() const {
    return (bucket_index) >=
           static_cast<size_t>(1 << set->directory.global_depth);
  }
  void skip() {
    size_type directory_size = 1 << set->directory.global_depth;
    while (bucket_index < directory_size) {
      if (element_index < set->directory.buckets[bucket_index]->count) {
        bool is_lowest = true;
        size_t local_step =
            1 << set->directory.buckets[bucket_index]->local_depth;
        for (size_t idx = bucket_index % local_step; idx < bucket_index;
             idx += local_step) {
          if (set->directory.buckets[idx] ==
              set->directory.buckets[bucket_index]) {
            is_lowest = false;
            break;
          }
        }
        if (is_lowest)
          break;
        ++bucket_index;
      } else {
        ++bucket_index;
        element_index = 0;
      }
    }
    if (bucket_index >= directory_size) {
      bucket_index = directory_size;
      element_index = 0;
    }
  }
};
#endif // ADS_SET_H