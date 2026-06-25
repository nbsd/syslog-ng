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


#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <openssl/rand.h>

#include "cr_randomstuff.h"

//----------------------------------------------------------------------
//exists
//in: element Item that is searched for
//in: arr Array Container of items being searched in
//in: size Number of items in array

static gboolean cr_exists(size_t element, const size_t arr[], int size)
{
  for (int i = 0; i < size; ++i)
    {
      if (arr[i] == element)
        {
          return TRUE;
        }
    }
  return FALSE;
}



//----------------------------------------------------------------------
// cr_distincRandomEz
// Provides an array of pseudo random integer numbers
// Memory is allocated here and the caller is responsible to free it.
// Note: There might be better random number generators, but here, no
// additional dependencies e.g. to gsl library is prefered.
// in range:  Range of random numbers is in closed interval [0, range]
//            range < RAND_MAX
// in k: Count of random numbers. Because each number exists only once
//       in array, k <= (range + 1)
// in seed: Initialization seed of random generator.
//
// return Pointer to array of k numbers of type size_t or NULL in
//        case of ERROR

size_t *cr_distinctRandomEz(size_t range, int k, int seed)
{
  (void) seed;
  if ((size_t)RAND_MAX <= range)
    {
      g_print("ERROR, range %lu is out of range. Must be less than RAND_MAX %d\n", range, RAND_MAX);
      return NULL;
    }

  if ((k < 0) || ((size_t)k > (range + 1U)))
    {
      g_print("ERROR, Invalid k: %d or range: %lu\n", k, range);
      return NULL;
    }

  //-- size_t *k_random = new size_t[k];
  size_t *k_random = (size_t *) g_malloc0( ((size_t)k) * sizeof(size_t) );
  if (k_random == NULL)
    {
      g_print("ERROR, Failed to allocated memory for k_random");
      return NULL;
    }
  memset(k_random, -1, ((size_t)k) * sizeof(size_t));

  int i = 0;
  while (i < k)
    {
      //-- size_t r = distribution(gen);
      size_t r;
      if (RAND_bytes((unsigned char *)&r, sizeof(r)) != 1)
        {
          free(k_random);
          g_print("ERROR, OpenSSL RAND_bytes returns with error");
          return NULL;
        }
      r = r % (range + 1U);
      if (cr_exists(r, k_random, k))
        {
          continue;
        }
      k_random[i] = r;
      i++;
    }
  return k_random;
}

