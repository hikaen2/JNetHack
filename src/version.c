/*	SCCS Id: @(#)version.c	3.1	92/01/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include	"hack.h"
#include	"date.h"
#ifndef BETA
# ifdef SHORT_FILENAMES
# include	"patchlev.h"
# include	"../japanese/jpatchle.h"
# else
# include	"patchlevel.h"
/*JP*/
# include	"../japanese/jpatchlevel.h"
# endif
#endif

int
doversion()
{
#ifdef BETA
	pline("%s NetHack Beta Version %d.%d.%d-%d - last build %s.",
#else
	pline("%s NetHack Version %d.%d.%d - last build %s.",
#endif
		PORT_ID, VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL,
#ifdef BETA
		EDITLEVEL,
#endif
		BUILD_DATE);	/* from date.h, generated by 'makedefs' */
/*JP*/
	pline("%s JNetHack Version %d.%d.%d",
		PORT_ID, JVERSION_MAJOR, JVERSION_MINOR, JPATCHLEVEL );
	return 0;
}

int
doextversion()
{
	display_file(OPTIONS_USED, TRUE);
	return 0;
}

#ifdef MICRO
boolean
comp_times(filetime)
long filetime;
{
	return((boolean)(filetime < BUILD_TIME));
}
#endif

/*version.c*/
