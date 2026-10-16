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

/* Hash storage skeleton for the experimental transaction runtime.
   All service bodies are intentionally unimplemented.  Fatal placeholders
   prevent accidental use from looking like a successful hash operation.  */

#include "libgomp.h"
#include "newtm-hash.h"

/* Physical lifetime and collision linkage only.  Policy data remain opaque;
   transaction contexts, reader identities and private buffers belong to the
   separate transaction layer, not to the hash implementation.  */
struct newtm_hash_entry
{
  const void *address;
  size_t size;
  size_t references;
  size_t bucket_index;
  void *policy_data;
  struct newtm_hash_entry *next;
};

struct newtm_hash_bucket
{
  gomp_mutex_t mutex;
  struct newtm_hash_entry *entries;
  size_t entry_count;
};

struct newtm_hash_table
{
  struct newtm_hash_bucket *buckets;
  size_t bucket_count;
  newtm_hash_policy_destroy_fn destroy_policy;
};

/* TODO: Create an empty table with BUCKET_COUNT buckets and one mutex per bucket.
   DESTROY_POLICY owns nonnull entry metadata; it may be null only when all
   metadata remain null.  Store the new table in *OUT.  Return OK, INVALID
   for zero capacity/null OUT, or OVERFLOW for an unrepresentable allocation.
   On failure clear *OUT when OUT is valid.  Allocation failure is fatal.  */
enum newtm_hash_status
newtm_hash_create (size_t bucket_count,
                   newtm_hash_policy_destroy_fn destroy_policy,
                   struct newtm_hash_table **out)
{
  (void) bucket_count;
  (void) destroy_policy;
  (void) out;
  gomp_fatal ("newtm_hash_create: not implemented");
}

/* TODO: With all users quiescent, destroy TABLE and its bucket mutexes if empty.
   Return INVALID for null TABLE, BUSY if any entry remains, or OK on success.
   BUSY must leave the complete table intact; success invalidates TABLE.  */
enum newtm_hash_status
newtm_hash_destroy (struct newtm_hash_table *table)
{
  (void) table;
  gomp_fatal ("newtm_hash_destroy: not implemented");
}

/* TODO: Find or insert ADDRESS with SIZE bytes in TABLE under its bucket mutex.
   Return a retained entry in *OUT, creating it with null metadata if absent.
   Return OK, INVALID for null arguments/zero SIZE, SIZE_MISMATCH for an
   existing key with another size, or OVERFLOW for counters/allocation size.
   Clear *OUT on failure when valid; make no partial changes.  Allocation
   failure is fatal.  Do not read or reserve the program object.  */
enum newtm_hash_status
newtm_hash_acquire (struct newtm_hash_table *table, const void *address,
                    size_t size, struct newtm_hash_entry **out)
{
  (void) table;
  (void) address;
  (void) size;
  (void) out;
  gomp_fatal ("newtm_hash_acquire: not implemented");
}

/* TODO: Look up ADDRESS without insertion and retain the entry under its bucket
   mutex.  Return OK, NOT_FOUND, INVALID for null arguments, or OVERFLOW for
   the reference counter.  Clear *OUT on failure when valid.  The caller must
   release a successful reference; it is not a transactional reservation.  */
enum newtm_hash_status
newtm_hash_lookup_acquire (struct newtm_hash_table *table,
                           const void *address,
                           struct newtm_hash_entry **out)
{
  (void) table;
  (void) address;
  (void) out;
  gomp_fatal ("newtm_hash_lookup_acquire: not implemented");
}

/* TODO: Release one caller-owned reference to ENTRY in TABLE under its bucket lock.
   Unlink at zero references, then unlock, destroy metadata and free the entry.
   Require valid matching TABLE/ENTRY and reservations already withdrawn.
   Return no value.  ENTRY may become invalid; never force removal while used.  */
void
newtm_hash_release (struct newtm_hash_table *table,
                    struct newtm_hash_entry *entry)
{
  (void) table;
  (void) entry;
  gomp_fatal ("newtm_hash_release: not implemented");
}

/* TODO: Return the immutable canonical program-object address stored in ENTRY.
   Require a valid retained reference; no lock or object dereference is needed.
   The returned address does not extend the lifetime of the program object.  */
const void *
newtm_hash_entry_address (const struct newtm_hash_entry *entry)
{
  (void) entry;
  gomp_fatal ("newtm_hash_entry_address: not implemented");
}

/* TODO: Return ENTRY's immutable object size in bytes.  Require a valid retained
   reference; do not read the object or acquire a transactional reservation.  */
size_t
newtm_hash_entry_size (const struct newtm_hash_entry *entry)
{
  (void) entry;
  gomp_fatal ("newtm_hash_entry_size: not implemented");
}

/* TODO: Call nonnull VISIT on the address, size and metadata slot of retained ENTRY
   under its bucket mutex in TABLE, forwarding ARGUMENT (which may be null).
   Require valid matching TABLE/ENTRY.  Return no value; results travel through
   ARGUMENT.  Do not retain the lock after the callback or allow reentrancy.  */
void
newtm_hash_entry_visit (struct newtm_hash_table *table,
                        struct newtm_hash_entry *entry,
                        newtm_hash_visit_fn visit, void *argument)
{
  (void) table;
  (void) entry;
  (void) visit;
  (void) argument;
  gomp_fatal ("newtm_hash_entry_visit: not implemented");
}

/* TODO: Return a consistent entry count, saturated at SIZE_MAX, for valid TABLE.
   Lock every bucket in ascending order, sum counts, unlock in reverse order.
   For diagnostics only; never call from a visit callback.  References and
   transactional participants are not counted.  */
size_t
newtm_hash_count (struct newtm_hash_table *table)
{
  (void) table;
  gomp_fatal ("newtm_hash_count: not implemented");
}

