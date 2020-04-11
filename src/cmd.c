/*	SCCS Id: @(#)cmd.c	3.3	1999/10/31	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "func_tab.h"
/* #define DEBUG */	/* uncomment for debugging */

/*
 * Some systems may have getchar() return EOF for various reasons, and
 * we should not quit before seeing at least NR_OF_EOFS consecutive EOFs.
 */
#if defined(SYSV) || defined(DGUX) || defined(HPUX)
#define NR_OF_EOFS	20
#endif

#ifdef DEBUG
/*
 * only one "wiz_debug_cmd" routine should be available (in whatever
 * module you are trying to debug) or things are going to get rather
 * hard to link :-)
 */
extern void NDECL(wiz_debug_cmd);
#endif

#ifdef DUMB	/* stuff commented out in extern.h, but needed here */
extern int NDECL(doapply); /**/
extern int NDECL(dorub); /**/
extern int NDECL(dojump); /**/
extern int NDECL(doextlist); /**/
extern int NDECL(dodrop); /**/
extern int NDECL(doddrop); /**/
extern int NDECL(dodown); /**/
extern int NDECL(doup); /**/
extern int NDECL(donull); /**/
extern int NDECL(dowipe); /**/
extern int NDECL(do_mname); /**/
extern int NDECL(ddocall); /**/
extern int NDECL(dotakeoff); /**/
extern int NDECL(doremring); /**/
extern int NDECL(dowear); /**/
extern int NDECL(doputon); /**/
extern int NDECL(doddoremarm); /**/
extern int NDECL(dokick); /**/
extern int NDECL(dofire); /**/
extern int NDECL(dothrow); /**/
extern int NDECL(doeat); /**/
extern int NDECL(done2); /**/
extern int NDECL(doengrave); /**/
extern int NDECL(dopickup); /**/
extern int NDECL(ddoinv); /**/
extern int NDECL(dotypeinv); /**/
extern int NDECL(dolook); /**/
extern int NDECL(doprgold); /**/
extern int NDECL(doprwep); /**/
extern int NDECL(doprarm); /**/
extern int NDECL(doprring); /**/
extern int NDECL(dopramulet); /**/
extern int NDECL(doprtool); /**/
extern int NDECL(dosuspend); /**/
extern int NDECL(doforce); /**/
extern int NDECL(doopen); /**/
extern int NDECL(doclose); /**/
extern int NDECL(dosh); /**/
extern int NDECL(dodiscovered); /**/
extern int NDECL(doset); /**/
extern int NDECL(dotogglepickup); /**/
extern int NDECL(dowhatis); /**/
extern int NDECL(doquickwhatis); /**/
extern int NDECL(dowhatdoes); /**/
extern int NDECL(dohelp); /**/
extern int NDECL(dohistory); /**/
extern int NDECL(doloot); /**/
extern int NDECL(dodrink); /**/
extern int NDECL(dodip); /**/
extern int NDECL(dosacrifice); /**/
extern int NDECL(dopray); /**/
extern int NDECL(doturn); /**/
extern int NDECL(doredraw); /**/
extern int NDECL(doread); /**/
extern int NDECL(dosave); /**/
extern int NDECL(dosearch); /**/
extern int NDECL(doidtrap); /**/
extern int NDECL(dopay); /**/
extern int NDECL(dosit); /**/
extern int NDECL(dotalk); /**/
extern int NDECL(docast); /**/
extern int NDECL(dovspell); /**/
extern int NDECL(dotele); /**/
extern int NDECL(dountrap); /**/
extern int NDECL(doversion); /**/
extern int NDECL(doextversion); /**/
extern int NDECL(doswapweapon); /**/
extern int NDECL(dowield); /**/
extern int NDECL(dowieldquiver); /**/
extern int NDECL(dozap); /**/
extern int NDECL(doorganize); /**/
#endif /* DUMB */

#ifdef OVL1
static int NDECL((*timed_occ_fn));
#endif /* OVL1 */

