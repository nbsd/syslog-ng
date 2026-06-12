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


// based on THESIS_secure-logging-cr\src\shared\Crypto.c
//
//  Crypto.c
//  shared
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 15.11.23.
//

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <glib.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/cmac.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/params.h>
#endif

#include "cr_pi_shared.h"
#include "cr_crypto.h"


// The folling must be replaced due to GitHub code checker
// static unsigned char GAMMA[2 * AES_BLOCK_LEN] = {[0 ... (2 * AES_BLOCK_LEN - 1)] = PAD1};
// cr_pi_shared.h:54: #define AES_BLOCK_LEN 16

SLOGCR_STATIC_ASSERT(AES_BLOCK_LEN == 16, "Wrong_AES_Block_Size_for_provided_GAMMA_initialization");

#define FILL_16(val) val, val, val, val, val, val, val, val, \
                      val, val, val, val, val, val, val, val

static unsigned char GAMMA[2 * AES_BLOCK_LEN] = { FILL_16(PAD1), FILL_16(PAD1) };


static int cr_under_PRG(unsigned char *seed, unsigned int *counter, const EVP_CIPHER *cipher, unsigned char *buffer,
                        int size);
static int cr_AES_encrypt(const EVP_CIPHER *cipher, unsigned char *plaintext, int plaintextSize, unsigned char *key,
                          unsigned char *iv, unsigned char *ciphertextBuffer);
static int cr_exists(int element, const int arr[], size_t size);
static void cr_handleErrors(void);


//----------------------------------------------------------------------
// cr_CreatePRG128Context
//
// in seed: Seed to be copied into context
// returns pointer to cr_PRG128Context

cr_PRG128Context *cr_CreatePRG128Context(unsigned char seed[16])
{
  cr_PRG128Context *context = g_malloc0(sizeof(cr_PRG128Context));
  if (context == NULL)
    {
      g_warning("Failed to allocate memory.");
      return NULL;
    }
  context->counter = 0;
  memcpy(context->seed, seed, 16);
  return context;
}



//----------------------------------------------------------------------
// cr_CreatePRGContext
//
// in seed: Seed to be copied into context
// returns pointer to cr_PRGContext

cr_PRGContext *cr_CreatePRGContext(unsigned char seed[KEY_SIZE])
{
  cr_PRGContext *context = g_malloc0(sizeof(cr_PRGContext));
  if (context == NULL)
    {
      g_warning("Failed to allocate memory.");
      return NULL;
    }
  context->counter = 0;
  memcpy(context->seed, seed, KEY_SIZE);
  return context;
}



//----------------------------------------------------------------------
// cr_under_PRG
//
// PRG helper method
// in seed:
// in/out counter:
// in cipher:
// in/out  buffer:
// in size: size of buffer
//
// returns 1 when SUCCESS else 0

static int cr_under_PRG(unsigned char *seed, unsigned int *counter, const EVP_CIPHER *cipher, unsigned char *buffer,
                        int size)
{
  // calculate the number of aes blocks and ceil if it is not a multiple of the AES block size.
  double temp_m = ceil((double)size / AES_BLOCK_LEN);
  unsigned int blocks = (unsigned int) temp_m;
  int paddedSize = blocks * AES_BLOCK_LEN;

  unsigned char *input = calloc(paddedSize, sizeof(unsigned char));
  unsigned char *output = calloc(paddedSize, sizeof(unsigned char));

  // Each AES block contain the next higher counter sequence.
  for (unsigned long i = 0; i < blocks; ++i)
    {
      unsigned char *destinationPtr = &input[i * AES_BLOCK_LEN];
      unsigned long ctr = i + *counter;

      // Copy the unsigned long value to the block in the input buffer
      memcpy(destinationPtr, &ctr, sizeof(unsigned long));
    }

  // save to global counter.
  *counter += blocks;

  int outputLen = cr_AES_encrypt(cipher, input, paddedSize, seed, NULL, output);
  free(input);

  // Validate output buffer length
  if (outputLen != paddedSize)
    {
      g_warning("outputLen != paddedSize\n");
      free(output);
      return 0; //-- ERROR
    }
  // Fill result buffer, but only with the requested amount of random data.
  memcpy(buffer, output, size);
  free(output);

  return 1; //-- SUCCESS
}



