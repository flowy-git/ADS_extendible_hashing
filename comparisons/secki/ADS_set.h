#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include<vector>  //fuer dump



template <typename Key, size_t N = 128>
class ADS_set {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = key_type &;
  using const_reference = const key_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = Iterator;
  using const_iterator = Iterator;
  using key_equal = std::equal_to<key_type>;
  using hasher = std::hash<key_type>;

  struct Bucket{
    size_type ld;     //lokale Tiefe
    size_type curr_size{0};     //Anzahl Elemente in Bucket
    key_type content[N];      //Array aus Elementen
    Bucket* chain;      //Verkettung von Buckets - zeigt auf naechsten Bucket in Kette
    Bucket(size_type ld, Bucket* chain = nullptr): ld(ld), chain(chain) {}      //nur erster Bucket nullptr -> tail
  };

private:
  size_type gd{0};    //globale Tiefe
  Bucket** lookup;    //Tabelle mit Pointern auf Buckets
  size_type look_size{1};   //Tabellengroesse
  size_type set_size{0};    //Anzahl der Elemente in Datenstruktur
  Bucket* head;   //pointer auf Bucket, der noch nicht referenziert wird
  Bucket* tail;   //pointer auf Bucket, der selbst nie referenziert

  size_type hash(const key_type& key) const { return (hasher{}(key) & ((1<<gd)-1)); }
  void expand();
  void split(const key_type &key);
  void insert_(const key_type& key);
  void split_insert(const key_type& key);

public:
  ADS_set();
  ADS_set(const ADS_set &other);
  ~ADS_set();
  ADS_set(std::initializer_list<key_type> ilist): ADS_set{} { insert(ilist); } 
  template<typename InputIt> ADS_set(InputIt first, InputIt last): ADS_set{} { insert(first, last); } 

  ADS_set &operator=(const ADS_set &other);
  ADS_set &operator=(std::initializer_list<key_type> ilist);

  size_type size() const{ return set_size; }
  bool empty() const;
  size_type count(const key_type &key) const;
  iterator find(const key_type &key) const;


  void insert(std::initializer_list<key_type> ilist){ insert(ilist.begin(), ilist.end()); }
  template<typename InputIt> void insert(InputIt first, InputIt last);
  std::pair<iterator,bool> insert(const key_type &key);

  void swap(ADS_set &other);
  size_type erase(const key_type &key);
  void clear();

  const_iterator begin() const { return const_iterator{head}; }
  const_iterator end() const { return const_iterator{tail, tail->curr_size}; }

  friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
    if(lhs.set_size != rhs.set_size) return false;
    for (const auto &elem: rhs) if (!lhs.count(elem)) return false;
    return true;
  }

  friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {
    if(lhs.set_size != rhs.set_size) return true;
    for (const auto &elem: rhs) if (!lhs.count(elem)) return true;
    return false;
  }

  void dump(std::ostream &o = std::cerr) const;
};




template <typename Key, size_t N>
void ADS_set<Key,N>::expand(){
  ++gd; 
  look_size *= 2;

  Bucket** temp = new Bucket*[look_size];

  for(size_t i{0}; i < (look_size/2); ++i) temp[i] = lookup[i];
  for(size_t i{0}; i < (look_size/2); ++i) temp[i + (look_size/2)] = lookup[i];

  delete[] lookup;
  lookup = temp;
}


