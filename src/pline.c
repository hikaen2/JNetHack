/*	SCCS Id: @(#)pline.c	3.1	92/11/20	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#define NEED_VARARGS /* Uses ... */	/* comment line for pre-compiled headers */
#include "hack.h"
#include "epri.h"

#ifndef OVLB
STATIC_DCL boolean no_repeat;
#else /* OVLB */
STATIC_OVL boolean no_repeat = FALSE;
#endif /* OVLB */

#ifdef OVLB
static char *FDECL(You_buf, (int));

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vpline, (const char *, va_list));

void
pline VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vpline(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vpline(const char *line, va_list the_args) {
# else
static void
vpline(line, the_args) const char *line; va_list the_args; {
# endif

#else	/* USE_STDARG | USE_VARARG */

#define vpline pline

void
pline VA_DECL(const char *, line)
#endif	/* USE_STDARG | USE_VARARG */

	char pbuf[BUFSZ];
/* Do NOT use VA_START and VA_END in here... see above */

	if(!line || !*line) return;
	if(!index(line, '%'))
	    Strcpy(pbuf,line);
	else
	    Vsprintf(pbuf,line,VA_ARGS);
	if(!flags.window_inited) {
	    raw_print(pbuf);
	    return;
	}
#ifndef MAC
	if(no_repeat && !strcmp(pbuf, toplines))
	    return;
#endif /* MAC */
	if (vision_full_recalc) vision_recalc(0);
	if (u.ux) flush_screen(1);		/* %% */
	putstr(WIN_MESSAGE, 0, pbuf);
}

/*VARARGS1*/
void
Norep VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, const char *);
	no_repeat = TRUE;
	vpline(line, VA_ARGS);
	no_repeat = FALSE;
	VA_END();
	return;
}

/* work buffer for You(), Your(), and verbalize() */
static char *you_buf = 0;
static int you_buf_siz = 0;

static char *
You_buf(siz) int siz; {
	if (siz > you_buf_siz) {
		if (you_buf_siz > 0) free((genericptr_t) you_buf);
		you_buf_siz = siz + 10;
		you_buf = (char *) alloc((unsigned) you_buf_siz);
	}
	return you_buf;
}

/*VARARGS1*/
void
You VA_DECL(const char *, line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	tmp = You_buf((int)strlen(line) + 9);
/*	Strcpy(tmp, "You ");*/
#define JPYOU
#ifdef JPYOU
	Strcpy(tmp, "あなたは");
#else
	tmp[0]='\0';
#endif
	Strcat(tmp, line);
	vpline(tmp, VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
Your VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	tmp = You_buf((int)strlen(line) + 9);
	Strcpy(tmp, "あなたの");
	Strcat(tmp, line);
	vpline(tmp, VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
verbalize VA_DECL(const char *,line)
	char *tmp;
	if (!flags.soundok) return;
	VA_START(line);
	VA_INIT(line, const char *);
	tmp = You_buf((int)strlen(line) + 3);
/*JP	Strcpy(tmp, "\"");*/
	Strcpy(tmp, "「");
	Strcat(tmp, line);
/*JP	Strcat(tmp, "\"");*/
	Strcat(tmp, "」");
	vpline(tmp, VA_ARGS);
	VA_END();
}

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vraw_printf,(const char *,va_list));

void
raw_printf VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vraw_printf(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vraw_printf(const char *line, va_list the_args) {
# else
static void
vraw_printf(line, the_args) const char *line; va_list the_args; {
# endif

#else  /* USE_STDARG | USE_VARARG */

void
raw_printf VA_DECL(const char *, line)
#endif
/* Do NOT use VA_START and VA_END in here... see above */

	if(!index(line, '%'))
	    raw_print(line);
	else {
	    char pbuf[BUFSZ];
	    Vsprintf(pbuf,line,VA_ARGS);
	    raw_print(pbuf);
	}
}


/*VARARGS1*/
void
impossible VA_DECL(const char *, s)
	VA_START(s);
	VA_INIT(s, const char *);
	vpline(s,VA_ARGS);
	pline("Program in disorder - perhaps you'd better Quit.");
	VA_END();
}

const char *
align_str(alignment)
    aligntyp alignment;
{
    switch ((int)alignment) {
/*JP	case A_CHAOTIC: return "chaotic";
	case A_NEUTRAL: return "neutral";
	case A_LAWFUL:	return "lawful";
	case A_NONE:	return "unaligned";*/
	case A_CHAOTIC: return "混沌";
	case A_NEUTRAL: return "中立";
	case A_LAWFUL:	return "秩序";
	case A_NONE:	return "無心";
    }
/*JP    return "unknown";*/
    return "不明";
}

void
mstatusline(mtmp)
register struct monst *mtmp;
{
	aligntyp alignment;

	if (mtmp->ispriest || mtmp->data == &mons[PM_ALIGNED_PRIEST]
				|| mtmp->data == &mons[PM_ANGEL])
		alignment = EPRI(mtmp)->shralign;
	else
		alignment = mtmp->data->maligntyp;

	alignment = (alignment > 0) ? A_LAWFUL :
		(alignment < 0) ? A_CHAOTIC :
		A_NEUTRAL;
/*JP	pline("Status of %s (%s):  Level %d  Gold %lu  HP %d(%d) AC %d%s%s",
		mon_nam(mtmp),
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mgold,
		mtmp->mhp,
		mtmp->mhpmax,
		find_mac(mtmp),
		mtmp->mcan ? ", cancelled" : "" ,
		mtmp->mtame ? ", tame" : "");*/
	pline("%sの状態 (%s):  Level %d  Gold %lu  HP %d(%d) AC %d%s%s",
		mon_nam(mtmp),
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mgold,
		mtmp->mhp,
		mtmp->mhpmax,
		find_mac(mtmp),
		mtmp->mcan ? ", 封印されている" : "" ,
		mtmp->mtame ? ", 飼いならされている" : "");
}

void
ustatusline()
{
/*JP	pline("Status of %s (%s%s):  Level %d  Gold %lu  HP %d(%d)  AC %d",*/
	pline("%sの状態 (%s %s):  Level %d  Gold %lu  HP %d(%d)  AC %d",
		plname,
/*JP		    (u.ualign.record >= 20) ? "piously " :
		    (u.ualign.record > 13) ? "devoutly " :
		    (u.ualign.record > 8) ? "fervently " :
		    (u.ualign.record > 3) ? "stridently " :
		    (u.ualign.record == 3) ? "" :
		    (u.ualign.record >= 1) ? "haltingly " :
		    (u.ualign.record == 0) ? "nominally " :
					    "insufficiently ",*/
		    (u.ualign.record >= 20) ? "敬虔" :
		    (u.ualign.record > 13) ? "信心深い" :
		    (u.ualign.record > 8) ? "熱烈" :
		    (u.ualign.record > 3) ? "声のかん高い" :
		    (u.ualign.record == 3) ? "" :
		    (u.ualign.record >= 1) ? "有名無実" :
		    (u.ualign.record == 0) ? "迷惑" :
					    "不適当",
		align_str(u.ualign.type),
# ifdef POLYSELF
		u.mtimedone ? mons[u.umonnum].mlevel : u.ulevel,
		u.ugold,
		u.mtimedone ? u.mh : u.uhp,
		u.mtimedone ? u.mhmax : u.uhpmax,
# else
		u.ulevel,
		u.ugold,
		u.uhp,
		u.uhpmax,
# endif
		u.uac);
}

#endif /* OVLB */

/*pline.c*/