//----------------------------------------------------------------------
// cr_PRG128
//
// Wrapper of cr_under_PRG
// in/out ctx: Pointer to instance of cr_PRG128Context
// in/out  buffer:
// in size: size of buffer
//
// returns 1 when SUCCESS else 0

int cr_PRG128(cr_PRG128Context *ctx, unsigned char *buffer, int size)
{
  return cr_under_PRG(ctx->seed, &ctx->counter, EVP_aes_128_ecb(), buffer, size);
}



//----------------------------------------------------------------------
// cr_PRG
//
// Wrapper of cr_under_PRG
// in/out ctx: Pointer to instance of cr_PRGContext
// in/out  buffer:
// in size: size of buffer
//
// returns 1 when SUCCESS else 0

int cr_PRG(cr_PRGContext *ctx, unsigned char *buffer, int size)
{
  return cr_under_PRG(ctx->seed, &ctx->counter, EVP_aes_256_ecb(), buffer, size);
}



//----------------------------------------------------------------------
// cr_AES_256_CTR_encrypt
//
// Wrapper of cr_AES_encrypt
// in plaintext:
// in plintextSize:
// in key:
// in iv:
// in chiphertextBuffer:
//
// returns 1 when SUCCESS else 0

int cr_AES_256_CTR_encrypt(unsigned char *plaintext, int plaintextSize, unsigned char *key, unsigned char *iv,
                           unsigned char *ciphertextBuffer)
{
  return cr_AES_encrypt(EVP_aes_256_ctr(), plaintext, plaintextSize, key, iv, ciphertextBuffer);
}



//----------------------------------------------------------------------
// cr_AES_256_CTR_decrypt
//
// Wrapper of OpenSSL AES 256 decryption
// based on: https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption#Encrypting_the_message
// in ciphertext:
// in ciphertextSize:
// in key:
// in iv:
// in plaintextBuffer:
//
// returns Length of plain text

int cr_AES_256_CTR_decrypt(unsigned char *ciphertext, int ciphertextSize, unsigned char *key, unsigned char *iv,
                           unsigned char *plaintextBuffer)
{
  EVP_CIPHER_CTX *ctx;

  int len = 0;

  int plaintextLen;
  /* Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new()))
    cr_handleErrors();

  /*
   * Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv)) /* Failed to initialize aes in ECB mode. */
    cr_handleErrors();

  // Disable padding, the total amount of data encrypted or decrypted must then be a multiple of the block size or an error will occur.
  if (1 != EVP_CIPHER_CTX_set_padding(ctx, 0))
    cr_handleErrors();

  /*
   * Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary.
   */
  if (1 != EVP_DecryptUpdate(ctx, plaintextBuffer, &len, ciphertext, ciphertextSize)) /* Failed to decrypt plaintext. */
    cr_handleErrors();
  plaintextLen = len;

  /*
   * Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if (1 != EVP_DecryptFinal_ex(ctx, plaintextBuffer + len, &len))
    cr_handleErrors();
  plaintextLen += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintextLen;
}



//----------------------------------------------------------------------
// cr_AES_encrypt
//
// Wrapper of OpenSSL AES 256 encryption
// based on: https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption#Encrypting_the_message
// in cipher:
// in plaintext:
// in plaintextSize:
// in key:
// in iv:
// in ciphertextBuffer:
//
// returns Length of encrypted text (0 in case of ERROR)

static int cr_AES_encrypt(const EVP_CIPHER *cipher, unsigned char *plaintext, int plaintextSize, unsigned char *key,
                          unsigned char *iv, unsigned char *ciphertextBuffer)
{
  EVP_CIPHER_CTX *ctx;
  int len;
  int ciphertextLen;

  /* Create and initialise the context */
  if (!(ctx = EVP_CIPHER_CTX_new()))
    {
      cr_handleErrors();
      return 0;
    }


  /*
   * Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits
   */
  if (1 != EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv))
    {
      cr_handleErrors();
      return 0;
    }

  // Disable padding, the total amount of data encrypted or decrypted must then be a multiple of the block size or an error will occur.
  EVP_CIPHER_CTX_set_padding(ctx, 0);

  /*
   * Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if (1 != EVP_EncryptUpdate(ctx, ciphertextBuffer, &len, plaintext, plaintextSize))
    {
      cr_handleErrors();
      return 0;
    }
  ciphertextLen = len;

  /*
   * Finalise the encryption. Further ciphertext bytes may be written at this stage.
   */
  if (1 != EVP_EncryptFinal_ex(ctx, ciphertextBuffer + len, &len))
    {
      cr_handleErrors();
      return 0;
    }
  ciphertextLen += len;


  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertextLen;
}



