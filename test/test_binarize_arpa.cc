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
  VocabDictionary voc(vocab_filename);
  BinarizeArpa obj(voc,arpa_filename);
  obj.processArpa();
  return 0;
}
