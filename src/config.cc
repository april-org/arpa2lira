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
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
}

// from APRIL
#include "smart_ptr.h"

// from Arpa2Lira
#include "config.h"

namespace Arpa2Lira {
  std::string Config::tmpdir = "/tmp";
  const std::string Config::TEMPLATE_SUFIX = "/file-a2l-XXXXXX";

  int Config::openTemporaryFile(int flags, AprilUtils::UniquePtr<char> &result) {
    result.reset(new char[tmpdir.size() + TEMPLATE_SUFIX.size() + 1]);
    result = strcpy(result.get(), tmpdir.c_str());
    result = strcat(result.get(), TEMPLATE_SUFIX.c_str());
    return mkostemp(result.get(), flags);
  }

  void Config::setTemporaryDirectory(const char *tmpdir_) {
    tmpdir = tmpdir_;
  }
}
