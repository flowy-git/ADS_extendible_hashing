#ifndef ADS_SET_H
#define ADS_SET_H

#include <set>
#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 128>
class ADS_set {

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
    //using key_compare = std::less<key_type>;                         // B+-Tree
    using key_equal = std::equal_to<key_type>;                       // Hashing
    using hasher = std::hash<key_type>;                              // Hashing
//DEBUG set private before hadn in
private:

    size_type globalDepth{0};
    struct Bucket{
        size_type localDepth{0};
        value_type elements[N]{};
        size_type elementsSize{0};
        //size_type indexPointing;
    };
    Bucket** hashtable;
    size_type hashtableCapacity;
    size_type cashedValues;

    size_type last_n_bits(const key_type& k,const unsigned int& n ) const;
    size_type bin_last_n_bits(const size_type& k,const unsigned int& n ) const{
        size_type mask = (1 << n) - 1;
        return k & mask;
    };


    void doubleCapacity();

public:

    void add(const key_type &key); //DEBUG make private later again

    ADS_set();                                                           // PH1
    ADS_set(std::initializer_list<key_type> ilist);                      // PH1
    template<typename InputIt> ADS_set(InputIt first, InputIt last);     // PH1
    ADS_set(const ADS_set &other);

    ~ADS_set();

    ADS_set &operator=(const ADS_set &other);
    ADS_set &operator=(std::initializer_list<key_type> ilist);

    size_type size() const;                                              // PH1
    bool empty() const;                                                  // PH1

    void insert(std::initializer_list<key_type> ilist);                  // PH1
    std::pair<iterator,bool> insert(const key_type &key);
    template<typename InputIt> void insert(InputIt first, InputIt last); // PH1

    void clear();
    size_type erase(const key_type &key);

    size_type count(const key_type &key) const;                          // PH1
    iterator find(const key_type &key) const;

    void swap(ADS_set &other);

    const_iterator begin() const;
    const_iterator end() const;

    void dump(std::ostream &o = std::cerr) const{
        o << "---Current layout----\n";
        o << "Global Depth: " << globalDepth << "\n";
        o << "Capacity: " << hashtableCapacity << "\n";
        o << "Size: " << size() << "\n";

        for(size_type h = 0 ; h < hashtableCapacity; h++){
            o << "_______________\n";
            o << "Bucket: " <<  hashtable[h] << "\n";
            o << "Size: " << (*hashtable[h]).elementsSize << "\n";
            o << "Hashtable Index: " << h << "\n";
            o << "local Depth: " << (*hashtable[h]).localDepth << "\n";
            //o<< "Index pointed by: " << hashtable[h]->indexPointing <<"\n";
            for(size_type g = 0; g < (*hashtable[h]).elementsSize ; g++){
                o << "value: " << (*hashtable[h]).elements[g]   << "\n";
                //std::cout  << "    index : " << last_n_bits((*hashtable[h]).elements[g],globalDepth) << "\n";
                //o << " Bucket index: " << g << "\n";
            }
        }
        o << "Layout ENDS HERE\n";
    };

    friend bool operator==(const ADS_set &lhs, const ADS_set &rhs){
        if (lhs.size() != rhs.size()) {
            return false;
        }
        auto it = lhs.begin();
        while(it != lhs.end()){
            if(! rhs.count(*it))
            {
                return false;
            }
            ++it;
        }
        return true;
    };
    friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs){    return !(lhs == rhs);};
};



//con-/destructors

template <typename Key, size_t N>
ADS_set<Key, N>::ADS_set() : globalDepth(0), hashtable(nullptr), hashtableCapacity(1), cashedValues(0) {
    hashtable = new Bucket*[1];
    hashtable[0] = new Bucket;
}

template <typename Key, size_t N>
ADS_set<Key, N>::ADS_set(std::initializer_list<key_type> ilist) : ADS_set() {
    for (const key_type &element : ilist) {
        add(element);
    }
}

template <typename Key, size_t N>
template <typename InputIt> ADS_set<Key, N>::ADS_set(InputIt first, InputIt last)
        : ADS_set() {  // Delegate to default constructor
    for (InputIt it = first; it != last; ++it) {
        add(*it);
    }
}

