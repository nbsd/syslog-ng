/*
 * Copyright (c) 2024 Gergo Ferenc Kovacs
 * Copyright (c) 2025 Airbus Commercial Aircraft
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

#include <criterion/criterion.h>
#include "libtest/cr_template.h"
#include "libtest/msg_parse_lib.h"
#include "libtest/stopwatch.h"

#include "apphook.h"
#include "cfg.h"
#include "logmatcher.h"
#include "timeutils/cache.h"

#include <locale.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <glib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h> // Required for rand() and srand()
#include <time.h>   // Required for time() to seed the generator
#include <stdint.h> // For fixed-width integers like uint8_t

// Secure logging functions
#include <openssl/rand.h>
#include "cr_pi_shared.h"
#include "cr_pi_logger_context.h"
#include "cr_pi_logger.h"
#include "cr_pi_types.h"
#include "cr_result.h"
#include "cr_pi_verifier.h"
#include "utils_slog.h"
#include "slog.h"

#define MAX_TEST_MESSAGES 1000
#define MIN_TEST_MESSAGES 10
#define PERFORMANCE_COUNTER 100000

// Local parse options
static MsgFormatOptions test_parse_options;

// Filenames and directory templates
static gchar *testDirTmpl = "/tmp/slog-XXXXXX/";
static gchar *hostKeyFile = "host.key";
static gchar *macFile = "mac.dat";
static gchar *mac0File = "mac0.dat";

static gchar *sz_sfn_crmasterkey = "master.key";
static gchar *sz_sfn_crloggerout = "crlogger.enc";

// Test data for secure logging
static gchar *macAddr = "a08cefa7b520";
static gchar *serial = "CAC7119N43";
static gchar *prefix = "slog/";
static gchar *context_id = "test-context-id";

// Data needed to run a test for classic secure-logging
typedef struct _testData
{
  guchar hostKey[KEY_LENGTH];
  GString *testName;
  GString *keyFile;
  GString *macFile;
  GString *mac0File;
  GString *testDir;
  enum LogMode logmode;
} TestData;


// Data needed to run a test for Crash Recovery
typedef struct _testDataCR
{
  guchar masterkey[KEY_LENGTH];
  GString *gstr_test_name;
  GString *gstr_key_path;
  GString *gstr_dir_path;
  GString *gstr_enc_path;
} TestDataCR;


/*************************************************************************/
/* Utility functions needed for testing the secure logging functionality */
/*************************************************************************/

// Generate random number between low and high
int randomNumber(int low, int high)
{
  return rand() % ((high + 1) - low) + low;
}

// Function to fill a buffer with random bytes
void fill_buffer_random(uint8_t *buffer, size_t size)
{
  for (size_t i = 0; i < size; i++)
    {
      buffer[i] = (uint8_t)(rand() & 0xFF);
    }
}

// Generate a sample message with a fixed and a random part
LogMessage *create_random_sample_message(void)
{
  LogMessage *msg;

  GString *msg_str = g_string_new("<155>2019-07-11T10:34:56+01:00 aicorp syslog-ng[23323]:");

  // Append a random string
  int num = randomNumber(10, 500);
  for (int i = 0; i < num; i++)
    {
      // 65 to 90 are upper case letters
      g_string_append_c(msg_str, randomNumber(65, 90));
    }

  msg = msg_format_parse(&test_parse_options, (const guchar *) msg_str->str, msg_str->len);
  log_msg_set_saddr_ref(msg, g_sockaddr_inet_new("10.11.12.13", 1010));
  log_msg_set_match(msg, 0, "whole-match", -1);
  log_msg_set_match(msg, 1, "first-match", -1);
  log_msg_set_tag_by_name(msg, "alma");
  log_msg_set_tag_by_name(msg, "korte");
  log_msg_clear_tag_by_name(msg, "narancs");
  log_msg_set_tag_by_name(msg, "citrom");
  log_msg_set_tag_by_name(msg, "tag,containing,comma");
  msg->rcptid = 555;
  msg->host_id = 0xcafebabe;

  // Fix some externally or automatically defined values
  log_msg_set_value(msg, LM_V_HOST_FROM, "kismacska", -1);
  msg->timestamps[LM_TS_RECVD].ut_sec = 1139684315;
  msg->timestamps[LM_TS_RECVD].ut_usec = 639000;
  msg->timestamps[LM_TS_RECVD].ut_gmtoff = get_local_timezone_ofs(1139684315);

  g_string_free(msg_str, TRUE);
  return msg;
}


LogMessage *create_random_sample_message_with_special_symbols(void)
{
  LogMessage *msg;

  GDateTime *now = g_date_time_new_now_local();
  gchar *iso_string = g_date_time_format(now, "%Y-%m-%dT%H:%M:%S%:z");
  GString *timestamp_gstr = g_string_new(iso_string);
  GString *msg_gstr = g_string_new("<155>");
  g_string_append(msg_gstr, timestamp_gstr->str);
  g_string_append(msg_gstr, " aicorp syslog-ng[23323]:");
  // Append a random string
  int num = randomNumber(10, 137);
  for (int i = 0; i < num; i++)
    {
      // 65 to 90 are upper case letters
      g_string_append_c(msg_gstr, randomNumber(65, 90));
    }
  GString *utf8_gstr =
    g_string_new(" Hello, World! 🌍, Voilà! Fröhliche Grüße aus Düsseldorf, Straße № 1, ßäöüßÄÖÜ. ¿Qué pasa, señor! (Special chars: !@#$%^&*§©®™) 🤪");
  g_string_append(msg_gstr, utf8_gstr->str);

  msg = msg_format_parse(&test_parse_options, (const guchar *) msg_gstr->str, msg_gstr->len);
  log_msg_set_saddr_ref(msg, g_sockaddr_inet_new("10.11.12.13", 1010));
  log_msg_set_match(msg, 0, "whole-match", -1);
  log_msg_set_match(msg, 1, "first-match", -1);
  log_msg_set_tag_by_name(msg, "alma");
  log_msg_set_tag_by_name(msg, "korte");
  log_msg_clear_tag_by_name(msg, "narancs");
  log_msg_set_tag_by_name(msg, "citrom");
  log_msg_set_tag_by_name(msg, "tag,containing,comma");
  msg->rcptid = 555;
  msg->host_id = 0xcafebabe;

  // Fix some externally or automatically defined values
  log_msg_set_value(msg, LM_V_HOST_FROM, "kismacska", -1);
  msg->timestamps[LM_TS_RECVD].ut_sec = 1139684315;
  msg->timestamps[LM_TS_RECVD].ut_usec = 639000;
  msg->timestamps[LM_TS_RECVD].ut_gmtoff = get_local_timezone_ofs(1139684315);

  g_free(iso_string);
  g_date_time_unref(now);
  g_string_free(timestamp_gstr, TRUE);
  g_string_free(utf8_gstr, TRUE);
  g_string_free(msg_gstr, TRUE);

  return msg;
}


// Create a slog template instance
LogTemplate *createTemplate(TestData *testData, enum LogMode logmode)
{
  GString *slog_templ_str = g_string_new("slog");
  // g_print("createTemplate logmode: %d\n", (gint)logmode);

  // Initialize the template
  if (LOGMODE_ENCRYPTED == logmode)
    {
      g_string_printf(slog_templ_str, "$(slog -k %s -m %s --logmode enc $RAWMSG)", testData->keyFile->str,
                      testData->macFile->str);
    }
  else if (LOGMODE_PLAIN_DIRECT == logmode)
    {
      g_string_printf(slog_templ_str, "$(slog -k %s -m %s --logmode direct $RAWMSG)", testData->keyFile->str,
                      testData->macFile->str);
    }
  else if (LOGMODE_PLAIN_BASE64 == logmode)
    {
      g_string_printf(slog_templ_str, "$(slog -k %s -m %s --logmode base64 $RAWMSG)", testData->keyFile->str,
                      testData->macFile->str);
    }
  else
    {
      cr_assert(FALSE, "Wrong LogMode");
    }

  LogTemplate *slog_templ = compile_template(slog_templ_str->str);

  cr_assert(slog_templ != NULL, "Template '%s' does not compile correctly", slog_templ_str->str);

  g_string_free(slog_templ_str, TRUE);

  return slog_templ;
}

