/*	SCCS Id: @(#)pray.c	3.1	93/04/24	*/
/* Copyright (c) Benson I. Margulies, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "epri.h"

STATIC_PTR int NDECL(prayer_done);
static int NDECL(in_trouble);
static void FDECL(fix_worst_trouble,(int));
static void FDECL(angrygods,(ALIGNTYP_P));
static void FDECL(pleased,(ALIGNTYP_P));
static void FDECL(godvoice,(ALIGNTYP_P,const char*));
static void FDECL(god_zaps_you,(ALIGNTYP_P));
static void FDECL(gods_angry,(ALIGNTYP_P));
static void FDECL(gods_upset,(ALIGNTYP_P));
static void FDECL(consume_offering,(struct obj *));
static boolean FDECL(water_prayer,(BOOLEAN_P));

/*
 * Logic behind deities and altars and such:
 * + prayers are made to your god if not on an altar, and to the altar's god
 *   if you are on an altar
 * + If possible, your god answers all prayers, which is why bad things happen
 *   if you try to pray on another god's altar
 * + sacrifices work basically the same way, but the other god may decide to
 *   accept your allegiance, after which they are your god.  If rejected,
 *   your god takes over with your punishment.
 * + if you're in Gehennom, all messages come from the chaotic god
 */
static
struct ghods {
	char	classlet;
	const char *law, *balance, *chaos;
}  gods[] = {
{'A', /* Central American */	"Quetzalcoatl", "Camaxtli", "Huhetotl"},
{'B', /* Hyborian */		"Mitra", "Crom", "Set"},
{'C', /* Babylonian */		"Anu", "Ishtar", "Anshar"},
{'E', /* Elven */		"Solonor Thelandira",
					"Aerdrie Faenya", "Erevan Ilesere"},
{'H', /* Greek */		"Athena", "Hermes", "Poseidon"},
{'K', /* Celtic */		"Lugh", "Brigit", "Macannan Mac Lir"},
{'P', /* Chinese */		"Shan Lai Ching", "Chih Sung-tzu", "Huan Ti"},
{'R', /* Nehwon */		"Issek", "Mog", "Kos"},
{'S', /* Japanese */		"Amaterasu Omikami", "Raiden", "Susanowo"},
#ifdef TOURIST
{'T', /* Discworld */		"Blind Io", "The Lady", "Offler"},
#endif
{'V', /* Norse */		"Tyr", "Odin", "Loki"},
{'W', /* Egyptian */		"Ptah", "Thoth", "Anhur"},
{0,0,0,0}
};

/*
 *	Moloch, who dwells in Gehennom, is the "renegade" cruel god
 *	responsible for the theft of the Amulet from Marduk, the Creator.
 *	Moloch is unaligned.
 */
static const char	*Moloch = "Moloch";

static const char *godvoices[] = {
/*JP    "booms out",
    "thunders",
    "rings out",
    "booms",*/
    "響きわたった",
    "雷のように響いた",
    "とどろいた",
    "響いた",
};

/* values calculated when prayer starts, and used when completed */
static aligntyp p_aligntyp;
static int p_trouble;
static int p_type; /* (-1)-3: (-1)=really naughty, 3=really good */

#define PIOUS 20
#define DEVOUT 14
#define FERVENT 9
#define STRIDENT 4

#define TROUBLE_STONED 10
#define TROUBLE_STRANGLED 9
#define TROUBLE_LAVA 8
#define TROUBLE_SICK 7
#define TROUBLE_STARVING 6
#define TROUBLE_HIT 5
#define TROUBLE_LYCANTHROPE 4
#define TROUBLE_STUCK_IN_WALL 3
#define TROUBLE_CURSED_BLINDFOLD 2
#define TROUBLE_CURSED_LEVITATION 1

#define TROUBLE_PUNISHED (-1)
#define TROUBLE_CURSED_ITEMS (-2)
#define TROUBLE_BLIND (-3)
#define TROUBLE_HUNGRY (-4)
#define TROUBLE_POISONED (-5)
#define TROUBLE_WOUNDED_LEGS (-6)
#define TROUBLE_STUNNED (-7)
#define TROUBLE_CONFUSED (-8)
#define TROUBLE_HALLUCINATION (-9)

/* We could force rehumanize of polyselfed people, but we can't tell
   unintentional shape changes from the other kind. Oh well. */

/* Return 0 if nothing particular seems wrong, positive numbers for
   serious trouble, and negative numbers for comparative annoyances. This
   returns the worst problem. There may be others, and the gods may fix
   more than one.

This could get as bizarre as noting surrounding opponents, (or hostile dogs),
but that's really hard.
 */

#define ugod_is_angry() (u.ualign.record < 0)
#define on_altar()	IS_ALTAR(levl[u.ux][u.uy].typ)
#define on_shrine()	((levl[u.ux][u.uy].altarmask & AM_SHRINE) != 0)
#define a_align(x,y)	((aligntyp)Amask2align(levl[x][y].altarmask & ~AM_SHRINE))

static int
in_trouble()
{
	register struct obj *otmp;
	int i, j, count=0;

/* Borrowed from eat.c */

#define	SATIATED	0
#define NOT_HUNGRY	1
#define	HUNGRY		2
#define	WEAK		3
#define	FAINTING	4
#define FAINTED		5
#define STARVED		6

	if(Stoned) return(TROUBLE_STONED);
	if(Strangled) return(TROUBLE_STRANGLED);
	if(u.utrap && u.utraptype == TT_LAVA) return(TROUBLE_LAVA);
	if(Sick) return(TROUBLE_SICK);
	if(u.uhs >= WEAK) return(TROUBLE_STARVING);
	if(u.uhp < 5 || (u.uhp*7 < u.uhpmax)) return(TROUBLE_HIT);
#ifdef POLYSELF
	if(u.ulycn >= 0) return(TROUBLE_LYCANTHROPE);
#endif
	for (i= -1; i<=1; i++) for(j= -1; j<=1; j++) {
		if (!i && !j) continue;
		if (!isok(u.ux+i, u.uy+j) || IS_ROCK(levl[u.ux+i][u.uy+j].typ))
			count++;
	}
	if(count==8
#ifdef POLYSELF
	    && !passes_walls(uasmon)
#endif
	    ) return(TROUBLE_STUCK_IN_WALL);
	if((uarmf && uarmf->otyp==LEVITATION_BOOTS && uarmf->cursed) ||
		(uleft && uleft->otyp==RIN_LEVITATION && uleft->cursed) ||
		(uright && uright->otyp==RIN_LEVITATION && uright->cursed))
		return(TROUBLE_CURSED_LEVITATION);
	if(ublindf && ublindf->cursed) return(TROUBLE_CURSED_BLINDFOLD);

	if(Punished) return(TROUBLE_PUNISHED);
	for(otmp=invent; otmp; otmp=otmp->nobj)
		if((otmp->otyp==LOADSTONE || otmp->otyp==LUCKSTONE) &&
			otmp->cursed)
		    return(TROUBLE_CURSED_ITEMS);
	if((uarmh && uarmh->cursed) ||	/* helmet */
	   (uarms && uarms->cursed) ||	/* shield */
	   (uarmg && uarmg->cursed) ||	/* gloves */
	   (uarm && uarm->cursed) ||	/* armor */
	   (uarmc && uarmc->cursed) ||	/* cloak */
	   (uarmf && uarmf->cursed && uarmf->otyp != LEVITATION_BOOTS) ||
					/* boots */
#ifdef TOURIST
	   (uarmu && uarmu->cursed) ||  /* shirt */
#endif
	   (welded(uwep)) ||
	   (uleft && uleft->cursed && uleft->otyp != RIN_LEVITATION) ||
	   (uright && uright->cursed && uright->otyp != RIN_LEVITATION) ||
	   (uamul && uamul->cursed))

	   return(TROUBLE_CURSED_ITEMS);

	if(Blinded > 1) return(TROUBLE_BLIND);
	if(u.uhs >= HUNGRY) return(TROUBLE_HUNGRY);
	for(i=0; i<A_MAX; i++)
	    if(ABASE(i) < AMAX(i)) return(TROUBLE_POISONED);
	if(Wounded_legs) return (TROUBLE_WOUNDED_LEGS);
	if(HStun) return (TROUBLE_STUNNED);
	if(HConfusion) return (TROUBLE_CONFUSED);
	if(Hallucination) return(TROUBLE_HALLUCINATION);

	return(0);
}

