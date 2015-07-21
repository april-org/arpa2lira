/*
 * This file is part of Arpa2Lira for APRIL toolkit (A Pattern Recognizer In
 * Lua).
 *
 * Copyright 2015, Salvador España-Boquera, Francisco Zamora-Martinez
 *
 * Arpa2Lira is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3 as published by the
 * Free Software Foundation
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
}

// from APRIL
#include "error_print.h"
#include "smart_ptr.h"

// from Arpa2Lira
#include "config.h"

namespace Arpa2Lira {
  unsigned int Config::num_threads = 1u;
  std::string Config::tmpdir = "/tmp";
  const std::string Config::TEMPLATE_SUFIX = "/file-a2l-XXXXXX";
  AprilUtils::vector<std::string> Config::tmp_filenames;
  Config::SignalsManager Config::signals_manager;
  AprilUtils::UniquePtr<ThreadPool> Config::thread_pool(new ThreadPool(1u));
  
  int Config::openTemporaryFile(int flags,
                                AprilUtils::UniquePtr<char []> &filename) {
    filename.reset(new char[tmpdir.size() + TEMPLATE_SUFIX.size() + 1]);
    filename = strcpy(filename.get(), tmpdir.c_str());
    filename = strcat(filename.get(), TEMPLATE_SUFIX.c_str());
    int fd = mkostemp(filename.get(), flags);
    tmp_filenames.push_back(std::string(filename.get()));
    return fd;
  }

  void Config::setTemporaryDirectory(const char *tmpdir_) {
    tmpdir = tmpdir_;
  }
  
  void Config::setNumberOfThreads(unsigned int n) {
    if (!Config::thread_pool->empty()) {
      ERROR_EXIT(1, "Unable to change number of threads\n");
    }
    Config::num_threads = n;
    Config::thread_pool = new ThreadPool(n);
  }
  
  unsigned int Config::getNumberOfThreads() {
    return Config::num_threads;
  }

  /////////////////////////////////////////////////////////////////////////
  
  Config::SignalsManager::SignalsManager() {
    signal(SIGINT, Config::SignalsManager::signalHandler);
    signal(SIGABRT, Config::SignalsManager::signalHandler);
    signal(SIGTERM, Config::SignalsManager::signalHandler);
  }
  
  Config::SignalsManager::~SignalsManager() {
    removeTemporaryFiles();
  }
  
  void Config::SignalsManager::removeTemporaryFiles() {
    while(!tmp_filenames.empty()) {
      std::string &filename = tmp_filenames.back();
      if (access(filename.c_str(), F_OK) == 0) {
        if (unlink(filename.c_str()) != 0) {
          ERROR_PRINT2("Unable to remove filename %s: %s\n",
                       filename.c_str(), strerror(errno));
        }
      }
      tmp_filenames.pop_back();
    }
  }
  
  void Config::SignalsManager::signalHandler(int signum) {
    UNUSED_VARIABLE(signum);
    Config::signals_manager.removeTemporaryFiles();
    ERROR_EXIT1(1, "Received signal number %d\n", signum);
  }

}
