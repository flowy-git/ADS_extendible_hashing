#ifndef ADS_SET_H
#define ADS_SET_H
#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <list>

template <typename Key, size_t N = 89>
class ADS_set {
public:
  using value_type = Key;
  using key_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using key_equal = std::equal_to<key_type>;                       // Hashing
  using hasher = std::hash<key_type>;                              // Hashing
  class Iterator;
  using const_iterator = ADS_set::Iterator;
  using iterator = const_iterator;

private:
  enum class Mode {free, used, freeagain};
  struct Element {
    key_type key;
    Mode mode {Mode::free};
    bool ele_equal(const Element &rhs){
      return (this->key == rhs.key && this->mode == rhs.mode);
    }
  };
  struct Bucket {
    Element objects[N];
    size_t bucket_count{0};
    void split(ADS_set<Key, N>& ads_set);
    bool operator==(const Bucket& other) {
      return other.objects == (*this).objects;
    }
    Bucket(){}
    Bucket(const Bucket& other) : bucket_count(other.bucket_count){
      for(size_t i{0}; i < N; ++i)
        objects[i] = other.objects[i];
    }
  };
  struct Indexer{
    size_t num_indexes{1};
    //size_t bucket_counter{1};
    Bucket** indexes;
    //Bucket** bucketlist;
    Indexer(size_t n) : num_indexes(n){ //, bucket_counter(n) {
      indexes = new Bucket*[num_indexes];
      //bucketlist = new Bucket*[bucket_counter];
      }
    Indexer(const Indexer &other) : num_indexes(other.num_indexes){//, bucket_counter(other.bucket_counter){
      indexes = new Bucket*[num_indexes];
      //bucketlist = new Bucket*[bucket_counter];
      for (size_t i = 0; i < num_indexes; ++i) {
        indexes[i] = other.indexes[i] ? new Bucket(*other.indexes[i]) : nullptr;
      }
      /*for (size_t i = 0; i < bucket_counter; ++i) {
        bucketlist[i] = other.bucketlist[i] ? new Bucket(*other.bucketlist[i]) : nullptr;
      }*/
    }
    
    /*void addtoBucketList(){
      size_t newcount = bucket_counter + 1;
      Bucket** newBucketlist = new Bucket*[newcount];
        for(size_t i{0}; i<bucket_counter; ++i)
          newBucketlist[i] = bucketlist[i];
      delete[] bucketlist;
      bucketlist = newBucketlist;
      bucket_counter = newcount;
    }*/
   ~Indexer() {
      delete[] indexes;
      //delete[] bucketlist;
    }
  };
  
  Indexer catalog;
  size_t index_size{1};
  size_t current_size {0};
  float max_lf {0.7};
  
