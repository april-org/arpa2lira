#include "../src/binarize_arpa.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr,"usage: %s vocab_filename arpa_filename output_prefix\n",argv[0]);
    exit(0);
  }
  const char *vocab_filename  = argv[1];
  const char *arpa_filename   = argv[2];
  const char *prefix_filename = argv[3];
  VocabDictionary voc(vocab_filename);
  BinarizeArpa obj(voc,arpa_filename);
  obj.processArpa(prefix_filename);
  return 0;
}