// Create a collection of random log messages for testing purposes
void createLogMessages(gint num, LogMessage **log, gint variant)
{
  if (num <= 0)
    {
      cr_log_error("Invalid argument passed to createLog. num = %d", num);
    }
  for (int i = 0; i < num; i++)
    {
      if (0 == variant)
        {
          log[i] = create_random_sample_message();
        }
      else
        {
          log[i] = create_random_sample_message_with_special_symbols();
        }
    }
}


// Apply the template to a single log message
GString *applyTemplate(LogTemplate *templ, LogMessage *msg)
{
  GString *output = g_string_new(prefix);

  LogTemplateEvalOptions options = {NULL, LTZ_LOCAL, 999, context_id, LM_VT_STRING, log_template_default_escape_method}; //-- TODO clarify escape
  // Execute secure logging template
  log_template_append_format_with_context(
    templ,       // Secure logging template
    &msg,        // Message(s) to pass to the template
    1,           // Number of message to pass to the template
    &options,
    output);     // Output string after applying the template

  return output;
}

// Find an integer in an array of integers
int findInArray(int index, int *buffer, int size)
{
  for (int i = 0; i < size; i++)
    {
      if (buffer[i] == index)
        {
          return 1;
        }
    }
  return 0;
}

// helper for verifyMaliciousMessages and verifyMessages to clean-up correctly
static void gstring_destroy (gpointer data)
{
  g_string_free (data, TRUE); //-- TRUE -> also free the underlying buffer
}

// Verify messages with malicious modification and detect which entry is corrupted
GString **verifyMaliciousMessages(guchar *hostkey, gchar *macFileName, GString **templateOutput,
                                  size_t totalNumberOfMessages, int *brokenEntries, enum LogMode logmode)
{
  cr_assert(totalNumberOfMessages > 0, "Total number of message must be >0");

  guchar keyZero[KEY_LENGTH];
  memcpy(keyZero, hostkey, KEY_LENGTH);

  guint64 next = 0;
  guint64 start = 0;
  guint64 numberOfLogEntries = 0UL;

  GString **outputBuffer = g_new0(GString *, totalNumberOfMessages);

  guchar mac[CMAC_LENGTH];

  gboolean ret = readAggregatedMAC(macFileName, mac);
  cr_assert(ret == TRUE, "Unable to read aggregated MAC from file %s", macFileName);

  int problemsFound = 0;
  guchar cmac_tag[CMAC_LENGTH];
  gsize cmac_tag_capacity = G_N_ELEMENTS(cmac_tag);

  GPtrArray *tmpTemplate = g_ptr_array_new();
  g_ptr_array_add(tmpTemplate, templateOutput[0]);

  GHashTable *tab = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
  cr_assert_not_null(tab, "Can not create GHashTable");

  initVerify(totalNumberOfMessages, hostkey, &next, &start, tmpTemplate);
  g_ptr_array_free(tmpTemplate, TRUE);

  GPtrArray *template = g_ptr_array_new();
  GPtrArray *output = g_ptr_array_new_with_free_func (gstring_destroy);

  for (size_t i = 0; i < totalNumberOfMessages; i++)
    {
      g_ptr_array_add(template, templateOutput[i]);
      g_ptr_array_add(output, g_string_new(NULL));

      ret = iterateBuffer(1, template, &next, hostkey, keyZero, 0, output, &numberOfLogEntries, cmac_tag,
                          cmac_tag_capacity, tab, logmode);
      if (ret == FALSE)
        {
          brokenEntries[problemsFound] = i;
          problemsFound++;
        }
      g_ptr_array_remove_index(template, 0);
      g_ptr_array_remove_index(output, 0);
    }

  ret = finalizeVerify(start, totalNumberOfMessages, mac, cmac_tag, &tab);

  cr_assert(ret == FALSE, "Aggregated MAC is correct.");

  g_ptr_array_free(template, FALSE);
  g_ptr_array_free(output, TRUE);

  return outputBuffer;
}

// Verify log messages and compare them with the original
void verifyMessages(guchar *hostkey, gchar *macFileName, GString **templateOutput, LogMessage **original,
                    gsize totalNumberOfMessages, enum LogMode logmode)
{
  guchar keyZero[KEY_LENGTH];
  memcpy(keyZero, hostkey, KEY_LENGTH);

  guint64 next = 0;
  guint64 start = 0;
  guint64 numberOfLogEntries = 0UL;

  GPtrArray *template = g_ptr_array_new();

  for (gsize i = 0; i < totalNumberOfMessages; i++)
    {
      g_ptr_array_add(template, templateOutput[i]);
    }

  GHashTable *tab = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
  cr_assert_not_null(tab, "Can not create GHashTable");

  gboolean b = initVerify(totalNumberOfMessages, hostkey, &next, &start, template);
  cr_assert(b == TRUE, "Init verify returns FALSE.");

  GPtrArray *output = g_ptr_array_new_with_free_func (gstring_destroy);

  guchar mac[CMAC_LENGTH];

  gboolean ret = readAggregatedMAC(macFileName, mac);
  cr_assert(ret == TRUE, "Unable to read aggregated MAC from file %s", macFileName);

  guchar cmac_tag[CMAC_LENGTH];
  gsize cmac_tag_capacity = G_N_ELEMENTS(cmac_tag);
  ret = initVerify(totalNumberOfMessages, hostkey, &next, &start, template);
  cr_assert(ret == TRUE, "initVerify failed");

  //------------ initial MAC file, mac0.dat
  char pathMac0[PATH_MAX]; //-- full path of MAC0 file mac0.dat
  ret = get_path_mac0(macFileName, pathMac0, PATH_MAX);
  cr_assert(ret == TRUE, "Unable to get path of mac0.dat");
  guchar MAC0[CMAC_LENGTH]; //-- initial MAC
  memset(MAC0, 0, CMAC_LENGTH);
  ret = readAggregatedMAC(pathMac0, MAC0);
  cr_assert(ret == TRUE, "Unable to read initial MAC from file %s", pathMac0);
  memcpy(cmac_tag, MAC0, CMAC_LENGTH); //-- cmac_tag provides the initial MAC mac0
  //------------

  g_print("totalNumberOfMessages: %ld, logmode: %d\n", totalNumberOfMessages, (gint)logmode);
  ret = iterateBuffer(totalNumberOfMessages, template, &next, hostkey, keyZero, 0, output,
                      &numberOfLogEntries, cmac_tag, cmac_tag_capacity, tab, logmode);
  cr_assert(ret == TRUE, "iterateBuffer failed");

  ret = finalizeVerify(start, totalNumberOfMessages, (guchar *)mac, cmac_tag, &tab);
  cr_assert(ret == TRUE, "finalizeVerify failed");

  for (size_t i = 0; i < totalNumberOfMessages; i++)
    {
      GString *str = (GString *)g_ptr_array_index(output, i);
      char *plaintextMessage = (str->str) + CTR_LEN_SIMPLE + COLON + BLANK;
      LogMessage *result = msg_format_parse(&test_parse_options, (const guchar *) plaintextMessage, strlen(plaintextMessage));
      log_msg_set_saddr(result, original[i]->saddr);
      assert_log_messages_equal(original[i], result);
      log_msg_unref(result);
    }

  g_ptr_array_free(template, FALSE);
  g_ptr_array_free(output, TRUE);
}