template <typename Key, size_t N >
ADS_set<Key, N>::ADS_set(const ADS_set &other):  globalDepth(other.globalDepth),hashtable(nullptr),hashtableCapacity(other.hashtableCapacity), cashedValues(other.cashedValues){
    hashtable = new Bucket*[other.hashtableCapacity];


    //size_type sprungWeite = hashtableCapacity / numberOfPointers; // gibt an  in welchen intervallen pointer im hashtable vorkommen
    for (size_type i = 0; i < hashtableCapacity; ++i) {
        if (i < static_cast<size_type>(1 << other.hashtable[i]->localDepth)) {
            Bucket *b = new Bucket;
            b->elementsSize = other.hashtable[i]->elementsSize;
            b->localDepth = other.hashtable[i]->localDepth;
            for (size_type j = 0; j < b->elementsSize; ++j) {
                b->elements[j] = other.hashtable[i]->elements[j];
            }
            hashtable[i] = b;

            size_type differenz = globalDepth - b->localDepth;
            size_type numberOfPointers = 1 << differenz;
            size_type sprungWiete = hashtableCapacity / numberOfPointers;
            for (size_type k = i + sprungWiete; k < hashtableCapacity; k += sprungWiete) {
                hashtable[k] = b;
            }
        }
    }

}


template <typename Key, size_t N>
ADS_set<Key, N>::~ADS_set() {
    for(auto i = hashtableCapacity; i-- > 0;) {
        if (i < (size_type) (1 << hashtable[i]->localDepth)) {
            delete hashtable[i];
        }
    }
    delete[] hashtable;
}


//Methods
template <typename Key, size_t N >
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::size() const{
    return cashedValues;
}

template <typename Key, size_t N>
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::erase(const key_type &key) {
    if (!count(key)) {
        return 0;
    }
    size_type index = last_n_bits(key, globalDepth);
    size_type i = 0;

    for (; i < hashtable[index]->elementsSize; ++i) {
        if (key_equal{}(key, hashtable[index]->elements[i])) {
            break;
        }
    }

    if (i < hashtable[index]->elementsSize) {
        for (++i; i < hashtable[index]->elementsSize; ++i) {
            hashtable[index]->elements[i - 1] = hashtable[index]->elements[i];
        }
        --hashtable[index]->elementsSize;
        --cashedValues;
        return 1;
    }
    return 0;
}


template <typename Key, size_t N >
bool ADS_set<Key, N>::empty() const{
    return cashedValues==0;
}

template <typename Key, size_t N >
void ADS_set<Key, N>::insert(std::initializer_list<key_type> ilist){
    for(const key_type& element : ilist){
        add(element);
    }
}

template <typename Key, size_t N >
template<typename InputIt> void ADS_set<Key, N>::insert(InputIt first, InputIt last) {
    for (InputIt it = first; it != last; ++it) {
        add(*it);
    }
}

template <typename Key, size_t N >
typename ADS_set<Key,N>::size_type ADS_set<Key, N>::count(const key_type &key) const{
    size_type index = last_n_bits(key,globalDepth);

    for(size_type i =0; i<hashtable[index]->elementsSize; i++){
        if(key_equal{}(hashtable[index]->elements[i],key)){
            return 1;
        }
    }
    return 0;
}

template <typename Key, size_t N >
void ADS_set<Key, N>::add(const key_type &key){
    if(count(key)){return;}
    size_type index = last_n_bits(key, this->globalDepth);

    //sollte im bucket noch platz sein
    if (hashtable[index]->elementsSize < N) {
        hashtable[index]->elements[hashtable[index]->elementsSize] = key;
        hashtable[index]->elementsSize++;
        cashedValues++;
        return;
    }
        //sollte im bucket kein platz mehr sein, aber localdepth noch ungleich global depth
    else if(globalDepth > hashtable[index]->localDepth){
        Bucket* b1 = new Bucket;
        Bucket* b2 = new Bucket;

        b1->localDepth = hashtable[index]->localDepth + 1;
        b2->localDepth = hashtable[index]->localDepth + 1;


        //iteriert über alle elemente im betroffenen bucket und fügt diese jewils entweder in den ersten oder zweiten neuen bucket ein
        for(size_type i = 0; i <  hashtable[index]->elementsSize; i++) {
            //beginnt der index mit 0 wird in b1 eingesetzt, beginnt der index mit 1 in b2
            //e.g. index ist 101 mit dem pendant 001 und einer tiefe von 3. berechnet wird also 2 hoch (3-1) = 4, 4 in binär ist 100. je nachdem ob der index nun größer/gleich oder kleiner ist
            //lässt sich sagen ob der index mit 0 oder 1 anfängt
            if (last_n_bits(hashtable[index]->elements[i], b1->localDepth) < (size_type) (1<< (b1->localDepth-1))) { // ebenso könnte man local depth von b2 oder auch dem alten bucket + 1 checken
                //DEBUG

                b1->elements[b1->elementsSize] = hashtable[index]->elements[i];
                b1->elementsSize +=1;

            }else{
                //DEBUG

                b2->elements[b2->elementsSize] = hashtable[index]->elements[i];
                b2->elementsSize +=1;
            }
        }

        size_type lBits = last_n_bits(key, b1->localDepth);

        size_type lower =  lBits < (size_type) (1 << (b1->localDepth-1)) ? lBits : lBits - (1 << (b1->localDepth-1)) ;
        size_type upper = lower + (1 << (b1->localDepth-1));

        //std::cout << "\n\nAddFreed: " << hashtable[index] <<"\n";
        auto tmp = hashtable[index];
        //b1->indexPointing = lower;
        //b2->indexPointing = upper;

        size_type diff = globalDepth - b1->localDepth;
        size_type numberOfPointers = 1 << diff; //gibt an wie viele indexes aus dem Hashtable auf den Bucket zeigen
        size_type sprungWeite = hashtableCapacity / numberOfPointers; // gibt an in welchen intervallen pointer im hashtable vorkommen
        for(size_type a =lower%sprungWeite; a <hashtableCapacity; a += sprungWeite){
            hashtable[a] = b1;
        }
        for(size_type a =upper% sprungWeite; a <hashtableCapacity; a += sprungWeite){
            hashtable[a] = b2;
        }

        delete tmp; //alter bucket muss gelöscht werden wei ldie daten jetzt auf zwei neue aufgeteilt sind
        add(key);
    }
        // wenn lokale tiefe = lokale tiefe(hashtable wird verdoppelt und add rekursiv erneut aufgerufen)
    else if(globalDepth == (*(hashtable[index])).localDepth){
        doubleCapacity();
        add(key);
    }
}

