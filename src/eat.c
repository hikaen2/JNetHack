/*	SCCS Id: @(#)eat.c	3.2	96/09/14	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
/* #define DEBUG	*//* uncomment to enable new eat code debugging */

#ifdef DEBUG
# ifdef WIZARD
#define debugpline	if (wizard) pline
# else
#define debugpline	pline
# endif
#endif

STATIC_PTR int NDECL(eatmdone);
STATIC_PTR int NDECL(eatfood);
STATIC_PTR int NDECL(opentin);
STATIC_PTR int NDECL(unfaint);

#ifdef OVLB
static const char *FDECL(food_xname, (struct obj *,BOOLEAN_P));
static void FDECL(choke, (struct obj *));
static void NDECL(recalc_wt);
static struct obj *FDECL(touchfood, (struct obj *));
static void NDECL(do_reset_eat);
static void FDECL(done_eating, (BOOLEAN_P));
static void FDECL(cprefx, (int));
static int FDECL(intrinsic_possible, (int,struct permonst *));
static void FDECL(givit, (int,struct permonst *));
static void FDECL(cpostfx, (int));
static void FDECL(start_tin, (struct obj *));
static int FDECL(eatcorpse, (struct obj *));
static void FDECL(start_eating, (struct obj *));
static void FDECL(fprefx, (struct obj *));
static void FDECL(fpostfx, (struct obj *));
static int NDECL(bite);

static int FDECL(rottenfood, (struct obj *));
static void NDECL(eatspecial);
static void FDECL(eataccessory, (struct obj *));
static const char * FDECL(foodword, (struct obj *));

char msgbuf[BUFSZ];

#endif /* OVLB */

/* hunger texts used on bottom line (each 8 chars long) */
#define SATIATED	0
#define NOT_HUNGRY	1
#define HUNGRY		2
#define WEAK		3
#define FAINTING	4
#define FAINTED		5
#define STARVED		6

#ifndef OVLB

STATIC_DCL NEARDATA const char comestibles[];
STATIC_DCL NEARDATA const char allobj[];
STATIC_DCL boolean force_save_hs;

#else

STATIC_OVL NEARDATA const char comestibles[] = { FOOD_CLASS, 0 };

/* Gold must come first for getobj(). */
STATIC_OVL NEARDATA const char allobj[] = {
	GOLD_CLASS, WEAPON_CLASS, ARMOR_CLASS, POTION_CLASS, SCROLL_CLASS,
	WAND_CLASS, RING_CLASS, AMULET_CLASS, FOOD_CLASS, TOOL_CLASS,
	GEM_CLASS, ROCK_CLASS, BALL_CLASS, CHAIN_CLASS, SPBOOK_CLASS, 0 };

STATIC_OVL boolean force_save_hs = FALSE;

/*JP const char *hu_stat[] = {
	"Satiated",
	"        ",
	"Hungry  ",
	"Weak    ",
	"Fainting",
	"Fainted ",
	"Starved "
}; */
const char *hu_stat[] = {
	"満腹    ",
	"        ",
	"ぺこぺこ",
	"衰弱    ",
	"ふらふら",
	"卒倒    ",
	"餓死    "
};

#endif /* OVLB */
#ifdef OVL1

/*
 * Decide whether a particular object can be eaten by the possibly
 * polymorphed character.  Not used for monster checks.
 */
boolean
is_edible(obj)
register struct obj *obj;
{
	/* protect invocation tools but not Rider corpses (handled elsewhere)*/
     /* if (obj->oclass != FOOD_CLASS && obj_resists(obj, 0, 0)) */
	if (objects[obj->otyp].oc_unique)
		return FALSE;
	/* above also prevents the Amulet from being eaten, so we must never
	   allow fake amulets to be eaten either [which is already the case] */

	if (metallivorous(uasmon) && is_metallic(obj))
		return TRUE;
	if (u.umonnum == PM_GELATINOUS_CUBE && is_organic(obj) &&
		/* [g.cubes can eat containers and retain all contents
		    as engulged items, but poly'd player can't do that] */
	    !Has_contents(obj))
		return TRUE;

     /* return((boolean)(!!index(comestibles, obj->oclass))); */
	return (boolean)(obj->oclass == FOOD_CLASS);
}

#endif /* OVL1 */
#ifdef OVLB

void
init_uhunger()
{
	u.uhunger = 900;
	u.uhs = NOT_HUNGRY;
}

/*JP static const struct { const char *txt; int nut; } tintxts[] = {
	{"deep fried",	 60},
	{"pickled",	 40},
	{"soup made from", 20},
	{"pureed",	500},
	{"rotten",	-50},
	{"homemade",	 50},
	{"", 0}
}; */
static const struct { const char *txt; int nut; } tintxts[] = {
	{"の揚げ物",	 60},
	{"の漬物",	 40},
	{"のスープ",	 20},
	{"のピューレ",	500},
	{"腐った",	-50},
	{"自家製の",	 50},
	{"", 0}
};
#define TTSZ	SIZE(tintxts)

static NEARDATA struct {
	struct	obj *tin;
	int	usedtime, reqtime;
} tin;

static NEARDATA struct {
	struct	obj *piece;	/* the thing being eaten, or last thing that
				 * was partially eaten, unless that thing was
				 * a tin, which uses the tin structure above */
	int	usedtime,	/* turns spent eating */
		reqtime;	/* turns required to eat */
	int	nmod;		/* coded nutrition per turn */
	Bitfield(canchoke,1);	/* was satiated at beginning */
	Bitfield(fullwarn,1);	/* have warned about being full */
	Bitfield(eating,1);	/* victual currently being eaten */
	Bitfield(doreset,1);	/* stop eating at end of turn */
} victual;

static char *eatmbuf = 0;	/* set by cpostfx() */

STATIC_PTR
int
eatmdone()		/* called after mimicing is over */
{
	/* release `eatmbuf' */
	if (eatmbuf) {
	    if (nomovemsg == eatmbuf) nomovemsg = 0;
	    free((genericptr_t)eatmbuf),  eatmbuf = 0;
	}
	/* update display */
	if (u.usym == 0) {
	    u.usym = Upolyd ? uasmon->mlet : S_HUMAN;
	    newsym(u.ux,u.uy);
	}
	return 0;
}

/* ``[the(] singular(food, xname) [)]'' with awareness of unique monsters */
static const char *
food_xname(food, the_pfx)
struct obj *food;
boolean the_pfx;
{
	const char *result;
#if 0	/*JP*/
	int mnum = food->corpsenm;

	if (food->otyp == CORPSE && (mons[mnum].geno & G_UNIQ)) {
	    /* grab xname()'s modifiable return buffer for our own use */
	    char *bufp = xname(food);
/*JP
	    Sprintf(bufp, "%s%s corpse",
		    (the_pfx && !type_is_pname(&mons[mnum])) ? "the " : "",
		    s_suffix(mons[mnum].mname));
*/
	    Sprintf(bufp, "%sの死体",
		    s_suffix(mons[mnum].mname));
	    result = bufp;
	} else {
#endif
	    /* the ordinary case */
	    result = singular(food, xname);
	    if (the_pfx) result = the(result);
#if 0	/*JP*/
	}
#endif
	return result;
}

/* Created by GAN 01/28/87
 * Amended by AKP 09/22/87: if not hard, don't choke, just vomit.
 * Amended by 3.  06/12/89: if not hard, sometimes choke anyway, to keep risk.
 *		  11/10/89: if hard, rarely vomit anyway, for slim chance.
 */
static void
choke(food)	/* To a full belly all food is bad. (It.) */
	register struct obj *food;
{
	/* only happens if you were satiated */
	if (u.uhs != SATIATED) {
		if (food->otyp != AMULET_OF_STRANGULATION)
			return;
	} else {
		if (Role_is('K') && u.ualign.type == A_LAWFUL)
			adjalign(-1);		/* gluttony is unchivalrous */
	}

	exercise(A_CON, FALSE);

	if (Breathless || (!Strangled && !rn2(20))) {
		/* choking by eating AoS doesn't involve stuffing yourself */
		if (food->otyp == AMULET_OF_STRANGULATION) {
/*JP			You("choke, but recover your composure.");*/
			You("首を絞められた．しかしなんともなかった．");
			return;
		}
/*JP		You("stuff yourself and then vomit voluminously.");*/
		pline("がつがつと口に詰め込んだが, ドバっと吐き出してしまった．");
		morehungry(1000);	/* you just got *very* sick! */
		vomit();
	} else {
		killer_format = KILLED_BY_AN;
		/*
		 * Note all "killer"s below read "Choked on %s" on the
		 * high score list & tombstone.  So plan accordingly.
		 */
		if(food) {
/*JP
			You("choke over your %s.", foodword(food));
*/
			You("%sを喉に詰まらせてしまった．", foodword(food));
			if (food->oclass == GOLD_CLASS) {
/*JP
				killer = "a very rich meal";
*/
				killer = "とても豪華な料理で";
			} else {
				killer = food_xname(food, FALSE);
			}
		} else {
/*JP
			You("choke over it.");
			killer = "quick snack";
*/
			pline("喉に詰まらせてしまった．");
			killer = "早食いで";
		}
/*JP
		You("die...");
*/
		pline("あなたは死にました．．．");
		done(CHOKING);
	}
}

static void
recalc_wt()	/* modify object wt. depending on time spent consuming it */
{
	register struct obj *piece = victual.piece;

#ifdef DEBUG
	debugpline("Old weight = %d", piece->owt);
	debugpline("Used time = %d, Req'd time = %d",
		victual.usedtime, victual.reqtime);
#endif
	/* weight(piece) = weight of full item */
	if(victual.usedtime)
	    piece->owt = eaten_stat(weight(piece), piece);
#ifdef DEBUG
	debugpline("New weight = %d", piece->owt);
#endif
}