STATIC_PTR int NDECL(doprev_message);
STATIC_PTR int NDECL(timed_occupation);
STATIC_PTR int NDECL(doextcmd);
STATIC_PTR int NDECL(domonability);
# ifdef WIZARD
STATIC_PTR int NDECL(wiz_wish);
STATIC_PTR int NDECL(wiz_identify);
STATIC_PTR int NDECL(wiz_map);
STATIC_PTR int NDECL(wiz_genesis);
STATIC_PTR int NDECL(wiz_where);
STATIC_PTR int NDECL(wiz_detect);
STATIC_PTR int NDECL(wiz_level_tele);
STATIC_PTR int NDECL(wiz_show_seenv);
STATIC_PTR int NDECL(wiz_show_vision);
STATIC_PTR int NDECL(wiz_show_wmodes);
#ifdef __BORLANDC__
extern void FDECL(show_borlandc_stats, (winid));
#endif
STATIC_DCL void FDECL(count_obj, (struct obj *, long *, long *, BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void FDECL(obj_chain, (winid, const char *, struct obj *, long *, long *));
STATIC_DCL void FDECL(mon_invent_chain, (winid, const char *, struct monst *, long *, long *));
STATIC_DCL void FDECL(mon_chain, (winid, const char *, struct monst *, long *, long *));
STATIC_DCL void FDECL(contained, (winid, const char *, long *, long *));
STATIC_PTR int NDECL(wiz_show_stats);
# endif
STATIC_PTR int NDECL(enter_explore_mode);
STATIC_PTR int NDECL(wiz_attributes);
STATIC_PTR int NDECL(doconduct); /**/

#ifdef OVLB
STATIC_DCL void FDECL(enlght_line, (const char *,const char *,const char *));
#ifdef UNIX
static void NDECL(end_of_input);
#endif
#endif /* OVLB */

STATIC_DCL char *NDECL(parse);

#ifdef OVL1

STATIC_PTR int
doprev_message()
{
    return nh_doprev_message();
}

/* Count down by decrementing multi */
STATIC_PTR int
timed_occupation()
{
	(*timed_occ_fn)();
	if (multi > 0)
		multi--;
	return multi > 0;
}

/* If you have moved since initially setting some occupations, they
 * now shouldn't be able to restart.
 *
 * The basic rule is that if you are carrying it, you can continue
 * since it is with you.  If you are acting on something at a distance,
 * your orientation to it must have changed when you moved.
 *
 * The exception to this is taking off items, since they can be taken
 * off in a number of ways in the intervening time, screwing up ordering.
 *
 *	Currently:	Take off all armor.
 *			Picking Locks / Forcing Chests.
 *			Setting traps.
 */
void
reset_occupations()
{
	reset_remarm();
	reset_pick();
	reset_trapset();
}

/* If a time is given, use it to timeout this function, otherwise the
 * function times out by its own means.
 */
void
set_occupation(fn, txt, xtime)
int NDECL((*fn));
const char *txt;
int xtime;
{
	if (xtime) {
		occupation = timed_occupation;
		timed_occ_fn = fn;
	} else
		occupation = fn;
	occtxt = txt;
	occtime = 0;
	return;
}

#ifdef REDO

static char NDECL(popch);

/* Provide a means to redo the last command.  The flag `in_doagain' is set
 * to true while redoing the command.  This flag is tested in commands that
 * require additional input (like `throw' which requires a thing and a
 * direction), and the input prompt is not shown.  Also, while in_doagain is
 * TRUE, no keystrokes can be saved into the saveq.
 */
#define BSIZE 20
static char pushq[BSIZE], saveq[BSIZE];
static NEARDATA int phead, ptail, shead, stail;

static char
popch() {
	/* If occupied, return '\0', letting tgetch know a character should
	 * be read from the keyboard.  If the character read is not the
	 * ABORT character (as checked in pcmain.c), that character will be
	 * pushed back on the pushq.
	 */
	if (occupation) return '\0';
	if (in_doagain) return(char)((shead != stail) ? saveq[stail++] : '\0');
	else		return(char)((phead != ptail) ? pushq[ptail++] : '\0');
}

char
pgetchar() {		/* curtesy of aeb@cwi.nl */
	register int ch;

	if(!(ch = popch()))
		ch = nhgetch();
	return((char)ch);
}

/* A ch == 0 resets the pushq */
void
pushch(ch)
char ch;
{
	if (!ch)
		phead = ptail = 0;
	if (phead < BSIZE)
		pushq[phead++] = ch;
	return;
}

/* A ch == 0 resets the saveq.	Only save keystrokes when not
 * replaying a previous command.
 */
void
savech(ch)
char ch;
{
	if (!in_doagain) {
		if (!ch)
			phead = ptail = shead = stail = 0;
		else if (shead < BSIZE)
			saveq[shead++] = ch;
	}
	return;
}
#endif /* REDO */

#endif /* OVL1 */
#ifdef OVLB

STATIC_PTR int
doextcmd()	/* here after # - now read a full-word command */
{
	int idx, retval;

	/* keep repeating until we don't run help or quit */
	do {
	    idx = get_ext_cmd();
	    if (idx < 0) return 0;	/* quit */

	    retval = (*extcmdlist[idx].ef_funct)();
	} while (extcmdlist[idx].ef_funct == doextlist);

	return retval;
}

int
doextlist()	/* here after #? - now list all full-word commands */
{
	register const struct ext_func_tab *efp;
	char	 buf[BUFSZ];
	winid datawin;

	datawin = create_nhwindow(NHW_TEXT);
	putstr(datawin, 0, "");
/*JP
	putstr(datawin, 0, "            Extended Commands List");
*/
	putstr(datawin, 0, "            拡張コマンド一覧");
	putstr(datawin, 0, "");
/*JP
	putstr(datawin, 0, "    Press '#', then type:");
*/
	putstr(datawin, 0, "    '#'を押したあとタイプせよ:");
	putstr(datawin, 0, "");

	for(efp = extcmdlist; efp->ef_txt; efp++) {
		Sprintf(buf, "    %-14s  - %s.", efp->ef_txt, efp->ef_desc);
		putstr(datawin, 0, buf);
	}
	display_nhwindow(datawin, FALSE);
	destroy_nhwindow(datawin);
	return 0;
}

#ifdef TTY_GRAPHICS
#define MAX_EXT_CMD 40		/* Change if we ever have > 40 ext cmds */
/*
 * This is currently used only by the tty port and is
 * controlled via runtime option 'extmenu'
 */
int
extcmd_via_menu()	/* here after # - now show pick-list of possible commands */
{
    const struct ext_func_tab *efp;
    menu_item *pick_list = (menu_item *)0;
    winid win;
    anything any;
    const struct ext_func_tab *choices[MAX_EXT_CMD];
    char buf[BUFSZ];
    char cbuf[QBUFSZ], prompt[QBUFSZ], fmtstr[20];
    int i, n, nchoices, acount;
    int ret,  biggest;
    int accelerator, prevaccelerator;
    int  matchlevel = 0;

    ret = 0;
    cbuf[0] = '\0';
    biggest = 0;
    while (!ret) {
	    i = n = 0;
	    accelerator = 0;
	    any.a_void = 0;
	    /* populate choices */
	    for(efp = extcmdlist; efp->ef_txt; efp++) {
		if (!matchlevel || !strncmp(efp->ef_txt, cbuf, matchlevel)) {
			choices[i++] = efp;
			if ((int)strlen(efp->ef_desc) > biggest) {
				biggest = strlen(efp->ef_desc);
				Sprintf(fmtstr,"%%-%ds", biggest + 15);
			}
#ifdef DEBUG
			if (i >= MAX_EXT_CMD - 2) {
			    impossible("Exceeded %d extended commands in doextcmd() menu",
					MAX_EXT_CMD - 2);
			    return 0;
			}
#endif
		}
	    }
	    choices[i] = (struct ext_func_tab *)0;
	    nchoices = i;
	    /* if we're down to one, we have our selection so get out of here */
	    if (nchoices == 1) {
		for (i = 0; extcmdlist[i].ef_txt != (char *)0; i++)
			if (!strncmpi(extcmdlist[i].ef_txt, cbuf, matchlevel)) {
				ret = i;
				break;
			}
		break;
	    }

	    /* otherwise... */
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    prevaccelerator = 0;
	    acount = 0;
	    for(i = 0; choices[i]; ++i) {
		accelerator = choices[i]->ef_txt[matchlevel];
		if (accelerator != prevaccelerator || nchoices < (ROWNO - 3)) {
		    if (acount) {
 			/* flush the extended commands for that letter already in buf */
			Sprintf(buf, fmtstr, prompt);
			any.a_char = prevaccelerator;
			add_menu(win, NO_GLYPH, &any, any.a_char, 0,
					ATR_NONE, buf, FALSE);
			acount = 0;
		    }
		}
		prevaccelerator = accelerator;
		if (!acount || nchoices < (ROWNO - 3)) {
		    Sprintf(prompt, "%s [%s]", choices[i]->ef_txt,
				choices[i]->ef_desc);
		} else if (acount == 1) {
		    Sprintf(prompt, "%s or %s", choices[i-1]->ef_txt,
				choices[i]->ef_txt);
		} else {
		    Strcat(prompt," or ");
		    Strcat(prompt, choices[i]->ef_txt);
		}
		++acount;
	    }
	    if (acount) {
		/* flush buf */
		Sprintf(buf, fmtstr, prompt);
		any.a_char = prevaccelerator;
		add_menu(win, NO_GLYPH, &any, any.a_char, 0, ATR_NONE, buf, FALSE);
	    }
	    Sprintf(prompt, "Extended Command: %s", cbuf);
	    end_menu(win, prompt);
	    n = select_menu(win, PICK_ONE, &pick_list);
	    destroy_nhwindow(win);
	    if (n==1) {
		if (matchlevel > (QBUFSZ - 2)) {
			free((genericptr_t)pick_list);
#ifdef DEBUG
			impossible("Too many characters (%d) entered in extcmd_via_menu()",
				matchlevel);
#endif
			ret = -1;
		} else {
			cbuf[matchlevel++] = pick_list[0].item.a_char;
			cbuf[matchlevel] = '\0';
			free((genericptr_t)pick_list);
		}
	    } else {
		if (matchlevel) {
			ret = 0;
			matchlevel = 0;
		} else
			ret = -1;
	    }
    }
    return ret;
}
#endif

STATIC_PTR int
domonability()
{
	if (can_breathe(youmonst.data)) return dobreathe();
	else if (attacktype(youmonst.data, AT_SPIT)) return dospit();
	else if (youmonst.data->mlet == S_NYMPH) return doremove();
	else if (youmonst.data->mlet == S_UMBER) return doconfuse();
	else if (is_were(youmonst.data)) return dosummon();
	else if (webmaker(youmonst.data)) return dospinweb();
	else if (is_hider(youmonst.data)) return dohide();
	else if (is_mind_flayer(youmonst.data)) return domindblast();
	else if (u.umonnum == PM_GREMLIN) {
	    if(IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
		if (split_mon(&youmonst, (struct monst *)0))
		    dryup(u.ux, u.uy);
/*JP	    } else pline("There is no fountain here.");*/
	    } else pline("ここには泉はない．");
	} else if (is_unicorn(youmonst.data)) {
	    use_unicorn_horn((struct obj *)0);
	    return 1;
	} else if (youmonst.data->msound == MS_SHRIEK) {
/*JP	    You("shriek.");*/
	    You("金切り声をあげた．");
	    if(u.uburied)
/*JP		pline("Unfortunately sound does not carry well through rock.");*/
		pline("不幸にも音は岩をうまく伝わらない．");
	    else aggravate();
	} else if (Upolyd)
/*JP		pline("Any special ability you may have is purely reflexive.");*/
		pline("あなたの持っている特殊能力はどれも受動的だ．");
/*JP	else You("don't have a special ability in your normal form!");*/
	else You("特殊能力を持っていない！");
	return 0;
}

STATIC_PTR int
enter_explore_mode()
{
	if(!discover && !wizard) {
/*JP		pline("Beware!  From explore mode there will be no return to normal game.");*/
		pline("警告！発見モードに入ったら通常モードには戻れない．");
/*JP		if (yn("Do you want to enter explore mode?") == 'y') {*/
		if (yn("発見モードに移りますか？") == 'y') {
			clear_nhwindow(WIN_MESSAGE);
/*JP			You("are now in non-scoring explore mode.");*/
			You("スコアがのらない発見モードに移行した．");
			discover = TRUE;
		}
		else {
			clear_nhwindow(WIN_MESSAGE);
/*JP			pline("Resuming normal game.");*/
			pline("通常モードを再開．");
		}
	}
	return 0;
}

#ifdef WIZARD
STATIC_PTR int
wiz_wish()	/* Unlimited wishes for debug mode by Paul Polderman */
{
	if (wizard) {
	    boolean save_verbose = flags.verbose;

	    flags.verbose = FALSE;
	    makewish();
	    flags.verbose = save_verbose;
	    (void) encumber_msg();
	} else
/*JP	    pline("Unavailable command '^W'.");*/
	    pline("'^W'コマンドは使えない．");
	return 0;
}

STATIC_PTR int
wiz_identify()
{
	if (wizard)	identify_pack(0);
/*JP	else		pline("Unavailable command '^I'.");*/
	else		pline("'^I'コマンドは使えない．");
	return 0;
}

/* reveal the level map and any traps on it */
STATIC_PTR int
wiz_map()
{
	if (wizard) {
	    struct trap *t;

	    for (t = ftrap; t != 0; t = t->ntrap) {
		t->tseen = 1;
		map_trap(t, TRUE);
	    }
	    do_mapping();
	} else
/*JP	    pline("Unavailable command '^F'.");*/
	    pline("'^F'コマンドは使えない．");
	return 0;
}

STATIC_PTR int
wiz_genesis()
{
	if (wizard)	(void) create_particular();
/*JP	else		pline("Unavailable command '^G'.");*/
	else		pline("'^G'コマンドは使えない．");
	return 0;
}

STATIC_PTR int
wiz_where()
{
	if (wizard) print_dungeon();
/*JP	else	    pline("Unavailable command '^O'.");*/
	else	    pline("'^O'コマンドは使えない．");
	return 0;
}

STATIC_PTR int
wiz_detect()
{
	if(wizard)  (void) findit();
/*JP	else	    pline("Unavailable command '^E'.");*/
	else	    pline("'^E'コマンドは使えない．");
	return 0;
}

STATIC_PTR int
wiz_level_tele()
{
	if (wizard)	level_tele();
/*JP	else		pline("Unavailable command '^V'.");*/
	else		pline("'^V'コマンドは使えない．");
	return 0;
}

STATIC_PTR int
wiz_show_seenv()
{
	winid win;
	int x, y, v, startx, stopx, curx;
	char row[COLNO+1];

	win = create_nhwindow(NHW_TEXT);
	/*
	 * Each seenv description takes up 2 characters, so center
	 * the seenv display around the hero.
	 */
	startx = max(1, u.ux-(COLNO/4));
	stopx = min(startx+(COLNO/2), COLNO);
	/* can't have a line exactly 80 chars long */
	if (stopx - startx == COLNO/2) startx++;

	for (y = 0; y < ROWNO; y++) {
	    for (x = startx, curx = 0; x < stopx; x++, curx += 2) {
		if (x == u.ux && y == u.uy) {
		    row[curx] = row[curx+1] = '@';
		} else {
		    v = levl[x][y].seenv & 0xff;
		    if (v == 0)
			row[curx] = row[curx+1] = ' ';
		    else
			Sprintf(&row[curx], "%02x", v);
		}
	    }
	    /* remove trailing spaces */
	    for (x = curx-1; x >= 0; x--)
		if (row[x] != ' ') break;
	    row[x+1] = '\0';

	    putstr(win, 0, row);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
	return 0;
}

STATIC_PTR int
wiz_show_vision()
{
	winid win;
	int x, y, v;
	char row[COLNO+1];

	win = create_nhwindow(NHW_TEXT);
	Sprintf(row, "Flags: 0x%x could see, 0x%x in sight, 0x%x temp lit",
		COULD_SEE, IN_SIGHT, TEMP_LIT);
	putstr(win, 0, row);
	putstr(win, 0, "");
	for (y = 0; y < ROWNO; y++) {
	    for (x = 1; x < COLNO; x++) {
		if (x == u.ux && y == u.uy)
		    row[x] = '@';
		else {
		    v = viz_array[y][x]; /* data access should be hidden */
		    if (v == 0)
			row[x] = ' ';
		    else
			row[x] = '0' + viz_array[y][x];
		}
	    }
	    /* remove trailing spaces */
	    for (x = COLNO-1; x >= 1; x--)
		if (row[x] != ' ') break;
	    row[x+1] = '\0';

	    putstr(win, 0, &row[1]);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
	return 0;
}

STATIC_PTR int
wiz_show_wmodes()
{
	winid win;
	int x,y;
	char row[COLNO+1];
	struct rm *lev;

	win = create_nhwindow(NHW_TEXT);
	for (y = 0; y < ROWNO; y++) {
	    for (x = 0; x < COLNO; x++) {
		lev = &levl[x][y];
		if (x == u.ux && y == u.uy)
		    row[x] = '@';
		if (IS_WALL(lev->typ) || lev->typ == SDOOR)
		    row[x] = '0' + (lev->wall_info & WM_MASK);
		else if (lev->typ == CORR)
		    row[x] = '#';
		else if (IS_ROOM(lev->typ) || IS_DOOR(lev->typ))
		    row[x] = '.';
		else
		    row[x] = 'x';
	    }
	    row[COLNO] = '\0';
	    putstr(win, 0, row);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
	return 0;
}

#endif /* WIZARD */


/* -enlightenment and conduct- */
static winid en_win;
static const char
/*JP	*You_ = "You ",
  	*are  = "are ",  *were  = "were ",
  	*have = "have ", *had   = "had ",
	*can  = "can ",  *could = "could ";*/
	*You_ = "あなたは",
	*are  = "である",  *were  = "であった",
	*have = "をもっている", *had   = "をもっていた",
	*can  = "できる",  *could = "できた";
/*JP
static const char
	*have_been  = "have been ",
	*have_never = "have never ", *never = "never ";
*/

#define enl_msg(prefix,present,past,suffix) \
			enlght_line(prefix, suffix, final ? past : present)
#define you_are(attr)	enl_msg(You_,are,were,attr)
#define you_have(attr)	enl_msg(You_,have,had,attr)
#define you_can(attr)	enl_msg(You_,can,could,attr)
#define you_have_been(goodthing) enl_msg(You_,have_been,were,goodthing)
#define you_have_never(badthing) enl_msg(You_,have_never,never,badthing)
#define you_have_X(something)	enl_msg(You_,have,(const char *)"",something)

static void
enlght_line(start, middle, end)
const char *start, *middle, *end;
{
	char buf[BUFSZ];

	Sprintf(buf, "%s%s%s．", start, middle, end);
	putstr(en_win, 0, buf);
}

void
enlightenment(final)
int final;	/* 0 => still in progress; 1 => over, survived; 2 => dead */
{
	int ltmp;
	char buf[BUFSZ];

	en_win = create_nhwindow(NHW_MENU);
/*JP	putstr(en_win, 0, final ? "Final Attributes:" : "Current Attributes:");*/
	putstr(en_win, 0, final ? "最終属性：" : "現在の属性：");
	putstr(en_win, 0, "");

#ifdef ELBERETH
	if (u.uevent.uhand_of_elbereth) {
	    static const char *hofe_titles[3] = {
/*JP				"the Hand of Elbereth",
				"the Envoy of Balance",
				"the Glory of Arioch"*/
				"エルベレスの御手",
				"調和の使者",
				"アリオッチの名誉"
	    };
/*JP*/
	    if(u.uevent.uhand_of_elbereth == 2)
	      you_are(hofe_titles[u.uevent.uhand_of_elbereth - 1]);
	    else
	      you_have(hofe_titles[u.uevent.uhand_of_elbereth - 1]);
	}
#endif

	/* note: piousness 20 matches MIN_QUEST_ALIGN (quest.h) */
#if 0 /*JP*/
	if (u.ualign.record >= 20)	you_are("piously aligned");
	else if (u.ualign.record > 13)	you_are("devoutly aligned");
	else if (u.ualign.record > 8)	you_are("fervently aligned");
	else if (u.ualign.record > 3)	you_are("stridently aligned");
	else if (u.ualign.record == 3)	you_are("aligned");
	else if (u.ualign.record > 0)	you_are("haltingly aligned");
	else if (u.ualign.record == 0)	you_are("nominally aligned");
	else if (u.ualign.record >= -3)	you_have("strayed");
	else if (u.ualign.record >= -8)	you_have("sinned");
	else you_have("transgressed");
#endif /*JP*/
	if (u.ualign.record >= 20)	you_are("敬虔な人間");
	else if (u.ualign.record > 13)	you_are("信心深い人間");
	else if (u.ualign.record > 8)	you_are("熱烈な人間");
	else if (u.ualign.record > 3)	you_are("声のかん高い人間");
	else if (u.ualign.record == 3)	you_are("普通の人間");
	else if (u.ualign.record > 0)	you_are("どもりの人間");
	else if (u.ualign.record == 0)	you_are("有名無実の人間");
	else if (u.ualign.record >= -3)	you_are("迷惑な人間");
	else if (u.ualign.record >= -8)	you_are("許しがたい罪を負った人間");
	else you_are("逸脱した人間");
#ifdef WIZARD
	if (wizard) {
		Sprintf(buf, " %d", u.ualign.record);
/*JP		enl_msg("Your alignment ", "is", "was", buf);*/
		enl_msg("あなたの属性値は", "である", "であった",buf);
	}
#endif

	/*** Resistances to troubles ***/
#if 0	/*JP*/
	if (Fire_resistance) you_are("fire resistant");
	if (Cold_resistance) you_are("cold resistant");
	if (Sleep_resistance) you_are("sleep resistant");
	if (Disint_resistance) you_are("disintegration-resistant");
	if (Shock_resistance) you_are("shock resistant");
	if (Poison_resistance) you_are("poison resistant");
	if (Drain_resistance) you_are("level-drain resistant");
	if (Sick_resistance) you_are("immune to sickness");
	if (Antimagic) you_are("magic-protected");
	if (Acid_resistance) you_are("acid resistant");
	if (Stone_resistance)
		you_are("petrification resistant");
	if (Invulnerable) you_are("invulnerable");
#endif
	if (Fire_resistance) you_have("火への耐性");
	if (Cold_resistance) you_have("寒さへの耐性");
	if (Disint_resistance) you_have("粉砕への耐性");
	if (Shock_resistance) you_have("ショックへの耐性");
	if (Sleep_resistance) you_have("眠りへの耐性");
	if (Poison_resistance) you_have("毒への耐性");
	if (Drain_resistance) you_have("レベルダウンへの耐性");
	if (Sick_resistance) you_have("病気に対する免疫");
	if (Antimagic) you_have("魔法防御能力");
	if (Stone_resistance)
		you_have("石化への耐性");
	if (Invulnerable) you_are("不死身");

	/*** Troubles ***/
	if (Halluc_resistance)
/*JP		enl_msg("You resist", "", "ed", " hallucinations");*/
		you_have("幻覚への耐性");
	if (final) {
/*JP		if (Hallucination) you_are("hallucinating");*/
		if (Hallucination) you_are("幻覚状態");
/*JP		if (Stunned) you_are("stunned");*/
		if (Stunned) you_are("くらくら状態");
/*JP		if (Confusion) you_are("confused");*/
		if (Confusion) you_are("混乱状態");
/*JP		if (Blinded) you_are("blinded");*/
		if (Stunned) you_are("盲目");
		if (Sick) {
			if (u.usick_type & SICK_VOMITABLE)
/*JP				you_are("sick from food poisoning");*/
				enl_msg("あなたは食中毒で気分が", "悪い", "悪かった", "");
			if (u.usick_type & SICK_NONVOMITABLE)
/*JP				you_are("sick from illness");*/
				enl_msg("あなたは病気で気分が", "悪い", "悪かった", "");
		}
	}
/*JP	if (Stoned) you_are("turning to stone");*/
	if (Stoned) you_are("石化状態");
/*JP	if (Strangled) you_are((u.uburied) ? "buried" : "being strangled");*/
	if (Strangled){
		Sprintf(buf, "あなたは%s", (u.uburied) ? "窒息して" : "首を絞められて");
		enl_msg(buf,"いる","いた","");
	}
	if (Glib) {
/*JP		Sprintf(buf, "slippery %s", makeplural(body_part(FINGER)));
		you_have(buf);
*/
		Sprintf(buf, "%sがぬるぬるして", makeplural(body_part(FINGER)));
		enl_msg(buf,"いる","いた","");
	}
/*JP	if (Slimed) you_are("turning into slime");*/
	if (Slimed) enl_msg("あなたは液状になって", "いる","いた","");
/*JP	if (Fumbling) enl_msg("You fumble", "", "d", "");*/
	if (Fumbling) enl_msg("あなたはよく物を","落している","落した","");
	if (Wounded_legs) {
/*JP		Sprintf(buf, "wounded %s", makeplural(body_part(LEG)));
		you_have(buf);
*/
		Sprintf(buf, "あなたは%sを負傷して", makeplural(body_part(LEG)));
		enl_msg(buf,"いる","いた","");
	}
/*JP	if (Hunger) enl_msg("You hunger", "", "ed", " rapidly");*/
	if (Hunger) enl_msg("あなたはすぐに", "腹が減る", "腹が減った", "");

	/*** Vision and senses ***/
#if 0/*JP*/
	if (See_invisible) enl_msg(You_, "see", "saw", " invisible");
	if (Blind_telepat) you_are("telepathic");
	if (Warning) you_are("warned");
	if (Undead_warning) you_are("warned of undead");
	if (Searching) you_have("automatic searching");
	if (Clairvoyant) you_are("clairvoyant");
	if (Infravision) you_have("infravision");
	if (Detect_monsters) you_are("sensing the presence of monsters");
#endif
	if (See_invisible) enl_msg(You_, "見られる", "見られた", "見えないものを");
	if (Blind_telepat) you_have("テレパシー");
	if (Warning) enl_msg("あなたは","警戒能力を持っている","警戒能力を持っていた","");
	if (Undead_warning) you_have("不死の生物への警戒能力");
	if (Searching) you_have("探査能力");
	if (Clairvoyant) you_have("千里眼能力");
	if (Infravision) you_have("暗闇で物を見る能力");
	if (Detect_monsters) you_have("怪物を探す能力");

	/*** Appearance and behavior ***/
#if 0 /*JP*/
	if (Adornment) you_are("adorned");
	if (Invisible) you_are("invisible");
	else if (Invis) you_are("invisible to others");
	/* ordinarily "visible" is redundant; this is a special case for
	   the situation when invisibility would be an expected attribute */
	else if ((HInvis || EInvis || pm_invisible(youmonst.data)) && BInvis)
	    you_are("visible");
	if (Displaced) you_are("displaced");
	if (Stealth) you_are("stealthy");
	if (Aggravate_monster) enl_msg("You aggravate", "", "d", " monsters");
	if (Conflict) enl_msg("You cause", "", "d", " conflict");
#endif
	if (Adornment) you_have("装飾品");
	if (Invisible) you_are("透明");
	else if (Invis) you_are("他人に対して透明");
	else if ((HInvis || EInvis || pm_invisible(youmonst.data)) && BInvis)
	    you_are("不透明");
	if (Displaced) you_have("幻影能力");
	if (Stealth) you_have("人目を盗む能力");
	if (Aggravate_monster) enl_msg("あなたは","反感をかっている","反感をかっていた","");		      
	if (Conflict) enl_msg("あなたは", "引き起こしている","引き起こしていた","闘争を");

	/*** Transportation ***/
#if 0 /*JP*/
	if (Jumping) you_can("jump");
	if (Teleportation) you_can("teleport");
	if (Teleport_control) you_have("teleport control");
	if (Lev_at_will) you_are("levitating, at will");
	else if (Levitation) you_are("levitating");	/* without control */
	else if (Flying) you_can("fly");
	if (Wwalking) you_can("walk on water");
	if (Swimming) you_can("swim");        
	if (Breathless) you_can("survive without air");
	else if (Amphibious) you_can("breathe water");
	if (Passes_walls) you_can("walk through walls");
#endif
	if (Jumping) you_can("跳躍することが");
	if (Teleportation) you_can("瞬間移動が");
	if (Teleport_control) you_have("瞬間移動の制御能力");
	if (Lev_at_will) you_have("上手く浮遊する能力");
	else if (Levitation) you_are("浮遊状態");	/* without control */
	else if (Flying) you_can("飛ぶことが");

	/*** Physical attributes ***/
/*JP	if (Slow_digestion) you_have("slower digestion");*/
	if (Slow_digestion) enl_msg("食物の消化が", "遅い", "遅かった", "");
/*JP	if (Regeneration) enl_msg("You regenerate", "", "d", "");*/
	if (Regeneration) you_have("再生能力");
/*JP	if (Protection) you_are("protected");*/
	if (Protection) enl_msg("あなたは","守られている","守られていた","");
	if (Protection_from_shape_changers)
/*JP		you_are("protected from shape changers");*/
		you_have("変化怪物への耐性");
/*JP	if (Polymorph) you_are("polymorphing");*/
	if (Polymorph) enl_msg("あなたは","変化している","変化していた","");
/*JP	if (Polymorph_control) you_have("polymorph control");*/
	if (Polymorph_control) you_have("変化の制御能力");
	if (u.ulycn >= LOW_PM) {
		Strcpy(buf, an(mons[u.ulycn].mname));
		you_are(buf);
	}
	if (Upolyd) {
/*JP	    if (u.ulycn >= LOW_PM) Strcpy(buf, "in beast form");*/
	    if (u.ulycn >= LOW_PM) Strcpy(buf, "獣の姿を");
/*JP	    else Sprintf(buf, "polymorphed into %s", an(youmonst.data->mname));*/
	    else Sprintf(buf, "%sに変化", a_monnam(&youmonst));
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%d)", u.mtimedone);
#endif
/*JP	    you_are(buf);*/
	    enl_msg(buf, "している", "していた", "");
	}
/*JP	if (Unchanging) you_can("not change from your current form");*/
	if (Unchanging) enl_msg("今の姿から変化することが",
				"できない", "できなかった", "");
/*JP	if (Fast) you_are(Very_fast ? "very fast" : "fast");*/
	if (Fast) you_have(Very_fast ? "とても素早く行動する能力" :
			   "素早く行動する能力");
/*JP	if (Reflecting) you_have("reflection");*/
	if (Reflecting) you_have("反射能力");
/*JP	if (Free_action) you_have("free action");*/
	if (Free_action) you_have("拘束されない能力");
/*JP	if (Fixed_abil) you_have("fixed abilities");*/
	if (Fixed_abil) enl_msg("能力が変化", "しない", "しなかった", "");
	if (Lifesaved)
/*JP		enl_msg("Your life ", "will be", "would have been", " saved");*/
		enl_msg("あなたの生命は","保存されている","保存されていた","");

	/*** Miscellany ***/
#if 0 /*JP*/
	if (Luck) {
	    ltmp = abs((int)Luck);
	    Sprintf(buf, "%s%slucky",
		    ltmp >= 10 ? "extremely " : ltmp >= 5 ? "very " : "",
		    Luck < 0 ? "un" : "");
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%d)", Luck);
#endif
	    you_are(buf);
	}
#endif
	if (Luck) {
	    ltmp = abs((int)Luck);
	    Sprintf(buf, "%s%s",
		    ltmp >= 10 ? "猛烈に" : ltmp >= 5 ? "とても" : "",
		    Luck < 0 ? "不幸" : "幸福");
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), "(%d)", Luck);
#endif
	    you_are(buf);
	}
#ifdef WIZARD
/*JP	 else if (wizard) enl_msg("Your luck ", "is", "was", " zero");*/
	 else if (wizard) enl_msg("あなたの運はゼロ", "である", "であった", "");
#endif
#if 0 /*JP*/
	if (u.moreluck > 0) you_have("extra luck");
	else if (u.moreluck < 0) you_have("reduced luck");
	if (carrying(LUCKSTONE) || stone_luck(TRUE)) {
	    ltmp = stone_luck(FALSE);
	    if (ltmp <= 0)
		enl_msg("Bad luck ", "does", "did", " not time out for you");
	    if (ltmp >= 0)
		enl_msg("Good luck ", "does", "did", " not time out for you");
	}
#endif
 	if (u.moreluck > 0) you_have("さらなる幸運");
	else if (u.moreluck < 0) you_have("さらなる悪運");
	if (carrying(LUCKSTONE) || stone_luck(TRUE)) {
	    ltmp = stone_luck(FALSE);
	    if (ltmp <= 0)
		enl_msg("悪運は", "去っていない", "去っていなかった", "");
	    if (ltmp >= 0)
		enl_msg("幸運は", "去っていない", "去っていなかった", "");
	}
#if 0
	if (u.ugangr) {
	    Sprintf(buf, " %sangry with you",
		    u.ugangr > 6 ? "extremely " : u.ugangr > 3 ? "very " : "");
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%d)", u.ugangr);
#endif
	    enl_msg(u_gname(), " is", " was", buf);
	} else
#endif
	if (u.ugangr) {
	    Sprintf(buf, "%sは%s怒って", u_gname(),
		    u.ugangr > 6 ? "猛烈に" : u.ugangr > 3 ? "とても" : "");
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), "(%d)", u.ugangr);
#endif
	    enl_msg(buf, "いる", "いた", "");
	} else
	    /*
	     * We need to suppress this when the game is over, because death
	     * can change the value calculated by can_pray(), potentially
	     * resulting in a false claim that you could have prayed safely.
	     */
	  if (!final) {
#if 0	/*JP*/
	    /* "can [not] safely pray" vs "could [not] have safely prayed" */
	    Sprintf(buf, "%s%ssafely pray%s", can_pray(FALSE) ? "" : "not ",
		    final ? "have " : "", final ? "ed" : "");
	    Sprintf(buf, "%ssafely pray", can_pray(FALSE) ? "" : "not ");
	    you_can(buf);
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), " (%d)", u.ublesscnt);
#endif
#endif
	    if(can_pray(FALSE))
	      Sprintf(buf, "できる");
	    else
	      Sprintf(buf, "できない");
