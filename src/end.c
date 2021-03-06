/*	SCCS Id: @(#)end.c	3.2	96/08/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#define NEED_VARARGS	/* comment line for pre-compiled headers */

#include "hack.h"
#include "eshk.h"
#ifndef NO_SIGNAL
#include <signal.h>
#endif
#include "dlb.h"

STATIC_PTR void FDECL(done_intr, (int));
static void FDECL(disclose,(int,BOOLEAN_P));
static void FDECL(get_valuables, (struct obj *));
static void FDECL(sort_valuables, (struct obj **,int));
static void FDECL(savelife, (int));
static void NDECL(list_vanquished);
static void NDECL(list_genocided);
#if defined(UNIX) || defined(VMS)
static void FDECL(done_hangup, (int));
#endif

#if defined(__BEOS__) || defined(MICRO) || defined(WIN32) || defined(OS2) 
extern void FDECL(nethack_exit,(int));
#else
#define nethack_exit exit
#endif

#define done_stopprint program_state.stopprint

#ifdef AMIGA
void NDECL(clear_icon);
# define NH_abort()	Abort(0)
#else
# ifdef SYSV
# define NH_abort()	(void) abort()
# else
# define NH_abort()	abort()
# endif
#endif

/*
 * The order of these needs to match the macros in hack.h.
 */

/*JP
** 日本語では「死んだ」と「殺された」はニュアンスが違うよって
** ステータスを1つ追加する．
*/

/*JP
#define KILLED		 0 <- (変更)
#define CHOKING		 1
#define POISONING	 2
#define STARVING	 3
#define DROWNING	 4
#define BURNING		 5
#define DISSOLVED	 6
#define CRUSHING	 7
#define STONING		 8
#define DIED		 9 <- (追加)
#define GENOCIDED	10
#define PANICKED	11
#define TRICKED		12
#define QUIT		13
#define ESCAPED		14
#define ASCENDED	15
*/

#ifdef NH_EXTENSION_REPORT
int	report_flag = 0;
#endif

#if 0 /* JP */
static NEARDATA const char *deaths[] = {		/* the array of death */
	"died", "choked", "poisoned", "starvation", "drowning",
	"burning", "dissolving under the heat and pressure",
/*JP	"crushed", "turned to stone", "genocided",*/
	"crushed", "turned to stone", "died", "genocided",
	"panic", "trickery",
	"quit", "escaped", "ascended"
};
#endif

#if 0 /*JP*/
static NEARDATA const char *ends[] = {		/* "when you..." */
	"died", "choked", "were poisoned", "starved", "drowned",
	"burned", "dissolved in the lava",
	"were crushed", "turned to stone", "were genocided",
	"panicked", "were tricked",
	"quit", "escaped", "ascended"
};
#endif /*JP*/
static NEARDATA const char *ends[] = {		/* "when you..." */
	"殺された", "窒息した", "毒におかされた", "餓死した", "溺死した",
	"焼死した", "溶岩に溶けた",
	"押し潰された", "石になった", "死んだ", "虐殺された",
	"パニックにおちいった", "奇妙な出来事に会った",
	"抜けた", "脱出した", "昇天した"
};

	/* these probably ought to be generated by makedefs, like LAST_GEM */
#define FIRST_GEM    DILITHIUM_CRYSTAL
#define FIRST_AMULET AMULET_OF_ESP
#define LAST_AMULET  AMULET_OF_YENDOR

static struct obj *gems[LAST_GEM+1 - FIRST_GEM + 1],	/* 1 extra for glass */
		  *amulets[LAST_AMULET+1 - FIRST_AMULET];

static struct val_list { struct obj **list; int size; } valuables[] = {
	{ gems,    sizeof gems / sizeof *gems },
	{ amulets, sizeof amulets / sizeof *amulets },
	{ 0, 0 }
};

/*ARGSUSED*/
void
done1(sig_unused)   /* called as signal() handler, so sent at least one arg */
int sig_unused;
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
	} else {
		(void)done2();
	}
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
		exit_nhwindows((char *)0);
		NH_abort();
	    } else if (c == 'q') done_stopprint++;
	}
#endif
#ifndef LINT
	done(QUIT);
