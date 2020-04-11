/*	SCCS Id: @(#)end.c	3.1	93/06/30	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#define NEED_VARARGS	/* comment line for pre-compiled headers */

#include "hack.h"
#include "eshk.h"
#ifndef NO_SIGNAL
#include <signal.h>
#endif


STATIC_PTR int NDECL(done_intr);
static void FDECL(disclose,(int,BOOLEAN_P));
static struct obj *FDECL(get_valuables, (struct obj *));
static void FDECL(savelife, (int));
static void NDECL(list_vanquished);
static void NDECL(list_genocided);

#ifdef AMIGA
void NDECL(clear_icon);
#endif

/*
 * The order of these needs to match the macros in hack.h.
 */
static NEARDATA const char *deaths[] = {		/* the array of death */
	"died", "choked", "poisoned", "starvation", "drowning",
/*JP	"burning", "crushed", "turned to stone", "genocided",*/
	"burning", "crushed", "turned to stone", "died", "genocided",
	"panic", "trickery",
	"quit", "escaped", "ascended"
};

static NEARDATA const char *ends[] = {		/* "when you..." */
/*JP	"died", "choked", "were poisoned", "starved", "drowned",
	"burned", "were crushed", "turned to stone", "were genocided",
	"panicked", "were tricked",
	"quit", "escaped", "ascended"*/
  "殺された","窒息した", "毒におかされた", "餓死した", "溺死した",
  "燃死した", "押し潰された", "石になった", "死んだ", "虐殺された",
  "パニックにおちいった", "奇妙な出来事に会った",
  "逃亡した", "脱出した", "昇天した"
};