template <typename Key, size_t N>
void ADS_set<Key,N>::split(const key_type &key){
  size_type idx{hash(key)};
  ++((**(lookup + idx)).ld);
  if((**(lookup + idx)).ld > gd){
    expand();
    idx = hash(key);
  } 

  Bucket temp = **(lookup + idx);   //temporaerer Bucket als Zwischenspeicher fuer zu splittenden Bucket
  (**(lookup + idx)).curr_size = 0;   //werte im aktuellen Bucket invalidieren

  size_type offset{ 1u << ((**(lookup + idx)).ld) };   //offset in lookup zwischen 2 ptrn, die nach split auf selben Bucket zeigen muessen
  size_type first{ (idx & ((1<< (((**(lookup + idx)).ld)-1))-1)) + offset/2 };   //erster ptr, der auf neuen Bucket zeigen soll (hash aus Index mit altem ld, halbes offset dazu, da mit zweitem ptr auf alten Bucket begonnen wird)

  lookup[first] = new Bucket{temp.ld, head};    //neuer Bucket zeigt auf alten head
  head = lookup[first];   //wird danach selbst neuer head

  for(size_type i{first}; i < look_size; i += offset){    //jeder zweite ptr auf altem Bucket wird auf neuen verbogen
    lookup[i] = lookup[first];
  }

  for(size_t i{0}; i < temp.curr_size; ++i){    //erneutes Einfuegen der Inhalte des urspruenglichen Buckets
    split_insert(temp.content[i]);
  }
}


template <typename Key, size_t N>
void ADS_set<Key,N>::insert_(const key_type& key){
  while((**(lookup + hash(key))).curr_size == N) split(key);    //solange Bucket voll - split
  size_type h{hash(key)};
  (**(lookup + h)).content[(**(lookup + h)).curr_size] = key;   //einfuegen an naechster Stelle in Bucket
  ++((**(lookup + h)).curr_size);
  ++set_size;
}



template <typename Key, size_t N>   //ohne ++set_size
void ADS_set<Key,N>::split_insert(const key_type& key){
  while((**(lookup + hash(key))).curr_size == N) split(key);
  (**(lookup + hash(key))).content[(**(lookup + hash(key))).curr_size] = key;
  ++((**(lookup + hash(key))).curr_size);
}




template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set() {
  lookup = new Bucket*[look_size];
  *(lookup) = new Bucket{0};
  head = *(lookup);
  tail = *(lookup);
}


template <typename Key, size_t N>
ADS_set<Key,N>::ADS_set(const ADS_set<Key,N> &other): ADS_set{} {
  for(const auto &elem: other) insert_(elem);
}


template <typename Key, size_t N>
ADS_set<Key,N>::~ADS_set(){     
  Bucket* next_del{head};
  while(next_del){        //loeschen aller Buckets
    Bucket* help{next_del->chain};
    delete next_del;
    next_del = help;
  }
  delete[] lookup;  //loeschen des index
} 



template <typename Key, size_t N>
ADS_set<Key,N>& ADS_set<Key,N>::operator=(const ADS_set &other){
  if(*this == other) return *this;
  ADS_set temp{other};
  swap(temp);
  return *this;
}


template <typename Key, size_t N>
ADS_set<Key,N>& ADS_set<Key,N>::operator=(std::initializer_list<key_type> ilist){
  ADS_set tmp{ilist};
  swap(tmp);
  return *this;
}



template <typename Key, size_t N>
bool ADS_set<Key,N>::empty() const{
  bool empty{true};
  for(size_t i{0}; i < look_size; ++i) if((**(lookup + i)).curr_size != 0) empty = false;
  return empty;
}


template <typename Key, size_t N>
size_t ADS_set<Key,N>::count(const key_type &key) const{
  size_t b_size{(**(lookup + hash(key))).curr_size};
  for(size_t i{0}; i < b_size; ++i){
    if(key_equal{}(((**(lookup + hash(key))).content[i]), key)) return 1;
  }
  return 0;
}


template <typename Key, size_t N>
typename ADS_set<Key,N>::Iterator ADS_set<Key,N>::find(const key_type &key) const{
  size_type h{hash(key)};
  size_type b_size{(**(lookup + h)).curr_size};
  for(size_type i{0}; i < b_size; ++i){
    if(key_equal{}(((**(lookup + h)).content[i]), key)){
      //std::cerr << "////////////\n///////////Found element: bucket_index=" << h << " element_index=" << i << std::endl;
      return iterator{lookup[h], i};
    }
  }
  return this->end();
}