/*JP
const char leftglow[] = "left ring softly glows";
const char rightglow[] = "right ring softly glows";
*/
const char leftglow[] = "左の指輪";
const char rightglow[] = "右の指輪";

static void
fix_worst_trouble(trouble)
register int trouble;
{
	int i;
	struct obj *otmp;
	const char *what = NULL;

	u.ublesscnt += rnz(100);
	switch (trouble) {
	    case TROUBLE_STONED:
/*JP		    You("feel more limber.");*/
		    You("軟らかくなったのを感じた．");
		    Stoned = 0;
		    break;
	    case TROUBLE_STRANGLED:
/*JP		    You("can breathe again.");*/
		    You("呼吸できるようになった．");
		    Strangled = 0;
		    break;
	    case TROUBLE_LAVA:
/*JP		    You("are back on solid ground.");*/
		    You("固い地面に戻った．");
		    /* teleport should always succeed, but if not,
		     * just untrap them.
		     */
		    if(!safe_teleds())
			u.utrap = 0;
		    break;
	    case TROUBLE_STARVING:
		    losestr(-1);
		    /* fall into... */
	    case TROUBLE_HUNGRY:
/*JP		    Your("stomach feels content.");*/
		    Your("胃袋は満たされた．");
		    init_uhunger ();
		    flags.botl = 1;
		    break;
	    case TROUBLE_SICK:
/*JP		    You("feel better.");*/
		    You("気分が良くなった．");
		    make_sick(0L,FALSE);
		    break;
	    case TROUBLE_HIT:
/*JP		    You("feel much better.");*/
		    You("とても気分がよくなった．");
		    if (u.uhpmax < u.ulevel * 5 + 11)
			u.uhp = u.uhpmax += rnd(5);
		    else
			u.uhp = u.uhpmax;
		    flags.botl = 1;
		    break;
	    case TROUBLE_STUCK_IN_WALL:
/*JP		    Your("surroundings change.");*/
		    Your("環境が変化した．");
		    tele();
		    break;
	    case TROUBLE_CURSED_LEVITATION:
		    if (uarmf && uarmf->otyp==LEVITATION_BOOTS
						&& uarmf->cursed)
			otmp = uarmf;
		    else if (uleft && uleft->otyp==RIN_LEVITATION
						&& uleft->cursed) {
			otmp = uleft;
			what = leftglow;
		    } else {
			otmp = uright;
			what = rightglow;
		    }
		    goto decurse;
	    case TROUBLE_CURSED_BLINDFOLD:
		    otmp = ublindf;
		    goto decurse;
	    case TROUBLE_PUNISHED:
/*JP		    Your("chain disappears.");*/
		    Your("鎖は消えた．");
		    unpunish();
		    break;
#ifdef POLYSELF
	    case TROUBLE_LYCANTHROPE:
/*JP		    You("feel purified.");*/
		    You("清められたような気がした．");
		    if(uasmon == &mons[u.ulycn] && !Polymorph_control)
			rehumanize();
		    u.ulycn = -1;       /* now remove the curse */
		    break;
#endif
	    case TROUBLE_CURSED_ITEMS:
		    if (uarmh && uarmh->cursed)		/* helmet */
			    otmp = uarmh;
		    else if (uarms && uarms->cursed)	/* shield */
			    otmp = uarms;
		    else if (uarmg && uarmg->cursed)	/* gloves */
			    otmp = uarmg;
		    else if (uarm && uarm->cursed)	/* armor */
			    otmp = uarm;
		    else if (uarmc && uarmc->cursed)	/* cloak */
			    otmp = uarmc;
		    else if (uarmf && uarmf->cursed)	/* boots */
			    otmp = uarmf;
#ifdef TOURIST
		    else if (uarmu && uarmu->cursed)	/* shirt */
			    otmp = uarmu;
#endif
		    else if (uleft && uleft->cursed) {
			    otmp = uleft;
			    what = leftglow;
		    } else if (uright && uright->cursed) {
			    otmp = uright;
			    what = rightglow;
		    } else if (uamul && uamul->cursed) /* amulet */
			    otmp = uamul;
		    else if (welded(uwep)) otmp = uwep;
		    else {
			    for(otmp=invent; otmp; otmp=otmp->nobj)
				if ((otmp->otyp==LOADSTONE ||
				     otmp->otyp==LUCKSTONE) && otmp->cursed)
					break;
		    }
decurse:
		    uncurse(otmp);
		    otmp->bknown = TRUE;
		    if (!Blind)
/*JP			    Your("%s %s.",
				 what ? what :
				 (const char *)aobjnam (otmp, "softly glow"),
				 Hallucination ? hcolor() : amber);*/
			    Your("%sは%sうっすらと輝いた．",
				 what ? what :
				 (const char *)xname (otmp),
				 jconj_adj(Hallucination ? hcolor() : amber));
		    break;
	    case TROUBLE_HALLUCINATION:
/*JP		    pline ("Looks like you are back in Kansas.");*/
		    pline ("カンサスに戻ってきたような気がした．");
		    make_hallucinated(0L,FALSE,0L);
		    break;
	    case TROUBLE_BLIND:
/*JP		    Your("%s feel better.", makeplural(body_part(EYE)));*/
		    Your("%sは回復した．", makeplural(body_part(EYE)));
		    make_blinded(0L,FALSE);
		    break;
	    case TROUBLE_POISONED:
		    if (Hallucination)
/*JP			pline("There's a tiger in your tank.");*/
			pline("あなたのタンクの中に虎がいる．");
		    else
/*JP			You("feel in good health again.");*/
			You("また健康になったような気がした．");
		    for(i=0; i<A_MAX; i++) {
			if(ABASE(i) < AMAX(i)) {
				ABASE(i) = AMAX(i);
				flags.botl = 1;
			}
		    }
		    break;
	    case TROUBLE_WOUNDED_LEGS:
		    heal_legs();
		    break;
	    case TROUBLE_STUNNED:
		    make_stunned(0L,TRUE);
		    break;
	    case TROUBLE_CONFUSED:
		    make_confused(0L,TRUE);
		    break;
	}
}

static void
god_zaps_you(resp_god)
aligntyp resp_god;
{
/*JP	pline("Suddenly, a bolt of lightning strikes you!");*/
	pline("突然，稲妻があなたに命中した！");
	if (Reflecting) {
	    shieldeff(u.ux, u.uy);
	    if (Blind)
/*JP		pline("For some reason you're unaffected.");*/
		pline("何らかの理由であなたは影響を受けない．");
	    else {
		if (Reflecting & W_AMUL) {
/*JP		    pline("It reflects from your medallion.");*/
		    pline("それはあなたのメダリオンによって反射された．");
		    makeknown(AMULET_OF_REFLECTION);
		} else {
/*JP		    pline("It reflects from your shield.");*/
		    pline("それはあなたの盾によって反射された．");
		    makeknown(SHIELD_OF_REFLECTION);
		}
	    }
	    goto ohno;
	} else if (Shock_resistance) {
	    shieldeff(u.ux, u.uy);
/*JP	    pline("It seems not to affect you.");*/
	    pline("それはあなたに影響を与えないようだ．");
ohno:
/*JP	    pline("%s is not deterred...", align_gname(resp_god));
	    pline("A wide-angle disintegration beam hits you!");*/
	    pline("%sはやめなかった．．．", align_gname(resp_god));
	    pline("粉砕の光線があなたに命中した！");
	    if (Disint_resistance) {
/*JP		You("bask in its %s glow for a minute...", Black);*/
		You("一瞬，その%s輝きで暖まった．．．", Black);
/*JP		godvoice(resp_god, "I believe it not!");*/
		godvoice(resp_god, "信じられぬ！");
		if(Is_astralevel(&u.uz)) {

		    /* one more try on the astral level */
/*JP		    verbalize("Thou cannot escape my wrath, mortal!");*/
		    verbalize("汝我が怒りから逃がれることならん，人間よ！");
		    summon_minion(resp_god, FALSE);
		    summon_minion(resp_god, FALSE);
		    summon_minion(resp_god, FALSE);
/*JP		    verbalize("Destroy %s, my servants!", him[flags.female]);*/
		    verbalize("%sを殺せ！，わが下僕よ！", him[flags.female]);
		}
		return;
	    }
	}
	{
	    char killerbuf[64];
/*JP	    You("fry to a crisp.");*/
	    You("パリパリになった．");
	    killer_format = KILLED_BY;
	    Sprintf(killerbuf, "%sの怒りに触れ", align_gname(resp_god));
	    killer = killerbuf;
	    done(DIED2);
	}
}

