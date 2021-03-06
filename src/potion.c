/*	SCCS Id: @(#)potion.c	3.2	96/08/09	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

#ifdef OVLB
boolean notonhead = FALSE;

static NEARDATA int nothing, unkn;
static NEARDATA const char beverages[] = { POTION_CLASS, 0 };

static long FDECL(itimeout, (long));
static long FDECL(itimeout_incr, (long,int));
static void NDECL(ghost_from_bottle);
static short FDECL(mixtype, (struct obj *,struct obj *));

/* force `val' to be within valid range for intrinsic timeout value */
static long
itimeout(val)
long val;
{
    if (val >= TIMEOUT) val = TIMEOUT;
    else if (val < 1) val = 0;

    return val;
}

/* increment `old' by `incr' and force result to be valid intrinsic timeout */
static long
itimeout_incr(old, incr)
long old;
int incr;
{
    return itimeout((old & TIMEOUT) + (long)incr);
}

/* set the timeout field of intrinsic `which' */
void
set_itimeout(which, val)
long *which, val;
{
    *which &= ~TIMEOUT;
    *which |= itimeout(val);
}

/* increment the timeout field of intrinsic `which' */
void
incr_itimeout(which, incr)
long *which;
int incr;
{
    set_itimeout(which, itimeout_incr(*which, incr));
}

void
make_confused(xtime,talk)
long xtime;
boolean talk;
{
	long old = HConfusion;

	if (!xtime && old) {
		if (talk)
/*JP		    You_feel("less %s now.",
			Hallucination ? "trippy" : "confused");*/
		    You("%sがおさまった",
			Hallucination ? "ヘロヘロ" : "混乱");
	}
	if ((xtime && !old) || (!xtime && old)) flags.botl = TRUE;

	set_itimeout(&HConfusion, xtime);
}

void
make_stunned(xtime,talk)
long xtime;
boolean talk;
{
	long old = HStun;

	if (!xtime && old) {
		if (talk)
/*JP		    You_feel("%s now.",
			Hallucination ? "less wobbly" : "a bit steadier");*/
		    You_feel("%s．",
			Hallucination ? "へろりらがおさまった" : "だんだんしっかりしてきた");
	}
	if (xtime && !old) {
/*JP		if (talk) You("stagger...");*/
		if (talk) You("くらくらした．．．");
	}
	if ((!xtime && old) || (xtime && !old)) flags.botl = TRUE;

	set_itimeout(&HStun, xtime);
}

void
make_sick(xtime, cause, talk, type)
long xtime;
const char *cause;	/* sickness cause */
boolean talk;
int type;
{
	long old = Sick;

	if (xtime > 0L) {
	    if (u.usym == S_FUNGUS) return;
	    if (!old) {
		/* newly sick */
/*JP		You_feel("deathly sick.");*/
		You("病気で死にそうだ．");
	    } else {
		/* already sick */
/*JP		if (talk) You_feel("%s worse.",
			      xtime <= Sick/2L ? "much" : "even");*/
		if (talk) You("%s悪化したような気がする．",
			      xtime <= Sick/2L ? "さらに" : "もっと");
	    }
	    set_itimeout(&Sick, xtime);
	    u.usick_type |= type;
	    flags.botl = TRUE;
	} else if (old && (type & u.usick_type)) {
	    /* was sick, now not */
	    u.usick_type &= ~type;
	    if (u.usick_type) { /* only partly cured */
/*JP		if (talk) You_feel("somewhat better.");*/
		if (talk) You("ちょっとよくなった．");
		set_itimeout(&Sick, Sick * 2); /* approximation */
	    } else {
/*JP		if (talk) pline("What a relief!");*/
		if (talk) pline("ああこれでほっとした！");
		Sick = 0L;		/* set_itimeout(&Sick, 0L) */
	    }
	    flags.botl = TRUE;
	}

	if (Sick) {
	    exercise(A_CON, FALSE);
	    if (cause) {
		(void) strncpy(u.usick_cause, cause, sizeof(u.usick_cause));
		u.usick_cause[sizeof(u.usick_cause)-1] = 0;
		}
	    else
		u.usick_cause[0] = 0;
	} else
	    u.usick_cause[0] = 0;
}

void
make_vomiting(xtime, talk)
long xtime;
boolean talk;
{
	long old = Vomiting;

	if(!xtime && old)
/*JP	    if(talk) You_feel("much less nauseous now.");*/
	    if(talk) You("吐き気がおさまった．");

	set_itimeout(&Vomiting, xtime);
}


void
make_blinded(xtime, talk)
long xtime;
boolean talk;
{
	long old = Blinded;
	boolean changed = FALSE;

	if (u.usleep) talk = FALSE;

	if (!xtime && old && !Blindfolded && haseyes(uasmon)) {
	    if (talk) {
		if (Hallucination)
/*JP		    pline("Far out!  Everything is all cosmic again!");*/
		    pline("げ！なにもかもがまた虹色に見える！");
/*JP		else		   You("can see again.");*/
		else		   You("また見えるようになった．");
	    }
	    changed = TRUE;
	}
	if (xtime && !old && !Blindfolded && haseyes(uasmon)) {
	    if (talk) {
		if (Hallucination)
/*JP			pline("Oh, bummer!  Everything is dark!  Help!");*/
			pline("暗いよー，狭いよー，恐いよー！");
		else
/*JP			pline("A cloud of darkness falls upon you.");*/
			pline("暗黒の雲があなたを覆った．");
	    }
	    changed = TRUE;

	    /* Before the hero goes blind, set the ball&chain variables. */
	    if (Punished) set_bc(0);
	}
	set_itimeout(&Blinded, xtime);
	if (changed) {
	    flags.botl = 1;
	    vision_full_recalc = 1;
	    if (Telepat) see_monsters();
	}
}

#ifdef	JPEXTENSION
void
make_totter(xtime, talk)
long xtime;	/* nonzero if this is an attempt to turn on hallucination */
boolean talk;
{
	const char *message = 0;

	if (!xtime)
	    message = "方向感覚が正常になった．";
	else
	    message = "方向感覚が麻痺した．";

	set_itimeout(&Totter, xtime);
	pline(message);
}
#endif

void
make_hallucinated(xtime, talk, mask)
long xtime;	/* nonzero if this is an attempt to turn on hallucination */
boolean talk;
long mask;	/* nonzero if resistance status should change by mask */
{
	boolean changed = 0;
#ifdef LINT
	const char *message = 0;
#else
	const char *message;
#endif

	if (!xtime)
/*JP	    message = "Everything looks SO boring now.";*/
	    message = "何もかもが＊退屈＊に見える．";
	else
/*JP	    message = "Oh wow!  Everything seems so cosmic!";*/
	    message = "ワーオ！何もかも虹色に見える！";

	if (mask) {
	    if (HHallucination) changed = TRUE;

	    if (!xtime) HHalluc_resistance |= mask;
	    else HHalluc_resistance &= ~mask;
	} else {
	    if (!HHalluc_resistance && (!!HHallucination != !!xtime))
		changed = TRUE;
	    set_itimeout(&HHallucination, xtime);
	}

	if (changed) {
	    if (u.uswallow) {
		swallowed(0);	/* redraw swallow display */
	    } else {
		/* The see_* routines should be called *before* the pline. */
		see_monsters();
		see_objects();
	    }
	    flags.botl = 1;
	    if (!Blind && talk) pline(message);
	}
}

static void
ghost_from_bottle()
{
	struct monst *mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, NO_MM_FLAGS);

	if (!mtmp) {
/*JP		pline("This bottle turns out to be empty.");*/
		pline("瓶は空っぽだった．");
		return;
	}
	if (Blind) {
/*JP		pline("As you open the bottle, %s emerges.", something);*/
		pline("瓶を開けると，何かが出てきた．");
		return;
	}
