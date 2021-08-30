//
// Created by Peter Ivony on 28.04.21.
//

#ifndef ADS_HASHTABELLE_ADS_SET_H
#define ADS_HASHTABELLE_ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <
        typename Key,
        size_t N = 4
>
class ADS_set {
public:
    class Iterator;

    using value_type = Key;
    using key_type = Key;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator;
    using const_iterator = const iterator;
    using key_equal = std::equal_to<key_type>;
    using hasher = std::hash<key_type>;
private:
    struct Block;
    size_type global_depth;
    size_type current_size;
    Block** index_table;
public:

    //Constructors
    ADS_set() : global_depth{0}, current_size{0} {
        index_table = new Block*[1];
        index_table[0] = new Block(0);
    }
    ADS_set(std::initializer_list<key_type> ilist) : ADS_set() {
        insert(ilist);
    }
    template<typename InputIt>
    ADS_set(InputIt first, InputIt last) : ADS_set(){
        insert(first, last);
    }

    ADS_set(const ADS_set& other) : ADS_set(other.begin(), other.end()){}

    //Assignment operators - Phase 2
    ADS_set& operator=(const ADS_set& other){// to do
        if (this == &other) return *this;
        ADS_set tmp{other};
        swap(tmp);
        return *this;
    }

    ADS_set& operator=(std::initializer_list<key_type> ilist){
        ADS_set tmp(ilist);
        swap(tmp);
        return *this;
    }

    //Destructor
     ~ADS_set(){
        for (size_type i{0}; i < static_cast<size_type>(1 << global_depth); ++i)
            if (index_table[i] != nullptr){
                size_type loc_depth{index_table[i]->local_depth};
                delete index_table[i];
                for (size_type j{i}; j < static_cast<size_type>(1 << global_depth); j += (1 << loc_depth))
                    index_table[j] = nullptr;
            }
        delete[] index_table;
    }

    //Methods
        //General
    size_type size() const{
        return current_size;
    }
    bool empty() const{
        return (current_size == 0);
    }
    void swap(ADS_set& other){
        using std::swap;
        swap(this->index_table, other.index_table);
        swap(this->global_depth, other.global_depth);
        swap(this->current_size, other.current_size);
    }
        //Insert
    void insert(std::initializer_list<key_type> ilist){
        for (const key_type& item : ilist){
            insert(item);
        }
    }
    std::pair<iterator, bool> insert(const key_type& key){

        //Hashing the key
        size_type index{hasher{}(key) % (1 << global_depth)};
        index %= (1 << index_table[index]->local_depth);

        //Check if key already in table
        const auto ptr = index_table[index]->find(key);
        const ptrdiff_t index_in_block{ptr - index_table[index]->values};
        if (ptr != nullptr) return std::make_pair<iterator, bool>(Iterator(index_table, global_depth, ptr, index, index_in_block), false);

        //Key fits into the hash table
        if (index_table[index]->num_of_elem < N) {
            auto return_key = index_table[index]->emplace(key);
            ++current_size;
            return std::make_pair<iterator, bool>(Iterator(index_table, global_depth, return_key, index, index_table[index]->num_of_elem - 1), true);
        }

        const size_type loc_depth_of_indexed = (index_table[index])->local_depth;
        //Key doesn't fit, but global depth not yet reached in block
        if (loc_depth_of_indexed < global_depth){

            size_type first_index{index % (1 << loc_depth_of_indexed)}; //New bucket will be created here - see line 103
            size_type old_bucket_index{first_index + (1 << loc_depth_of_indexed) }; //One of the indexes that will still point to the old bucket

            //Set half of the pointers to the new bucket
            bool first_it = true; //For the first iteration, we have to create the new bucket
            for (size_type i{first_index};
                 i < static_cast<size_type>(1 << global_depth); // smaller than table size
                 i += 1 << (loc_depth_of_indexed + 1) //Every second pointer
                ) {
                if (first_it){
                    index_table[i] = new Block(loc_depth_of_indexed+1);
                    first_it = false;
                } else {
                    index_table[i] = index_table[first_index];
                }
            }

            for (size_type j{0}; j < index_table[old_bucket_index]->num_of_elem; j++){
                auto new_hash = hasher{}(index_table[old_bucket_index]->values[j]) % (1 << global_depth);
                bool in_old = false;
                for (size_type i{old_bucket_index};
                    i < static_cast<size_type>(1 << global_depth); // smaller than table size
                    i += 1 << (loc_depth_of_indexed + 1) //Every second pointer
                    ) {
                    if (i == new_hash) {
                        in_old = true;
                        break;
                    }
                }
                if (!in_old) {
                    index_table[first_index]->emplace(index_table[old_bucket_index]->values[j]); //Insert key into the new bucket
                    index_table[old_bucket_index]->remove(j); //Remove the key from the old one
                    j--;
                }
            }

            //Change old bucket's local depth
            index_table[old_bucket_index]->local_depth++;
            //Try again the insert
            return insert(key);

        //Global depth == Local depth
        } else if (loc_depth_of_indexed == global_depth) {
            //Create new, extended address table (later will be index_table)
            auto temp = new Block*[1 << (global_depth + 1)];
            //Set all pointers in new address table to the existing buckets
            for (size_t i{0}; i < static_cast<size_type>(1 << global_depth); ++i)
                temp[i] = temp[i + (1 << global_depth)] = index_table[i];

            //Delete old table
            delete[] index_table;
            //Reassign index_table
            index_table = temp; //temp implicitly destroyed after function exit
            //Increase global depth
            global_depth++;
            //Try again the insert
            return insert(key);
        }
        return std::pair<Iterator, bool>(end(), false);
    }