#ifdef WIZARD
	    if (wizard) Sprintf(eos(buf), "(%d)", u.ublesscnt);
#endif
	    enl_msg("あなたは", buf, buf, "安全に祈ることが");
	  }

    {
	const char *p;

	buf[0] = '\0';
	if (final < 2) {    /* still in progress, or quit/escaped/ascended */
#if 0 /*JP*/
	    p = "survived after being killed ";
	    switch (u.umortality) {
	    case 0:  p = !final ? (char *)0 : "survived";  break;
	    case 1:  Strcpy(buf, "once");  break;
	    case 2:  Strcpy(buf, "twice");  break;
	    case 3:  Strcpy(buf, "thrice");  break;
	    default: Sprintf(buf, "%d times", u.umortality);
		     break;
#endif
	    p = "死んだ後復活してた";
	    switch (u.umortality) {
	    case 0:  p = !final ? (char *)0 : "生き延びていた";  break;
	    case 1:  Strcpy(buf, "一度");  break;
	    case 2:  Strcpy(buf, "二度");  break;
	    case 3:  Strcpy(buf, "三度");  break;
	    default: Sprintf(buf, "%d回", u.umortality);
		     break;
	    }
	} else {		/* game ended in character's death */
#if 0 /*JP*/
	    p = "are dead";
	    switch (u.umortality) {
	    case 0:  impossible("dead without dying?");
	    case 1:  break;			/* just "are dead" */
	    default: Sprintf(buf, " (%d%s time!)", u.umortality,
			     ordin(u.umortality));
		     break;
	    }
#endif
	    p = "死んでいる";
	    switch (u.umortality) {
	    case 0:  impossible("dead without dying?");
	    case 1:  break;			/* just "are dead" */
	    default: Sprintf(buf, "(%d回！)", u.umortality);
		     break;
	    }
	}
/*JP	if (p) enl_msg(You_, "have been killed ", p, buf);*/
	if (p) enl_msg(You_, "死んでいる", p, buf);
    }

	display_nhwindow(en_win, TRUE);
	destroy_nhwindow(en_win);
	return;
#endif /*JP*/
}

