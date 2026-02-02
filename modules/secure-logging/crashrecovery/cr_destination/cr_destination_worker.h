/*
 * Copyright (c) 2015-2026 Airbus Commercial Aircraft
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

/* File: cr_destination_worker.h */

#ifndef CR_DESTINATION_WORKER_H_INCLUDED
#define CR_DESTINATION_WORKER_H_INCLUDED 1

#include "logthrdest/logthrdestdrv.h"
#include "thread-utils.h"


typedef struct _CrDestinationWorker
{
  LogThreadedDestWorker super;
  FILE *file;
  ThreadId thread_id;
} CrDestinationWorker;

LogThreadedDestWorker *cr_destination_dw_new(LogThreadedDestDriver *o, gint worker_index);

#endif