    template<typename InputIt>
    void insert(InputIt first, InputIt last){
        for (; first != last; ++first)
            insert(*first);
    }
        //Delete - Phase 2
    void clear(){
        ADS_set tmp;
        swap(tmp);
    }

    size_type erase(const key_type& key){
        size_type index{hasher{}(key) % (1 << global_depth)};
        index %= (1 << index_table[index]->local_depth);
        const auto i = index_table[index]->find(key); //We try to find the key in the indexed bucket

        if (i == nullptr) return 0; //We didn't find it :(

        index_table[index]->remove(i - index_table[index]->values); //One less key :(
        current_size--;
        return 1;
    }

        //Search
    size_type count(const key_type& key) const{
        size_type index{hasher{}(key) % (1 << global_depth)};
        index %= (1 << index_table[index]->local_depth);
        return (index_table[index]->find(key) == nullptr) ? 0 : 1;
    }
    iterator find(const key_type& key) const{
        size_type index{hasher{}(key) % (1 << global_depth)};
        index %= (1 << index_table[index]->local_depth);
        const auto ptr = index_table[index]->find(key);
        if (ptr == nullptr) return end();
        const ptrdiff_t diff{ptr - index_table[index]->values};
        return Iterator(index_table, global_depth, ptr, index, diff);
    }
        //Iterators - Phase 2
    const_iterator begin() const{
        return Iterator(index_table, global_depth);
    }

    const_iterator end() const{
        return Iterator();
    }

    // Boolean operators
    friend bool operator==(const ADS_set& lhs, const ADS_set& rhs){
        if (lhs.size() != rhs.size()) return false;
        for (auto i{rhs.begin()}; i != rhs.end(); i++)
            if (lhs.count(*i) == 0) return false;
        return true;
    }
    friend bool operator!=(const ADS_set<Key,N>& lhs, const ADS_set<Key,N>& rhs){ return !(lhs == rhs);}

    //Debugging
    void dump(std::ostream& o = std::cerr) const{
        o << "======================= Extended Hashtable =======================" << std::endl;
        o << "Global depth: " << global_depth << "\nNumber of stored keys: " << current_size << "\nIs container empty?: " << empty() << std::endl;
        o << "--------------- Table contents ---------------" << std::endl;
        for (size_type i{0}; i < static_cast<size_type>(1 << global_depth); ++i){
            o << "Index " << i << ": ";
            for (size_type j{0}; j < index_table[i]->num_of_elem; ++j){
                o << index_table[i]->values[j] << ' ';
            }
            o << std::endl;
        }
        o << "--------------- Iterator output ---------------" << std::endl;
        size_type ctr{0};
        for (auto it{begin()}; it != end(); ++it){
            o << ' ' << *it;
            ++ctr;
            if (ctr == 10){
                o << '\n';
                ctr = 0;
            }

        }
        o << '\n';
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
    pointer ptr;
    Block** index_table;
    size_type global_depth, table_index, block_index;

    void skip(){ //Must be edited if i want to jump over already visited blocks!
        for (; table_index < static_cast<size_type>(1 << global_depth); ++table_index){
            if (table_index < static_cast<size_type>(1 << index_table[table_index]->local_depth)){
                for (; block_index < index_table[table_index]->num_of_elem; ++block_index){
                    if (!ptr){
                        ptr = index_table[table_index]->values;
                        return;
                    } else if (ptr  && (ptr < index_table[table_index]->values + index_table[table_index]->num_of_elem) ){
                        ++block_index;
                        return;
                    }
                    ptr = nullptr;
                }
                block_index = 0;
            }
        }
        table_index = 0;
    }

public:
    explicit Iterator() : ptr{nullptr}, index_table{nullptr}, global_depth{0}, table_index{0}, block_index{0}{}
    //for any iterator
    Iterator(Block** index_table, size_type global_depth, pointer ptr, size_type table_index, size_type block_index) : ptr{ptr}, index_table{index_table}, global_depth{global_depth}, table_index{table_index}, block_index{block_index}{}
    //for begin()
    Iterator(Block** index_table, size_type depth) : Iterator (index_table, depth, nullptr, 0, 0){ skip(); }

    reference operator*() const{
        return *ptr;
    }
    pointer operator->() const{
        return ptr;
    }
    iterator& operator++(){
        ++ptr;
        skip();
        return *this;
    }
    iterator operator++(int){
        auto to_return{*this};
        ++*this;
        return to_return;
    }
    friend bool operator==(const iterator &lhs, const iterator &rhs){
        return lhs.ptr == rhs.ptr;
    }
    friend bool operator!=(const iterator &lhs, const iterator &rhs){
        return !(lhs==rhs);
    }
};


template <typename Key,size_t N>
struct ADS_set<Key,N>::Block{
    key_type* values;
    size_type local_depth, num_of_elem;

    Block(size_type local_depth) : local_depth{local_depth}, num_of_elem{0}{
        values = new key_type[N];
    }
    const key_type* emplace(const key_type& k){  //returns nullptr if full
        return (num_of_elem == N) ? nullptr : &(values[num_of_elem++] = k);
    }
    void remove(size_type i){ //deletes key at index i
        for (auto j{i}; j < num_of_elem-1; ++j)
            values[j] = values[j+1];
        --num_of_elem; //One less key :(
    }
    const key_type* find(const key_type& k) const{  //return nullptr if element not found, else iterator to element
        for (size_type i{0}; i < num_of_elem; ++i)
            if (key_equal{}(values[i], k)) return &values[i];
        return nullptr;
    }
    ~Block(){
        delete[] values;
    }
};

template <typename Key, size_t N>
void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }

#endif //ADS_HASHTABELLE_ADS_SET_H