static void
angrygods(resp_god)
aligntyp resp_god;
{
	register int	maxanger;

	if(Inhell) resp_god = A_NONE;
	u.ublessed = 0;

	/* changed from tmp = u.ugangr + abs (u.uluck) -- rph */
	/* added test for alignment diff -dlc */
	if(resp_god != u.ualign.type)
	    maxanger =  u.ualign.record/2 + (Luck > 0 ? -Luck/3 : -Luck);
	else
	    maxanger =  3*u.ugangr +
		((Luck > 0 || u.ualign.record >= STRIDENT) ? -Luck/3 : -Luck);
	if (maxanger < 0) maxanger = 0; /* possible if bad align & good luck */
	maxanger =  (maxanger > 15 ? 15 : maxanger);  /* be reasonable */
	switch (maxanger ? rn2(maxanger): 0) {

	    case 0:
/*JP	    case 1:	You("feel that %s is %s.", align_gname(resp_god),
			    Hallucination ? "bummed" : "displeased");*/
	    case 1:	You("%sが%sいるのを感じた．", align_gname(resp_god),
			    Hallucination ? "ねだって" : "立腹して");
			break;
	    case 2:
	    case 3:
			godvoice(resp_god,NULL);
# ifdef POLYSELF
/*JP			pline("\"Thou %s, %s.\"",
			      ugod_is_angry() ? "hast strayed from the path" :
						"art arrogant",
			      u.usym == S_HUMAN ? "mortal" : "creature");*/
			pline("「汝%s，%s．」",
			      ugod_is_angry() ? "その道から踏み出ておる":
						"傲慢なり",
			      u.usym == S_HUMAN ? "人間よ" : "生物よ");
# else
/*JP			pline("\"Thou %s, mortal.\"",
			      ugod_is_angry() ? "hast strayed from the path" :
						"art arrogant");*/
			pline("「汝%s，人間よ．」"
			      ugod_is_angry() ? "その道から踏み出ておる":
						"傲慢なり");
# endif
/*JP			verbalize("Thou must relearn thy lessons!");*/
			verbalize("汝もう一度学ぶべし！");
			(void) adjattrib(A_WIS, -1, FALSE);
			if (u.ulevel > 1) {
			    losexp();
			    if(u.uhp < 1) u.uhp = 1;
			    if(u.uhpmax < 1) u.uhpmax = 1;
			} else  {
			    u.uexp = 0;
			    flags.botl = 1;
			}
			break;
	    case 6:	if (!Punished) {
			    gods_angry(resp_god);
			    punish((struct obj *)0);
			    break;
			} /* else fall thru */
	    case 4:
	    case 5:	gods_angry(resp_god);
			if (!Blind && !Antimagic)
/*JP			    pline("%s glow surrounds you.",
				  An(Hallucination ? hcolor() : Black));*/
			    pline("%s光があなたを取り巻いた．",
				  An(Hallucination ? hcolor() : Black));
			rndcurse();
			break;
	    case 7:
	    case 8:	godvoice(resp_god,NULL);
/*JP			verbalize("Thou durst %s me?",
				  (on_altar() &&
				   (a_align(u.ux,u.uy) != resp_god)) ?
				  "scorn":"call upon");*/
			verbalize("汝，我%s？",
				  (on_altar() &&
				   (a_align(u.ux,u.uy) != resp_god)) ?
				  "をさげすむか？":"祈りを求めしか？");
# ifdef POLYSELF
/*JP			pline("\"Then die, %s!\"",
			      u.usym == S_HUMAN ? "mortal" : "creature");*/
			pline("「死ね，%s！」",
			      u.usym == S_HUMAN ? "人間よ" : "生物よ");
# else
/*JP			verbalize("Then die, mortal!");*/
			verbalize("死ね！，人間よ！");
# endif

			summon_minion(resp_god, FALSE);
			break;

	    default:	gods_angry(resp_god);
			god_zaps_you(resp_god);
			break;
	}
	u.ublesscnt = rnz(300);
	return;
}

