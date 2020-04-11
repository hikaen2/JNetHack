/*	SCCS Id: @(#)cmd.c	3.1	93/06/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "func_tab.h"

/*
 * Some systems may have getchar() return EOF for various reasons, and
 * we should not quit before seeing at least NR_OF_EOFS consecutive EOFs.
 */
#if defined(SYSV) || defined(DGUX) || defined(HPUX)
#define	NR_OF_EOFS	20
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
extern int NDECL(dowield); /**/
extern int NDECL(dozap); /**/
extern int NDECL(doorganize); /**/
#endif /* DUMB */

#ifdef OVL1
static int NDECL((*timed_occ_fn));
#endif /* OVL1 */

STATIC_PTR int NDECL(doprev_message);
STATIC_PTR int NDECL(timed_occupation);
STATIC_PTR int NDECL(doextcmd);
# ifdef POLYSELF
STATIC_PTR int NDECL(domonability);
# endif
# ifdef WIZARD
STATIC_PTR int NDECL(wiz_wish);
STATIC_PTR int NDECL(wiz_identify);
STATIC_PTR int NDECL(wiz_map);
STATIC_PTR int NDECL(wiz_genesis);
STATIC_PTR int NDECL(wiz_where);
STATIC_PTR int NDECL(wiz_detect);
STATIC_PTR int NDECL(wiz_level_tele);
# endif
# ifdef EXPLORE_MODE
STATIC_PTR int NDECL(enter_explore_mode);
# endif
# if defined(WIZARD) || defined(EXPLORE_MODE)
STATIC_PTR int NDECL(wiz_attributes);
# endif

#ifdef OVLB
static void FDECL(enlght_line, (const char *,const char *,const char *));
#ifdef UNIX
static void NDECL(end_of_input);
#endif
#endif /* OVLB */

STATIC_OVL char *NDECL(parse);

#ifdef UNIX
extern boolean hu;
#endif

#ifdef OVL1

STATIC_PTR int
doprev_message()
{
    return nh_doprev_message();
}

