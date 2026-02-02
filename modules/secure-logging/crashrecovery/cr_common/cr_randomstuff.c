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

#include "cr_randomstuff.h"

//----------------------------------------------------------------------
//exists
//in: element Item that is searched for
//in: arr Array Container of items being searched in
//in: size Number of items in array

static gboolean cr_exists(size_t element, const size_t arr[], size_t size)
{
  for (size_t i = 0; i < size; ++i)
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
  if (RAND_MAX <= range)
    {
      g_print("ERROR, range %lu is out of range. Must be less than RAND_MAX %d\n", range, RAND_MAX);
      return NULL;
    }

  if (k < 0 || (size_t)k > (range + 1))
    {
      g_print("ERROR, Invalid k: %d or range: %lu\n", k, range);
      return NULL;
    }

  size_t min = 0;
  size_t max = range;

  //-- size_t *k_random = new size_t[k];
  size_t *k_random = (size_t *) g_malloc0( k * sizeof(size_t) );
  for (int n = 0; n < k; n++)
    {
      k_random[n] = -1;
    }

  //-- initialize pseudo random generator
  // if (0 == seed)
  //   srand(time(NULL));
  // else
  srand(seed);

  int i = 0;
  while (i < k)
    {
      //-- size_t r = distribution(gen);
      size_t r = (rand() % (max - min + 1)) + min;
      if (cr_exists(r, k_random, k))
        {
          continue;
        }
      k_random[i] = r;
      i++;
    }
  return k_random;
}

