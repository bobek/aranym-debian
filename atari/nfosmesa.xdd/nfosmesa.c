/*
	NF OSMesa MiNT device driver

	ARAnyM (C) 2004,2005 Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

# include "mint/mint.h"

# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/fcntl.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/signal.h"
# include "mint/ssystem.h"
# include "mint/stat.h"
# include "cookie.h"

# include "libkern/libkern.h"

#include "../natfeat/nf_ops.h"
#include "../nfosmesa/nfosmesa_nfapi.h"

/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	3
# define VER_STATUS	

/*
 * messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p NFOSMesa device driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"� " MSG_BUILDDATE " ARAnyM Development Team.\r\n\r\n"


/****************************************************************************/
/* BEGIN kernel interface */

struct kerinfo *kernel;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/* Install time
 */
static ushort install_date;
static ushort install_time;

/*
 * device driver routines - top half
 */
static long _cdecl	nfosmesa_open	(FILEPTR *f);
static long _cdecl	nfosmesa_write	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	nfosmesa_read	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	nfosmesa_lseek	(FILEPTR *f, long where, int whence);
static long _cdecl	nfosmesa_ioctl	(FILEPTR *f, int mode, void *buf);
static long _cdecl	nfosmesa_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	nfosmesa_close	(FILEPTR *f, int pid);
static long _cdecl	nfosmesa_select	(FILEPTR *f, long proc, int mode);
static void _cdecl	nfosmesa_unselect	(FILEPTR *f, long proc, int mode);

/*
 * device driver map
 */

static DEVDRV raw_devtab =
{
	nfosmesa_open,
	nfosmesa_write,
	nfosmesa_read,
	nfosmesa_lseek,
	nfosmesa_ioctl,
	nfosmesa_datime,
	nfosmesa_close,
	nfosmesa_select,
	nfosmesa_unselect,
	NULL, NULL
};


/*
 * debugging stuff
 */

# ifdef DEV_DEBUG
#  define DEBUG(x)	KERNEL_DEBUG x
#  define TRACE(x)	KERNEL_TRACE x
#  define ALERT(x)	KERNEL_ALERT x
# else
#  define DEBUG(x)
#  define TRACE(x)
#  define ALERT(x)	KERNEL_ALERT x
# endif

# ifdef INT_DEBUG
#  define DEBUG_I(x)	KERNEL_DEBUG x
#  define TRACE_I(x)	KERNEL_TRACE x
#  define ALERT_I(x)	KERNEL_ALERT x
# else
#  define DEBUG_I(x)
#  define TRACE_I(x)
#  define ALERT_I(x)	KERNEL_ALERT x
# endif

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

/*
 * global data structures
 */

static struct dev_descr raw_dev_descriptor =
{
	&raw_devtab,
	0,		/* dinfo -> fc.aux */
	0,		/* flags */
	NULL,	/* struct tty * */
	0,		/* drvsize */
	S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
	NULL,	/* bdevmap */
	0,		/* bdev */
	0		/* reserved */
};

static struct nf_ops *nfOps;
static unsigned long nfOSMesaId=0;

/* END global data & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN initialization - top half */

DEVDRV * _cdecl
init (struct kerinfo *k)
{
	kernel = k;
	
	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	
	DEBUG (("%s: enter init", __FILE__));

	/* Check NF presence */
	nfOps = kernel->nf_ops;
	if (!nfOps) {
		c_conws("__NF cookie not present on this system\r\n");
		return NULL;
	}

	/* Check NFOSMesa presence */
	nfOSMesaId=nfOps->get_id("OSMESA");
	if (nfOSMesaId==0) {
		c_conws("NF OSMesa functions not present on this system\r\n");
		return NULL;
	}

	/* Check API version */
	if (nfOps->call(nfOSMesaId+GET_VERSION, 0, 0)!=ARANFOSMESA_NFAPI_VERSION) {
		c_conws("NF OSMesa functions use an incompatible API\r\n");
		return NULL;
	}

	/* Install device */
	if (d_cntl (DEV_INSTALL, "u:\\dev\\nfosmesa", (long) &raw_dev_descriptor)<=0) {
		ALERT (("[NFOSMESA] init: Unable to install device"));
		return NULL;
	}		

	install_time = timestamp;
	install_date = datestamp;

	return (DEVDRV *) 1L;
}

/* END initialization - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver routines - top half */

static long _cdecl
nfosmesa_open (FILEPTR *f)
{
	return E_OK;
}

static long _cdecl
nfosmesa_close (FILEPTR *f, int pid)
{
	return E_OK;
}


/* raw write/read routines
 */
static long _cdecl
nfosmesa_write (FILEPTR *f, const char *buf, long bytes)
{
	return ENOSYS;
}

static long _cdecl
nfosmesa_read (FILEPTR *f, char *buf, long bytes)
{
	return ENOSYS;
}

static long _cdecl
nfosmesa_lseek (FILEPTR *f, long where, int whence)
{
	return ENOSYS;
}

static long _cdecl
nfosmesa_ioctl (FILEPTR *f, int mode, void *buf)
{
	unsigned long *tmp =(unsigned long *)buf;
	
	if (mode!=NFOSMESA_IOCTL) {
		return ENOSYS;
	}
	
	/* Execute command */
	return nfOps->call(nfOSMesaId+tmp[0],tmp[1],tmp[2]);
}

static long _cdecl
nfosmesa_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	if (rwflag)
		return EACCES;
	
	*timeptr++ = install_time;
	*timeptr = install_date;
	
	return E_OK;
}

static long _cdecl
nfosmesa_select (FILEPTR *f, long proc, int mode)
{
	/* we're always ready for I/O */
	return 1;
}

static void _cdecl
nfosmesa_unselect (FILEPTR *f, long proc, int mode)
{
}

/* END device driver routines - top half */
/****************************************************************************/