int
done1()
{
#ifndef NO_SIGNAL
	(void) signal(SIGINT,SIG_IGN);
#endif
	if(flags.ignintr) {
#ifndef NO_SIGNAL
		(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
		clear_nhwindow(WIN_MESSAGE);
		curs_on_u();
		wait_synch();
		if(multi > 0) nomul(0);
		return 0;
	}
	return done2();
}

int
done2()
{
/*JP	if(yn("Really quit?") == 'n') {*/
	if(yn("本当に?") == 'n') {
#ifndef NO_SIGNAL
		(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#endif
		clear_nhwindow(WIN_MESSAGE);
		curs_on_u();
		wait_synch();
		if(multi > 0) nomul(0);
		if(multi == 0) {
		    u.uinvulnerable = FALSE;	/* avoid ctrl-C bug -dlc */
		    u.usleep = 0;
		}
		return 0;
	}
#if defined(WIZARD) && (defined(UNIX) || defined(VMS) || defined(LATTICE))
	if(wizard) {
	    int c;
# ifdef VMS
	    const char *tmp = "Enter debugger?";
# else
#  ifdef LATTICE
	    const char *tmp = "Create SnapShot?";
#  else
	    const char *tmp = "Dump core?";
#  endif
# endif
	    if ((c = ynq(tmp)) == 'y') {
		(void) signal(SIGINT, (SIG_RET_TYPE) done1);
		exit_nhwindows(NULL);
#ifdef AMIGA
		Abort(0);
#else
# ifdef SYSV
		(void)
# endif
		    abort();
#endif
	    } else if (c == 'q') done_stopprint++;
	}
#endif
#ifndef LINT
	done(QUIT);
#endif
	return 0;
}

STATIC_PTR
int
done_intr()
{
	done_stopprint++;
#ifndef NO_SIGNAL
	(void) signal(SIGINT, SIG_IGN);
# if defined(UNIX) || defined(VMS)
	(void) signal(SIGQUIT, SIG_IGN);
# endif
#endif /* NO_SIGNAL */
	return 0;
}

#if defined(UNIX) || defined(VMS)
static
int
done_hangup()
{
	done_hup++;
	(void)signal(SIGHUP, SIG_IGN);
	(void)done_intr();
	return 0;
}
#endif

void
done_in_by(mtmp)
register struct monst *mtmp;
{
	char buf[BUFSZ];

/*JP	You("die...");*/
	pline("あなたは死にました．．．");
	mark_synch();	/* flush buffered screen output */
	buf[0] = '\0';
	if (type_is_pname(mtmp->data) || (mtmp->data->geno & G_UNIQ)) {
/*JP	     if (!(type_is_pname(mtmp->data) && (mtmp->data->geno & G_UNIQ)))
		Strcat(buf, "the ");*/
	     killer_format = KILLED_BY;
	}
	if (mtmp->minvis)
/*JP		Strcat(buf, "invisible ");*/
		Strcat(buf, "透明な");
	if (Hallucination)
/*JP		Strcat(buf, "hallucinogen-distorted ");*/
		Strcat(buf, "幻覚で歪んだ");

	if (mtmp->mnamelth) Sprintf(eos(buf), "%sと呼ばれる", NAME(mtmp));

	if(mtmp->data == &mons[PM_GHOST]) {
		register char *gn = (char *) mtmp->mextra;
		if (!Hallucination && !mtmp->minvis && *gn) {
/*JP			Strcat(buf, "the ");*/
			killer_format = KILLED_BY;
		}
/*JP		Sprintf(eos(buf), (*gn ? "ghost of %s" : "ghost%s"), gn);*/
		Sprintf(eos(buf), (*gn ? "%sの幽霊" : "ghost%s"), gn);
	} else if(mtmp->isshk) {
/*JP		Sprintf(eos(buf), "%s %s, the shopkeeper",
			(mtmp->female ? "Ms." : "Mr."), shkname(mtmp));*/
		Sprintf(eos(buf), "%sと言う名の店主",
			shkname(mtmp));
		killer_format = KILLED_BY;
	} else if (mtmp->ispriest || mtmp->isminion) {
		killer = priestname(mtmp);
		Strcat(buf, killer);
/*JP		if (!strncmp(killer, "the ", 4)) Strcat(buf, killer+4);
		else Strcat(buf, killer);*/
	} else Strcat(buf, jtrns_mon(mtmp->data->mname));
/*JP	if (mtmp->mnamelth) Sprintf(eos(buf), " called %s", NAME(mtmp));*/
	killer = buf;
	if (mtmp->data->mlet == S_WRAITH)
		u.ugrave_arise = PM_WRAITH;
	else if (mtmp->data->mlet == S_MUMMY)
		u.ugrave_arise = (pl_character[0]=='E') ?
						PM_ELF_MUMMY : PM_HUMAN_MUMMY;
	else if (mtmp->data->mlet == S_VAMPIRE)
		u.ugrave_arise = PM_VAMPIRE;
	if (u.ugrave_arise > -1 && (mons[u.ugrave_arise].geno & G_GENOD))
		u.ugrave_arise = -1;
	if (mtmp->data->mlet == S_COCKATRICE)
		done(STONING);
	else
		done(DIED);
	return;
}

/*VARARGS1*/
boolean panicking;
extern boolean hu;	/* from save.c */

void
panic VA_DECL(const char *, str)
	VA_START(str);
	VA_INIT(str, char *);

	if(panicking++)
#ifdef AMIGA
	    Abort(0);
#else
# ifdef SYSV
	    (void)
# endif
		abort();    /* avoid loops - this should never happen*/
#endif

	if (flags.window_inited) exit_nhwindows(NULL);
	flags.window_inited = 0; /* they're gone; force raw_print()ing */

/*JP	raw_print(" Suddenly, the dungeon collapses.");*/
	raw_print("突然迷宮が崩れた．");
#if defined(WIZARD) && !defined(MICRO)
	if(!wizard) {
/*JP	    raw_printf("Report error to %s and it may be possible to rebuild.",*/
	    raw_printf("%sに報告．エラー発生．再実行不可能．",
# ifdef WIZARD_NAME	/*(KR1ED)*/
		WIZARD_NAME);
# else
		WIZARD);
# endif
	}
	set_error_savefile();
	hu = FALSE;
	(void) dosave0();
#endif
	{
	    char buf[BUFSZ];
	    Vsprintf(buf,str,VA_ARGS);
	    raw_print(buf);
	}
#if defined(WIZARD) && (defined(UNIX) || defined(VMS) || defined(LATTICE))
	if (wizard)
# ifdef AMIGA
		Abort(0);
# else
#  ifdef SYSV
		(void)
#  endif
		    abort();	/* generate core dump */
# endif
#endif
	VA_END();
	done(PANICKED);
}