template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key,N>::insert(InputIt first, InputIt last){
  for(auto it{first}; it != last; ++it){
    if(!count(*it)){
      insert_(*it);
    }
  }
}


template <typename Key, size_t N>
std::pair<typename ADS_set<Key,N>::Iterator,bool> ADS_set<Key,N>::insert(const key_type &key){
  if(find(key) != this->end()) return std::pair<iterator,bool>(iterator{find(key)}, false);
  insert_(key);
  return std::pair<iterator,bool>(iterator{find(key)}, true);
}



template <typename Key, size_t N>
void ADS_set<Key,N>::swap(ADS_set<Key,N> &other){
  using std::swap;
  swap(gd, other.gd);
  swap(lookup, other.lookup);
  swap(look_size, other.look_size);
  swap(set_size, other.set_size);
  swap(head, other.head);
  swap(tail, other.tail);
}


template <typename Key, size_t N>
size_t ADS_set<Key,N>::erase(const key_type &key){
  size_type h{hash(key)};
  size_type b_size{(**(lookup + h)).curr_size};
  for(size_type i{0}; i < b_size; ++i){
    if(key_equal{}(((**(lookup + h)).content[i]), key)){    //zu loeschender Wert gefunden
      --((**(lookup + h)).curr_size);
      --set_size;
      for(size_type ii{i}; ii < (**(lookup + h)).curr_size; ++ii){    //ueberspeichern von zu loeschendem Wert mit Werten "rechts" davon
        (**(lookup + h)).content[ii] = (**(lookup + h)).content[ii+1];
      }
      return 1;
    }
  }
  return 0;
}


template <typename Key, size_t N>
void ADS_set<Key,N>::clear(){
  ADS_set tmp;
  swap(tmp);
}



template <typename Key, size_t N>
void ADS_set<Key,N>::dump(std::ostream &o) const{
  std::vector<Bucket*> shown;
  o << "\n--------------------\nGlobal Depth: " << gd << "\nIndex Size: " << look_size << "\nHead: " << head << "\nTail: " << tail << "\n\n";
  for(size_t i{0}; i < look_size; ++i){
    if(!std::count(shown.begin(), shown.end(), lookup[i])){
      o << "\n\nBucket Addresse: " << lookup[i] << "   Local Depth: " <<  (**(lookup + i)).ld << "   Current Size: " << (**(lookup + i)).curr_size << "\nChain: " << (**(lookup + i)).chain << "\nInhalt:\n";
      for(size_t ii{0}; ii < (**(lookup + i)).curr_size; ++ii){
        o << "   " << (**(lookup + i)).content[ii];
      }
    }
    shown.push_back(lookup[i]);
  }
}




template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;
private:
  Bucket* buck;
  size_type idx;
  void skip(){ while(buck != nullptr && buck->chain != nullptr && buck->curr_size <= idx) { buck = buck->chain; idx = 0; } }
public:
  explicit Iterator(Bucket* buck = nullptr, size_type idx = 0): buck(buck), idx(idx) {
    skip();  //leere Buckets ueberspringen, evtl bis tail      
  }
  reference operator*() const { return buck->content[idx]; }
  pointer operator->() const { return &(buck->content[idx]); }
  Iterator &operator++() {
    ++idx;
    skip();
    return *this;
  }
  Iterator operator++(int){
    auto copy{*this};
    ++*this;
    return copy;
  }
  friend bool operator==(const Iterator &lhs, const Iterator &rhs){
    return (lhs.buck == rhs.buck && lhs.idx == rhs.idx);
  }
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs) { 
    return (lhs.buck != rhs.buck || lhs.idx != rhs.idx); 
  }
};

template <typename Key, size_t N> void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }



#endif //ADS_SET_H 