#endif
	return 0;
}

/*ARGSUSED*/
STATIC_PTR void
done_intr(sig_unused) /* called as signal() handler, so sent at least one arg */
int sig_unused;
{
	done_stopprint++;
#ifndef NO_SIGNAL
	(void) signal(SIGINT, SIG_IGN);
# if defined(UNIX) || defined(VMS)
	(void) signal(SIGQUIT, SIG_IGN);
# endif
#endif /* NO_SIGNAL */
	return;
}

#if defined(UNIX) || defined(VMS)
static void
done_hangup(sig)	/* signal() handler */
int sig;
{
	program_state.done_hup++;
	(void)signal(SIGHUP, SIG_IGN);
	done_intr(sig);
	return;
}
#endif

void
done_in_by(mtmp)
register struct monst *mtmp;
{
	char buf[BUFSZ];
	boolean distorted = (boolean)(Hallucination && canspotmon(mtmp));

/*JP
	You("die...");
*/
	pline("あなたは死にました．．．");
	mark_synch();	/* flush buffered screen output */
	buf[0] = '\0';
	if ((mtmp->data->geno & G_UNIQ) != 0) {
/*JP
	    if (!type_is_pname(mtmp->data))
		Strcat(buf, "the ");
*/
	    killer_format = KILLED_BY;
	}
	if (mtmp->minvis && !(mtmp->ispriest || mtmp->isminion))
/*JP
		Strcat(buf, "invisible ");
*/
		Strcat(buf, "透明な");
	if (distorted)
/*JP
		Strcat(buf, "hallucinogen-distorted ");
*/
		Strcat(buf, "幻覚で歪んだ");

	if (mtmp->mnamelth) Sprintf(eos(buf), "%sと呼ばれる", NAME(mtmp));

	if(mtmp->data == &mons[PM_GHOST]) {
		register char *gn = (char *) mtmp->mextra;
		if (!distorted && !mtmp->minvis && *gn) {
/*JP
			Strcat(buf, "the ");
*/
			killer_format = KILLED_BY;
		}
/*JP
		Sprintf(eos(buf), (*gn ? "ghost of %s" : "ghost%s"), gn);
*/
		Sprintf(eos(buf), (*gn ? "%sの幽霊" : "ghost%s"), gn);
	} else if(mtmp->isshk) {
/*JP
		Sprintf(eos(buf), "%s %s, the shopkeeper",
			(mtmp->female ? "Ms." : "Mr."), shkname(mtmp));
*/
		Sprintf(eos(buf), "%sという名の店主",
			shkname(mtmp));
		killer_format = KILLED_BY;
	} else if (mtmp->ispriest || mtmp->isminion) {
		char priestnambuf[BUFSZ];

		killer = priestname(mtmp, priestnambuf);
/*JP
		if (!strncmp(killer, "the ", 4)) Strcat(buf, killer+4);
		else Strcat(buf, killer);
*/
		Strcat(buf, killer);
/*JP
	} else Strcat(buf, mtmp->data->mname);
*/
	} else Strcat(buf, jtrns_mon(mtmp->data->mname, mtmp->female));

/*JP
	if (mtmp->mnamelth) Sprintf(eos(buf), " called %s", NAME(mtmp));
*/
	killer = buf;
	if (mtmp->data->mlet == S_WRAITH)
		u.ugrave_arise = PM_WRAITH;
	else if (mtmp->data->mlet == S_MUMMY)
		u.ugrave_arise = Role_is('E') ? PM_ELF_MUMMY : PM_HUMAN_MUMMY;
	else if (mtmp->data->mlet == S_VAMPIRE && !Role_is('E'))
		u.ugrave_arise = PM_VAMPIRE;
	if (u.ugrave_arise >= LOW_PM &&
				(mvitals[u.ugrave_arise].mvflags & G_GENOD))
		u.ugrave_arise = NON_PM;
	if (mtmp->data->mlet == S_COCKATRICE)
		done(STONING);
	else
/*JP		done(DIED);*/
		done(KILLED);
	return;
}