STATIC_PTR int
wiz_attributes()
{
	if (wizard || discover)
		enlightenment(0);
	else
/*JP		pline("Unavailable command '^X'.");*/
		pline("'^X'コマンドは使えない．");
	return 0;
}

/* KMH, #conduct
 * (shares enlightenment's tense handling)
 */
STATIC_PTR int
doconduct()
{
	show_conduct(0);
	return 0;
}

void
show_conduct(final)
int final;
{
	char buf[BUFSZ];
	int ngenocided;

	/* Create the conduct window */
	en_win = create_nhwindow(NHW_MENU);
/*JP	putstr(en_win, 0, "Voluntary challenges:");*/
	putstr(en_win, 0, "自発的行動");
	putstr(en_win, 0, "");

	if (!u.uconduct.food)
/*JP	    enl_msg(You_, "have gone", "went", " without food");*/
	    enl_msg("あなたは", "食事をしていない", "食事せずに死んだ", "");
	    /* But beverages are okay */
	else if (!u.uconduct.flesh)
/*JP	    you_have_been("a strict vegan");*/
	    you_are("厳格な菜食主義者");
	else if (!u.uconduct.meat)
/*JP	    you_have_been("vegetarian");*/
	    you_are("菜食主義者");

	if (!u.uconduct.gnostic)
/*JP	    you_have_been("an atheist");*/
	    you_are("無神論者");

	if (!u.uconduct.weaphit)
/*JP	    you_have_never("hit with a wielded weapon");*/
	    enl_msg("あなたは装備している武器で攻撃し", "ていない", "なかった", "");
#ifdef WIZARD
	else if (wizard) {
/*JP	    Sprintf(buf, "used a wielded weapon %ld time%s",
		    u.uconduct.weaphit, plur(u.uconduct.weaphit));
*/
	    Sprintf(buf, "あなたは%ld回装備した武器を使用",
		    u.uconduct.weaphit);
	    enl_msg(buf, "している", "した", "");
	}
#endif
	if (!u.uconduct.killer)
/*JP	    you_have_been("a pacifist");*/
	    you_are("平和主義者");

	if (!u.uconduct.literate)
/*JP	    you_have_been("illiterate");*/
	    enl_msg("あなたは読み書きして", "いない", "いなかった", "");
#ifdef WIZARD
	else if (wizard) {
/*JP	    Sprintf(buf, "read items or engraved %ld time%s",*/
	    Sprintf(buf, "%ld回読んだり書いたり",
		    u.uconduct.literate);
	    enl_msg(buf, "している", "した", "");
	}
#endif

	ngenocided = num_genocides();
	if (ngenocided == 0) {
/*JP	    you_have_never("genocided any monsters");*/
	    enl_msg("あなたは怪物を虐殺して", "いない", "いなかった", "");
	} else {
/*JP	    Sprintf(buf, "genocided %d type%s of monster%s",
		    ngenocided, plur(ngenocided), plur(ngenocided));
*/
	    Sprintf(buf, "%d種の怪物を虐殺",
		    ngenocided);
	    enl_msg(buf, "している", "した", "");
	}

	if (!u.uconduct.polypiles)
/*JP	    you_have_never("polymorphed an object");*/
	    enl_msg("あなたは物体を変化させて", "いない", "いなかった", "");
#ifdef WIZARD
	else if (wizard) {
/*JP	    Sprintf(buf, "polymorphed %ld item%s",
		    u.uconduct.polypiles, plur(u.uconduct.polypiles));
*/
	    Sprintf(buf, "%ldの道具を変化",
		    u.uconduct.polypiles);
	    enl_msg(buf, "させている", "させた", "");
	}
#endif

	if (!u.uconduct.polyselfs)
/*JP	    you_have_never("changed form");*/
	    enl_msg("あなたは変化して", "いない", "いなかった", "");
#ifdef WIZARD
	else if (wizard) {
/*JP	    Sprintf(buf, "changed form %ld time%s",*/
	    Sprintf(buf, "%ld回姿を変え",
		    u.uconduct.polyselfs);
	    enl_msg(buf, "ている", "た", "");
	}
#endif

	if (!u.uconduct.wishes)
/*JP	    you_have_X("used no wishes");*/
	    enl_msg("あなたは望みを叶えて", "いない", "いなかった", "");
	else {
/*JP	    Sprintf(buf, "used %ld wish%s",
		    u.uconduct.wishes, (u.uconduct.wishes > 1L) ? "es" : "");
*/
	    Sprintf(buf, "%ld回望みを叶え",
		    u.uconduct.wishes);
	    enl_msg(buf, "ている", "た", "");

	    if (!u.uconduct.wisharti)
/*JP		enl_msg(You_, "have not wished", "did not wish",
			" for any artifacts");
*/
		enl_msg("あなたは聖器を望んで", "いない", "いなかった", "");
	}

	/* Pop up the window and wait for a key */
	display_nhwindow(en_win, TRUE);
	destroy_nhwindow(en_win);
}

