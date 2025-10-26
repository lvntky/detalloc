#include <detalloc.h>

const char *det_version_string(void) { return "0.1.0"; }

det_config_t det_default_config(void) {
  det_config_t cfg;

  cfg.block_size = DET_DEFAULT_BLOCK_SIZE;
  cfg.num_blocks = 0;
  cfg.align = DET_DEFAULT_ALIGN;
  cfg.thread_safe = false;

  return cfg;
}