/* Count down by decrementing multi */
STATIC_PTR int
timed_occupation() {
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
 */
void
reset_occupations() {

	reset_remarm();
	reset_pick();
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
	char buf[BUFSZ];
	register const struct ext_func_tab *efp = extcmdlist;
again:
#ifdef COM_COMPL
	get_ext_cmd(buf);
#else
	getlin("#", buf);
#endif
	if(buf[0] == '\0' || buf[0] == '\033')
		return 0;
	if(buf[0] == '?') {
		(void) doextlist();
		goto again;
	}
	while(efp->ef_txt) {
		if(!strncmpi(efp->ef_txt, buf,BUFSIZ))
			return (*(efp->ef_funct))();
		efp++;
	}
/*JP	pline("%s: unknown extended command.", buf);*/
	pline("%s: 拡張コマンド？", buf);
	return 0;
}

int
doextlist()	/* here after #? - now list all full-word commands */
{
	register const struct ext_func_tab *efp;
	char	 buf[BUFSZ];
	winid datawin;

	datawin = create_nhwindow(NHW_TEXT);
	putstr(datawin, 0, "");
/*JP	putstr(datawin, 0, "            Extended Commands List");*/
	putstr(datawin, 0, "            拡張コマンド一覧");
	putstr(datawin, 0, "");
#ifdef COM_COMPL
/*JP	putstr(datawin, 0, "    Press '#', then type (first letter only):");*/
	putstr(datawin, 0, "    '#'を押したあとタイプせよ(最初の1文字):");
#else
/*JP	putstr(datawin, 0, "    Press '#', then type:");*/
	putstr(datawin, 0, "    '#'を押したあとタイプせよ:");
#endif
	putstr(datawin, 0, "");

	for(efp = extcmdlist; efp->ef_txt; efp++) {
		Sprintf(buf, "    %-8s  - %s.", efp->ef_txt, efp->ef_desc);
		putstr(datawin, 0, buf);
	}
	display_nhwindow(datawin, FALSE);
	destroy_nhwindow(datawin);
	return 0;
}

#ifdef POLYSELF
STATIC_PTR int
domonability()
{
	if (can_breathe(uasmon)) return dobreathe();
	else if (attacktype(uasmon, AT_SPIT)) return dospit();
	else if (u.usym == S_NYMPH) return doremove();
	else if (u.usym == S_UMBER) return doconfuse();
	else if (is_were(uasmon)) return dosummon();
	else if (webmaker(uasmon)) return dospinweb();
	else if (is_hider(uasmon)) return dohide();
	else if(u.umonnum == PM_GREMLIN) {
	    if(IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
		struct monst *mtmp;
		if ((mtmp = cloneu()) != 0) {
			mtmp->mhpmax = (u.mhmax /= 2);
/*JP			You("multiply.");*/
			You("分裂した．");
			dryup(u.ux,u.uy);
		}
/*JP	    } else pline("There is no fountain here.");*/
	    } else pline("ここには泉はない．");
	}
	else if (u.usym == S_UNICORN) {
	    use_unicorn_horn((struct obj *)0);
	    return 1;
	} else if (u.umonnum == PM_MIND_FLAYER) return domindblast();
	else if (uasmon->msound == MS_SHRIEK) {
/*JP	    You("shriek.");*/
	    You("金切り声をあげた．");
	    aggravate();
	} else if (u.umonnum >= 0)
/*JP		pline("Any special ability you may have is purely reflexive.");*/
		pline("あなたの持っている特殊能力はどれも受動的だ．");
/*JP	else You("don't have a special ability!");*/
	else You("特殊能力を持っていない！");
	return 0;
}
#endif

#ifdef EXPLORE_MODE
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
#endif

#ifdef WIZARD
STATIC_PTR int
wiz_wish()	/* Unlimited wishes for debug mode by Paul Polderman */
{
	if (wizard) {
	    makewish();
	    (void) encumber_msg();
	} else
/*JP	    pline("Unavailable command '^W'.");*/
	    pline("'^W'コマンドは使えない．");
	return 0;
}

STATIC_PTR int
wiz_identify()
{
	struct obj *obj;

	if (!wizard)
/*JP		pline("Unavailable command '^I'.");*/
		pline("'^I'コマンドは使えない．");
	else {
		for (obj = invent; obj; obj = obj->nobj)
			if (!objects[obj->otyp].oc_name_known || !obj->known
			    || !obj->dknown || !obj->bknown || !obj->rknown)
				(void) identify(obj);
	}
	return 0;
}

STATIC_PTR int
wiz_map()
{
	if (wizard)	do_mapping();
/*JP	else		pline("Unavailable command '^F'.");*/
	else		pline("'^F'コマンドは使えない．");
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

#endif /* WIZARD */

/* -enlightenment- */
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

#define enl_msg(prefix,present,past,suffix) \
			enlght_line(prefix, suffix, final ? past : present)
#define you_are(attr)	enl_msg(You_,are,were,attr)
#define you_have(attr)	enl_msg(You_,have,had,attr)
#define you_can(attr)	enl_msg(You_,can,could,attr)

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
boolean final;
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
/*JP	if (u.ualign.record >= 20)	you_are("piously aligned");
	else if (u.ualign.record > 13)	you_are("devoutly aligned");
	else if (u.ualign.record > 8)	you_are("fervently aligned");
	else if (u.ualign.record > 3)	you_are("stridently aligned");
	else if (u.ualign.record == 3)	you_are("aligned");
	else if (u.ualign.record > 0)	you_are("haltingly aligned");
	else if (u.ualign.record == 0)	you_are("nominally aligned");
	else if (u.ualign.record >= -3)	you_have("strayed");
	else if (u.ualign.record >= -8)	you_have("sinned");
	else you_have("transgressed");*/
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
		enl_msg("あなたの属性値は","である","であった",buf);
	}
#endif

/*JP	if (Telepat) you_are("telepathic");*/
	if (Telepat) you_have("テレパシー能力");
/*JP	if (Searching) you_have("automatic searching");*/
	if (Searching) you_have("探査能力");
/*JP	if (Teleportation) you_can("teleport");*/
	if (Teleportation) you_can("瞬間移動");
/*JP	if (Teleport_control) you_have("teleport control");*/
	if (Teleport_control) you_have("瞬間移動の制御能力");
/*JP	if (See_invisible) enl_msg(You_, "see", "saw", " invisible");*/
	if (See_invisible) enl_msg(You_, "見れる", "見れた", "見えないものを");
/*JP	if (Invisible) you_are("invisible");*/
	if (Invisible) you_are("透明");
/*JP	else if (Invis) you_are("invisible to others");*/
	else if (Invis) you_are("他人に対して透明");
/*JP	if (Fast) you_are((Fast & ~INTRINSIC) ? "very fast" : "fast");*/
	if (Fast) you_have((Fast & ~INTRINSIC) ? "とても素早く行動する能力" : "素早く行動する能力");
/*JP	if (Stealth) you_are("stealthy");*/
	if (Stealth) you_have("人目を盗む能力");
/*JP	if (Regeneration) enl_msg("You regenerate", "", "d", "");*/
	if (Regeneration) you_have("再生能力");
/*JP	if (Hunger) enl_msg("You hunger", "", "ed", " rapidly");*/
	if (Hunger) enl_msg("あなたはすぐに", "腹が減る", "腹が減った", "");
/*JP	if (Conflict) enl_msg("You cause", "", "d", " conflict");*/
	if (Conflict) enl_msg("あなたは","引き起こす","引き起こした","闘争を");
/*JP	if (Aggravate_monster) enl_msg("You aggravate", "", "d", " monsters");*/
	if (Aggravate_monster) enl_msg("あなたは","反感をかっている","反感をかっていた","");		      
/*JP	if (Poison_resistance) you_are("poison resistant");*/
	if (Poison_resistance) you_have("毒への耐性");
/*JP	if (Fire_resistance) you_are("fire resistant");*/
	if (Fire_resistance) you_have("火への耐性");
/*JP	if (Cold_resistance) you_are("cold resistant");*/
	if (Cold_resistance) you_have("寒さへの耐性");
/*JP	if (Shock_resistance) you_are("shock resistant");*/
	if (Shock_resistance) you_have("ショックへの耐性");
/*JP	if (Sleep_resistance) you_are("sleep resistant");*/
	if (Sleep_resistance) you_have("眠りへの耐性");
/*JP	if (Disint_resistance) you_are("disintegration-resistant");*/
	if (Disint_resistance) you_have("粉砕への耐性");
	if (Protection_from_shape_changers)
/*JP		you_are("protected from shape changers");*/
		you_have("変化怪物への耐性");
#ifdef POLYSELF
/*JP	if (Polymorph) you_are("polymorphing");*/
	if (Polymorph) enl_msg("あなたは","変化している","変化していた","");
/*JP	if (Polymorph_control) you_have("polymorph control");*/
	if (Polymorph_control) you_have("変化の制御能力");
#endif
	if (HHalluc_resistance)
/*JP		enl_msg("You resist", "", "ed", " hallucinations");*/
		you_have("幻覚への耐性");
	if (final) {
/*JP		if (Hallucination) you_are("hallucinating");*/
		if (Hallucination) you_are("幻覚状態");
/*JP		if (Stunned) you_are("stunned");*/
		if (Stunned) you_are("くらくら状態");
/*JP		if (Confusion) you_are("confused");*/
		if (Confusion) you_are("混乱状態");
/*JP		if (Sick) you_are("sick");*/
		if (Sick) you_are("病気");
/*JP		if (Blinded) you_are("blinded");*/
		if (Blinded) you_are("盲目");
	}
	if (Wounded_legs) {
/*JP		Sprintf(buf, "wounded %s", makeplural(body_part(LEG)));*/
		Sprintf(buf, "あなたは%sを負傷して", makeplural(body_part(LEG)));
/*JP		you_have(buf);*/
		enl_msg(buf,"いる","いた","");
	}
	if (Glib) {
/*JP		Sprintf(buf, "slippery %s", makeplural(body_part(FINGER)));*/
		Sprintf(buf, "%sが痺れて", makeplural(body_part(FINGER)));
/*JP		you_have(buf);*/
		enl_msg(buf,"いる","いた","");
	}
/*JP	if (Strangled) you_are("being strangled");*/
	if (Strangled) you_are("窒息状態");
/*JP	if (Stoned) you_are("turning to stone");*/
	if (Stoned) you_are("石化状態");
	if (Lifesaved)
/*JP		enl_msg("Your life ", "will be", "would have been", " saved");*/
		enl_msg("あなたの生命は","保存されている","保存されていた","");
/*JP	if (Adornment) you_are("adorned");*/
	if (Adornment) you_have("装飾品");
/*JP	if (Warning) you_are("warned");*/
	if (Warning) enl_msg("あなたは","警戒能力を持っている","警戒能力を持っていた","");
/*JP	if (Protection) you_are("protected");*/
	if (Protection) enl_msg("あなたは","守られている","守られていた","");
/*JP	if (Reflecting) you_have("reflection");*/
	if (Reflecting) you_have("反射能力");
/*JP	if (Levitation) you_are("levitating");*/
	if (Levitation) you_are("浮遊状態");
#ifdef POLYSELF
/*JP	else if (is_flyer(uasmon)) you_can("fly");*/
	else if (is_flyer(uasmon)) you_can("飛行");
#endif
/*JP	if (Fumbling) enl_msg("You fumble", "", "d", "");*/
	if (Fumbling) enl_msg("あなたはよく物を","落している","落した","");
/*JP	if (Jumping) you_can("jump");*/
	if (Jumping) you_can("跳躍");
/*JP	if (Wwalking) you_can("walk on water");*/
	if (Wwalking) you_can("水上歩行");
#ifdef POLYSELF
/*JP	if (passes_walls(uasmon)) you_can("walk through walls");*/
	if (passes_walls(uasmon)) you_can("壁を通りぬけることが");
#endif
/*JP	if (Breathless) you_can("survive without air");*/
	if (Breathless) you_can("空気なしで生存することが");
/*JP	else if (Amphibious) you_can("breathe water");*/
	else if (Amphibious) you_can("水中呼吸");
/*JP	if (Antimagic) you_are("magic-protected");*/
	if (Antimagic) you_have("魔法防御能力");
/*JP	if (Displaced) you_are("displaced");*/
	if (Displaced) you_have("幻影能力");
/*JP	if (Clairvoyant) you_are("clairvoyant");*/
	if (Clairvoyant) you_have("千里眼能力");
#ifdef POLYSELF
	if (u.ulycn != -1) {	
/*JP		Strcpy(buf, an(mons[u.ulycn].mname));*/
		Strcpy(buf, jtrns_mon(mons[u.ulycn].mname));
		you_are(buf);
	}
#endif
	if (Luck) {
	    ltmp = abs((int)Luck);
/*JP	    Sprintf(buf, "%s%slucky",*/
	    Sprintf(buf, "%s%s",
/*JP		    ltmp >= 10 ? "extremely " : ltmp >= 5 ? "very " : "",
		    Luck < 0 ? "un" : "");*/
		    ltmp >= 10 ? "猛烈に" : ltmp >= 5 ? "とても" : "",
		    Luck < 0 ? "不幸" : "幸福");
#ifdef WIZARD
/*JP	    if (wizard) Sprintf(eos(buf), " (%d)", Luck);*/
	    if (wizard) Sprintf(eos(buf), "(%d)", Luck);
#endif
	    you_are(buf);
	}
#ifdef WIZARD
/*JP	 else if (wizard) enl_msg("Your luck ", "is", "was", " zero");*/
	 else if (wizard) enl_msg("あなたの運はゼロ","である","あった","");
#endif
	ltmp = stone_luck(TRUE);
/*JP	if (ltmp > 0) you_have("extra luck");*/
	if (ltmp > 0) you_have("特殊な運");
/*JP	else if (ltmp < 0) you_have("reduced luck");*/
	else if (ltmp < 0) you_have("特殊な悪運");
	if (carrying(LUCKSTONE)) {
	    ltmp = stone_luck(FALSE);
	    if (ltmp <= 0)
/*JP		enl_msg("Bad luck ", "does", "did", " not time out for you");*/
		enl_msg("悪運は", "去っていない","去っていなかった","");
	    if (ltmp >= 0)
		enl_msg("幸運は", "去っていない","去っていなかった","");
	}

	if (u.umortality>1) {
/*JP*/
	    --u.umortality;
	    if (u.umortality == 1)
/*JP		Sprintf(buf, "once");*/
		Sprintf(buf, "一度");
	    else
/*JP		Sprintf(buf, "%d times", u.umortality);*/
		Sprintf(buf, "%d回", u.umortality);
/*JP	    enl_msg("You ", "have been killed ", "died ", buf);*/
	    enl_msg("あなたは", "死んでいる", "死んでいた", buf);
	}

	display_nhwindow(en_win, TRUE);
	destroy_nhwindow(en_win);
	return;
}

