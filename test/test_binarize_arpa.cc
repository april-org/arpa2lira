#include "../src/binarize_arpa.h"
#include "../src/config.h"

using namespace Arpa2Lira;

int main(int argc, char **argv) {
  if (argc != 4) {
    ERROR_EXIT1(1, "usage: %s vocab_filename arpa_filename tmp_dir\n",
                argv[0]);
  }
  const char *vocab_filename  = argv[1];
  const char *arpa_filename   = argv[2];
  const char *tmp_dir         = argv[3];
  Config::setTemporaryDirectory(tmp_dir);
  Config::setNumberOfThreads(4u);
  VocabDictionary voc(vocab_filename);
  const char *begin_ccue = "<s>";
  const char *end_ccue = "<s>";
  BinarizeArpa obj(voc,arpa_filename,begin_ccue,end_ccue);
  fprintf(stderr,"objeto creado\n");
  obj.processArpa();
  return 0;
}
