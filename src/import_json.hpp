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
 * Import from data in JSON format.  Input types are:
 *
 * Block hash data:
 *   {"block_hash":"a7df...", "file_hash":"b9e7...", "file_offset":4096,
 *   "low_entropy_label":"W", "entropy":8, "block_label":"txt"}
 *
 * Source data:
 *   {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
 *   "low_entropy_count":4}
 *
 * Source name:
 *   {"file_hash":"b9e7...", "repository_name":"repository1",
 *   "filename":"filename1.dat"}
 *
 * Identify type by field, one of: "block_hash", "filesize", or "filename".
 */

#ifndef TAB_HASHDIGEST_READER_HPP
#define TAB_HASHDIGEST_READER_HPP

#include <zlib.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include "hashdb.hpp"
#include "progress_tracker.hpp"
#include "hex_helper.hpp"
#include <string.h> // for strerror
#include <fstream>
#include "rapidjson.h"
#include "writer.h"
#include "document.h"

class import_json_t {
  private:

  // state
  const std::string& hashdb_dir;
  const std::string& json_file;
  const std::string& cmd;
  size_t line_number;

  // resources
  hashdb::import_manager_t manager;
  std::ifstream in;
  progress_tracker_t progress_tracker;

  // do not allow these
  import_json_t();
  import_json_t(const import_json_t&);
  import_json_t& operator=(const import_json_t&);

  // private, used by read()
  import_json_t(const std::string& p_hashdb_dir,
                const std::string& p_json_file,
                const std::string& p_cmd) :
        hashdb_dir(p_hashdb_dir),
        json_file(p_json_file),
        cmd(p_cmd),
        line_number(0),
        manager(hashdb_dir, cmd),
        in(json_file),
        progress_tracker(hashdb_dir, 0) {
  }

  ~import_json_t() {
    in.close();
  }

  void show_bad_line(const std::string& line) {
    std::cerr << "Invalid line " << line_number << ": '" << line << "'\n";
  }

  // Block hash data:
  //   {"block_hash":"a7df...", "file_hash":"b9e7...", "file_offset":4096,
  //   "low_entropy_label":"W", "entropy":8, "block_label":"txt"}
  void add_block_hash_data(const rapidjson::Document& document,
                           const std::string& line) {

    // block_hash
    if (!document.HasMember("block_hash") ||
                  !document["block_hash"].IsString()) {
      show_bad_line(line);
      return;
    }
    std::string block_hash = document["block_hash"].GetString();

    // file_hash
    if (!document.HasMember("file_hash") ||
                  !document["file_hash"].IsString()) {
      show_bad_line(line);
      return;
    }
    std::string file_hash = document["file_hash"].GetString();

    // file_offset
    if (!document.HasMember("file_offset") ||
                  !document["file_offset"].IsNumber()) {
      show_bad_line(line);
      return;
    }
    uint64_t file_offset = document["file_offset"].GetUint64();

    // low_entropy_label (optional)
    std::string low_entropy_label =
                 (document.HasMember("low_entropy_label") &&
                  document["low_entropy_label"].IsString()) ?
                     document["low_entropy_label"].GetString() : "";

    // entropy (optional)
    uint64_t entropy =
                 (document.HasMember("entropy") &&
                  document["entropy"].IsNumber()) ?
                     document["entropy"].GetUint64() : 0;

    // block_label (optional)
    std::string block_label =
                 (document.HasMember("block_label") &&
                  document["block_label"].IsString()) ?
                     document["block_label"].GetString() : "";

    // get or create source ID
    std::pair<bool, uint64_t> id_pair = manager.insert_source_id(
                                                hex_to_bin(file_hash));

    // add the block hash
    manager.insert_hash(hex_to_bin(block_hash), id_pair.second, file_offset,
                        low_entropy_label, entropy, block_label);
  }

