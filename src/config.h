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
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// from APRIL
#include "smart_ptr.h"
#include "vector.h"

namespace Arpa2Lira {
  class Config {
    static std::string tmpdir;
    static const std::string TEMPLATE_SUFIX;
    /// This is a list of tmp filenames generated using openTemporaryFile()
    static AprilUtils::vector<std::string> tmp_filenames;
    
    /// This manager captures SIGINT and others in order to remove all tmp files.
    class SignalsManager {
    public:
      SignalsManager();
      ~SignalsManager();
      void removeTemporaryFiles();
      static void signalHandler(int signum);
    };
    
    /// This static instance forces its automatic construction and destruction
    static SignalsManager signals_manager;
    
  public:
    /// Opens a file and returns its file descriptor and random filename
    static int openTemporaryFile(int flags,
                                 AprilUtils::UniquePtr<char []> &filename);
    static void setTemporaryDirectory(const char *tmpdir_);
  };
} // namespace Arpa2Lira

#endif // CONFIG_H