void
reset_eat()		/* called when eating interrupted by an event */
{
    /* we only set a flag here - the actual reset process is done after
     * the round is spent eating.
     */
	if(victual.eating && !victual.doreset) {
#ifdef DEBUG
	    debugpline("reset_eat...");
#endif
	    victual.doreset = TRUE;
	}
	return;
}

static struct obj *
touchfood(otmp)
register struct obj *otmp;
{
	if (otmp->quan > 1L) {
	    if(!carried(otmp))
		(void) splitobj(otmp, 1L);
	    else
		otmp = splitobj(otmp, otmp->quan - 1L);
#ifdef DEBUG
	    debugpline("split object,");
#endif
	}

	if (!otmp->oeaten) {
	    if(((!carried(otmp) && costly_spot(otmp->ox, otmp->oy) &&
		 !otmp->no_charge)
		 || otmp->unpaid) &&
		 (otmp->otyp == CORPSE || objects[otmp->otyp].oc_delay > 1)) {
		/* create a dummy duplicate to put on bill */
/*JP		verbalize("You bit it, you bought it!");*/
		verbalize("喰ったならお買いあげいただこう！");
		bill_dummy_object(otmp);
		otmp->no_charge = 1;	/* you now own this */
	    }
	    otmp->oeaten = (otmp->otyp == CORPSE ?
				mons[otmp->corpsenm].cnutrit :
				objects[otmp->otyp].oc_nutrition);
	}

	if (carried(otmp)) {
	    freeinv(otmp);
	    if (inv_cnt() >= 52 && !merge_choice(invent, otmp))
		dropy(otmp);
	    else
		otmp = addinv(otmp); /* unlikely but a merge is possible */
	}
	return(otmp);
}

/* When food decays, in the middle of your meal, we don't want to dereference
 * any dangling pointers, so set it to null (which should still trigger
 * do_reset_eat() at the beginning of eatfood()) and check for null pointers
 * in do_reset_eat().
 */
void
food_disappears(obj)
register struct obj *obj;
{
	if (obj == victual.piece) victual.piece = (struct obj *)0;
	if (obj->timed) obj_stop_timers(obj);
}

static void
do_reset_eat()
{
#ifdef DEBUG
	debugpline("do_reset_eat...");
#endif
	if (victual.piece) {
		victual.piece = touchfood(victual.piece);
		recalc_wt();
	}
	victual.fullwarn = victual.eating = victual.doreset = FALSE;
	/* Do not set canchoke to FALSE; if we continue eating the same object
	 * we need to know if canchoke was set when they started eating it the
	 * previous time.  And if we don't continue eating the same object
	 * canchoke always gets recalculated anyway.
	 */
	stop_occupation();
	newuhs(FALSE);
}

STATIC_PTR
int
eatfood()		/* called each move during eating process */
{
	if(!victual.piece ||
	 (!carried(victual.piece) && !obj_here(victual.piece, u.ux, u.uy))) {
		/* maybe it was stolen? */
		do_reset_eat();
		return(0);
	}
	if(!victual.eating) return(0);

	if(++victual.usedtime <= victual.reqtime) {
	    if(bite()) return(0);
	    return(1);	/* still busy */
	} else {	/* done */
	    done_eating(TRUE);
	    return(0);
	}
}

static void
done_eating(message)
boolean message;
{
#ifndef NO_SIGNAL
	victual.piece->in_use = TRUE;
#endif
	occupation = 0; /* do this early, so newuhs() knows we're done */
	newuhs(FALSE);
	if (nomovemsg) {
		if (message) pline(nomovemsg);
		nomovemsg = 0;
	} else if (message)
/*JP
		You("finish eating %s.", food_xname(victual.piece, TRUE));
*/
		You("%sを食べ終えた．",  food_xname(victual.piece, TRUE));

	if(victual.piece->otyp == CORPSE)
		cpostfx(victual.piece->corpsenm);
	else
		fpostfx(victual.piece);

	if (carried(victual.piece)) useup(victual.piece);
	else useupf(victual.piece);
	victual.piece = (struct obj *) 0;
	victual.fullwarn = victual.eating = victual.doreset = FALSE;
}

static void
cprefx(pm)
register int pm;
{
	boolean fix_petrification = FALSE;

	if (Role_is('E') ? is_elf(&mons[pm]) : is_human(&mons[pm])) {
		if (uasmon != &playermon) {
/*JP			You("have a bad feeling deep inside.");*/
			You("嫌悪感におそわれた．");
		}
/*JP		You("cannibal!  You will regret this!");*/
		pline("共喰いだ！後悔するぞ！");
		Aggravate_monster |= FROMOUTSIDE;
		change_luck(-rn1(4,2));		/* -5..-2 */
	}

	switch(pm) {
	    case PM_LITTLE_DOG:
	    case PM_DOG:
	    case PM_LARGE_DOG:
	    case PM_KITTEN:
	    case PM_HOUSECAT:
	    case PM_LARGE_CAT:
/*JP		You_feel("that eating the %s was a bad idea.", mons[pm].mname); */
		pline("%sを食べるのはよくない気がした．", jtrns_mon(mons[pm].mname, -1));
		Aggravate_monster |= FROMOUTSIDE;
		break;
	    case PM_COCKATRICE:
	    case PM_MEDUSA:
		if (!resists_ston(&youmonst) &&
		    !(poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM))) {
		    char kbuf[BUFSZ];

/*JP
		    Sprintf(kbuf, "tasting %s meat", mons[pm].mname);
*/
		    Sprintf(kbuf, "%sの肉を食べ", jtrns_mon(mons[pm].mname, -1));
		    killer_format = KILLED_BY;
		    killer = kbuf;
/*JP
		    You("turn to stone.");
*/
		    You("石化した．");
		    done(STONING);
		}
		break;
	    case PM_LIZARD:
		if (Stoned) fix_petrification = TRUE;
		break;
	    case PM_DEATH:
	    case PM_PESTILENCE:
	    case PM_FAMINE:
		{ char buf[BUFSZ];
/*JP		    pline("Eating that is instantly fatal."); */
		    pline("食べたらすぐに死んでしまった．");
/*JP		    Sprintf(buf, "unwisely ate the body of %s",*/
		    Sprintf(buf, "愚かにも%sを食べて",
			    jtrns_mon(mons[pm].mname,-1));
		    killer = buf;
/*JP		    killer_format = NO_KILLER_PREFIX;*/
		    killer_format = KILLED_BY;
		    done(DIED);
		    /* It so happens that since we know these monsters */
		    /* cannot appear in tins, victual.piece will always */
		    /* be what we want, which is not generally true. */
		    (void) revive_corpse(victual.piece);
		    return;
		}
	    default:
		if (acidic(&mons[pm]) && Stoned)
		    fix_petrification = TRUE;
		break;
	}

	if (fix_petrification) {
	    Stoned = 0;
	    if (!Hallucination)
/*JP		You_feel("limber!");*/
		You("体が軟らかくなったような気がした！");
	    else
/*JP		pline("What a pity - you just ruined a future piece of %sart!",
		      ACURR(A_CHA) > 15 ? "fine " : "");*/
		pline("なんてことだ！%s芸術作品になれたかもしれないのに！",
		      ACURR(A_CHA) > 15 ? "貴重な" : "");
	}

	return;
}


/*
 * If you add an intrinsic that can be gotten by eating a monster, add it
 * to intrinsic_possible() and givit().  (It must already be in prop.h to
 * be an intrinsic property.)
 * It would be very easy to make the intrinsics not try to give you one
 * that you already had by checking to see if you have it in
 * intrinsic_possible() instead of givit().
 */