//----------------------------------------------------------------------
// cr_exits
//
// Helper to check if a number is in an array
// in element: Number to search for
// in arr:
// in size:
// returns 1 when number found in array else 0 when not present

static int cr_exists(int element, const int arr[], size_t size)
{
  for (size_t i = 0; i < size; ++i)
    {
      if (arr[i] == element)
        {
          return 1;
        }
    }
  return 0;
}



//----------------------------------------------------------------------
// cr_UniformRandomInt
//
// PRG number generation and upperbound handling to eliminate the modul bias
// in ctx: Context of cr_PRGContext
// in upperBound:
// returns PRG random number

unsigned int cr_UniformRandomInt(cr_PRGContext *ctx, const unsigned int upperBound)
{
  unsigned long long multipleOfUpperBound; //-- Fix ensure no overflow in case i386 (endless loop)
  unsigned int rand;
  unsigned char *randomBuffer;

  if (upperBound < 2)
    {
      g_error("PRG failed.");
      exit(EXIT_FAILURE);
    }

  // eliminate the modul bias
  // https://research.kudelskisecurity.com/2020/07/28/the-definitive-guide-to-modulo-bias-and-how-to-avoid-it/
  // https://github.com/jedisct1/libsodium/blob/master/src/libsodium/randombytes/randombytes.c#L145
  // min = (1U + ~upperBound) % upperBound;
  // https://github.com/openbsd/src/blob/master/lib/libc/crypt/arc4random_uniform.c
  // get the largest multiple which is less than the number of diffrent values taht can be represented by an unsigned int (2^32).
  // in the case the upper bound is a multiple of 2^32, the condition below holds anyeay, because it has to be less than 2^32,
  // so it will work with the largest possible unsigned int.

  multipleOfUpperBound = (1ULL << 32) - ((1ULL << 32) %
                                         upperBound); //-- Fix: unsigned long long else might result in 0 on i386 due overflow
  randomBuffer = g_malloc0(sizeof(unsigned int));

  for (;;)
    {
      if (1 != cr_PRG(ctx, randomBuffer, sizeof(unsigned int)))
        {
          g_error("PRG failed.");
          exit(EXIT_FAILURE);
        }
      memcpy(&rand, randomBuffer, sizeof(unsigned int));
      if (rand < multipleOfUpperBound)
        break;
    }

  g_free(randomBuffer);
  return rand % upperBound;
}


#if 0
int cr_DRN_dummy(unsigned char seed[KEY_SIZE], const int the_k, const int upperBound, int kRandom[THE_K])
{
  unsigned int number;
  int i = 0;
  int high = upperBound - 1; //-- index based random number. First index is 0.
  int low = 0;
  (void) seed;

  if (upperBound < the_k) //-- BUG FIX to avoid endless while loop for special ..
    {
      // .. unlikely case of small amount of log lines
      g_warning("cr_DRN_dummy, upperBound provides only %d different random numbrs but the_k %d are needed at least to leave while loop!\n",
                upperBound, the_k);
      return 0; //-- ERROR
    }

  if (2 >= upperBound && upperBound > INT_MAX)
    {
      g_warning("Failed: cr_DRN_dummy, upperBound %d out of range, the_k: %d\n", upperBound, the_k);
      return 0; //-- ERROR
    }

  // Fill the array with -1
  // thats why the upper bound cant be larger than int_max
  for (int r = 0; r < the_k; ++r)
    kRandom[r] = -1;

  i = 0;
  while (i < the_k)
    {
      // rand()%((high+1)-low)+low;
      number = rand() % ((high + 1) - low) + low;
      // check if the random number already exists in the array of the_k random numbers.
      if (!cr_exists(number, kRandom, the_k))
        {
          kRandom[i] = number;
          i++;
        }
    }
  return 1; //-- SUCCESS
}
#endif


