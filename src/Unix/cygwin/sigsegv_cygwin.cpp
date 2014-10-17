#include "sysdeps.h"
#include "cpu_emulation.h"
#define DEBUG 0
#include "debug.h"

/******************************************************************************/

#ifdef OS_cygwin

#define WIN32_LEAN_AND_MEAN /* avoid including junk */
#include <windows.h>
#include <winerror.h>

#ifdef CPU_i386
#define CONTEXT_NAME    ContextRecord
#define CONTEXT_TYPE    CONTEXT
#define CONTEXT_ATYPE   CONTEXT_TYPE *
#define CONTEXT_REGS    (&CONTEXT_NAME->Edi)
#define REG_RIP 7
#define REG_RAX 5
#define REG_RBX 2
#define REG_RCX 4
#define REG_RDX 3
#define REG_RBP 6
#define REG_RSI 1
#define REG_RDI 0
#define REG_RSP 10
#endif

#ifdef CPU_x86_64
#define CONTEXT_NAME    ContextRecord
#define CONTEXT_TYPE    CONTEXT
#define CONTEXT_ATYPE   CONTEXT_TYPE *
#define CONTEXT_REGS    (&CONTEXT_NAME->Rax)
#define REG_RIP 16
#define REG_RAX 0
#define REG_RBX 3
#define REG_RCX 1
#define REG_RDX 2
#define REG_RBP 5
#define REG_RSI 6
#define REG_RDI 7
#define REG_RSP 4
#define REG_R8  8
#define REG_R9  9
#define REG_R10 10
#define REG_R11 11
#define REG_R12 12
#define REG_R13 13
#define REG_R14 14
#define REG_R15 15
#endif

#if defined(CPU_i386) || defined(CPU_x86_64)
#define CONTEXT_AEFLAGS (CONTEXT_NAME->EFlags)
#define CONTEXT_AEIP	CONTEXT_REGS[REG_RIP]
#define CONTEXT_AEAX	CONTEXT_REGS[REG_RAX]
#define CONTEXT_AEBX	CONTEXT_REGS[REG_RBX]
#define CONTEXT_AECX	CONTEXT_REGS[REG_RCX]
#define CONTEXT_AEDX	CONTEXT_REGS[REG_RDX]
#define CONTEXT_AEBP	CONTEXT_REGS[REG_RBP]
#define CONTEXT_AESI	CONTEXT_REGS[REG_RSI]
#define CONTEXT_AEDI	CONTEXT_REGS[REG_RDI]
#endif

#include "sigsegv_common_x86.h"

static int main_exception_filter_installed = 0;

static LONG WINAPI
main_exception_filter (EXCEPTION_POINTERS *ExceptionInfo)
{
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
  {
  	CONTEXT_ATYPE CONTEXT_NAME = ExceptionInfo->ContextRecord;
  	char *fault_addr = (char *)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
	D(bug("\nsegfault: pc=%08x, eip=%08lx, addr=%p (0x%08x)", m68k_getpc(), CONTEXT_REGS[REG_RIP], fault_addr, (memptr)(uintptr)(fault_addr - FMEMORY)));
    handle_access_fault(CONTEXT_NAME, (memptr)(uintptr)(fault_addr - FMEMORY));
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

/* In Cygwin programs, SetUnhandledExceptionFilter has no effect because Cygwin
   installs a global exception handler.  We have to dig deep in order to install
   our main_exception_filter.  */

/* Data structures for the current thread's exception handler chain.
   On the x86 Windows uses register fs, offset 0 to point to the current
   exception handler; Cygwin mucks with it, so we must do the same... :-/ */

/* Magic taken from winsup/cygwin/include/exceptions.h.  */

struct exception_list
  {
    struct exception_list *prev;
    int (*handler) (EXCEPTION_RECORD *, void *, CONTEXT *, void *);
  };
typedef struct exception_list exception_list;

/* Magic taken from winsup/cygwin/exceptions.cc.  */

__asm__ (".equ __except_list,0");

extern exception_list *_except_list __asm__ ("%fs:__except_list");

/* For debugging.  _except_list is not otherwise accessible from gdb.  */
static exception_list *
__attribute__((used))
debug_get_except_list ()
{
  return _except_list;
}

/* Cygwin's original exception handler.  */
static int (*cygwin_exception_handler) (EXCEPTION_RECORD *, void *, CONTEXT *, void *);

/* Our exception handler.  */
static int
aranym_exception_handler (EXCEPTION_RECORD *exception, void *frame, CONTEXT *context, void *dispatch)
{
  EXCEPTION_POINTERS ExceptionInfo;
  ExceptionInfo.ExceptionRecord = exception;
  ExceptionInfo.ContextRecord = context;
  if (main_exception_filter (&ExceptionInfo) == EXCEPTION_CONTINUE_SEARCH)
    return cygwin_exception_handler (exception, frame, context, dispatch);
  else
    return 0;
}

static void
do_install_main_exception_filter ()
{
  /* We cannot insert any handler into the chain, because such handlers
     must lie on the stack (?).  Instead, we have to replace(!) Cygwin's
     global exception handler.  */
  cygwin_exception_handler = _except_list->handler;
  _except_list->handler = aranym_exception_handler;
}

static void
install_main_exception_filter ()
{

  if (!main_exception_filter_installed)
    {
      do_install_main_exception_filter ();
      main_exception_filter_installed = 1;
    }
}

static void uninstall_main_exception_filter ()
{
	if (main_exception_filter_installed)
	{
		_except_list->handler = cygwin_exception_handler;
		main_exception_filter_installed = 0;
	}
}

void uninstall_sigsegv ()
{
	uninstall_main_exception_filter();
}

void cygwin_abort()
{
	uninstall_sigsegv();
#undef abort
	abort();
}

void install_sigsegv ()
{
  install_main_exception_filter ();
}

#endif /* OS_cygwin */