static void
disclose(how,taken)
int how;
boolean taken;
{
	char	c;
	char	qbuf[QBUFSZ];

	if (invent && !done_stopprint &&
		(!flags.end_disclose[0] || index(flags.end_disclose, 'i'))) {
	    if(taken)
/*JP		Sprintf(qbuf,"Do you want to see what you had when you %s?",
			(how == QUIT) ? "quit" : "died");*/
		Sprintf(qbuf,"%sとき何を持っていたか見ますか？",
			(how == QUIT) ? "やめた" : "死んだ");
	    else
/*JP		Strcpy(qbuf,"Do you want your possessions identified?");*/
		Strcpy(qbuf,"持ち物を識別しますか？"); 
	    if ((c = yn_function(qbuf, ynqchars, 'y')) == 'y') {
	    /* New dump format by maartenj@cs.vu.nl */
		struct obj *obj;

		for(obj = invent; obj && !done_stopprint; obj = obj->nobj) {
		    makeknown(obj->otyp);
		    obj->known = obj->bknown = obj->dknown = obj->rknown = 1;
		}
		(void) display_inventory(NULL, FALSE);
		container_contents(invent, TRUE, TRUE);
	    }
	    if (c == 'q')  done_stopprint++;
	    if (taken) {
		/* paybill has already given the inventory locations
		 * in the shop and put it on the main object list
		 */
		struct obj *obj;

		for(obj = invent; obj; obj = obj->nobj) {
		    obj->owornmask = 0;
		    if(rn2(5)) curse(obj);
		}
		invent = (struct obj *) 0;
	    }
	}

	if (!done_stopprint &&
		(!flags.end_disclose[0] || index(flags.end_disclose, 'a'))) {
/*JP	    c = yn_function("Do you want to see your attributes?",ynqchars,'y');*/
	    c = yn_function("属性を見ますか？",ynqchars,'y');
	    if (c == 'y') enlightenment(TRUE);	/* final */
	    if (c == 'q') done_stopprint++;
	}

	if (!done_stopprint &&
		(!flags.end_disclose[0] || index(flags.end_disclose, 'v'))) {
	    list_vanquished();
	}

	if (!done_stopprint &&
		(!flags.end_disclose[0] || index(flags.end_disclose, 'g'))) {
	    list_genocided();
	}
}

/* try to get the player back in a viable state after being killed */
static void
savelife(how)
int how;
{
	u.uswldtim = 0;
	u.uhp = u.uhpmax;
	if (u.uhunger < 500) {
	    u.uhunger = 500;
	    newuhs(FALSE);
	}
	if (how == CHOKING) init_uhunger();
/*JP	nomovemsg = "You survived that attempt on your life.";*/
	nomovemsg = "あなたは生きながらえた．";
	flags.move = 0;
	if(multi > 0) multi = 0; else multi = -1;
	if(u.utrap && u.utraptype == TT_LAVA) u.utrap = 0;
	flags.botl = 1;
	u.ugrave_arise = -1;
	curs_on_u();
}

/*
 *  Get valuables from the given list. NOTE: The list is destroyed as it is
 *  processed, so don't expect to use it again!
 */
static struct obj *
get_valuables(list)
    struct obj *list;
{
    struct obj *obj, *next_obj, *c_vals, *temp;
    struct obj *valuables = (struct obj *)0;