// Generate keys to be used for the tests
void generateHostKey(guchar *hostkey, gchar *hostKeyFileName)
{
  // Create keys for the test
  guchar masterkey[KEY_LENGTH];
  gboolean ret = generateMasterKey(masterkey);
  cr_assert(ret, "Unable to generate master key");

  deriveHostKey(masterkey, macAddr, serial, hostkey);
  cr_assert(hostkey != NULL, "Unable to derive host key from master key for addr %s and serial number %s", macAddr,
            serial);

  ret = writeKey(hostkey, 0, hostKeyFileName);
  cr_assert(ret, "Unable to write host key to file %s", hostKeyFileName);
}

// Create a temporary directory
GString *createTemporaryDirectory(gchar *template)
{
  gchar buf[PATH_MAX];

  // Buffer for temporary path
  g_strlcpy(buf, template, strlen(template) + 1);

  // Create random directory
  gchar *tmpDir = g_mkdtemp(buf);

  cr_assert(tmpDir != NULL, "Unable to create temporary directory %s: %s", template, strerror(errno));

  GString *result = g_string_new(tmpDir);

  return result;
}

// Create a fully qualified path to a temporary file
GString *createTemporaryFilePath(GString *dirname, gchar *basename)
{
  GString *filePath = g_string_new(dirname->str);

  g_string_append(filePath, basename);

  return filePath;
}

// Delete temporary file generated by a test
void removeTemporaryFile(gchar *fileName, gboolean force)
{
  // Remove file
  int ret = unlink(fileName);
  if (!force && ret != 0)
    {
      cr_log_info("removeTemporaryFile %s: %s", strerror(errno), fileName);
    }
}

// Remove temporary directory generated by a test
void removeTemporaryDirectory(gchar *dirName, gboolean force)
{
  // Remove directory
  int ret = rmdir(dirName);
  if (!force && ret != 0)
    {
      cr_log_info("removeTemporaryDirectory %s: %s", strerror(errno), dirName);
    }
  if (g_file_test(dirName, G_FILE_TEST_IS_DIR) && (TRUE == force))
    {
      //-- another way to remove a directory ---
      char szCmd[PATH_MAX];
      g_snprintf(szCmd, sizeof(szCmd), "sync");
      ret = system(szCmd);
      if (0 == ret)
        {
          cr_log_info("Command %s executed successfully.", szCmd);
        }
      else
        {
          cr_log_info("Command %s returns %d", szCmd, ret);
        }
      g_snprintf(szCmd, sizeof(szCmd), "rm -rf %s", dirName);
      ret = system(szCmd);
      if (0 == ret)
        {
          cr_log_info("Command %s executed successfully.", szCmd);
        }
      else
        {
          cr_log_info("Command %s returns %d", szCmd, ret);
        }
      if (g_file_test(dirName, G_FILE_TEST_IS_DIR))
        {
          cr_log_info("Temporary directory was not deleted: %s", dirName);
        }
    }
}

// Initialize a test
TestData *initialize(gchar *name, enum LogMode logmode)
{
  setlocale(LC_ALL, "");
  cr_log_info("[%s] Initialization", name);

  TestData *testData = g_new0(TestData, 1);

  testData->testName = g_string_new(name);
  testData->testDir = createTemporaryDirectory(testDirTmpl);
  testData->keyFile = createTemporaryFilePath(testData->testDir, hostKeyFile);
  testData->macFile = createTemporaryFilePath(testData->testDir, macFile);
  testData->mac0File = createTemporaryFilePath(testData->testDir, mac0File);
  testData->logmode = logmode;
  generateHostKey(testData->hostKey, testData->keyFile->str);

  return testData;
}

// Close a test and free resources
void closure(TestData *testData)
{
  cr_log_info("[%s] Closure", testData->testName->str);

  removeTemporaryFile(testData->keyFile->str, TRUE);
  removeTemporaryFile(testData->macFile->str, TRUE);
  removeTemporaryFile(testData->mac0File->str, TRUE);
  removeTemporaryDirectory(testData->testDir->str, TRUE);

  g_string_free(testData->testName, TRUE);
  g_string_free(testData->testDir, TRUE);
  g_string_free(testData->keyFile, TRUE);
  g_string_free(testData->macFile, TRUE);
  g_string_free(testData->mac0File, TRUE);

  g_free(testData);
}

//-- helper clean-up function which pointer is used when GPtrArray for GString* is created
static void unref_gstring_wrapper(void *data)
{
  if (data)
    g_string_free((GString *)data, TRUE);
}

//-- helper for writing binary data to file, used for Crash Recovery cr_logger test
static int helper_cr_test_write_binary_file(const char *file_path, const unsigned char *data, size_t length)
{
  FILE *file = fopen(file_path, "wb"); // "wb" for write binary
  if (file == NULL)
    {
      perror("Failed to open file");
      return -1; //-- ERROR
    }
  size_t bytes_written = fwrite(data, 1, length, file);
  if (bytes_written != length)
    {
      perror("Failed to write data");
      fclose(file);
      return -1; //-- ERROR
    }
  fclose(file);
  return 0; //-- Success
}

//-- test guard begin for Crash Recovery tests
TestDataCR *initialize_cr(gchar *name)
{
  setlocale(LC_ALL, "");
  cr_log_info("[%s] Initialization", name);
  //-- path gedöns
  TestDataCR *td = g_new0(TestDataCR, 1);
  td->gstr_test_name = g_string_new(name);
  td->gstr_dir_path = createTemporaryDirectory(testDirTmpl);
  td->gstr_key_path = createTemporaryFilePath(td->gstr_dir_path, sz_sfn_crmasterkey);
  td->gstr_enc_path = createTemporaryFilePath(td->gstr_dir_path, sz_sfn_crloggerout);
  //-- key
  RAND_bytes(td->masterkey, KEY_LENGTH);
  if (helper_cr_test_write_binary_file(td->gstr_key_path->str, td->masterkey, KEY_LENGTH) != 0)
    {
      cr_assert(FALSE, "Failed to write master key to file");
    }
  return td;
}

//-- test guard end for Crash Recovery tests
void closure_cr(TestDataCR *td)
{
  cr_log_info("[%s] Closure", td->gstr_test_name->str);
  removeTemporaryFile(td->gstr_key_path->str, TRUE);
  removeTemporaryFile(td->gstr_enc_path->str, TRUE);
  removeTemporaryDirectory(td->gstr_dir_path->str, TRUE);
  g_string_free(td->gstr_key_path, TRUE);
  g_string_free(td->gstr_enc_path, TRUE);
  g_string_free(td->gstr_dir_path, TRUE);
  g_string_free(td->gstr_test_name, TRUE);
  g_free(td);
}

//-- Helper to create GStirng with test info and log mode
//   in testinfo
//   in logmode
//   Caller is owner of returned GString* and has to call later
//   g_string_free(gstrTest, TRUE);
GString *concat_testinfo_logmode(const char *testinfo, enum LogMode logmode)
{
  GString *gstrTest = g_string_new(testinfo);
  switch (logmode)
    {
    case LOGMODE_ENCRYPTED:
      g_string_append(gstrTest, "_LOGMODE_ENCRYPTED");
      break;
    case LOGMODE_PLAIN_DIRECT:
      g_string_append(gstrTest, "_LOGMODE_PLAIN_DIRECT");
      break;
    case LOGMODE_PLAIN_BASE64:
      g_string_append(gstrTest, "_LOGMODE_PLAIN_BASE64");
      break;
    default:
      cr_assert(false, "Invalid enum LogMode value %d, testinfo: %s", (gint) logmode, testinfo);
    }
  cr_log_info("Generated string: %s", gstrTest->str);
  return gstrTest; //-- caller must call later g_string_free(gstrTest, TRUE);
}