/*JP	pline("As you open the bottle, an enormous %s emerges!",
		Hallucination ? rndmonnam() : (const char *)"ghost");*/
	pline("瓶を開けると，巨大な%sが出てきた！",
		Hallucination ? rndmonnam() : (const char *)"幽霊");
	if(flags.verbose)
/*JP	    You("are frightened to death, and unable to move.");*/
	    You("まっさおになって驚き，動けなくなった．");
	nomul(-3);
/*JP	nomovemsg = "You regain your composure.";*/
	nomovemsg = "あなたは平静を取り戻した．";
}

int
dodrink() {
	register struct obj *otmp;
	const char *potion_descr;

	if (Strangled) {
/*JP		pline("If you can't breathe air, how can you drink liquid?");*/
		pline("息もできないのに，どうやって液体を飲むんだい？");
		return 0;
	}
	/* Is there a fountain to drink from here? */
	if (IS_FOUNTAIN(levl[u.ux][u.uy].typ) && !Levitation) {
/*JP		if(yn("Drink from the fountain?") == 'y') {*/
		if(yn("泉の水を飲みますか？") == 'y') {
			drinkfountain();
			return 1;
		}
	}
#ifdef SINKS
	/* Or a kitchen sink? */
	if (IS_SINK(levl[u.ux][u.uy].typ)) {
/*JP		if (yn("Drink from the sink?") == 'y') {*/
		if (yn("流し台の水を飲みますか？") == 'y') {
			drinksink();
			return 1;
		}
	}
#endif

	/* Or are you surrounded by water? */
	if (Underwater) {
/*JP		if (yn("Drink the water around you?") == 'y') {*/
		if (yn("まわりの水を飲みますか？") == 'y') {
/*JP		    pline("Do you know what lives in this water!");*/
		    pline("この水中で何が生きているのか知ってるかい！");
			return 1;
		}
	}

/*JP	otmp = getobj(beverages, "drink");*/
	otmp = getobj(beverages, "飲む");
	if(!otmp) return(0);
#ifndef NO_SIGNAL
	otmp->in_use = TRUE;		/* you've opened the stopper */
#endif
#define POTION_OCCUPANT_CHANCE(n) (13 + 2*(n))	/* also in muse.c */

	potion_descr = OBJ_DESCR(objects[otmp->otyp]);
	if (potion_descr && !strcmp(potion_descr, "milky") &&
		    flags.ghost_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.ghost_count))) {
		ghost_from_bottle();
		useup(otmp);
		return(1);
	} else if (potion_descr && !strcmp(potion_descr, "smoky") &&
		    flags.djinni_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.djinni_count))) {
		djinni_from_bottle(otmp);
		useup(otmp);
		return(1);
	}
	return dopotion(otmp);
}

int
dopotion(otmp)
register struct obj *otmp;
{
	int retval;

	nothing = unkn = 0;
	if((retval = peffects(otmp)) >= 0) return(retval);

	if(nothing) {
	    unkn++;
/*JP	    You("have a %s feeling for a moment, then it passes.",
		  Hallucination ? "normal" : "peculiar");*/

	    You("%s気分におそわれたが，すぐに消えさった．",
		  Hallucination ? "普通の" : "独特な");
	}
	if(otmp->dknown && !objects[otmp->otyp].oc_name_known) {
		if(!unkn) {
			makeknown(otmp->otyp);
			more_experienced(0,10);
		} else if(!objects[otmp->otyp].oc_uname)
			docall(otmp);
	}
	useup(otmp);
	return(1);
}

