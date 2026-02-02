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



//  based on
//  LoggerContext.h
//  logger
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Gollum on 17.01.24.
//

#ifndef cr_pi_logger_context_h
#define cr_pi_logger_context_h

// A struct to hold the context of the logger.
typedef struct
{
  char *p_MasterKeyPath;
  char *p_OutputDirectoryPath;
  char *p_InputPlainLogPath;
  char *p_OutputEncLogPath;
  int maxLogs;
} cr_pi_logger_context;

#endif /* cr_pi_logger_context_h */