static void
pleased(g_align)
	aligntyp g_align;
{
	int trouble = p_trouble;	/* what's your worst difficulty? */
	int pat_on_head = 0;

/*JP	You("feel that %s is %s.", align_gname(g_align),
	    u.ualign.record >= DEVOUT ?
	    Hallucination ? "pleased as punch" : "well-pleased" :
	    u.ualign.record >= STRIDENT ?
	    Hallucination ? "ticklish" : "pleased" :
	    Hallucination ? "full" : "satisfied");*/
	pline("%sが%sような気がした．", align_gname(g_align),
	    u.ualign.record >= DEVOUT ?
	    Hallucination ? "くそ機嫌いい" : "ご機嫌麗しい" :
	    u.ualign.record >= STRIDENT ?
	    Hallucination ? "くすぐったがっている" : "上機嫌である" :
	    Hallucination ? "腹いっぱいである" : "満足している");

	/* not your deity */
	if (on_altar() && p_aligntyp != u.ualign.type) {
		adjalign(-1);
		return;
	} else if (u.ualign.record < 2) adjalign(1);

	/* depending on your luck & align level, the god you prayed to will:
	   - fix your worst problem if it's major.
	   - fix all your major problems.
	   - fix your worst problem if it's minor.
	   - fix all of your problems.
	   - do you a gratuitous favor.

	   if you make it to the the last category, you roll randomly again
	   to see what they do for you.

	   If your luck is at least 0, then you are guaranteed rescued
	   from your worst major problem. */

	if (!trouble && u.ualign.record >= DEVOUT) pat_on_head = 1;
	else {
	    int action = rn1(on_altar() ? 3 + on_shrine() : 2, Luck+1);

	    if (!on_altar()) action = max(action,2);
	    if (u.ualign.record < STRIDENT)
		action = (u.ualign.record > 0 || !rnl(2)) ? 1 : 0;

	    switch(min(action,5)) {
	    case 5: pat_on_head = 1;
	    case 4: do fix_worst_trouble(trouble);
		    while ((trouble = in_trouble()) != 0);
		    break;

	    case 3: fix_worst_trouble(trouble);
	    case 2: while ((trouble = in_trouble()) > 0)
		    fix_worst_trouble(trouble);
		    break;

	    case 1: if (trouble > 0) fix_worst_trouble(trouble);
	    case 0: break; /* your god blows you off, too bad */
	    }
	}

    if(pat_on_head)
	switch(rn2((Luck + 6)>>1)) {
	case 0:	break;
	case 1:
	    if (uwep && (welded(uwep) || uwep->oclass == WEAPON_CLASS ||
			 uwep->otyp == PICK_AXE || uwep->otyp == UNICORN_HORN)
				&& (!uwep->blessed)) {
		if (uwep->cursed) {
		    uwep->cursed = FALSE;
		    uwep->bknown = TRUE;
		    if (!Blind)
/*JP			Your("%s %s.", aobjnam(uwep, "softly glow"),
			     Hallucination ? hcolor() : amber);*/
			Your("%sは%sうっすらと輝いた．", xname(uwep), 
			     jconj_adj(Hallucination ? hcolor() : amber));
/*JP		    else You("feel the power of %s over your %s.",
			u_gname(), xname(uwep));*/
		    else pline("%sの力が%sに注がれているのを感じた．",
			u_gname(), xname(uwep));
		} else if(uwep->otyp < BOW || uwep->otyp > CROSSBOW) {
		    uwep->blessed = uwep->bknown = TRUE;
		    if (!Blind)
/*JP			Your("%s with %s aura.",
			     aobjnam(uwep, "softly glow"),
			     an(Hallucination ? hcolor() : light_blue));*/
			Your("%sは%sぼんやりしたオーラにつつまれた．",
			     xname(uwep), 
			     Hallucination ? hcolor() : light_blue);
/*JP		    else You("feel the blessing of %s over your %s.",*/
		    else pline("%sの祝福が%sに注がれているのを感じた．",
			u_gname(), xname(uwep));
		}
	    }
	    break;
	case 3:
	    /* takes 2 hints to get the music to enter the stronghold */
	    if (flags.soundok && !u.uevent.uopened_dbridge) {
		if(u.uevent.uheard_tune < 1) {
		    godvoice(g_align,NULL);
#ifdef POLYSELF
/*JP		    verbalize("Hark, %s!",
			  u.usym == S_HUMAN ? "mortal" : "creature");*/
		    verbalize("聞け，%s！",
			  u.usym == S_HUMAN ? "人間よ" : "生物よ");
#else
/*JP		    verbalize("Hark, mortal!");*/
		    verbalize("聞け，人間よ！");
#endif
		    verbalize(
/*JP			"To enter the castle, thou must play the right tune!");*/
			"汝城に入らんとするなら，正しき旋律で演奏せねばならぬ！");
		    u.uevent.uheard_tune++;
		    break;
		} else if (u.uevent.uheard_tune < 2) {
/*JP		    You(Hallucination ? "hear a funeral march..." : "hear a divine music...");
		    pline("It sounds like:  \"%s\".", tune);*/
		    You(Hallucination ? "葬式の行進曲を聞いた．．．" : "神の音楽を聞いた．．．");
		    pline("それは次のように聞こえた:  「%s」", tune);
		    u.uevent.uheard_tune++;
		    break;
		}
	    }
	    /* Otherwise, falls into next case */
	case 2:
	    if (!Blind)
/*JP		You("are surrounded by %s glow.",
		    an(Hallucination ? hcolor() : golden));*/
		You("%s輝きにつつまれた．",
		    an(Hallucination ? hcolor() : golden));
	    u.uhp = u.uhpmax += 5;
	    ABASE(A_STR) = AMAX(A_STR);
	    if (u.uhunger < 900)	init_uhunger();
	    if (u.uluck < 0)	u.uluck = 0;
	    make_blinded(0L,TRUE);
	    flags.botl = 1;
	    break;
	case 4: {
	    register struct obj *otmp;

	    if (Blind)
/*JP		You("feel the power of %s.", u_gname());*/
		You("%sの力を感じた．", u_gname());
/*JP	    else You("are surrounded by %s aura.",*/
	    else You("%sオーラにつつまれた．",
		     an(Hallucination ? hcolor() : light_blue));
	    for(otmp=invent; otmp; otmp=otmp->nobj) {
		if (otmp->cursed) {
		    uncurse(otmp);
		    if (!Blind) {
/*JP			Your("%s %s.", aobjnam(otmp, "softly glow"),*/
			Your("%sは%sうっすらと輝いた．", xname(otmp),
			     jconj_adj(Hallucination ? hcolor() : amber));
			otmp->bknown = TRUE;
		    }
		}
	    }
	    break;
	}
	case 5: {
/*JP	    const char *msg="\"and thus I grant thee the gift of %s!\"";*/
	    const char *msg="「さらに汝に%sをさずけよう！」";
/*JP	    godvoice(u.ualign.type, "Thou hast pleased me with thy progress,");*/
	    godvoice(u.ualign.type, "汝の成長は非常に望ましい，");
	    if (!(HTelepat & INTRINSIC))  {
		HTelepat |= FROMOUTSIDE;
/*JP		pline(msg, "Telepathy");*/
		pline(msg, "テレパシー");
		if (Blind) see_monsters();
	    } else if (!(Fast & INTRINSIC))  {
		Fast |= FROMOUTSIDE;
/*JP		pline(msg, "Speed");*/
		pline(msg, "速さ");
	    } else if (!(Stealth & INTRINSIC))  {
		Stealth |= FROMOUTSIDE;
/*JP		pline(msg, "Stealth");*/
		pline(msg, "忍の力");
	    } else {
		if (!(Protection & INTRINSIC))  {
		    Protection |= FROMOUTSIDE;
		    if (!u.ublessed)  u.ublessed = rn1(3, 2);
		} else u.ublessed++;
/*JP		pline(msg, "my protection");*/
		pline(msg, "我が護り");
	    }
/*JP	    verbalize("Use it wisely in my name!");*/
	    verbalize("我名に於いて有効に使うがよい！");
	    break;
	}
	case 7:
	case 8:
#ifdef ELBERETH
	    if (u.ualign.record >= PIOUS && !u.uevent.uhand_of_elbereth) {
		register struct obj *obj = uwep;	/* to be blessed */
		boolean already_exists, in_hand;

		HSee_invisible |= FROMOUTSIDE;
		HFire_resistance |= FROMOUTSIDE;
		HCold_resistance |= FROMOUTSIDE;
		HPoison_resistance |= FROMOUTSIDE;
		godvoice(u.ualign.type,NULL);

		switch(u.ualign.type) {
		case A_LAWFUL:
		    u.uevent.uhand_of_elbereth = 1;
/*JP		    verbalize("I crown thee...      The Hand of Elbereth!");*/
		    verbalize("汝にさずけよう．．．     エルベレスの御手を！");
		    if (obj && (obj->otyp == LONG_SWORD) && !obj->oartifact)
			obj = oname(obj, artiname(ART_EXCALIBUR), 1);
		    break;
		case A_NEUTRAL:
		    u.uevent.uhand_of_elbereth = 2;
/*JP		    verbalize("Thou shalt be my Envoy of Balance!");*/
		    verbalize("汝，我が調和の使者なり！");
		    if (uwep && uwep->oartifact == ART_VORPAL_BLADE) {
			obj = uwep;	/* to be blessed and rustproofed */
/*JP			Your("%s goes snicker-snack!", xname(obj));*/
			Your("%sは軽くなった！", xname(obj));
			obj->dknown = TRUE;
		    } else if (!exist_artifact(LONG_SWORD,
						artiname(ART_VORPAL_BLADE))) {
		        obj = mksobj(LONG_SWORD, FALSE, FALSE);
			obj = oname(obj, artiname(ART_VORPAL_BLADE), 0);
/*JP		        pline("%s %s %s your %s!", Blind ? "Something" : "A",
			      Blind ? "lands" : "sword appears",
			      Levitation ? "beneath" : "at",
			      makeplural(body_part(FOOT)));*/
		        pline("%sがあなたの%s%sに%s！", Blind ? "何か" : "剣が",
			      body_part(FOOT),
			      Levitation ? "の下方" : "元",
			      Blind ? "着地した" : "現われた");
			obj->spe = 1;
			dropy(obj);
		    }
		    break;
		case A_CHAOTIC:
		    /* This does the same damage as Excalibur.
		     * Disadvantages: doesn't do bonuses to undead;
		     *   doesn't aid searching.
		     * Advantage: part of that bonus is a level drain.
		     * Disadvantage: player cannot start with a +5 weapon and
		     * turn it into a Stormbringer.
		     * Advantage: they don't need to already have a sword of
		     *   the right type to get it...
		     * However, if Stormbringer already exists in the game, an
		     * ordinary good broadsword is given and the messages are
		     * a bit different.
		     */
		    u.uevent.uhand_of_elbereth = 3;
		    in_hand = (uwep && uwep->oartifact == ART_STORMBRINGER);
		    already_exists = exist_artifact(RUNESWORD,
						artiname(ART_STORMBRINGER));
/*JP		    verbalize("Thou art chosen to %s for My Glory!",
			      already_exists && !in_hand ?
			      "take lives" : "steal souls");*/
		    verbalize("汝，我が栄光のため%sとして選ばれん！",
			      already_exists && !in_hand ?
			      "生きながらえん者" : "魂を奪いしためる者");
		    if (in_hand) {
			obj = uwep;	/* to be blessed and rustproofed */
		    } else if (!already_exists) {
		        obj = mksobj(RUNESWORD, FALSE, FALSE);
			obj = oname(obj, artiname(ART_STORMBRINGER), 0);
/*JP		        pline("%s %s %s your %s!", Blind ? "Something" :
			      An(Hallucination ? hcolor() : Black),
			      Blind ? "lands" : "sword appears",
			      Levitation ? "beneath" : "at",
			      makeplural(body_part(FOOT)));*/
		        pline("%s%sがあなたの%s%sに%s！", Blind ? "何か" :
			      An(Hallucination ? hcolor() : Black),
			      Blind ? "" : "剣が",
			      makeplural(body_part(FOOT)),
			      Levitation ? "の下方に" : "元",
			      Blind ? "着地した" : "現われた");
			obj->spe = 1;
			dropy(obj);
		    }
		    break;
		default:
		    obj = 0;	/* lint */
		    break;
		}
		/* enhance weapon regardless of alignment or artifact status */
		if (obj && obj->oclass == WEAPON_CLASS) {
		    bless(obj);
		    obj->oeroded = 0;
		    obj->oerodeproof = TRUE;
		    obj->bknown = obj->rknown = TRUE;
		    if (obj->spe < 1) obj->spe = 1;
		} else	/* opportunity knocked, but there was nobody home... */
/*JP		    You("feel unworthy.");*/
		    You("無駄だと感じた．");
		break;
	    }
#endif

/*JP	case 6:	pline ("An object appears at your %s!",*/
	case 6:	pline ("物体があなたの%s元に現われた！",
		       makeplural(body_part(FOOT)));
	    bless(mkobj_at(SPBOOK_CLASS, u.ux, u.uy, TRUE));
	    break;

	default:	impossible("Confused deity!");
	    break;
	}
	u.ublesscnt = rnz(350);
#ifndef ELBERETH
	u.ublesscnt += (u.uevent.udemigod * rnz(1000));
#else
	u.ublesscnt += ((u.uevent.udemigod + u.uevent.uhand_of_elbereth)
							* rnz(1000));
#endif
	return;
}

