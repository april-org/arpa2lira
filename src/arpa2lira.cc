#include "binarize_arpa.h"
#include "config.h"

using namespace Arpa2Lira;

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s vocab_filename arpa_filename lira_filename\n",
            argv[0]);
    exit(1);
  }
  const char *vocab_filename  = argv[1];
  const char *arpa_filename   = argv[2];
  const char *lira_filename   = argv[3];
  const char *begin_ccue      = "<s>";
  const char *end_ccue        = "</s>";
  BinarizeArpa obj(vocab_filename,arpa_filename,begin_ccue,end_ccue);
  obj.processArpa();
  obj.generate_lira(lira_filename);
  return 0;
}
