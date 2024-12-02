#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 64 /* default N (implementation-defined) */>
class ADS_set {
public:
  class Iterator/* iterator type (implementation-defined) */;
  using value_type = Key;
  using key_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using const_iterator = Iterator/* iterator type */;
  using iterator = const_iterator;
  using key_compare = std::less<key_type>;                         // B+-Tree
  using key_equal = std::equal_to<key_type>;                       // Hashing
  using hasher = std::hash<key_type>;                              // Hashing
private:
    struct Bucket{
        size_type local_depth;
        size_type size;
        size_type count;
        key_type* elements;
        Bucket(size_type depth) : local_depth(depth), size(N), count(0){
            elements = new key_type[N];
        }
        ~Bucket() {
            delete[] elements;
        }
        bool isFull() const{return count == size;}
        void insert(const key_type& key) {elements[count++] = key;} //does not protect against overflow - must check !isFull before call
    };
    struct Directory{
        size_type global_depth;
        Bucket** buckets;
        Directory(size_type depth) : global_depth(depth){
            size_type size = 1 << global_depth;
            buckets = new Bucket*[size];
            for(size_t i{0}; i < size; ++i){
                buckets[i] = new Bucket(global_depth);
            }
        }
        ~Directory() {
            size_type size = 1 << global_depth;
            bool* deleted = new bool[size]{false};  // Track which buckets have been deleted
            for (size_type i{0}; i < size; ++i) {
                if (!deleted[i]) {
                    for (size_type j = i; j < size; j += (1 << buckets[i]->local_depth)) {
                        deleted[j] = true;  // Mark all references to this bucket as deleted
                    }
                    delete buckets[i];
                }
            }
            delete[] deleted;
            delete[] buckets;
        }
        /* ~Directory(){
            size_type size = 1 << global_depth;
            for(int i{0}; i < size; ++i){
                delete buckets[i];
            }
            delete[] buckets;
        } */
    };

    Directory directory;
    void split_bucket(size_type hash);
    void double_catalog();
    size_type current_size{0};
    size_type h(const key_type &key) const {
        return hasher{}(key) & ((1 << directory.global_depth) - 1);
    }
    size_type hashed(const key_type &key, size_t mod) const {
        return hasher{}(key) & ((1 << mod) - 1);
    }


public:

  ADS_set();                                                           // PH1
  ADS_set(std::initializer_list<key_type> ilist);                      // PH1
  template<typename InputIt> ADS_set(InputIt first, InputIt last);     // PH1
  //ADS_set(const ADS_set &other);

  //~ADS_set();

  //ADS_set &operator=(const ADS_set &other);
  //ADS_set &operator=(std::initializer_list<key_type> ilist);

  size_type size() const;                                              // PH1
  bool empty() const;                                                  // PH1

  void insert(std::initializer_list<key_type> ilist);                  // PH1
  //std::pair<iterator,bool> insert(const key_type &key);
  template<typename InputIt> void insert(InputIt first, InputIt last); // PH1
  void insert(const key_type& key);

  //void clear();
  //size_type erase(const key_type &key);

  size_type count(const key_type &key) const;                          // PH1
  //iterator find(const key_type &key) const;

/*   void swap(ADS_set &other); */

/*   const_iterator begin() const;
  const_iterator end() const; */

void dump(std::ostream &o = std::cerr) const {
    o << "ADS_set dump:\n";
    o << "Global depth: " << directory.global_depth << "\n";
    size_t num_buckets = 1 << directory.global_depth;
    for (size_t i = 0; i < num_buckets; ++i) {
        o << "Bucket " << i << " (local depth: " << directory.buckets[i]->local_depth
          << ", size: " << directory.buckets[i]->size
          << ", count: " << directory.buckets[i]->count << "): ";
        for (size_t j = 0; j < directory.buckets[i]->count; ++j) {
            o << directory.buckets[i]->elements[j] << " - ";
        }
        o << "\n";
    }
}

  friend bool operator==(const ADS_set &lhs, const ADS_set &rhs){
    return false;
  };
  friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs){
    return false;
  };
};

template <typename Key, size_t N>
ADS_set<Key, N>::ADS_set() : directory(1) {}
template <typename Key, size_t N>
ADS_set<Key, N>::ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} {insert(ilist);}
template <typename Key, size_t N >
template <typename InputIt> ADS_set<Key, N>::ADS_set(InputIt first, InputIt last) : ADS_set{} {
    insert(first, last);
}
/* template <typename Key, size_t N>
ADS_set<Key, N>::ADS_set(const ADS_set &other) {}

template <typename Key, size_t N>
ADS_set<Key, N>::~ADS_set() {} */