/* either blesses or curses water on the altar,
 * returns true if it found any water here.
 */
static boolean
water_prayer(bless_water)
    boolean bless_water;
{
    register struct obj* otmp;
    register long changed = 0;
    boolean other = FALSE;

    for(otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
	/* turn water into (un)holy water */
	if (otmp->otyp == POT_WATER) {
	    otmp->blessed = bless_water;
	    otmp->cursed = !bless_water;
	    otmp->bknown = !Blind;
	    changed += otmp->quan;
	} else if(otmp->oclass == POTION_CLASS)
	    other = TRUE;
    }
    if(!Blind && changed) {
/*JP	pline("%s potion%s on the altar glow%s %s for a moment.",
	      ((other && changed > 1L) ? "Some of the" : (other ? "A" : "The")),
	      (changed > 1L ? "s" : ""), (changed > 1L ? "" : "s"),
	      (bless_water ? amber : Black));*/
	pline("%s祭壇の薬は一瞬%s輝いた．",
	      (other && changed > 1L) ? "いくつかの" : "",
	      jconj_adj(bless_water ? amber : Black));
    }
    return((boolean)(changed > 0L));
}

static void
godvoice(g_align, words)
    aligntyp g_align;
    const char *words;
{
    const char *quot = "";
/*JP*/
    const char *quot2 = "";
    if(words)
/*JP	quot = "\"";*/
      {
	quot = "「";
	quot2 = "」";
      }
    else
	words = "";

/*JP
    pline("The voice of %s %s: %s%s%s", align_gname(g_align),
	  godvoices[rn2(SIZE(godvoices))], quot, words, quot);*/
    pline("%sの声が%s: %s%s%s", align_gname(g_align),
	  godvoices[rn2(SIZE(godvoices))], quot, words, quot2);
}

static void
gods_angry(g_align)
    aligntyp g_align;
{
/*JP    godvoice(g_align, "Thou hast angered me.");*/
    godvoice(g_align, "汝，我を怒りしめた．");
}

/* The g_align god is upset with you. */
static void
gods_upset(g_align)
	aligntyp g_align;
{
	if(g_align == u.ualign.type) u.ugangr++;
	else if(u.ugangr) u.ugangr--;
	angrygods(g_align);
}

static NEARDATA const char sacrifice_types[] = { FOOD_CLASS, AMULET_CLASS, 0 };

static void
consume_offering(otmp)
register struct obj *otmp;
{
    if (Hallucination)
	switch (rn2(3)) {
	    case 0:
/*JP		Your("sacrifice sprouts wings and a propeller and roars away!");*/
		Your("献上物は羽をはやし，プロペラがまわり，飛んでった！");
		break;
	    case 1:
/*JP		Your("sacrifice puffs up, swelling bigger and bigger, and pops!");*/
		Your("献上物は噴煙をあげ，どんどん膨れ，そしてはじけた！");
		break;
	    case 2:
/*JP		Your("sacrifice collapses into a cloud of dancing particles and fades away!");*/
		Your("献上物は細かく砕け，踊り出し，どこかに行ってしまった！");
		break;
	}
    else if (Blind && u.ualign.type == A_LAWFUL)
/*JP	Your("sacrifice disappears!");*/
	Your("献上物は消えた！");
/*JP    else Your("sacrifice is consumed in a %s!",
	      u.ualign.type == A_LAWFUL ? "flash of light" : "burst of flame");*/
    else Your("献上物は%s消えさった！",
	      u.ualign.type == A_LAWFUL ? "まばゆい光を放ち" : "炎を上げ");
    if (carried(otmp)) useup(otmp);
    else useupf(otmp);
    exercise(A_WIS, TRUE);
}

