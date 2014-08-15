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
 * This file manages the hashdb settings.
 */

#ifndef    HASHDB_SETTINGS_STORE_HPP
#define    HASHDB_SETTINGS_STORE_HPP

#include "hashdb_settings.hpp"
#include "dfxml/src/dfxml_writer.h"
#include "hashdb_settings_reader.hpp"
#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>

// hashdb tuning options
class hashdb_settings_store_t {
  public:

  static hashdb_settings_t read_settings(const std::string& hashdb_dir) {
    // hashdb_dir must exist
    if (access(hashdb_dir.c_str(), F_OK) != 0) {
      std::cerr << "Unable to read database '" << hashdb_dir
                << "'.  Aborting.\n";
      exit(1);
    }

    hashdb_settings_t settings;
    std::string filename = hashdb_dir + "/settings.xml";
    try {
      hashdb_settings_reader_t::read_settings(filename, settings);
    } catch (std::runtime_error& e) {
      std::cerr << "Unable to read database settings: " << e.what() 
                << "\nAborting.\n";
      exit(1);
    }
    return settings;
  }

  static void write_settings(const std::string& hashdb_dir,
                             const hashdb_settings_t& settings) {

    // calculate the settings filename
    std::string filename = hashdb_dir + "/settings.xml";
    std::string filename_old = hashdb_dir + "/_old_settings.xml";

    // if present, move existing settings to old
    if (access(filename.c_str(), F_OK) == 0) {
      int status = std::rename(filename.c_str(), filename_old.c_str());
      if (status != 0) {
        std::cerr << "Warning: unable to back up '" << filename << "' to '" << filename_old << "'.\n";
      }
    }

    // write out the settings
    dfxml_writer x(filename, false);
    x.push("settings");
    settings.report_settings(x);
    x.pop();
  }
};

#endif