void corruptKey(TestData *testData)
{
  GError *error = NULL;
  GIOChannel *keyfile = g_io_channel_new_file(testData->keyFile->str, "w+", &error);

  cr_assert(keyfile != NULL, "Cannot open key file: %s", testData->keyFile->str);

  GIOStatus status = g_io_channel_set_encoding(keyfile, NULL, &error);

  cr_assert(status == G_IO_STATUS_NORMAL, " Unable to set encoding for key file %s", testData->keyFile->str);

  gsize outlen = 0;

  int buflen = KEY_LENGTH + CMAC_LENGTH + sizeof(guint64);

  gchar data[buflen];

  // Overwrite the first 8 byte of the key with random values
  for (int i = 0; i < buflen; i++)
    {
      data[i] = randomNumber(1, 128);
    }

  // Overwrite the first 8 byte of the key with random values
  for (int i = 0; i < 8; i++)
    {
      testData->hostKey[i] = randomNumber(1, 128);
    }

  // Copy the corrupted key to the buffer
  memcpy(data, testData->hostKey, KEY_LENGTH);

  // Write garbage to key file
  status = g_io_channel_write_chars(keyfile, data, buflen, &outlen, &error);

  cr_assert(status == G_IO_STATUS_NORMAL, "Unable to write updated key to file %s", testData->keyFile->str);

  status = g_io_channel_shutdown(keyfile, TRUE, &error);
  g_io_channel_unref(keyfile);

  cr_assert(status == G_IO_STATUS_NORMAL, " Unable to close key file %s", testData->keyFile->str);
}


/*************************************************************************/
/* Unit test setup and teardown                                          */
/*************************************************************************/

void setup(void)
{
  srand(time(NULL));
  app_startup();
  init_parse_options_and_load_syslogformat(&test_parse_options);

  // This flag is required in order to pass the unaltered message to the slog template
  // It is required to be set after the initialization of the template tests above,
  // as this sets the parse options to the defaults
  test_parse_options.flags |= LP_STORE_RAW_MESSAGE;

  cfg_load_module(configuration, "secure-logging");
}

void teardown(void)
{
  deinit_template_tests();
  app_shutdown();
}

/*************************************************************************/
/* Test suite                                                            */
/*************************************************************************/
TestSuite(secure_logging, .init = setup, .fini = teardown);

void test_slog_template_format(void)
{
  g_print("-- test_slog_template_format\n");
  enum LogMode logmode = LOGMODE_ENCRYPTED;
  TestData *testData = initialize("test_slog_template_format", logmode);

  GString *templ = g_string_new("");

  // $(slog -k keyfile)
  g_string_printf(templ, "$(slog -k %s)", testData->keyFile->str);
  assert_template_failure(templ->str, SLOG_ERROR_PREFIX ": Template parsing failed. Invalid number of arguments");

  // $(slog -k keyfile -m)
  g_string_printf(templ, "$(slog -k %s -m)", testData->keyFile->str);
  assert_template_failure(templ->str, "Missing argument for -m");

  // $(slog -k -m macfile)
  g_string_printf(templ, "$(slog -k -m %s)", testData->macFile->str);
  assert_template_failure(templ->str, "Invalid path or non existing regular file: -m");

  // $(slog -k keyfile -m macfile)
  g_string_printf(templ, "$(slog -k %s -m %s)", testData->keyFile->str, testData->macFile->str);
  assert_template_failure(templ->str, SLOG_ERROR_PREFIX ": Template parsing failed. Invalid number of arguments");

  // template slog {
  //      template("$(slog --key-file `mypath`/host.key --mac-file `mypath`/mac.dat --logmode direct $RAWMSG)\n");
  // };

  testData->logmode = LOGMODE_PLAIN_DIRECT;
  g_string_printf(templ, "$(slog -k %s -m %s --logmode direct)", testData->keyFile->str, testData->macFile->str);
  assert_template_failure(templ->str, SLOG_ERROR_PREFIX ": Template parsing failed. Invalid number of arguments");

  testData->logmode = LOGMODE_PLAIN_BASE64;
  g_string_printf(templ, "$(slog -k %s -m %s --logmode base64)", testData->keyFile->str, testData->macFile->str);
  assert_template_failure(templ->str, SLOG_ERROR_PREFIX ": Template parsing failed. Invalid number of arguments");

  testData->logmode = LOGMODE_ENCRYPTED;
  g_string_printf(templ, "$(slog -k %s -m %s --logmode enc)", testData->keyFile->str, testData->macFile->str);
  assert_template_failure(templ->str, SLOG_ERROR_PREFIX ": Template parsing failed. Invalid number of arguments");

  g_string_free(templ, TRUE);

  closure(testData);
}




//-- verification

void common_slog_verification(enum LogMode logmode)
{
  GString *gstrTest = concat_testinfo_logmode("test_slog_verification", logmode);
  TestData *testData = initialize(gstrTest->str, logmode);
  LogMessage *msg = create_random_sample_message();
  LogTemplate *slog_templ = createTemplate(testData, logmode);
  GString *output = applyTemplate(slog_templ, msg);
  size_t num = 1;
  verifyMessages(testData->hostKey, testData->macFile->str, &output, &msg, num, logmode);
  log_template_unref(slog_templ);
  closure(testData);
  g_string_free(gstrTest, TRUE);
}

void test_slog_verification(void)
{
  enum LogMode logmode = LOGMODE_ENCRYPTED;
  common_slog_verification(logmode);
}

void test_slog_verification_plain_direct(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_DIRECT;
  common_slog_verification(logmode);
}

void test_slog_verification_plain_base64(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_BASE64;
  common_slog_verification(logmode);
}

//-- verification_bulk

void common_slog_verification_bulk(enum LogMode logmode)
{
  GString *gstrTest = concat_testinfo_logmode("test_slog_verification_bulk", logmode);
  TestData *testData = initialize(gstrTest->str, logmode);
  LogTemplate *slog_templ = createTemplate(testData, logmode);

  // Create a collection of log messages
  size_t num = randomNumber(MIN_TEST_MESSAGES, MAX_TEST_MESSAGES);
  LogMessage **logs = g_new0(LogMessage *, num);

  createLogMessages(num, logs, 0);

  // Template output
  GString **output = g_new0(GString *, num);

  // Apply slog template to each message
  for (size_t i = 0; i < num; i++)
    {
      output[i] = applyTemplate(slog_templ, logs[i]);
    }

  // Verify the previously created log
  verifyMessages(testData->hostKey, testData->macFile->str, output, logs, num, logmode);

  // Release message resources
  for (size_t i = 0; i < num; i++)
    {
      log_msg_unref(logs[i]);
      g_string_free(output[i], TRUE);
    }

  log_template_unref(slog_templ);
  g_free(output);
  g_free(logs);

  closure(testData);
  g_string_free(gstrTest, TRUE);
}

void test_slog_verification_bulk(void)
{
  enum LogMode logmode = LOGMODE_ENCRYPTED;
  common_slog_verification_bulk(logmode);
}

void test_slog_verification_bulk_plain_direct(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_DIRECT;
  common_slog_verification_bulk(logmode);
}

void test_slog_verification_bulk_plain_base64(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_DIRECT;
  common_slog_verification_bulk(logmode);
}




//-- Key