/* template <typename Key, size_t N>
ADS_set<Key, N>& ADS_set<Key, N>::operator=(const ADS_set<Key, N>& other) {}
template <typename Key, size_t N>
ADS_set<Key, N>& ADS_set<Key, N>::operator=(std::initializer_list<key_type> ilist) {} */
template <typename Key, size_t N> void ADS_set<Key, N>::split_bucket(size_type hash){
    Bucket* old_bucket = directory.buckets[hash];
    size_type new_local = old_bucket->local_depth + 1;
    Bucket* new_bucket = new Bucket(new_local);
    size_type mask = (1 << old_bucket->local_depth);
    for(size_type i{0}; i < static_cast<size_type>(1 << directory.global_depth); ++i){
        if((i & (mask - 1)) == (hash & (mask - 1))) {
            if(i & mask){
                directory.buckets[i] = new_bucket;
            }
            else{
                directory.buckets[i] = old_bucket;
            }
        }
    }
    size_type old_count = old_bucket->count;
    old_bucket->count = 0;
    for (size_type i = 0; i < old_count; ++i) {
        size_type new_hash = h(old_bucket->elements[i]);
        if ((new_hash & mask) == 0) {
            old_bucket->insert(old_bucket->elements[i]);
        } 
        else {
            new_bucket->insert(old_bucket->elements[i]);
        }
    }
    old_bucket->local_depth = new_local;
}
template <typename Key, size_t N> void ADS_set<Key, N>::double_catalog(){
    size_type old_size = 1 << directory.global_depth;
    size_type new_size = old_size << 1;
    Bucket** newBuckets = new Bucket*[new_size];
    for(size_t i{0}; i < old_size; ++i){
        newBuckets[i] = directory.buckets[i];
        newBuckets[i + old_size] = directory.buckets[i];
    }
    delete[] directory.buckets;
    directory.buckets = newBuckets;
    ++directory.global_depth;
}

template <typename Key, size_t N >
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::size() const{
    return current_size;
}

template <typename Key, size_t N >
bool ADS_set<Key, N>::empty() const{
    return current_size == 0;
}

template <typename Key, size_t N >
void ADS_set<Key, N>::insert(const key_type& key){
    size_type hash = h(key);
    Bucket* bucket = directory.buckets[hash];
    for (size_t i = 0; i < bucket->count; ++i) {
        if (key_equal{}(bucket->elements[i], key)) {
            return; // Key already exists
        }
    }
    if(!bucket->isFull()){
        bucket->insert(key);
    }
    else{
        while(bucket->isFull()){
            if(bucket->local_depth == directory.global_depth){
                double_catalog();
                hash = h(key);
            }
            split_bucket(hash);
            hash = h(key);
            bucket = directory.buckets[hash];
        }
        if(!bucket->isFull()){ //not needed just for debug
            bucket->insert(key);
        }
        else{throw std::runtime_error("yeah nah the split didnt work");} //not needed just for debug
    }
    ++current_size;
}
template <typename Key, size_t N >
void ADS_set<Key, N>::insert(std::initializer_list<key_type> ilist){
    insert(ilist.begin(), ilist.end());
}
template <typename Key, size_t N >
template<typename InputIt> void ADS_set<Key, N>::insert(const InputIt first, InputIt last) {
    for (auto it{first}; it != last; ++it) {
        insert(*it);
  }
}
/*template <typename Key, size_t N>
std::pair<typename ADS_set<Key, N>::iterator,bool> ADS_set<Key, N>::insert(const typename ADS_set<Key, N>::key_type &key) {} */

/*template <typename Key, size_t N >
void ADS_set<Key, N>::clear(){}
template <typename Key, size_t N>
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::erase(const key_type &key) {} */

template <typename Key, size_t N >
typename ADS_set<Key,N>::size_type ADS_set<Key, N>::count(const key_type &key) const{ //once iterator present, can rely on find()
    size_type hash = h(key);
    Bucket* bucket = directory.buckets[hash];
    for (size_t i{0}; i < bucket->count; ++i) {
        if (key_equal{}(bucket->elements[i], key)) {
            return 1;
        }
    }
    return 0;
}



//////////// DEBUG VERSION /////////
/* template <typename Key, size_t N>
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::count(const key_type &key) const {
    size_type hash = h(key);  // Calculate the hash for the given key
    std::cout << "Key: " << key << ", Hash: " << hash << std::endl;
    Bucket* bucket = directory.buckets[hash];
    std::cout << "Bucket Local Depth: " << bucket->local_depth << std::endl;
    std::cout << "Bucket Count: " << bucket->count << std::endl;
    std::cout << "Bucket Elements: ";
    for (size_type i = 0; i < bucket->count; ++i) {
        std::cout << bucket->elements[i] << " ";}
    std::cout << std::endl;
    for (size_type i = 0; i < bucket->count; ++i) {
        if (bucket->elements[i] == key) {
            return 1;}}
    return 0;
} */
////////// DEBUG VERSION /////////


/*template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator ADS_set<Key, N>::find(const key_type &key) const{} */

/*void ADS_set<Key, N>::swap(ADS_set &other){}

template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator ADS_set<Key, N>::begin() const {}
template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator ADS_set<Key, N>::end() const {} */






#endif
