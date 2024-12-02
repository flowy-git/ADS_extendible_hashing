#ifndef ADS_SET_H
#define ADS_SET_H
#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>

template <typename Key, size_t N = 64> class ADS_set {
public:
  using value_type = Key;
  using key_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using key_equal = std::equal_to<key_type>;
  using hasher = std::hash<key_type>;
  class Iterator;
  using const_iterator = ADS_set::Iterator;
  using iterator = const_iterator;

private:
  enum class Mode { free, used, freeagain };
  struct Element {
    key_type key;
    Mode mode{Mode::free};
    bool ele_equal(const Element &rhs) {
      return (this->key == rhs.key && this->mode == rhs.mode);
    }
  };
  struct Bucket {
    size_t local_depth{0};
    Element objects[N];
    size_t bucket_count{0};
    void split(ADS_set<Key, N> &ads_set);
    bool operator==(const Bucket &other) const {
      for (size_t i = 0; i < bucket_count; ++i) {
        if (!key_equal{}(objects[i].key, other.objects[i].key)) {
          return false;
        }
      }
      return true;
    }
    Bucket() {}
    Bucket(const Bucket &other) : bucket_count(other.bucket_count) {
      for (size_t i{0}; i < N; ++i)
        objects[i] = other.objects[i];
    }
  };
  struct Indexer {
    size_t num_indexes{2};
    Bucket **indexes;
    Indexer(size_t n) : num_indexes(n) {
      indexes = new Bucket *[num_indexes];
      for (size_t i = 0; i < num_indexes; ++i) {
        indexes[i] = nullptr;
      }
    }
    Indexer(const Indexer &other) : num_indexes(other.num_indexes) {
      indexes = new Bucket *[num_indexes];
      for (size_t i = 0; i < num_indexes; ++i) {
        if (other.indexes[i]) {
          indexes[i] = new Bucket(*other.indexes[i]);
        } else {
          indexes[i] = nullptr;
        }
      }
    }
    ~Indexer() { delete[] indexes; }
  };

  Indexer catalog;
  size_t index_size{2};
  size_t current_size{0};
  size_t global_depth{1};
  float max_lf{0.5};

  void add(const key_type &key);
  Element *locate(const key_type &key) const;
  size_type h(const key_type &key) const {
    return hasher{}(key) & ((1 << global_depth) - 1);
  }
  size_type hashed(const key_type &key, size_t mod) const {
    return hasher{}(key) & ((1 << mod) - 1);
  }
  void double_catalog();
public:
  ADS_set() : catalog(2), index_size(2) {
    Bucket *bucket1 = new Bucket();
    Bucket *bucket2 = new Bucket();
    catalog.indexes[0] = bucket1;
    catalog.indexes[1] = bucket2;
    catalog.indexes[0]->local_depth = 1;
    catalog.indexes[1]->local_depth = 1;
    current_size = 0;
  }
  ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} { insert(ilist); }
  template <typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{} {
    insert(first, last);
  }
  ADS_set(const ADS_set &other)
    : catalog(other.index_size), index_size(other.index_size), current_size(other.current_size), global_depth(other.global_depth) {
    for (size_t i = 0; i < index_size; ++i) {
        if (!catalog.indexes[i]) {
            catalog.indexes[i] = new Bucket(*other.catalog.indexes[i]);
        } else {
            catalog.indexes[i] = other.catalog.indexes[i];
        }
    }
}
  ~ADS_set() {
    for (size_t i = 0; i < index_size; ++i) {
        if (catalog.indexes[i] && is_lowest_pointer(i)) {
            delete catalog.indexes[i];
            catalog.indexes[i] = nullptr;
        }
    }
}

  bool is_lowest_pointer(size_t bucket_index) const {
    if (catalog.indexes[bucket_index]->local_depth < global_depth) {
      int step = (1 << catalog.indexes[bucket_index]->local_depth);
      for (int i = bucket_index - step; i >= 0; i -= step) {
        if (catalog.indexes[i] == catalog.indexes[bucket_index]) {
          return false;
        }
      }
    }
    return true;
  }
  ADS_set &operator=(const ADS_set &other) {
    if (this == &other) {
      return *this;
    }
    clear();
    for (auto it = other.begin(); it != other.end(); ++it) {
      insert(*it);
    }
    return *this;
  }
  ADS_set &operator=(std::initializer_list<key_type> ilist) {
    clear();
    insert(ilist);
    return *this;
  }
  std::pair<iterator, bool> insert(const key_type &key) {
    auto elem_ptr = locate(key);
    if (elem_ptr != nullptr) {
        return {find(key), false};
    }
    add(key);
    return {find(key), true};
}
  void clear() {
    for (size_t i = 0; i < index_size; ++i) {
      if (catalog.indexes[i] != nullptr) {
        for (size_t j = 0; j < N; ++j) {
          catalog.indexes[i]->objects[j].key = key_type{};
          catalog.indexes[i]->objects[j].mode = Mode::free;
        }
        catalog.indexes[i]->bucket_count = 0;
      }
    }
    current_size = 0;
  }
  size_type erase(const key_type &key) {
    auto it = find(key);
    if (it == end()) {
        return 0;
    }
    size_t bucket_index = it.bucket_index;
    size_t element_index = it.element_index;
    auto& bucket = catalog.indexes[bucket_index];

    for (size_t i = element_index; i + 1 < (bucket->bucket_count); ++i) {
        bucket->objects[i] = bucket->objects[i + 1];
    }
    bucket->objects[bucket->bucket_count - 1].mode = Mode::free;
    bucket->bucket_count--;
    current_size--;

    return 1;
}
  void swap(ADS_set &other) {
    Bucket **temp = other.catalog.indexes;
    other.catalog.indexes = this->catalog.indexes;
    this->catalog.indexes = temp;
    size_t temp_size = other.index_size;
    other.index_size = this->index_size;
    this->index_size = temp_size;
    size_t temp_current = other.current_size;
    other.current_size = this->current_size;
    this->current_size = temp_current;
    size_t depth_current = other.global_depth;
    other.global_depth = this->global_depth;
    this->global_depth = depth_current;
  }

  const_iterator begin() const {
    return const_iterator(catalog.indexes, index_size, global_depth);
  }
  const_iterator end() const { return const_iterator(); }

  size_type size() const { return current_size; }
  bool empty() const { return current_size == 0; }

  void insert(std::initializer_list<key_type> ilist) {
    insert(ilist.begin(), ilist.end());
  }

  template <typename InputIt> void insert(InputIt first, InputIt last);
  size_type count(const key_type &key) const { return locate(key) != nullptr; }
  void dump(std::ostream &o = std::cerr) const;
  void dumper(std::ostream &o = std::cerr) const;

  iterator find(const key_type &key) const {
    size_t hash = h(key);
    size_t local_hash = hashed(key, catalog.indexes[hash]->local_depth);
    for (size_t i{0}; i < N; ++i) {
      if(catalog.indexes[local_hash] != nullptr && catalog.indexes[local_hash]->objects[i].mode == Mode::used){
          if (key_equal{}(catalog.indexes[hash]->objects[i].key, key)) {
            return const_iterator{catalog.indexes, index_size, global_depth, local_hash, i};
          }
      }
    }
    return end();
  }
};