#if defined(WIZARD) || defined(EXPLORE_MODE)
STATIC_PTR int
wiz_attributes()
{
	if (wizard || discover)
		enlightenment(FALSE);
	else
/*JP		pline("Unavailable command '^X'.");*/
		pline("'^X'コマンドは使えない．");
	return 0;
}
#endif /* WIZARD || EXPLORE_MODE */

#endif /* OVLB */
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
	{C('d'), dokick},	/* "D" is for door!...? */
#ifdef WIZARD
	{C('e'), wiz_detect},
	{C('f'), wiz_map},
	{C('g'), wiz_genesis},
	{C('i'), wiz_identify},
#endif
	{C('l'), doredraw}, /* if number_pad is set */
#ifdef WIZARD
	{C('o'), wiz_where},
#endif
	{C('p'), doprev_message},
	{C('r'), doredraw},
	{C('t'), dotele},
#ifdef WIZARD
	{C('v'), wiz_level_tele},
	{C('w'), wiz_wish},
#endif
#if defined(WIZARD) || defined(EXPLORE_MODE)
	{C('x'), wiz_attributes},
#endif
#ifdef SUSPEND
	{C('z'), dosuspend},
#endif
	{'a', doapply},
	{'A', doddoremarm},
	{M('a'), doorganize},