void common_slog_corrupted_key(enum LogMode logmode)
{
  GString *gstrTest = concat_testinfo_logmode("test_slog_corrupted_key", logmode);
  TestData *testData = initialize(gstrTest->str, logmode);

  // Part 1: Log several messages -> They must be encrypted
  // Part 2: Corrupt the key
  // Log several messages -> They should be logged in plain text

  LogTemplate *slog_templ = createTemplate(testData, logmode);

  // Create a collection of log messages
  size_t num = randomNumber(MIN_TEST_MESSAGES, MAX_TEST_MESSAGES);

  LogMessage **logs = g_new0(LogMessage *, num);
  createLogMessages(num, logs, 0);

  GString **output = g_new0(GString *, num);

  // Part 1: Apply slog template to each message
  for (size_t i = 0; i < num; i++)
    {
      output[i] = applyTemplate(slog_templ, logs[i]);
    }

  // Verify messages
  verifyMessages(testData->hostKey, testData->macFile->str, output, logs, num, logmode);

  // Release message resources
  for (size_t i = 0; i < num; i++)
    {
      log_msg_unref(logs[i]);
      g_string_free(output[i], TRUE);
    }

  log_template_unref(slog_templ);
  g_free(logs);
  g_free(output);

  // Part 2: Corrupt the key
  corruptKey(testData);

  // Re-initialize the template
  slog_templ = createTemplate(testData, logmode);

  // Create a collection of log messages
  num = randomNumber(MIN_TEST_MESSAGES, MAX_TEST_MESSAGES);
  logs = g_new0(LogMessage *, num);
  createLogMessages(num, logs, 0);

  output = g_new0(GString *, num);

  // Apply slog template to each message
  for (size_t i = 0; i < num; i++)
    {
      output[i] = applyTemplate(slog_templ, logs[i]);

      // Create new message from the text content of the original message
      LogMessage *myOut = msg_format_parse(&test_parse_options, (const guchar *) output[i]->str, output[i]->len);

      // Initialize the new message with value from the original
      log_msg_set_saddr(myOut, logs[i]->saddr);
      myOut->timestamps[LM_TS_STAMP].ut_sec = logs[i]->timestamps[LM_TS_STAMP].ut_sec;
      myOut->timestamps[LM_TS_STAMP].ut_usec = logs[i]->timestamps[LM_TS_STAMP].ut_usec;
      myOut->timestamps[LM_TS_STAMP].ut_gmtoff = logs[i]->timestamps[LM_TS_STAMP].ut_gmtoff;
      myOut->pri = logs[i]->pri;
      gssize dlen;
      log_msg_set_value(myOut, LM_V_HOST, log_msg_get_value(logs[i], LM_V_HOST, &dlen), -1);
      log_msg_set_value(myOut, LM_V_PROGRAM, log_msg_get_value(logs[i], LM_V_PROGRAM, &dlen), -1);
      log_msg_set_value(myOut, LM_V_MESSAGE, log_msg_get_value(logs[i], LM_V_MESSAGE, &dlen), -1);
      log_msg_set_value(myOut, LM_V_PID, log_msg_get_value(logs[i], LM_V_PID, &dlen), -1);
      log_msg_set_value(myOut, LM_V_MSGID, log_msg_get_value(logs[i], LM_V_MSGID, &dlen), -1);
      assert_log_messages_equal(myOut, logs[i]);

      // Release message resources
      log_msg_unref(myOut);
      log_msg_unref(logs[i]);
      g_string_free(output[i], TRUE);
    }

  log_template_unref(slog_templ);
  g_free(output);
  g_free(logs);

  closure(testData);
  g_string_free(gstrTest, TRUE);
}

void test_slog_corrupted_key(void)
{
  enum LogMode logmode = LOGMODE_ENCRYPTED;
  common_slog_corrupted_key(logmode);
}

void test_slog_corrupted_key_plain_direct(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_DIRECT;
  common_slog_corrupted_key(logmode);
}

void test_slog_corrupted_key_plain_base64(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_BASE64;
  common_slog_corrupted_key(logmode);
}


//-- malicous_modifications

void common_slog_malicious_modifications(enum LogMode logmode)
{
  GString *gstrTest = concat_testinfo_logmode("test_slog_malicious_modifications", logmode);
  TestData *testData = initialize(gstrTest->str, logmode);

  LogTemplate *slog_templ = createTemplate(testData, logmode);

  // Create a collection of log messages
  size_t num = randomNumber(MIN_TEST_MESSAGES, MAX_TEST_MESSAGES);

  LogMessage **logs = g_new0(LogMessage *, num);
  g_print("Num: %lu\n", num);
  createLogMessages(num, logs, 0);

  // Template output
  GString **output = g_new0(GString *, num);

  // Apply slog template to each message
  for (size_t i = 0; i < num; i++)
    {
      output[i] = applyTemplate(slog_templ, logs[i]);
    }


  int mods = randomNumber(1, num - 1);
  int entriesToModify[mods];

  mods = 1;

  // We might modify the same entry twice
  for (int i = 0; i < mods; i++)
    {
      entriesToModify[i] = randomNumber(0, num - 1);
      g_print("MODIFYING ENTRY %d\n", entriesToModify[i]);


      // Overwrite with invalid string (invalid with high probability!)
      g_string_overwrite(output[entriesToModify[i]], randomNumber(COUNTER_LENGTH + COLON,
                                                                  (output[entriesToModify[i]]->len) - 1), "999999999999999999999999999999999999999999999999999999999999999");
    }

  // Verify the previously created log
  int brokenEntries[mods];
  for (int i = 0; i < mods; i++)
    {
      brokenEntries[i] = -1;
    }
  GString **ob = verifyMaliciousMessages(testData->hostKey, testData->macFile->str, output, num, brokenEntries, logmode);

  for (gsize i = 0; i < num; i++)
    {
      if (1 == findInArray(i, entriesToModify, mods))
        {
          cr_assert(1 == findInArray(i, brokenEntries, mods), "Modified entry %lu not detected.", i);
        }
      else
        {
          cr_assert(0 == findInArray(i, brokenEntries, mods), "Unmodified entry %lu detected as modified.", i);
        }
    }

  // Release message resources
  for (size_t i = 0; i < num; i++)
    {
      log_msg_unref(logs[i]);
      g_string_free(output[i], TRUE);
    }

  log_template_unref(slog_templ);
  g_free(output);
  g_free(logs);
  g_free(ob);

  closure(testData);
  g_string_free(gstrTest, TRUE);
}

void test_slog_malicious_modifications(void)
{
  enum LogMode logmode = LOGMODE_ENCRYPTED;
  common_slog_malicious_modifications(logmode);
}

void test_slog_malicious_modifications_plain_direct(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_DIRECT;
  common_slog_malicious_modifications(logmode);
}

void test_slog_malicious_modifications_plain_base64(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_BASE64;
  common_slog_malicious_modifications(logmode);
}


//-- Performance

void common_slog_performance(enum LogMode logmode)
{
  LogTemplateEvalOptions my_default_template_eval_optons = {NULL, LTZ_LOCAL, 999, context_id, LM_VT_STRING, log_template_default_escape_method}; //-- TODO clarify escape

  GString *gstrTest = concat_testinfo_logmode("test_slog_performance", logmode);
  TestData *testData = initialize(gstrTest->str, logmode);
  LogTemplate *slog_templ = createTemplate(testData, logmode);

  GString *res = g_string_sized_new(1024);
  gint i;

  LogMessage *msg = create_random_sample_message();

  start_stopwatch();
  for (i = 0; i < PERFORMANCE_COUNTER; i++)
    {
      //-- DEFAULT_TEMPLATE_EVAL_OPTIONS seems to miss escape
      //-- log_template_format(slog_templ, msg, &DEFAULT_TEMPLATE_EVAL_OPTIONS, res);
      log_template_format(slog_templ, msg, &my_default_template_eval_optons, res);

    }
  stop_stopwatch_and_display_result(PERFORMANCE_COUNTER, "%-90.*s", (int)strlen(slog_templ->template_str) - 1,
                                    slog_templ->template_str);

  // Free resources
  log_template_unref(slog_templ);
  g_string_free(res, TRUE);
  log_msg_unref(msg);

  closure(testData);
  g_string_free(gstrTest, TRUE);
}

