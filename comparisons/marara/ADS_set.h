#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>


    // NOT MY WORK; WORK OF MARA CIMPEAN
    // UPLOADED ONLY FOR PERFORMANCE REVIEW COMPARISON
    // NOT INTENDED FOR FINAL VERSION
template <typename Key, size_t N = 128>
class ADS_set {
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using const_iterator = Iterator;
    using iterator = Iterator;
    using key_equal = std::equal_to<key_type>;
    using hasher = std::hash<key_type>;
private:
  size_t current_size;

  struct Bucket {
    key_type values[N];
    size_t size = 0;
    size_t local_depth = 0;

    bool insert(const key_type& key) {
      if (size >= N) return false;
      values[size++] = key;
      return true;
    }
    bool contains_value(const key_type& key) const {
      for (size_t i = 0; i < size; ++i) {
        if (key_equal{}(values[i], key)) {
          return true;
        }
      }
      return false;
    }
  };

  size_t global_depth = 1; //the same as index_size or
  Bucket** buckets;

  size_t hash(const key_type& key, size_t depth) const {
    return hasher{}(key) & ((1 << depth) - 1);
  }

  void grow() {
    size_t old_size = 1 << global_depth;
    size_t new_size = old_size * 2;
    Bucket** new_buckets = new Bucket*[new_size];
    for (size_t i = 0; i < old_size; ++i) {
      new_buckets[i] = buckets[i];
      new_buckets[i + old_size] = buckets[i];
    }
    delete[] buckets;
    buckets = new_buckets;
    ++global_depth;
  }

  void split_bucket(size_t index) {
    Bucket* old_bucket = buckets[index];
    size_t local_depth = old_bucket->local_depth;
    if (local_depth == global_depth) {
      grow();
    }
    Bucket* new_bucket = new Bucket;
    new_bucket->local_depth = ++old_bucket->local_depth;
    size_t mask = 1 << (new_bucket->local_depth - 1);
    key_type temp_values[N];
    size_t temp_size = old_bucket->size;
    for (size_t i = 0; i < temp_size; i++)
      temp_values[i] = old_bucket->values[i];
    old_bucket->size = 0;
    for (size_t i = 0; i < temp_size; ++i) {
      const key_type& key = temp_values[i];
      size_t hash_value = hash(key, global_depth);
      if (hash_value & mask) 
        new_bucket->insert(key);
      else
        old_bucket->insert(key);
    }
    /*if (static_cast<int>(index + mask) < (1 << global_depth) && buckets[index + mask] == old_bucket) {
      buckets[index + mask] = new_bucket;
    }
    else if (buckets[index] == old_bucket){
      buckets[index] = new_bucket;
    }*/
    for (int i = 0; i < (1 << global_depth); ++i)
      if ((i & mask) == mask && buckets[i] == old_bucket) {
        buckets[i] = new_bucket;
      }
  }

  bool add_element(const key_type& key) {
    size_t index = hash(key, global_depth);
    if (buckets[index]->contains_value(key)) {
      return false;
    }
    if (buckets[index]->size < N) {
      buckets[index]->insert(key);
      return true;
    }
    split_bucket(index);
    return add_element(key);
  }
  
