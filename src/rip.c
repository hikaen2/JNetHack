/*	SCCS Id: @(#)rip.c	3.2	95/05/01	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-1996
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

static void FDECL(center, (int, char *));

extern const char *killed_by_prefix[];

#if defined(TTY_GRAPHICS) || defined(X11_GRAPHICS) || defined(mac) || defined(__beos__)

static const char *rip_txt[] = {
"              ----------                      ----------",
"             /          \\                    /          \\",
"            /    REST    \\                  /    This    \\",
"           /      IN      \\                /  release of  \\",
"          /     PEACE      \\              /   NetHack is   \\",
"         /                  \\            /   dedicated to   \\",
"         |                  |            |  the memory of   |",
"         |                  |            |                  |",
"         |                  |            |  Izchak Miller   |",
"         |                  |            |   1935 - 1994    |",
"         |                  |            |                  |",
"         |                  |            |     Ascended     |",
"         |       1001       |            |                  |",
"      *  |     *  *  *      | *        * |      *  *  *     | *",
" _____)/\\|\\__//(\\/(/\\)/\\//\\/|_)________)/|\\\\_/_/(\\/(/\\)/\\/\\/|_)____",
0
};

#define STONE_LINE_CENT 19	/* char[] element of center of stone face */
#define STONE_LINE_LEN 16	/* # chars that fit on one line
				 * (note 1 ' ' border)
				 */
#define NAME_LINE 6		/* *char[] line # for player name */
#define GOLD_LINE 7		/* *char[] line # for amount of gold */
#define DEATH_LINE 8		/* *char[] line # for death description */
#define YEAR_LINE 12		/* *char[] line # for year */

char **rip;

static void
center(line, text)
int line;
char *text;
{
	register char *ip,*op;
	ip = text;
	op = &rip[line][STONE_LINE_CENT - ((strlen(text)+1)>>1)];
	while(*ip) *op++ = *ip++;
}


void
genl_outrip(tmpwin, how)
winid tmpwin;
int how;
{
	register char **dp;
	register char *dpx;
	char buf[BUFSZ];
	register int x;
	int line;

	rip = dp = (char **) alloc(sizeof(rip_txt));
	for (x = 0; rip_txt[x]; x++) {
		dp[x] = (char *) alloc((unsigned int)(strlen(rip_txt[x]) + 1));
		Strcpy(dp[x], rip_txt[x]);
	}
	dp[x] = (char *)0;

	/* Put name on stone */
	Sprintf(buf, "%s", plname);
	buf[STONE_LINE_LEN] = 0;
	center(NAME_LINE, buf);

	/* Put $ on stone */
	Sprintf(buf, "%ld Au", u.ugold);
	buf[STONE_LINE_LEN] = 0; /* It could be a *lot* of gold :-) */
	center(GOLD_LINE, buf);

	/* Put together death description */
	switch (killer_format) {
		default: impossible("bad killer format?");
		case KILLED_BY_AN:
			Strcpy(buf, an(killer));
			Strcat(buf, killed_by_prefix[how]);
			break;
		case KILLED_BY:
			Strcpy(buf, killer);
			Strcat(buf, killed_by_prefix[how]);
			break;
		case NO_KILLER_PREFIX:
			Strcpy(buf, killer);
			break;
	}

	/* Put death type on stone */
	for (line=DEATH_LINE, dpx = buf; line<YEAR_LINE; line++) {
		register int i,i0;
		char tmpchar;
/*JP		*/
		unsigned char *uc;
		int jstone_line;

		if ((i0=strlen(dpx))<= STONE_LINE_LEN)
		  jstone_line = STONE_LINE_LEN;
		else if (i0/2 <= STONE_LINE_LEN )
		  jstone_line = ((i0+3)/4)*2;
		else if (i0/3 <= STONE_LINE_LEN )
		  jstone_line = ((i0+5)/6)*2;
		else
		  jstone_line = ((i0+7)/8)*2;

/*JP		if ( (i0=strlen(dpx)) > STONE_LINE_LEN) {
				for(i = STONE_LINE_LEN;
				    ((i0 > STONE_LINE_LEN) && i); i--)*/
		if ( i0 > jstone_line) {
				for(i = jstone_line;
				    ((i0 > jstone_line) && i); i--)
					if(dpx[i] == ' ') i0 = i;
/*JP*/
/*				if(!i) i0 = STONE_LINE_LEN;*/
				if(!i){
				  i0=0;
/*JP				  while(i0<STONE_LINE_LEN){*/
				  while(i0<jstone_line){
				    uc = (unsigned char *)(dpx+i0);
				    if(*uc <128)
				      ++i0;
				    else
				      i0+=2;
				  }
				}
		}
		tmpchar = dpx[i0];
		dpx[i0] = 0;
		center(line, dpx);
		if (tmpchar != ' ') {
			dpx[i0] = tmpchar;
			dpx= &dpx[i0];
		} else  dpx= &dpx[i0+1];
	}

	/* Put year on stone */
	Sprintf(buf, "%4d", getyear());
	center(YEAR_LINE, buf);

	putstr(tmpwin, 0, "");
	for(; *dp; dp++)
		putstr(tmpwin, 0, *dp);

	putstr(tmpwin, 0, "");
	putstr(tmpwin, 0, "");

	for (x = 0; rip_txt[x]; x++) {
		free((genericptr_t)rip[x]);
	}
	free((genericptr_t)rip);
}

#endif

/*rip.c*/
