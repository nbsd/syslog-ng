/*
 * Copyright (c) 2019-2026 Airbus Commercial Aircraft
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */


/* ---------------------------------------------------------------------
 * Crash Recovery (verifier) main
 *
 * Reads in existing encrypted log file and converts it according BN19 to
 * plain text
 *
 * [BN19] Erik-Oliver Blass and Guevara Noubir.
 * Forward Integrity and Crash Recovery for Secure Logs. Cryptology ePrint Archive, Paper 2019/506. 2019.
 * url: https://eprint.iacr.org/2019/506.
 *
 * This implementation was ported from Florian Hördt's master thesis example C++ code into C.
 * https://github.com/Genfood/secure-logging-cr
 *
 * Arguments
 * --key, -k:     Full file name (path) of the master key (same as initial key of the Logger)
 * --in, -i:      Full file name (path) of encrypted log file
 * --out, -o:     Full file name (path) of decrypted log file
 * --maxlogs, -m: The number of log lines the original plain log file provides
 *
 * Returns 0 on SUCCESS and non-zero on FAILURE (main logic)
 */

#include <stdio.h>
#include <locale.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cr_matrix.h"
#include "cr_crypto.h"
#include "cr_pi_types.h"
#include "cr_pi_verifier.h"
#include "cr_plain_gauss_helper.h"