/*	'b', 'B' : go sw */
	{'c', doclose},
	{'C', do_mname},
	{M('c'), dotalk},
	{'d', dodrop},
	{'D', doddrop},
	{M('d'), dodip},
	{'e', doeat},
	{'E', doengrave},
/* Soon to be
	{'f', dofight, "fighting"},
	{'F', doFight, "fighting"},
 */
	{M('f'), doforce},
/*	'g', 'G' : multiple go */
/*	'h', 'H' : go west */
	{'h', dohelp}, /* if number_pad is set */
	{'i', ddoinv},
	{'I', dotypeinv},		/* Robert Viduya */
	{M('i'), doinvoke},
/*	'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N' : move commands */
	{'j', dojump}, /* if number_pad is on */
	{M('j'), dojump},
	{'k', dokick}, /* if number_pad is on */
	{'l', doloot}, /* if number_pad is on */
	{M('l'), doloot},
/*	'n' prefixes a count if number_pad is on */
#ifdef POLYSELF
	{M('m'), domonability},
#endif /* POLYSELF */
	{'N', ddocall}, /* if number_pad is on */
	{M('n'), ddocall},
	{M('N'), ddocall},
	{'o', doopen},
	{'O', doset},
	{M('o'), dosacrifice},
	{'p', dopay},
	{'P', doputon},
	{M('p'), dopray},
	{'q', dodrink},
	{'Q', done2},
	{'r', doread},
	{'R', doremring},
	{M('r'), dorub},