    for (obj = list; obj; obj = next_obj) {
	if (Has_contents(obj)) {
	    c_vals = get_valuables(obj->cobj);

	    if (c_vals) {
		/* find the end of the list */
		for (temp = c_vals; temp->nobj; temp = temp->nobj) ;

		temp->nobj = valuables;
		valuables = c_vals;
	    }
	}

	next_obj = obj->nobj;

	if ((obj->oclass == GEM_CLASS && obj->otyp < LUCKSTONE)
	    || obj->oclass == AMULET_CLASS) {
	    obj->nobj = valuables;
	    valuables = obj;
	}
    }
    return valuables;
}

/* Be careful not to call panic from here! */
void
done(how)
int how;
{
	struct permonst *upmon;
	boolean taken;
	char kilbuf[BUFSZ], pbuf[BUFSZ];
	winid endwin = WIN_ERR;
	boolean have_windows = flags.window_inited;
/*JP*/
	char amulet_tmp[BUFSZ];

	/* kilbuf: used to copy killer in case it comes from something like
	 *	xname(), which would otherwise get overwritten when we call
	 *	xname() when listing possessions
	 * pbuf: holds Sprintf'd output for raw_print and putstr
	 */
	if (how == ASCENDED)
		killer_format = NO_KILLER_PREFIX;
	/* Avoid killed by "a" burning or "a" starvation */
	if (!killer && (how == STARVING || how == BURNING))
		killer_format = KILLED_BY;
	Strcpy(kilbuf, (!killer || how >= PANICKED ? deaths[how] : killer));
	killer = kilbuf;
#ifdef WIZARD
	if (wizard && how == TRICKED) {
/*JP		You("are a very tricky wizard, it seems.");*/
		You("とてもトリッキーな魔法使いのようだ．");
		return;
	}
#endif
	if (how < PANICKED) u.umortality++;
	if (Lifesaved && how <= GENOCIDED) {
/*JP		pline("But wait...");*/
		pline("ちょっとまった．．．");
		makeknown(AMULET_OF_LIFE_SAVING);
/*JP		Your("medallion %s!",
		      !Blind ? "begins to glow" : "feels warm");*/
		Your("メダリオンは%s！",
		      !Blind ? "輝きはじめた" : "暖かくなりはじめた");
/*JP		if (how == CHOKING) You("vomit ...");*/
		if (how == CHOKING) You("吐血した．．．"); 
/*JP		You("feel much better!");*/
		You("気分がよくなった！");
/*JP		pline("The medallion crumbles to dust!");*/
		pline("メダリオンはこなごなにくだけた！");
		useup(uamul);

		(void) adjattrib(A_CON, -1, TRUE);
		if(u.uhpmax <= 0) u.uhpmax = 10;	/* arbitrary */
		savelife(how);
		if (how == GENOCIDED)
/*JP			pline("Unfortunately you are still genocided...");*/
			pline("不幸にもあなたは虐殺された．．．");
		else {
			killer = 0;
			return;
		}
	}
#if defined(WIZARD) || defined(EXPLORE_MODE)
	if ((wizard || discover) && how <= GENOCIDED) {
/*JP		if(yn("Die?") == 'y') goto die;*/
		if(yn("死んでみる？") == 'y') goto die;
/*JP		pline("OK, so you don't %s.",
			(how == CHOKING) ? "choke" : "die");*/
		You("%sなかった．",
			(how == CHOKING) ? "絞殺され" : "死な");
		if(u.uhpmax <= 0) u.uhpmax = u.ulevel * 8;	/* arbitrary */
		savelife(how);
		killer = 0;
		return;
	}
#endif /* WIZARD || EXPLORE_MODE */
	/* Sometimes you die on the first move.  Life's not fair.
	 * On those rare occasions you get hosed immediately, go out
	 * smiling... :-)  -3.
	 */
	if (moves <= 1 && how < PANICKED)
	    /* You die... --More-- */
/*JP	    pline("Do not pass go.  Do not collect 200 zorkmids.");*/
	    pline("注意一秒，怪我一生，死亡一歩．");

die:
	if (have_windows) wait_synch();	/* flush screen output */
#ifndef NO_SIGNAL
	(void) signal(SIGINT, (SIG_RET_TYPE) done_intr);
# if defined(UNIX) || defined(VMS)
	(void) signal(SIGQUIT, (SIG_RET_TYPE) done_intr);
	(void) signal(SIGHUP, (SIG_RET_TYPE) done_hangup);
# endif
#endif /* NO_SIGNAL */
#ifdef POLYSELF
	if (u.mtimedone)
	    upmon = uasmon;
	else
#endif
	upmon = player_mon();

	if (u.ugrave_arise < 0) { /* >= 0 means create no corpse */
	    if (how == STONING)
		u.ugrave_arise = -2;

/*
 * If you're burned to a crisp, why leave a corpse?
 */
	    else if (how != BURNING && how != PANICKED)
		(void) mk_named_object(CORPSE, upmon, u.ux, u.uy, plname,
							(int)strlen(plname));
	}

	if (how == QUIT) {
		killer_format = NO_KILLER_PREFIX;
		if (u.uhp < 1) {
			how = DIED;
/* note that killer is pointing at kilbuf */
/* JP			Strcpy(kilbuf, "quit while already on Charon's boat");*/
			Strcpy(kilbuf, "カロンの舟に乗っている間に抜けた");
		}
	}
	if (how == ESCAPED || how == PANICKED)
		killer_format = NO_KILLER_PREFIX;

	/* paybill() must be called unconditionally, or strange things will
	 * happen to bones levels */
	taken = paybill(how != QUIT);
	paygd();
	clearpriests();
	clearlocks();
#ifdef AMIGA
	clear_icon();
#endif
	if (have_windows) display_nhwindow(WIN_MESSAGE, FALSE);

	if (strcmp(flags.end_disclose, "none") && how != PANICKED)
		disclose(how, taken);

	if (how < GENOCIDED) {
#ifdef WIZARD
/*JP	    if (!wizard || yn("Save bones?") == 'y')*/
	    if (!wizard || yn("骨をうめる？") == 'y')
#endif
		savebones();
	}

	/* calculate score */
	{
	    long tmp;
	    int deepest = deepest_lev_reached(FALSE);

	    u.ugold += hidden_gold();	/* accumulate gold from containers */
	    tmp = u.ugold - u.ugold0;
	    if (tmp < 0L)
		tmp = 0L;
	    if (how < PANICKED)
		tmp -= tmp / 10L;
	    u.urexp += tmp;
	    u.urexp += 50L * (long)(deepest - 1);
	    if (deepest > 20)
		u.urexp += 1000L * (long)((deepest > 30) ? 10 : deepest - 20);
	    if (how == ASCENDED) u.urexp *= 2L;
	}

	/* clean up unneeded windows */
	if (have_windows) {
	    destroy_nhwindow(WIN_MAP);
	    destroy_nhwindow(WIN_STATUS);
	    destroy_nhwindow(WIN_MESSAGE);

	    if(!done_stopprint || flags.tombstone)
		endwin = create_nhwindow(NHW_TEXT);

	    if(how < GENOCIDED && flags.tombstone) outrip(endwin, how);
	} else
	    done_stopprint = 1; /* just avoid any more output */

/* changing kilbuf really changes killer. we do it this way because
   killer is declared a (const char *)
*/
/*JP	if (u.uhave.amulet) Strcat(kilbuf, " (with the Amulet)");*/
	if (u.uhave.amulet){
	  Strcpy(amulet_tmp,kilbuf);
	  Strcpy(kilbuf,"魔除けを手に");
	  Strcat(kilbuf,amulet_tmp);
	}
	if (!done_stopprint) {
/*JP	    Sprintf(pbuf, "%s %s the %s...",
		   (pl_character[0] == 'S') ? "Sayonara" :
		   (pl_character[0] == 'T') ? "Aloha" : "Goodbye", plname,
		   how != ASCENDED ? (const char *) pl_character :
		   (const char *) (flags.female ? "Demigoddess" : "Demigod"));*/
	    Sprintf(pbuf, "%s．%sの%s．．．．",
		   (pl_character[0] == 'S') ? "武士道とは死ぬこと" :
		   (pl_character[0] == 'T') ? "アローハ" : "さようなら", 
		   how != ASCENDED ? (const char *) jtrns_mon(pl_character) :
		   (const char *) (flags.female ? "女神" : "神"),
		    plname );
	    putstr(endwin, 0, pbuf);
	    putstr(endwin, 0, "");
	}
	if (how == ESCAPED || how == ASCENDED) {
		register struct monst *mtmp;
		register struct obj *otmp;
		struct obj *jewels;
		long i;
		register long worthlessct = 0;

		/*
		 *  Put items that count into the jewels chain.  Rewriting
		 *  the invent chain and all the container chains (within
		 *  invent) here is safe.  They will never be used again.
		 */
		jewels = get_valuables(invent);

		/* add points for jewels */
		for(otmp = jewels; otmp; otmp = otmp->nobj) {
			if(otmp->oclass == GEM_CLASS)
				u.urexp += otmp->quan *
					    objects[otmp->otyp].oc_cost;
			else	/* amulet */
				u.urexp += objects[otmp->otyp].oc_cost;
		}

		keepdogs();
		viz_array[0][0] |= IN_SIGHT; /* need visibility for naming */
		mtmp = mydogs;
/*JP		if(!done_stopprint) Strcpy(pbuf, "You");*/
		if(!done_stopprint) Strcpy(pbuf, "あなた");
		if(mtmp) {
			while(mtmp) {
				if(!done_stopprint) {
/*JP				    Strcat(pbuf, " and ");*/
				    Strcat(pbuf, "と");
				    Strcat(pbuf, mon_nam(mtmp));
				}
				Strcat(pbuf, "は");
				if(mtmp->mtame)
					u.urexp += mtmp->mhp;
				mtmp = mtmp->nmon;
			}
			if(!done_stopprint)
				putstr(endwin, 0, pbuf);
			pbuf[0] = 0;
		} else {
/*JP			if(!done_stopprint)
				Strcat(pbuf, " ");*/
		}
		if(!done_stopprint) {
			Sprintf(eos(pbuf),
/*JP				"%s with %ld point%s,",
				how==ASCENDED ? "went to your reward"
				: "escaped from the dungeon",*/
				"%ldポイントマークし%s．",
				u.urexp,
				how==ASCENDED ? "報酬を受けとった"
				: "迷宮から脱出した");
				putstr(endwin, 0, pbuf);
		}

		/* print jewels chain here */
		for(otmp = jewels; otmp; otmp = otmp->nobj) {
			makeknown(otmp->otyp);
			if(otmp->oclass == GEM_CLASS &&
			   otmp->otyp < LUCKSTONE) {
				i = otmp->quan *
					objects[otmp->otyp].oc_cost;
				if(i == 0) {
					worthlessct += otmp->quan;
					continue;
				}
			} else {		/* amulet */
				otmp->known = 1;
				i = objects[otmp->otyp].oc_cost;
			}
			if(!done_stopprint) {
/*JP			    Sprintf(pbuf, "        %s (worth %ld zorkmids),",*/
			    Sprintf(pbuf, "        %s (%ldゴールドの価値)．",
				    doname(otmp), i);
			    putstr(endwin, 0, pbuf);
			}
		}
		if(worthlessct && !done_stopprint) {
		    Sprintf(pbuf,
/*JP			  "        %ld worthless piece%s of colored glass,",*/
			  "        %ld個の価値のない色つきガラス．",
			  worthlessct);
		    putstr(endwin, 0, pbuf);
		}
	} else if (!done_stopprint) {
/*JP		Strcpy(pbuf, "You ");*/
		Strcpy(pbuf, "あなたは");
/*JP		Strcat(pbuf, ends[how]);*/
		if (how != ASCENDED) {
/*JP		    Strcat(pbuf, " in ");*/
		    if (Is_astralevel(&u.uz))
/*JP			Strcat(pbuf, "The Astral Plane");*/
			Strcat(pbuf, "精霊界");
		    else{
		      Strcat(pbuf, jtrns_obj('d',dungeons[u.uz.dnum].dname));
/*JP		    Strcat(pbuf, " ");*/
		    }
		    if (!In_endgame(&u.uz)
#ifdef MULDGN
					       && !Is_knox(&u.uz)
#endif
			)
/*JP			Sprintf(eos(pbuf), "on dungeon level %d ", (*/
			Sprintf(eos(pbuf), "の地下%d階で", (
#ifdef MULDGN
						 In_quest(&u.uz) ?
						    dunlev(&u.uz) :
#endif
						    depth(&u.uz)));
/*JP*/
		    else
		      Strcat(pbuf,"で");
		}
		Sprintf(eos(pbuf),
/*JP			"with %ld point%s,", u.urexp, plur(u.urexp));*/
			"%ldポイントマークし", u.urexp);
		putstr(endwin, 0, pbuf);
	}
	if (!done_stopprint) {
/*JP	    Sprintf(pbuf, "and %ld piece%s of gold, after %ld move%s.",*/
	    Sprintf(pbuf, "%ldゴールドを得，%ld歩動いた．",
		    u.ugold, moves);
	    putstr(endwin, 0, pbuf);
	}
	if (!done_stopprint) {
	    Sprintf(pbuf,
/*JP	     "You were level %u with a maximum of %d hit point%s when you %s.",*/
	     "%sとき，あなたはレベル%uで，最大体力は%dであった．",
		    ends[how],u.ulevel, u.uhpmax);
	    putstr(endwin, 0, pbuf);
	    putstr(endwin, 0, "");
	}
	if (!done_stopprint)
	    display_nhwindow(endwin, TRUE);
	if (have_windows)
	    exit_nhwindows(NULL);
	/* "So when I die, the first thing I will see in Heaven is a
	 * score list?" */
	topten(how);
	if(done_stopprint) { raw_print(""); raw_print(""); }
	terminate(0);
}