/*VARARGS1*/
void
panic VA_DECL(const char *, str)
	VA_START(str);
	VA_INIT(str, char *);

	if (program_state.panicking++)
	    NH_abort();	/* avoid loops - this should never happen*/

	if (iflags.window_inited) {
	    raw_print("\r\nOops...");
	    wait_synch();	/* make sure all pending output gets flushed */
	    exit_nhwindows((char *)0);
	    iflags.window_inited = 0; /* they're gone; force raw_print()ing */
	}

	raw_print(!program_state.something_worth_saving ?
#if 0 /*JP*/
		  "Program initialization has failed." :
		  "Suddenly, the dungeon collapses.");
#endif /*JP*/
		  "プログラムの初期化に失敗した．" :
		  "突然迷宮が崩れた．");
#if defined(WIZARD) && !defined(MICRO)
	if (!wizard)
/*JP	    raw_printf("Report error to \"%s\"%s.",*/
	    raw_printf("%sに報告．%s",
# ifdef WIZARD_NAME	/*(KR1ED)*/
			WIZARD_NAME,
# else
			WIZARD,
# endif
			!program_state.something_worth_saving ? "" :
/*JP			" and it may be possible to rebuild.");*/
			"エラー発生．再実行不可能．");
	if (program_state.something_worth_saving) {
	    set_error_savefile();
	    (void) dosave0();
	}
#endif
	{
	    char buf[BUFSZ];
	    Vsprintf(buf,str,VA_ARGS);
	    raw_print(buf);
	}
#if defined(WIZARD) && (defined(UNIX) || defined(VMS) || defined(LATTICE))
	if (wizard)
	    NH_abort();	/* generate core dump */
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

		for (obj = invent; obj; obj = obj->nobj) {
		    makeknown(obj->otyp);
		    obj->known = obj->bknown = obj->dknown = obj->rknown = 1;
		}
		(void) display_inventory((char *)0, TRUE);
		container_contents(invent, TRUE, TRUE);
	    }
	    if (c == 'q')  done_stopprint++;
	}

	if (!done_stopprint &&
		(!flags.end_disclose[0] || index(flags.end_disclose, 'a'))) {
/*JP	    c = yn_function("Do you want to see your attributes?",ynqchars,'y');*/
	    c = yn_function("属性を見ますか？",ynqchars,'y');
	    if (c == 'y') enlightenment(how >= PANICKED ? 1 : 2); /* final */
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
#ifdef	NH_EXTENSION_REPORT
	if (0 || (flags.reportscore && !wizard && !discover)){
	  if (!done_stopprint &&
		(!flags.end_disclose[0] || index(flags.end_disclose, 'a'))) {
	    c = yn_function("今回のプレイ結果をスコアサーバ(http://www.jnethack.org/)に報告しますか？",ynqchars,'y');
	    if(c == 'y')
	      report_flag = 1;
	    if (c == 'q') done_stopprint++;
	  }
	}
#endif

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
	u.ugrave_arise = NON_PM;
	curs_on_u();
}

/*
 *  Get valuables from the given list. NOTE: The list is destroyed as it is
 *  processed, so don't expect to use it again!  [Revised code: the list
 *  remains intact, but object quanties for some elements are altered.]
 */
static void
get_valuables(list)
struct obj *list;	/* inventory or container contents */
{
    register struct obj *obj;
    register int i;

    /* find amulets and gems; artifact amulets are treated as ordinary ones */
    for (obj = list; obj; obj = obj->nobj)
	if (Has_contents(obj)) {
	    get_valuables(obj->cobj);
	} else if (obj->oclass == AMULET_CLASS) {
	    i = obj->otyp - FIRST_AMULET;
	    if (!amulets[i]) amulets[i] = obj;
	    else amulets[i]->quan += obj->quan;	/*(always adds one)*/
	} else if (obj->oclass == GEM_CLASS && obj->otyp < LUCKSTONE) {
	    i = min(obj->otyp, LAST_GEM + 1) - FIRST_GEM;
	    if (!gems[i]) gems[i] = obj;
	    else gems[i]->quan += obj->quan;
	}
    return;
}

/*
 *  Sort collected valuables, most frequent to least.  We could just
 *  as easily use qsort, but we don't care about efficiency here.
 */