  void add(const key_type &key);
  bool first_split{false};
  Element *locate(const key_type &key) const;
  size_type h(const key_type &key) const { return hasher{}(key) % (index_size); }
  size_type g(const key_type &) const { return 1; }
  void double_catalog();

public:
  ADS_set(): catalog(1), index_size(1){
    Bucket* bucket1 = new Bucket();
    //catalog.bucketlist[0] = bucket1;
    catalog.indexes[0] = bucket1;
    current_size = 0;
    }
  ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} {insert(ilist);}
  template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{} {insert(first, last);}
  ///PHASE 2 SHITE///
  ADS_set(const ADS_set &other) : catalog(other.catalog), index_size(other.index_size), current_size(other.current_size){

  }
  ~ADS_set() {
    for(size_t i{0}; i < index_size; ++i){
      if(catalog.indexes[i] != nullptr){
        for(size_t j{0}; j < index_size; ++j)
          if(i!=j && catalog.indexes[i] == catalog.indexes[j])
            catalog.indexes[j] = nullptr;
      }
    }

    for(size_t i{0}; i < index_size; ++i)
      delete catalog.indexes[i];
 
  }
  ADS_set &operator=(const ADS_set &other){
     clear();
    ADS_set helper(other);
    this->catalog = helper.catalog;
    this->index_size = helper.index_size;
    this->current_size = helper.current_size;
    helper.catalog.indexes = nullptr;
    //helper.catalog.bucketlist = nullptr;
    helper.catalog.num_indexes = 0;
    helper.catalog.bucket_counter = 0;
    return *this;
  }
  ADS_set &operator=(std::initializer_list<key_type> ilist){
    clear();
    insert(ilist);
    return *this;
  }
  std::pair<iterator,bool> insert(const key_type &key){
    std::pair<iterator,bool> newpair;
    if(!count(key)){
      add(key);
      newpair = std::make_pair(find(key), true);
    }
    else
      newpair = std::make_pair(find(key), false);
    return newpair;
  }
  void clear(){
    for(size_t i{0}; i<index_size; ++i){
      for(size_t j{0}; j<catalog.indexes[i]->bucket_count; ++j)
        catalog.indexes[i]->objects[j].mode = Mode::free;
      catalog.indexes[i]->bucket_count = 0;
    }
    current_size = 0;
  }
  size_type erase(const key_type &key){
      size_t hash {h(key)};
      for(size_t i {0}; i < N; ++i){
        if(catalog.indexes[hash]->objects[i].mode == Mode::used)
          if(key_equal{}(catalog.indexes[hash]->objects[i].key, key)){
            catalog.indexes[hash]->objects[i].mode = Mode::free;
            catalog.indexes[hash]->bucket_count -= 1;
            current_size -= 1;
            return 1;
          }
  }
  return 0;

  }
  void swap(ADS_set &other){
    ADS_set helper = ADS_set(other);
    other.catalog = this->catalog;
    other.index_size = this->index_size;
    other.current_size = this->current_size;
    this->catalog = helper.catalog;
    this->index_size = helper.index_size;
    this->current_size = helper.current_size;
  }
  iterator find(const key_type &key) const{
    for(size_t i{0}; i < index_size; ++i){
      if(catalog.indexes[i]->bucket_count > 0){
        for(size_t j{0}; j< N; ++j)
          if(catalog.indexes[i]->objects[j].mode == Mode::used)
            if(key_equal{}(catalog.indexes[i]->objects[j].key, key)) //line ADS_set.h:151
              return const_iterator(&(catalog.indexes[i]), j);//, &catalog.bucket_counter, &catalog.bucketlist);
      }        
    }
    return end();
  }
  const_iterator begin() const{
    for(size_t i{0}; i < index_size; ++i){
      if(catalog.indexes[i]->bucket_count > 0){
        for(size_t j{0}; j< N; ++i)
          if(catalog.indexes[i]->objects[j].mode == Mode::used)
            return const_iterator(&(catalog.indexes[i]), j);//, &catalog.bucket_counter, &catalog.bucketlist);
      }        
    }
    return end();
  }
  const_iterator end() const{return const_iterator(nullptr);}


  ///PHASE 2 SHITE///

  size_type size() const { return current_size; }
  bool empty() const { return current_size == 0; }

  void insert(std::initializer_list<key_type> ilist) {insert(ilist.begin(), ilist.end());}
  //void insert(const std::list<key_type>& ilist) {insert(ilist.begin(), ilist.end());}

  template<typename InputIt> void insert(InputIt first, InputIt last); // PH1
  size_type count(const key_type &key) const {return locate(key) != nullptr;}
  void dump(std::ostream &o = std::cerr) const;
};

