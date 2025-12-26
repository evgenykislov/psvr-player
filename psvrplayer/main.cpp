/*! Заглушка для запуска программы с предустановленными аргументами */

#include <cassert>

// Правильная точка входа
extern int psvrplayer_main(int argc, char** argv);

int main(int argc, char** argv) {
  char* custom_args[] = {argv[0], // обязательно должен быть на первом месте
      "--hotkeys",
      nullptr // обязательно должен быть на последнем месте
      };

  assert(sizeof(custom_args) >= (2 * sizeof(custom_args[0])));
  return psvrplayer_main(sizeof(custom_args) / sizeof(custom_args[0]) - 1, custom_args);


  return psvrplayer_main(argc, argv);
}