int
peffects(otmp)
	register struct obj	*otmp;
{
	register int i, ii, lim;

	switch(otmp->otyp){
	case POT_RESTORE_ABILITY:
	case SPE_RESTORE_ABILITY:
		unkn++;
		if(otmp->cursed) {
/*JP		    pline("Ulch!  This makes you feel mediocre!");*/
		    pline("うーん、どうもさえないなあ．");
		    break;
		} else {
/*JP		    pline("Wow!  This makes you feel %s!",
			  (otmp->blessed) ? "great" : "good");*/
		    pline("ワーオ！気分が%sなった！",
			  (otmp->blessed) ? "とてもよく" : "よく");
		    i = rn2(A_MAX);		/* start at a random point */
		    for (ii = 0; ii < A_MAX; ii++) {
			lim = AMAX(i);
			if (i == A_STR && u.uhs >= 3) --lim;	/* WEAK */
			if (ABASE(i) < lim) {
			    ABASE(i) = lim;
			    flags.botl = 1;
			    /* only first found if not blessed */
			    if (!otmp->blessed) break;
			}
			if(++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_HALLUCINATION:
		if (Hallucination || HHalluc_resistance) nothing++;
		make_hallucinated(itimeout_incr(HHallucination,
					   rn1(200, 600 - 300 * bcsign(otmp))),
				  TRUE, 0L);
		break;
	case POT_WATER:
		if(!otmp->blessed && !otmp->cursed) {
/*JP			pline("This tastes like water.");*/
			pline("水のような味がする");
			u.uhunger += rnd(10);
			newuhs(FALSE);
			break;
		}
		unkn++;
		if(is_undead(uasmon) || is_demon(uasmon) ||
				u.ualign.type == A_CHAOTIC) {
		    if(otmp->blessed) {
/*JP
			pline("This burns like acid!");
*/
			pline("酸のように舌がひりひりする！");
			exercise(A_CON, FALSE);
			if (u.ulycn >= LOW_PM) {
/*JP
			    Your("affinity to %s disappears!",
*/
			    Your("%sへの親近感はなくなった！",
				 makeplural(mons[u.ulycn].mname));
			    if (uasmon == &mons[u.ulycn])
				you_unwere(FALSE);
			    u.ulycn = NON_PM;	/* cure lycanthropy */
			}
/*JP
			losehp(d(2,6), "potion of holy water", KILLED_BY_AN);
*/
			losehp(d(2,6), "聖水で", KILLED_BY_AN);
		    } else if(otmp->cursed) {
/*JP
			You_feel("quite proud of yourself.");
*/
			You("自尊心を感じた．");
			healup(d(2,6),0,0,0);
			if (u.ulycn >= LOW_PM && !Upolyd) you_were();
			exercise(A_CON, TRUE);
		    }
		} else {
		    if(otmp->blessed) {
/*JP
			You_feel("full of awe.");
*/
			You("畏怖の念にかられた．");
			make_sick(0L, (char *) 0, TRUE, SICK_ALL);
			exercise(A_WIS, TRUE);
			exercise(A_CON, TRUE);
			if (u.ulycn >= LOW_PM)
			    you_unwere(TRUE);	/* "Purified" */
			/* make_confused(0L,TRUE); */
		    } else {
			if(u.ualign.type == A_LAWFUL) {
/*JP
			    pline("This burns like acid!");
			    losehp(d(2,6), "potion of unholy water",
*/
			    pline("酸のように舌がひりひりする！");
			    losehp(d(2,6), "不浄水で",
				KILLED_BY_AN);
			} else
/*JP
			    You_feel("full of dread.");
*/
			    You("恐怖の念にかられた．");
			if (u.ulycn >= LOW_PM && !Upolyd) you_were();
			exercise(A_CON, FALSE);
		    }
		}
		break;
	case POT_BOOZE:
		unkn++;
/*JP
		pline("Ooph!  This tastes like %s%s!",
		      otmp->odiluted ? "watered down " : "",
		      Hallucination ? "dandelion wine" : "liquid fire");
*/
		pline("うぇっぷ！これは%s%sのような味がする！",
		      otmp->odiluted ? "水で薄めた" : "",
		      Hallucination ? "タンポポワイン" : "燃料オイル");
		if (!otmp->blessed)
		    make_confused(itimeout_incr(HConfusion, d(3,8)), FALSE);
		/* the whiskey makes us feel better */
		if (!otmp->odiluted) healup(1, 0, FALSE, FALSE);
		u.uhunger += 10 * (2 + bcsign(otmp));
		newuhs(FALSE);
		exercise(A_WIS, FALSE);
		if(otmp->cursed) {
/*JP			You("pass out.");*/
			You("気絶した");
			multi = -rnd(15);
/*JP			nomovemsg = "You awake with a headache.";*/
			nomovemsg = "目がさめたが頭痛がする．";
		}
		break;
	case POT_ENLIGHTENMENT:
		if(otmp->cursed) {
			unkn++;
/*JP			You("have an uneasy feeling...");*/
			You("不安な気持になった．．．");
			exercise(A_WIS, FALSE);
		} else {
			if (otmp->blessed) {
				(void) adjattrib(A_INT, 1, FALSE);
				(void) adjattrib(A_WIS, 1, FALSE);
			}
/*JP			You_feel("self-knowledgeable...");*/
			You("自分自身が判るような気がした．．．");
			display_nhwindow(WIN_MESSAGE, FALSE);
			enlightenment(0);
/*JP			pline_The("feeling subsides.");*/
			pline("その感じはなくなった．");
			exercise(A_WIS, TRUE);
		}
		break;
	case POT_INVISIBILITY:
	case SPE_INVISIBILITY:
		if (Invisible) {		/* no current effect */
		    nothing++;
		} else if (Blind ||		/* current effect sensed */
			(HInvis & I_BLOCKED)) {
/*JP		    You_feel("rather airy."),  unkn++;*/
		    You("空気のようになった気がした．"), unkn++;

		} else if (See_invisible) {	/* eyes override sensing */
		    nothing++;
		} else {			/* current effect seen */
		    pline(Hallucination ?
/*JP			 "Far out, man!  You can see right through yourself!" :
			 "Gee!  All of a sudden, you can't see yourself.");*/
			 "なんてこったい！自分自身を通して真実が見えるようになった！":
			 "げ！突然あなたの姿は見えなくなった．");
		}
		if (otmp->blessed) HInvis |= FROMOUTSIDE;
		else incr_itimeout(&HInvis, rn1(15,31));
		newsym(u.ux,u.uy);	/* update position */
		if(otmp->cursed) {
/*JP		    pline("For some reason, you feel your presence is known.");*/
		    pline("なんらかの理由で，存在が知られているような気がした．");
		    aggravate();
		}
		break;
	case POT_SEE_INVISIBLE:
		/* tastes like fruit juice in Rogue */
	case POT_FRUIT_JUICE:
		unkn++;
		if (otmp->cursed)
/*JP		    pline("Yecch!  This tastes %s.",
			  Hallucination ? "overripe" : "rotten");*/
		    pline("オェー！これは%sジュースの味がする．",
 			  Hallucination ? "熟しすぎた" : "腐った");
		else pline(Hallucination ?
/*JP		"This tastes like 10%% real %s%s juice all-natural beverage." :
				"This tastes like %s%s juice.",
			  otmp->odiluted ? "reconstituted " : "", pl_fruit);*/
			   "10%%%sの純自然飲料のような味がする．" :
			   "%s%sジュースのような味がする．",
			  otmp->odiluted ? "成分調整された" : "", pl_fruit);
		if (otmp->otyp == POT_FRUIT_JUICE) {
		    u.uhunger += (otmp->odiluted ? 5 : 10) * (2 + bcsign(otmp));
		    newuhs(FALSE);
		    break;
		}
		if (!otmp->cursed) {
			/* Tell them they can see again immediately, which
			 * will help them identify the potion...
			 */
			make_blinded(0L,TRUE);
		}
		if (otmp->blessed)
			HSee_invisible |= FROMOUTSIDE;
		else
			incr_itimeout(&HSee_invisible, rn1(100,750));
		set_mimic_blocking(); /* do special mimic handling */
		see_monsters();	/* see invisible monsters */
		newsym(u.ux,u.uy); /* see yourself! */
		break;
	case POT_PARALYSIS:
		if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
/*JP		    You("are motionlessly suspended.");*/
		    You("空中で動けなくなった．");
		else
/*JP		    Your("%s are frozen to the %s!",
			 makeplural(body_part(FOOT)), surface(u.ux, u.uy));*/
		    You("動けなくなった！");
		nomul(-(rn1(10, 25 - 12*bcsign(otmp))));
		nomovemsg = You_can_move_again;
		exercise(A_DEX, FALSE);
		break;
	case POT_MONSTER_DETECTION:
	case SPE_DETECT_MONSTERS:
		if (monster_detect(otmp, 0))
			return(1);		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_OBJECT_DETECTION:
	case SPE_DETECT_TREASURE:
		if (object_detect(otmp, 0))
			return(1);		/* nothing detected */
		exercise(A_WIS, TRUE);
		break;
	case POT_SICKNESS:
/*JP		pline("Yecch!  This stuff tastes like poison.");*/
		pline("ウェー！毒のような味がする．");
		if (otmp->blessed) {
/*JP		    pline("(But in fact it was mildly stale %s juice.)",*/
		pline("(しかし実際それは腐りかけた%sジュースだ．)",
			  pl_fruit);
		    if (!Role_is('H'))
/*JP			losehp(1, "mildly contaminated potion", KILLED_BY_AN);*/
			losehp(1, "病気に汚染された薬で", KILLED_BY_AN);
		} else {
		    if(Poison_resistance)
			pline(
/*JP		    "(But in fact it was biologically contaminated %s juice.)",*/
		    "(しかし実際それは細菌に汚染された%sジュースだ．)",
			      pl_fruit);
		    if (Role_is('H'))
/*JP			pline("Fortunately, you have been immunized.");*/
			pline("幸運なことに，あなたは免疫がある．");
		    else {
			int typ = rn2(A_MAX);
			poisontell(typ);
			(void) adjattrib(typ,
					Poison_resistance ? -1 : -rn1(4,3),
					TRUE);
			if(!Poison_resistance)
				losehp(rnd(10)+5*!!(otmp->cursed),
/*JP				       "contaminated potion", KILLED_BY_AN);*/
				       "疫病に汚染された薬で", KILLED_BY_AN);
			exercise(A_CON, FALSE);
		    }
		}
		if(Hallucination) {
/*JP			You("are shocked back to your senses!");*/
			You("五感に衝撃を受けた！");
			make_hallucinated(0L,FALSE,0L);
		}
		break;
	case POT_CONFUSION:
		if(!Confusion)
		    if (Hallucination) {
/*JP			pline("What a trippy feeling!");*/
			pline("なんかヘロヘロする！");
			unkn++;
		    } else
/*JP			pline("Huh, What?  Where am I?");*/
			pline("ほえ？私は誰？");
		else	nothing++;
		make_confused(itimeout_incr(HConfusion,
					    rn1(7, 16 - 8 * bcsign(otmp))),
			      FALSE);
		break;
	case POT_GAIN_ABILITY:
		if(otmp->cursed) {
/*JP		    pline("Ulch!  That potion tasted foul!");*/
		    pline("ウェ！悪臭がする！");
		    unkn++;
		} else {      /* If blessed, increase all; if not, try up to */
		    int itmp; /* 6 times to find one which can be increased. */
		    i = -1;		/* increment to 0 */
		    for (ii = A_MAX; ii > 0; ii--) {
			i = (otmp->blessed ? i + 1 : rn2(A_MAX));
			/* only give "your X is already as high as it can get"
			   message on last attempt (except blessed potions) */
			itmp = (otmp->blessed || ii == 1) ? 0 : -1;
			if (adjattrib(i, 1, itmp) && !otmp->blessed)
			    break;
		    }
		}
		break;
	case POT_SPEED:
		if(Wounded_legs && !otmp->cursed) {
			heal_legs();
			unkn++;
			break;
		}		/* and fall through */
	case SPE_HASTE_SELF:
		if(!(Fast & ~INTRINSIC)) /* wwf@doe.carleton.ca */
/*JP			You("are suddenly moving %sfaster.",
				Fast ? "" : "much ");*/
			You("突然%s速く移動できるようになった．",
				Fast ? "" : "とても");
		else {
/*JP			Your("%s get new energy.",*/
			pline("%sにエネルギーが注ぎこまれるような感じがした",
				makeplural(body_part(LEG)));
			unkn++;
		}
		exercise(A_DEX, TRUE);
		incr_itimeout(&Fast, rn1(10, 100 + 60 * bcsign(otmp)));
		break;
	case POT_BLINDNESS:
		if(Blind) nothing++;
		make_blinded(itimeout_incr(Blinded,
					   rn1(200, 250 - 125 * bcsign(otmp))),
			     TRUE);
		break;
	case POT_GAIN_LEVEL:
		if (otmp->cursed) {
			unkn++;
			/* they went up a level */
			if((ledger_no(&u.uz) == 1 && u.uhave.amulet) ||
				Can_rise_up(u.ux, u.uy, &u.uz)) {
/*JP			    const char *riseup ="rise up, through the %s!";*/
			    const char *riseup ="%sを突き抜けた！";
			    if(ledger_no(&u.uz) == 1) {
			        You(riseup, ceiling(u.ux,u.uy));
				goto_level(&earth_level, FALSE, FALSE, FALSE);
			    } else {
			        register int newlev = depth(&u.uz)-1;
				d_level newlevel;

				get_level(&newlevel, newlev);
				if(on_level(&newlevel, &u.uz)) {
/*JP				    pline("It tasted bad.");*/
				    pline("とてもまずい．");
				    break;
				} else You(riseup, ceiling(u.ux,u.uy));
				goto_level(&newlevel, FALSE, FALSE, FALSE);
			    }
			}
/*JP			else You("have an uneasy feeling.");*/
			else You("不安な気持になった．");
			break;
		}
		pluslvl();
		if (otmp->blessed)
			/* blessed potions place you at a random spot in the
			 * middle of the new level instead of the low point
			 */
			u.uexp = rndexp();
		break;
	case POT_HEALING:
/*JP		You_feel("better.");*/
		You("気分がよくなった．");
		healup(d(6 + 2 * bcsign(otmp), 4),
		       !otmp->cursed ? 1 : 0, !!otmp->blessed, !otmp->cursed);
		exercise(A_CON, TRUE);
		break;
	case POT_EXTRA_HEALING:
/*JP		You_feel("much better.");*/
		You("気分がとてもよくなった．");
		healup(d(6 + 2 * bcsign(otmp), 8),
		       otmp->blessed ? 5 : !otmp->cursed ? 2 : 0,
		       !otmp->cursed, TRUE);
		make_hallucinated(0L,TRUE,0L);
		exercise(A_CON, TRUE);
		exercise(A_STR, TRUE);
		break;
	case POT_LEVITATION:
	case SPE_LEVITATION:
		if (otmp->cursed) HLevitation &= ~I_SPECIAL;
		if(!Levitation) {
			/* kludge to ensure proper operation of float_up() */
			HLevitation = 1;
			float_up();
			/* reverse kludge */
			HLevitation = 0;
			if (otmp->cursed && !Is_waterlevel(&u.uz)) {
	if((u.ux != xupstair || u.uy != yupstair)
	   && (u.ux != sstairs.sx || u.uy != sstairs.sy || !sstairs.up)
	   && (!xupladder || u.ux != xupladder || u.uy != yupladder)
	) {
/*JP					You("hit your %s on the %s.",
						body_part(HEAD),
						ceiling(u.ux,u.uy));
					losehp(uarmh ? 1 : rnd(10),
						"colliding with the ceiling",
						KILLED_BY);*/
					You("%sを%sにぶつけた．",
						body_part(HEAD),
						ceiling(u.ux,u.uy));
					losehp(uarmh ? 1 : rnd(10),
						"天井に頭をぶつけて",
						KILLED_BY);
				} else (void) doup();
			}
		} else
			nothing++;
		if (otmp->blessed) {
		    incr_itimeout(&HLevitation, rn1(50,250));
		    HLevitation |= I_SPECIAL;
		} else incr_itimeout(&HLevitation, rn1(140,10));
		break;
	case POT_GAIN_ENERGY:			/* M. Stephenson */
		{	register int num;
			if(otmp->cursed)
/*JP			    You_feel("lackluster.");*/
			    You("意気消沈した．");
			else
/*JP			    pline("Magical energies course through your body.");*/
			    pline("魔法のエネルギーがあなたの体に満たされた．");
			num = rnd(5) + 5 * otmp->blessed + 1;
			u.uenmax += (otmp->cursed) ? -num : num;
			u.uen += (otmp->cursed) ? -num : num;
			if(u.uenmax <= 0) u.uenmax = 0;
			if(u.uen <= 0) u.uen = 0;
			flags.botl = 1;
			exercise(A_WIS, TRUE);
		}
		break;
	case POT_OIL:				/* P. Winner */
		{
			boolean good_for_you = FALSE;

			if (otmp->lamplit) {
			    if (likes_fire(uasmon)) {
/*JP				pline("Ahh, a refreshing drink.");*/
				pline("わーお，生き返る．");
				good_for_you = TRUE;
			    } else {
/*JP				You("burn your %s", body_part(FACE));*/
				Your("%sは黒焦げになった．", body_part(FACE));
				losehp(d(Fire_resistance ? 1 : 3, 4),
/*JP				       "burning potion of oil", KILLED_BY_AN);*/
				       "燃えている油を飲んで", KILLED_BY_AN);
			    }
			} else if(otmp->cursed)
/*JP			    pline("This tastes like castor oil.");*/
			    pline("ビーバー香のような味がする．");
			else
/*JP			    pline("That was smooth!");*/
			    pline("口あたりがよい！");
			exercise(A_WIS, good_for_you);
		}
		break;
	default:
		impossible("What a funny potion! (%u)", otmp->otyp);
		return(0);
	}
	return(-1);
}

void
healup(nhp, nxtra, curesick, cureblind)
	int nhp, nxtra;
	register boolean curesick, cureblind;
{
	if (nhp) {
		if (Upolyd) {
			u.mh += nhp;
			if (u.mh > u.mhmax) u.mh = (u.mhmax += nxtra);
		} else {
			u.uhp += nhp;
			if(u.uhp > u.uhpmax) u.uhp = (u.uhpmax += nxtra);
		}
	}
	if(cureblind)	make_blinded(0L,TRUE);
	if(curesick)	make_sick(0L, (char *) 0, TRUE, SICK_ALL);
	flags.botl = 1;
	return;
}

void
strange_feeling(obj,txt)
register struct obj *obj;
register const char *txt;
{
	if(flags.beginner)
/*JP		You("have a %s feeling for a moment, then it passes.",
		Hallucination ? "normal" : "strange");*/
		You("%s気分におそわれたが，すぐに消えさった．",
		Hallucination ? "普通の" : "奇妙な");
	else
		pline(txt);

	if(!obj)	/* e.g., crystal ball finds no traps */
		return;

	if(obj->dknown && !objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	useup(obj);
}

const char *bottlenames[] = {
/*JP	"bottle", "phial", "flagon", "carafe", "flask", "jar", "vial"*/
	"瓶","薬瓶","一升瓶","水差し","フラスコ","壷","ガラス瓶"
};

void
potionhit(mon, obj)
register struct monst *mon;
register struct obj *obj;
{
	register const char *botlnam = bottlenames[rn2(SIZE(bottlenames))];
	boolean isyou = (mon == &youmonst);
	int distance;

	if(isyou) {
		distance = 0;
/*JP		pline_The("%s crashes on your %s and breaks into shards.",*/
		pline("%sがあなたの%sの上で壊れ破片となった．",
			botlnam, body_part(HEAD));
/*JP		losehp(rnd(2), "thrown potion", KILLED_BY_AN);*/
		losehp(rnd(2), "投げられた薬で", KILLED_BY_AN);
	} else {
		distance = distu(mon->mx,mon->my);
		if (mon->msleep) mon->msleep = 0;
/*JP		if (!cansee(mon->mx,mon->my)) pline("Crash!");*/
		if (!cansee(mon->mx,mon->my)) pline("ガシャン！");
		else {
		    char *mnam = mon_nam(mon);
		    char buf[BUFSZ];

		    if(has_head(mon->data)) {
/*JP			Sprintf(buf, "%s %s",*/
			Sprintf(buf, "%sの%s",
				s_suffix(mnam),
/*JP				(notonhead ? "body" : "head"));*/
				(notonhead ? "体" : "頭"));
		    } else {
			Strcpy(buf, mnam);
		    }
/*JP		    pline_The("%s crashes on %s and breaks into shards.",*/
		    pline("%sが%sの上で壊れ破片となった．",
			   botlnam, buf);
		}
		if(rn2(5) && mon->mhp > 1)
			mon->mhp--;
	}

	/* oil doesn't instantly evaporate */
	if (obj->otyp != POT_OIL && cansee(mon->mx,mon->my))
/*JP		pline("%s evaporates.", The(xname(obj)));*/
		pline("%sは蒸発した．", The(xname(obj)));

	if (isyou) switch (obj->otyp) {
	    case POT_OIL:
		if (obj->lamplit)
		    splatter_burning_oil(u.ux, u.uy);
		break;
	    }
	else switch (obj->otyp) {

	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
	case POT_HEALING:
	case POT_EXTRA_HEALING:
		if(mon->mhp < mon->mhpmax) {
		    mon->mhp = mon->mhpmax;
		    if (canseemon(mon))
/*JP			pline("%s looks sound and hale again.", Monnam(mon));*/
			pline("%sは元気になったように見える．", Monnam(mon));
		}
		break;
	case POT_SICKNESS:
		if((mon->mhpmax > 3) && !resist(mon, POTION_CLASS, 0, NOTELL))
			mon->mhpmax /= 2;
		if((mon->mhp > 2) && !resist(mon, POTION_CLASS, 0, NOTELL))
			mon->mhp /= 2;
		if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
		if (canseemon(mon))
/*JP		    pline("%s looks rather ill.", Monnam(mon));*/
		    pline("%sは病気っぽく見える．", Monnam(mon));
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if(!resist(mon, POTION_CLASS, 0, NOTELL))  mon->mconf = TRUE;
		break;
	case POT_INVISIBILITY:
		mon_set_minvis(mon);
		break;
	case POT_PARALYSIS:
		if (mon->mcanmove) {
			mon->mcanmove = 0;
			/* really should be rnd(5) for consistency with players
			 * breathing potions, but...
			 */
			mon->mfrozen = rnd(25);
		}
		break;
	case POT_SPEED:
		if (mon->mspeed == MSLOW) mon->mspeed = 0;
		else mon->mspeed = MFAST;
		break;
	case POT_BLINDNESS:
		if(haseyes(mon->data)) {
		    register int btmp = 64 + rn2(32) +
			rn2(32) * !resist(mon, POTION_CLASS, 0, NOTELL);
		    btmp += mon->mblinded;
		    mon->mblinded = min(btmp,127);
		    mon->mcansee = 0;
		}
		break;
	case POT_WATER:
		if (is_undead(mon->data) || is_demon(mon->data) ||
			is_were(mon->data)) {
		    if (obj->blessed) {
/*JP
			pline("%s shrieks in pain!", Monnam(mon));
*/
			pline("%sは苦痛の叫び声をあげた！", Monnam(mon));
			mon->mhp -= d(2,6);
			if (mon->mhp < 1) killed(mon);
			else if (is_were(mon->data) && !is_human(mon->data))
			    new_were(mon);	/* revert to human */
		    } else if (obj->cursed) {
			if (canseemon(mon))
/*JP
			    pline("%s looks healthier.", Monnam(mon));
*/
			    pline("%sはより元気になったように見える．", Monnam(mon));
			mon->mhp += d(2,6);
			if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
			if (is_were(mon->data) && is_human(mon->data) &&
				!Protection_from_shape_changers)
			    new_were(mon);	/* transform into beast */
		    }
		} else if(mon->data == &mons[PM_GREMLIN]) {
		    struct monst *mtmp2 = clone_mon(mon);

		    if (mtmp2) {
			mtmp2->mhpmax = (mon->mhpmax /= 2);
			if (canseemon(mon))
/*JP			    pline("%s multiplies.", Monnam(mon));*/
			    pline("%sは分裂した．", Monnam(mon));
		    }
		}
		break;
	case POT_OIL:
		if (obj->lamplit)
			splatter_burning_oil(mon->mx, mon->my);
		break;
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
		break;
*/
	}
	/* Note: potionbreathe() does its own docall() */
	if (distance==0 || ((distance < 3) && rn2(5)))
		potionbreathe(obj);
	else if (obj->dknown && !objects[obj->otyp].oc_name_known &&
		   !objects[obj->otyp].oc_uname && cansee(mon->mx,mon->my))
		docall(obj);
	if(*u.ushops && obj->unpaid) {
	        register struct monst *shkp =
			shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

		if(!shkp)
		    obj->unpaid = 0;
		else {
		    (void)stolen_value(obj, u.ux, u.uy,
				 (boolean)shkp->mpeaceful, FALSE);
		    subfrombill(obj, shkp);
		}
	}
	obfree(obj, (struct obj *)0);
}

void
potionbreathe(obj)
register struct obj *obj;
{
	register int i, ii, isdone, kn = 0;

	switch(obj->otyp) {
	case POT_RESTORE_ABILITY:
	case POT_GAIN_ABILITY:
		if(obj->cursed) {
/*JP		    pline("Ulch!  That potion smells terrible!");*/
		    pline("ウゲェ！薬はものすごい匂いがする！");
		    break;
		} else {
		    i = rn2(A_MAX);		/* start at a random point */
		    for(isdone = ii = 0; !isdone && ii < A_MAX; ii++) {
			if(ABASE(i) < AMAX(i)) {
			    ABASE(i)++;
			    /* only first found if not blessed */
			    isdone = !(obj->blessed);
			    flags.botl = 1;
			}
			if(++i >= A_MAX) i = 0;
		    }
		}
		break;
	case POT_EXTRA_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		/*FALL THROUGH*/
	case POT_HEALING:
		if (Upolyd && u.mh < u.mhmax) u.mh++, flags.botl = 1;
		if (u.uhp < u.uhpmax) u.uhp++, flags.botl = 1;
		exercise(A_CON, TRUE);
		break;
	case POT_SICKNESS:
		if (!Role_is('H')) {
			if (Upolyd) {
			    if (u.mh <= 5) u.mh = 1; else u.mh -= 5;
			} else {
			    if (u.uhp <= 5) u.uhp = 1; else u.uhp -= 5;
			}
			flags.botl = 1;
			exercise(A_CON, FALSE);
		}
		break;
	case POT_HALLUCINATION:
/*JP		You("have a momentary vision.");*/
		You("一瞬幻影につつまれた．");
		break;
	case POT_CONFUSION:
	case POT_BOOZE:
		if(!Confusion)
/*JP			You_feel("somewhat dizzy.");*/
			You("めまいを感じた．");
		make_confused(itimeout_incr(HConfusion, rnd(5)), FALSE);
		break;
	case POT_INVISIBILITY:
		if (!See_invisible && !Invis)
/*JP			pline("For an instant you could see through yourself!");*/
			pline("一瞬自分自身が見えなくなった！");
		break;
	case POT_PARALYSIS:
		kn++;
/*JP		pline("%s seems to be holding you.", Something);*/
		pline("%sがあなたをつかまえているような気がした．", Something);
		nomul(-rnd(5));
		nomovemsg = You_can_move_again;
		exercise(A_DEX, FALSE);
		break;
	case POT_SPEED:
/*JP		if (!Fast) Your("knees seem more flexible now.");*/
		if (!Fast) Your("膝はより敏速に動くようになった．");
		incr_itimeout(&Fast, rnd(5));
		exercise(A_DEX, TRUE);
		break;
	case POT_BLINDNESS:
		if (!Blind && !u.usleep) {
		    kn++;
/*JP		    pline("It suddenly gets dark.");*/
		    pline("突然暗くなった．");
		}
		make_blinded(itimeout_incr(Blinded, rnd(5)), FALSE);
		break;
	case POT_WATER:
		if(u.umonnum == PM_GREMLIN) {
		    struct monst *mtmp;
		    if ((mtmp = cloneu()) != 0) {
			mtmp->mhpmax = (u.mhmax /= 2);
/*JP			You("multiply.");*/
			You("分裂した．");
		    }
		} else if (u.ulycn >= LOW_PM) {
		    /* vapor from [un]holy water will trigger
		       transformation but won't cure lycanthropy */
		    if (obj->blessed && uasmon == &mons[u.ulycn])
			you_unwere(FALSE);
		    else if (obj->cursed && !Upolyd)
			you_were();
		}
/*
	case POT_GAIN_LEVEL:
	case POT_LEVITATION:
	case POT_FRUIT_JUICE:
	case POT_MONSTER_DETECTION:
	case POT_OBJECT_DETECTION:
	case POT_OIL:
*/
		break;
	}
	/* note: no obfree() */
	if (obj->dknown){
	    if (kn)
		makeknown(obj->otyp);
	    else if (!objects[obj->otyp].oc_name_known &&
						!objects[obj->otyp].oc_uname)
		docall(obj);
	}
}

static short
mixtype(o1, o2)
register struct obj *o1, *o2;
/* returns the potion type when o1 is dipped in o2 */
{
	/* cut down on the number of cases below */
	if (o1->oclass == POTION_CLASS &&
	    (o2->otyp == POT_GAIN_LEVEL ||
	     o2->otyp == POT_GAIN_ENERGY ||
	     o2->otyp == POT_HEALING ||
	     o2->otyp == POT_EXTRA_HEALING ||
	     o2->otyp == POT_ENLIGHTENMENT ||
	     o2->otyp == POT_FRUIT_JUICE)) {
		struct obj *swp;

		swp = o1; o1 = o2; o2 = swp;
	}

	switch (o1->otyp) {
		case POT_HEALING:
			switch (o2->otyp) {
			    case POT_SPEED:
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_EXTRA_HEALING;
			}
		case POT_EXTRA_HEALING:
			switch (o2->otyp) {
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_GAIN_ABILITY;
			}
		case UNICORN_HORN:
			switch (o2->otyp) {
			    case POT_SICKNESS:
				return POT_FRUIT_JUICE;
			    case POT_HALLUCINATION:
			    case POT_BLINDNESS:
			    case POT_CONFUSION:
				return POT_WATER;
			}
			break;
		case AMETHYST:		/* "a-methyst" == "not intoxicated" */
			if (o2->otyp == POT_BOOZE)
			    return POT_FRUIT_JUICE;
			break;
		case POT_GAIN_LEVEL:
		case POT_GAIN_ENERGY:
			switch (o2->otyp) {
			    case POT_CONFUSION:
				return (rn2(3) ? POT_BOOZE : POT_ENLIGHTENMENT);
			    case POT_HEALING:
				return POT_EXTRA_HEALING;
			    case POT_EXTRA_HEALING:
				return POT_GAIN_ABILITY;
			    case POT_FRUIT_JUICE:
				return POT_SEE_INVISIBLE;
			    case POT_BOOZE:
				return POT_HALLUCINATION;
			}
			break;
		case POT_FRUIT_JUICE:
			switch (o2->otyp) {
			    case POT_SICKNESS:
				return POT_SICKNESS;
			    case POT_SPEED:
				return POT_BOOZE;
			    case POT_GAIN_LEVEL:
			    case POT_GAIN_ENERGY:
				return POT_SEE_INVISIBLE;
			}
			break;
		case POT_ENLIGHTENMENT:
			switch (o2->otyp) {
			    case POT_LEVITATION:
				if (rn2(3)) return POT_GAIN_LEVEL;
				break;
			    case POT_FRUIT_JUICE:
				return POT_BOOZE;
			    case POT_BOOZE:
				return POT_CONFUSION;
			}
			break;
	}

	return 0;
}


boolean
get_wet(obj)
register struct obj *obj;
/* returns TRUE if something happened (potion should be used up) */
{
	char Your_buf[BUFSZ];

	if (snuff_lit(obj)) return(TRUE);

	if (obj->greased) {
		grease_protect(obj,(char *)0,FALSE);
		return(FALSE);
	}
	(void) Shk_Your(Your_buf, obj);
	/* (Rusting and diluting unpaid shop goods ought to be charged for.) */
	switch (obj->oclass) {
	    case WEAPON_CLASS:
		if (!obj->oerodeproof && is_rustprone(obj) &&
		    (obj->oeroded < MAX_ERODE) && !rn2(10)) {
/*JP
			pline("%s %s some%s.",
			      Your_buf, aobjnam(obj, "rust"),
			      obj->oeroded ? " more" : "what");
*/
			pline("%s%sは%s錆びた", Your_buf, xname(obj),
			      obj->oeroded ? "さらに" : "");
			obj->oeroded++;
			return TRUE;
		} else break;
	    case POTION_CLASS:
		if (obj->otyp == POT_WATER) return FALSE;
/*JP
		pline("%s %s%s.", Your_buf, aobjnam(obj,"dilute"),
		      obj->odiluted ? " further" : "");
*/
		pline("%s%sは%s薄まった．", Your_buf, xname(obj),
		      obj->odiluted ? "さらに" : "");
		if (obj->odiluted) {
			obj->odiluted = 0;
#ifdef UNIXPC
			obj->blessed = FALSE;
			obj->cursed = FALSE;
#else
			obj->blessed = obj->cursed = FALSE;
#endif
			obj->otyp = POT_WATER;
		} else obj->odiluted++;
		return TRUE;
	    case SCROLL_CLASS:
		if (obj->otyp != SCR_BLANK_PAPER
#ifdef MAIL
		    && obj->otyp != SCR_MAIL
#endif
		    ) {
			if (!Blind) {
/*JP				boolean oq1 = obj->quan == 1L;*/
/*JP				pline_The("scroll%s fade%s.",
					oq1 ? "" : "s",
					oq1 ? "s" : "");*/
				pline("巻物の文字は薄れた．");
			}
			if(obj->unpaid) {
			    subfrombill(obj, shop_keeper(*u.ushops));
/*JP			    You("erase it, you pay for it.");*/
			    You("文字を消してしまった，賠償せねばならない．");
			    bill_dummy_object(obj);
			}
			obj->otyp = SCR_BLANK_PAPER;
			obj->spe = 0;
			return TRUE;
		} else break;
	    case SPBOOK_CLASS:
		if (obj->otyp != SPE_BLANK_PAPER) {

			if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
/*JP	pline("%s suddenly heats up; steam rises and it remains dry.",*/
	pline("%sは突然熱くなり，蒸気がたちこめ，乾いてしまった．",
				The(xname(obj)));
			} else {
			    if (!Blind) {
/*JP				    boolean oq1 = obj->quan == 1L;*/
/*JP				    pline_The("spellbook%s fade%s.",
					oq1 ? "" : "s", oq1 ? "s" : "");*/
				    pline("魔法書の文字は薄れた．");
			    }
			    if(obj->unpaid) {
			        subfrombill(obj, shop_keeper(*u.ushops));
/*JP			        You("erase it, you pay for it.");*/
			        You("文字を消してしまった，賠償せねばならない．");
			        bill_dummy_object(obj);
			    }
			    obj->otyp = SPE_BLANK_PAPER;
			}
			return TRUE;
		}
	}
/*JP
	pline("%s %s wet.", Your_buf, aobjnam(obj,"get"));
*/
	pline("%s%sは濡れた", Your_buf, xname(obj));
	return FALSE;
}

int
dodip()
{
	register struct obj *potion, *obj;
	const char *tmp;
	uchar here;
	char allowall[2];
	short mixture;
	char qbuf[QBUFSZ], Your_buf[BUFSZ];

	allowall[0] = ALL_CLASSES; allowall[1] = '\0';
/*JP	if(!(obj = getobj(allowall, "dip")))*/
	if(!(obj = getobj(allowall, "浸す")))
		return(0);

	here = levl[u.ux][u.uy].typ;
	/* Is there a fountain to dip into here? */
	if (IS_FOUNTAIN(here)) {
/*JP		if(yn("Dip it into the fountain?") == 'y') {*/
		if(yn("泉に浸しますか？") == 'y') {
			dipfountain(obj);
			return(1);
		}
	} else if (is_pool(u.ux,u.uy)) {
/*JP		tmp = (here == POOL) ? "pool" : "moat";*/
		tmp = (here == POOL) ? "水たまり" : "堀";
/*JP		Sprintf(qbuf, "Dip it into the %s?", tmp);*/
		Sprintf(qbuf, "%sに浸しますか？", tmp);
		if (yn(qbuf) == 'y') {
		    if (Levitation)
			floating_above(tmp);
		    else
			(void) get_wet(obj);
		    return 1;
		}
	}

/*JP
	if(!(potion = getobj(beverages, "dip into")))
*/
	if(!(potion = getobj(beverages, "に浸す")))
		return(0);
	if (potion == obj && potion->quan == 1L) {
/*JP
		pline("That is a potion bottle, not a Klein bottle!");
*/
		pline("これは薬瓶だ！クラインの壷じゃない！");
		return 0;
	}
	if(potion->otyp == POT_WATER) {
		boolean useeit = !Blind;
		if (useeit) (void) Shk_Your(Your_buf, obj);
		if (potion->blessed) {
			if (obj->cursed) {
				if (useeit)
/*JP
				    pline("%s %s %s.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  hcolor(amber));
*/
				    pline("%s%sはうっすらと%s輝いた．",
					  Your_buf,
					  xname(obj),
					  jconj_adj(hcolor("琥珀色の")));
				uncurse(obj);
				obj->bknown=1;
	poof:
				if(!(objects[potion->otyp].oc_name_known) &&
				   !(objects[potion->otyp].oc_uname))
					docall(potion);
				useup(potion);
				return(1);
			} else if(!obj->blessed) {
				if (useeit) {
				    tmp = hcolor(light_blue);
/*JP
				    pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "softly glow"),
					  index(vowels, *tmp) ? "n" : "", tmp);
*/
				    pline("%s%sはぼんやりとした%sオーラにつつまれた",
					  Your_buf, xname(obj), tmp);
				}
				bless(obj);
				obj->bknown=1;
				goto poof;
			}
		} else if (potion->cursed) {
			if (obj->blessed) {
				if (useeit)
/*JP
				    pline("%s %s %s.",
					  Your_buf,
					  aobjnam(obj, "glow"),
					  hcolor((const char *)"brown"));
*/
				    pline("%s%sは%s輝いた",
					  Your_buf, xname(obj),
					  jconj_adj(hcolor((const char *)"茶色の")));
				unbless(obj);
				obj->bknown=1;
				goto poof;
			} else if(!obj->cursed) {
				if (useeit) {
				    tmp = hcolor(Black);
/*JP
				    pline("%s %s with a%s %s aura.",
					  Your_buf,
					  aobjnam(obj, "glow"),
					  index(vowels, *tmp) ? "n" : "", tmp);
*/
				    pline("%s%sは%sオーラにつつまれた．",
					  Your_buf, xname(obj),tmp);
				}
				curse(obj);
				obj->bknown=1;
				goto poof;
			}
		} else
			if (get_wet(obj))
			    goto poof;
	}
	else if(obj->oclass == POTION_CLASS && obj->otyp != potion->otyp) {
		/* Mixing potions is dangerous... */
/*JP		pline_The("potions mix...");*/
		pline("薬は調合された．．．");
		if (obj->cursed || !rn2(10)) {
/*JP			pline("BOOM!  They explode!");*/
			pline("バーン！爆発した！");
			exercise(A_STR, FALSE);
			potionbreathe(obj);
			useup(obj);
			useup(potion);
/*JP			losehp(rnd(10), "alchemic blast", KILLED_BY_AN);*/
			losehp(rnd(10), "調合の失敗で", KILLED_BY_AN);
			return(1);
		}

		obj->blessed = obj->cursed = obj->bknown = 0;
		if (Blind || Hallucination) obj->dknown = 0;

		if ((mixture = mixtype(obj, potion)) != 0) {
			obj->otyp = mixture;
		} else {
		    switch (obj->odiluted ? 1 : rnd(8)) {
			case 1:
				obj->otyp = POT_WATER;
				break;
			case 2:
			case 3:
				obj->otyp = POT_SICKNESS;
				break;
			case 4:
				{
				  struct obj *otmp;
				  otmp = mkobj(POTION_CLASS,FALSE);
				  obj->otyp = otmp->otyp;
				  obfree(otmp, (struct obj *)0);
				}
				break;
			default:
				if (!Blind)
/*JP			  pline_The("mixture glows brightly and evaporates.");*/
				  pline("混ぜると薬は明るく輝き蒸発した．");
				useup(obj);
				useup(potion);
				return(1);
		    }
		}


		obj->odiluted = (obj->otyp != POT_WATER);

		if (obj->otyp == POT_WATER && !Hallucination) {
/*JP			pline_The("mixture bubbles%s.",
				Blind ? "" : ", then clears");*/
			pline("薬を混ぜると%s泡だった．",
				Blind ? "" : "しばらく");
		} else if (!Blind) {
/*JP			pline_The("mixture looks %s.",
				hcolor(OBJ_DESCR(objects[obj->otyp])));*/
			pline("混ぜた薬は%sに見える．",
				jtrns_obj('!',OBJ_DESCR(objects[obj->otyp])));
		}

		useup(potion);
		return(1);
	}

	if(obj->oclass == WEAPON_CLASS && obj->otyp <= SHURIKEN) {
	    if(potion->otyp == POT_SICKNESS && !obj->opoisoned) {
		char buf[BUFSZ];
		Strcpy(buf, The(xname(potion)));
/*JP		pline("%s form%s a coating on %s.",
			buf, potion->quan == 1L ? "s" : "", the(xname(obj)));*/
		pline("%sが%sに染みこんだ．",
			buf, the(xname(obj)));
		obj->opoisoned = TRUE;
		goto poof;
	    } else if(obj->opoisoned &&
		      (potion->otyp == POT_HEALING ||
		       potion->otyp == POT_EXTRA_HEALING)) {
/*JP		pline("A coating wears off %s.", the(xname(obj)));*/
		pline("毒が%sから剥げおちた．", the(xname(obj)));
		obj->opoisoned = 0;
		goto poof;
	    }
	}

	if (potion->otyp == POT_OIL &&
		(obj->oclass == WEAPON_CLASS || is_weptool(obj))) {
	    boolean wisx = FALSE;
	    if (potion->lamplit) {	/* burning */
		int omat = objects[obj->otyp].oc_material;
		if (obj->oerodeproof || obj_resists(obj, 5, 95) ||
			/* `METAL' should not be confused with is_metallic() */
			omat == METAL || omat == MITHRIL || omat == BONE) {
/*JP
		    pline("%s seem%s to burn for a moment.",
			  Yname2(obj),
			  (obj->quan > 1L) ? "" : "s");
*/
		    pline("%sはしばらくの間燃えた．", Yname2(obj));
		} else {
		    if (omat == PLASTIC) obj->oeroded = MAX_ERODE;
/*JP
		    pline_The("burning oil %s %s.",
			    obj->oeroded == MAX_ERODE ? "destroys" : "damages",
			    yname(obj));
*/
		    pline("%sは燃えている油によって%s．",
			    yname(obj),
			    obj->oeroded == MAX_ERODE ? "破壊された" : "傷つけられた");
		    if (obj->oeroded == MAX_ERODE) {
			obj_extract_self(obj);
			obfree(obj, (struct obj *)0);
			obj = (struct obj *) 0;
		    } else {
			/* should check for and do something about
			   damaging unpaid shop goods here */
			obj->oeroded++;
		    }
		}
	    } else if (potion->cursed || !is_metallic(obj) ||
		    /* arrows,&c are classed as metallic due to arrowhead
		       material, but dipping in oil shouldn't repair them */
		    objects[obj->otyp].oc_wepcat == WEP_AMMO) {
/*JP
		pline_The("potion spills and covers your %s with oil.",
			  makeplural(body_part(FINGER)));
*/
		pline("油は飛び散りあなたの%sにかかった．",
  		       makeplural(body_part(FINGER)));
		incr_itimeout(&Glib, d(2,10));
	    } else if (!obj->oeroded) {
		/* uses up potion, doesn't set obj->greased */
/*JP
		pline("%s gleam%s with an oily sheen.",
		      Yname2(obj),
		      (obj->quan > 1L) ? "" : "s");
*/
		pline("%sは油の光沢できらりと光った．",
		      Yname2(obj));
	    } else {
/*JP
		pline("%s %s less %s.",
		      Yname2(obj),
		      (obj->quan > 1L) ? "are" : "is",
		      is_corrodeable(obj) ? "corroded" : "rusty");
*/
		pline("%sの錆が取れた．", Yname2(obj));
		obj->oeroded--;
		wisx = TRUE;
	    }
	    exercise(A_WIS, wisx);
	    makeknown(potion->otyp);
	    useup(potion);
	    return 1;
	}

	/* Allow filling of MAGIC_LAMPs to prevent identification by player */
	if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP) &&
	   (potion->otyp == POT_OIL)) {
	    /* Turn off engine before fueling, turn off fuel too :-)  */
	    if (obj->lamplit || potion->lamplit) {
		useup(potion);
		explode(u.ux, u.uy, 11, d(6,6), 0);
		exercise(A_WIS, FALSE);
		return 1;
	    }
	    /* Adding oil to an empty magic lamp renders it into an oil lamp */
	    if ((obj->otyp == MAGIC_LAMP) && obj->spe == 0) {
		obj->otyp = OIL_LAMP;
		obj->age = 0;
	    }
	    if (obj->age > 1000L) {
/*JP
		pline("%s is full.", Yname2(obj));
*/
		pline("%sには沢山入っている．", Yname2(obj));
	    } else {
/*JP
		You("fill %s with oil.", yname(obj));
*/
		You("%sに油を入れた．", yname(obj));
		check_unpaid(potion);	/* surcharge for using unpaid item */
		obj->age += 2*potion->age;	/* burns more efficiently */
		if (obj->age > 1500L) obj->age = 1500L;
		useup(potion);
		exercise(A_WIS, TRUE);
	    }
	    obj->spe = 1;
	    update_inventory();
	    return 1;
	}

	if ((obj->otyp == UNICORN_HORN || obj->otyp == AMETHYST) &&
	    (mixture = mixtype(obj, potion)) != 0) {
		/* with multiple merged potions, we should split off one and
		   just clear it, but clearing them all together is easier */
/*JP		boolean more_than_one = potion->quan > 1L;*/
		potion->otyp = mixture;
		potion->blessed = 0;
		if (mixture == POT_WATER)
		    potion->cursed = potion->odiluted = 0;
		else
		    potion->cursed = obj->cursed;  /* odiluted left as-is */
		if (Blind)
			potion->dknown = FALSE;
		else {
			if (mixture == POT_WATER &&
#ifdef DCC30_BUG
			    (potion->dknown = !Hallucination,
			     potion->dknown != 0))
#else
			    (potion->dknown = !Hallucination) != 0)
#endif
/*JP				pline_The("potion%s clear%s.",
					more_than_one ? "s" : "",
					more_than_one ? "" : "s");*/
				pline("薬は透明になった．");
			else
/*JP				pline_The("potion%s turn%s %s.",
					more_than_one ? "s" : "",
					more_than_one ? "" : "s",
					hcolor(OBJ_DESCR(objects[mixture])));*/
				pline("薬は%sになった．",
					jconj_adj(jtrns_obj('!', hcolor(OBJ_DESCR(objects[mixture])))));
		}
		return(1);
	}

/*JP	pline("Interesting...");*/
	pline("面白い．．．");
	return(1);
}