void test_slog_performance(void)
{
  enum LogMode logmode = LOGMODE_ENCRYPTED;
  common_slog_performance(logmode);
}

void test_slog_performance_plain_direct(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_DIRECT;
  common_slog_performance(logmode);
}

void test_slog_performance_plain_base64(void)
{
  enum LogMode logmode = LOGMODE_PLAIN_BASE64;
  common_slog_performance(logmode);
}

//-- sLogGMAC

void test_slog_tag_generation_sLogGMAC(void)
{
  g_print("-- test_slog_tag_generation_sLogGMAC\n");

  GString *Msg = g_string_new("Log msg 01");
  GString *Msg2 = g_string_new("Log msg 02"); //-- same length but different content
  guchar Key[KEY_LENGTH];
  guchar Key2[KEY_LENGTH];
  guchar IV[IV_LENGTH];
  guchar IV2[IV_LENGTH];
  guchar Tag[AES_BLOCKSIZE];
  guchar Tag2[AES_BLOCKSIZE];
  guchar Tag3[AES_BLOCKSIZE];

  memset(Tag, 0, AES_BLOCKSIZE);
  memset(Tag2, 0, AES_BLOCKSIZE);
  memset(Tag3, 0, AES_BLOCKSIZE);

  fill_buffer_random(IV, IV_LENGTH);
  fill_buffer_random(Key, KEY_LENGTH);

  int ret =  sLogGMAC((guchar *)Msg->str, (int)Msg->len, Key, IV, Tag);
  cr_assert(0 == ret, "sLogGMAC should return 0 in case success");

  memcpy(Key2, Key, KEY_LENGTH);
  memcpy(IV2, IV, IV_LENGTH);

  ret =  sLogGMAC((guchar *)Msg2->str, (int)Msg2->len, Key2, IV2, Tag2);
  cr_assert(0 == ret, "sLogGMAC should return 0 in case success");

  ret = memcmp(Tag, Tag2, AES_BLOCKSIZE);
  cr_assert(0 != ret, "sLogGMAC should produce different Tags for different messages");

  ret = memcmp(Key, Key2, KEY_LENGTH);
  cr_assert(0 == ret, "sLogGMAC should not change a key");

  ret = memcmp(IV, IV2, IV_LENGTH);
  cr_assert(0 == ret, "sLogGMAC should not change an IV");

  Key2[0] = (guchar)((1 + Key[0]) & 0xFF); //-- ensure different keys
  ret =  sLogGMAC((guchar *)Msg2->str, (int)Msg2->len, Key2, IV2, Tag3);
  cr_assert(0 == ret, "sLogGMAC should return 0 in case success");

  ret = memcmp(Tag2, Tag3, AES_BLOCKSIZE);
  cr_assert(0 != ret, "sLogGMAC provide different Tags for different keys");

  //-- empty string
  ret =  sLogGMAC((guchar *)"", 0, Key, IV, Tag);
  cr_assert(0 == ret, "sLogGMAC should return 0 in case success: ret: %d", ret);
  //dbg_hexdump("Tag", Tag, AES_BLOCKSIZE);

  g_string_free(Msg, TRUE);
  g_string_free(Msg2, TRUE);
}

void test_slog_base64_helper(void)
{
  // "AAAAAAAAAAA="
  gsize len = get_decoded_base64_length("AAAAAAAAAAA=");
  cr_assert(8 == len, "expected: 8 == get_decoded_base64_length(\"AAAAAAAAAAA=\")");
  len = get_base64_length(8);
  cr_assert(12 == len, "expected: 12 == get_base64_length(12) but returns: %ld", len);

  // a+RIrUdesq/MLyP/iUdcvE22rWhBeeKOQQLoQQ==
  gboolean is_b64 = is_likely_base64("a+RIrUdesq/MLyP/iUdcvE22rWhBeeKOQQLoQQ==");
  cr_assert(TRUE == is_b64);
  len = get_decoded_base64_length("a+RIrUdesq/MLyP/iUdcvE22rWhBeeKOQQLoQQ==");
  cr_assert(28 == len,
            "expected: 28 == get_decoded_base64_length(\"a+RIrUdesq/MLyP/iUdcvE22rWhBeeKOQQLoQQ==\")");
  len = get_base64_length(28);
  cr_assert(40 == len, "expected: 40 == get_base64_length(28) but returns: %ld", len);


  is_b64 = is_likely_base64("a+RIrUdesq/MLyP/iUdcvE22rWhBeeKOQQLoQQ");
  cr_assert(FALSE == is_b64);

  is_b64 = is_likely_base64("a+RIrUde:sq/MLyP/iUdcvE22rWhBeeKOQQLoQQ==");
  cr_assert(FALSE == is_b64);

  is_b64 = is_likely_base64("a+RIrUde:q/MLyP/iUdcvE22rWhBeeKOQQLoQQ==");
  cr_assert(FALSE == is_b64);

  gchar
  szBuffer[] =
    "l3IzYa7JGYviwmmVi5f9oOBIUpm/JDJeWE0MokxpbmUgMDcgw7bDpMO8w5bDhMOcw59pIFNwZWNpYWwgY2hhcnM6ICFAI+KCrCw8wqMkJV4mKlxgwqfCqcKu4oSiKSDwn6Sq";
  is_b64 = is_likely_base64(szBuffer);
  cr_assert(TRUE == is_b64);
  gsize len_bin_data = 0;
  guchar *pbin_data = g_base64_decode(szBuffer, &len_bin_data);
  cr_assert_not_null(pbin_data, "g_base64_decode expected to return a valid pointer");
  cr_assert(len_bin_data > 0);
  guchar *pt = g_try_new0(guchar, len_bin_data); //-- safe
  GString *dest = g_string_new("");
  gsize len_bin_part = 28;
  cr_assert(len_bin_data > len_bin_part);
  gsize textlen = len_bin_data - len_bin_part;
  g_string_append_len(dest, (const char *)(pbin_data + len_bin_part), textlen);
  g_print("cut out: %s\n", dest->str);
  char *search = strstr(dest->str, "Line 07");
  cr_assert_not_null(search);
  g_string_free(dest, TRUE);
  g_free(pt);
  g_free(pbin_data);
}


static void helper_truncate_utf8_gstring(gint nr,
                                         const gchar *sz_the_string,
                                         gsize olimit,
                                         glong scnt, gsize ocnt,
                                         glong scnt2, gsize ocnt2)
{
  // g_print("-- Test %d ---\n", nr);
  GString *gslog = g_string_new(sz_the_string); //-- Example: "Grüße ✈️🛩️\n"
  gsize max_octet_len = olimit; //-- 8
  glong cnt_symbol = g_utf8_strlen(gslog->str, -1);
  gsize cnt_octet = gslog->len;
  // g_print("%s\n", gslog->str);
  // g_print("truncated str, symbols: %ld, octets: %ld\n", cnt_symbol, cnt_octet);
  // dbg_hexdump("truncated str", (unsigned char *) gslog->str, gslog->len);
  cr_assert(scnt == cnt_symbol, "expected: %ld == cnt_symbol (is: %ld)", scnt, cnt_symbol );
  cr_assert(ocnt == cnt_octet, "expected: %ld == cnt_octet (is: %ld)", ocnt, cnt_octet);
  //-- function to test
  //g_print("String will be truncated to a maximal octet length of %ld\n", max_octet_len);
  truncate_utf8_gstring(gslog, max_octet_len);
  cnt_symbol = g_utf8_strlen(gslog->str, -1);
  cnt_octet = gslog->len;
  // g_print("%s\n", gslog->str);
  // g_print("truncated str, symbols: %ld, octets: %ld\n", cnt_symbol, cnt_octet);
  // dbg_hexdump("truncated str", (unsigned char *) gslog->str, gslog->len);
  cr_assert(scnt2 == cnt_symbol, "expected: %ld == cnt_symbol (is: %ld)", scnt2, cnt_symbol );
  cr_assert(ocnt2 == cnt_octet, "expected: %ld == cnt_octet (is: %ld)", ocnt2, cnt_octet);
  g_string_free(gslog, TRUE);
  // g_print("-- Done Test %d ---\n\n", nr);
}

