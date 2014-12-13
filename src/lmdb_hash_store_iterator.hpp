// Author:  Bruce Allen <bdallen@nps.edu>
// Created: 2/25/2013
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on February 25, 2013 by Bruce Allen.

/**
 * \file
 * Provides a lmdb_hash_store iterator which dereferences into pair type.
 */

#ifndef LMDB_HASH_STORE_ITERATOR_HPP
#define LMDB_HASH_STORE_ITERATOR_HPP
#include "source_lookup_encoding.hpp"
#include "hashdb_element.hpp"
#include "hash_t_selector.h"
#include "liblmdb/lmdb.h"
#include "liblmdb/lmdb.h"
#include "lmdb_resources.h"

class lmdb_hash_store_iterator_t {
  private:
  pthread_resources_t* resources;
  hash_t hash;
  uint64_t value;
  MDB_cursor* cursor;  // cursor just for this iterator
  bool at_end;

  pair_t* dereference() {
    MDB_val key;
    MDB_val data;
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_CURRENT);
    if (rc == MDB_NOTFOUND) {
      // program error
      assert(0);
    } else if (rc != 0) {
      // possible invalid parameter
      assert(0);
    }

    return mdb_to_pair(key, data);
  }

  void increment() {
    if (at_end) {
      assert(0);
    }
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
    if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else if (rc != 0) {
      // program error
      assert(0);
    }
  }

  public:
  // set to key, data pair
  lmdb_hash_store_iterator_t(
                  pthread_resources_t* p_resources,
                  hash_t p_hash,
                  uint64_t p_value):
                        resources(p_resources),
                        hash(p_hash),
                        value(p_value),
                        cursor(),
                        at_end(false) {

    // create a cursor object just for this iterator
    int rc = mdb_cursor_open(resources->txn, resources->dbi, &cursor);
    if (rc != 0) {
      assert(0);
    }

    // set the cursor to this key, data position
    MDB_val key;
    MDB_val data;
    pair_to_mdb(hash, value, key, data);
    int rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_BOTH);
    if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else if (rc != 0) {
      // program error
      assert(0);
    }
  }

  // set to lower bound or upper bound for key
  lmdb_hash_store_iterator_t(
                  pthread_resources_t* p_resources,
                  hash_t p_hash,
                  bool is_lower_bound) :
                        resources(p_resources),
                        hash(p_hash),
                        value(0),
                        cursor(),
                        at_end(false) {

    // set the cursor to this key
    MDB_val key;
    MDB_val data;
    uint64_t value = 0;
    pair_to_mdb(hash, value, key, data);

    // create a cursor object just for this iterator
    int rc = mdb_cursor_open(resources->txn, resources->dbi, &cursor);
    if (rc != 0) {
      assert(0);
    }

    // set the cursor
    int rc;
    if (is_lower_bound) {
      rc = mdb_cursor_get(cursor, &key, &data, MDB_GET_MULTIPLE);
    } else {
      rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT_NODUP);
    }
    if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else if (rc != 0) {
      // program error
      assert(0);
    }
  }

  // set to begin or end
  lmdb_hash_store_iterator_t(
                  pthread_resources_t* p_resources,
                  bool is_begin) :
                        resources(p_resources),
                        hash(0),
                        value(0),
                        cursor(),
                        at_end(true) {

    // create a cursor object just for this iterator
    int rc = mdb_cursor_open(resources->txn, resources->dbi, &cursor);
    if (rc != 0) {
      assert(0);
    }

    // set the cursor
    int rc;
    if (is_begin) {
      rc = mdb_cursor_get(cursor, &key, &data, MDB_FIRST);
    } else {
      rc = MDB_NOTFOUND;
    }
    if (rc == MDB_NOTFOUND) {
      at_end = true;
    } else if (rc != 0) {
      // program error
      assert(0);
    }
  }

  // this useless default constructor is required by std::pair
  lmdb_hash_store_iterator_t() : resources(), hash(), value(0), cursor(0), at_end(false) {
  }

  // override the copy constructor and the assignment operator to quiet the
  // compiler about using a pointer data member
  lmdb_hash_store_iterator_t(const lmdb_hash_store_iterator_t& other) :
               resources(other.resources),
               hash(other.hash),
               value(other.value),
               cursor(other.cursor),
               at_end(other.at_end) {
  }
  lmdb_hash_store_iterator_t& operator=(const lmdb_hash_store_iterator_t& other) {
    resources = other.resources;
    hash = other.hash;
    value = other.value;
    cursor = other.cursor;
    at_end = other.at_end;
    return *this;
  }

  // the Forward Iterator concept consits of ++, *, ->, ==, !=
  lmdb_hash_store_iterator_t& operator++() {
    increment();
    return *this;
  }

  lmdb_hash_store_iterator_t operator++(int) {  // c++11 delete would be better.
    lmdb_hash_store_iterator_t temp(*this);
    increment();
    return temp;
  }

  std::pair<key_t, data_t>& operator*() {
    return dereference();
  }

  pair_t* operator->() {
    return dereference();
  }

  bool operator==(const hashdb_iterator_t& other) const {
    if (at_end == other.at_end) return true;
    if (at_end || other.at_end) return false;
    pair_t this_pair = *this;
    pair_t other_pair = *other;
    return (this_pair.first == other_pair.first &&
            this_pair.second == other_pair.second);
  }

  }
  bool operator!=(const hashdb_iterator_t& other) const {
    if (at_end == other.at_end) return false;
    if (at_end || other.at_end) return true;
    pair_t this_pair = *this;
    pair_t other_pair = *other;
    return (this_pair.first != other_pair.first ||
            this_pair.second != other_pair.second);
  }
};

#endif

