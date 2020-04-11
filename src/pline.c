/*	SCCS Id: @(#)pline.c	3.2	96/02/01	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-1996
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#define NEED_VARARGS /* Uses ... */	/* comment line for pre-compiled headers */
#include "hack.h"
#include "epri.h"

#ifdef OVLB

static boolean no_repeat = FALSE;

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

	if (!line || !*line) return;
	if (index(line, '%')) {
	    Vsprintf(pbuf,line,VA_ARGS);
	    line = pbuf;
	}
	if (!flags.window_inited) {
	    raw_print(line);
	    return;
	}
#ifndef MAC
	if (no_repeat && !strcmp(line, toplines))
	    return;
#endif /* MAC */
	if (vision_full_recalc) vision_recalc(0);
	if (u.ux) flush_screen(1);		/* %% */
	putstr(WIN_MESSAGE, 0, line);
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

/* work buffer for You(), &c and verbalize() */
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

/* `prefix' must be a string literal, not a pointer */
#define YouPrefix(pointer,prefix,text) \
 Strcpy((pointer = You_buf((int)(strlen(text) + sizeof prefix))), prefix)

#define YouMessage(pointer,prefix,text) \
 strcat((YouPrefix(pointer, prefix, text), pointer), text)

/*VARARGS1*/
void
You VA_DECL(const char *, line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
/*JP	vpline(YouMessage(tmp, "You ", line), VA_ARGS);*/
	vpline(YouMessage(tmp, "あなたは", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
Your VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
/*JP	vpline(YouMessage(tmp, "Your ", line), VA_ARGS);*/
	vpline(YouMessage(tmp, "あなたの", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_feel VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
/*JP	vpline(YouMessage(tmp, "You feel ", line), VA_ARGS);*/
	vpline(YouMessage(tmp, "あなたは", line), VA_ARGS);
	VA_END();
}


/*VARARGS1*/
void
You_cant VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
/*JP	vpline(YouMessage(tmp, "You can't ", line), VA_ARGS);*/
	vpline(YouMessage(tmp, "あなたは", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
pline_The VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
/*JP	vpline(YouMessage(tmp, "The ", line), VA_ARGS);*/
	vpline(YouMessage(tmp, "", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_hear VA_DECL(const char *,line)
	char *tmp;
	const char *adj;
	char *p;
	VA_START(line);
	VA_INIT(line, const char *);

	if (!Underwater)
/*JP		YouPrefix(tmp, "You hear ", line);*/
		adj = "";
	else
/*JP		YouPrefix(tmp, "You barely hear ", line);*/
		adj = "かすかに";

	tmp = You_buf(strlen(adj) + strlen(line) + sizeof("あなたは   "));

	Strcpy(tmp, "あなたは");
	if(p = (char *)strstr(line, "聞い")){
	  strncat(tmp, line, (p - line));
	  strcat(tmp, adj);
	  strcat(tmp, p);
	}
	else{
		Strcat(tmp, line);
	}

/*JP	vpline(strcat(tmp, line), VA_ARGS);*/
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
	tmp = You_buf((int)strlen(line) + sizeof "\"\"");
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
	char info[BUFSZ], monnambuf[BUFSZ];

	if (mtmp->ispriest || mtmp->data == &mons[PM_ALIGNED_PRIEST]
				|| mtmp->data == &mons[PM_ANGEL])
		alignment = EPRI(mtmp)->shralign;
	else
		alignment = mtmp->data->maligntyp;
	alignment = (alignment > 0) ? A_LAWFUL :
		(alignment < 0) ? A_CHAOTIC :
		A_NEUTRAL;

	info[0] = 0;
/*JP	if (mtmp->mtame) {	  Strcat(info, ", tame");*/
	if (mtmp->mtame) {	  Strcat(info, ", 飼いならされている");
#ifdef WIZARD
	    if (wizard)		  Sprintf(eos(info), " (%d)", mtmp->mtame);
#endif
	}
/*JP	else if (mtmp->mpeaceful) Strcat(info, ", peaceful");*/
	else if (mtmp->mpeaceful) Strcat(info, ", 友好的");
/*JP	if (mtmp->meating)	  Strcat(info, ", eating");*/
	if (mtmp->meating)	  Strcat(info, ", 食事中");
/*JP	if (mtmp->mcan)		  Strcat(info, ", cancelled");*/
	if (mtmp->mcan)		  Strcat(info, ", 封印されている");
/*JP	if (mtmp->mconf)	  Strcat(info, ", confused");*/
	if (mtmp->mconf)	  Strcat(info, ", 混乱状態");
	if (mtmp->mblinded || !mtmp->mcansee)
/*JP				  Strcat(info, ", blind");*/
				  Strcat(info, ", 盲目");
/*JP	if (mtmp->mstun)	  Strcat(info, ", stunned");*/
	if (mtmp->mstun)	  Strcat(info, ", くらくら状態");
/*	if (mtmp->msleep)	  Strcat(info, ", asleep");*/
	if (mtmp->msleep)	  Strcat(info, ", 眠っている");
#if 0	/* unfortunately mfrozen covers temporary sleep and being busy
	   (donning armor, for instance) as well as paralysis */
/*JP	else if (mtmp->mfrozen)	  Strcat(info, ", paralyzed");*/
	else if (mtmp->mfrozen)	  Strcat(info, ", 麻痺状態");
#else
	else if (mtmp->mfrozen || !mtmp->mcanmove)
/*JP				  Strcat(info, ", can't move");*/
				  Strcat(info, ", 動けない");
#endif
				  /* [arbitrary reason why it isn't moving] */
	else if (mtmp->mstrategy & STRAT_WAITMASK)
/*JP				  Strcat(info, ", meditating");*/
				  Strcat(info, ", 黙想中");
/*JP	else if (mtmp->mflee)	  Strcat(info, ", scared");*/
	else if (mtmp->mflee)	  Strcat(info, ", 怯えている");
/*JP	if (mtmp->mtrapped)	  Strcat(info, ", trapped");*/
	if (mtmp->mtrapped)	  Strcat(info, ", 罠にかかっている");
	if (mtmp->mspeed)	  Strcat(info,
/*JP					mtmp->mspeed == MFAST ? ", fast" :
					mtmp->mspeed == MSLOW ? ", slow" :
					", ???? speed");*/
					mtmp->mspeed == MFAST ? ", 素早い" :
					mtmp->mspeed == MSLOW ? ", 遅い" :
					", 速度 ????");
/*JP	if (mtmp->mundetected)	  Strcat(info, ", concealed");*/
	if (mtmp->mundetected)	  Strcat(info, ", 隠れている");
/*JP	if (mtmp->minvis)	  Strcat(info, ", invisible");*/
	if (mtmp->minvis)	  Strcat(info, ", 不可視");
	if (mtmp == u.ustuck)	  Strcat(info,
/*JP			(u.mtimedone && sticks(uasmon)) ? ", held by you" :
				u.uswallow ? (is_animal(u.ustuck->data) ?
				", swallowed you" :
				", engulfed you") :
				", holding you");*/
			(u.mtimedone && sticks(uasmon)) ? ", あなたが掴まえている" :
				u.uswallow ? (is_animal(u.ustuck->data) ?
				", あなたを飲み込んでいる" :
				", あなたを巻き込んでいる") :
				", あなたを掴まえている");

	Strcpy(monnambuf, mon_nam(mtmp));
	/* avoid "Status of the invisible newt ..., invisible" */
/*JP	if (mtmp->minvis && strstri(monnambuf, "invisible")) {*/
	if (mtmp->minvis && (strstri(monnambuf, "invisible") ||
			     strstr(monnambuf, "姿の見えない"))) {
	    mtmp->minvis = 0;
	    Strcpy(monnambuf, mon_nam(mtmp));
	    mtmp->minvis = 1;
	}

/*JP	pline("Status of %s (%s):  Level %d  HP %d(%d)  AC %d%s.",
		monnambuf,
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mhp,
		mtmp->mhpmax,
		find_mac(mtmp),
		info);*/
	pline("%sの状態 (%s):  Level %d  HP %d(%d)  AC %d%s.",
		monnambuf,
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mhp,
		mtmp->mhpmax,
		find_mac(mtmp),
		info);
}

void
ustatusline()
{
	char info[BUFSZ];

	info[0] = '\0';
	if (Sick) {
/*JP		Strcat(info, ", dying from");*/
		Strcat(info, ", ");
		if (u.usick_type & SICK_VOMITABLE)
/*JP			Strcat(info, " food poisoning");*/
			Strcat(info, "食中毒");
		if (u.usick_type & SICK_NONVOMITABLE) {
			if (u.usick_type & SICK_VOMITABLE)
/*JP				Strcat(info, " and");*/
				Strcat(info, "と");
/*JP			Strcat(info, " illness");*/
			Strcat(info, "病気");
		}
		Strcat(info, "で死につつある");
	}
/*JP	if (Stoned)		Strcat(info, ", solidifying");*/
	if (Stoned)		Strcat(info, ", 石化しつつある");
/*JP	if (Strangled)		Strcat(info, ", being strangled");*/
	if (Strangled)		Strcat(info, ", 首を締められている");
/*JP	if (Vomiting)		Strcat(info, ", nauseated");*/ /* !"nauseous" */
	if (Vomiting)		Strcat(info, ", 吐き気がする");
/*JP	if (Confusion)		Strcat(info, ", confused");*/
	if (Confusion)		Strcat(info, ", 混乱状態");
	if (Blind) {
/*JP	    Strcat(info, ", blind");*/
	    Strcat(info, ", ");
	    if (u.ucreamed) {
/*JP		if ((long)u.ucreamed < Blinded || Blindfolded
						|| !haseyes(uasmon))
		    Strcat(info, ", cover");
		Strcat(info, "ed by sticky goop");*/
		Strcat(info, "ねばねばべとつくもので");
		if ((long)u.ucreamed < Blinded || Blindfolded
						|| !haseyes(uasmon))
		    Strcat(info, "覆われている, ");
	    }	/* note: "goop" == "glop"; variation is intentional */
	    Strcat(info, "盲目");
	}
/*JP	if (Stunned)		Strcat(info, ", stunned");*/
	if (Stunned)		Strcat(info, ", くらくら状態");
	if (Wounded_legs) {
	    const char *what = body_part(LEG);
	    if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
		what = makeplural(what);
/*JP				Sprintf(eos(info), ", injured %s", what);*/
				Sprintf(eos(info), ", %sにけがをしている", what);
	}
/*JP	if (Glib)		Sprintf(eos(info), ", slippery %s",*/
	if (Glib)		Sprintf(eos(info), ", %sがぬるぬる",
					makeplural(body_part(HAND)));
/*JP	if (u.utrap)		Strcat(info, ", trapped");*/
	if (u.utrap)		Strcat(info, ", 罠にかかっている");
/*JP	if (Fast)		Strcat(info, ", fast");*/
	if (Fast)		Strcat(info, ", 素早い");
/*JP	if (u.uundetected)	Strcat(info, ", concealed");*/
	if (u.uundetected)	Strcat(info, ", 隠れている");
/*JP	if (Invis)		Strcat(info, ", invisible");*/
	if (Invis)		Strcat(info, ", 不可視");
	if (u.ustuck) {
/*JP	    if (u.mtimedone && sticks(uasmon))
		Strcat(info, ", holding ");
	    else
		Strcat(info, ", held by ");
	    Strcat(info, mon_nam(u.ustuck));*/
	    Strcat(info, ", ");
	    Strcat(info, mon_nam(u.ustuck));
	    if (u.mtimedone && sticks(uasmon))
		Strcat(info, "を掴まえている");
	    else
		Strcat(info, "に掴まえられている");
	}

/*JP	pline("Status of %s (%s%s):  Level %d  HP %d(%d)  AC %d%s.",*/
	pline("%sの状態 (%s %s):  Level %d  HP %d(%d)  AC %d%s.",
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
		u.mtimedone ? mons[u.umonnum].mlevel : u.ulevel,
		u.mtimedone ? u.mh : u.uhp,
		u.mtimedone ? u.mhmax : u.uhpmax,
		u.uac,
		info);
}

#endif /* OVLB */

/*pline.c*/

