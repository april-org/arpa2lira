#include "hat_trie.h"

class HAT_TRIE_DICT {
  hattrie_t* hat;
public:
  HAT_TRIE_DICT() {
    hat = hattrie_create();
  }
  ~HAT_TRIE_DICT() {
    hattrie_free(hat);
  }
  bool get(const char* k, size_t sz, int &value) {
    int *ptr = (int*)hattrie_tryget(hat, k, sz);
    if (ptr) {
      value = *ptr;
      return true;
    }
    return false;
  }
  bool exists(const char* k, size_t sz) {
    return hattrie_tryget(hat, k, sz);
  }
  void set(const char* k, size_t sz, int value) {
    int* ptr = (int*)hattrie_get(hat, k, sz);
    ptr[0] = value;
  }
};

