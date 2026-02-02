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

/* File: cr_destination_plugin.c */


#include "cfg-parser.h"
#include "plugin.h"
#include "plugin-types.h"

extern CfgParser cr_destination_parser;

static Plugin  cr_destination_plugins[] =
{
  {
    .type = LL_CONTEXT_DESTINATION,
    .name = "cr_destination",
    .parser = &cr_destination_parser,
  },
};

gboolean
cr_destination_module_init(PluginContext *context, CfgArgs *args)
{
  plugin_register(context, cr_destination_plugins, G_N_ELEMENTS(cr_destination_plugins));
  return TRUE;
}

const ModuleInfo module_info =
{
  .canonical_name = "cr_destination",
  .version = SYSLOG_NG_VERSION,
  .description = "Part of syslog-ng module secure-logging variant crash recovery",
  .core_revision = SYSLOG_NG_SOURCE_REVISION,
  .plugins = cr_destination_plugins,
  .plugins_len = G_N_ELEMENTS(cr_destination_plugins),
};