void copy_gstring_to_buffer(GString *gstr, char *dest, size_t dest_size)
{
  if (dest_size < gstr->len + 1)
    {
      g_print("ERROR: dest_size %ld to small for string len %ld\n", dest_size, gstr->len);
      return;
    }
  // Copy the string (including null terminator)
  strncpy(dest, gstr->str, dest_size - 1);
  dest[dest_size - 1] = '\0'; // Ensure null termination
}

void test_slog_truncate_utf8_gstring(void)
{
  //-- test truncation of utf-8 string
  //   count of symbols and octets before and after limitation to octet count
  setlocale(LC_ALL, "");
  helper_truncate_utf8_gstring(1, "Grüße 😊", 8, 7, 12, 6, 8);
  helper_truncate_utf8_gstring(2, "Grüße 😊\n", 8, 8, 13, 6, 8);
  helper_truncate_utf8_gstring(3, "Grüße 😊😊\n", 12, 9, 17, 7, 9);
  helper_truncate_utf8_gstring(4, "Grüße ✈️🛩️\n", 13, 11, 22, 8, 12);

  /* Notes to Test 4
   Character,  Hex (UTF-8),  Octets,   Codepoint,  Description
   G,          47,           1,        U+0047,     Latin Capital G
   r,          72,           1,        U+0072,     Latin Small r
   ü,          C3 BC,        2,        U+00FC,     Latin Small u with diaeresis
   ß,          C3 9F,        2,        U+00DF,     Latin Small sharp s
   e,          65,           1,        U+0065,     Latin Small e
   [space],    20,           1,        U+0020,     Space
   ✈️ ,         E2 9C 88,     3,        U+2708,     Airplane
   VS16,       EF B8 8F,     3,        U+FE0F,     Variation Selector-16
   🛩️,         F0 9F 9B A9,  4,        U+1F6E9,    Small Airplane
   VS16,       EF B8 8F,     3,        U+FE0F,     Variation Selector-16
   \n,         0A,           1,        U+000A,     Line Feed
   Total,,22,11,

   Grüße ✈️🛩️\n

   original str, symbols: 11, octets: 22
   original str (22 bytes):
   00000000: 47 72 c3 bc c3 9f 65 20  e2 9c 88 ef b8 8f f0 9f  |Gr....e ........|
   00000016: 9b a9 ef b8 8f 0a                                 |......|
   String will be truncated to a maximal octet length of 13
   Grüße ✈

   truncated str, symbols: 8, octets: 12
   truncated str (12 bytes):
   00000000: 47 72 c3 bc c3 9f 65 20  e2 9c 88 0a              |Gr....e ....|
   -- Done Test 4 ---
   */
}


void test_slog_cr_logger_entries(void)
{
  //-- test parts of standalone cr_looger and cr_verifier
  TestDataCR *td = initialize_cr("test_slog_cr_logger_entries");
  const gsize BLOCKSIZE = 2112; //-- MESSAGE_LEN_SLOGCR + 64 = 2048 + 64 = 2112
  const gsize the_n = 1093; //-- count of lines in generated plain log file
  gdouble result = the_n * THE_C;
  gdouble temp = ceil(result);
  gsize the_m = (gsize)temp;
  g_print("n: %ld, m: %ld\n", the_n, the_m);

  //-- handle cr_logger context
  cr_pi_logger_context loggerCtx = {NULL, NULL, NULL, NULL, INT_MAX};
  loggerCtx.p_MasterKeyPath = td->gstr_key_path->str;
  loggerCtx.p_OutputDirectoryPath = td->gstr_dir_path->str;
  loggerCtx.p_InputPlainLogPath = NULL; //-- not used here, we do access gpa_logs directly
  loggerCtx.p_OutputEncLogPath = td->gstr_enc_path->str;
  loggerCtx.maxLogs = the_n;

  //-- create logs, gpa_logs
  GPtrArray *gpa_logs = g_ptr_array_new_with_free_func(unref_gstring_wrapper);
  for (guint i = 0; i < the_n; ++i)
    {
      GString *line = g_string_new("");
      g_string_printf(line, "Log msg number %d 👽🛸🌍\n", i);
      g_ptr_array_add(gpa_logs, line);
    }
  g_print("gpa_logs->len: %d\n", gpa_logs->len);

  cr_PIContext *lctx = cr_CreatePIContext(gpa_logs->len, TRUE, loggerCtx.p_MasterKeyPath, loggerCtx.p_OutputDirectoryPath,
                                          loggerCtx.p_OutputEncLogPath);
  if (FALSE == cr_Init(lctx) )
    {
      g_printerr("ERROR: Initialization failed!\n");
      g_free(lctx->keyPath);
      g_free(lctx);
      g_ptr_array_free(gpa_logs, TRUE);
      cr_assert(FALSE, "cr_Init failed"); //return 1; //-- ERROR
    }

  //-- create encrypted log file
  GString *cmdsync = g_string_new("");
  g_string_printf(cmdsync, "sync");
  int status;

  gboolean is_add = FALSE;
  g_print("gpa_logs->len: %d\n\n", gpa_logs->len);
  for (guint i = 0; i < gpa_logs->len; ++i)
    {
      GString *line = (GString *) g_ptr_array_index(gpa_logs, i);
      if (line->len > MESSAGE_LEN_SLOGCR)
        {
          truncate_utf8_gstring(line, MESSAGE_LEN_SLOGCR);
        }
      // function to test: cr_AddLogEntry
      is_add = cr_AddLogEntry(lctx, (unsigned char *)(line->str), line->len); //-- line->len: count of octets
      if (FALSE == is_add)
        {
          g_print("ERROR: cr_addLogEntry was not successful. break and quit.\n");
          break; //-- ERROR, Note: Cancel on first error
        }
    } //-- for gpa_logs->len
  if (FALSE == is_add)
    {
      g_print("\n\n%d Logs have NOT been written successfully\n", gpa_logs->len);
      fclose(lctx->logFile); //-- no dtor for ctx
      fclose(lctx->keyFile);
      g_free(lctx->keyPath);
      g_free(lctx);
      g_ptr_array_free(gpa_logs, TRUE);
      cr_assert(FALSE, "cr_AdLogEntry failed");
    }

  //-- logger finished
  fclose(lctx->logFile);
  fclose(lctx->keyFile);
  status = system(cmdsync->str);
  g_print("Warning: sync returned with status: %d\n", status);

  //-- check output file whether it contains the expected amount of entries
  gboolean ret = FALSE;
  gsize count = 42 + the_m;

  //-- function to test: get_plain_log_lines_from_cr_logger_enc
  ret = get_plain_log_lines_from_cr_logger_enc(td->gstr_enc_path->str, BLOCKSIZE, THE_C, &count);
  cr_assert_eq(ret, TRUE, "Expected result to be TRUE");
  cr_assert(the_n == count, "expected: %ld == count (is: %ld)", the_n, count);

  //-- now prepare verification
  cr_VerifierContext vctx;
  memset(&vctx, 0, sizeof(cr_VerifierContext));
  copy_gstring_to_buffer(td->gstr_key_path, vctx.masterKeyPath, PATH_MAX);
  copy_gstring_to_buffer(td->gstr_enc_path, vctx.inEncFilePath, PATH_MAX);
  copy_gstring_to_buffer(td->gstr_dir_path, vctx.logFileDirectory, PATH_MAX);

  //-- verifier output file patih
  gchar *full_path = g_build_filename(td->gstr_dir_path->str, "verifier_out.txt", NULL);
  if (full_path)
    {
      g_print("Verifier out path: %s\n", full_path);
      strncpy(vctx.outPlainFilePath, full_path, PATH_MAX);
      vctx.outPlainFilePath[PATH_MAX - 1] = '\0';
      g_free(full_path);
    }

  //-- verifier protocol output file path
  gchar *protocol_full_path = g_build_filename(td->gstr_dir_path->str, "verifier_protocol.txt", NULL);
  if (protocol_full_path)
    {
      g_print("Verifier protocol path: %s\n", protocol_full_path);
      strncpy(vctx.outProtocolPath, protocol_full_path, PATH_MAX);
      vctx.outProtocolPath[PATH_MAX - 1] = '\0';
      g_free(protocol_full_path);
    }
  vctx.n = (int) the_n;
  vctx.m = (int) the_m;
  vctx.protocolFile = fopen(vctx.outProtocolPath, "w");
  if (NULL ==  vctx.protocolFile)
    {
      g_warning("Can not create protocol output file!\n");
    }
  status = system(cmdsync->str);
  g_print("Warning: sync returned with status: %d\n", status);
  cr_assert(NULL != vctx.protocolFile);

  //-- function to test, verifiy the encrypted log file
  cr_Result vres = cr_Verify(&vctx);
  fclose(vctx.protocolFile);
  status = system(cmdsync->str);
  g_print("Warning: sync returned with status: %d\n", status);
#if 0
  {
    GString *cmd = g_string_new("");
    g_string_printf(cmd, "mkdir -p ~/temp && sync && cp -R %s ~/temp", td->gstr_dir_path->str);
    int status = system(cmd->str);
    g_print("%s, status: %d\n", cmd->str, status);
    g_string_free(cmd, TRUE);
  }
#endif
  //-- clean up
  g_free(lctx->keyPath);
  g_free(lctx);
  g_ptr_array_free(gpa_logs, TRUE);
  g_string_free(cmdsync, TRUE);
  cr_assert(vres.success == TRUE);
  closure_cr(td);
}