#ifdef NOSAVEONHANGUP
int
hangup()
{
	(void) signal(SIGINT, SIG_IGN);
	clearlocks();
# ifndef VMS
	terminate(1);
# endif
}
#endif


void
container_contents(list, identified, all_containers)
	struct obj *list;
	boolean identified, all_containers;
{
	register struct obj *box, *obj;
	char buf[BUFSZ];

	for (box = list; box; box = box->nobj) {
	    if (Is_container(box) && box->otyp != BAG_OF_TRICKS) {
		if (box->cobj) {
		    winid tmpwin = create_nhwindow(NHW_MENU);
/*JP		    Sprintf(buf, "Contents of %s:", the(xname(box)));*/
		    Sprintf(buf, "%sの中身：", the(xname(box)));
		    putstr(tmpwin, 0, buf); putstr(tmpwin, 0, "");
		    for (obj = box->cobj; obj; obj = obj->nobj) {
			if (identified) {
			    makeknown(obj->otyp);
			    obj->known = obj->bknown =
			    obj->dknown = obj->rknown = 1;
			}
			putstr(tmpwin, 0, doname(obj));
		    }
		    display_nhwindow(tmpwin, TRUE);
		    destroy_nhwindow(tmpwin);
		    if (all_containers)
			container_contents(box->cobj, identified, TRUE);
		} else {
/*JP		    pline("%s is empty.", The(xname(box)));*/
		    pline("%sは空っぽだ．", The(xname(box)));
		    display_nhwindow(WIN_MESSAGE, FALSE);
		}
	    }
	    if (!all_containers)
		break;
	}
}