#ifdef OVL1

#ifndef M
# ifndef NHSTDC
#  define M(c)		(0x80 | (c))
# else
#  define M(c)		((c) - 128)
# endif /* NHSTDC */
#endif
#ifndef C
#define C(c)		(0x1f & (c))
#endif

static const struct func_tab cmdlist[] = {
	{C('d'), FALSE, dokick}, /* "D" is for door!...?  Msg is in dokick.c */
#ifdef WIZARD
	{C('e'), TRUE, wiz_detect},
	{C('f'), TRUE, wiz_map},
	{C('g'), TRUE, wiz_genesis},
	{C('i'), TRUE, wiz_identify},
#endif
	{C('l'), TRUE, doredraw}, /* if number_pad is set */
#ifdef WIZARD
	{C('o'), TRUE, wiz_where},
#endif
	{C('p'), TRUE, doprev_message},
	{C('r'), TRUE, doredraw},
	{C('t'), TRUE, dotele},
#ifdef WIZARD
	{C('v'), TRUE, wiz_level_tele},
	{C('w'), TRUE, wiz_wish},
#endif
	{C('x'), TRUE, wiz_attributes},
#ifdef SUSPEND
	{C('z'), TRUE, dosuspend},
#endif
	{'a', FALSE, doapply},
	{'A', FALSE, doddoremarm},
	{M('a'), TRUE, doorganize},
/*	'b', 'B' : go sw */
	{'c', FALSE, doclose},
	{'C', TRUE, do_mname},
	{M('c'), TRUE, dotalk},
	{'d', FALSE, dodrop},
	{'D', FALSE, doddrop},
	{M('d'), FALSE, dodip},
	{'e', FALSE, doeat},
	{'E', FALSE, doengrave},
	{M('e'), TRUE, enhance_weapon_skill},
	{'f', FALSE, dofire},
/*	'F' : fight (one time) */
	{M('f'), FALSE, doforce},
/*	'g', 'G' : multiple go */
/*	'h', 'H' : go west */
	{'h', TRUE, dohelp}, /* if number_pad is set */
	{'i', TRUE, ddoinv},
	{'I', TRUE, dotypeinv},		/* Robert Viduya */
	{M('i'), TRUE, doinvoke},
/*	'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N' : move commands */
	{'j', FALSE, dojump}, /* if number_pad is on */
	{M('j'), FALSE, dojump},
	{'k', FALSE, dokick}, /* if number_pad is on */
	{'l', FALSE, doloot}, /* if number_pad is on */
	{M('l'), FALSE, doloot},
/*	'n' prefixes a count if number_pad is on */
	{M('m'), TRUE, domonability},
	{'N', TRUE, ddocall}, /* if number_pad is on */
	{M('n'), TRUE, ddocall},
	{M('N'), TRUE, ddocall},
	{'o', FALSE, doopen},
	{'O', TRUE, doset},
	{M('o'), FALSE, dosacrifice},
	{'p', FALSE, dopay},
	{'P', FALSE, doputon},
	{M('p'), TRUE, dopray},
	{'q', FALSE, dodrink},
	{'Q', FALSE, dowieldquiver},
	{M('q'), TRUE, done2},
	{'r', FALSE, doread},
	{'R', FALSE, doremring},
	{M('r'), FALSE, dorub},
/*JP	{'s', TRUE, dosearch, "searching"},*/
	{'s', TRUE, dosearch, "捜す"},
	{'S', TRUE, dosave},
	{M('s'), FALSE, dosit},
	{'t', FALSE, dothrow},
	{'T', FALSE, dotakeoff},
	{M('t'), TRUE, doturn},
/*	'u', 'U' : go ne */
	{'u', FALSE, dountrap}, /* if number_pad is on */
	{M('u'), FALSE, dountrap},
	{'v', TRUE, doversion},
	{'V', TRUE, dohistory},
	{M('v'), TRUE, doextversion},
	{'w', FALSE, dowield},
	{'W', FALSE, dowear},
	{M('w'), FALSE, dowipe},
	{'x', FALSE, doswapweapon},
	{'X', TRUE, enter_explore_mode},
/*	'y', 'Y' : go nw */
	{'z', FALSE, dozap},
	{'Z', TRUE, docast},
	{'<', FALSE, doup},
	{'>', FALSE, dodown},
	{'/', TRUE, dowhatis},
	{'&', TRUE, dowhatdoes},
	{'?', TRUE, dohelp},
	{M('?'), TRUE, doextlist},
#ifdef SHELL
	{'!', TRUE, dosh},
#endif
/*JP	{'.', TRUE, donull, "waiting"},
	{' ', TRUE, donull, "waiting"},*/
	{'.', TRUE, donull, "休憩する"},
	{' ', TRUE, donull, "休憩する"},
	{',', FALSE, dopickup},
	{':', TRUE, dolook},
	{';', TRUE, doquickwhatis},
	{'^', TRUE, doidtrap},
	{'\\', TRUE, dodiscovered},		/* Robert Viduya */
	{'@', TRUE, dotogglepickup},
/*JP*/
	{'`', TRUE, dotogglelang},
	{WEAPON_SYM,  TRUE, doprwep},
	{ARMOR_SYM,  TRUE, doprarm},
	{RING_SYM,  TRUE, doprring},
	{AMULET_SYM, TRUE, dopramulet},
	{TOOL_SYM, TRUE, doprtool},
	{'*', TRUE, doprinuse},	/* inventory of all equipment in use */
	{GOLD_SYM, TRUE, doprgold},
	{SPBOOK_SYM, TRUE, dovspell},			/* Mike Stephenson */
	{'#', TRUE, doextcmd},
	{0,0,0,0}
};