  bool contains(const key_type& key) const {
    size_t index = hash(key, global_depth);
    
    return buckets[index]->contains_value(key);
  }

public: 
  // PH1
  ADS_set(){ 
    current_size = 0;
    buckets = new Bucket*[2];
    for (size_t i = 0; i < 2; ++i) {  
      buckets[i] = new Bucket;
      buckets[i]->local_depth = 1;
    }
  }
  // PH1 /s
  ADS_set(std::initializer_list<key_type> ilist) : ADS_set{std::begin(ilist), std::end(ilist)} {}
  // PH1 /s
  template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{} {
    insert(first, last);
  }
  // PH2 - optimize
  ADS_set(const ADS_set &other): ADS_set{other.begin(), other.end()} {}
  // PH2
  ~ADS_set() {
    size_t size = 1 << global_depth;
    for (int i = size - 1; i >= 0; i--) {
      if (buckets[i]->local_depth == global_depth)
        delete buckets[i];
      else { 
        bool ok = 1;
        if (i - (1 << buckets[i]->local_depth) >= 0)
          ok = 0;
        if (ok)
          delete buckets[i];
      }
    }
    delete[] buckets;
  }
  // PH2 /s
  ADS_set &operator=(const ADS_set &other) {
    if (this == &other)
      return *this;
    ADS_set temp{other};
    swap(temp);
    return *this;
  }
  // PH2 /s
  ADS_set &operator=(std::initializer_list<key_type> ilist) {
    ADS_set temp{ilist};
    swap(temp);
    return *this;
  }
  // PH1 /s
  size_type size() const {
    return current_size;
  }
  // PH1 /s
  bool empty() const {
    return current_size == 0;
  }
  // PH1 /s
  void insert(std::initializer_list<key_type> ilist) {
    insert(std::begin(ilist), std::end(ilist));
  }
  // PH2 - done?
  std::pair<iterator,bool> insert(const key_type &key) {
    if (add_element(key)) {
      ++current_size;
      return {find(key), true};
    }
    else {
      return {find(key), false};
    }
    return {end(), false};
  }
  // PH1
  template<typename InputIt> void insert(InputIt first, InputIt last) {
    for (auto it = first; it != last; ++it) {
      if (add_element(*it)) {
        ++current_size;
      }
    }
  }
  // PH2 /s
  void clear() {
    ADS_set temp;
    swap(temp);
  }
  // PH2 - needs optimization
  size_type erase(const key_type &key) {
    size_t index = hash(key, global_depth);
    for (size_t i = 0; i < buckets[index]->size; i++) {
      if (key_equal{}(buckets[index]->values[i], key)) {
        for (size_t j = i + 1; j < buckets[index]->size; j++) {
          buckets[index]->values[j - 1] = buckets[index]->values[j];
        }
        buckets[index]->size--;
        current_size--;
        return 1;
      }
    }
    return 0;
  }
  // PH1
  size_type count(const key_type& key) const {
    return contains(key) ? 1 : 0;
  }
  // PH2 - needs finishing up
  iterator find(const key_type &key) const {
    size_t bindex = hash(key, global_depth);
    bindex = hash(key, buckets[bindex]->local_depth);
    for (size_t vindex = 0; vindex < buckets[bindex]->size; ++vindex) {
      if (key_equal{}(buckets[bindex]->values[vindex], key)) {
        return iterator{buckets, bindex, vindex, global_depth};
      }
    }
    return end();
  }
  // PH2
  void swap(ADS_set &other) {
    std::swap(current_size, other.current_size);
    std::swap(buckets, other.buckets);
    std::swap(global_depth, other.global_depth);
  }
  // PH2
  const_iterator begin() const {
    return const_iterator{buckets, global_depth};
  }
  // PH2
  const_iterator end() const {
    return const_iterator{};
  }
  // PH1
  void dump(std::ostream& o = std::cerr) const {
    o << "ExtendibleHashing<" << typeid(Key).name() << ", " << N << ">, depth = " << global_depth << "\n";
    o << "current size = " << current_size << "\n";
    size_t size = 1 << global_depth;
    for (size_t i = 0; i < size; ++i) {
      o << i << ": ";
      for (size_t j = 0; j < buckets[i]->size; ++j) {
        o << buckets[i]->values[j] << " ";
      }
      o << '\n';
    }
  }
  // PH2
  friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
    if (lhs.size() != rhs.size())
      return false;
    for (const auto& element: lhs) 
      if (rhs.count(element) == 0)
        return false;
    return true;
  }
  //PH2
  friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {
    return !(lhs == rhs);
  }

};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;
private:
  Bucket** buckets;
  size_t bucket_index;
  size_t value_index;
  size_t depth;
  pointer p;
  void skip() {
    while (static_cast<int>(bucket_index) < (1 << depth)) {
      if (!buckets[bucket_index] || value_index >= buckets[bucket_index]->size) {
        value_index = 0;
        bucket_index++;
      } 
      else {
        bool lowest = true;
        if (buckets[bucket_index]->local_depth < depth) {
          int step = 1 << buckets[bucket_index]->local_depth;
          for (int i = bucket_index - step; i >= 0 && lowest; i -= step) {
            if (buckets[i] == buckets[bucket_index]) {
              lowest = false;
              value_index = 0;
              bucket_index++;
            }
          }
        }
        if (lowest) {
          p = &buckets[bucket_index]->values[value_index];
          return;
        }
      }
    }
    p = nullptr;
  }
public:
  explicit Iterator(Bucket** b, size_t depth) : 
  buckets{b}, bucket_index{0}, value_index{0}, depth{depth}, p{&buckets[0]->values[0]} {
    skip();
  }
  Iterator(Bucket** b, size_t bindex, size_t vindex, size_t depth) : 
    buckets{b}, bucket_index{bindex}, value_index{vindex}, depth{depth}, p{&buckets[bindex]->values[vindex]} {
      skip();
  }
  Iterator(): buckets{nullptr}, bucket_index{0}, value_index{0}, depth{0}, p{nullptr} {}
  reference operator*() const {
    return *p;
  }
  pointer operator->() const {
    return p;
  }
  Iterator& operator++() {
    if (p) {
      ++value_index;
      skip();
    }
    return *this;
  }
  Iterator operator++(int) {
    auto temp = *this;
    ++(*this);
    return temp;
  }
  friend bool operator==(const Iterator &lhs, const Iterator &rhs) {
    return lhs.p == rhs.p;
  }
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs) {
    return !(lhs == rhs);
  }
};



template <typename Key, size_t N>
void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }

#endif