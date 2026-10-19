/* Copyright (C) 2026 Free Software Foundation, Inc.

   This file is part of the GNU Offloading and Multi Processing Library
   (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* Internal access interface for the experimental transaction runtime.
   Trace-only skeleton: both operations print parameters and return INVALID.
   No argument is dereferenced and no buffer or context is modified.  The
   functional contracts below describe future work, not current behavior.  */

#ifndef LIBGOMP_NEWTM_TRANSACTION_H
#define LIBGOMP_NEWTM_TRANSACTION_H

#include <stddef.h>

struct newtm_context;

enum newtm_access_status
{
  NEWTM_ACCESS_OK,
  NEWTM_ACCESS_CONFLICT,
  NEWTM_ACCESS_INVALID
};

/* Future contract: read SIZE bytes at OFFSET in registered canonical OBJECT
   into private DESTINATION using active, owned CTX.  Require positive SIZE,
   valid nonoverlapping storage and an in-bounds range.  Obtain a compatible
   reservation before reading; observe this attempt's pending writes.
   On OK fill DESTINATION completely; on failure leave it unchanged.  Never
   publish data or release the attempt here.  No bucket lock escapes a return.
   Current stub: print all parameters, return INVALID, modify nothing.  */
extern enum newtm_access_status newtm_read (struct newtm_context *ctx,
					  const void *object, size_t offset,
					  void *destination, size_t size)
  __attribute__ ((visibility ("hidden")));

/* Future contract: write SIZE bytes from private SOURCE at OFFSET in registered
   canonical OBJECT using active, owned CTX.  Require positive SIZE, valid
   nonoverlapping storage, a writable supported type and an in-bounds range.
   Obtain an exclusive reservation before changing private data.  On OK update
   only this attempt's private view; on failure do not apply this operation,
   even partially.  Preserve SOURCE and original memory in all normal returns.
   Do not perform commit or full cleanup.  No bucket lock escapes a return.
   Current stub: print all parameters, return INVALID, modify nothing.  */
extern enum newtm_access_status newtm_write (struct newtm_context *ctx,
					   const void *object, size_t offset,
					   const void *source, size_t size)
  __attribute__ ((visibility ("hidden")));

#endif /* LIBGOMP_NEWTM_TRANSACTION_H */