struct ext_func_tab extcmdlist[] = {
#if 0 /*JP*/
	{"adjust", "adjust inventory letters", doorganize, TRUE},
	{"chat", "talk to someone", dotalk, TRUE},	/* converse? */
	{"conduct", "list which challenges you have adhered to", doconduct, TRUE},
	{"dip", "dip an object into something", dodip, FALSE},
	{"enhance", "advance or check weapons skills", enhance_weapon_skill,
							TRUE},
	{"force", "force a lock", doforce, FALSE},
	{"invoke", "invoke an object's powers", doinvoke, TRUE},
	{"jump", "jump to a location", dojump, FALSE},
	{"loot", "loot a box on the floor", doloot, FALSE},
	{"monster", "use a monster's special ability", domonability, TRUE},
	{"name", "name an item or type of object", ddocall, TRUE},
	{"offer", "offer a sacrifice to the gods", dosacrifice, FALSE},
	{"pray", "pray to the gods for help", dopray, TRUE},
	{"quit", "exit without saving current game", done2, TRUE},
#ifdef STEED
	{"ride", "ride (or stop riding) a monster", doride, FALSE},
#endif
	{"rub", "rub a lamp", dorub, FALSE},
	{"sit", "sit down", dosit, FALSE},
	{"turn", "turn undead", doturn, TRUE},
	{"twoweapon", "toggle two-weapon combat", dotwoweapon, FALSE},
	{"untrap", "untrap something", dountrap, FALSE},
	{"version", "list compile time options for this version of NetHack",
		doextversion, TRUE},
	{"wipe", "wipe off your face", dowipe, FALSE},
	{"?", "get this list of extended commands", doextlist, TRUE},
#endif /*JP*/

	{"adjust", "持ち物一覧の調整", doorganize, TRUE},
	{"chat", "誰かと話す", dotalk, TRUE},	/* converse? */
	{"conduct", "どういう行動をとったか見る", doconduct, TRUE},
	{"dip", "何かに物を浸す", dodip, FALSE},
	{"enhance", "武器熟練度を高める", enhance_weapon_skill, TRUE},
	{"force", "鍵をこじあける", doforce, FALSE},
	{"invoke", "物の特別な力を使う", doinvoke, TRUE},
	{"jump", "他の位置に飛びうつる", dojump, FALSE},
	{"loot", "床の上の箱を開ける", doloot, TRUE},
	{"monster", "怪物の特別能力を使う", domonability, TRUE},
	{"name", "アイテムや物に名前をつける", ddocall, TRUE},
	{"offer", "神に供物を捧げる", dosacrifice, FALSE},
	{"pray", "神に祈る", dopray, TRUE},
	{"quit", "セーブしないで終了", done2, TRUE},
#ifdef STEED
	{"ride", "怪物に乗る(または降りる)", doride, FALSE},
#endif
	{"rub", "ランプをこする", dorub, FALSE},
	{"sit", "座る", dosit, FALSE},
	{"turn", "アンデットを土に返す", doturn, TRUE},
	{"twoweapon", "両手持ちの切り替え", dotwoweapon, FALSE},
	{"untrap", "罠をはずす", dountrap, FALSE},
	{"version", "コンパイル時のオプションを表示する",
		doextversion, TRUE},
	{"wipe", "顔を拭う", dowipe, FALSE},
	{"?", "この拡張コマンド一覧を表示する", doextlist, TRUE},
#if defined(WIZARD)
	/*
	 * There must be a blank entry here for every entry in the table
	 * below.
	 */
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
	{(char *)0, (char *)0, donull, TRUE},
#ifdef DEBUG
	{(char *)0, (char *)0, donull, TRUE},
#endif
	{(char *)0, (char *)0, donull, TRUE},
#endif
	{(char *)0, (char *)0, donull, TRUE}	/* sentinel */
};

#if defined(WIZARD)
static const struct ext_func_tab debug_extcmdlist[] = {
	{"light sources", "show mobile light sources", wiz_light_sources, TRUE},
	{"seenv", "show seen vectors", wiz_show_seenv, TRUE},
	{"stats", "show memory statistics", wiz_show_stats, TRUE},
	{"timeout", "look at timeout queue", wiz_timeout_queue, TRUE},
	{"vision", "show vision array", wiz_show_vision, TRUE},
#ifdef DEBUG
	{"wizdebug", "wizard debug command", wiz_debug_cmd, TRUE},
#endif
	{"wmode", "show wall modes", wiz_show_wmodes, TRUE},
	{(char *)0, (char *)0, donull, TRUE}
};

/*
 * Insert debug commands into the extended command list.  This function
 * assumes that the last entry will be the help entry.
 *
 * You must add entries in ext_func_tab every time you add one to the
 * debug_extcmdlist().
 */
void
add_debug_extended_commands()
{
	int i, j, k, n;

	/* count the # of help entries */
	for (n = 0; extcmdlist[n].ef_txt[0] != '?'; n++)
	    ;

	for (i = 0; debug_extcmdlist[i].ef_txt; i++) {
	    for (j = 0; j < n; j++)
		if (strcmp(debug_extcmdlist[i].ef_txt, extcmdlist[j].ef_txt) < 0) break;

	    /* insert i'th debug entry into extcmdlist[j], pushing down  */
	    for (k = n; k >= j; --k)
		extcmdlist[k+1] = extcmdlist[k];
	    extcmdlist[j] = debug_extcmdlist[i];
	    n++;	/* now an extra entry */
	}
}


static const char *template = "%-18s %4ld  %6ld";
static const char *count_str = "                   count  bytes";
static const char *separator = "------------------ -----  ------";

