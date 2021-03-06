// Author:  Bruce Allen
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
 * Export data in JSON format.  Lines are one of:
 *   source data, block hash data, or comment.
 */

#include <config.h>
// this process of getting WIN32 defined was inspired
// from i686-w64-mingw32/sys-root/mingw/include/windows.h.
// All this to include winsock2.h before windows.h to avoid a warning.
#if defined(__MINGW64__) && defined(__cplusplus)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#ifdef WIN32
  // including winsock2.h now keeps an included header somewhere from
  // including windows.h first, resulting in a warning.
  #include <winsock2.h>
#endif

#include <iostream>
#include <cassert>
#include "../src_libhashdb/hashdb.hpp"
#include "progress_tracker.hpp"

void export_json_sources(const hashdb::scan_manager_t& manager,
                         std::ostream& os) {

  std::string file_hash = manager.first_source();
  while (file_hash.size() != 0) {

    // get source data
    std::string json_source_string = manager.export_source_json(file_hash);

    // program error
    if (json_source_string.size() == 0) {
      assert(0);
    }

    os << json_source_string << "\n";

    // next
    file_hash = manager.next_source(file_hash);
  }
}

void export_json_hashes(const hashdb::scan_manager_t& manager,
                        progress_tracker_t& progress_tracker,
                        std::ostream& os) {

  // space for variables in order to use the tracker
  std::string block_hash;
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_sub_counts_t source_sub_counts;

  block_hash = manager.first_hash();
  while (block_hash.size() != 0) {

    // get hash data
    std::string json_hash_string = manager.export_hash_json(block_hash);

    // program error
    if (json_hash_string.size() == 0) {
      assert(0);
    }

    // emit the JSON
    os << json_hash_string << "\n";

    // update the progress tracker, this accurate approach is expensive
    manager.find_hash(block_hash, k_entropy, block_label,
                      count, source_sub_counts);
    progress_tracker.track_hash_data(source_sub_counts.size());

    // next
    block_hash = manager.next_hash(block_hash);
  }
}

void export_json_range(const hashdb::scan_manager_t& manager,
                       const std::string& begin_block_hash,
                       const std::string& end_block_hash,
                       progress_tracker_t& progress_tracker,
                       std::ostream& os) {

  // the subset of cited sources to export
  std::set<std::string> source_hashes;

  // space for variables in order to use the tracker
  std::string block_hash;
  uint64_t k_entropy;
  std::string block_label;
  uint64_t count;
  hashdb::source_sub_counts_t source_sub_counts;

  // export the block hashes that are in range
  block_hash = manager.first_hash();
  while (block_hash.size() != 0) {

    manager.find_hash(block_hash, k_entropy, block_label,
                      count, source_sub_counts);

    if (block_hash >= begin_block_hash && block_hash <= end_block_hash) {
      // process the block hash since it is in range

      // get JSON hash data
      std::string json_hash_string = manager.export_hash_json(block_hash);

      // program error
      if (json_hash_string.size() == 0) {
        assert(0);
      }

      // emit the JSON
      os << json_hash_string << "\n";

      // note the sources involved
      for (hashdb::source_sub_counts_t::const_iterator it =
           source_sub_counts.begin(); it != source_sub_counts.end(); ++it) {
        source_hashes.insert(it->file_hash);
      }
    }

    // update the progress tracker
    progress_tracker.track_hash_data(source_sub_counts.size());

    // next
    block_hash = manager.next_hash(block_hash);
  }

  // export the cited sources
  for (std::set<std::string>::const_iterator it2 = source_hashes.begin();
       it2 != source_hashes.end(); ++it2) {

    // get source data
    std::string json_source_string = manager.export_source_json(*it2);

    // program error
    if (json_source_string.size() == 0) {
      assert(0);
    }

    os << json_source_string << "\n";
  }
}