template <typename Key, size_t N>
typename ADS_set<Key, N>::Element *
ADS_set<Key, N>::locate(const key_type &key) const {
  size_t hash = {h(key)};
  if (catalog.indexes[hash] != nullptr) {
    for (size_t i{0}; i < N; ++i) {
      if (catalog.indexes[hash]->objects[i].mode == Mode::used)
        if (key_equal{}(catalog.indexes[hash]->objects[i].key, key)){
          return &catalog.indexes[hash]->objects[i];
        }
    }
  }
  return nullptr;
}

template <typename Key, size_t N> void ADS_set<Key, N>::double_catalog() {
  size_t new_num = index_size * 2;
  Bucket **new_indexes = new Bucket *[new_num];

  for (size_t i = 0; i < new_num; ++i) {
    new_indexes[i] = catalog.indexes[i % index_size];
  }

  delete[] catalog.indexes;
  catalog.indexes = new_indexes;
  index_size = new_num;
  ++global_depth;
}

template <typename Key, size_t N>
void ADS_set<Key, N>::Bucket::split(ADS_set<Key, N> &ads_set) {
  size_t pointer_count = 0;
  for (size_t i = 0; i < ads_set.index_size; ++i) {
    if (ads_set.catalog.indexes[i] == this) {
      ++pointer_count;
    }
    if (pointer_count > 2)
      break;
  }
  if (pointer_count < 2) {
    ads_set.double_catalog();
    this->split(ads_set);
    return;
  }
  key_type *helper = new key_type[N];
  size_t counter{0};
  for (size_t i{0}; i < N; ++i) {
    if (this->objects[i].mode == Mode::used) {
      helper[counter] = this->objects[i].key;
      ++counter;
    }
  }
  Bucket *new_bucket = new Bucket();
  new_bucket->local_depth = ++this->local_depth;
  size_t mask = 1 << (this->local_depth - 1);

  this->bucket_count = 0;
  for (size_t i = 0; i < N; ++i) {
    this->objects[i].mode = Mode::free;
  }
  ads_set.current_size -= counter;

  for (size_t i{0}; i < counter; ++i) {
    auto element = helper[i];
    size_t hash_value = ads_set.hashed(element, this->local_depth);
    if (hash_value & mask) {
      new_bucket->objects[new_bucket->bucket_count++] = {element, Mode::used};
    } else {
      this->objects[this->bucket_count++] = {element, Mode::used};
    }
    ++ads_set.current_size;
  }
  for (size_t i = 0; i < ads_set.index_size; ++i) {
    if (ads_set.catalog.indexes[i] == this) {
      if (i & mask) {
        ads_set.catalog.indexes[i] = new_bucket;
      }
    }
  }
  delete[] helper;
}

