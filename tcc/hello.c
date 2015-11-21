// sudo apt-get install tcc libtcc-dev
// gcc hello.c -o hello -ltcc -ldl

#include <libtcc.h>
#include <assert.h>
#include <stdio.h>

#define CODE "\
int f() {     \
  return 42;  \
}"

void error_handler(void* opaque, const char* msg) {
  fprintf (stderr, "tcc error: %s\n", msg);
}

int main() {
  TCCState *s = tcc_new();

  tcc_set_error_func(s, NULL, error_handler);
  tcc_set_lib_path(s, "/usr/lib/tcc");
  tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
  assert(tcc_compile_string(s, CODE) != -1);
  assert(tcc_relocate(s) != -1);
  int (*a)() = tcc_get_symbol(s, "f");
  printf("a is: %d\n", a());
  
  tcc_delete(s);
}