STATIC_OVL void
count_obj(chain, total_count, total_size, top, recurse)
	struct obj *chain;
	long *total_count;
	long *total_size;
	boolean top;
	boolean recurse;
{
	long count, size;
	struct obj *obj;

	for (count = size = 0, obj = chain; obj; obj = obj->nobj) {
	    if (top) {
		count++;
		size += sizeof(struct obj) + obj->oxlth + obj->onamelth;
	    }
	    if (recurse && obj->cobj)
		count_obj(obj->cobj, total_count, total_size, TRUE, TRUE);
	}
	*total_count += count;
	*total_size += size;
}

STATIC_OVL void
obj_chain(win, src, chain, total_count, total_size)
	winid win;
	const char *src;
	struct obj *chain;
	long *total_count;
	long *total_size;
{
	char buf[BUFSZ];
	long count = 0, size = 0;

	count_obj(chain, &count, &size, TRUE, FALSE);
	*total_count += count;
	*total_size += size;
	Sprintf(buf, template, src, count, size);
	putstr(win, 0, buf);
}

STATIC_OVL void
mon_invent_chain(win, src, chain, total_count, total_size)
	winid win;
	const char *src;
	struct monst *chain;
	long *total_count;
	long *total_size;
{
	char buf[BUFSZ];
	long count = 0, size = 0;
	struct monst *mon;

	for (mon = chain; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, TRUE, FALSE);
	*total_count += count;
	*total_size += size;
	Sprintf(buf, template, src, count, size);
	putstr(win, 0, buf);
}

STATIC_OVL void
contained(win, src, total_count, total_size)
	winid win;
	const char *src;
	long *total_count;
	long *total_size;
{
	char buf[BUFSZ];
	long count = 0, size = 0;
	struct monst *mon;

	count_obj(invent, &count, &size, FALSE, TRUE);
	count_obj(fobj, &count, &size, FALSE, TRUE);
	count_obj(level.buriedobjlist, &count, &size, FALSE, TRUE);
	count_obj(migrating_objs, &count, &size, FALSE, TRUE);
	for (mon = fmon; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, FALSE, TRUE);
	for (mon = migrating_mons; mon; mon = mon->nmon)
	    count_obj(mon->minvent, &count, &size, FALSE, TRUE);

	*total_count += count; *total_size += size;

	Sprintf(buf, template, src, count, size);
	putstr(win, 0, buf);
}

STATIC_OVL void
mon_chain(win, src, chain, total_count, total_size)
	winid win;
	const char *src;
	struct monst *chain;
	long *total_count;
	long *total_size;
{
	char buf[BUFSZ];
	long count, size;
	struct monst *mon;

	for (count = size = 0, mon = chain; mon; mon = mon->nmon) {
	    count++;
	    size += sizeof(struct monst) + mon->mxlth + mon->mnamelth;
	}
	*total_count += count;
	*total_size += size;
	Sprintf(buf, template, src, count, size);
	putstr(win, 0, buf);
}

/*
 * Display memory usage of all monsters and objects on the level.
 */
static int
wiz_show_stats()
{
	char buf[BUFSZ];
	winid win;
	long total_obj_size = 0, total_obj_count = 0;
	long total_mon_size = 0, total_mon_count = 0;

	win = create_nhwindow(NHW_TEXT);
	putstr(win, 0, "Current memory statistics:");
	putstr(win, 0, "");
	Sprintf(buf, "Objects, size %d", (int) sizeof(struct obj));
	putstr(win, 0, buf);
	putstr(win, 0, "");
	putstr(win, 0, count_str);

	obj_chain(win, "invent", invent, &total_obj_count, &total_obj_size);
	obj_chain(win, "fobj", fobj, &total_obj_count, &total_obj_size);
	obj_chain(win, "buried", level.buriedobjlist,
				&total_obj_count, &total_obj_size);
	obj_chain(win, "migrating obj", migrating_objs,
				&total_obj_count, &total_obj_size);
	mon_invent_chain(win, "minvent", fmon,
				&total_obj_count,&total_obj_size);
	mon_invent_chain(win, "migrating minvent", migrating_mons,
				&total_obj_count, &total_obj_size);

	contained(win, "contained",
				&total_obj_count, &total_obj_size);

	putstr(win, 0, separator);
	Sprintf(buf, template, "Total", total_obj_count, total_obj_size);
	putstr(win, 0, buf);

	putstr(win, 0, "");
	putstr(win, 0, "");
	Sprintf(buf, "Monsters, size %d", (int) sizeof(struct monst));
	putstr(win, 0, buf);
	putstr(win, 0, "");

	mon_chain(win, "fmon", fmon,
				&total_mon_count, &total_mon_size);
	mon_chain(win, "migrating", migrating_mons,
				&total_mon_count, &total_mon_size);

	putstr(win, 0, separator);
	Sprintf(buf, template, "Total", total_mon_count, total_mon_size);
	putstr(win, 0, buf);

#ifdef __BORLANDC__
	show_borlandc_stats(win);
#endif

	display_nhwindow(win, FALSE);
	destroy_nhwindow(win);
	return 0;
}

void
sanity_check()
{
	obj_sanity_check();
	timer_sanity_check();
}

#endif /* WIZARD */

#define unctrl(c)	((c) <= C('z') ? (0x60 | (c)) : (c))
#define unmeta(c)	(0x7f & (c))


void
rhack(cmd)
register char *cmd;
{
	boolean do_walk, do_rush, prefix_seen, bad_command,
		firsttime = (cmd == 0);

#ifdef NEWBIE
	if(flags.newbie){
	    if(!(moves % 256)){
		if(!newbie.search){
		    pline("ヒント: 's'で秘密の通路や扉を発見できる．");
		    newbie.search = 1;
		}
		else if(!newbie.pickup){
		    pline("ヒント: ','で通路に落ちている物体を拾うことができる．");
		    newbie.pickup = 1;
		}
		else if(!newbie.open){
		    pline("ヒント: 'o'で扉を開けることができる．");
		    newbie.open = 1;
		}
		else if(!newbie.down){
		    pline("ヒント: 下の階へ行くには'>'だ．");
		    newbie.down = 1;
		}
		else if(!newbie.pray){
		    pline("ヒント: ピンチのときに'M-p'で祈ると助かることがある．");
		    newbie.pray = 1;
		}
		else if(!newbie.offer && newbie.found_altar && newbie.found_altar + 256 < moves){
		    pline("ヒント: 祭壇に'M-o'で捧げものをしよう．いいことがあるかも．");
		    newbie.offer = 1;
		}
		else if(!newbie.loot && newbie.found_chest  && newbie.found_chest + 256 < moves){
		    pline("ヒント: 宝箱は'M-l'で中身の出し入れをすることができる．");
		    newbie.loot = 1;
		}
	    }
	}
#endif

	if (firsttime) {
		flags.nopick = 0;
		cmd = parse();
	}
	if (*cmd == '\033') {
		flags.move = FALSE;
		return;
	}
#ifdef REDO
	if (*cmd == DOAGAIN && !in_doagain && saveq[0]) {
		in_doagain = TRUE;
		stail = 0;
		rhack((char *)0);	/* read and execute command */
		in_doagain = FALSE;
		return;
	}
	/* Special case of *cmd == ' ' handled better below */
	if(!*cmd || *cmd == (char)0377)
#else
	if(!*cmd || *cmd == (char)0377 || (!flags.rest_on_space && *cmd == ' '))
#endif
	{
		nhbell();
		flags.move = FALSE;
		return;		/* probably we just had an interrupt */
	}

	/* handle most movement commands */
	do_walk = do_rush = prefix_seen = FALSE;
	switch (*cmd) {
	 case 'g':  if (movecmd(cmd[1])) {
			flags.run = 2;
			do_rush = TRUE;
		    } else
			prefix_seen = TRUE;
		    break;
	 case '5':  if (!iflags.num_pad) break;	/* else FALLTHRU */
	 case 'G':  if (movecmd(lowc(cmd[1]))) {
			flags.run = 3;
			do_rush = TRUE;
		    } else
			prefix_seen = TRUE;
		    break;
	 case '-':  if (!iflags.num_pad) break;	/* else FALLTHRU */
	/* Effects of movement commands and invisible monsters:
	 * m: always move onto space (even if 'I' remembered)
	 * F: always attack space (even if 'I' not remembered)
	 * normal movement: attack if 'I', move otherwise
	 */
	 case 'F':  if (movecmd(cmd[1])) {
			flags.forcefight = 1;
			do_walk = TRUE;
		    } else
			prefix_seen = TRUE;
		    break;
	 case 'm':  if (movecmd(cmd[1]) || u.dz) {
			flags.run = 0;
			flags.nopick = 1;
			if (!u.dz) do_walk = TRUE;
			else cmd[0] = cmd[1];	/* "m<" or "m>" */
		    } else
			prefix_seen = TRUE;
		    break;
	 case 'M':  if (movecmd(lowc(cmd[1]))) {
			flags.run = 1;
			flags.nopick = 1;
			do_rush = TRUE;
		    } else
			prefix_seen = TRUE;
		    break;
	 case '0':  if (!iflags.num_pad) break;
		    (void)ddoinv(); /* a convenience borrowed from the PC */
		    flags.move = FALSE;
		    multi = 0;
		    return;
	 default:   if (movecmd(*cmd)) {	/* ordinary movement */
			do_walk = TRUE;
		    } else if (movecmd(iflags.num_pad ?
				       unmeta(*cmd) : lowc(*cmd))) {
			flags.run = 1;
			do_rush = TRUE;
		    } else if (movecmd(unctrl(*cmd))) {
			flags.run = 3;
			do_rush = TRUE;
		    }
		    break;
	}
	if (do_walk) {
	    if (multi) flags.mv = TRUE;
	    domove();
	    flags.forcefight = 0;
	    return;
	} else if (do_rush) {
	    if (firsttime) {
		if (!multi) multi = max(COLNO,ROWNO);
		u.last_str_turn = 0;
	    }
	    flags.mv = TRUE;
	    domove();
	    return;
	} else if (prefix_seen && cmd[1] == '\033') {	/* <prefix><escape> */
	    /* don't report "unknown command" for change of heart... */
	    bad_command = FALSE;
	} else if (*cmd == ' ' && !flags.rest_on_space) {
	    bad_command = TRUE;		/* skip cmdlist[] loop */

	/* handle all other commands */
	} else {
	    register const struct func_tab *tlist;
	    int res, NDECL((*func));

	    for (tlist = cmdlist; tlist->f_char; tlist++) {
		if ((*cmd & 0xff) != (tlist->f_char & 0xff)) continue;

		if (u.uburied && !tlist->can_if_buried) {
/*JP		    You_cant("do that while you are buried!");*/
		    You("埋まっている時にそんなことはできない！");
		    res = 0;
		} else {
		    /* we discard 'const' because some compilers seem to have
		       trouble with the pointer passed to set_occupation() */
		    func = ((struct func_tab *)tlist)->f_funct;
		    if (tlist->f_text && !occupation && multi)
			set_occupation(func, tlist->f_text, multi);
		    res = (*func)();		/* perform the command */
		}
		if (!res) {
		    flags.move = FALSE;
		    multi = 0;
		}
		return;
	    }
	    /* if we reach here, cmd wasn't found in cmdlist[] */
	    bad_command = TRUE;
	}

	if (bad_command) {
	    char expcmd[10];
	    register char *cp = expcmd;

	    while (*cmd && (int)(cp - expcmd) < (int)(sizeof expcmd - 3)) {
		if (*cmd >= 040 && *cmd < 0177) {
		    *cp++ = *cmd++;
		} else if (*cmd & 0200) {
		    *cp++ = 'M';
		    *cp++ = '-';
		    *cp++ = *cmd++ &= ~0200;
		} else {
		    *cp++ = '^';
		    *cp++ = *cmd++ ^ 0100;
		}
	    }
	    *cp = '\0';
/*JP	    Norep("Unknown command '%s'.", expcmd);*/
	    Norep("'%s'コマンド？", expcmd);
	}
	/* didn't move */
	flags.move = FALSE;
	multi = 0;
	return;
}