template <typename Key, size_t N >
void ADS_set<Key, N>::clear(){
    Bucket* b = hashtable[0];
    b->elementsSize = 0;
    b->localDepth = 0;

    for(auto i = hashtableCapacity; i-- > 1;) {
        if (i < (size_type) (1 << hashtable[i]->localDepth)) {
            delete hashtable[i];
        }
        hashtable[i] = b;
    }
    cashedValues = 0;
}


//alle privaten Methoden
template <typename Key, size_t N >
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::last_n_bits(const key_type& k,const unsigned int& n ) const {
    unsigned int ret;
    ret =  hasher{}(k) % (1 << n);
    return ret;
}


template <typename Key, size_t N >
//doubles the capacity by creating a hashtable double the size, then creating one bucket and using the add Method to insert all old values
void ADS_set<Key, N>::doubleCapacity(){
    size_type oldCapacity = hashtableCapacity;
    hashtableCapacity *= 2;

    Bucket** newTable = new Bucket*[hashtableCapacity];
    //creating a copy of the old hashtable to keep acces but still be able to use the add method for re-inserting
    for(size_type k = 0; k < oldCapacity; k++){
        newTable[k] = hashtable[k];
    }

    for(size_type m = oldCapacity; m < hashtableCapacity; m++){
        newTable[m] = hashtable[m-oldCapacity];
    }
    delete[] hashtable;
    hashtable = newTable;
    ++globalDepth;

}

//overloads
template <typename Key, size_t N>
ADS_set<Key, N>& ADS_set<Key, N>::operator=(const ADS_set<Key, N>& other) {
    if (this != &other) {
        this->clear();

        for(auto i = other.hashtableCapacity; i-- > 0;) {
            if (i < (size_type) (1 << other.hashtable[i]->localDepth)) {
                for(size_type j = 0; j < other.hashtable[i]->elementsSize; j++){
                    this->add(other.hashtable[i]->elements[j]);
                }
            }
        }
    }
    return *this;
}

template <typename Key, size_t N>
ADS_set<Key, N>& ADS_set<Key, N>::operator=(std::initializer_list<key_type> ilist){
    this->clear();
    this->insert(ilist);
    return *this;
}

//iterators


template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator ADS_set<Key, N>::begin() const {
    size_type oof = hashtableCapacity -1;
    for(auto i = hashtableCapacity; i-- > 0;) {
        if (i < (size_type) (1 << hashtable[i]->localDepth) && hashtable[i]->elementsSize !=0 ) {
            oof = i;
            break;
        }
    }
    if(!this->empty()) {
        Iterator i(this->hashtable, oof, 0);
        return i;
    }else{
        Iterator i(this->hashtable, oof, 0, true);
        return i;
    }

    //value_type** hashtable, size_type size, size_type valuecount, size_type in
}

template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator ADS_set<Key, N>::end() const {

    return Iterator{this->hashtable, 0, hashtable[0]->elementsSize > 0 ? hashtable[0]->elementsSize -1 : 0, true };
}

