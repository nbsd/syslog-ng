#include "generate.h"
#include "sgroup.h"
#include "driver.h"
#include "apphook.h"

gint
generate_main(int argc, char *argv[])
{
  gchar *filename;
  PersistTool *self;

  if (!generate_output_dir || !config_file_generate)
    {
      fprintf(stderr,"config-file and output-dir are required options\n");
      return 1;
    }

  if (!g_file_test(generate_output_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
    {
      fprintf(stderr, "Directory doesn't exist: %s\n", generate_output_dir);
      return 1;
    }
  filename = g_build_path(G_DIR_SEPARATOR_S, generate_output_dir, DEFAULT_PERSIST_FILE, NULL);
  if (g_file_test(filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS) && !force_generate)
    {
      fprintf(stderr, "Persist file exists; filename = %s\n",filename);
      return 1;
    }
  else if (g_file_test(filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
    {
      if (unlink(filename) != 0)
        {
          fprintf(stderr, "Can't delete existing persist file; file = %s, error = %s\n", filename, strerror(errno));
          return 1;
        }
    }

  if (!g_file_test(config_file_generate, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
    {
      fprintf(stderr, "Given config file doesn't exists; filename = %s\n", config_file_generate);
      return 1;
    }

  app_startup();
  generate_persist_file = TRUE;
  self = persist_tool_new(filename, persist_mode_edit);
  if (cfg_read_config(self->cfg, config_file_generate, FALSE, NULL))
    {
      self->cfg->state = self->state;
      self->state = NULL;
      cfg_generate_persist_file(self->cfg, filename);
      fprintf(stderr,"New persist file generated: %s\n", filename);
      persist_state_commit(self->cfg->state);
    }
  persist_tool_free(self);
  g_free(filename);
  app_shutdown();
  return 0;
}
