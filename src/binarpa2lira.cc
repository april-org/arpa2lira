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


// from APRIL
#include "error_print.h"
#include "qsort.h"
#include "smart_ptr.h"
#include "unused_variable.h"

BinArpa2Lira::BinArpa2Lira(int ngramOrder, int ngramcounts[]) :
  ngram2state(1<<20,4), // initial num_buckets, max_load_factor
  ngramOrder(ngramOrder),
  begin_cc(begin_cc),
  end_cc(end_cc)
{
  assert(ngramOrder <= maxNgramOrder);
  for (int i=0; i<ngramOrder; ++i)
    counts[i] = ngramcounts[i];
  num_states = 2; // first two states are reserved for zerogram and final states

}

void BinArpa2Lira::allocate_mmap_level(const char *mmapped_filename, int level) {
  // open file
  int &file_descriptor = mmaped_file_descriptor[level]; // alias
  size_t &filesize = mmaped_file_size[level]; // alias
  char* &filemapped = mmaped_file[level]; // alias
  assert((file_descriptor = open(mmapped_filename, O_RDONLY)) >= 0);
  // find size of input file
  struct stat statbuf;
  assert(fstat(file_descriptor,&statbuf) >= 0);
  filesize = statbuf.st_size;
  // mmap the input file
  assert((filemapped = (char*)mmap(0, filesize,
                                   PROT_READ, MAP_SHARED,
                                   file_descriptor, 0))  !=(caddr_t)-1);
  // we cannot remove the file for the moment, it must be mmapped
  // through the entire execution of this class since the hash table
  // may have pointers pointing inside
}

void BinArpa2Lira::release_mmap_level(int level) {
  int &file_descriptor = mmaped_file_descriptor[level]; // alias
  size_t &filesize = mmaped_file_size[level]; // alias
  char* &filemapped = mmaped_file[level]; // alias
  if (munmap(filemapped, filesize) == -1) {
    ERROR_EXIT("munmap error\n");
    exit(1);
  }
  close(file_descriptor);
}

void BinArpa2Lira::process_mmaped_buffer(const char *buffer, int level) {
  // buffer is a 
}
    
void BinArpa2Lira::process_level(const char *mmapped_filename, int level) {
  allocate_mmap_level(mmapped_filename,level);
  process_mmaped_buffer(mmaped_file[level],level);
}


BinArpa2Lira::~BinArpa2Lira() {
  // release resources
  for (levelnt level=0; level<ngramOrder; ++level) {
    release_level(level);
  }
}
