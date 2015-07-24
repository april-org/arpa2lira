#include "../src/binarize_arpa.h"
#include "../src/config.h"

using namespace Arpa2Lira;

int main(int argc, char **argv) {
  if (argc != 5) {
    ERROR_EXIT1(1, "usage: %s vocab_filename arpa_filename lira_filename tmp_dir\n",
                argv[0]);
  }
  const char *vocab_filename  = argv[1];
  const char *arpa_filename   = argv[2];
  const char *lira_filename   = argv[3];
  const char *tmp_dir         = argv[4];
  Config::setTemporaryDirectory(tmp_dir);
  Config::setNumberOfThreads(4u);
  const char *begin_ccue = "<s>";
  const char *end_ccue = "</s>";
  BinarizeArpa obj(vocab_filename,arpa_filename,begin_ccue,end_ccue);
  fprintf(stderr,"objeto creado\n");
  obj.processArpa();
  obj.generate_lira(lira_filename);
  return 0;
}