int
dosacrifice()
{
    register struct obj *otmp;
    int value = 0;
    aligntyp altaralign = a_align(u.ux,u.uy);

    if (!on_altar()) {
/*JP	You("are not standing on an altar.");*/
	You("祭壇の上に立っていない．");
	return 0;
    }

    if (In_endgame(&u.uz)) {
/*JP	if (!(otmp = getobj(sacrifice_types, "sacrifice"))) return 0;*/
	if (!(otmp = getobj(sacrifice_types, "捧げる"))) return 0;
    } else
/*JP	if (!(otmp = floorfood("sacrifice", 0))) return 0;*/
	if (!(otmp = floorfood("捧げる", 0))) return 0;

    /*
      Was based on nutritional value and aging behavior (< 50 moves).
      Sacrificing a food ration got you max luck instantly, making the
      gods as easy to please as an angry dog!

      Now only accepts corpses, based on the games evaluation of their
      toughness.  Human sacrifice, as well as sacrificing unicorns of
      your alignment, is strongly discouraged.  (We can't tell whether
      a pet corpse was tame, so you can still sacrifice it.)
     */

#define MAXVALUE 24 /* Highest corpse value (besides Wiz) */

    if (otmp->otyp == CORPSE) {
	register struct permonst *ptr = &mons[otmp->corpsenm];
	extern int monstr[];

	if (otmp->corpsenm == PM_ACID_BLOB || (monstermoves <= otmp->age + 50))
	    value = monstr[otmp->corpsenm] + 1;
	if (otmp->oeaten)
	    value = eaten_stat(value, otmp);

	if ((pl_character[0]=='E') ? is_elf(ptr) : is_human(ptr)) {
#ifdef POLYSELF
	    if (is_demon(uasmon)) {
/*JP		You("find the idea very satisfying.");*/
		You("その考えは十分満足できるものだとわかった．");
		exercise(A_WIS, TRUE);
	    } else
#endif
		if (u.ualign.type != A_CHAOTIC) {
/*JP		    pline("You'll regret this infamous offense!");*/
		    pline("汝、この侮辱の行ないを後悔するべし！");
		    exercise(A_WIS, FALSE);
		}

	    if (altaralign != A_CHAOTIC && altaralign != A_NONE) {
		/* curse the lawful/neutral altar */
/*JP		pline("The altar is stained with %sn blood.",
		      (pl_character[0]=='E') ? "elve" : "huma");*/
		pline("祭壇は%sの血で汚れている．",
		      (pl_character[0]=='E') ? "エルフ" : "人");
		if(!Is_astralevel(&u.uz))
		    levl[u.ux][u.uy].altarmask = AM_CHAOTIC;
		angry_priest();
	    } else {
		register struct monst *dmon;
		/* Human sacrifice on a chaotic or unaligned altar */
		/* is equivalent to demon summoning */
		if(u.ualign.type != A_CHAOTIC) {
/*JP		pline("The blood floods the altar, which vanishes in %s cloud!",
			  an(Hallucination ? hcolor() : Black));*/
		pline("血が祭壇から溢れ，祭壇は%s雲となり消えた！",
			  an(Hallucination ? hcolor() : Black));
		    levl[u.ux][u.uy].typ = ROOM;
		    levl[u.ux][u.uy].altarmask = 0;
		    if(Invisible) newsym(u.ux, u.uy);
		} else {
/*JP		    pline("The blood covers the altar!");*/
		    pline("血は祭壇を覆った！");
		    change_luck(2);
		}
		if ((dmon = makemon(&mons[dlord(altaralign)], u.ux, u.uy))) {
/*JP		    You("have summoned %s!", a_monnam(dmon));*/
		    You("%sを召喚した！", a_monnam(dmon));
		    if (sgn(u.ualign.type) == sgn(dmon->data->maligntyp))
			dmon->mpeaceful = TRUE;
/*JP		    You("are terrified, and unable to move.");*/
		    You("恐くなり，動けなくなった．");
		    nomul(-3);
/*JP		} else pline("The cloud dissipates.");*/
		} else pline("雲は飛び散った．");
	    }

	    if (u.ualign.type != A_CHAOTIC) {
		adjalign(-5);
		u.ugangr += 3;
		(void) adjattrib(A_WIS, -1, TRUE);
		if (!Inhell) angrygods(u.ualign.type);
		change_luck(-5);
	    } else adjalign(5);
	    if (carried(otmp)) useup(otmp);
	    else useupf(otmp);
	    return(1);
	} else if (is_undead(ptr)) { /* Not demons--no demon corpses */
	    if (u.ualign.type != A_CHAOTIC)
		value += 1;
	} else if (ptr->mlet == S_UNICORN) {
	    int unicalign = sgn(ptr->maligntyp);

	    /* If same as altar, always a very bad action. */
	    if (unicalign == altaralign) {
/*JP		pline("Such an action is an insult to %s!",
		      (unicalign == A_CHAOTIC)
		      ? "chaos" : unicalign ? "law" : "balance");*/
		pline("そのような行動は『%s』に反する！",
		      (unicalign == A_CHAOTIC)
		      ? "混沌" : unicalign ? "秩序" : "調和");
		(void) adjattrib(A_WIS, -1, TRUE);
		value = -5;
	    } else if (u.ualign.type == altaralign) {
		/* If different from altar, and altar is same as yours, */
		/* it's a very good action */
		if (u.ualign.record < ALIGNLIM)
/*JP		    You("feel appropriately %s.", align_str(u.ualign.type));*/
		    You("%sにふさわしいと感じた．", align_str(u.ualign.type));
/*JP		else You("feel you are thoroughly on the right path.");*/
		else You("完全に正しい道を歩んでいるのを感じた．");
		adjalign(5);
		value += 3;
	    } else
		/* If sacrificing unicorn of your alignment to altar not of */
		/* your alignment, your god gets angry and it's a conversion */
		if (unicalign == u.ualign.type) {
		    u.ualign.record = -1;
		    value = 1;
		} else value += 3;
	}
    }

    if (otmp->otyp == AMULET_OF_YENDOR) {
	if (!In_endgame(&u.uz)) {
	    if (Hallucination)
/*JP		    You("feel homesick.");*/
		    You("故郷が恋しくなった．");
	    else
/*JP		    You("feel an urge to return to the surface.");*/
		    You("地上に帰りたい気持に駆り立てられた．");
	    return 1;
	} else {
	    /* The final Test.	Did you win? */
	    if(uamul == otmp) Amulet_off();
	    u.uevent.ascended = 1;
	    if(carried(otmp)) useup(otmp); /* well, it's gone now */
	    else useupf(otmp);
/*JP	    You("offer the Amulet of Yendor to %s...", a_gname());*/
	    You("イェンダーの魔除けを%sに献上した．．．",a_gname());
	    if (u.ualign.type != altaralign) {
		/* And the opposing team picks you up and
		   carries you off on their shoulders */
		adjalign(-99);
/*JP		pline("%s accepts your gift, and gains dominion over %s...",*/
		pline("%sはあなたの送り物を受けとり，%sの権力を得た．．．",
		      a_gname(), u_gname());
/*JP		pline("%s is enraged...", u_gname());*/
		pline("%sは大激怒した．．．", u_gname());
/*JP		pline("Fortunately, %s permits you to live...", a_gname());*/
		pline("幸運にも，%sはあなたの存在を許している．．．",a_gname());
/*JP		pline("A cloud of %s smoke surrounds you...",*/
		pline("%s煙があなたを取り囲んだ．．．",
/*JP		      Hallucination ? hcolor() : (const char *)"orange");*/
		      Hallucination ? hcolor() : (const char *)"オレンジ色の");
		done(ESCAPED);
	    } else { /* super big win */
		adjalign(10);
/*JPpline("An invisible choir sings, and you are bathed in radiance...");*/
pline("どこからともなく聖歌隊の歌が聞こえ，あなたは光に包まれた．．．");
/*JP		godvoice(altaralign, "Congratulations, mortal!");*/
		godvoice(altaralign, "よくやった，人間よ！");
		display_nhwindow(WIN_MESSAGE, FALSE);
/*JPverbalize("In return for thy service, I grant thee the gift of Immortality!");*/
verbalize("汝の偉業に対し，不死の体を捧げようぞ！");
/*JP		You("ascend to the status of Demigod%s...",
		    flags.female ? "dess" : "");*/
		You("昇天し，%s神となった．．．",
		    flags.female ? "女" : "");
		done(ASCENDED);
	    }
	}
    }

    if (otmp->otyp == FAKE_AMULET_OF_YENDOR) {
	    if (flags.soundok)
/*JP		You("hear a nearby thunderclap.");*/
		You("近くに雷が落ちた音を聞いた．");
	    if (!otmp->known) {
/*JP		You("realize you have made a %s.",
		    Hallucination ? "boo-boo" : "mistake");*/
		You("%sを犯したことに気がついた．",
		    Hallucination ? "ブーブー" : "間違い");
		otmp->known = TRUE;
		change_luck(-1);
		return 1;
	    } else {
		/* don't you dare try to fool the gods */
		change_luck(-3);
		adjalign(-1);
		u.ugangr += 3;
		value = -3;
	    }
    }

    if (value == 0) {
	pline(nothing_happens);
	return (1);
    }

    if(Is_astralevel(&u.uz) && (altaralign != u.ualign.type)) {
	/*
	 * REAL BAD NEWS!!! High altars cannot be converted.  Even an attempt
	 * gets the god who owns it truely pissed off.
	 */
/*JP	You("feel the air around you grow charged...");*/
	You("回りの空気にエネルギーが満ちていくような気がした．．．");
/*JP	pline("Suddenly, you realize that %s has noticed you...", a_gname());*/
	pline("突然，%sがあなたをじっと見ているのに気がついた．．．",a_gname());
/*JP	godvoice(altaralign, "So, mortal!  You dare desecrate my High Temple!");*/
	godvoice(altaralign, "人間よ！おまえは我が神聖なる寺院を汚すのか！");
	/* Throw everything we have at the player */
	god_zaps_you(altaralign);
    } else if (value < 0) /* I don't think the gods are gonna like this... */
	gods_upset(altaralign);
    else {
	int saved_anger = u.ugangr;
	int saved_cnt = u.ublesscnt;
	int saved_luck = u.uluck;

	/* Sacrificing at an altar of a different alignment */
	if (u.ualign.type != altaralign) {
	    /* Is this a conversion ? */
	    if(ugod_is_angry()) {
		if(u.ualignbase[0] == u.ualignbase[1] &&
		   altaralign != A_NONE) {
/*JP		    You("have a strong feeling that %s is angry...", u_gname());*/
		    You("%sが怒っているような気がひどくした．．．", u_gname());
		    consume_offering(otmp);
/*JP		    pline("%s accepts your allegiance.", a_gname());*/
		    pline("%sはあなたの属性を受けいれた．", a_gname());
/*JP		    You("have a sudden sense of a new direction.");*/
		    You("突然，別の感覚にめざめた．");

		    /* The player wears a helm of opposite alignment? */
		    if (uarmh && uarmh->otyp == HELM_OF_OPPOSITE_ALIGNMENT)
			u.ualignbase[0] = altaralign;
		    else
			u.ualign.type = u.ualignbase[0] = altaralign;
		    flags.botl = 1;
		    /* Beware, Conversion is costly */
		    change_luck(-3);
		    u.ublesscnt += 300;
		    adjalign((int)(u.ualignbase[1] * (ALIGNLIM / 2)));
		} else {
/*JP		    pline("%s rejects your sacrifice!", a_gname());*/
		    pline("%sはあなたの献上物を受けいれない！", a_gname());
/*JP		    godvoice(altaralign, "Suffer, infidel!");*/
		    godvoice(altaralign, "異端者よ！失せろ！！");
		    adjalign(-5);
		    u.ugangr += 3;
		    (void) adjattrib(A_WIS, -2, TRUE);
		    if (!Inhell) angrygods(u.ualign.type);
		    change_luck(-5);
		}
		return(1);
	    } else {
		consume_offering(otmp);
/*JP		You("sense a conflict between %s and %s.",*/
		You("%sと%s間の争いを感じた．",
		    u_gname(), a_gname());
		if (rn2(8 + (int)u.ulevel) > 5) {
		    struct monst *pri;
/*JP		    You("feel the power of %s increase.", u_gname());*/
		    You("%sの力が増大したのを感じた．", u_gname());
		    exercise(A_WIS, TRUE);
		    change_luck(1);
		    /* Yes, this is supposed to be &=, not |= */
		    levl[u.ux][u.uy].altarmask &= AM_SHRINE;
		    /* the following accommodates stupid compilers */
		    levl[u.ux][u.uy].altarmask =
			levl[u.ux][u.uy].altarmask | (Align2amask(u.ualign.type));
		    if (!Blind)
/*JP			pline("The altar glows %s.",
			      Hallucination ? hcolor() :
			      u.ualign.type == A_LAWFUL ? White :
			      u.ualign.type ? Black : (const char *)"gray");*/
			pline("祭壇は%s輝いた．",
			      jconj_adj(Hallucination ? hcolor() :
			      u.ualign.type == A_LAWFUL ? White :
			      u.ualign.type ? Black : (const char *)"灰色に"));

		    if(rnl((int)u.ulevel) > 6 && u.ualign.record > 0 &&
		       rnd(u.ualign.record) > (3*ALIGNLIM)/4)
			summon_minion(altaralign, TRUE);
		    /* anger priest; test handles bones files */
		    if((pri = findpriest(temple_occupied(u.urooms))) &&
		       !p_coaligned(pri))
			angry_priest();
		} else {
/*JP		    pline("Unluckily, you feel the power of %s decrease.",*/
		    pline("不幸にも，%sの力が減少したのを感じた．",
			  u_gname());
		    change_luck(-1);
		    exercise(A_WIS, FALSE);
		    if(rnl((int)u.ulevel) > 6 && u.ualign.record > 0 &&
		       rnd(u.ualign.record) > (7*ALIGNLIM)/8)
			summon_minion(altaralign, TRUE);
		}
		return(1);
	    }
	}

	consume_offering(otmp);
	/* OK, you get brownie points. */
	if(u.ugangr) {
	    u.ugangr -=
		((value * (u.ualign.type == A_CHAOTIC ? 2 : 3)) / MAXVALUE);
	    if(u.ugangr < 0) u.ugangr = 0;
	    if(u.ugangr != saved_anger) {
		if (u.ugangr) {
/*JP		    pline("%s seems %s.", u_gname(),
			  Hallucination ? "groovy" : "slightly mollified");*/
		    pline("%sは%sに見える．", u_gname(),
			  Hallucination ? "素敵" : "ちょっと和らいだよう");

		    if ((int)u.uluck < 0) change_luck(1);
		} else {
/*JP		    pline("%s seems %s.", u_gname(), Hallucination ?
			  "cosmic (not a new fact)" : "mollified");*/
		    pline("%sは%sに見える．", u_gname(), Hallucination ?
			  "虹色(新事実ではない)" : "軽蔑したよう");

		    if ((int)u.uluck < 0) u.uluck = 0;
		}
	    } else { /* not satisfied yet */
		if (Hallucination)
/*JP		    pline("The gods seem tall.");*/
		    pline("神は背が高いように見える．");
/*JP		else You("have a feeling of inadequacy.");*/
		else You("まだまだ青いと感じた．");
	    }
	} else if(ugod_is_angry()) {
	    if(value > MAXVALUE) value = MAXVALUE;
	    if(value > -u.ualign.record) value = -u.ualign.record;
	    adjalign(value);
/*JP	    You("feel partially absolved.");*/
	    You("少しだけゆるしてもらえたのを感じた．");
	} else if (u.ublesscnt > 0) {
	    u.ublesscnt -=
		((value * (u.ualign.type == A_CHAOTIC ? 500 : 300)) / MAXVALUE);
	    if(u.ublesscnt < 0) u.ublesscnt = 0;
	    if(u.ublesscnt != saved_cnt) {
		if (u.ublesscnt) {
		    if (Hallucination)
/*JP			You("realize that the gods are not like you and I.");*/
			pline("神はあなたと私を好いてないことに気がついた．");
		    else
/*JP			You("have a hopeful feeling.");*/
			pline("希望が見えてきたような気がした．");
		    if ((int)u.uluck < 0) change_luck(1);
		} else {
		    if (Hallucination)
/*JP			pline("Overall, there is a smell of fried onions.");*/
			pline("全体的に，たまねぎを揚げた匂いがする．");
		    else
/*JP			You("have a feeling of reconciliation.");*/
			You("一体感を感じた．");
		    if ((int)u.uluck < 0) u.uluck = 0;
		}
	    }
	} else {
	    int nartifacts = nartifact_exist();

	    /* you were already in pretty good standing */
	    /* The player can gain an artifact */
	    /* The chance goes down as the number of artifacts goes up */
	    if (u.ulevel > 2 && !rn2(10 + (2 * nartifacts * nartifacts))) {
		otmp = mk_artifact((struct obj *)0, a_align(u.ux,u.uy));
		if (otmp) {
		    if (otmp->spe < 0) otmp->spe = 0;
		    if (otmp->cursed) uncurse(otmp);
		    dropy(otmp);
/*JP		    pline("An object appears at your %s!",*/
		    pline("あなたの%s元に物体が現れた！",
			  makeplural(body_part(FOOT)));
/*JP		    godvoice(u.ualign.type, "Use my gift wisely!");*/
		    godvoice(u.ualign.type, "我れ与えしもの賢く使うべし！");
		    u.ublesscnt = rnz(300 + (50 * nartifacts));
		    exercise(A_WIS, TRUE);
		    return(1);
		}
	    }
	    change_luck((value * LUCKMAX) / (MAXVALUE * 2));
	    if (u.uluck != saved_luck) {
		if (Blind)
/*JP		    You("think something brushed your %s.", body_part(FOOT));*/
		    pline("何かがあなたの%sをくすぐった．", body_part(FOOT));
		else You(Hallucination ?
/*JP		    "see crabgrass at your %s.  A funny thing in a dungeon." :
		    "glimpse a four-leaf clover at your %s.",*/
		    "あなたの%s元にペンペン草をみつけた．迷宮にしては珍しい．":
		    "四葉のクローバーを%s元に見つけた．",
		    makeplural(body_part(FOOT)));
	    }
	}
    }
    return(1);
}