  //Source data:
  //  {"file_hash":"b9e7...", "filesize":8000, "file_type":"exe",
  //  "low_entropy_count":4}
  void add_source_data(const rapidjson::Document& document,
                       const std::string& line) {

    // file_hash
    if (!document.HasMember("file_hash") ||
                  !document["file_hash"].IsString()) {
      show_bad_line(line);
      return;
    }
    std::string file_hash = document["file_hash"].GetString();

    // filesize
    if (!document.HasMember("filesize") ||
                  !document["filesize"].IsNumber()) {
      show_bad_line(line);
      return;
    }
    uint64_t filesize = document["filesize"].GetUint64();

    // file_type (optional)
    std::string file_type =
                 (document.HasMember("file_type") &&
                  document["file_type"].IsString()) ?
                     document["file_type"].GetString() : "";

    // low_entropy_count (optional)
    uint64_t low_entropy_count =
                 (document.HasMember("low_entropy_count") &&
                  document["low_entropy_count"].IsNumber()) ?
                     document["low_entropy_count"].GetUint64() : 0;

    // get or create source ID
    std::pair<bool, uint64_t> id_pair = manager.insert_source_id(
                                                hex_to_bin(file_hash));

    // add the block hash
    manager.insert_source_data(id_pair.second, hex_to_bin(file_hash),
                               filesize, file_type, low_entropy_count);
  }

  // source name:
  //   {"file_hash":"b9e7...", "repository_name":"repository1",
  //   "filename":"filename1.dat"}
  void add_source_name(const rapidjson::Document& document,
                       const std::string& line) {

    // file_hash
    if (!document.HasMember("file_hash") ||
                  !document["file_hash"].IsString()) {
      show_bad_line(line);
      return;
    }
    std::string file_hash = document["file_hash"].GetString();

    // repository_name 
    if (!document.HasMember("repository_name") ||
                  !document["repository_name"].IsString()) {
      show_bad_line(line);
      return;
    }
    std::string repository_name = document["repository_name"].GetString();

    // filename 
    if (!document.HasMember("filename") ||
                  !document["filename"].IsString()) {
      show_bad_line(line);
      return;
    }
    std::string filename = document["filename"].GetString();

    // get or create source ID
    std::pair<bool, uint64_t> id_pair = manager.insert_source_id(
                                                hex_to_bin(file_hash));

    // add the source name
    manager.insert_source_name(id_pair.second, repository_name, filename);
  }

  void read_lines() {
    std::string line;
    while(getline(in, line)) {
      ++line_number;

      // skip comment lines
      if (line[0] == '#') {
        continue;
      }

      // skip empty lines
      if (line.size() == 0) {
        continue;
      }

      // open the line as a JSON DOM document
      rapidjson::Document document;
      if (document.Parse(line.c_str()).HasParseError() ||
          !document.IsObject()) {
        show_bad_line(line);
        continue;
      }

      // process block hash data
      if (document.HasMember("block_hash")) {
        add_block_hash_data(document, line);
      } else if (document.HasMember("filesize")) {
        add_source_data(document, line);
      } else if (document.HasMember("filename")) {
        add_source_name(document, line);
      } else {
        show_bad_line(line);
      }
    }
  }

  public:

  // read JSON file
  static std::pair<bool, std::string> read(
                     const std::string& p_hashdb_dir,
                     const std::string& p_json_file,
                     const std::string& p_cmd) {

    // validate hashdb_dir path
    std::pair<bool, std::string> pair;
    pair = hashdb::is_valid_hashdb(p_hashdb_dir);
    if (pair.first == false) {
      return pair;
    }

    // validate ability to open the JSON file
    std::ifstream p_in(p_json_file.c_str());
    if (!p_in.is_open()) {
      std::stringstream ss;
      ss << "Cannot open " << p_json_file << ": " << strerror(errno);
      return std::pair<bool, std::string>(false, ss.str());
    }
    p_in.close();

    // create the reader
    import_json_t reader(p_hashdb_dir, p_json_file, p_cmd);

    // read the lines
    reader.read_lines();

    // done
    return std::pair<bool, std::string>(true, "");
  }
};

#endif

