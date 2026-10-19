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

/* Trace-only access skeleton.  Do not validate or dereference arguments,
   acquire reservations, copy data, or change transaction state here.  */

#include "libgomp.h"
#include "newtm-transaction.h"
#include <stdio.h>

/* TODO: Implement the read contract in newtm-transaction.h and
   OPENMP_TRANSACTION_ACCESS_AGENT.md once its dependencies are available.  */
enum newtm_access_status
newtm_read (struct newtm_context *ctx, const void *object, size_t offset,
	    void *destination, size_t size)
{
  printf ("[newtm] newtm_read: ctx=%p, object=%p, offset=%zu, "
	  "destination=%p, size=%zu\n",
	  (void *) ctx, (void *) object, offset, destination, size);
  fflush (stdout);
  return NEWTM_ACCESS_INVALID;
}

/* TODO: Implement the write contract without publishing to original memory.
   A conflict must be signaled before the requested access takes place.  */
enum newtm_access_status
newtm_write (struct newtm_context *ctx, const void *object, size_t offset,
	     const void *source, size_t size)
{
  printf ("[newtm] newtm_write: ctx=%p, object=%p, offset=%zu, "
	  "source=%p, size=%zu\n",
	  (void *) ctx, (void *) object, offset, (void *) source, size);
  fflush (stdout);
  return NEWTM_ACCESS_INVALID;
}