int
xytod(x, y)	/* convert an x,y pair into a direction code */
schar x, y;
{
	register int dd;

	for(dd = 0; dd < 8; dd++)
	    if(x == xdir[dd] && y == ydir[dd]) return dd;

	return -1;
}

void
dtoxy(cc,dd)	/* convert a direction code into an x,y pair */
coord *cc;
register int dd;
{
	cc->x = xdir[dd];
	cc->y = ydir[dd];
	return;
}

int
movecmd(sym)	/* also sets u.dz, but returns false for <> */
char sym;
{
	register const char *dp;
	register const char *sdp;
	if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */

	u.dz = 0;
	if(!(dp = index(sdp, sym))) return 0;
	u.dx = xdir[dp-sdp];
	u.dy = ydir[dp-sdp];
	u.dz = zdir[dp-sdp];
#ifdef JPEXTENSION
	if(Totter){	/* mirror move */
	    u.dx = -u.dx;
	}
#endif
	if (u.dx && u.dy && u.umonnum == PM_GRID_BUG) {
		u.dx = u.dy = 0;
		return 0;
	}
	return !u.dz;
}

int
getdir(s)
const char *s;
{
	char dirsym;

#ifdef REDO	
	if(in_doagain)
	    dirsym = readchar();
	else
#endif
/*JP	    dirsym = yn_function (s ? s : "In what direction?",*/
	    dirsym = yn_function (s ? s : "どの方向？",
					(char *)0, '\0');
#ifdef REDO
	savech(dirsym);
#endif
	if(dirsym == '.' || dirsym == 's')
		u.dx = u.dy = u.dz = 0;
	else if(!movecmd(dirsym) && !u.dz) {
		if(!index(quitchars, dirsym))
/*JP			pline("What a strange direction!");*/
			pline("ずいぶんと奇妙な方向だ！");
		return 0;
	}
	if(!u.dz && (Stunned || (Confusion && !rn2(5)))) confdir();
	return 1;
}

#endif /* OVL1 */
#ifdef OVLB

void
confdir()
{
	register int x = (u.umonnum == PM_GRID_BUG) ? 2*rn2(4) : rn2(8);
	u.dx = xdir[x];
	u.dy = ydir[x];
	return;
}

#endif /* OVLB */
#ifdef OVL0

int
isok(x,y)
register int x, y;
{
	/* x corresponds to curx, so x==1 is the first column. Ach. %% */
	return x >= 1 && x <= COLNO-1 && y >= 0 && y <= ROWNO-1;
}

static NEARDATA int last_multi;

/*
 * convert a MAP window position into a movecmd
 */
int
click_to_cmd(x, y, mod)
    int x, y, mod;
{
    x -= u.ux;
    y -= u.uy;
    /* convert without using floating point, allowing sloppy clicking */
    if(x > 2*abs(y))
	x = 1, y = 0;
    else if(y > 2*abs(x))
	x = 0, y = 1;
    else if(x < -2*abs(y))
	x = -1, y = 0;
    else if(y < -2*abs(x))
	x = 0, y = -1;
    else
	x = sgn(x), y = sgn(y);

    if(x == 0 && y == 0)	/* map click on player to "rest" command */
	return '.';

    x = xytod(x, y);
    if(mod == CLICK_1) {
	return (iflags.num_pad ? ndir[x] : sdir[x]);
    } else {
	return (iflags.num_pad ? M(ndir[x]) :
		(sdir[x] - 'a' + 'A')); /* run command */
    }
}

STATIC_OVL char *
parse()
{
#ifdef LINT	/* static char in_line[COLNO]; */
	char in_line[COLNO];
#else
	static char in_line[COLNO];
#endif
	register int foo;
	boolean prezero = FALSE;

	multi = 0;
	flags.move = 1;
	flush_screen(1); /* Flush screen buffer. Put the cursor on the hero. */

	if (!iflags.num_pad || (foo = readchar()) == 'n')
	    for (;;) {
		foo = readchar();
		if (foo >= '0' && foo <= '9') {
		    multi = 10 * multi + foo - '0';
		    if (multi < 0 || multi >= LARGEST_INT) multi = LARGEST_INT;
		    if (multi > 9) {
			clear_nhwindow(WIN_MESSAGE);
/*JP			Sprintf(in_line, "Count: %d", multi);*/
			Sprintf(in_line, "数: %d", multi);
			pline(in_line);
			mark_synch();
		    }
		    last_multi = multi;
		    if (!multi && foo == '0') prezero = TRUE;
		} else break;	/* not a digit */
	    }

	if (foo == '\033') {   /* esc cancels count (TH) */
	    clear_nhwindow(WIN_MESSAGE);
	    multi = last_multi = 0;
# ifdef REDO
	} else if (foo == DOAGAIN || in_doagain) {
	    multi = last_multi;
	} else {
	    last_multi = multi;
	    savech(0);	/* reset input queue */
	    savech((char)foo);
# endif
	}

	if (multi) {
	    multi--;
	    save_cm = in_line;
	} else {
	    save_cm = (char *)0;
	}
	in_line[0] = foo;
	in_line[1] = '\0';
	if (foo == 'g' || foo == 'G' || (iflags.num_pad && foo == '5') ||
	    foo == 'm' || foo == 'M' || foo == 'F') {
	    foo = readchar();
#ifdef REDO
	    savech((char)foo);
#endif
	    in_line[1] = foo;
	    in_line[2] = 0;
	}
	clear_nhwindow(WIN_MESSAGE);
	if (prezero) in_line[0] = '\033';
	return(in_line);
}

#endif /* OVL0 */
#ifdef OVLB

#ifdef UNIX
static
void
end_of_input()
{
	exit_nhwindows("End of input?");
#ifndef NOSAVEONHANGUP
	if (!program_state.done_hup++)
	    (void) dosave0();
#endif
	clearlocks();
	terminate(EXIT_SUCCESS);
}
#endif

#endif /* OVLB */
#ifdef OVL0

char
readchar()
{
	register int sym;
	int x = u.ux, y = u.uy, mod = 0;

#ifdef REDO
	sym = in_doagain ? Getchar() : nh_poskey(&x, &y, &mod);
#else
	sym = Getchar();
#endif

#ifdef UNIX
# ifdef NR_OF_EOFS
	if (sym == EOF) {
	    register int cnt = NR_OF_EOFS;
	  /*
	   * Some SYSV systems seem to return EOFs for various reasons
	   * (?like when one hits break or for interrupted systemcalls?),
	   * and we must see several before we quit.
	   */
	    do {
		clearerr(stdin);	/* omit if clearerr is undefined */
		sym = Getchar();
	    } while (--cnt && sym == EOF);
	}
# endif /* NR_OF_EOFS */
	if (sym == EOF)
	    end_of_input();
#endif /* UNIX */

	if(sym == 0) /* click event */
	    sym = click_to_cmd(x, y, mod);
	return((char) sym);
}
#endif /* OVL0 */

/*cmd.c*/
