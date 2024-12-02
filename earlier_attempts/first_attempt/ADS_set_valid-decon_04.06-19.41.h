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
    size_t num_indexes{1};
    size_t bucket_counter{1};
    Bucket** indexes;
    Bucket** bucketlist;
    Indexer(size_t n) : num_indexes(n), bucket_counter(n) {
      indexes = new Bucket*[num_indexes];
      bucketlist = new Bucket*[bucket_counter];
      }
    
    void addtoBucketList(){
      size_t newcount = bucket_counter + 1;
      Bucket** newBucketlist = new Bucket*[newcount];
        for(size_t i{0}; i<bucket_counter; ++i)
          newBucketlist[i] = bucketlist[i];
      delete[] bucketlist;
      bucketlist = newBucketlist;
      bucket_counter = newcount;
    }
   ~Indexer() {
      delete[] indexes;
      delete[] bucketlist;
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
    catalog.bucketlist[0] = bucket1;
    catalog.indexes[0] = bucket1;
    current_size = 0;
    }
  ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} {insert(ilist);}
  template<typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{} {insert(first, last);}
  
  ~ADS_set() {
    std::list<Bucket*> killer;
    for (size_t i{0}; (i)<catalog.bucket_counter; ++i) {
      bool already_on_kill_list{false};
      for(auto test : killer)
        if(*test == *catalog.bucketlist[i])
          already_on_kill_list = true;
      if(!already_on_kill_list)
        killer.push_back(catalog.bucketlist[i]);
    }
    for (auto bucket : killer)
      delete bucket;
  }

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
            ads_set.catalog.addtoBucketList();
            ads_set.catalog.bucketlist[ads_set.catalog.bucket_counter-1] = new_bucket;
            //++ads_set.catalog.bucket_counter;
          ads_set.catalog.indexes[second_index] = new_bucket;
        for(size_t i{0}; i < counter; ++i)
          ads_set.add(helper[i]);
        }
        else{ads_set.double_catalog();}
        
  delete[] helper;
  //ads_set.rehash();
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
  Bucket* checking = catalog.indexes[hash];
  for(size_t i {0}; i < N; ++i){
    if(checking->objects[i].mode == Mode::used)
      if(key_equal{}(checking->objects[i].key, key))
        return &checking->objects[i];
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

#endif // ADS_SET_H
