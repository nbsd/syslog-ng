/*
 * Copyright (c) 2002-2019 One Identity
 * Copyright (c) 2019 Laszlo Budai <laszlo.budai@balabit.com>
 * Copyright (c) 2019 László Várady <laszlo.varady@balabit.com>
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

#include "syslog-ng.h"
#include "dynamic-window-counter.h"

DynamicWindowCounter *
dynamic_window_counter_new(void)
{
  DynamicWindowCounter *self = g_new0(DynamicWindowCounter, 1);
  g_atomic_counter_set(&self->ref_cnt, 1);

  return self;
}

void dynamic_window_counter_init(DynamicWindowCounter *self)
{
  self->window = self->iw_size;
}

DynamicWindowCounter *
dynamic_window_counter_ref(DynamicWindowCounter *self)
{
  g_assert(!self || g_atomic_counter_get(&self->ref_cnt) > 0);

  if (self)
    g_atomic_counter_inc(&self->ref_cnt);

  return self;
}

void dynamic_window_counter_unref(DynamicWindowCounter *self)
{
  g_assert(!self || g_atomic_counter_get(&self->ref_cnt));

  if (self && g_atomic_counter_dec_and_test(&self->ref_cnt))
    {
      g_free(self);
    }
}

void dynamic_window_counter_set_iw_size(DynamicWindowCounter *self, gsize iw_size)
{
  self->iw_size = iw_size;
}

gsize
dynamic_window_counter_request(DynamicWindowCounter *self, gsize requested_size)
{
  gsize offered = MIN(self->window, requested_size);
  self->window -= offered;

  return offered;
}

void dynamic_window_counter_release(DynamicWindowCounter *self, gsize release_size)
{
  self->window += release_size;
  g_assert(self->window <= self->iw_size);
}
