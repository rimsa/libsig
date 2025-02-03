/*--------------------------------------------------------------------*/
/*--- LibSIG                                                       ---*/
/*---                                                   tracking.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of LibSIG, a dynamic library signature tool.

   Copyright (C) 2025, Andrei Rimsa (andrei@cefetmg.br)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.

   The GNU General Public License is contained in the file COPYING.
*/

#include "global.h"

#include "pub_tool_threadstate.h"

#if RECORD_MODE == 1
	#define RECORD_INBOUNDS_ONLY
#elif RECORD_MODE == 2
	#define RECORD_OUTBOUNDS_ONLY
#elif RECORD_MODE == 3
	#define RECORD_IN_AND_OUTBOUNDS
#else
	#error "Invalid record mode"
#endif

static VgFile* outfile = 0;

static
const HChar* bound2str(BoundType bound) {
	switch (bound) {
		case Nobound:
			return "nobound";
		case Inbound:
			return "inbound";
		case Outbound:
			return "outbound";
		default:
			VG_(tool_panic)("unknown bound");
	}
}

Bool LSG_(has_ranges)(void) {
	return LSG_(clo).ranges != 0;
}

void LSG_(add_new_range)(Addr addr, SizeT size) {
	static BoundRange** last = &(LSG_(clo).ranges);

	tl_assert(addr != 0);
	tl_assert(size > 0);

	*last = (BoundRange*)
		LSG_MALLOC("lsg.tracking.anr.1", sizeof(BoundRange));
	(*last)->start = addr;
	(*last)->end = addr + size;
	(*last)->next = 0;

	LSG_DEBUG(3, "Added new range from 0x%lx to 0x%lx\n",
		(*last)->start, (*last)->end);

	last = &((*last)->next);
}

void LSG_(clear_all_ranges)(void) {
	BoundRange* current = LSG_(clo).ranges;
	while (current) {
		BoundRange* tmp = current->next;
		LSG_DATA_FREE(current, sizeof(BoundRange));
		current = tmp;
	}

	LSG_(clo).ranges = 0;
}

BoundType LSG_(addr2bound)(Addr addr) {
	BoundRange* current = LSG_(clo).ranges;
	while (current) {
		if (addr >= current->start && addr <= current->end)
			return Inbound;

		current = current->next;
	}

	return Outbound;
}

VG_REGPARM(2)
void LSG_(track_bound)(Addr addr, BoundType bound) {
	ThreadId tid;

	/* This is needed because thread switches can not reliable be tracked
	 * with callback LSG_(run_thread) only: we have otherwise no way to get
	 * the thread ID after a signal handler returns.
	 * This could be removed again if that bug is fixed in Valgrind.
	 * This is in the hot path but hopefully not to costly.
	 */
	tid = VG_(get_running_tid)();
#if 1
	/* LSG_(switch_thread) is a no-op when tid is equal to LSG_(current_tid).
	 * As this is on the hot path, we only call LSG_(switch_thread)(tid)
	 * if tid differs from the LSG_(current_tid).
	 */
	if (UNLIKELY(tid != LSG_(current_tid)))
		LSG_(switch_thread)(tid);
#else
	LSG_ASSERT(VG_(get_running_tid)() == LSG_(current_tid));
#endif

	// Only accounts when bound changes.
	if (LSG_(current_state).bound != bound) {
		// Update the new bound.
		LSG_(current_state).bound = bound;

#if defined(RECORD_INBOUNDS_ONLY)
        if (bound == Inbound) {
#elif defined(RECORD_OUTBOUNDS_ONLY)
        if (bound == Outbound) {
#else
        {
#endif
			Record* last = LSG_(current_state).records.last;
#if !defined(RECORD_IN_AND_OUTBOUNDS)
            if (last && last->addr == addr) {
                last->count++;
            } else {
#endif
                Record* curr = (Record*)
                    LSG_MALLOC("lsg.tracking.tb.1", sizeof(Record));
                curr->addr = addr;
#if defined(RECORD_IN_AND_OUTBOUNDS)
                curr->bound = bound;
#else
                curr->count = 1;
#endif
                curr->next = 0;

				if (last) {
					last->next = curr;
					LSG_(current_state).records.last = curr;
				} else {
					LSG_(current_state).records.head = curr;
					LSG_(current_state).records.last = curr;
				}
#if !defined(RECORD_IN_AND_OUTBOUNDS)
            }
#endif

            LSG_DEBUG(2, "Found %s at 0x%lx\n",
                bound2str(bound),
				(LSG_(current_state).records.last)->addr);
        }
	}
}

static void process_thread(thread_info* t) {
	Record* record;

	VG_(fprintf)(outfile, "# Thread: %u\n", LSG_(current_tid));

	record = t->state.records.head;
	while (record) {
#if defined(RECORD_IN_AND_OUTBOUNDS)
		VG_(fprintf)(outfile, "0x%lx,%s\n", record->addr, bound2str(record->bound));
#else
		VG_(fprintf)(outfile, "0x%lx,%d\n", record->addr, record->count);
#endif
		record = record->next;
	}
}

void LSG_(dump_records)(const HChar* filename) {
	outfile = VG_(fopen)(filename, VKI_O_WRONLY|VKI_O_TRUNC, 0);
	if (outfile == NULL) {
		outfile = VG_(fopen)(filename, VKI_O_CREAT|VKI_O_WRONLY,
				VKI_S_IRUSR|VKI_S_IWUSR);
	}
	LSG_ASSERT(outfile != 0);

	LSG_(forall_threads)(process_thread);

	VG_(fclose)(outfile);
	outfile = 0;
}
