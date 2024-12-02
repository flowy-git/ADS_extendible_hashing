#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <list>

template <typename Key, size_t N = 7>
class ADS_set {
public:
  //class /* iterator type (implementation-defined) */;
  using value_type = Key;
  using key_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  //using const_iterator = /* iterator type */;
  //using iterator = const_iterator;
  using key_equal = std::equal_to<key_type>;                       // Hashing
  using hasher = std::hash<key_type>;                              // Hashing

private:
  enum class Mode {free, used, freeagain};
  struct Element {
    key_type key;
    Mode mode {Mode::free};
  };
  struct Bucket {
    Element objects[N];
    size_t bucket_count{0};
    void split(ADS_set<Key, N>& ads_set);
    bool operator==(const Bucket& other) {
      return other.objects == (*this).objects;
    }
  };
  struct Indexer{
    size_t num_indexes{2};
    Bucket** indexes;
    Indexer(size_t n) : num_indexes(n) {indexes = new Bucket*[num_indexes];}
    ~Indexer() {delete[] indexes;}
    
    size_type h(const key_type &key) const { return hasher{}(key) % (num_indexes); }

    bool locate(const key_type &key) const {
    size_t hash {h(key)};
    for(size_t i {0}; i < N; ++i){
      if(indexes[hash]->objects[i].mode == Mode::used){
        if(key_equal{}(indexes[hash]->objects[i].key, key)){
          return true;}
      }
      }
      return false;
    }
    
  };
  std::list<Bucket*> bucketlist;
  Indexer catalog;
  size_t index_size{2};
  size_t current_size {0};
  float max_lf {0.7};
  Element *locate(const key_type &key) const;

    void double_catalog() {
    size_t new_num = index_size * 2;
    Bucket** new_indexes = new Bucket*[new_num];
    for(size_t i = 0, j = 0; i < index_size; ++i, j += 2) {
      new_indexes[j] = catalog.indexes[i];
      new_indexes[j + 1] = catalog.indexes[i];
    }
    delete[] catalog.indexes;
    catalog.indexes = new_indexes;
    index_size = new_num;
    }

  void add(const key_type &key) {
    size_t hash {catalog.h(key)};
    if(catalog.indexes[hash]->bucket_count >= N*max_lf){
      catalog.indexes[hash]->split(*this);
      add(key);    }
    else{
      size_t i{0};
      while(catalog.indexes[hash]->objects[i].mode != Mode::free){
      ++i;}
      catalog.indexes[hash]->objects[i].key = key;
      catalog.indexes[hash]->objects[i].mode = Mode::used;
      catalog.indexes[hash]->bucket_count += 1;
      ++current_size;    }
  }

public:
  ADS_set(): catalog(2) {
    Bucket* bucket1 = new Bucket();
    bucketlist.push_back(bucket1);
    catalog.indexes[0] = bucket1;
    catalog.indexes[1] = bucket1;
    }
  ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} {insert(ilist);}
  template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{} {insert(first, last);}

  size_type size() const { return current_size; }
  bool empty() const { return current_size == 0; }

  void insert(std::initializer_list<key_type> ilist) {insert(ilist.begin(), ilist.end());}
  void insert(std::list<key_type> ilist) {insert(ilist.begin(), ilist.end());}
  //std::pair<iterator,bool> insert(const key_type &key);
  template<typename InputIt> void insert(InputIt first, InputIt last); // PH1

  bool count(const key_type &key) const {return catalog.locate(key);}      //count all that are Mode::used                    // PH1

  void dump(std::ostream &o) const {
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
};

template <typename Key, size_t N>
void ADS_set<Key,N>::Bucket::split(ADS_set<Key, N>& ads_set) {
  std::list<key_type> helper;
  for(size_t i{0}; i < N; ++i){
    helper.push_back(this->objects[i].key);
  }
    size_t pointer_count {0};
    size_t second_index {0};
    for(size_t i=0; i < ads_set.index_size; ++i) {
        if(ads_set.catalog.indexes[i] == this) {
            ++pointer_count;
            if(pointer_count == 1) {
            } else if(pointer_count == 2) {
              second_index = i;
              break;
            }
        }
    }
        if(pointer_count > 1){
          for(size_t i{0}; i < N; ++i)
            this->objects[i].mode = Mode::free;
          ads_set.current_size -= this->bucket_count;
          this->bucket_count = 0;
          Bucket* new_bucket = new Bucket();
          ads_set.bucketlist.push_back(new_bucket);
          ads_set.catalog.indexes[second_index] = new_bucket;
        ads_set.insert(helper);
        }
        else{
                helper.clear();
                ads_set.double_catalog();
                split(ads_set);
        }
}

template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key,N>::insert(InputIt first, InputIt last) {
  for (auto it {first}; it != last; ++it) {
    if (!count(*it)) {
      add(*it);
    }
  }
}

#endif // ADS_SET_H