/*JP	{'s', dosearch, "searching"},*/
	{'s', dosearch, "探す"},
	{'S', dosave},
	{M('s'), dosit},
	{'t', dothrow},
	{'T', dotakeoff},
	{M('t'), doturn},
/*	'u', 'U' : go ne */
	{'u', dountrap}, /* if number_pad is on */
	{M('u'), dountrap},
	{'v', doversion},
	{'V', dohistory},
	{M('v'), doextversion},
	{'w', dowield},
	{'W', dowear},
	{M('w'), dowipe},
	{'x', dovspell},			/* Mike Stephenson */
#ifdef EXPLORE_MODE
	{'X', enter_explore_mode},
#endif
/*	'y', 'Y' : go nw */
	{'z', dozap},
	{'Z', docast},
	{'<', doup},
	{'>', dodown},
	{'/', dowhatis},
	{'&', dowhatdoes},
	{'?', dohelp},
	{M('?'), doextlist},
#ifdef SHELL
	{'!', dosh},
#endif
/*JP	{'.', donull, "waiting"},
	{' ', donull, "waiting"},*/
	{'.', donull, "休憩する"},
	{' ', donull, "休憩する"},
	{',', dopickup},
	{':', dolook},
	{';', doquickwhatis},
	{'^', doidtrap},
	{'\\', dodiscovered},		/* Robert Viduya */
	{'@', dotogglepickup},