static void
sort_valuables(list, size)
struct obj *list[];
int size;		/* max value is less than 20 */
{
    register int i, j;
    register struct obj *obj;

    /* move greater quantities to the front of the list */
    for (i = 1; i < size; i++) {
	if ((obj = list[i]) == 0) continue;	/* empty slot */
	for (j = i; j > 0; --j)
	    if (list[j-1] && list[j-1]->quan >= obj->quan) break;
	    else list[j] = list[j-1];
	list[j] = obj;
    }
    return;
}

/* Be careful not to call panic from here! */
void
done(how)
int how;
{
	boolean taken;
	char kilbuf[BUFSZ], pbuf[BUFSZ];
	winid endwin = WIN_ERR;
	boolean bones_ok, have_windows = iflags.window_inited;
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
/*JP	Strcpy(kilbuf, (!killer || how >= PANICKED ? deaths[how] : killer));*/
	Strcpy(kilbuf, (!killer || how >= PANICKED ? ends[how] : killer));
	killer = kilbuf;
#ifdef WIZARD
	if (wizard && how == TRICKED) {
/*JP		You("are a very tricky wizard, it seems.");*/
		You("とても扱いにくい魔法使いのようだ．");
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
		Your("魔除けは%s！",
		      !Blind ? "輝きはじめた" : "暖かくなりはじめた");
/*JP		if (how == CHOKING) You("vomit ...");*/
		if (how == CHOKING) You("胃の中のものを吐いた．．．");
/*JP		You_feel("much better!");*/
/*JP		pline_The("medallion crumbles to dust!");*/
		You("気分がよくなった！");
		pline("魔除けはこなごなにくだけた！");

		useup(uamul);

		(void) adjattrib(A_CON, -1, TRUE);
		if(u.uhpmax <= 0) u.uhpmax = 10;	/* arbitrary */
		savelife(how);
		if (how == GENOCIDED)
/*JP			pline("Unfortunately you are still genocided...");*/
			pline("なんてことだ．あなたは虐殺されてしまった．．．");
		else {
			killer = 0;
			killer_format = 0;
			return;
		}
	}
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
		killer_format = 0;
		return;
	}

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

	bones_ok = (how < GENOCIDED) && can_make_bones();

	if (bones_ok && u.ugrave_arise < LOW_PM) {
	    if (how == BURNING)		/* corpse gets burnt up too */
		u.ugrave_arise = (NON_PM - 2);	/* leave no corpse */
	    else if (how == STONING)
		u.ugrave_arise = (NON_PM - 1);	/* statue instead of corpse */
	    else if (u.ugrave_arise == NON_PM)
		(void) mk_named_object(CORPSE, Upolyd ? uasmon : player_mon(),
				       u.ux, u.uy, plname);
	}

	if (how == QUIT) {
		killer_format = NO_KILLER_PREFIX;
		if (u.uhp < 1) {
			how = DIED;
			u.umortality++;	/* skipped above when how==QUIT */
			/* note that killer is pointing at kilbuf */
/*JP
			Strcpy(kilbuf, "quit while already on Charon's boat");
*/
			Strcpy(kilbuf, "カロンの舟に乗っている間に抜けた");
		}
	}
	if (how == ESCAPED || how == PANICKED)
		killer_format = NO_KILLER_PREFIX;

	if (how != PANICKED) {
	    /* these affect score and/or bones, but avoid them during panic */
	    taken = paybill(how != QUIT);
	    paygd();
	    clearpriests();
	} else	taken = FALSE;	/* lint; assert( !bones_ok ); */

	clearlocks();
#ifdef AMIGA
	clear_icon();
