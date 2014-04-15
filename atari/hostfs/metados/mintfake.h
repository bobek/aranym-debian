/*
 * $Id$
 *
 * The ARAnyM MetaDOS driver.
 *
 * This is the FreeMiNT configuration file modified for
 * the ARAnyM HOSTFS.DOS MetaDOS driver
 *
 * 2002 STan
 */

#ifndef _mintfake_h_
#define _mintfake_h_

# include "debug.h"

# ifndef ARAnyM_MetaDOS
# define ARAnyM_MetaDOS
# endif

/* MetaDOS function header macros to let the functions create also metados independent */
# define MetaDOSFile  void *devMD, char *pathNameMD, FILEPTR *fpMD, long retMD, int opcodeMD,
# define MetaDOSDir   void *devMD, char *pathNameMD, DIR *dirMD, long retMD, int opcodeMD,
# define MetaDOSDTA0  void *devMD, char *pathNameMD, DTABUF *dtaMD, long retMD, int opcodeMD
# define MetaDOSDTA0pass  devMD, pathNameMD, dtaMD, retMD, opcodeMD
# define MetaDOSDTA   MetaDOSDTA0,

/* disable some defines from the sys/mint/ *.h */
#ifdef O_GLOBAL
# undef O_GLOBAL
#endif

#ifdef is_terminal
# undef is_terminal
#endif
# define is_terminal(f) (0)

/* disable all tty functions */
# define tty_read(f, buf, count) (0)
# define tty_write(f, buf, count) (0)
# define tty_ioctl(f, cmd, arg) (0)


/* rollback the settings from the FreeMiNT CVS's sys/mint/config.h) */
#ifdef CREATE_PIPES
# undef  CREATE_PIPES
#endif

#ifdef SYSUPDATE_DAEMON
# undef  SYSUPDATE_DAEMON
#endif

#ifdef OLDSOCKDEVEMU
# undef  OLDSOCKDEVEMU
#endif

#ifdef WITH_KERNFS
# undef  WITH_KERNFS
#endif
# define WITH_KERNFS 0

#ifdef DEV_RANDOM
# undef  DEV_RANDOM
#endif

#ifdef PATH_MAX
# undef  PATH_MAX
#endif
# define PATH_MAX 1024

#ifdef SPRINTF_MAX
# undef  SPRINTF_MAX
#endif
# define SPRINTF_MAX 128


#endif /* _mintfake_h_ */