template <typename Key, size_t N>
typename ADS_set<Key, N>::const_iterator ADS_set<Key, N>::find(const key_type &key) const{
    if(!count(key)){
        return  this->end();
    }
    size_type index = last_n_bits(key,globalDepth);
    size_type shortedIndex = bin_last_n_bits(index, hashtable[index]->localDepth);
    //std::cout << "index is: " << index << "\n";
    size_type j = 0;
    for(size_type i = 0; i < hashtable[shortedIndex]->elementsSize; i++){
        if(key_equal{}(hashtable[shortedIndex]->elements[i],key)){
            break;
        }else{
            j++;
        }
    }
    //std::cout << "creating iterator now\n";
    return Iterator{hashtable,  shortedIndex , j,  false};
}

template <typename Key, size_t N>
std::pair<typename ADS_set<Key, N>::iterator,bool> ADS_set<Key, N>::insert(const typename ADS_set<Key, N>::key_type &key) {
    bool bo = count(key);
    if(!bo){
        add(key);
    }
    size_type index = last_n_bits(key,globalDepth);
    size_type shortedIndex = bin_last_n_bits(index, hashtable[index]->localDepth);
    size_type j = 0;
    for(size_type i = 0; i < hashtable[shortedIndex]->elementsSize; i++){
        if(key_equal{}(hashtable[shortedIndex]->elements[i],key)){
            break;
        }else{
            j++;
        }
    }
    iterator i{hashtable,  shortedIndex, j };
    return std::make_pair(i, !bo);
}


template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
private:
    Bucket** iHashtable;
    size_type tableLocation;
    size_type bucketLocation;
    bool end{0};

public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::forward_iterator_tag;

    explicit Iterator(Bucket** ht = nullptr, size_type tl = 0, size_type bl = -1, bool ende = false) : iHashtable{ht}, tableLocation{tl}, bucketLocation{bl}, end{ende}{

    }

    reference operator*() const{
        return iHashtable[tableLocation]->elements[bucketLocation];

    };
    pointer operator->() const{
        return &iHashtable[tableLocation]->elements[bucketLocation];
    };

    Iterator &operator++(){
        if(end){
            return *this;
        }
        if(tableLocation == 0 && (bucketLocation + 1 == iHashtable[0]->elementsSize || iHashtable[0]->elementsSize == 0 )){
            end = true;
            return *this;
        }
        //std::cout << "Current Hashtable index and Bucket Index: " <<  tableLocation << "   " << bucketLocation << "\n";
        if(bucketLocation + 1 < iHashtable[tableLocation]->elementsSize &&  iHashtable[tableLocation]->elementsSize != 0){
            bucketLocation++;
        }else {
            //loops through every bucket
            for (int i = (int) tableLocation; i > -1; i--) {
                //selects each bucket only once
                if (i < (int) (1 << iHashtable[i]->localDepth) && i < (int) tableLocation && iHashtable[i]->elementsSize != 0 ) {
                    tableLocation = i;
                    bucketLocation = 0;
                    break;
                }else if( i == 0){
                    end = true;
                }

            }
        }
        return *this;
    };
    Iterator operator++(int){ auto retV = *this; ++*this; return retV;};
    friend bool operator==(const Iterator &lhs, const Iterator &rhs){
        if(lhs.end && rhs.end){
            return true;
        }
        if((lhs.end && !rhs.end ) || (!lhs.end && rhs.end)){
            return false;
        }
        return key_equal{}(lhs.iHashtable[lhs.tableLocation]->elements[lhs.bucketLocation],rhs.iHashtable[rhs.tableLocation]->elements[rhs.bucketLocation]);
    }
    friend bool operator!=(const Iterator &lhs, const Iterator &rhs){return !(lhs == rhs);};
};

template <typename Key, size_t N>
void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }

template <typename Key, size_t N>
void ADS_set<Key, N>::swap(ADS_set &other){


    std::swap(this->hashtable, other.hashtable);
    std::swap(this->hashtableCapacity, other.hashtableCapacity);
    std::swap(this->cashedValues, other.cashedValues);
    std::swap(this->globalDepth, other.globalDepth);


    /*
    Bucket** tmphashtable  = other.hashtable;
    size_type tmphashtableCapacity = other.hashtableCapacity;
    size_type tmpcashedValues = other.cashedValues;
    size_type tmpglobalDepth = other.globalDepth;

    other.hashtable =           this->hashtable;
    other.hashtableCapacity =   this->hashtableCapacity;
    other.cashedValues =        this->cashedValues;
    other.globalDepth =         this->globalDepth;

    this->hashtable =           tmphashtable ;
    this->hashtableCapacity =   tmphashtableCapacity;
    this->cashedValues =        tmpcashedValues;
    this->globalDepth =         tmpglobalDepth;
    */
}


#endif // ADS_SET_H


#if 0

Performance sachen:

beim verdoppeln des hashtables die struktur übernehmen und nicht von vorn eintragen

#endif