#endif
	if (have_windows) display_nhwindow(WIN_MESSAGE, FALSE);

	if (strcmp(flags.end_disclose, "none") && how != PANICKED)
		disclose(how, taken);
	/* finish_paybill should be called after disclosure but before bones */
	if (bones_ok && taken) finish_paybill();

	/* calculate score, before creating bones [container gold] */
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

	if (bones_ok) {
#ifdef WIZARD
/*JP	    if (!wizard || yn("Save bones?") == 'y')*/
	    if (!wizard || yn("骨をうめる？") == 'y')
#endif
		savebones();
	}

	/* clean up unneeded windows */
	if (have_windows) {
	    wait_synch();
	    display_nhwindow(WIN_MESSAGE, TRUE);
	    destroy_nhwindow(WIN_MAP);
	    destroy_nhwindow(WIN_STATUS);
	    destroy_nhwindow(WIN_MESSAGE);
	    WIN_MESSAGE = WIN_STATUS = WIN_MAP = WIN_ERR;

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
	else if (how == ESCAPED) {
	    if (Is_astralevel(&u.uz))	/* offered Amulet to wrong deity */
/*JP		Strcat(kilbuf, " (in celestial disgrace)");*/
		Strcpy(kilbuf, "天上で恥辱を受け脱出した");
	    else if (carrying(FAKE_AMULET_OF_YENDOR))
/*JP		Strcat(kilbuf, " (with a fake Amulet)");*/
		Strcpy(kilbuf, "偽物の魔除けを掴まされ脱出した");
		/* don't bother counting to see whether it should be plural */
	}

	if (!done_stopprint) {
#if 0 /*JP*/
	    Sprintf(pbuf, "%s %s the %s...",
		   Role_is('S') ? "Sayonara" :
#ifdef TOURIST
		   Role_is('T') ? "Aloha" :
#endif
			"Goodbye", plname,
		   how != ASCENDED ? (const char *) pl_character :
		   (const char *) (flags.female ? "Demigoddess" : "Demigod"));
#endif /*JP*/
	    Sprintf(pbuf, "%s．%sの%s．．．．",
		   Role_is('S') ? "武士道とは死ぬこと" :
#ifdef TOURIST
		   Role_is('T') ? "アローハ" :
#endif
			"さようなら",
		   how != ASCENDED ? (const char *) jtrns_mon(pl_character, flags.female) :
		   (const char *) (flags.female ? "女神" : "神"),
		    plname );
	    putstr(endwin, 0, pbuf);
	    putstr(endwin, 0, "");
	}

	if (how == ESCAPED || how == ASCENDED) {
	    register struct monst *mtmp;
	    register struct obj *otmp;
	    register struct val_list *val;
	    register int i;

	    /*
	     * Collecting valuables renders `invent' invalid, but from
	     * this point on, it won't be used again.
	     */
	    for (val = valuables; val->list; val++)
		for (i = 0; i < val->size; i++) val->list[i] = (struct obj *)0;
	    get_valuables(invent);

	    /* add points for collected valuables */
	    for (val = valuables; val->list; val++)
		for (i = 0; i < val->size; i++)
		    if ((otmp = val->list[i]) != 0)
			u.urexp += otmp->quan
				  * (long)objects[otmp->otyp].oc_cost;

	    keepdogs(TRUE);
	    viz_array[0][0] |= IN_SIGHT; /* need visibility for naming */
	    mtmp = mydogs;
/*JP	    if (!done_stopprint) Strcpy(pbuf, "You");*/
	    if (!done_stopprint) Strcpy(pbuf, "あなた");
	    if (mtmp) {
		while (mtmp) {
		    if (!done_stopprint)
/*JP			Sprintf(eos(pbuf), " and %s", mon_nam(mtmp));*/
		      {
			Strcat(pbuf, "と");
			Strcat(pbuf, mon_nam(mtmp));
		      }
/*
		    Strcat(pbuf, "は");
*/
		    if (mtmp->mtame)
			u.urexp += mtmp->mhp;
		    mtmp = mtmp->nmon;
		}
/*JP*/
		if (!done_stopprint) Strcat(pbuf, "は");
		if (!done_stopprint) putstr(endwin, 0, pbuf);
		pbuf[0] = '\0';
	    } else {
/*JP		if (!done_stopprint) Strcat(pbuf, " ");*/
		if (!done_stopprint) Strcat(pbuf, "は");
	    }
	    if (!done_stopprint) {
#if 0 /*JP*/
		Sprintf(eos(pbuf), "%s with %ld point%s,",
			how==ASCENDED ? "went to your reward" :
					"escaped from the dungeon",
			u.urexp, plur(u.urexp));
#endif /*JP*/
		Sprintf(eos(pbuf), "%ldポイントマークし%s．",
			u.urexp,
			how==ASCENDED ? "報酬を受けとった" : "迷宮から脱出した");
		putstr(endwin, 0, pbuf);
	    }

	    /* list valuables here */
	    for (val = valuables; val->list; val++) {
		sort_valuables(val->list, val->size);
		for (i = 0; i < val->size && !done_stopprint; i++) {
		    if ((otmp = val->list[i]) == 0) continue;
		    if (otmp->oclass != GEM_CLASS || otmp->otyp <= LAST_GEM) {
			makeknown(otmp->otyp);
			otmp->known = 1;	/* for fake amulets */
			otmp->onamelth = 0;
/*JP			Sprintf(pbuf, "%8ld %s (worth %ld zorkmids),",*/
			Sprintf(pbuf, "%8ld個の%s(%ldゴールドの価値)．",
				otmp->quan, xname(otmp),
				otmp->quan * (long)objects[otmp->otyp].oc_cost);
		    } else {
			Sprintf(pbuf,
/*JP				"%8ld worthless piece%s of colored glass,",*/
/*JP				otmp->quan, plur(otmp->quan));*/
				"%ld個の価値のない色つきガラス．",
				otmp->quan);
		    }
		    putstr(endwin, 0, pbuf);
		}
	    }

	} else if (!done_stopprint) {
	    /* did not escape or ascend */
/*JP	    const char *where = dungeons[u.uz.dnum].dname;*/
	    const char *where = jtrns_obj('d', dungeons[u.uz.dnum].dname);
/*JP	    if (Is_astralevel(&u.uz)) where = "The Astral Plane";*/
	    if (In_endgame(&u.uz)) where = "精霊界にて";
/*JP	    Sprintf(pbuf, "You %s in %s", ends[how], where);*/
	    Sprintf(pbuf, "あなたは%s", where);
	    if (!In_endgame(&u.uz) && !Is_knox(&u.uz))
/*JP		Sprintf(eos(pbuf), " on dungeon level %d",*/
		Sprintf(eos(pbuf), "の地下%d階で",
			In_quest(&u.uz) ? dunlev(&u.uz) : depth(&u.uz));
/*JP	    Sprintf(eos(pbuf), " with %ld point%s,",
		    u.urexp, plur(u.urexp));*/
	    Sprintf(eos(pbuf), " %ldポイントマークし",
		    u.urexp);
	    putstr(endwin, 0, pbuf);
	}

	if (!done_stopprint) {
/*JP	    Sprintf(pbuf, "and %ld piece%s of gold, after %ld move%s.",
		    u.ugold, plur(u.ugold), moves, plur(moves));*/
	    Sprintf(pbuf, "%ldゴールドを得，%ld歩動いた．",
		    u.ugold, moves);

	    putstr(endwin, 0, pbuf);
	}
	if (!done_stopprint) {
	    Sprintf(pbuf,
/*JP	     "You were level %d with a maximum of %d hit point%s when you %s.",
		    u.ulevel, u.uhpmax, plur(u.uhpmax), ends[how]);*/
	     "%sとき，あなたはレベル%uで，最大体力は%dであった．",
		    ends[how],u.ulevel, u.uhpmax);

	    putstr(endwin, 0, pbuf);
	    putstr(endwin, 0, "");
	}

	if (!done_stopprint)
	    display_nhwindow(endwin, TRUE);
	if (endwin != WIN_ERR)
	    destroy_nhwindow(endwin);

	/* "So when I die, the first thing I will see in Heaven is a
	 * score list?" */
	if (flags.toptenwin) {
	    topten(how);
	    if (have_windows)
		exit_nhwindows((char *)0);
	} else {
	    if (have_windows)
		exit_nhwindows((char *)0);
	    topten(how);
	}

	if(done_stopprint) { raw_print(""); raw_print(""); }
	terminate(EXIT_SUCCESS);
}


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
		    putstr(tmpwin, 0, buf);
		    putstr(tmpwin, 0, "");
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