int main(int argc, char *argv[])
{
  setlocale(LC_ALL, "");

  gboolean is_arg_key = FALSE;
  gboolean is_arg_logs = FALSE;
  gboolean is_arg_out = FALSE;
  gboolean is_arg_maxlogs = FALSE;

  if (1 > argc)
    {
      g_printerr("ERROR: cr_verifier: Wrong count of arguments!\n");
      return 1; //-- ERROR
    }

  GOptionEntry entries[] =
  {
    { "key", 'k', 0, G_OPTION_ARG_NONE, &is_arg_key, "Full file name (path) of the master key", NULL },
    { "in", 'i', 0, G_OPTION_ARG_NONE, &is_arg_logs, "Full file name (path) of encrypted log file", NULL },
    { "out", 'o', 0, G_OPTION_ARG_NONE, &is_arg_out, "Full file name (path) of decrypted log file", NULL },
    { "maxlogs", 'm', 0, G_OPTION_ARG_NONE, &is_arg_maxlogs, "Number of log lines the original plain log file provides", NULL },
    { 0 }
  };

  GError *error = NULL;
  GOptionContext *context;

  if (FALSE == cr_check_cpu_cfg())
    {
      g_printerr("\nERROR: CPU cfg. Check build system and CPU_AVX2, CPU_SSE2 or CPU_OTHER. See cr_plain_gauss_helper.h\n");
      return 1; //-- ERROR
    }

  //-- ensure memory alignemnt (not only when AVX2 CPU cfg is used)
  if ( (sizeof(cr_XOR_TYPE) % AVX2_ALIGNMENT) != 0)
    {
      g_printerr("ERROR: Wrong sizeof of cr_XOR_TYPE. Memory needs to be aligned.");
      return 1; //-- ERROR
    }

  context = g_option_context_new ("- verifier (Crash Recovery)");
  g_option_context_add_main_entries(context, entries, NULL);

  GString *gstrcpu = get_cpu_config_info(FALSE);
  g_option_context_set_summary(context, gstrcpu->str);

  if (!g_option_context_parse(context, &argc, &argv, &error))
    {
      // If parsing failed, print the error and exit.
      g_printerr("ERROR: Parsing options: %s\n", error->message);
      g_error_free(error);
      g_option_context_free(context);
      return 1; //-- ERROR
    }

  if ((5 != argc) && (FALSE == is_arg_key) && (FALSE == is_arg_logs) && (FALSE == is_arg_out)
      && (FALSE == is_arg_maxlogs))
    {
      g_printerr("ERROR: Expecting argc equal 4\n");
      g_option_context_free(context);
      return 1; //-- ERROR
    }


  cr_VerifierContext ctx;
  memset(&ctx, 0, sizeof(cr_VerifierContext));

  //-- check master key, --key, -k
  strncpy(ctx.masterKeyPath, argv[1], PATH_MAX - 1);
  ctx.masterKeyPath[PATH_MAX - 1] = '\0';
  if ( ! g_file_test(ctx.masterKeyPath, G_FILE_TEST_IS_REGULAR))
    {
      g_printerr("ERROR: Invalid full file name of master key: %s\n", ctx.masterKeyPath);
      g_option_context_free(context);
      return 1; //-- ERROR
    }

  //-- now ONE input file, --in, -i
  //-- check if path of encrypted input log file is valid
  strncpy(ctx.inEncFilePath, argv[2], PATH_MAX - 1);
  ctx.inEncFilePath[PATH_MAX - 1] = '\0';
  if ( ! g_file_test(ctx.inEncFilePath, G_FILE_TEST_IS_REGULAR))
    {
      g_printerr("ERROR: Invalid full file name of encrypted input log file: %s\n", ctx.inEncFilePath);
      g_option_context_free(context);
      return 1; //-- ERROR
    }

  //-- output file (decrypted log file), --out, -o
  //-- check if directory of given output file exists
  strncpy(ctx.outPlainFilePath, argv[3], PATH_MAX - 1);
  ctx.outPlainFilePath[PATH_MAX - 1] = '\0';
  gchar *dirname = g_path_get_dirname(ctx.outPlainFilePath);
  if (NULL != dirname)
    {
      if ( ! g_file_test(dirname, G_FILE_TEST_IS_DIR))
        {
          g_printerr("ERROR: Invalid out directory: %s of file %s\n", dirname, ctx.outPlainFilePath);
          g_option_context_free(context);
          g_free(dirname);
          return 1; //-- ERROR
        }
      // derive directory from output file - needed?
      strncpy(ctx.logFileDirectory, dirname, PATH_MAX - 1);
      char last = ctx.logFileDirectory[strlen(ctx.logFileDirectory)];
      if ( '/' != last )
        {
          strncat(ctx.logFileDirectory, "/", PATH_MAX - 1); //-- unify
        }
    }
  g_free (dirname);

  //-- write protocol in the same directory (ctx.logFileDirectory)
  //   derive name form input file ctx.inEncFilePath
  g_snprintf(ctx.outProtocolPath, PATH_MAX, "%s.verifier_protocol.txt", ctx.inEncFilePath);

  //-- check maxlogs, --maxlogs, -m
  int maxlogs;
  gchar *endptr;
  maxlogs = strtol(argv[4], &endptr, 10);
  if (*endptr != '\0')
    {
      g_printerr("ERROR: Invalid maxlogs, expected a number: %s\n", argv[4]);
      g_option_context_free(context);
      return 1; //-- ERROR
    }
  if (maxlogs <= 0)
    {
      g_printerr("ERROR: Out of range: maxlogs: %d\n", maxlogs);
      g_option_context_free(context);
      return 1; //-- ERROR
    }

  ctx.useMetal = FALSE; //-- no metal, no GPU, unused in the from C++ ported C project
  ctx.n = maxlogs; //-- count of log lines
  double temp_m = ceil(ctx.n * THE_C);
  ctx.m = (int) temp_m;

  g_print("key: %s\n", argv[1]);
  g_print("in: %s\n", argv[2]);
  g_print("out: %s\n", argv[3]);
  g_print("maxlogs: %s\n", argv[4]);

  g_option_context_free(context);

  g_print("ctx.masterKeyPath: %s\n", ctx.masterKeyPath);
  g_print("ctx.logFileDirectory: %s\n", ctx.logFileDirectory);
  g_print("ctx.inEncFilePath: %s\n", ctx.inEncFilePath);
  g_print("ctx.outPlainFilePath: %s\n", ctx.outPlainFilePath);
  g_print("ctx.outProtocolPath: %s\n", ctx.outProtocolPath);
  g_print("ctx.n: %d\n", ctx.n);
  g_print("ctx.m: %d\n", ctx.m);

  char szBuffer[256];
  memset(szBuffer, 0, sizeof(szBuffer));
  get_human_timestamp(szBuffer);
  g_print("%s\n", szBuffer);

  struct timespec start, end;
  start = get_ts_now();

  //-- create protocol output file here, it will be used here and in cr_pi_verifier.c ---
  ctx.protocolFile = fopen(ctx.outProtocolPath, "w");
  if (NULL == ctx.protocolFile)
    {
      g_warning("Can not create protocol output file!\n");
      return 1; //-- ERROR
    }
  get_human_timestamp(szBuffer);
  (void) fprintf(ctx.protocolFile, "%s, %s\n\n", "Verifier protocol", szBuffer);
  (void) fprintf(ctx.protocolFile, "%s\n", gstrcpu->str);
  (void) fprintf(ctx.protocolFile, "ctx.inEncFilePath: %s\nctx.outPlainFilePath: %s\nctx.outProtocolPath: %s\n",
                 ctx.inEncFilePath,
                 ctx.outPlainFilePath,
                 ctx.outProtocolPath);
  (void) fprintf(ctx.protocolFile, "ctx.n: %d\nctx.m: %d\n", ctx.n, ctx.m);


  // HERE THE MAGIC HAPPENS
  //
  // start the verification of the provided log file.
  cr_Result result = cr_Verify(&ctx);
  //
  //
  //

  //-- provide summary in protocol ---
  end = get_ts_now();
  const char  *szTextFin = "\n-- Crash Recovery Verifier (read + decrypt)";
  uint64_t ndiff = get_time_diff_in_milliseconds(start, end, szTextFin);
  add_to_protocol_timediff(&ctx, ndiff, szTextFin);
  get_human_timestamp(szBuffer);
  (void) fprintf(ctx.protocolFile, "\n%s\n", szBuffer);
  if (result.success == FALSE)
    {
      (void) fprintf(ctx.protocolFile, "Verify returns with an error!\n");
    }
  else
    {
      (void) fprintf(ctx.protocolFile, "Verify returns successful!\n");
    }
  cr_print_cpu_cfg();
  g_print("\n");
  memset(szBuffer, 0, sizeof(szBuffer));
  get_human_timestamp(szBuffer);
  g_print("%s\n", szBuffer);

  //-- clean up ---
  if (NULL != ctx.protocolFile)
    {
      fclose(ctx.protocolFile); //-- close protocol output file
      ctx.protocolFile = NULL;
    }
  if (NULL != gstrcpu)
    {
      g_string_free(gstrcpu, TRUE);
      gstrcpu = NULL;
    }

  //-- return value, main logic
  if (result.success == FALSE)
    return 1; //-- ERROR
  return 0;
}

