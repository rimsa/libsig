/*--------------------------------------------------------------------*/
/*--- LibSIG                                                       ---*/
/*---                                                     global.h ---*/
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

#ifndef LSG_GLOBAL
#define LSG_GLOBAL

#include "pub_tool_basics.h"
#include "pub_tool_vki.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_machine.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_options.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_clientstate.h"


#define LSG_(str) VGAPPEND(vgLibSIG_,str)

/*------------------------------------------------------------*/
/*--- LibSIG compile options                           --- */
/*------------------------------------------------------------*/

/* Enable debug output */
#define LSG_ENABLE_DEBUG	1
#define LSG_DEBUG_MEM	0

/* Syscall Timing in microseconds? 
 * (define to 0 if you get compile errors) */
#define LSG_MICROSYSTIME 0

// Record inbound only: 1
// Record outbound only: 2
// Record inbound and outbound: 3
#define RECORD_MODE 2

typedef
	enum {
		Nobound,
		Inbound,    // This marks program code (i.e.: text section)
		Outbound,   // This marks library code (everything that is not internal)
	}
	BoundType;

typedef struct _Record	Record;
struct _Record {
	Addr addr;
#if RECORD_MODE == 3
	BoundType bound;
#else
	Int count;
#endif

	Record* next;
};

typedef struct _BoundRange	BoundRange;
struct _BoundRange {
	Addr start;
	Addr end;

	BoundRange* next;
};

/*------------------------------------------------------------*/
/*--- Command line options                                 ---*/
/*------------------------------------------------------------*/

typedef struct _CommandLineOptions	CommandLineOptions;
struct _CommandLineOptions {
	BoundRange* ranges;
	const HChar* records_file;
#if LSG_ENABLE_DEBUG
	Int   verbose;
#endif
};

/*------------------------------------------------------------*/
/*--- Structure declarations                               ---*/
/*------------------------------------------------------------*/

typedef struct _thread_info			thread_info;

/*
 * Execution state of main thread or a running signal handler in
 * a thread while interrupted by another signal handler.
 * As there's no scheduling among running signal handlers of one thread,
 * we only need a subset of a full thread state.
 */
typedef struct _exec_state exec_state;
struct _exec_state {
	BoundType bound;
	struct {
		Record* head;
		Record* last;
	} records;
};

/* Thread State 
 *
 * This structure stores thread specific info while a thread is *not*
 * running. See function switch_thread() for save/restore on thread switch.
 *
 */
struct _thread_info {
	exec_state state;
};

/*------------------------------------------------------------*/
/*--- Functions                                            ---*/
/*------------------------------------------------------------*/

/* from clo.c */
void LSG_(set_clo_defaults)(void);
Bool LSG_(process_cmd_line_option)(const HChar*);
void LSG_(print_usage)(void);
void LSG_(print_debug_usage)(void);

/* from main.c */
void LSG_(fini)(Int exitcode);

/* from threads.c */
void LSG_(init_threads)(void);
void LSG_(destroy_threads)(void);
thread_info** LSG_(get_threads)(void);
thread_info* LSG_(get_current_thread)(void);
void LSG_(switch_thread)(ThreadId tid);
void LSG_(forall_threads)(void (*func)(thread_info*));
void LSG_(run_thread)(ThreadId tid);
void LSG_(sync_current_thread)(void);

/* from tracking.c */
Bool LSG_(has_ranges)(void);
void LSG_(add_new_range)(Addr addr, SizeT size);
void LSG_(clear_all_ranges)(void);
BoundType LSG_(addr2bound)(Addr addr);
void LSG_(track_bound)(Addr addr, BoundType bound) VG_REGPARM(2);
void LSG_(dump_records)(const HChar* filename);

/*------------------------------------------------------------*/
/*--- Exported global variables                            ---*/
/*------------------------------------------------------------*/

extern CommandLineOptions LSG_(clo);
extern exec_state LSG_(current_state);
extern ThreadId   LSG_(current_tid);

/*------------------------------------------------------------*/
/*--- Debug output                                         ---*/
/*------------------------------------------------------------*/

#if LSG_ENABLE_DEBUG

#define LSG_DEBUGIF(x) \
	if (UNLIKELY((LSG_(clo).verbose >x)))

#define LSG_DEBUG(x,format,args...)   \
	LSG_DEBUGIF(x) {                  \
		VG_(printf)(format,##args);   \
	}

#define LSG_ASSERT(cond)              \
	if (UNLIKELY(!(cond))) {          \
		tl_assert(cond);              \
	}

#else

#define LSG_DEBUGIF(x) if (0)
#define LSG_DEBUG(x...) {}
#define LSG_ASSERT(cond) tl_assert(cond);

#endif

#if LSG_DEBUG_MEM

void* LSG_(malloc)(const HChar* cc, UWord s, const HChar* f);
void* LSG_(realloc)(const HChar* cc, void* p, UWord s, const HChar* f);
void LSG_(free)(void* p, const HChar* f);
HChar* LSG_(strdup)(const HChar* cc, const HChar* s, const HChar* f);

#define LSG_MALLOC(_cc,x)			LSG_(malloc)((_cc),x,__FUNCTION__)
#define LSG_FREE(p)					LSG_(free)(p,__FUNCTION__)
#define LSG_REALLOC(_cc,p,x)		LSG_(realloc)((_cc),p,x,__FUNCTION__)
#define LSG_STRDUP(_cc,s)			LSG_(strdup)((_cc),s,__FUNCTION__)

#else

#define LSG_MALLOC(_cc,x)			VG_(malloc)((_cc),x)
#define LSG_FREE(p)					VG_(free)(p)
#define LSG_REALLOC(_cc,p,x)		VG_(realloc)((_cc),p,x)
#define LSG_STRDUP(_cc,s)			VG_(strdup)((_cc),s)
#endif

#define LSG_UNUSED(arg)				(void)arg;
#define LSG_DATA_FREE(p,x)			\
	do { 							\
		VG_(memset)(p, 0x41, x);	\
		LSG_FREE(p); 				\
	} while (0)

#endif /* LSG_GLOBAL */