/*JP*/	
	{'`', dotogglelang},
	{WEAPON_SYM,  doprwep},
	{ARMOR_SYM,  doprarm},
	{RING_SYM,  doprring},
	{AMULET_SYM, dopramulet},
	{TOOL_SYM, doprtool},
	{GOLD_SYM, doprgold},
	{SPBOOK_SYM, dovspell},			/* Mike Stephenson */
	{'#', doextcmd},
	{0,0,0}
};

const struct ext_func_tab extcmdlist[] = {
#if 0
	{"adjust", "adjust inventory letters", doorganize},
	{"chat", "talk to someone", dotalk},	/* converse? */
	{"dip", "dip an object into something", dodip},
	{"force", "force a lock", doforce},
	{"invoke", "invoke an object's powers", doinvoke},
	{"jump", "jump to a location", dojump},
	{"loot", "loot a box on the floor", doloot},
#ifdef POLYSELF
	{"monster", "use a monster's special ability", domonability},
#endif
	{"name", "name an item or type of object", ddocall},
	{"offer", "offer a sacrifice to the gods", dosacrifice},
	{"pray", "pray to the gods for help", dopray},
	{"rub", "rub a lamp", dorub},
	{"sit", "sit down", dosit},
	{"turn", "turn undead", doturn},
	{"untrap", "untrap something", dountrap},
	{"version", "list compile time options for this version of NetHack",
		doextversion},
	{"wipe", "wipe off your face", dowipe},
	{"?", "get this list of extended commands", doextlist},
	{NULL, NULL, donull}
#endif
	{"adjust", "持ち物一覧の調整", doorganize},
	{"chat", "誰かと話す", dotalk},	/* converse? */
	{"dip", "何かに物を浸す", dodip},
	{"force", "鍵をこじあける", doforce},
	{"invoke", "物の特別な力を使う", doinvoke},
	{"jump", "他の位置に飛びうつる", dojump},
	{"loot", "床の上の箱を開ける", doloot},
#ifdef POLYSELF
	{"monster", "怪物の特別能力を使う", domonability},
#endif
	{"name", "アイテムや物に名前をつける", ddocall},
	{"offer", "神に供物を捧げる", dosacrifice},
	{"pray", "神に祈る", dopray},
	{"rub", "ランプをこする", dorub},
	{"sit", "座る", dosit},
	{"turn", "アンデットを土に返す", doturn},
	{"untrap", "罠をはずす", dountrap},
	{"version", "コンパイル時のオプションを表示する",
		doextversion},
	{"wipe", "顔を拭う", dowipe},
	{"?", "この拡張コマンド一覧を表示する", doextlist},
	{NULL, NULL, donull}
};

#define unctrl(c)	((c) <= C('z') ? (0x60 | (c)) : (c))
#define unmeta(c)	(0x7f & (c))


