/*--------------------------------------------------------------------*/
/*--- LibSIG                                                       ---*/
/*---                                                       main.c ---*/
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

#include "config.h"
#include "global.h"

#include "pub_tool_threadstate.h"

/*------------------------------------------------------------*/
/*--- Global variables                                     ---*/
/*------------------------------------------------------------*/

/* for all threads */
CommandLineOptions LSG_(clo);

/* thread and signal handler specific */
exec_state LSG_(current_state);

/*------------------------------------------------------------*/
/*--- Exported global variables                            ---*/
/*------------------------------------------------------------*/

extern Int VG_(cl_exec_fd);
extern Bool VG_(resolve_filename)(Int fd, const HChar** buf);

/*------------------------------------------------------------*/
/*--- Instrumentation                                      ---*/
/*------------------------------------------------------------*/

#if defined(VG_BIGENDIAN)
# define LSG_Endness Iend_BE
#elif defined(VG_LITTLEENDIAN)
# define LSG_Endness Iend_LE
#else
# error "Unknown endianness"
#endif

static
void LSG_(add_instrumentation)(IRSB* sbOut, Addr addr, BoundType bound) {
	addStmtToIRSB(sbOut, IRStmt_Dirty(unsafeIRDirty_0_N(2, "track_bound",
					VG_(fnptr_to_fnentry)(LSG_(track_bound)),
					mkIRExprVec_2(
						mkIRExpr_HWord((HWord) addr),
						mkIRExpr_HWord((HWord) bound)
					))));
}

static IRSB* LSG_(instrument)(VgCallbackClosure* closure, IRSB* sbIn,
		const VexGuestLayout* layout, const VexGuestExtents* vge,
		const VexArchInfo* archinfo_host, IRType gWordTy, IRType hWordTy) {
	Int i;
	IRStmt* st;
	IRSB* sbOut;
	BoundType current, last = Nobound;

	if (gWordTy != hWordTy) {
		/* We don't currently support this case. */
		VG_(tool_panic)("host/guest word size mismatch");
	}

	LSG_DEBUG(3, "+ instrument(%#lx)\n", (Addr) closure->readdr);

	/* Set up SB for instrumented IR */
	sbOut = deepCopyIRSBExceptStmts(sbIn);

	// Copy verbatim any IR preamble preceding the first IMark
	i = 0;
	while (i < sbIn->stmts_used && sbIn->stmts[i]->tag != Ist_IMark) {
		addStmtToIRSB(sbOut, sbIn->stmts[i]);
		i++;
	}

	// Get the first statement, and origAddr from it
	LSG_ASSERT(sbIn->stmts_used > 0);
	LSG_ASSERT(i < sbIn->stmts_used);
	st = sbIn->stmts[i];
	LSG_ASSERT(Ist_IMark == st->tag);

	for (/*use current i*/; i < sbIn->stmts_used; i++) {
		st = sbIn->stmts[i];
		LSG_ASSERT(isFlatIRStmt(st));

		switch (st->tag) {
			case Ist_NoOp:
			case Ist_AbiHint:
			case Ist_PutI:
			case Ist_MBE:
				break;

			case Ist_IMark: {
				Addr cia = st->Ist.IMark.addr + st->Ist.IMark.delta;
				UInt isize = st->Ist.IMark.len;

				// If Vex fails to decode an instruction, the size will be zero.
				// Pretend otherwise.
				if (isize == 0)
					isize = VG_MIN_INSTR_SZB;

				// Sanity-check size.
				tl_assert(
						(VG_MIN_INSTR_SZB <= isize && isize <= VG_MAX_INSTR_SZB)
								|| VG_CLREQ_SZB == isize);

				current = LSG_(addr2bound)(cia);
				if (current != last) {
					LSG_(add_instrumentation)(sbOut, cia, current);
					last = current;
				}

				break;
			}

			case Ist_Put:
			case Ist_WrTmp:
			case Ist_Store:
			case Ist_StoreG:
			case Ist_LoadG:
			case Ist_Dirty:
			case Ist_CAS:
			case Ist_LLSC:
			case Ist_Exit:
				break;

			default:
				tl_assert(0);
				break;
		}

		/* Copy the original statement */
		addStmtToIRSB(sbOut, st);

		LSG_DEBUGIF(5) {
			VG_(printf)("   pass  ");
			ppIRStmt(st);
			VG_(printf)("\n");
		}
	}

	return sbOut;
}

static
void finish(void) {
	HChar* filename;

	LSG_DEBUG(0, "finish()\n");

	LSG_(sync_current_thread)();

	if (LSG_(clo).records_file) {
		filename = VG_(expand_file_name)("--records",
						LSG_(clo).records_file);
		LSG_(dump_records)(filename);
		VG_(free)(filename);
	}

	LSG_(destroy_threads)();
	LSG_(clear_all_ranges)();
}

void LSG_(fini)(Int exitcode) {
	finish();
}

/*--------------------------------------------------------------------*/
/*--- Setup                                                        ---*/
/*--------------------------------------------------------------------*/

static void update_range_from_text_section(void) {
	const HChar* progname;
	const DebugInfo* di;

	if (!VG_(resolve_filename)(VG_(cl_exec_fd), &(progname)))
		VG_(tool_panic)("No program name found");

	for (di = VG_(next_DebugInfo)(0); di; di = VG_(next_DebugInfo)(di)) {
		Addr addr;
		SizeT size;
		const HChar* objname;

		objname = VG_(DebugInfo_get_filename)(di);
		if (VG_(strcmp)(objname, progname))
			continue;

		addr = VG_(DebugInfo_get_text_avma)(di);
		if (addr) {
			size = VG_(DebugInfo_get_text_size)(di);
			LSG_ASSERT(size > 0);

			LSG_(add_new_range)(addr, size);
		}
	}

	// Make sure we have at least one range.
	tl_assert(LSG_(has_ranges)());
}

static void lsg_start_client_code_callback(ThreadId tid, ULong blocks_done) {
	// If no range was selected, lets find one
	// based on the text section of the program.
	// This test could not have been done in post_clo_init,
	// since the program was not loaded yet.
	if (!LSG_(has_ranges)())
		update_range_from_text_section();
}

static
void LSG_(post_clo_init)(void) {
	LSG_(init_threads)();
	LSG_(run_thread)(1);
}

static
void LSG_(pre_clo_init)(void) {
	VG_(details_name)("libsig");
	VG_(details_version)(NULL);
	VG_(details_description)("a dynamic library signature tool");
	VG_(details_copyright_author)("Copyright (C) 2025, and GNU GPL'd, "
			"by Andrei Rimsa");
	VG_(details_bug_reports_to)(VG_BUGS_TO);
	VG_(details_avg_translation_sizeB)(500);

	VG_(basic_tool_funcs)(LSG_(post_clo_init),
			LSG_(instrument), LSG_(fini));

	VG_(needs_command_line_options)(LSG_(process_cmd_line_option),
			LSG_(print_usage), LSG_(print_debug_usage));

	VG_(track_start_client_code)(&lsg_start_client_code_callback);

	LSG_(set_clo_defaults)();
}

VG_DETERMINE_INTERFACE_VERSION(LSG_(pre_clo_init))

/*--------------------------------------------------------------------*/
/*--- end                                                   main.c ---*/
/*--------------------------------------------------------------------*/