void
terminate(status)
int status;
{
#ifdef MAC
	if (!hu) {
		getreturn("to exit");
	}
#endif
/*JP*/
	jputchar('\0');	/* reset terminal */
	exit(status);
}

static void
list_vanquished()
{
    register int i, lev;
    int ntypes = 0, max_lev = 0, nkilled;
    long total_killed = 0L;
    char c;
    static winid klwin;
    char buf[BUFSZ];

    /* get totals first */
    for (i = 0; i < NUMMONS; i++) {
	if (u.nr_killed[i]) ntypes++;
	total_killed += (long)u.nr_killed[i];
	if (mons[i].mlevel > max_lev) max_lev = mons[i].mlevel;
    }

    /* vanquished foes list;
     * includes all dead monsters, not just those killed by the player
     */
    if (ntypes != 0) {
/*JP	c = yn_function("Do you want an account of foes vanquished?",*/
	c = yn_function("倒した敵の一覧を見ますか？",
			ynqchars, 'n');
	if (c == 'q') done_stopprint++;
	if (c == 'y') {
	    klwin = create_nhwindow(NHW_MENU);
/*JP	    putstr(klwin, 0, "Vanquished foes:");*/
	    putstr(klwin, 0, "倒した敵：");
	    putstr(klwin, 0, "");

	    /* countdown by monster "toughness" */
	    for (lev = max_lev; lev >= 0; lev--)
	      for (i = 0; i < NUMMONS; i++)
		if (mons[i].mlevel == lev && (nkilled = u.nr_killed[i])) {
		    if (i == PM_WIZARD_OF_YENDOR || mons[i].geno & G_UNIQ) {
			Sprintf(buf, type_is_pname(&mons[i]) ? jtrns_mon(mons[i].mname) :
				The(jtrns_mon(mons[i].mname)));
			if (nkilled > 1)
/*JP			    Sprintf(eos(buf)," (%d time%s)",*/
			    Sprintf(eos(buf)," (%d回)",
				    nkilled);
		    } else {
			/* trolls or undead might have come back,
			   but we don't keep track of that */
			if (nkilled == 1)
			    Strcpy(buf, an(jtrns_mon(mons[i].mname)));
			else
/*JP			    Sprintf(buf, "%d %s",*/
			    Sprintf(buf, "%d匹の%s",
				    nkilled, makeplural(jtrns_mon(mons[i].mname)));
		    }
		    putstr(klwin, 0, buf);
		}
	    /*
	     * if (Hallucination)
	     *     putstr(klwin, 0, "and a partridge in a pear tree");
	     */
	    if (ntypes > 1) {
		putstr(klwin, 0, "");
/*JP		Sprintf(buf, "%ld creatures vanquished.", total_killed);*/
		Sprintf(buf, "%ld匹の生物を倒した．", total_killed);
		putstr(klwin, 0, buf);
	    }
	    display_nhwindow(klwin, TRUE);
	    destroy_nhwindow(klwin);
	}
    }
}