/* should be called with either EXIT_SUCCESS or EXIT_FAILURE */
void
terminate(status)
int status;
{

#ifdef MAC
	getreturn("to exit");
#endif
	/* don't bother to try to release memory if we're in panic mode, to
	   avoid trouble in case that happens to be due to memory problems */
	if (!program_state.panicking) {
	    freedynamicdata();
	    dlb_cleanup();
	}
	jputchar('\0');	/* reset terminal */
	if (iflags.DECgraphics){
	  putchar(033);
	  putchar('$');
	  putchar(')');
	  putchar('B');
	}
	nethack_exit(status);
}

static void
list_vanquished()
{
    register int i, lev;
    int ntypes = 0, max_lev = 0, nkilled;
    long total_killed = 0L;
    char c;
    winid klwin;
    char buf[BUFSZ];

    /* get totals first */
    for (i = LOW_PM; i < NUMMONS; i++) {
	if (mvitals[i].died) ntypes++;
	total_killed += (long)mvitals[i].died;
	if (mons[i].mlevel > max_lev) max_lev = mons[i].mlevel;
    }

    /* vanquished creatures list;
     * includes all dead monsters, not just those killed by the player
     */
    if (ntypes != 0) {
/*JP	c = yn_function("Do you want an account of creatures vanquished?",*/
	c = yn_function("倒した敵の一覧を見ますか？",
			ynqchars, 'n');
	if (c == 'q') done_stopprint++;
	if (c == 'y') {
	    klwin = create_nhwindow(NHW_MENU);
/*JP	    putstr(klwin, 0, "Vanquished creatures:");*/
	    putstr(klwin, 0, "倒した敵：");
	    putstr(klwin, 0, "");

	    /* countdown by monster "toughness" */
	    for (lev = max_lev; lev >= 0; lev--)
	      for (i = LOW_PM; i < NUMMONS; i++)
		if (mons[i].mlevel == lev && (nkilled = mvitals[i].died) > 0) {
		    if ((mons[i].geno & G_UNIQ) && i != PM_HIGH_PRIEST) {
/*JP			Sprintf(buf, "%s%s",
				!type_is_pname(&mons[i]) ? "The " : "",
				mons[i].mname);*/
			Sprintf(buf, "%s",
				jtrns_mon(mons[i].mname, -1));
			if (nkilled > 1)
/*JP			    Sprintf(eos(buf)," (%d time%s)",
				    nkilled, plur(nkilled));*/
			    Sprintf(eos(buf)," (%d 回)",
				    nkilled);
		    } else {
			/* trolls or undead might have come back,
			   but we don't keep track of that */
			if (nkilled == 1)
/*JP			    Strcpy(buf, an(mons[i].mname));*/
			    Strcpy(buf, jtrns_mon(mons[i].mname, -1));
			else
/*JP			    Sprintf(buf, "%d %s",
				    nkilled, makeplural(mons[i].mname));*/
			    Sprintf(buf, "%d匹の%s",
				    nkilled, jtrns_mon(mons[i].mname, -1));
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
    winid klwin;
    char buf[BUFSZ];

    /* get totals first */
    for (i = LOW_PM; i < NUMMONS; i++) {
	if (mvitals[i].mvflags & G_GENOD) ngenocided++;
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

	    for (i = LOW_PM; i < NUMMONS; i++)
		if (mvitals[i].mvflags & G_GENOD) {
		    if ((mons[i].geno & G_UNIQ) && i != PM_HIGH_PRIEST)
#if 0 /*JP*/
			Sprintf(buf, "%s%s",
				!type_is_pname(&mons[i]) ? "" : "the ",
				mons[i].mname);
#endif /*JP*/
			Sprintf(buf, "%s",
				jtrns_mon(mons[i].mname, -1));
		    else
/*JP			Strcpy(buf, makeplural(mons[i].mname));*/
			Strcpy(buf, jtrns_mon(mons[i].mname, -1));
		    putstr(klwin, 0, buf);
		}

	    putstr(klwin, 0, "");
/*JP	    Sprintf(buf, "%d species genocided.", ngenocided);*/
	    Sprintf(buf, "%d種類の種を虐殺した．", ngenocided);
	    putstr(klwin, 0, buf);

	    display_nhwindow(klwin, TRUE);
	    destroy_nhwindow(klwin);
	}
    }
}

/*end.c*/