void
rhack(cmd)
register char *cmd;
{
	register const struct func_tab *tlist = cmdlist;
	boolean firsttime = FALSE;
	register int res;

	if(!cmd) {
		firsttime = TRUE;
		flags.nopick = 0;
		cmd = parse();
	}
	if(*cmd == (char)033) {
		flags.move = 0;
		return;
	}
#ifdef REDO
	if (*cmd == DOAGAIN && !in_doagain && saveq[0]) {
		in_doagain = TRUE;
		stail = 0;
		rhack(NULL);	/* read and execute command */
		in_doagain = FALSE;
		return;
	}
	/* Special case of *cmd == ' ' handled better below */
	if(!*cmd || *cmd == (char)0377) {
#else
	if(!*cmd || *cmd == (char)0377 ||
	   (!flags.rest_on_space && *cmd == ' ')) {
#endif
		nhbell();
		flags.move = 0;
		return;		/* probably we just had an interrupt */
	}
	if(movecmd(*cmd)) {
	walk:
		if(multi) flags.mv = 1;
		domove();
		return;
	}
	if(movecmd(flags.num_pad ? unmeta(*cmd) : lowc(*cmd))) {
		flags.run = 1;
	rush:
		if(firsttime){
			if(!multi) multi = COLNO;
			u.last_str_turn = 0;
		}
		flags.mv = 1;
		domove();
		return;
	}
	if(*cmd == 'g' && movecmd(cmd[1])) {
		flags.run = 2;
		goto rush;
	}
	if (((*cmd == 'G' || (flags.num_pad && *cmd == '5')) &&
	    movecmd(lowc(cmd[1]))) || movecmd(unctrl(*cmd))) {
		flags.run = 3;
		goto rush;
	}
	if((*cmd == 'm' || (flags.num_pad && *cmd == '-')) &&
	    movecmd(cmd[1])) {
		flags.run = 0;
		flags.nopick = 1;
		goto walk;
	}
	if(*cmd == 'M' && movecmd(lowc(cmd[1]))) {
		flags.run = 1;
		flags.nopick = 1;
		goto rush;
	}
	if (flags.num_pad && *cmd == '0') {
		(void)ddoinv();	/* A convenience borrowed from the PC */
		flags.move = 0;
		multi = 0;
		return;
	}
	while(tlist->f_char) {
		if((*cmd & 0xff) == (tlist->f_char & 0xff)){
			/* Special case of *cmd == ' ' handled here */
			if (*cmd == ' ' && !flags.rest_on_space)
				break;

			/* Now control-A can stop lengthy commands */
			/* in the PC version only -- use ^C-N otherwise */
			if (tlist->f_text && !occupation && multi)
#ifdef GCC_WARN
				set_occupation(tlist->f_funct,
						tlist->f_text, multi);
#else
				set_occupation(((struct func_tab *)tlist)->f_funct,
					tlist->f_text, multi);
#endif
			res = (*(tlist->f_funct))();
			if(!res) {
				flags.move = 0;
				multi = 0;
			}
			return;
		}
		tlist++;
	}
	{ char expcmd[10];
	  register char *cp = expcmd;
	  while(*cmd && cp-expcmd < sizeof(expcmd)-2) {
		if(*cmd >= 040 && *cmd < 0177)
			*cp++ = *cmd++;
		else if (*cmd & 0200) {
			*cp++ = 'M';
			*cp++ = '-';
			*cp++ = *cmd++ &=~ 0200;
		}
		else {
			*cp++ = '^';
			*cp++ = *cmd++ ^ 0100;
		}
	  }
	  *cp = 0;
/*JP	  Norep("Unknown command '%s'.", expcmd);*/
	  Norep("'%s'コマンド？", expcmd);
	}
	multi = flags.move = 0;
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

#ifdef WALKIES
void
dtoxy(cc,dd)	/* convert a direction code into an x,y pair */
coord *cc;
register int dd;
{
	cc->x = xdir[dd];
	cc->y = ydir[dd];
	return;
}
#endif /* WALKIES */

int
movecmd(sym)	/* also sets u.dz, but returns false for <> */
char sym;
{
	register const char *dp;
	register const char *sdp = flags.num_pad ? ndir : sdir;

	u.dz = 0;
	if(!(dp = index(sdp, sym))) return 0;
	u.dx = xdir[dp-sdp];
	u.dy = ydir[dp-sdp];
	u.dz = zdir[dp-sdp];
#ifdef POLYSELF
	if (u.dx && u.dy && u.umonnum == PM_GRID_BUG) {
		u.dx = u.dy = 0;
		return 0;
	}
#endif
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
/*JP	    dirsym = yn_function (s ? s : "In what direction?", NULL, '\0');*/
	    dirsym = yn_function (s ? s : "どの方向？", NULL, '\0');
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
	register int x =
#ifdef POLYSELF
		(u.umonnum == PM_GRID_BUG) ? 2*rn2(4) :
#endif
							rn2(8);
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
	return (flags.num_pad ? ndir[x] : sdir[x]);
    } else {
	return (flags.num_pad ? M(ndir[x]) :
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

	if (!flags.num_pad || (foo = readchar()) == 'n')
	    for (;;) {
		foo = readchar();
		if (foo >= '0' && foo <= '9') {
		    multi = 10 * multi + foo - '0';
		    if (multi < 0 || multi > LARGEST_INT) multi = LARGEST_INT;
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
	    save_cm = NULL;
	}
	in_line[0] = foo;
	in_line[1] = '\0';
	if (foo == 'g' || foo == 'G' || (flags.num_pad && foo == '5') ||
	    foo == 'm' || foo == 'M') {
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
	if(!hu) {
	    hu = TRUE;
	    (void) dosave0();
	}
#endif
	clearlocks();
	terminate(0);
}
#endif

#endif /* OVLB */
#ifdef OVL0

char
readchar()
{
	register int sym;
	int x, y, mod;

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