static void
list_genocided()
{
    register int i;
    int ngenocided = 0;
    char c;
    static winid klwin;
    char buf[BUFSZ];

    /* get totals first */
    for (i = 0; i < NUMMONS; i++) {
	if (mons[i].geno & G_GENOD) ngenocided++;
    }

    /* genocided species list */
    if (ngenocided != 0) {
/*JP	c = yn_function("Do you want a list of species genocided?",*/
	c = yn_function("虐殺した種の一覧を見ますか？",
			ynqchars, 'n');
	if (c == 'q') done_stopprint++;
	if (c == 'y') {
	    klwin = create_nhwindow(NHW_MENU);
/*JP	    putstr(klwin, 0, "Genocided species:");*/
	    putstr(klwin, 0, "虐殺した種:");
	    putstr(klwin, 0, "");

	    for (i = 0; i < NUMMONS; i++)
		if (mons[i].geno & G_GENOD)
		    putstr(klwin, 0, makeplural(jtrns_mon(mons[i].mname)));

	    putstr(klwin, 0, "");
/*JP	    Sprintf(buf, "%d species genocided.", ngenocided);*/
	    Sprintf(buf, "%d の種を虐殺した．", ngenocided);
	    putstr(klwin, 0, buf);

	    display_nhwindow(klwin, TRUE);
	    destroy_nhwindow(klwin);
	}   
    }
}

/*end.c*/