int
dopray()
{
    int alignment;

    p_aligntyp = on_altar() ? a_align(u.ux,u.uy) : u.ualign.type;
    p_trouble = in_trouble();

#ifdef POLYSELF
    if (is_demon(uasmon) && (p_aligntyp != A_CHAOTIC)) {
/*JP	pline("The very idea of praying to a %s god is repugnant to you.",
	      p_aligntyp ? "lawful" : "neutral");*/
	pline("%sの神に祈りをささげるのは常識に背く．",
	      p_aligntyp ? "秩序" : "中立");
	return(0);
    }
#endif

    if (u.ualign.type && u.ualign.type == -p_aligntyp)
	alignment = -u.ualign.record;
	/* Opposite alignment altar */
    else if (u.ualign.type != p_aligntyp) alignment = u.ualign.record / 2;
	/* Different (but non-opposite) alignment altar */
    else alignment = u.ualign.record;

/*JP    You("begin praying to %s.", align_gname(p_aligntyp));*/
    You("%sに祈りを捧げた．", align_gname(p_aligntyp));
    if ((!p_trouble && (u.ublesscnt > 0)) ||
	((p_trouble < 0) && (u.ublesscnt > 100)) || /* minor difficulties */
	((p_trouble > 0) && (u.ublesscnt > 200))) /* big trouble */
	p_type = 0;
    else if ((int)Luck < 0 || u.ugangr || alignment < 0)
	p_type = 1;
    else /* alignment >= 0 */ {
	if(on_altar() && u.ualign.type != p_aligntyp)
	    p_type = 2;
	else
	    p_type = 3;
    }

#ifdef POLYSELF
    if (is_undead(uasmon) && !Inhell &&
	(p_aligntyp == A_LAWFUL || (p_aligntyp == A_NEUTRAL && !rn2(10))))
	p_type = -1;
#endif

#ifdef WIZARD
    if (wizard && p_type >= 0) {
/*JP	if (yn("Force the gods to be pleased?") == 'y') {*/
	if (yn("無理矢理神に微笑んでもらいますか？") == 'y') {
	    u.ublesscnt = 0;
	    if (u.uluck < 0) u.uluck = 0;
	    if (u.ualign.record <= 0) u.ualign.record = 1;
	    u.ugangr = 0;
	    if(p_type < 2) p_type = 3;
	}
    }
#endif
    nomul(-3);
/*JP    nomovemsg = "You finish your prayer.";*/
    nomovemsg = "祈り終えた．";
    afternmv = prayer_done;

    if(p_type == 3 && !Inhell) {
	/* if you've been true to your god you can't die while you pray */
	if (!Blind)
/*JP	    You("are surrounded by a shimmering light.");*/
	    You("かすかな光につつまれた．");
	u.uinvulnerable = TRUE;
    }

    return(1);
}