//----------------------------------------------------------------------
// cr_DRN
//
// Fill array with random numbers
//
// in seed
// in the_k
// in upperBound:
// in kRandom: Array of count the_k
//
// returns 1 on SUCCESS and 0 on FAILURE

int cr_DRN(unsigned char seed[KEY_SIZE], const int the_k, const int upperBound, int kRandom[THE_K])
{
  unsigned int rand;
  int i = 0;

//-- BUG FIX to avoid endless while loop. Anyhow count of log lines should much greater, e.g.: at least 4096.
  if (upperBound < the_k)
    {
      g_warning("Failed: cr_DRN, upperBound provides only %d different random numbers but the_k %d are needed at least to leave while loop!\n",
                upperBound, the_k);
      return 0; //-- ERROR
    }

  if (2 >= upperBound && upperBound > INT_MAX)
    {
      g_warning("Failed: cr_DRN, upperBound %d out of range, the_k: %d\n", upperBound, the_k);
      return 0; //-- ERROR
    }

  // Fill the array with -1
  // thats why the upper bound cant be larger than int_max
  for (int r = 0; r < the_k; ++r)
    {
      kRandom[r] = -1;
    }

  cr_PRGContext *ctx = cr_CreatePRGContext(seed);
  if (NULL == ctx)
    {
      g_warning("Failed: cr_DRN, ctx is NULL!\n");
      return 0; //-- ERROR
    }

  while (i < the_k)
    {
      //-- TODO endless loop in case i386? How is ensured that i is incremented?

      //-- rand % upperbound so upperBound must be >= the_k else endless loop while
      rand = cr_UniformRandomInt(ctx, upperBound); //-- fixed
      // check if the random number already exists in the arra of k random numbers.
      if (!cr_exists(rand, kRandom, the_k))
        {
          kRandom[i] = rand;
          i++;
        }
    }

  g_free(ctx);
  return 1; //-- SUCCESS
}



/*
 Two phases PRF:
 1. CMAC, which returns a 128bit MAC tag which will than be used as key for a
 2. AES-128-ECB encryption of a counter.
 */
int cr_PRF(unsigned char *input, size_t inputSize, unsigned char *key, unsigned char *output, uint8_t outputSize)
{
  size_t outputLenCMAC;

  // add a byte for the outputSize, max len for the outputSize is 2^8 = 128
  unsigned char _input[inputSize + 1], seed[16];
  memcpy(_input, input, inputSize);

  // The output size is an input value of the PRF, and should therefore change the output of the PRF the same way, as the key or input, would do.
  // input || outputSize, set the last byte to the output size.
  _input[inputSize] = outputSize;

  if (!cr_CMAC(key, _input, inputSize, seed, &outputLenCMAC, CMAC_LEN))
    {
      g_warning("Failed to create CMAC as seed for a PRG as output for the variable PRF.");
      return 0;
    }

  // stretch or cut the output of the PRF, by applying a PRG.
  cr_PRG128Context *ctx = cr_CreatePRG128Context(seed);
  if (NULL == ctx)
    {
      g_warning("Failed: ctx is NULL.");
      return 0;
    }
  if (!cr_PRG128(ctx, output, outputSize))
    {
      g_warning("Failed to create PRF output, when using PRG.");
      g_free(ctx);
      return 0;
    }

  g_free(ctx);
  return 1;
}



