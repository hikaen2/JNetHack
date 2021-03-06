/*	SCCS Id: @(#)pcunix.c	3.2	94/11/07	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file collects some Unix dependencies; pager.c contains some more */

#include "hack.h"

#include	<sys/stat.h>
/*JP*/
#if 0
#include	<jctype.h>
#endif

#ifdef OVLB

static struct stat buf;
# ifdef WANT_GETHDATE
static struct stat hbuf;
# endif

void
gethdate(name)
char *name;
{
# ifdef WANT_GETHDATE
/* old version - for people short of space */
/*
/* register char *np;
/*      if(stat(name, &hbuf))
/*	      error("Cannot get status of %s.",
/*		      (np = rindex(name, '/')) ? np+1 : name);
/*
/* version using PATH from: seismo!gregc@ucsf-cgl.ARPA (Greg Couch) */

/*
 * The problem with   #include  <sys/param.h> is that this include file
 * does not exist on all systems, and moreover, that it sometimes includes
 * <sys/types.h> again, so that the compiler sees these typedefs twice.
 */
#define	 MAXPATHLEN      1024

    register char *np, *path;
    char filename[MAXPATHLEN+1], *getenv();

    if (index(name, '/') != (char *)0 || (path = getenv("PATH")) == (char *)0)
	path = "";

    for (;;) {
	if ((np = index(path, ':')) == (char *)0)
	    np = path + strlen(path);       /* point to end str */
	if (np - path <= 1)		     /* %% */
	    Strcpy(filename, name);
	else {
	    (void) strncpy(filename, path, np - path);
	    filename[np - path] = '/';
	    Strcpy(filename + (np - path) + 1, name);
	}
	if (stat(filename, &hbuf) == 0)
	    return;
	if (*np == '\0')
	path = "";
	path = np + 1;
    }
    error("Cannot get status of %s.", (np = rindex(name, '/')) ? np+1 : name);
# endif /* WANT_GETHDATE */
}

#if 0
int
uptodate(fd)
int fd;
{
# ifdef WANT_GETHDATE
    if(fstat(fd, &buf)) {
	pline("Cannot get status of saved level? ");
	return(0);
    }
    if(buf.st_mtime < hbuf.st_mtime) {
	pline("Saved level is out of date. ");
	return(0);
    }
# else
#  if defined(MICRO) && !defined(NO_FSTAT)
    if(fstat(fd, &buf)) {
	if(moves > 1) pline("Cannot get status of saved level? ");
	else pline("Cannot get status of saved game");
	return(0);
    } 
    if(comp_times(buf.st_mtime)) { 
	if(moves > 1) pline("Saved level is out of date");
	else pline("Saved game is out of date. ");
	/* This problem occurs enough times we need to give the player
	 * some more information about what causes it, and how to fix.
	 */
#  ifdef MSDOS
	    pline("Make sure that your system's date and time are correct.");
	    pline("They must be more current than NetHack.EXE's date/time stamp.");
#  endif /* MSDOS */
	return(0);
    }
#  endif  /* MICRO */
# endif /* WANT_GETHDATE */
    return(1);
}
#endif

# ifndef WIN32
void
regularize(s)
/*
 * normalize file name - we don't like .'s, /'s, spaces, and
 * lots of other things
 */
register char *s;
{
/*JP	register char *lp;*/
	register unsigned char *lp;

#ifdef MSDOS
	lp = (unsigned char *)ic2str( s );
	strcpy(s, lp);
#endif

	for (lp = s; *lp; lp++){
/*JP*/
		if (is_kanji(*lp)){
/*		  lp += 2;*/
		  lp++;
		  continue;
		}
		if (*lp <= ' ' || *lp == '"' || (*lp >= '*' && *lp <= ',') ||
		    *lp == '.' || *lp == '/' || (*lp >= ':' && *lp <= '?') ||
# ifdef OS2
		    *lp == '&' || *lp == '(' || *lp == ')' ||
# endif
		    *lp == '|' || *lp >= 127 || (*lp >= '[' && *lp <= ']'))
                        *lp = '_';
	 }
}
# endif /* WIN32 */
#endif /* OVLB */