/* intrinsic_possible() returns TRUE iff a monster can give an intrinsic. */
static int
intrinsic_possible(type, ptr)
int type;
register struct permonst *ptr;
{
	switch (type) {
	    case FIRE_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_FIRE) {
			debugpline("can get fire resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_FIRE);
#endif
	    case SLEEP_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_SLEEP) {
			debugpline("can get sleep resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_SLEEP);
#endif
	    case COLD_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_COLD) {
			debugpline("can get cold resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_COLD);
#endif
	    case DISINT_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_DISINT) {
			debugpline("can get disintegration resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_DISINT);
#endif
	    case SHOCK_RES:	/* shock (electricity) resistance */
#ifdef DEBUG
		if (ptr->mconveys & MR_ELEC) {
			debugpline("can get shock resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_ELEC);
#endif
	    case POISON_RES:
#ifdef DEBUG
		if (ptr->mconveys & MR_POISON) {
			debugpline("can get poison resistance");
			return(TRUE);
		} else  return(FALSE);
#else
		return(ptr->mconveys & MR_POISON);
#endif
	    case TELEPORT:
#ifdef DEBUG
		if (can_teleport(ptr)) {
			debugpline("can get teleport");
			return(TRUE);
		} else  return(FALSE);
#else
		return(can_teleport(ptr));
#endif
	    case TELEPORT_CONTROL:
#ifdef DEBUG
		if (control_teleport(ptr)) {
			debugpline("can get teleport control");
			return(TRUE);
		} else  return(FALSE);
#else
		return(control_teleport(ptr));
#endif
	    case TELEPAT:
#ifdef DEBUG
		if (telepathic(ptr)) {
			debugpline("can get telepathy");
			return(TRUE);
		} else  return(FALSE);
#else
		return(telepathic(ptr));
#endif
	    default:
		return(FALSE);
	}
	/*NOTREACHED*/
}

/* givit() tries to give you an intrinsic based on the monster's level
 * and what type of intrinsic it is trying to give you.
 */
static void
givit(type, ptr)
int type;
register struct permonst *ptr;
{
	register int chance;

#ifdef DEBUG
	debugpline("Attempting to give intrinsic %d", type);
#endif
	/* some intrinsics are easier to get than others */
	switch (type) {
		case POISON_RES:
			if ((ptr == &mons[PM_KILLER_BEE] ||
					ptr == &mons[PM_SCORPION]) && !rn2(4))
				chance = 1;
			else
				chance = 15;
			break;
		case TELEPORT:
			chance = 10;
			break;
		case TELEPORT_CONTROL:
			chance = 12;
			break;
		case TELEPAT:
			chance = 1;
			break;
		default:
			chance = 15;
			break;
	}

	if (ptr->mlevel <= rn2(chance))
		return;		/* failed die roll */

	switch (type) {
	    case FIRE_RES:
#ifdef DEBUG
		debugpline("Trying to give fire resistance");
#endif
		if(!(HFire_resistance & FROMOUTSIDE)) {
/*JP			You(Hallucination ? "be chillin'." :
			    "feel a momentary chill."); */
		    You(Hallucination ? "「クール宅配便」されているようだ" :
			    "一瞬寒けがした．");
			HFire_resistance |= FROMOUTSIDE;
		}
		break;
	    case SLEEP_RES:
#ifdef DEBUG
		debugpline("Trying to give sleep resistance");
#endif
		if(!(HSleep_resistance & FROMOUTSIDE)) {
/*JP			You_feel("wide awake."); */
		    You("ぱっちり目がさめた．");
		    HSleep_resistance |= FROMOUTSIDE;
		}
		break;
	    case COLD_RES:
#ifdef DEBUG
		debugpline("Trying to give cold resistance");
#endif
		if(!(HCold_resistance & FROMOUTSIDE)) {
/*JP			You_feel("full of hot air."); */
		    You("熱風を全身に感じた．");
			HCold_resistance |= FROMOUTSIDE;
		}
		break;
	    case DISINT_RES:
#ifdef DEBUG
		debugpline("Trying to give disintegration resistance");
#endif
		if(!(HDisint_resistance & FROMOUTSIDE)) {
			You_feel(Hallucination ?
/*JP			    "totally together, man." :
			    "very firm.");*/
			  "世界人類と兄弟になったような気がした．" :
			  "とても頑丈になったような気がした．");
			HDisint_resistance |= FROMOUTSIDE;
		}
		break;
	    case SHOCK_RES:	/* shock (electricity) resistance */
#ifdef DEBUG
		debugpline("Trying to give shock resistance");
#endif
		if(!(HShock_resistance & FROMOUTSIDE)) {
			if (Hallucination)
/*JP				You_feel("grounded in reality."); */
				You("アースされたような気がした．");
			else
/*JP				Your("health currently feels amplified!");*/
				pline("健康が増幅されたような気がした！");
			HShock_resistance |= FROMOUTSIDE;
		}
		break;
	    case POISON_RES:
#ifdef DEBUG
		debugpline("Trying to give poison resistance");
#endif
		if(!(HPoison_resistance & FROMOUTSIDE)) {
/*JP			You_feel("healthy.");*/
			You("健康的になったような気がした．");
			HPoison_resistance |= FROMOUTSIDE;
		}
		break;
	    case TELEPORT:
#ifdef DEBUG
		debugpline("Trying to give teleport");
#endif
		if(!(HTeleportation & FROMOUTSIDE)) {
/*JP			You_feel(Hallucination ? "diffuse." :
			    "very jumpy.");*/
			pline(Hallucination ? "体が飛び散ったような気がした．" :
			    "跳躍力が高まったような気がした．");
			HTeleportation |= FROMOUTSIDE;
		}
		break;
	    case TELEPORT_CONTROL:
#ifdef DEBUG
		debugpline("Trying to give teleport control");
#endif
		if(!(HTeleport_control & FROMOUTSIDE)) {
			You_feel(Hallucination ?
/*JP			    "centered in your personal space." :
			    "in control of yourself.");*/
			    "自己中心的になったような気がした．" :
			    "自分自身を制御できるような気がした．");
			HTeleport_control |= FROMOUTSIDE;
		}
		break;
	    case TELEPAT:
#ifdef DEBUG
		debugpline("Trying to give telepathy");
#endif
		if(!(HTelepat & FROMOUTSIDE)) {
			You_feel(Hallucination ?
/*JP			    "in touch with the cosmos." :
			    "a strange mental acuity.");*/
			    "宇宙の神秘に触れたような気がした．" :
			    "奇妙な精神的鋭さを感じた．");
			HTelepat |= FROMOUTSIDE;
			/* If blind, make sure monsters show up. */
			if (Blind) see_monsters();
		}
		break;
	    default:
#ifdef DEBUG
		debugpline("Tried to give an impossible intrinsic");
#endif
		break;
	}
}

static void
cpostfx(pm)		/* called after completely consuming a corpse */
register int pm;
{
	register int tmp = 0;

	/* in case `afternmv' didn't get called for previously mimicking
	   gold, clean up now to avoid `eatmbuf' memory leak */
	if (eatmbuf) (void)eatmdone();

	switch(pm) {
	    case PM_WRAITH:
		pluslvl();
		break;
	    case PM_HUMAN_WERERAT:
		u.ulycn = PM_WERERAT;
		break;
	    case PM_HUMAN_WEREJACKAL:
		u.ulycn = PM_WEREJACKAL;
		break;
	    case PM_HUMAN_WEREWOLF:
		u.ulycn = PM_WEREWOLF;
		break;
	    case PM_NURSE:
		if (Upolyd) u.mh = u.mhmax;
		else u.uhp = u.uhpmax;
		flags.botl = 1;
		break;
	    case PM_STALKER:
		if(!Invis) {
			set_itimeout(&HInvis, (long)rn1(100, 50));
		} else {
/*JP			if (!(HInvis & INTRINSIC)) You_feel("hidden!");*/
			if (!(HInvis & INTRINSIC)) Your("姿は隠された！");
			HInvis |= FROMOUTSIDE;
			HSee_invisible |= FROMOUTSIDE;
		}
		newsym(u.ux, u.uy);
		/* fall into next case */
	    case PM_YELLOW_LIGHT:
		/* fall into next case */
	    case PM_GIANT_BAT:
		make_stunned(HStun + 30,FALSE);
		/* fall into next case */
	    case PM_BAT:
		make_stunned(HStun + 30,FALSE);
		break;
	    case PM_GIANT_MIMIC:
		tmp += 10;
		/* fall into next case */
	    case PM_LARGE_MIMIC:
		tmp += 20;
		/* fall into next case */
	    case PM_SMALL_MIMIC:
		tmp += 20;
		if (u.usym != S_MIMIC) {
		    char buf[BUFSZ];

/*JP
		    You_cant("resist the temptation to mimic a pile of gold.");
*/
		    You("金貨の山を真似したい誘惑にかられた．");
		    nomul(-tmp);
/*JP
		    Sprintf(buf, "You now prefer mimicking %s again.",
			    an(Upolyd ? uasmon->mname :
				Role_is('E') ? "elf" : "human"));
*/
		    Sprintf(buf, "こんどは%sの真似がしたくなった．",
			    an(Upolyd ? uasmon->mname :
				Role_is('E') ? "エルフ" : "人間"));
		    eatmbuf = strcpy((char *) alloc(strlen(buf) + 1), buf);
		    nomovemsg = eatmbuf;
		    afternmv = eatmdone;
		    u.usym = 0; /* hack! no monster sym 0; use for gold */
		    newsym(u.ux,u.uy);
		    curs_on_u();
		    /* make gold symbol show up now */
		    display_nhwindow(WIN_MAP, TRUE);
		}
		break;
	    case PM_QUANTUM_MECHANIC:
/*JP		Your("velocity suddenly seems very uncertain!");*/
		Your("速度が突然、不確定になった！");
		if (Fast & INTRINSIC) {
			Fast &= ~INTRINSIC;
/*JP			You("seem slower.");*/
			You("遅くなったようだ．");
		} else {
			Fast |= FROMOUTSIDE;
/*JP			You("seem faster.");*/
			You("速くなったようだ．");
		}
		break;
	    case PM_LIZARD:
		if (HStun > 2)  make_stunned(2L,FALSE);
		if (HConfusion > 2)  make_confused(2L,FALSE);
		break;
	    case PM_CHAMELEON:
/*JP		You_feel("a change coming over you.");*/
		pline("変化が訪れた．");
		polyself();
		break;
	    case PM_MIND_FLAYER:
		if (ABASE(A_INT) < ATTRMAX(A_INT)) {
			if (!rn2(2)) {
/*JP				pline("Yum! That was real brain food!");*/
				pline("ウェ！これこそ本当の脳味噌だ！");
				(void) adjattrib(A_INT, 1, FALSE);
				break;	/* don't give them telepathy, too */
			}
		}
		else {
/*JP			pline("For some reason, that tasted bland.");*/
			pline("どうしたわけか，口あたりがいい．");
		}
		/* fall through to default case */
	    default: {
		register struct permonst *ptr = &mons[pm];
		int i, count;

		if (dmgtype(ptr, AD_STUN) || dmgtype(ptr, AD_HALU) ||
		    pm == PM_VIOLET_FUNGUS) {
/*JP			pline ("Oh wow!  Great stuff!");*/
			pline ("ワーォ！なんだこれは！");
			make_hallucinated(HHallucination + 200,FALSE,0L);
		}
		if(is_giant(ptr)) gainstr((struct obj *)0, 0);

		/* Check the monster for all of the intrinsics.  If this
		 * monster can give more than one, pick one to try to give
		 * from among all it can give.
		 *
		 * If a monster can give 4 intrinsics then you have
		 * a 1/1 * 1/2 * 2/3 * 3/4 = 1/4 chance of getting the first,
		 * a 1/2 * 2/3 * 3/4 = 1/4 chance of getting the second,
		 * a 1/3 * 3/4 = 1/4 chance of getting the third,
		 * and a 1/4 chance of getting the fourth.
		 *
		 * And now a proof by induction:
		 * it works for 1 intrinsic (1 in 1 of getting it)
		 * for 2 you have a 1 in 2 chance of getting the second,
		 *	otherwise you keep the first
		 * for 3 you have a 1 in 3 chance of getting the third,
		 *	otherwise you keep the first or the second
		 * for n+1 you have a 1 in n+1 chance of getting the (n+1)st,
		 *	otherwise you keep the previous one.
		 * Elliott Kleinrock, October 5, 1990
		 */

		 count = 0;	/* number of possible intrinsics */
		 tmp = 0;	/* which one we will try to give */
		 for (i = 1; i <= LAST_PROP; i++) {
			if (intrinsic_possible(i, ptr)) {
				count++;
				/* a 1 in count chance of replacing the old
				 * one with this one, and a count-1 in count
				 * chance of keeping the old one.  (note
				 * that 1 in 1 and 0 in 1 are what we want
				 * for the first one
				 */
				if (!rn2(count)) {
#ifdef DEBUG
					debugpline("Intrinsic %d replacing %d",
								i, tmp);
#endif
					tmp = i;
				}
			}
		 }

		 /* if any found try to give them one */
		 if (count) givit(tmp, ptr);
	    }
	    break;
	}
	return;
}

STATIC_PTR
int
opentin()		/* called during each move whilst opening a tin */
{
	register int r;
	const char *what;
	int which;

	if(!carried(tin.tin) && !obj_here(tin.tin, u.ux, u.uy))
					/* perhaps it was stolen? */
		return(0);		/* %% probably we should use tinoid */
	if(tin.usedtime++ >= 50) {
/*JP		You("give up your attempt to open the tin.");*/
		You("缶を開けるのをあきらめた．");
		return(0);
	}
	if(tin.usedtime < tin.reqtime)
		return(1);		/* still busy */
	if(tin.tin->otrapped ||
	   (tin.tin->cursed && tin.tin->spe != -1 && !rn2(8))) {
/*JP		b_trapped("tin", 0);*/
		b_trapped("缶", 0);
		goto use_me;
	}
/*JP	You("succeed in opening the tin.");*/
	You("缶を開けるのに成功した．");
	if(tin.tin->spe != 1) {
	    if (tin.tin->corpsenm == NON_PM) {
/*JP		pline("It turns out to be empty.");*/
		pline("缶は空っぽだった．");
		tin.tin->dknown = tin.tin->known = TRUE;
		goto use_me;
	    }
	    r = tin.tin->cursed ? 4 :		/* Always rotten if cursed */
		    (tin.tin->spe == -1) ? 5 :	/* "homemade" if player made */
			rn2(TTSZ-1);		/* else take your pick */
	    if (tin.tin->spe == -1 && !tin.tin->blessed && !rn2(7))
		r = 4;				/* some homemade tins go bad */
	    which = 0;	/* 0=>plural, 1=>as-is, 2=>"the" prefix */
	    if (Hallucination) {
		what = rndmonnam();
	    } else {
		what = mons[tin.tin->corpsenm].mname;
		if (mons[tin.tin->corpsenm].geno & G_UNIQ)
		    which = type_is_pname(&mons[tin.tin->corpsenm]) ? 1 : 2;
	    }
	    if (which == 0) what = makeplural(what);
/*JP	    pline("It smells like %s%s.", (which == 2) ? "the " : "", what);*/
 	    pline("%sのような匂いがした．", jtrns_mon(what, -1));

/*JP	    if (yn("Eat it?") == 'n') {*/
	    if (yn("食べますか？") == 'n') {
		if (!Hallucination) tin.tin->dknown = tin.tin->known = TRUE;
/*JP		if (flags.verbose) You("discard the open tin.");*/
		if (flags.verbose) You("開けた缶を捨てた．");
		goto use_me;
	    }
	    /* in case stop_occupation() was called on previous meal */
	    victual.piece = (struct obj *)0;
	    victual.fullwarn = victual.eating = victual.doreset = FALSE;

/*JP	    You("consume %s %s.", tintxts[r].txt,
			mons[tin.tin->corpsenm].mname); */
/*JP
	tintextsには最初の4つに，揚げ物，漬物，スープ，ピューレが入って
	おり，これらは「○○のスープ」のように記述するのが自然である．
*/
	    You("%s%sをたいらげた．",
		(r < 4) ? jtrns_mon(mons[tin.tin->corpsenm].mname, -1) : tintxts[r].txt,
		(r < 4) ? tintxts[r].txt : jtrns_mon(mons[tin.tin->corpsenm].mname, -1));
	    tin.tin->dknown = tin.tin->known = TRUE;
	    cprefx(tin.tin->corpsenm); cpostfx(tin.tin->corpsenm);

	    /* check for vomiting added by GAN 01/16/87 */
	    if(tintxts[r].nut < 0) make_vomiting((long)rn1(15,10), FALSE);
	    else lesshungry(tintxts[r].nut);

	    if(r == 0) {			/* Deep Fried */
	        /* Assume !Glib, because you can't open tins when Glib. */
		incr_itimeout(&Glib, rnd(15));
/*JP		pline("Eating deep fried food made your %s very slippery.",*/
		pline("あなたの%sは揚げすぎた食べ物のため滑りやすくなった．",
		      makeplural(body_part(FINGER)));
	    }
	} else {
	    if (tin.tin->cursed)
/*JP		pline("It contains some decaying %s substance.",*/
		pline("%s腐った物体が入っている．",
			hcolor(green));
	    else
/*JP		pline("It contains spinach.");*/
		pline("ホウレン草が入っている．");

/*JP	    if (yn("Eat it?") == 'n') {*/
	    if (yn("食べますか？") == 'n') {
		if (!Hallucination && !tin.tin->cursed)
		    tin.tin->dknown = tin.tin->known = TRUE;
		if (flags.verbose)
/*JP		    You("discard the open tin.");*/
		    You("開けた缶を捨てた．");
		goto use_me;
	    }
	    if (!tin.tin->cursed)
/*JP		pline("This makes you feel like %s!",
		      Hallucination ? "Swee'pea" : "Popeye");*/
		pline("%sのような気分になった！",
		      Hallucination ? "スウィッピー" : "ポパイ");
	    lesshungry(600);
	    gainstr(tin.tin, 0);
	}
	tin.tin->dknown = tin.tin->known = TRUE;
use_me:
	if (carried(tin.tin)) useup(tin.tin);
	else useupf(tin.tin);
	tin.tin = (struct obj *) 0;
	return(0);
}

static void
start_tin(otmp)		/* called when starting to open a tin */
	register struct obj *otmp;
{
	register int tmp;

	if (metallivorous(uasmon)) {
/*JP		You("bite right into the metal tin...");*/
		You("金属の缶を噛みはじめた．．．");
		tmp = 1;
	} else if (nolimbs(uasmon)) {
/*JP		You("cannot handle the tin properly to open it.");*/
		You("缶をうまく開けられない．");
		return;
	} else if (otmp->blessed) {
/*JP		pline_The("tin opens like magic!");*/
		pline("缶は魔法のように開いた！");
		tmp = 1;
	} else if(uwep) {
		switch(uwep->otyp) {
		case TIN_OPENER:
			tmp = 1;
			break;
		case DAGGER:
		case ELVEN_DAGGER:
		case ORCISH_DAGGER:
		case ATHAME:
		case CRYSKNIFE:
			tmp = 3;
			break;
		case PICK_AXE:
		case AXE:
			tmp = 6;
			break;
		default:
			goto no_opener;
		}
/*JP		pline("Using your %s you try to open the tin.",
			aobjnam(uwep, (char *)0));*/
		You("%sを使って缶を開けようとした．",
			xname(uwep));
	} else {
no_opener:
/*JP		pline("It is not so easy to open this tin.");*/
		pline("この缶を開けるのは容易なことではない．");
		if(Glib) {
/*JP			pline_The("tin slips from your %s.",*/
			pline("缶はあなたの%sから滑り落ちた．",
			      makeplural(body_part(FINGER)));
			if(otmp->quan > 1L) {
				register struct obj *obj;
				obj = splitobj(otmp, 1L);
				if(otmp == uwep) setuwep(obj);
			}
			if (carried(otmp)) dropx(otmp);
			else stackobj(otmp);
			return;
		}
		tmp = rn1(1 + 500/((int)(ACURR(A_DEX) + ACURRSTR)), 10);
	}
	tin.reqtime = tmp;
	tin.usedtime = 0;
	tin.tin = otmp;
/*JP	set_occupation(opentin, "opening the tin", 0);*/
	set_occupation(opentin, "缶を開ける", 0);
	return;
}

int
Hear_again()		/* called when waking up after fainting */
{
	flags.soundok = 1;
	return 0;
}

/* called on the "first bite" of rotten food */
static int
rottenfood(obj)
struct obj *obj;
{
/*JP	pline("Blecch!  Rotten %s!", foodword(obj));*/
	pline("ゲェ！腐った%sだ！", foodword(obj));
	if(!rn2(4)) {
/*JP		if (Hallucination) You_feel("rather trippy.");*/
		if (Hallucination) You("へろへろした．");
/*JP		else You_feel("rather %s.", body_part(LIGHT_HEADED));*/
		else You("%s．", body_part(LIGHT_HEADED));
		make_confused(HConfusion + d(2,4),FALSE);
	} else if(!rn2(4) && !Blind) {
/*JP		pline("Everything suddenly goes dark.");*/
		pline("突然全てが暗くなった．");
		make_blinded((long)d(2,10),FALSE);
	} else if(!rn2(3)) {
		const char *what, *where;
		if (!Blind)
/*JP		    what = "goes",  where = "dark";*/
		    what = "なった",  where = "暗闇に";
		else if (Levitation || Is_airlevel(&u.uz) ||
			 Is_waterlevel(&u.uz))
/*JP		    what = "you lose control of",  where = "yourself";*/
		    what = "制御できなくなった",  where = "自分を";
		else
/*JP		    what = "you slap against the",  where = surface(u.ux,u.uy);*/
		    what = "にぶつかった",  where = surface(u.ux,u.uy);
/*JP		pline_The("world spins and %s %s.", what, where);*/
		pline("世界が回転し，%s%s.", where, what);
		flags.soundok = 0;
		nomul(-rnd(10));
/*JP		nomovemsg = "You are conscious again.";*/
		nomovemsg = "あなたはまた正気づいた．";
		afternmv = Hear_again;
		return(1);
	}
	return(0);
}

static int
eatcorpse(otmp)		/* called when a corpse is selected as food */
	register struct obj *otmp;
{
	int tp = 0, mnum = otmp->corpsenm;
	long rotted = 0L;
	/*JP
	boolean uniq = !!(mons[mnum].geno & G_UNIQ);
	*/

	if (mnum != PM_LIZARD) {
		long age = peek_at_iced_corpse_age(otmp);

		rotted = (monstermoves - age)/(10L + rn2(20));
		if (otmp->cursed) rotted += 2L;
		else if (otmp->blessed) rotted -= 2L;
	}

	if (mnum != PM_ACID_BLOB && rotted > 5L) {
/*JP
		pline("Ulch - that %s was tainted!",
		      mons[mnum].mlet == S_FUNGUS ? "fungoid vegetation" :
		      is_meaty(&mons[mnum]) ? "meat" : "protoplasm");
*/
		pline("オェ！この%sは腐っている！", 
		      mons[mnum].mlet == S_FUNGUS ? "細菌に汚染された植物" :
		      is_meaty(&mons[mnum]) ? "肉" : "生物");
		if (u.usym == S_FUNGUS) {
/*JP
			pline("It doesn't seem at all sickening, though...");
*/
			pline("しかし，いたって元気だ．．．");
		} else {
			char buf[BUFSZ];
			long sick_time;

			sick_time = (long) rn1(10, 10);
			/* make sure new ill doesn't result in improvement */
			if (Sick && (sick_time > Sick))
			    sick_time = (Sick > 1L) ? Sick - 1L : 1L;
/*JP
			if (!uniq)
			    Sprintf(buf, "rotted %s", corpse_xname(otmp,TRUE));
			else
			    Sprintf(buf, "%s%s rotted corpse",
				    !type_is_pname(&mons[mnum]) ? "the " : "",
				    s_suffix(mons[mnum].mname));
*/
			Sprintf(buf, "腐った%sを食べ食中毒で", corpse_xname(otmp,TRUE));
			make_sick(sick_time, buf, TRUE, SICK_VOMITABLE);
		}
		if (carried(otmp)) useup(otmp);
		else useupf(otmp);
		return(1);
	} else if (acidic(&mons[mnum]) && !resists_acid(&youmonst)) {
		tp++;
/*JP
		You("have a very bad case of stomach acid.");
		losehp(rnd(15), "acidic corpse", KILLED_BY_AN);
*/
		pline("胃酸の調子がとても悪い．");
		losehp(rnd(15), "酸の死体で", KILLED_BY_AN);
	} else if (poisonous(&mons[mnum]) && rn2(5)) {
		tp++;
/*JP
		pline("Ecch - that must have been poisonous!");
*/
		pline("ウゲェー，有毒だったにちがいない！");  
		if(!Poison_resistance) {
			losestr(rnd(4));
/*JP
			losehp(rnd(15), "poisonous corpse", KILLED_BY_AN);
		} else	You("seem unaffected by the poison.");
*/
			losehp(rnd(15), "有毒な死体で", KILLED_BY_AN);
		} else	You("毒の影響を受けないようだ．");
	/* now any corpse left too long will make you mildly ill */
	} else if ((rotted > 5L || (rotted > 3L && rn2(5)))
					&& u.usym != S_FUNGUS) {
		tp++;
/*JP
		You("feel%s sick.", (Sick) ? " very" : "");
		losehp(rnd(8), "cadaver", KILLED_BY_AN);
*/
		You("%s気分が悪い．", (Sick) ? "とても" : "");
		losehp(rnd(8), "死体で", KILLED_BY_AN);
	}
	if (!tp && mnum != PM_LIZARD && (otmp->orotten || !rn2(7))) {
	    if (rottenfood(otmp)) {
		otmp->orotten = TRUE;
		(void)touchfood(otmp);
		return(1);
	    }
	    otmp->oeaten >>= 2;
	} else {
/*JP
	    pline("%s%s %s!",
		  !uniq ? "This " : !type_is_pname(&mons[mnum]) ? "The " : "",
		  food_xname(otmp, FALSE),
		  (carnivorous(uasmon) && !herbivorous(uasmon)) ?
			"is delicious" : "tastes terrible");
*/
	    pline("この%sは%s！", food_xname(otmp, FALSE),
		  (carnivorous(uasmon) && !herbivorous(uasmon)) ?
		  	"とても旨い" : "ひどい味だ");
	}

	/* delay is weight dependent */
	victual.reqtime = 3 + (mons[mnum].cwt >> 6);
	return(0);
}

static void
start_eating(otmp)		/* called as you start to eat */
	register struct obj *otmp;
{
#ifdef DEBUG
	debugpline("start_eating: %lx (victual = %lx)", otmp, victual.piece);
	debugpline("reqtime = %d", victual.reqtime);
	debugpline("(original reqtime = %d)", objects[otmp->otyp].oc_delay);
	debugpline("nmod = %d", victual.nmod);
	debugpline("oeaten = %d", otmp->oeaten);
#endif
	victual.fullwarn = victual.doreset = FALSE;
	victual.eating = TRUE;

	if (otmp->otyp == CORPSE) {
	    cprefx(victual.piece->corpsenm);
	    if (!victual.piece && victual.eating) do_reset_eat();
	    if (victual.eating == FALSE) return; /* died and lifesaved */
	}

	if (bite()) return;

	if(++victual.usedtime >= victual.reqtime) {
	    /* print "finish eating" message if they just resumed -dlc */
	    done_eating(victual.reqtime > 1 ? TRUE : FALSE);
	    return;
	}

/*JP
	Sprintf(msgbuf, "eating %s", food_xname(otmp, TRUE));
*/
	Sprintf(msgbuf, "%sを食べる", food_xname(otmp, TRUE));
	set_occupation(eatfood, msgbuf, 0);
}


static void
fprefx(otmp)		/* called on "first bite" of (non-corpse) food */
struct obj *otmp;
{
	switch(otmp->otyp) {

	    case FOOD_RATION:
		if(u.uhunger <= 200)
/*JP
		    if (Hallucination) pline("Oh wow, like, superior, man!");
		    else	       pline("That food really hit the spot!");
		else if(u.uhunger <= 700) pline("That satiated your stomach!");
*/

		    if (Hallucination) pline("まったりとして，それでいてしつこくない！これぞ究極のメニューだ！");
		    else	       pline("この食べ物は本当に申し分ない！");
		else if(u.uhunger <= 700) pline("満腹になった！");
		break;
	    case TRIPE_RATION:
		if (carnivorous(uasmon) && !humanoid(uasmon))
/*JP		    pline("That tripe ration was surprisingly good!");*/
		    pline("このほし肉はおどろくほど旨い！");
		else {
/*JP		    pline("Yak - dog food!");*/
		    pline("ウェー，ドッグフードだ！");
		    more_experienced(1,0);
		    flags.botl = 1;
		}
		if (rn2(2) && (!carnivorous(uasmon) || humanoid(uasmon))) {
			make_vomiting((long)rn1(victual.reqtime, 14), FALSE);
		}
		break;
	    case CLOVE_OF_GARLIC:
		if (is_undead(uasmon)) {
			make_vomiting((long)rn1(victual.reqtime, 5), FALSE);
			break;
		}
		/* Fall through otherwise */
	    default:
		if (otmp->otyp==SLIME_MOLD && !otmp->cursed
			&& otmp->spe == current_fruit)
/*JP		    pline("My, that was a %s %s!",*/
		    pline("おや，なんて%s%sだ！",
/*JP			  Hallucination ? "primo" : "yummy",*/
			  Hallucination ? "上品な" : "おいしい",
			  singular(otmp, xname));
		else
#ifdef UNIX
		if (otmp->otyp == APPLE || otmp->otyp == PEAR) {
		    if (!Hallucination) pline("Core dumped");
		    else {
/* This is based on an old Usenet joke, a fake a.out manual page */
			int x = rnd(100);
			if (x <= 75)
			    pline("Segmentation fault -- core dumped.");
			else if (x <= 99)
			    pline("Bus error -- core dumped.");
			else pline("Yo' mama -- core dumped.");
		    }
		} else
#endif
		if (otmp->otyp == EGG && stale_egg(otmp)) {
/*JP		    pline("Ugh.  Rotten egg.");*//* perhaps others like it */
		    pline("ウゲェー腐った卵だ．");
		    make_vomiting(Vomiting+d(10,4), TRUE);
		} else
#if 0 /*JP*/
		    pline("This %s is %s", singular(otmp, xname),
		      otmp->cursed ? (Hallucination ? "grody!" : "terrible!") :
		      (otmp->otyp == CRAM_RATION
		      || otmp->otyp == K_RATION
		      || otmp->otyp == C_RATION)
		      ? "bland." :
		      Hallucination ? "gnarly!" : "delicious!");
#endif /*JP*/
		    pline("この%sは%s", singular(otmp, xname),
		      otmp->cursed ? (Hallucination ? "酒っぽい！" : "ひどい味だ！") :
		      otmp->otyp == CRAM_RATION ? "さっぱりしている" :
		      Hallucination ? "こぶこぶしている！" : "うまい！");
		break;
	}
}

static void
eataccessory(otmp)
struct obj *otmp;
{
	int typ = otmp->otyp;
	int oldprop;

	/* Note: rings are not so common that this is unbalancing. */
	/* (How often do you even _find_ 3 rings of polymorph in a game?) */
	oldprop = !!(u.uprops[objects[typ].oc_oprop].p_flgs);
	otmp->known = otmp->dknown = 1; /* by taste */
	if (!rn2(otmp->oclass == RING_CLASS ? 3 : 5))
	  switch (otmp->otyp) {
	    default:
	        if (!objects[typ].oc_oprop) break; /* should never happen */

		if (!(u.uprops[objects[typ].oc_oprop].p_flgs & FROMOUTSIDE))
/*JP		    pline("Magic spreads through your body as you digest the %s.",*/
		    pline("あなたが%sを消化すると，その魔力が体にしみこんだ．",
/*JP			  otmp->oclass == RING_CLASS ? "ring" : "amulet");*/
			  otmp->oclass == RING_CLASS ? "指輪" : "魔除け");

		u.uprops[objects[typ].oc_oprop].p_flgs |= FROMOUTSIDE;

		switch (typ) {
		  case RIN_SEE_INVISIBLE:
		    set_mimic_blocking();
		    see_monsters();
		    if (Invis && !oldprop && !perceives(uasmon) && !Blind) {
			newsym(u.ux,u.uy);
/*JP			pline("Suddenly you can see yourself.");*/
			pline("突然自分自身が見えるようになった．");
			makeknown(typ);
		    }
		    break;
		  case RIN_INVISIBILITY:
		    if (!oldprop && !See_invisible && !Blind) {
			newsym(u.ux,u.uy);
/*JP			Your("body takes on a %s transparency...",
				Hallucination ? "normal" : "strange");*/
			pline("%sあなたの体は透過性をもった．．．",
				Hallucination ? "あたりまえなことだが" : "奇妙なことに");
			makeknown(typ);
		    }
		    break;
		  case RIN_PROTECTION_FROM_SHAPE_CHAN:
		    rescham();
		    break;
		  case RIN_LEVITATION:
		    if (!Levitation) {
			float_up();
			HLevitation += d(10,20);
			makeknown(typ);
		    }
		    break;
		}
		break;
	    case RIN_ADORNMENT:
		if (adjattrib(A_CHA, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_GAIN_STRENGTH:
		if (adjattrib(A_STR, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_INCREASE_DAMAGE:
		u.udaminc += otmp->spe;
		break;
	    case RIN_PROTECTION:
		Protection |= FROMOUTSIDE;
		u.ublessed += otmp->spe;
		flags.botl = 1;
		break;
	    case AMULET_OF_CHANGE:
		makeknown(typ);
		change_sex();
/*JP		You("are suddenly very %s!",
		    flags.female ? "feminine" : "masculine");*/
		You("突然%s！", 
		    flags.female ? "女っぽくなった" : "筋肉質になった");
		flags.botl = 1;
		break;
	    case AMULET_OF_STRANGULATION: /* bad idea! */
		choke(otmp);
		break;
	    case AMULET_OF_RESTFUL_SLEEP: /* another bad idea! */
		Sleeping = rnd(100);
		break;
	    case AMULET_OF_LIFE_SAVING:
	    case AMULET_OF_REFLECTION: /* nice try */
	    /* can't eat Amulet of Yendor or fakes,
	     * and no oc_prop even if you could -3.
	     */
		break;
	  }
}

static void
eatspecial() /* called after eating non-food */
{
	register struct obj *otmp = victual.piece;

	lesshungry(victual.nmod);
	victual.piece = (struct obj *)0;
	victual.eating = 0;
	if (otmp->oclass == GOLD_CLASS) {
		dealloc_obj(otmp);
		return;
	}
	if (otmp->oclass == POTION_CLASS) {
		otmp->quan++; /* dopotion() does a useup() */
		(void)dopotion(otmp);
	}
	if (otmp->oclass == RING_CLASS || otmp->oclass == AMULET_CLASS)
		eataccessory(otmp);
	else if (otmp->otyp == LEASH && otmp->leashmon)
		o_unleash(otmp);

	if (otmp == uwep && otmp->quan == 1L) uwepgone();
	if (otmp == uball) unpunish();
	if (otmp == uchain) unpunish(); /* but no useup() */
	else if (carried(otmp)) useup(otmp);
	else useupf(otmp);
}

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
static const char *foodwords[] = {
/*JP	"meal", "liquid", "wax", "food", "meat",
	"paper", "cloth", "leather", "wood", "bone", "scale",
	"metal", "metal", "metal", "silver", "gold", "platinum", "mithril",
	"plastic", "glass", "rich food", "stone"*/
	"肉", "液体", "油", "食料", "肉",
	"紙", "服", "皮", "木", "骨", "鱗",
	"金属", "金属", "金属", "銀", "金", "プラチナ", "ミスリル",
	"プラスチック", "ガラス", "高級料理", "石"
};

static const char *
foodword(otmp)
register struct obj *otmp;
{
/*JP	if (otmp->oclass == FOOD_CLASS) return "food";*/
	if (otmp->oclass == FOOD_CLASS) return "食料";
	if (otmp->oclass == GEM_CLASS &&
	    objects[otmp->otyp].oc_material == GLASS &&
	    otmp->dknown)
		makeknown(otmp->otyp);
	return foodwords[objects[otmp->otyp].oc_material];
}

static void
fpostfx(otmp)		/* called after consuming (non-corpse) food */
register struct obj *otmp;
{
	switch(otmp->otyp) {
	    case SPRIG_OF_WOLFSBANE:
		if (u.ulycn >= LOW_PM || is_were(uasmon))
		    you_unwere(TRUE);
		break;
	    case CARROT:
		make_blinded(0L,TRUE);
		break;
	    case FORTUNE_COOKIE:
		outrumor(bcsign(otmp), TRUE);
		break;
	    case LUMP_OF_ROYAL_JELLY:
		/* This stuff seems to be VERY healthy! */
		gainstr(otmp, 1);
		if (Upolyd) {
		    u.mh += otmp->cursed ? -rnd(20) : rnd(20);
		    if (u.mh > u.mhmax) {
			if (!rn2(17)) u.mhmax++;
			u.mh = u.mhmax;
		    } else if (u.mh <= 0) {
			rehumanize();
		    }
		} else {
		    u.uhp += otmp->cursed ? -rnd(20) : rnd(20);
		    if (u.uhp > u.uhpmax) {
			if(!rn2(17)) u.uhpmax++;
			u.uhp = u.uhpmax;
		    } else if (u.uhp <= 0) {
			killer_format = KILLED_BY_AN;
/*JP			killer = "rotten lump of royal jelly";*/
			killer = "腐ったロイヤルゼリーを食べ食中毒で";
			done(DIED);
		    }
		}
		if(!otmp->cursed) heal_legs();
		break;
	    case EGG:
		if(otmp->corpsenm == PM_COCKATRICE) {
		    if (!resists_ston(&youmonst) &&
			!(poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM))) {
			if (!Stoned) Stoned = 5;
			killer_format = KILLED_BY_AN;
/*JP			killer = "cockatrice egg";*/
			killer = "コカトリスの卵で";
		    }
		}
		break;
	}
	return;
}

int
doeat()		/* generic "eat" command funtion (see cmd.c) */
{
	register struct obj *otmp;
	int basenutrit;			/* nutrition of full item */

	if (Strangled) {
/*JP		pline("If you can't breathe air, how can you consume solids?");*/
		pline("息もできないのに，どうやって食べたらいいんだい？");
		return 0;
	}
/*JP	if (!(otmp = floorfood("eat", 0))) return 0;*/
	if (!(otmp = floorfood("食べる", 0))) return 0;
	if (check_capacity((char *)0)) return 0;

	/* We have to make non-foods take 1 move to eat, unless we want to
	 * do ridiculous amounts of coding to deal with partly eaten plate
	 * mails, players who polymorph back to human in the middle of their
	 * metallic meal, etc....
	 */
	if (!is_edible(otmp)) {
/*JP	    You("cannot eat that!");*/
	    You("それを食べられない！");
	    return 0;
	} else if ((otmp->owornmask & (W_ARMOR|W_TOOL|W_RING|W_AMUL)) != 0) {
/*JP	    You_cant("eat %s you're wearing.", something);*/
	    You("身につけている間は食べれない．");
	    return 0;
	}
	if (is_metallic(otmp) &&
	    u.umonnum == PM_RUST_MONSTER && otmp->oerodeproof) {
	    	otmp->rknown = TRUE;
		if (otmp->quan > 1L) {
			if(!carried(otmp))
				(void) splitobj(otmp, 1L);
			else
				otmp = splitobj(otmp, otmp->quan - 1L);
		}
/*JP		pline("Ulch - That %s was rustproofed!", xname(otmp));*/
		pline("ウゲェー！%sは防錆されている！", xname(otmp));
		/* The regurgitated object's rustproofing is gone now */
		otmp->oerodeproof = 0;
		make_stunned(HStun + rn2(10), TRUE);
/*JP		pline("You spit %s out onto the %s.", the(xname(otmp)),*/
		pline("あなたは%sを%sに吐き出した．", the(xname(otmp)),
			surface(u.ux, u.uy));
		if (carried(otmp)) {
			freeinv(otmp);
			dropy(otmp);
		}
		stackobj(otmp);
		return 1;
	}
	if (otmp->oclass != FOOD_CLASS) {
	    victual.reqtime = 1;
	    victual.piece = otmp;
		/* Don't split it, we don't need to if it's 1 move */
	    victual.usedtime = 0;
	    victual.canchoke = (u.uhs == SATIATED);
		/* Note: gold weighs 1 pt. for each 1000 pieces (see */
		/* pickup.c) so gold and non-gold is consistent. */
	    if (otmp->oclass == GOLD_CLASS)
		basenutrit = ((otmp->quan > 200000L) ? 2000
			: (int)(otmp->quan/100L));
	    else if(otmp->oclass == BALL_CLASS || otmp->oclass == CHAIN_CLASS)
		basenutrit = weight(otmp);
	    /* oc_nutrition is usually weight anyway */
	    else basenutrit = objects[otmp->otyp].oc_nutrition;
	    victual.nmod = basenutrit;
	    victual.eating = TRUE; /* needed for lesshungry() */

	    if (otmp->cursed)
		(void) rottenfood(otmp);

	    if (otmp->oclass == WEAPON_CLASS && otmp->opoisoned) {
/*JP		pline("Ecch - that must have been poisonous!");*/
		pline("ウゲェー，有毒だったに違いない！");  
		if(!Poison_resistance) {
		    losestr(rnd(4));
/*JP*/
/*JP		    losehp(rnd(15), xname(otmp), KILLED_BY_AN);*/
		    {
		      char jbuf[BUFSIZ];
		      Strcpy(jbuf,xname(otmp));
		      Strcat(jbuf,"で");
		      losehp(rnd(15), jbuf, KILLED_BY_AN);
		    }
		} else
/*JP		    You("seem unaffected by the poison.");*/
		    You("毒の影響を受けないようだ．");
	    } else if (!otmp->cursed)
/*JP		pline("This %s is delicious!",*/
		pline("この%sは旨い！",
		      otmp->oclass == GOLD_CLASS ? foodword(otmp) :
		      singular(otmp, xname));
	    eatspecial();
	    return 1;
	}

	if(otmp == victual.piece) {
	/* If they weren't able to choke, they don't suddenly become able to
	 * choke just because they were interrupted.  On the other hand, if
	 * they were able to choke before, if they lost food it's possible
	 * they shouldn't be able to choke now.
	 */
	    if (u.uhs != SATIATED) victual.canchoke = FALSE;
	    if(!carried(victual.piece)) {
		if(victual.piece->quan > 1L)
			(void) splitobj(victual.piece, 1L);
	    }
/*JP	    You("resume your meal.");*/
	    You("食事を再開した．");
	    start_eating(victual.piece);
	    return(1);
	}

	/* nothing in progress - so try to find something. */
	/* tins are a special case */
	if(otmp->otyp == TIN) {
	    start_tin(otmp);
	    return(1);
	}

	victual.piece = otmp = touchfood(otmp);
	victual.usedtime = 0;

	/* Now we need to calculate delay and nutritional info.
	 * The base nutrition calculated here and in eatcorpse() accounts
	 * for normal vs. rotten food.  The reqtime and nutrit values are
	 * then adjusted in accordance with the amount of food left.
	 */
	if(otmp->otyp == CORPSE) {
	    if(eatcorpse(otmp)) return(1);
	    /* else eatcorpse sets up reqtime and oeaten */
	} else {
	    victual.reqtime = objects[otmp->otyp].oc_delay;
	    if (otmp->otyp != FORTUNE_COOKIE &&
		(otmp->cursed ||
		 (((monstermoves - otmp->age) > (int) otmp->blessed ? 50:30) &&
		(otmp->orotten || !rn2(7))))) {

		if (rottenfood(otmp)) {
		    otmp->orotten = TRUE;
		    return(1);
		}
		otmp->oeaten >>= 1;
	    } else fprefx(otmp);
	}

	/* re-calc the nutrition */
	if (otmp->otyp == CORPSE) basenutrit = mons[otmp->corpsenm].cnutrit;
	else basenutrit = objects[otmp->otyp].oc_nutrition;

#ifdef DEBUG
	debugpline("before rounddiv: victual.reqtime == %d", victual.reqtime);
	debugpline("oeaten == %d, basenutrit == %d", otmp->oeaten, basenutrit);
#endif
	victual.reqtime = (basenutrit == 0 ? 0 :
		rounddiv(victual.reqtime * (long)otmp->oeaten, basenutrit));
#ifdef DEBUG
	debugpline("after rounddiv: victual.reqtime == %d", victual.reqtime);
#endif
	/* calculate the modulo value (nutrit. units per round eating)
	 * note: this isn't exact - you actually lose a little nutrition
	 *	 due to this method.
	 * TODO: add in a "remainder" value to be given at the end of the
	 *	 meal.
	 */
	if(victual.reqtime == 0)
	    /* possible if most has been eaten before */
	    victual.nmod = 0;
	else if ((int)otmp->oeaten > victual.reqtime)
	    victual.nmod = -((int)otmp->oeaten / victual.reqtime);
	else
	    victual.nmod = victual.reqtime % otmp->oeaten;
	victual.canchoke = (u.uhs == SATIATED);

	start_eating(otmp);
	return(1);
}

/* Take a single bite from a piece of food, checking for choking and
 * modifying usedtime.  Returns 1 if they choked and survived, 0 otherwise.
 */
static int
bite()
{
	if(victual.canchoke && u.uhunger >= 2000) {
		choke(victual.piece);
		return 1;
	}
	if (victual.doreset) {
		do_reset_eat();
		return 0;
	}
	force_save_hs = TRUE;
	if(victual.nmod < 0) {
		lesshungry(-victual.nmod);
		victual.piece->oeaten -= -victual.nmod;
	} else if(victual.nmod > 0 && (victual.usedtime % victual.nmod)) {
		lesshungry(1);
		victual.piece->oeaten--;
	}
	force_save_hs = FALSE;
	recalc_wt();
	return 0;
}

#endif /* OVLB */
#ifdef OVL0

void
gethungry()	/* as time goes by - called by moveloop() and domove() */
{
	if (u.uinvulnerable) return;	/* you don't feel hungrier */

	if ((!u.usleep || !rn2(10))	/* slow metabolic rate while asleep */
		&& (carnivorous(uasmon) || herbivorous(uasmon)))
	    u.uhunger--;		/* ordinary food consumption */

	if (moves % 2) {	/* odd turns */
	    /* Regeneration uses up food, unless due to an artifact */
	    if ((HRegeneration & (~W_ART)) &&
		(HRegeneration != W_WEP || !uwep->oartifact)) u.uhunger--;
	    if (near_capacity() > SLT_ENCUMBER) u.uhunger--;
	} else {		/* even turns */
	    if (Hunger) u.uhunger--;
	    /* Conflict uses up food too */
	    if ((Conflict & (~W_ARTI))) u.uhunger--;
	    /* +0 charged rings don't do anything, so don't affect hunger */
	    switch ((int)(moves % 20)) {	/* note: use even cases only */
	     case  4: if (uleft &&
			  (uleft->spe || !objects[uleft->otyp].oc_charged))
			    u.uhunger--;
		    break;
	     case  8: if (uamul) u.uhunger--;
		    break;
	     case 12: if (uright &&
			  (uright->spe || !objects[uright->otyp].oc_charged))
			    u.uhunger--;
		    break;
	     case 16: if (u.uhave.amulet) u.uhunger--;
		    break;
	     default: break;
	    }
	}
	newuhs(TRUE);
}

#endif /* OVL0 */
#ifdef OVLB

void
morehungry(num)	/* called after vomiting and after performing feats of magic */
register int num;
{
	u.uhunger -= num;
	newuhs(TRUE);
}


void
lesshungry(num)	/* called after eating (and after drinking fruit juice) */
register int num;
{
#ifdef DEBUG
	debugpline("lesshungry(%d)", num);
#endif
	u.uhunger += num;
	if(u.uhunger >= 2000) {
	    if (!victual.eating || victual.canchoke){
		if (victual.eating) {
			choke(victual.piece);
			reset_eat();
		} else
		if (tin.tin)
			choke(tin.tin);
		else
			choke((struct obj *) 0);
	    }
		/* no reset_eat() */
	} else {
	    /* Have lesshungry() report when you're nearly full so all eating
	     * warns when you're about to choke.
	     */
	    if (u.uhunger >= 1500) {
		if (!victual.eating || (victual.eating && !victual.fullwarn)) {
/*JP		    pline("You're having a hard time getting all of it down.");
		    nomovemsg = "You're finally finished.";*/
		    pline("全てを飲みこむには時間がかかる．");
		    nomovemsg = "やっと食べ終えた．";
		    if (!victual.eating)
			multi = -2;
		    else {
			victual.fullwarn = TRUE;
			if (victual.canchoke && victual.reqtime > 1) {
			    /* a one-gulp food will not survive a stop */
/*JP			    if (yn_function("Stop eating?",ynchars,'y')=='y') {*/
			    if (yn_function("食べるのを中断しますか？",ynchars,'y')=='y') {
				reset_eat();
				nomovemsg = (char *)0;
			    }
			}
		    }
		}
	    }
	}
	newuhs(FALSE);
}

STATIC_PTR
int
unfaint()
{
	(void) Hear_again();
	if(u.uhs > FAINTING)
		u.uhs = FAINTING;
	stop_occupation();
	flags.botl = 1;
	return 0;
}

#endif /* OVLB */
#ifdef OVL0

boolean
is_fainted()
{
	return((boolean)(u.uhs == FAINTED));
}

void
reset_faint()	/* call when a faint must be prematurely terminated */
{
	if(is_fainted()) nomul(0);
}

#if 0
void
sync_hunger()
{

	if(is_fainted()) {

		flags.soundok = 0;
		nomul(-10+(u.uhunger/10));
/*JP		nomovemsg = "You regain consciousness.";*/
		nomovemsg = "あなたは正気づいた．";
		afternmv = unfaint;
	}
}
#endif

void
newuhs(incr)		/* compute and comment on your (new?) hunger status */
boolean incr;
{
	unsigned newhs;
	static unsigned save_hs;
	static boolean saved_hs = FALSE;
	int h = u.uhunger;

	newhs = (h > 1000) ? SATIATED :
		(h > 150) ? NOT_HUNGRY :
		(h > 50) ? HUNGRY :
		(h > 0) ? WEAK : FAINTING;

	/* While you're eating, you may pass from WEAK to HUNGRY to NOT_HUNGRY.
	 * This should not produce the message "you only feel hungry now";
	 * that message should only appear if HUNGRY is an endpoint.  Therefore
	 * we check to see if we're in the middle of eating.  If so, we save
	 * the first hunger status, and at the end of eating we decide what
	 * message to print based on the _entire_ meal, not on each little bit.
	 */
	/* It is normally possible to check if you are in the middle of a meal
	 * by checking occupation == eatfood, but there is one special case:
	 * start_eating() can call bite() for your first bite before it
	 * sets the occupation.
	 * Anyone who wants to get that case to work _without_ an ugly static
	 * force_save_hs variable, feel free.
	 */
	/* Note: If you become a certain hunger status in the middle of the
	 * meal, and still have that same status at the end of the meal,
	 * this will incorrectly print the associated message at the end of
	 * the meal instead of the middle.  Such a case is currently
	 * impossible, but could become possible if a message for SATIATED
	 * were added or if HUNGRY and WEAK were separated by a big enough
	 * gap to fit two bites.
	 */
	if (occupation == eatfood || force_save_hs) {
		if (!saved_hs) {
			save_hs = u.uhs;
			saved_hs = TRUE;
		}
		u.uhs = newhs;
		return;
	} else {
		if (saved_hs) {
			u.uhs = save_hs;
			saved_hs = FALSE;
		}
	}

	if(newhs == FAINTING) {
		if(is_fainted()) newhs = FAINTED;
		if(u.uhs <= WEAK || rn2(20-u.uhunger/10) >= 19) {
			if(!is_fainted() && multi >= 0 /* %% */) {
				/* stop what you're doing, then faint */
				stop_occupation();
/*JP				You("faint from lack of food.");*/
				You("腹が減って倒れた．");
				flags.soundok = 0;
				nomul(-10+(u.uhunger/10));
/*JP				nomovemsg = "You regain consciousness.";*/
				nomovemsg = "あなたは正気づいた．";
				afternmv = unfaint;
				newhs = FAINTED;
			}
#ifdef NEWBIE
			if(!newbie.fainted){
			     pline("ヒント: ピンチのときはM-pで祈るという手もある．祈りすぎに注意！");
/*			     more();*/
			     newbie.fainted = 1;
			}
#endif
		} else
		if(u.uhunger < -(int)(200 + 20*ACURR(A_CON))) {
			u.uhs = STARVED;
			flags.botl = 1;
			bot();
/*JP			You("die from starvation.");*/
			You("餓死した．");
			killer_format = KILLED_BY;
			killer = "食料不足で餓死した";
			done(STARVING);
			/* if we return, we lifesaved, and that calls newuhs */
			return;
		}
	}

	if(newhs != u.uhs) {
		if(newhs >= WEAK && u.uhs < WEAK)
			losestr(1);	/* this may kill you -- see below */
		else if(newhs < WEAK && u.uhs >= WEAK)
			losestr(-1);
		switch(newhs){
		case HUNGRY:
			if (Hallucination) {
			    pline((!incr) ?
/*JP				"You now have a lesser case of the munchies." :
				"You are getting the munchies.");*/
				"ハラヘリが減った．":
				"ハラヘリヘリハラ．");
			} else
/*JP			    You((!incr) ? "only feel hungry now." :
				  (u.uhunger < 145) ? "feel hungry." :
				   "are beginning to feel hungry.");*/
			    You((!incr) ? "単に腹ペコ状態になった．" :
				  (u.uhunger < 145) ? "空腹感を感じた．" :
				   "空腹感をおぼえはじめた．");
			if (incr && occupation &&
			    (occupation != eatfood && occupation != opentin))
			    stop_occupation();
			break;
		case WEAK:
			if (Hallucination)
			    pline((!incr) ?
/*JP				  "You still have the munchies." :
      "The munchies are interfering with your motor capabilities.");*/
				  "ハラヘリが減らない．":
				  "ハラヘリがモーター性能を妨害している．");
			else if (incr &&
				(Role_is('W') || Role_is('E') || Role_is('V')))
/*JP			    pline("%s needs food, badly!", pl_character);*/
			    pline("至急食料が必要だ！");
			else
/*JP			    You((!incr) ? "feel weak now." :
				  (u.uhunger < 45) ? "feel weak." :
				   "are beginning to feel weak.");*/
			    You((!incr) ? "衰弱状態になった．":
				  (u.uhunger < 45) ? "衰弱してきた．" :
				   "弱くなってきたように感じた．");
			if (incr && occupation &&
			    (occupation != eatfood && occupation != opentin))
			    stop_occupation();
#ifdef NEWBIE
			if(!newbie.eat){
			     pline("ヒント: 'e'で食料を補給できる。落ちている死体も食うことができる．");
/*			     more();*/
			     newbie.eat = 1;
			}
#endif
			break;
		}
		u.uhs = newhs;
		flags.botl = 1;
		bot();
		if(u.uhp < 1) {
/*JP			You("die from hunger and exhaustion.");*/
			You("空腹と衰弱で死んだ．");
			killer_format = KILLED_BY;
			killer = "衰弱で";
			done(STARVING);
			return;
		}
	}
}

#endif /* OVL0 */
#ifdef OVLB

/* Returns an object representing food.  Object may be either on floor or
 * in inventory.
 */
struct obj *
floorfood(verb,corpsecheck)	/* get food from floor or pack */
	const char *verb;
	int corpsecheck; /* 0, no check, 1, corpses, 2, tinnable corpses */
{
	register struct obj *otmp;
	char qbuf[QBUFSZ];
	char c;
/*JP	boolean feeding = (!strcmp(verb, "eat"));*/
	boolean feeding = (!strcmp(verb, "食べる"));

	if (feeding && metallivorous(uasmon)) {
	    struct obj *gold;
	    struct trap *ttmp = t_at(u.ux, u.uy);

	    if (ttmp && ttmp->tseen && ttmp->ttyp == BEAR_TRAP) {
		/* If not already stuck in the trap, perhaps there should
		   be a chance to becoming trapped?  Probably not, because
		   then the trap would just get eaten on the _next_ turn... */
/*JP		Sprintf(qbuf, "There is a bear trap here (%s); eat it?",*/
		Sprintf(qbuf, "ここには熊の罠(%s)がある",
			(u.utrap && u.utraptype == TT_BEARTRAP) ?
/*JP				"holding you" : "armed");*/
				"あなたを掴まえている" : "掴まえている");
		if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
		    u.utrap = u.utraptype = 0;
		    deltrap(ttmp);
		    return mksobj(BEARTRAP, TRUE, FALSE);
		} else if (c == 'q') {
		    return (struct obj *)0;
		}
	    }

	    if ((gold = g_at(u.ux, u.uy)) != 0) {
		if (gold->quan == 1L)
/*JP		    Sprintf(qbuf, "There is 1 gold piece here; eat it?");*/
		    Sprintf(qbuf, "ここには1ゴールドある．食べますか？");
		else
/*JP		    Sprintf(qbuf, "There are %ld gold pieces here; eat them?",*/
		    Sprintf(qbuf, "ここには%ldゴールドある．食べますか？",
			    gold->quan);
		if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
		    obj_extract_self(gold);
		    return gold;
		} else if (c == 'q') {
		    return (struct obj *)0;
		}
	    }
	}

	/* Is there some food (probably a heavy corpse) here on the ground? */
	if (!(Levitation && !Is_airlevel(&u.uz)  && !Is_waterlevel(&u.uz))
	    && !u.uswallow) {
	    for(otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
		if(corpsecheck ?
		(otmp->otyp==CORPSE && (corpsecheck == 1 || tinnable(otmp))) :
		    feeding ? (otmp->oclass != GOLD_CLASS && is_edible(otmp)) :
						otmp->oclass==FOOD_CLASS) {
/*JP			Sprintf(qbuf, "There %s %s here; %s %s?",
				(otmp->quan == 1L) ? "is" : "are",
				doname(otmp), verb,
				(otmp->quan == 1L) ? "it" : "one");*/
			Sprintf(qbuf, "ここには%sがある．%s？",
				doname(otmp), jconj(verb,"ますか"));
			if((c = yn_function(qbuf,ynqchars,'n')) == 'y')
				return(otmp);
			else if(c == 'q')
				return((struct obj *) 0);
		}
	    }
	}
	/* We cannot use ALL_CLASSES since that causes getobj() to skip its
	 * "ugly checks" and we need to check for inedible items.
	 */
	otmp = getobj(feeding ? (const char *)allobj :
				(const char *)comestibles, verb);
	if (corpsecheck && otmp)
	    if (otmp->otyp != CORPSE || (corpsecheck == 2 && !tinnable(otmp))) {
/*JP		You_cant("%s that!", verb);*/
		You_cant("それを%sことはできない！", verb);
		return (struct obj *)0;
	    }
	return otmp;
}

/* Side effects of vomiting */
/* added nomul (MRS) - it makes sense, you're too busy being sick! */
void
vomit()		/* A good idea from David Neves */
{
	make_sick(0L, (char *) 0, TRUE, SICK_VOMITABLE);
	nomul(-2);
}

int
eaten_stat(base, obj)
register int base;
register struct obj *obj;
{
	long uneaten_amt, full_amount;

	uneaten_amt = (long)obj->oeaten;
	full_amount = (obj->otyp == CORPSE) ? (long)mons[obj->corpsenm].cnutrit
					: (long)objects[obj->otyp].oc_nutrition;

	base = (int)(full_amount ? (long)base * uneaten_amt / full_amount : 0L);
	return (base < 1) ? 1 : base;
}

#endif /* OVLB */

/*eat.c*/
