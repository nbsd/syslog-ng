/*
 * Copyright (c) 2025-2026 Airbus Commercial Aircraft
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

/* File: cr_destination-parser.h */

#ifndef CR_DESTINATION_PARSER_H_INCLUDED
#define CR_DESTINATION_PARSER_H_INCLUDED

#include "cfg-parser.h"
#include "driver.h"

extern CfgParser cr_destination_parser;

CFG_PARSER_DECLARE_LEXER_BINDING(cr_destination_, CR_DESTINATION_, LogDriver **)

#endif