int cr_KeyEvolution(unsigned char *key, unsigned char *nextKey)
{
  return cr_PRF(GAMMA, 32, key, nextKey, KEY_SIZE);
}



int cr_DeriveSubKeys(unsigned char masterSessionkey[KEY_SIZE], unsigned char encKey[KEY_SIZE],
                     unsigned char drnKey[KEY_SIZE], unsigned char tagKey[KEY_SIZE], unsigned char idKey[KEY_SIZE])
{
  cr_PRGContext *ctx = cr_CreatePRGContext(masterSessionkey);
  unsigned char *output = calloc(4 * KEY_SIZE, 1);

  if (cr_PRG(ctx, output, 4 * KEY_SIZE) != 1)
    {
      g_warning("Failed to derive sub keys.");
      return 0;
    }
  memcpy(encKey, output, KEY_SIZE);
  memcpy(drnKey, output + KEY_SIZE, KEY_SIZE);
  memcpy(tagKey, output + (2 * KEY_SIZE), KEY_SIZE);
  memcpy(idKey, output + (3 * KEY_SIZE), KEY_SIZE);

  free(ctx);
  free(output);
  return 1;
}


int cr_GenerateMasterKey(unsigned char *masterKey)
{
  return RAND_bytes(masterKey, KEY_SIZE);
}


int cr_GenerateIV(unsigned char *iv)
{
  return RAND_bytes(iv, IV_SIZE);
}


int cr_CMAC(unsigned char *key, unsigned char *input, size_t inputSize, unsigned char *output, size_t *outputSize,
            size_t maxOutputSize/* Prevent buffer overflows, in the case that the maximal possible outbut buffer size is smaler than the actual output buffer. */)
{
  EVP_MAC *mac = EVP_MAC_fetch(NULL, "CMAC", NULL);
  if (mac == NULL)
    {
      g_warning("Failed to fetch CMAC.");
      return 0; //-- ERROR
    }

  EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);

  if (!ctx)
    {
      g_warning("Failed to create MAC ctx.");
      EVP_MAC_free(mac);
      return 0; //-- ERROR
    }

  // Sets the name of the underlying cipher to be used. The mode of the cipher must be CBC.
  // https://www.openssl.org/docs/man3.1/man7/EVP_MAC-CMAC.html
  OSSL_PARAM params[2];
  params[0] = OSSL_PARAM_construct_utf8_string("cipher", "aes-256-cbc", 0);
  params[1] = OSSL_PARAM_construct_end();

  // braucht einen Array, nicht nur ein pointer auf einen Parameter.
  if (EVP_MAC_CTX_set_params(ctx, params) != 1)
    {
      g_error("Failed to set parameter.");
      // free
      EVP_MAC_CTX_free(ctx);
      EVP_MAC_free(mac);
      return 0; //-- ERROR
    }

  if (EVP_MAC_init(ctx, key, KEY_SIZE, NULL) != 1)
    {
      g_error("Failed to init CMAC.");
      // free
      EVP_MAC_CTX_free(ctx);
      EVP_MAC_free(mac);
      return 0; //-- ERROR
    }

  if (EVP_MAC_update(ctx, input, inputSize) != 1)
    {
      g_error("Failed to update CMAC.");
      // free
      EVP_MAC_CTX_free(ctx);
      EVP_MAC_free(mac);
      return 0; //-- ERROR
    }

  // If the maxOutputSize is to small, to hold the output -> the mission will be aborted.
  if (EVP_MAC_final(ctx, output, outputSize, maxOutputSize) != 1)
    {
      g_error("Failed to create CMAC.");
      // free
      EVP_MAC_CTX_free(ctx);
      EVP_MAC_free(mac);
      return 0; //-- ERROR
    }

  // free
  EVP_MAC_CTX_free(ctx);
  EVP_MAC_free(mac);
  return 1; //-- SUCCESS
}



void cr_handleErrors(void)
{
  g_error("cr_crypto.c, cr_handleErrors\n"); //-- TODO consider better interface without coredump

  ERR_print_errors_fp(stderr);
  abort();
}