template <typename Key, size_t N>
void ADS_set<Key, N>::add(const key_type &key) {
  size_t hash{h(key)};

  for (size_t i = 0; i < N; ++i) {
        if (catalog.indexes[hash]->objects[i].mode == Mode::used 
            && key_equal{}(catalog.indexes[hash]->objects[i].key, key)) {
            return;
        }
    }

  if (catalog.indexes[hash]->bucket_count >= N * max_lf) {
    catalog.indexes[hash]->split(*this);
    add(key);
    return;
  }
  for (size_t i = 0; i < N; ++i) {
        if (catalog.indexes[hash]->objects[i].mode == Mode::free) {
            catalog.indexes[hash]->objects[i].key = key;
            catalog.indexes[hash]->objects[i].mode = Mode::used;
            catalog.indexes[hash]->bucket_count += 1;
            ++current_size;
            return;
        }
    }
}

template <typename Key, size_t N>
template <typename InputIt>
void ADS_set<Key, N>::insert(InputIt first, InputIt last) {
  for (auto it{first}; it != last; ++it) {
    if (it != last)
      if (!count(*it)) {
        add(*it);
      }
  }
}

template <typename Key, size_t N>
void ADS_set<Key, N>::dump(std::ostream &o) const {
  o << "glodepth = " << global_depth << "\n";
  o << "indexer_size = " << index_size << ", current_size = " << current_size
    << "\n";
  for (size_t i{0}; i < index_size; ++i) {
    if (catalog.indexes[i] != nullptr) {
      o << "\nindexer" << i << ": ";
      o << "bucket_count" << catalog.indexes[i]->bucket_count << " : ";
      o << "Bucket depth" << catalog.indexes[i]->local_depth << "\n";
      for (size_t a{0}; a < N; ++a) {
        switch (catalog.indexes[i]->objects[a].mode) {
        case Mode::free:
          o << "-nil";
          break;
        case Mode::used:
          o << "" << catalog.indexes[i]->objects[a].key << " ";
          break;
        case Mode::freeagain:
          o << "-nil";
          break;
        }
      }
    }
  }
}

template <typename Key, size_t N> class ADS_set<Key, N>::Iterator {
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;
  size_t bucket_index;
  size_t element_index;

  Iterator()
      : bucket_index{0}, element_index{0}, indexes{nullptr}, index_size{0}, global_depth{0}, p{nullptr} {}

  explicit Iterator(Bucket **bucketlist, size_t index_size, size_t global_depth)
      : bucket_index{0}, element_index{0}, indexes{bucketlist}, index_size{index_size}, global_depth{global_depth}, p{nullptr} {
    p = &indexes[bucket_index]->objects[element_index].key;
    initialize();
  }

  Iterator(Bucket **bucketlist, size_t index_size, size_t global_depth,
           size_t bucket_index, size_t element_index)
      : bucket_index{bucket_index}, element_index{element_index}, indexes{bucketlist}, index_size{index_size}, global_depth{global_depth}, p{nullptr} {
    p = &indexes[bucket_index]->objects[element_index].key;
    initialize();
  }

  reference operator*() const { return *p; }
  pointer operator->() const { return p; }

  Iterator &operator++() {
    if (p) {
      ++element_index;
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

private:
  Bucket **indexes;
  size_t index_size;
  size_t global_depth;
  pointer p;

    void initialize() {
        while (bucket_index < index_size) {
            if (indexes[bucket_index] != nullptr &&
                is_lowest_pointer(bucket_index) &&
                element_index < indexes[bucket_index]->bucket_count &&
                indexes[bucket_index]->objects[element_index].mode == Mode::used) {
                p = &indexes[bucket_index]->objects[element_index].key;
                return;
            }
            ++bucket_index;
            element_index = 0;
        }
        p = nullptr;
    }

void skip() {
    while (bucket_index < index_size) {
        if (indexes[bucket_index] == nullptr ||
            element_index >= indexes[bucket_index]->bucket_count ||
            indexes[bucket_index]->objects[element_index].mode != Mode::used) {
            ++bucket_index;
            element_index = 0;
        } else {
            if (is_lowest_pointer(bucket_index)) {
                p = &indexes[bucket_index]->objects[element_index].key;
                return;
            } else {
                ++bucket_index;
                element_index = 0;
            }
        }
    }
    p = nullptr;
}

  bool is_lowest_pointer(size_t bucket_index) const {
    if (indexes[bucket_index]->local_depth < global_depth) {
      int step = (1 << indexes[bucket_index]->local_depth);
      for (int i = bucket_index - step; i >= 0; i -= step) {
        if (indexes[i] == indexes[bucket_index]) {
          return false;
        }
      }
    }
    return true;
  }
};

template <typename Key, size_t N>
bool operator==(const ADS_set<Key, N> &lhs, const ADS_set<Key, N> &rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (auto c : lhs)
    if (!rhs.count(c))
      return false;
  return true;
}

template <typename Key, size_t N>
bool operator!=(const ADS_set<Key, N> &lhs, const ADS_set<Key, N> &rhs) {
  return !(lhs == rhs);
}

template <typename Key, size_t N>
void swap(ADS_set<Key, N> &lhs, ADS_set<Key, N> &rhs) {
  lhs.swap(rhs);
}

#endif // ADS_SET_H
