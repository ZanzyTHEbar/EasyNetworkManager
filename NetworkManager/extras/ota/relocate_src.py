# Redirects the project source dir to this folder for [env:esp32dev_extras_ota].
# PlatformIO 6.x reads [platformio] src_dir globally, so per-env relocation has
# to happen here, before $BUILD_SCRIPT collects sources.
Import("env")

env.Replace(PROJECT_SRC_DIR=env.subst("$PROJECT_DIR") + "/extras/ota")