template <typename Key, size_t N>
void ADS_set<Key,N>::double_catalog() {
size_t new_num = index_size * 2;
Bucket** new_indexes = new Bucket*[new_num];

    for(size_t i = 0; i < new_num; ++i) {
      new_indexes[i] = catalog.indexes[i % index_size];
    }

    delete[] catalog.indexes;
    catalog.indexes = new_indexes;
    index_size = new_num;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::Bucket::split(ADS_set<Key, N>& ads_set) {
  key_type* helper = new key_type[N];
  size_t counter{0};
  for(size_t i{0}; i < N; ++i){
    if(this->objects[i].mode == Mode::used){
    helper[counter] = (this->objects[i].key);  //3%
    ++counter;}
  }
    size_t pointer_count {0};
    size_t second_index {0};
    for(size_t i=0; i < ads_set.index_size; ++i) { //23%
        if(ads_set.catalog.indexes[i] == this) {  //30%
            ++pointer_count;
            if(pointer_count == 2) {
              second_index = i;
              break;
            }
        }
    }
        if(pointer_count > 1){
          for(size_t i{0}; i < N; ++i)
            this->objects[i].mode = Mode::free;
          this->bucket_count = 0;
          Bucket* new_bucket = new Bucket();
          ads_set.current_size -= counter;
          ads_set.catalog.indexes[second_index] = new_bucket;
        for(size_t i{0}; i < counter; ++i)
          ads_set.add(helper[i]);
        }
        else{ads_set.double_catalog();}
  delete[] helper;
}

template <typename Key, size_t N>
void ADS_set<Key,N>::add(const key_type &key) {
  size_t hash {h(key)};
  //Bucket* checking = catalog.indexes[hash];
  if(catalog.indexes[hash]->bucket_count >= N*max_lf){
    catalog.indexes[hash]->split(*this);
    add(key);
  }
  else{
    size_t i{0};
    while(catalog.indexes[hash]->objects[i].mode != Mode::free){++i;}
    catalog.indexes[hash]->objects[i].key = key;
    catalog.indexes[hash]->objects[i].mode = Mode::used;
    catalog.indexes[hash]->bucket_count += 1;
    if (catalog.indexes[hash]->objects[i].mode == Mode::used) {
      ++current_size;
    }
  }
  //current_size=0;
  //for(size_t i{0}; i < index_size; ++i)
   // current_size += catalog.indexes[i]->bucket_count;

}

template <typename Key, size_t N>
typename ADS_set<Key,N>::Element *ADS_set<Key,N>::locate(const key_type &key) const {
  size_t hash {h(key)};
  for(size_t i {0}; i < N; ++i){
    if(catalog.indexes[hash]->objects[i].mode == Mode::used)
      if(key_equal{}(catalog.indexes[hash]->objects[i].key, key))
        return &catalog.indexes[hash]->objects[i];
  }
  return nullptr;
}

template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key,N>::insert(InputIt first, InputIt last) {
  for (auto it {first}; it != last; ++it) {
    if(it != last)
      if (!count(*it)) {
        add(*it);
      }
  }
}

template <typename Key, size_t N>
void ADS_set<Key,N>::dump(std::ostream &o) const {
  o << "indexer_size = " << index_size << ", current_size = " << current_size << "\n";
  for (size_t i {0}; i < index_size; ++i) {
    o << "indexer" << i << ": ";
    o << "bucket_count" << catalog.indexes[i]->bucket_count << "\n";
    o << "Bucket:\n";
    for ( size_t a {0}; a < N; ++a){
        switch (catalog.indexes[i]->objects[a].mode) {
          case Mode::free:
            o << "--FREE";
            break;
          case Mode::used:
            o << "--"<<catalog.indexes[i]->objects[a].key;
            break;
          case Mode::freeagain:
            o << "--FREEAGAIN";
            break;
    }
    }
    o << "\n";
  }
}
///////////WHOLE FUCKING ITERATOR NEEDS TO BE RE_DONE ->CANNOT USE BUCKETLIST; DESTROYS THE NOTION OF THE DATA STRUCTURE/////////
template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
  private:
    void next_element() {
        while (current_bucket && bucket_index < *total_buckets_ptr) {
            while (current_index < N) {
                if ((*current_bucket)->objects[current_index].mode == Mode::used) {
                    return;
                }
                ++current_index;
            }
            
            ++bucket_index;
            if (bucket_index == *total_buckets_ptr) {
                current_bucket = nullptr;
                return;
            }

            current_bucket = (bucketlist_ptr)[bucket_index];
            current_index = 0;
        }
    }
    void check_index(){
      if(bucket_index == 0 && (bucketlist_ptr[0]) == *current_bucket){
        return;
      }
      if(bucketlist_ptr[bucket_index] == *current_bucket)
        return;
      for(size_t i{bucket_index}; i <= *total_buckets_ptr; ++i){
        if(bucketlist_ptr[i] == *current_bucket){
          bucket_index = i;
          return;
        }
      }
    }
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;

  explicit Iterator(Bucket** bucket = nullptr, size_t index = 0, size_t const* total_buckets_ptr = nullptr, Bucket** const* bucketlist_ptr = nullptr)
        : bucketlist_ptr(bucketlist_ptr), total_buckets_ptr(total_buckets_ptr), current_bucket(bucket), current_index(index) {
        next_element();
    }
  private:
    Bucket** const* bucketlist_ptr;  // Pointer to the pointer of the bucket list
    size_t const* total_buckets_ptr;  // Pointer to the total number of buckets
    Bucket** current_bucket;  // Current bucket the iterator is pointing to
    size_t current_index{0};  // Current index in the current bucket
    size_t bucket_index{0};
  public:
  reference operator*() const { return (*current_bucket)->objects[current_index].key; }
  pointer operator->() const { return &((*current_bucket)->objects[current_index].key); }
  Iterator &operator++() {
    if (current_bucket && *current_bucket) {
        ++current_index;
        if (current_index >= N) {
            if (++bucket_index < *total_buckets_ptr) {
                current_bucket = (bucketlist_ptr)[bucket_index];
                current_index = 0;
            } else {
                current_bucket = nullptr;
            }
        }
    }
    return *this;
}
  Iterator operator++(int) {
    auto temp{*this};
    ++(*this); 
    return temp;
  }
  friend bool operator==(const Iterator &lhs, const Iterator &rhs) {
    if (lhs.current_bucket == nullptr && rhs.current_bucket == nullptr) return true; // Both at the end (past-the-last element)
    if (lhs.current_bucket == nullptr || rhs.current_bucket == nullptr) return false; // One is at the end, the other is not
    return lhs.current_bucket == rhs.current_bucket && lhs.current_index == rhs.current_index && lhs.total_buckets_ptr == rhs.total_buckets_ptr;
}
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs) { return !(lhs == rhs); }
};


template <typename Key, size_t N>
bool operator==(const ADS_set<Key,N> &lhs, const ADS_set<Key,N> &rhs){
  if (lhs.size() != rhs.size()) return false;
  for(auto c : lhs)
    if(!rhs.count(c))
      return false;
  return true;
} 

template <typename Key, size_t N>
bool operator!=(const ADS_set<Key,N> &lhs, const ADS_set<Key,N> &rhs){
  return !(lhs == rhs);
}


template <typename Key, size_t N>
void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs){lhs.swap(rhs);}

    
#endif // ADS_SET_H
