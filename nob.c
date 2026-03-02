#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD_FOLDER "build/"

int main(int argc, char **argv) {
  GO_REBUILD_URSELF(argc, argv);
  bool run = false;

  Cmd cmd = {0};

  if (!mkdir_if_not_exists(BUILD_FOLDER))
    return 1;

  cmd_append(&cmd, "cc");
  cmd_append(&cmd, "-Wall");
  cmd_append(&cmd, "-Wextra");
  cmd_append(&cmd, "-ggdb");
  cmd_append(&cmd, "-I./raylib-5.5_macos/include");
  cmd_append(&cmd, "-o", BUILD_FOLDER "main", "main.c");
  cmd_append(&cmd, "-L./raylib-5.5_macos/lib");
  cmd_append(&cmd, "-lraylib");
  cmd_append(&cmd, "-lm");
  cmd_append(&cmd, "-lpthread");
  cmd_append(&cmd, "-framework", "OpenGL");
  cmd_append(&cmd, "-framework", "CoreVideo");
  cmd_append(&cmd, "-framework", "IOKit");
  cmd_append(&cmd, "-framework", "Cocoa");
  cmd_append(&cmd, "-framework", "GLUT");
  cmd_append(&cmd, "-framework", "OpenGL");
  if (!cmd_run(&cmd))
    return 1;

  cmd_append(&cmd, BUILD_FOLDER "main");
  da_append_many(&cmd, argv, argc);
  if (!cmd_run(&cmd))
    return 1;

  return 0;
}