STATIC_PTR int
prayer_done()		/* M. Stephenson (1.0.3b) */
{
    aligntyp alignment = p_aligntyp;

#ifdef POLYSELF
    if(p_type == -1) {
	godvoice(alignment,
		 alignment == A_LAWFUL ?
/*JP		 "Vile creature, thou durst call upon me?" :
		 "Walk no more, perversion of nature!");*/
		 "卑劣な生物よ，汝，我に祈りを求めたか？" :
		 "動くな！死にぞこないの生物よ！");
/*JP	You("feel like you are falling apart.");*/
	You("バラバラになったような気がした．");
	rehumanize();
/*JP	losehp(rnd(20), "residual undead turning effect", KILLED_BY_AN);*/
	losehp(rnd(20), "不死の生物を土に返す力で", KILLED_BY_AN);
	exercise(A_CON, FALSE);
	return(1);
    }
#endif
    if (Inhell) {
/*JP	pline("Since you are in Gehennom, %s won't help you.",*/
	pline("ゲヘナに%sの力は届かない．",
	      align_gname(alignment));
	/* haltingly aligned is least likely to anger */
	if (u.ualign.record <= 0 || rnl(u.ualign.record))
	    angrygods(u.ualign.type);
	return(0);
    }

    if (p_type == 0) {
	if(on_altar() && u.ualign.type != alignment)
	    (void) water_prayer(FALSE);
	u.ublesscnt += rnz(250);
	change_luck(-3);
	gods_upset(u.ualign.type);
    } else if(p_type == 1) {
	if(on_altar() && u.ualign.type != alignment)
	    (void) water_prayer(FALSE);
	angrygods(u.ualign.type);	/* naughty */
    } else if(p_type == 2) {
	if(water_prayer(FALSE)) {
	    /* attempted water prayer on a non-coaligned altar */
	    u.ublesscnt += rnz(250);
	    change_luck(-3);
	    gods_upset(u.ualign.type);
	} else pleased(alignment);
    } else {
	u.uinvulnerable = FALSE;
	/* coaligned */
	if(on_altar())
	    (void) water_prayer(TRUE);
	pleased(alignment); /* nice */
    }
    return(1);
}

int
doturn()
{	/* Knights & Priest(esse)s only please */

	register struct monst *mtmp, *mtmp2;
	register int	xlev = 6;

	if((pl_character[0] != 'P') &&
	   (pl_character[0] != 'K')) {
		/* Try to use turn undead spell. */
		if (objects[SPE_TURN_UNDEAD].oc_name_known) {
		    register int sp_no;
		    for (sp_no = 0; sp_no < MAXSPELL &&
			 spl_book[sp_no].sp_id != NO_SPELL &&
			 spl_book[sp_no].sp_id != SPE_TURN_UNDEAD; sp_no++);

		    if (sp_no < MAXSPELL &&
			spl_book[sp_no].sp_id == SPE_TURN_UNDEAD)
			    return spelleffects(++sp_no, TRUE);
		}

/*JP		You("don't know how to turn undead!");*/
		You("不死の生き物を土に戻す方法を知らない！");
		return(0);
	}
	if (
#  ifdef POLYSELF
	    (u.ualign.type != A_CHAOTIC &&
		    (is_demon(uasmon) || is_undead(uasmon))) ||
#  endif
	    u.ugangr > 6 /* "Die, mortal!" */) {

/*JP		pline("For some reason, %s seems to ignore you.", u_gname());*/
		pline("何らかの理由で，%sはあなたを無視したようだ．", u_gname());
		aggravate();
		exercise(A_WIS, FALSE);
		return(0);
	}

	if (Inhell) {
/*JP	    pline("Since you are in Gehennom, %s won't help you.", u_gname());*/
	    pline("ゲヘナに%sの力は届かない．", u_gname());
	    aggravate();
	    return(0);
	}
/*JP	pline("Calling upon %s, you chant an arcane formula.", u_gname());*/
	pline("%sに祈りを求めると，あなたは不可思議な言葉の聖歌を聞いた．",u_gname());
	exercise(A_WIS, TRUE);
	for(mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;
	    if(cansee(mtmp->mx,mtmp->my)) {
		if(!mtmp->mpeaceful && (is_undead(mtmp->data) ||
		   (is_demon(mtmp->data) && (u.ulevel > (MAXULEV/2))))) {

		    if(Confusion) {
/*JP			pline("Unfortunately, your voice falters.");*/
			pline("不幸にもあなたの声はどもってしまった．");
			mtmp->mflee = mtmp->mfrozen = mtmp->msleep = FALSE;
			mtmp->mcanmove = TRUE;
		    } else if (! resist(mtmp, '\0', 0, TELL)) {
			switch (mtmp->data->mlet) {
			    /* this is intentional, lichs are tougher
			       than zombies. */
			case S_LICH:    xlev += 2;
			case S_GHOST:   xlev += 2;
			case S_VAMPIRE: xlev += 2;
			case S_WRAITH:  xlev += 2;
			case S_MUMMY:   xlev += 2;
			case S_ZOMBIE:
			    mtmp->mflee = TRUE; /* at least */
			    if(u.ulevel >= xlev &&
			       !resist(mtmp, '\0', 0, NOTELL)) {
				if(u.ualign.type == A_CHAOTIC) {
				    mtmp->mpeaceful = TRUE;
				} else { /* damn them */
/*JP				    You("destroy %s!", mon_nam(mtmp));*/
				    You("%sを殺した！", mon_nam(mtmp));
				    mondied(mtmp);
				}
			    }
			    break;
			default:    mtmp->mflee = TRUE;
			    break;
			}
		    }
		}
	    }
	}
	nomul(-5);
	return(1);
}

const char *
a_gname()
{
    return(a_gname_at(u.ux, u.uy));
}

const char *
a_gname_at(x,y)     /* returns the name of an altar's deity */
xchar x, y;
{
    if(!IS_ALTAR(levl[x][y].typ)) return((char *)0);

    return align_gname(a_align(x,y));
}

const char *
u_gname()  /* returns the name of the player's deity */
{
    return align_gname(u.ualign.type);
}

const char *
align_gname(alignment)
	register aligntyp alignment;
{
	register struct ghods *aghod;

/*JP	if(alignment == A_NONE) return(Moloch);*/
	if(alignment == A_NONE) return(jtrns_mon(Moloch));

	for(aghod=gods; aghod->classlet; aghod++)
	    if(aghod->classlet == pl_character[0])
		switch(alignment) {
		case A_CHAOTIC:	return(jtrns_mon(aghod->chaos));
		case A_NEUTRAL:	return(jtrns_mon(aghod->balance));
		case A_LAWFUL:	return(jtrns_mon(aghod->law));
		default: impossible("unknown alignment.");
			 return("Balance");
		}
	impossible("unknown character's god?");
/*JP	return("someone");*/
	return("誰か");
}

void
altar_wrath(x, y)
register int x, y;
{
    aligntyp altaralign = a_align(x,y);

    if(!strcmp(align_gname(altaralign), u_gname())) {
/*JP	godvoice(altaralign, "How darest thou desecrate my altar!");*/
	godvoice(altaralign, "汝，我が祭壇を汚すか！");
	(void) adjattrib(A_WIS, -1, FALSE);
    } else {
/*JP	pline("A voice (could it be %s?) whispers:",*/
	pline("ささやき声が聞こえる(たぶん%s？):",
	      align_gname(altaralign));
/*JP	verbalize("Thou shalt pay, infidel!");*/
	verbalize("異端者よ！献金せよ！");
	change_luck(-1);
    }
}

/*pray.c*/