void
djinni_from_bottle(obj)
register struct obj *obj;
{
	struct monst *mtmp;
	int chance;

	if(!(mtmp = makemon(&mons[PM_DJINNI], u.ux, u.uy, NO_MM_FLAGS))){
/*JP		pline("It turns out to be empty.");*/
		pline("薬は空っぽだった．");
		return;
	}

	if (!Blind) {
/*JP		pline("In a cloud of smoke, %s emerges!", a_monnam(mtmp));
		pline("%s speaks.", Monnam(mtmp));*/
		pline("煙の中から，%sが現われた！", a_monnam(mtmp));
		pline("%sは話した．", Monnam(mtmp));
	} else {
/*JP		You("smell acrid fumes.");
		pline("%s speaks.", Something);*/
		You("ツンとする匂いがした．");
		pline("%sが話しかけた．", Something);
	}

	chance = rn2(5);
	if (obj->blessed) chance = (chance == 4) ? rnd(4) : 0;
	else if (obj->cursed) chance = (chance == 0) ? rn2(4) : 4;
	/* 0,1,2,3,4:  b=80%,5,5,5,5; nc=20%,20,20,20,20; c=5%,5,5,5,80 */

	switch (chance) {
/*JP	case 0 : verbalize("I am in your debt.  I will grant one wish!");*/
	case 0 : verbalize("お前には借りができた．１つ願いをかなえてやろう！");
		makewish();
		mongone(mtmp);
		break;
/*JP	case 1 : verbalize("Thank you for freeing me!");*/
	case 1 : verbalize("私を助けてくれたことを感謝する！");
		(void) tamedog(mtmp, (struct obj *)0);
		break;
/*JP	case 2 : verbalize("You freed me!");*/
	case 2 : verbalize("解放してくれたのはお前か！");
		mtmp->mpeaceful = TRUE;
		set_malign(mtmp);
		break;
/*JP	case 3 : verbalize("It is about time!");*/
	case 3 : verbalize("さらばだ！");
/*JP		pline("%s vanishes.", Monnam(mtmp));*/
		pline("%sは消えた．", Monnam(mtmp));
		mongone(mtmp);
		break;
/*JP	default: verbalize("You disturbed me, fool!");*/
	default: verbalize("おまえは私の眠りの邪魔をした．おろかものめ！");
		break;
	}
}

#endif /* OVLB */

/*potion.c*/
