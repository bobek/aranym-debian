/*
	General ARAnyM exception class

	ARAnyM (C) 2005 Patrice Mandin

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

#include <stdarg.h>
#include <stdio.h>

#include "aranym_exception.h"

AranymException::AranymException(const char *fmt, ...)
{
	va_list argptr;
	
	va_start(argptr, fmt);
	vsnprintf(errMsg, sizeof(errMsg)-1, fmt, argptr);
	va_end(argptr);

	errMsg[sizeof(errMsg)-1]='\0';
}

AranymException::~AranymException()
{
}

char *AranymException::getErrorMessage(void)
{
	return &errMsg[0];
}