void test_slog_cr_logger_context(void)
{
  TestDataCR *td = initialize_cr("test_slog_cr_logger_entries");
  //const gsize BLOCKSIZE = 2112; //-- MESSAGE_LEN_SLOGCR + 64 = 2048 + 64 = 2112
  const gsize the_n = 1093; //-- count of lines in generated plain log file
  //g_print("test_slog_cr_logger_context\n");

  cr_pi_logger_context loggerctx = {td->gstr_key_path->str,
                                    td->gstr_dir_path->str,
                                    NULL,
                                    td->gstr_enc_path->str,
                                    the_n
                                   };

  //g_print("test_slog_cr_logger_context, loggerctx done\n");
  cr_PIContext *p_pictx = NULL;
  cr_PRGContext *p_prg = NULL;

  //-- function to test
  gboolean success = init_cr_logger_functionality(&loggerctx, &p_pictx, &p_prg);

  //g_print("test_slog_cr_logger_context, init_cr_lolgger_functionality done\n");

  cr_assert(success == TRUE, "expected success of init_cr_lolgger_functionalit");
  cr_assert(p_pictx != NULL, "p_picttx is NULL");
  cr_assert((*p_pictx).keyPath != NULL, "keyPath is NULL");
  cr_assert((*p_pictx).logFileDirectory != NULL, "logFileDirectory is NULL");
  cr_assert((*p_pictx).logFileName != NULL, "logFileName is NULL");
  cr_assert((*p_pictx).maxEntries == the_n, "the_n %ld is expected!", the_n);
  cr_assert(p_prg != NULL, "p_prg is NULL");

  int cmp = strcmp(td->gstr_dir_path->str, (*p_pictx).logFileDirectory);
  cr_assert(0 == cmp);

  g_print("td->gstr_enc_path->str: %s (%p), logFileName: %s (%p)\n", td->gstr_enc_path->str, td->gstr_enc_path->str,
          (*p_pictx).logFileName, (*p_pictx).logFileName);

  cmp = strcmp(td->gstr_enc_path->str, (*p_pictx).logFileName);
  cr_assert(0 == cmp);

  cr_assert((*p_pictx).logFile != NULL);
  cr_assert((*p_pictx).keyFile != NULL);
  cr_assert((*p_pictx).keyPath != NULL);

  fclose((*p_pictx).logFile);
  (*p_pictx).logFile = NULL;

  fclose((*p_pictx).keyFile);
  (*p_pictx).keyFile = NULL;

  g_free((*p_pictx).keyPath);
  g_free(p_pictx);
  p_pictx = NULL;
  g_free(p_prg);
  p_prg = NULL;
}




//-- template

Test(secure_logging, test_slog_template_format)
{
  test_slog_template_format();
}


//-- performance

Test(secure_logging, test_slog_performance)
{
  test_slog_performance();
}

Test(secure_logging, test_slog_performance_plain_direct)
{
  test_slog_performance_plain_direct();
}

Test(secure_logging, test_slog_performance_plain_base64)
{
  test_slog_performance_plain_base64();
}


//-- verification bulk

Test(secure_logging, test_slog_verification_bulk)
{
  test_slog_verification_bulk();
}

Test(secure_logging, test_slog_verification_bulk_plain_direct)
{
  test_slog_verification_bulk_plain_direct();
}

Test(secure_logging, test_slog_verification_bulk_plain_base64)
{
  test_slog_verification_bulk_plain_base64();
}


//-- verification

Test(secure_logging, test_slog_verification)
{
  test_slog_verification();
}

Test(secure_logging, test_slog_verification_plain_direct)
{
  test_slog_verification_plain_direct();
}

Test(secure_logging, test_slog_verification_plain_base64)
{
  test_slog_verification_plain_base64();
}


//-- key

Test(secure_logging, test_slog_corrupted_key)
{
  test_slog_corrupted_key();
}

Test(secure_logging, test_slog_corrupted_key_plain_direct)
{
  test_slog_corrupted_key_plain_direct();
}

Test(secure_logging, test_slog_corrupted_key_plain_base64)
{
  test_slog_corrupted_key_plain_base64();
}


//-- malicious

Test(secure_logging, test_slog_malicious_modifications)
{
  test_slog_malicious_modifications();
}

Test(secure_logging, test_slog_malicious_modifications_plain_direct)
{
  test_slog_malicious_modifications_plain_direct();
}

Test(secure_logging, test_slog_malicious_modifications_plain_base64)
{
  test_slog_malicious_modifications_plain_base64();
}


//-- sLogGMAC

Test(secure_logging, test_slog_tag_generation_sLogGMAC)
{
  test_slog_tag_generation_sLogGMAC();
}

//-- Base64

Test(secure_logging, test_slog_base64_helper)
{
  test_slog_base64_helper();
}


//-- utf-8 string truncation

Test(secure_logging, test_slog_truncate_utf8_gstring)
{
  test_slog_truncate_utf8_gstring();
}


//-- Crash Recovery

Test(secure_logging, test_slog_cr_logger_entries)
{
  test_slog_cr_logger_entries();
}

Test(secure_logging, test_slog_cr_logger_context)
{
  test_slog_cr_logger_context();
}

