#include "binarize_arpa.h"

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>           // mmap() is defined in this header
#include <fcntl.h>
#include <unistd.h>
}

BinarizeArpa::BinarizeArpa(VocabDictionary dict,
                           const char *inputFilename,
                           const char *outputPrefixFilename) : dict(dict) {
  ngramOrder = 0;
  // open file
  int file_descriptor = -1;
  assert((file_descriptor = open(inputFilename, O_RDONLY)) >= 0);
  // find size of input file
  struct stat statbuf;
  assert(fstat(file_descriptor,&statbuf) >= 0);
  int filesize = statbuf.st_size;
  // mmap the input file
  char *filemapped;
  assert((filemapped = (char*)mmap(0, filesize,
                                   PROT_READ, MAP_SHARED,
                                   file_descriptor, 0))  !=(caddr_t)-1);
  workingInput = inputFile = constString(filemapped, filesize);
}

void BinarizeArpa::processArpaHeader() {
  int level;
  constString cs;
  do {
    cs = workingInput.extract_line();
  } while (cs != "\\data\\");
  cs = workingInput.extract_line();
  while (cs.skip("ngram")) { //    ngram 1=103459
    assert(cs.extract_int(&level));
    assert(level== ++ngramOrder);
    cs.skip("=");
    assert(cs.extract_int(&counts[level-1]));
    fprintf(stderr,"%d -> %d\n",level,counts[level-1]);
    cs = workingInput.extract_line();
  }
}

template<int N, int M>
void BinarizeArpa::extractNgramLevel(const char *outputFilename) {
  // skip header
  char header[20];
  sprintf(header,"\\d-grams:",N);
  constString cs;
  do {
    cs = workingInput.extract_line();
  } while (!cs.is_prefix(header));
  
  int numNgrams = counts[N-1];
  char  *filemapped;

  // trying to open file in write mode
  int f_descr;
  mode_t writemode = S_IRUSR | S_IWUSR | S_IRGRP;
  if ((f_descr = open(outputFilename, O_RDWR | O_CREAT | O_TRUNC, writemode)) < 0) {
    fprintf(stderr,"Error creating file %s\n",outputFilename);
    exit(1);
  }
  size_t filesize = sizeof(Ngram<N,M>) * numNgrams;

  // make file of desired size:
  
  // go to the last byte position
  if (lseek(f_descr,filesize-1, SEEK_SET) == -1) {
    fprintf(stderr,"lseek error, position %u was tried\n",
            (unsigned int)(filesize-1));
    exit(1);
  }
  // write dummy byte at the last location
  if (write(f_descr,"",1) != 1) {
    fprintf(stderr,"write error\n");
    exit(1);
  }
  // mmap the output file
  if ((filemapped = (char*)mmap(0, filesize,
                                PROT_READ|PROT_WRITE, MAP_SHARED,
                                f_descr, 0))  == (caddr_t)-1) {
    fprintf(stderr,"mmap error\n");
    exit(1);
  }
  // aqui crear un vector de talla 
  Ngram<N,M> *p = (Ngram<N,M>*)filemapped;

  for (int i=0; i<numNgrams; ++i) {
    constString cs = workingInput.extract_line();
    float trans,bo;
    cs.extract_float(&trans);
    p[i].values[0] = arpa_prob(trans);
    cs.skip("\t");
    for (int j=0; j<N; ++j) {
      constString word = cs.extract_token();
      int wordId = dict(word);
      p[i].word[j] = wordId;
    }
    if (M>1) {
      if (!cs.extract_float(&bo)) {
        bo = logOne;
      }
      p[i].values[1] = arpa_prob(bo);
    }
  }

  // work done, free the resources ;)
  if (munmap(filemapped, filesize) == -1) {
    fprintf(stderr,"munmap error\n");
    exit(1);
  }
  close(f_descr);
}




