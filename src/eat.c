/*	SCCS Id: @(#)eat.c	3.3	1999/08/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
/* #define DEBUG */	/* uncomment to enable new eat code debugging */

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
STATIC_DCL const char *FDECL(food_xname, (struct obj *,BOOLEAN_P));
STATIC_DCL void FDECL(choke, (struct obj *));
STATIC_DCL void NDECL(recalc_wt);
STATIC_DCL struct obj *FDECL(touchfood, (struct obj *));
STATIC_DCL void NDECL(do_reset_eat);
STATIC_DCL void FDECL(done_eating, (BOOLEAN_P));
STATIC_DCL void FDECL(cprefx, (int));
STATIC_DCL int FDECL(intrinsic_possible, (int,struct permonst *));
STATIC_DCL void FDECL(givit, (int,struct permonst *));
STATIC_DCL void FDECL(cpostfx, (int));
STATIC_DCL void FDECL(start_tin, (struct obj *));
STATIC_DCL int FDECL(eatcorpse, (struct obj *));
STATIC_DCL void FDECL(start_eating, (struct obj *));
STATIC_DCL void FDECL(fprefx, (struct obj *));
STATIC_DCL void FDECL(fpostfx, (struct obj *));
STATIC_DCL int NDECL(bite);

STATIC_DCL int FDECL(rottenfood, (struct obj *));
STATIC_DCL void NDECL(eatspecial);
STATIC_DCL void FDECL(eataccessory, (struct obj *));
STATIC_DCL const char * FDECL(foodword, (struct obj *));

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

/* also used to see if you're allowed to eat cats and dogs */
#define CANNIBAL_ALLOWED() (Role_if(PM_CAVEMAN) || Race_if(PM_ORC))

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
	"��ʢ    ",
	"        ",
	"�ڤ��ڤ�",
	"���    ",
	"�դ�դ�",
	"´��    ",
	"���    "
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

	if (metallivorous(youmonst.data) && is_metallic(obj))
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
#define ROTTEN_TIN 4
	{"rotten",	-50},
#define HOMEMADE_TIN 5
	{"homemade",	 50},
	{"stir fried",   80},
	{"candied",      100},
	{"boiled",       50},
	{"dried",        55},
	{"szechuan",     70},
	{"french fried", 40},
	{"sauteed",      95},
	{"broiled",      80},
	{"smoked",       50},
	{"", 0}
}; */
static const struct { const char *txt; int nut; } tintxts[] = {
	{"���Ȥ�ʪ",	 60},
	{"����ʪ",	 40},
	{"�Υ�����",	 20},
	{"�Υԥ塼��",	500},
#define ROTTEN_TIN 4
	{"��ä�",	-50},
#define HOMEMADE_TIN 5
	{"��������",	 50},
	{"�����Ȥ�",     80},
	{"�����Ҥ�",    100},
	{"���",         50},
	{"����",         55},
	{"������",       70},
	{"�ե�����",   40},
	{"�Υ��ơ�",     95},
	{"�Υƥ�䥭",   80},
	{"������",       50},
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
	if (youmonst.m_ap_type) {
	    youmonst.m_ap_type = M_AP_NOTHING;
	    newsym(u.ux,u.uy);
	}
	return 0;
}

/* ``[the(] singular(food, xname) [)]'' with awareness of unique monsters */
STATIC_OVL const char *
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
	    Sprintf(bufp, "%s�λ���",
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
STATIC_OVL void
choke(food)	/* To a full belly all food is bad. (It.) */
	register struct obj *food;
{
	/* only happens if you were satiated */
	if (u.uhs != SATIATED) {
		if (food->otyp != AMULET_OF_STRANGULATION)
			return;
	} else if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL) {
			adjalign(-1);		/* gluttony is unchivalrous */
/*JP			You_feel("like a glutton!");*/
			You("�翩���Τ褦�ʵ���������");
	}

	exercise(A_CON, FALSE);

	if (Breathless || (!Strangled && !rn2(20))) {
		/* choking by eating AoS doesn't involve stuffing yourself */
		if (food->otyp == AMULET_OF_STRANGULATION) {
/*JP			You("choke, but recover your composure.");*/
			You("���ʤ��줿���������ʤ�Ȥ�ʤ��ä���");
			return;
		}
/*JP		You("stuff yourself and then vomit voluminously.");*/
		pline("���Ĥ��Ĥȸ��˵ͤ�������, �ɥФä��Ǥ��Ф��Ƥ��ޤä���");
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
			You("%s�򹢤˵ͤޤ餻�Ƥ��ޤä���", foodword(food));
			if (food->oclass == GOLD_CLASS) {
/*JP
				killer = "a very rich meal";
*/
				killer = "�ȤƤ��ڤ�������";
			} else {
				killer = food_xname(food, FALSE);
			}
		} else {
/*JP
			You("choke over it.");
			killer = "quick snack";
*/
			pline("���˵ͤޤ餻�Ƥ��ޤä���");
			killer = "�Ῡ����";
		}
/*JP
		You("die...");
*/
		pline("���ʤ��ϻ�ˤޤ���������");
		done(CHOKING);
	}
}

STATIC_OVL void
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

STATIC_OVL struct obj *
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
		verbalize("���ä��ʤ餪�㤤����������������");
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

/* renaming an object usually results in it having a different address;
   so the sequence start eating/opening, get interrupted, name the food,
   resume eating/opening would restart from scratch */
void
food_substitution(old_obj, new_obj)
struct obj *old_obj, *new_obj;
{
	if (old_obj == victual.piece) victual.piece = new_obj;
	if (old_obj == tin.tin) tin.tin = new_obj;
}

STATIC_OVL void
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

STATIC_OVL void
done_eating(message)
boolean message;
{
	victual.piece->in_use = TRUE;
	occupation = 0; /* do this early, so newuhs() knows we're done */
	newuhs(FALSE);
	if (nomovemsg) {
		if (message) pline(nomovemsg);
		nomovemsg = 0;
	} else if (message)
/*JP
		You("finish eating %s.", food_xname(victual.piece, TRUE));
*/
		You("%s�򿩤ٽ�������",  food_xname(victual.piece, TRUE));

	if(victual.piece->otyp == CORPSE)
		cpostfx(victual.piece->corpsenm);
	else
		fpostfx(victual.piece);

	if (carried(victual.piece)) useup(victual.piece);
	else useupf(victual.piece, 1L);
	victual.piece = (struct obj *) 0;
	victual.fullwarn = victual.eating = victual.doreset = FALSE;
}

STATIC_OVL void
cprefx(pm)
register int pm;
{
	if (!CANNIBAL_ALLOWED() && your_race(&mons[pm])) {
		if (Upolyd)
/*JP			You("have a bad feeling deep inside.");*/
		You("�������ˤ�����줿��");
/*JP		You("cannibal!  You will regret this!");*/
		pline("����������������뤾��");
		HAggravate_monster |= FROMOUTSIDE;
		change_luck(-rn1(4,2));		/* -5..-2 */
	}

	if (touch_petrifies(&mons[pm]) || pm == PM_MEDUSA) {
	    if (!Stone_resistance &&
		!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
		char kbuf[BUFSZ];

/*JP		Sprintf(kbuf, "tasting %s meat", mons[pm].mname);*/
		Sprintf(kbuf, "%s�����򿩤�", jtrns_mon(mons[pm].mname,-1));
		killer_format = KILLED_BY;
		killer = kbuf;
/*JP		You("turn to stone.");*/
		You("�в�������");
		done(STONING);
		victual.eating = FALSE;
		return; /* lifesaved */
	    }
	}

	switch(pm) {
	    case PM_LITTLE_DOG:
	    case PM_DOG:
	    case PM_LARGE_DOG:
	    case PM_KITTEN:
	    case PM_HOUSECAT:
	    case PM_LARGE_CAT:
		if (!CANNIBAL_ALLOWED()) {
/*JP		    You_feel("that eating the %s was a bad idea.", mons[pm].mname);*/
		    pline("%s�򿩤٤�ΤϤ褯�ʤ�����������", jtrns_mon(mons[pm].mname, -1));

		    HAggravate_monster |= FROMOUTSIDE;
		}
		break;
	    case PM_LIZARD:
		if (Stoned) fix_petrification();
		break;
	    case PM_DEATH:
	    case PM_PESTILENCE:
	    case PM_FAMINE:
		{ char buf[BUFSZ];
/*JP		    pline("Eating that is instantly fatal."); */
		    pline("���٤��餹���˻��Ǥ��ޤä���");
/*JP		    Sprintf(buf, "unwisely ate the body of %s",*/
		    Sprintf(buf, "�򤫤ˤ�%s�򿩤٤�",
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
	    case PM_GREEN_SLIME:
	    	if (!Unchanging && youmonst.data != &mons[PM_FIRE_VORTEX] &&
	    			youmonst.data != &mons[PM_FIRE_ELEMENTAL] &&
	    			youmonst.data != &mons[PM_GREEN_SLIME]) {
/*JP	    	    You("don't feel very well.");*/
	    	    You("������ʬ��������");
	    	    Slimed = 10L;
	    	}
	    	/* Fall through */
	    default:
		if (acidic(&mons[pm]) && Stoned)
		    fix_petrification();
		break;
	}
}

void
fix_petrification()
{
	Stoned = 0;
	if (Hallucination)
/*JP	    pline("What a pity - you just ruined a future piece of %sart!",
		  ACURR(A_CHA) > 15 ? "fine " : "");
*/
	    pline("�ʤ�Ƥ��Ȥ���%s�ݽѺ��ʤˤʤ줿���⤷��ʤ��Τˡ�",
		  ACURR(A_CHA) > 15 ? "���Ť�" : "");
	else
/*JP	    You_feel("limber!");*/
	    You("�Τ���餫���ʤä��褦�ʵ���������");
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
STATIC_OVL int
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
STATIC_OVL void
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
		    You(Hallucination ? "�֥����������ءפ���Ƥ���褦��" :
			    "��ִ�����������");
			HFire_resistance |= FROMOUTSIDE;
		}
		break;
	    case SLEEP_RES:
#ifdef DEBUG
		debugpline("Trying to give sleep resistance");
#endif
		if(!(HSleep_resistance & FROMOUTSIDE)) {
/*JP			You_feel("wide awake."); */
		    You("�Ѥä����ܤ����᤿��");
		    HSleep_resistance |= FROMOUTSIDE;
		}
		break;
	    case COLD_RES:
#ifdef DEBUG
		debugpline("Trying to give cold resistance");
#endif
		if(!(HCold_resistance & FROMOUTSIDE)) {
/*JP			You_feel("full of hot air."); */
		    You("Ǯ�������Ȥ˴�������");
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
			  "��������ȷ���ˤʤä��褦�ʵ���������" :
			  "�ȤƤ���ˤʤä��褦�ʵ���������");
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
				You("���������줿�褦�ʵ���������");
			else
/*JP				Your("health currently feels amplified!");*/
				pline("�򹯤��������줿�褦�ʵ���������");
			HShock_resistance |= FROMOUTSIDE;
		}
		break;
	    case POISON_RES:
#ifdef DEBUG
		debugpline("Trying to give poison resistance");
#endif
		if(!(HPoison_resistance & FROMOUTSIDE)) {
/*JP			You_feel("healthy.");*/
			You("��Ū�ˤʤä��褦�ʵ���������");
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
			pline(Hallucination ? "�Τ����ӻ��ä��褦�ʵ���������" :
			    "ķ���Ϥ���ޤä��褦�ʵ���������");
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
			    "�����濴Ū�ˤʤä��褦�ʵ���������" :
			    "��ʬ���Ȥ�����Ǥ���褦�ʵ���������");
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
			    "����ο���˿��줿�褦�ʵ���������" :
			    "��̯������Ū�Ԥ��򴶤�����");
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

STATIC_OVL void
cpostfx(pm)		/* called after completely consuming a corpse */
register int pm;
{
	register int tmp = 0;

	/* in case `afternmv' didn't get called for previously mimicking
	   gold, clean up now to avoid `eatmbuf' memory leak */
	if (eatmbuf) (void)eatmdone();

	switch(pm) {
	    case PM_WRAITH:
		pluslvl(FALSE);
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
			if (!(HInvis & INTRINSIC)) Your("�Ѥϱ����줿��");
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
		if (youmonst.data->mlet != S_MIMIC) {
		    char buf[BUFSZ];

/*JP		    You_cant("resist the temptation to mimic a pile of gold.");*/
		    You("��ߤλ��򿿻�������Ͷ�Ǥˤ���줿��");
		    nomul(-tmp);
/*JP		    Sprintf(buf, "You now prefer mimicking %s again.",
			    an(Upolyd ? youmonst.data->mname : urace.noun));*/
		    Sprintf(buf, "����ɤ�%s�ο������������ʤä���",
			    jtrns_mon(Upolyd ? youmonst.data->mname : urace.noun, -1));
		    eatmbuf = strcpy((char *) alloc(strlen(buf) + 1), buf);
		    nomovemsg = eatmbuf;
		    afternmv = eatmdone;
		    /* ??? what if this was set before? */
		    youmonst.m_ap_type = M_AP_OBJECT;
		    youmonst.mappearance = GOLD_PIECE;
		    newsym(u.ux,u.uy);
		    curs_on_u();
		    /* make gold symbol show up now */
		    display_nhwindow(WIN_MAP, TRUE);
		}
		break;
	    case PM_QUANTUM_MECHANIC:
/*JP		Your("velocity suddenly seems very uncertain!");*/
		Your("®�٤��������Գ���ˤʤä���");
		if (HFast & INTRINSIC) {
			HFast &= ~INTRINSIC;
/*JP			You("seem slower.");*/
			You("�٤��ʤä��褦����");
		} else {
			HFast |= FROMOUTSIDE;
/*JP			You("seem faster.");*/
			You("®���ʤä��褦����");
		}
		break;
	    case PM_LIZARD:
		if (HStun > 2)  make_stunned(2L,FALSE);
		if (HConfusion > 2)  make_confused(2L,FALSE);
		break;
	    case PM_CHAMELEON:
	    case PM_DOPPELGANGER:
	 /* case PM_SANDESTIN: */
		if (!Unchanging) {
/*JP		    You_feel("a change coming over you.");*/
		    pline("�Ѳ���ˬ�줿��");
		    polyself();
		}
		break;
	    case PM_MIND_FLAYER:
	    case PM_MASTER_MIND_FLAYER:
		if (ABASE(A_INT) < ATTRMAX(A_INT)) {
			if (!rn2(2)) {
/*JP				pline("Yum! That was real brain food!");*/
				pline("���������줳��������Ǿ̣������");
				(void) adjattrib(A_INT, 1, FALSE);
				break;	/* don't give them telepathy, too */
			}
		}
		else {
/*JP			pline("For some reason, that tasted bland.");*/
			pline("�ɤ������櫓�����������꤬������");
		}
		/* fall through to default case */
	    default: {
		register struct permonst *ptr = &mons[pm];
		int i, count;

		if (dmgtype(ptr, AD_STUN) || dmgtype(ptr, AD_HALU) ||
		    pm == PM_VIOLET_FUNGUS) {
/*JP			pline ("Oh wow!  Great stuff!");*/
			pline ("������ʤ������ϡ�");
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
		You("�̤򳫤���Τ򤢤���᤿��");
		return(0);
	}
	if(tin.usedtime < tin.reqtime)
		return(1);		/* still busy */
	if(tin.tin->otrapped ||
	   (tin.tin->cursed && tin.tin->spe != -1 && !rn2(8))) {
/*JP		b_trapped("tin", 0);*/
		b_trapped("��", 0);
		goto use_me;
	}
/*JP	You("succeed in opening the tin.");*/
	You("�̤򳫤���Τ�����������");
	if(tin.tin->spe != 1) {
	    if (tin.tin->corpsenm == NON_PM) {
/*JP		pline("It turns out to be empty.");*/
		pline("�̤϶��äݤ��ä���");
		tin.tin->dknown = tin.tin->known = TRUE;
		goto use_me;
	    }
	    r = tin.tin->cursed ? ROTTEN_TIN :	/* always rotten if cursed */
		    (tin.tin->spe == -1) ? HOMEMADE_TIN :  /* player made it */
			rn2(TTSZ-1);		/* else take your pick */
	    if (r == ROTTEN_TIN && (tin.tin->corpsenm == PM_LIZARD ||
			tin.tin->corpsenm == PM_LICHEN))
		r = HOMEMADE_TIN;		/* lizards don't rot */
	    else if (tin.tin->spe == -1 && !tin.tin->blessed && !rn2(7))
		r = ROTTEN_TIN;			/* some homemade tins go bad */
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
 	    pline("%s�Τ褦��������������", jtrns_mon(what, -1));
/*JP	    if (yn("Eat it?") == 'n') {*/
	    if (yn("���٤ޤ�����") == 'n') {
		if (!Hallucination) tin.tin->dknown = tin.tin->known = TRUE;
/*JP		if (flags.verbose) You("discard the open tin.");*/
		if (flags.verbose) You("�������̤�ΤƤ���");
		goto use_me;
	    }
	    /* in case stop_occupation() was called on previous meal */
	    victual.piece = (struct obj *)0;
	    victual.fullwarn = victual.eating = victual.doreset = FALSE;

/*JP	    You("consume %s %s.", tintxts[r].txt,
			mons[tin.tin->corpsenm].mname);
*/
/*JP
  ��ä�(4)����������(5)�����(8)������(9)�������֤ˡ�
  ����ʳ��ϸ��֡�
 */
	    if(r==4 || r==5 || r==8 || r==9)
		You("%s%s�򤿤��餲����",
		    tintxts[r].txt,
		    jtrns_mon(mons[tin.tin->corpsenm].mname, -1));
	    else
		You("%s%s�򤿤��餲����",
		    jtrns_mon(mons[tin.tin->corpsenm].mname, -1),
		    tintxts[r].txt);
		
	    /* KMH, conduct */
	    u.uconduct.flesh++;
	    if (is_meaty(&mons[tin.tin->corpsenm])) {
	    	u.uconduct.meat++;
	    	if (Role_if(PM_MONK)) {
/*JP	    	    pline("You feel guilty.");*/
	    	    pline("��򴶤�����");
	    	    adjalign(-1);
	    	}
	    }

	    tin.tin->dknown = tin.tin->known = TRUE;
	    cprefx(tin.tin->corpsenm); cpostfx(tin.tin->corpsenm);

	    /* check for vomiting added by GAN 01/16/87 */
	    if(tintxts[r].nut < 0) make_vomiting((long)rn1(15,10), FALSE);
	    else lesshungry(tintxts[r].nut);

	    if(r == 0) {			/* Deep Fried */
	        /* Assume !Glib, because you can't open tins when Glib. */
		incr_itimeout(&Glib, rnd(15));
/*JP		pline("Eating deep fried food made your %s very slippery.",*/
		pline("���ʤ���%s���Ȥ�����������ʪ�Τ�����䤹���ʤä���",
		      makeplural(body_part(FINGER)));
	    }
	} else {
	    if (tin.tin->cursed)
/*JP		pline("It contains some decaying %s substance.",*/
		pline("%s��ä�ʪ�Τ����äƤ��롥",
			hcolor(green));
	    else
/*JP		pline("It contains spinach.");*/
		pline("�ۥ���������äƤ��롥");

/*JP	    if (yn("Eat it?") == 'n') {*/
	    if (yn("���٤ޤ�����") == 'n') {
		if (!Hallucination && !tin.tin->cursed)
		    tin.tin->dknown = tin.tin->known = TRUE;
		if (flags.verbose)
/*JP		    You("discard the open tin.");*/
		    You("�������̤�ΤƤ���");
		goto use_me;
	    }
	    if (!tin.tin->cursed)
/*JP		pline("This makes you feel like %s!",
		      Hallucination ? "Swee'pea" : "Popeye");*/
		pline("%s�Τ褦�ʵ�ʬ�ˤʤä���",
		      Hallucination ? "�������åԡ�" : "�ݥѥ�");
	    lesshungry(600);
	    gainstr(tin.tin, 0);
	}
	tin.tin->dknown = tin.tin->known = TRUE;
use_me:
	if (carried(tin.tin)) useup(tin.tin);
	else useupf(tin.tin, 1L);
	tin.tin = (struct obj *) 0;
	return(0);
}

STATIC_OVL void
start_tin(otmp)		/* called when starting to open a tin */
	register struct obj *otmp;
{
	register int tmp;

	if (metallivorous(youmonst.data)) {
/*JP		You("bite right into the metal tin...");*/
		You("��°�δ̤���ߤϤ��᤿������");
		tmp = 1;
	} else if (nolimbs(youmonst.data)) {
/*JP		You("cannot handle the tin properly to open it.");*/
		You("�̤򤦤ޤ��������ʤ���");
		return;
	} else if (otmp->blessed) {
/*JP		pline_The("tin opens like magic!");*/
		pline("�̤���ˡ�Τ褦�˳�������");
		tmp = 1;
	} else if(uwep) {
		switch(uwep->otyp) {
		case TIN_OPENER:
			tmp = 1;
			break;
		case DAGGER:
		case SILVER_DAGGER:
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
		You("%s��Ȥäƴ̤򳫤��褦�Ȥ�����",
			xname(uwep));
	} else {
no_opener:
/*JP		pline("It is not so easy to open this tin.");*/
		pline("���δ̤򳫤���Τ��ưפʤ��ȤǤϤʤ���");
		if(Glib) {
/*JP			pline_The("tin slips from your %s.",*/
			pline("�̤Ϥ��ʤ���%s�������������",
			      makeplural(body_part(FINGER)));
			if(otmp->quan > 1L) {
				register struct obj *obj;
				obj = splitobj(otmp, 1L);
				if (otmp == uwep) setuwep(obj);
				if (otmp == uswapwep) setuswapwep(obj);
				if (otmp == uquiver) setuqwep(obj);
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
	set_occupation(opentin, "�̤򳫤���", 0);
	return;
}

int
Hear_again()		/* called when waking up after fainting */
{
	flags.soundok = 1;
	return 0;
}

/* called on the "first bite" of rotten food */
STATIC_OVL int
rottenfood(obj)
struct obj *obj;
{
/*JP	pline("Blecch!  Rotten %s!", foodword(obj));*/
	pline("��������ä�%s����", foodword(obj));
	if(!rn2(4)) {
/*JP		if (Hallucination) You_feel("rather trippy.");*/
		if (Hallucination) You("�ؤ�ؤ�����");
/*JP		else You_feel("rather %s.", body_part(LIGHT_HEADED));*/
		else You("%s��", body_part(LIGHT_HEADED));
		make_confused(HConfusion + d(2,4),FALSE);
	} else if(!rn2(4) && !Blind) {
/*JP		pline("Everything suddenly goes dark.");*/
		pline("�������Ƥ��Ť��ʤä���");
		make_blinded((long)d(2,10),FALSE);
	} else if(!rn2(3)) {
		const char *what, *where;
		if (!Blind)
/*JP		    what = "goes",  where = "dark";*/
		    what = "�ʤä�",  where = "�ŰǤ�";
		else if (Levitation || Is_airlevel(&u.uz) ||
			 Is_waterlevel(&u.uz))
/*JP		    what = "you lose control of",  where = "yourself";*/
		    what = "����Ǥ��ʤ��ʤä�",  where = "��ʬ��";
		else
/*JP		    what = "you slap against the",  where = surface(u.ux,u.uy);*/
		    what = "�ˤ֤Ĥ��ä�",  where = surface(u.ux,u.uy);
/*JP		pline_The("world spins and %s %s.", what, where);*/
		pline("��������ž����%s%s.", where, what);
		flags.soundok = 0;
		nomul(-rnd(10));
/*JP		nomovemsg = "You are conscious again.";*/
		nomovemsg = "���ʤ��Ϥޤ������Ť�����";
		afternmv = Hear_again;
		return(1);
	}
	return(0);
}

STATIC_OVL int
eatcorpse(otmp)		/* called when a corpse is selected as food */
	register struct obj *otmp;
{
	int tp = 0, mnum = otmp->corpsenm;
	long rotted = 0L;
	/*JP
	boolean uniq = !!(mons[mnum].geno & G_UNIQ);
	*/

	if (mnum != PM_LIZARD && mnum != PM_LICHEN) {
		long age = peek_at_iced_corpse_age(otmp);

		rotted = (monstermoves - age)/(10L + rn2(20));
		if (otmp->cursed) rotted += 2L;
		else if (otmp->blessed) rotted -= 2L;
	}

	/* KMH, conduct */
	u.uconduct.flesh++;
	if (is_meaty(&mons[mnum])) u.uconduct.meat++;

	if (mnum != PM_ACID_BLOB && rotted > 5L) {
/*JP		pline("Ulch - that %s was tainted!",
		      mons[mnum].mlet == S_FUNGUS ? "fungoid vegetation" :
		      is_meaty(&mons[mnum]) ? "meat" : "protoplasm");
*/
		pline("����������%s����äƤ��롪", 
		      mons[mnum].mlet == S_FUNGUS ? "�ٶݤ˱������줿��ʪ" :
		      is_meaty(&mons[mnum]) ? "��" : "��ʪ");
		if (Sick_resistance) {
/*JP			pline("It doesn't seem at all sickening, though...");
*/
			pline("�������������äƸ�����������");
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
			Sprintf(buf, "��ä�%s�򿩤ٿ�����", corpse_xname(otmp,TRUE));
			make_sick(sick_time, buf, TRUE, SICK_VOMITABLE);
		}
		if (carried(otmp)) useup(otmp);
		else useupf(otmp, 1L);
		return(1);
	} else if (acidic(&mons[mnum]) && !Acid_resistance) {
		tp++;
/*JP
		You("have a very bad case of stomach acid.");
		losehp(rnd(15), "acidic corpse", KILLED_BY_AN);
*/
		pline("�߻���Ĵ�Ҥ��ȤƤⰭ����");
		losehp(rnd(15), "���λ��Τ�", KILLED_BY_AN);
	} else if (poisonous(&mons[mnum]) && rn2(5)) {
		tp++;
/*JP
		pline("Ecch - that must have been poisonous!");
*/
		pline("����������ͭ�Ǥ��ä��ˤ������ʤ���");  
		if(!Poison_resistance) {
			losestr(rnd(4));
/*JP			losehp(rnd(15), "poisonous corpse", KILLED_BY_AN);*/
			losehp(rnd(15), "�Ǥλ��Τ�", KILLED_BY_AN);
/*JP		} else	You("seem unaffected by the poison.");*/
		} else	You("�Ǥαƶ�������ʤ��褦����");
	/* now any corpse left too long will make you mildly ill */
	} else if ((rotted > 5L || (rotted > 3L && rn2(5)))
					&& !Sick_resistance) {
		tp++;
/*JP		You("feel%s sick.", (Sick) ? " very" : "");
		losehp(rnd(8), "cadaver", KILLED_BY_AN);
*/
		You("%s��ʬ��������", (Sick) ? "�ȤƤ�" : "");
		losehp(rnd(8), "������Τ�", KILLED_BY_AN);
	}
	if (!tp && mnum != PM_LIZARD && mnum != PM_LICHEN &&
			(otmp->orotten || !rn2(7))) {
	    if (rottenfood(otmp)) {
		otmp->orotten = TRUE;
		(void)touchfood(otmp);
		return(1);
	    }
	    otmp->oeaten >>= 2;
	} else {
/*JP	    pline("%s%s %s!",
		  !uniq ? "This " : !type_is_pname(&mons[mnum]) ? "The " : "",
		  food_xname(otmp, FALSE),
		  (carnivorous(youmonst.data) && !herbivorous(youmonst.data)) ?
			"is delicious" : "tastes terrible");
*/
	    pline("����%s��%s��", food_xname(otmp, FALSE),
		  (carnivorous(youmonst.data) && !herbivorous(youmonst.data)) ?
		  	"�ȤƤ�ݤ�" : "�Ҥɤ�̣��");
	}
	if (Role_if(PM_MONK) && is_meaty(&mons[mnum])) {
/*JP	    pline("You feel guilty.");*/
	    pline("��򴶤�����");
	    adjalign(-1);
	}

	/* delay is weight dependent */
	victual.reqtime = 3 + (mons[mnum].cwt >> 6);
	return(0);
}

STATIC_OVL void
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
	Sprintf(msgbuf, "%s�򿩤٤�", food_xname(otmp, TRUE));
	set_occupation(eatfood, msgbuf, 0);
}


STATIC_OVL void
fprefx(otmp)		/* called on "first bite" of (non-corpse) food */
struct obj *otmp;
{
	/* KMH, conduct */
	if (objects[otmp->otyp].oc_material == FLESH) u.uconduct.flesh++;

	switch(otmp->otyp) {
	    case FOOD_RATION:
/*JP		if(u.uhunger <= 200)
		    if (Hallucination) pline("Oh wow, like, superior, man!");
		    else	       pline("That food really hit the spot!");
		else if(u.uhunger <= 700) pline("That satiated your stomach!");
*/
		if(u.uhunger <= 200){
		    if (Hallucination) pline("�ޤä���Ȥ��ơ�����Ǥ��Ƥ��Ĥ����ʤ������줾��ˤΥ�˥塼����");
		    else	       pline("���ο���ʪ�������˿���ʬ�ʤ���");
		}
		else if(u.uhunger <= 700) pline("��ʢ�ˤʤä���");
		break;
	    case TRIPE_RATION:
		u.uconduct.meat++;
		if (carnivorous(youmonst.data) && !humanoid(youmonst.data))
/*JP		    pline("That tripe ration was surprisingly good!");*/
 		    pline("���Τۤ����Ϥ��ɤ��ۤɻݤ���");
		else {
/*JP		    pline("Yak - dog food!");*/
		    pline("���Τۤ����Ϥ��ɤ��ۤɻݤ���");
		    more_experienced(1,0);
		    flags.botl = 1;
		}
		if (rn2(2) && (!carnivorous(youmonst.data) || humanoid(youmonst.data))) {
			make_vomiting((long)rn1(victual.reqtime, 14), FALSE);
		}
		break;
	    case CLOVE_OF_GARLIC:
		if (is_undead(youmonst.data)) {
			make_vomiting((long)rn1(victual.reqtime, 5), FALSE);
			break;
		}
		/* Fall through otherwise */
	    default:
		if (otmp->otyp == EGG && is_meaty(&mons[otmp->corpsenm]))
			u.uconduct.meat++;
		if (otmp->otyp==SLIME_MOLD && !otmp->cursed
			&& otmp->spe == current_fruit)
/*JP		    pline("My, that was a %s %s!",*/
		    pline("���䡤�ʤ��%s%s����",
/*JP			  Hallucination ? "primo" : "yummy",*/
			  Hallucination ? "���ʤ�" : "��������",
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
#ifdef MAC	/* KMH -- Why should Unix have all the fun? */
		if (otmp->otyp == APPLE) {
			pline("Delicious!  Must be a Macintosh!");
		} else
#endif
		if (otmp->otyp == EGG && stale_egg(otmp)) {
/*JP		    pline("Ugh.  Rotten egg.");*//* perhaps others like it */
		    pline("����������ä������");
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
		    pline("����%s��%s", singular(otmp, xname),
		      otmp->cursed ? (Hallucination ? "��äݤ���" : "�Ҥɤ�̣����") :
		      otmp->otyp == CRAM_RATION ? "���äѤꤷ�Ƥ���" :
		      Hallucination ? "���֤��֤��Ƥ��롪" : "���ޤ���");
		break;
	}
}

STATIC_OVL void
eataccessory(otmp)
struct obj *otmp;
{
	int typ = otmp->otyp;
	int oldprop;

	/* Note: rings are not so common that this is unbalancing. */
	/* (How often do you even _find_ 3 rings of polymorph in a game?) */
	oldprop = !!(u.uprops[objects[typ].oc_oprop].intrinsic);
	if (otmp == uleft || otmp == uright) {
	    Ring_gone(otmp);
	    if (u.uhp <= 0) return; /* died from sink fall */
	}
	otmp->known = otmp->dknown = 1; /* by taste */
	if (!rn2(otmp->oclass == RING_CLASS ? 3 : 5))
	  switch (otmp->otyp) {
	    default:
	        if (!objects[typ].oc_oprop) break; /* should never happen */

		if (!(u.uprops[objects[typ].oc_oprop].intrinsic & FROMOUTSIDE))
/*JP		    pline("Magic spreads through your body as you digest the %s.",
			  otmp->oclass == RING_CLASS ? "ring" : "amulet");
*/
		    pline("���ʤ���%s��ò�����ȡ��������Ϥ��Τˤ��ߤ������",
			  otmp->oclass == RING_CLASS ? "����" : "�����");

		u.uprops[objects[typ].oc_oprop].intrinsic |= FROMOUTSIDE;

		switch (typ) {
		  case RIN_SEE_INVISIBLE:
		    set_mimic_blocking();
		    see_monsters();
		    if (Invis && !oldprop && !ESee_invisible &&
		    		!perceives(youmonst.data) && !Blind) {
			newsym(u.ux,u.uy);
/*JP			pline("Suddenly you can see yourself.");*/
			pline("������ʬ���Ȥ�������褦�ˤʤä���");
			makeknown(typ);
		    }
		    break;
		  case RIN_INVISIBILITY:
			if (!oldprop && !EInvis && !BInvis &&
					!See_invisible && !Blind) {
			newsym(u.ux,u.uy);
/*JP			Your("body takes on a %s transparency...",
				Hallucination ? "normal" : "strange");*/
			pline("%s���ʤ����Τ�Ʃ�������ä�������",
				Hallucination ? "������ޤ��ʤ��Ȥ���" : "��̯�ʤ��Ȥ�");
			makeknown(typ);
		    }
		    break;
		  case RIN_PROTECTION_FROM_SHAPE_CHAN:
		    rescham();
		    break;
		  case RIN_LEVITATION:
		    if (!Levitation) {
			float_up();
			incr_itimeout(&HLevitation, d(10,20));
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
	    case RIN_GAIN_CONSTITUTION:
		if (adjattrib(A_CON, otmp->spe, -1))
		    makeknown(typ);
		break;
	    case RIN_INCREASE_ACCURACY:
		u.uhitinc += otmp->spe;
		break;
	    case RIN_INCREASE_DAMAGE:
		u.udaminc += otmp->spe;
		break;
	    case RIN_PROTECTION:
		HProtection |= FROMOUTSIDE;
		u.ublessed += otmp->spe;
		flags.botl = 1;
		break;
	    case RIN_FREE_ACTION:
		/* Give sleep resistance instead */
		if (!Sleep_resistance)
/*JP		    You_feel("wide awake.");*/
		    You("�Ѥä����ܤ����᤿��");
		HSleep_resistance |= FROMOUTSIDE;
		break;
	    case AMULET_OF_CHANGE:
		makeknown(typ);
		change_sex();
/*JP		You("are suddenly very %s!",
		    flags.female ? "feminine" : "masculine");*/
		You("����%s��", 
		    flags.female ? "���äݤ��ʤä�" : "�������ˤʤä�");
		flags.botl = 1;
		break;
	    case AMULET_OF_STRANGULATION: /* bad idea! */
		choke(otmp);
		break;
	    case AMULET_OF_RESTFUL_SLEEP: /* another bad idea! */
		HSleeping = rnd(100);
		break;
		case RIN_SUSTAIN_ABILITY:
	    case AMULET_OF_LIFE_SAVING:
	    case AMULET_OF_REFLECTION: /* nice try */
	    /* can't eat Amulet of Yendor or fakes,
	     * and no oc_prop even if you could -3.
	     */
		break;
	  }
}

STATIC_OVL void
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

	/* KMH -- idea by "Tommy the Terrorist" */
	if ((otmp->otyp == TRIDENT) && !otmp->cursed)
	{
/*JP		pline(Hallucination ? "Four out of five dentists agree." :
				"That was pure chewing satisfaction!")
*/
		pline(Hallucination ? "�޿ͤ˻Ϳͤλ���Ԥ��ȥ饤�ǥ�Ȥ����ᤷ�Ƥ��ޤ���" :
				"���˳��ߤ�������������������");
		exercise(A_WIS, TRUE);
	}
	if ((otmp->otyp == FLINT) && !otmp->cursed)
	{
/*JP		pline("Yabba-dabba delicious!");*/
		pline("��åХ��åФ��ޤ���");
/* Flintstones�Υѥ�ǥ��ߤ����Ǥ�������褯�Τ餺 */
		exercise(A_CON, TRUE);
	}

	if (otmp == uwep && otmp->quan == 1L) uwepgone();
	if (otmp == uquiver && otmp->quan == 1L) uqwepgone();
	if (otmp == uswapwep && otmp->quan == 1L) uswapwepgone();

	if (otmp == uball) unpunish();
	if (otmp == uchain) unpunish(); /* but no useup() */
	else if (carried(otmp)) useup(otmp);
	else useupf(otmp, 1L);
}

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
static const char *foodwords[] = {
/*JP	"meal", "liquid", "wax", "food", "meat",
	"paper", "cloth", "leather", "wood", "bone", "scale",
	"metal", "metal", "metal", "silver", "gold", "platinum", "mithril",
	"plastic", "glass", "rich food", "stone"
*/
	"��", "����", "��", "����", "��",
	"��", "��", "��", "��", "��", "��",
	"��°", "��°", "��°", "��", "��", "�ץ����", "�ߥ����",
	"�ץ饹���å�", "���饹", "�������", "��"
};

STATIC_OVL const char *
foodword(otmp)
register struct obj *otmp;
{
/*JP	if (otmp->oclass == FOOD_CLASS) return "food";*/
	if (otmp->oclass == FOOD_CLASS) return "����";
	if (otmp->oclass == GEM_CLASS &&
	    objects[otmp->otyp].oc_material == GLASS &&
	    otmp->dknown)
		makeknown(otmp->otyp);
	return foodwords[objects[otmp->otyp].oc_material];
}

STATIC_OVL void
fpostfx(otmp)		/* called after consuming (non-corpse) food */
register struct obj *otmp;
{
	switch(otmp->otyp) {
	    case SPRIG_OF_WOLFSBANE:
		if (u.ulycn >= LOW_PM || is_were(youmonst.data))
		    you_unwere(TRUE);
		break;
	    case CARROT:
		make_blinded(0L,TRUE);
		break;
	    case FORTUNE_COOKIE:
		outrumor(bcsign(otmp), BY_COOKIE);
		if (!Blind) u.uconduct.literate++;
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
			killer = "��ä�����를�꡼�򿩤ٿ����Ǥ�";
			done(DIED);
		    }
		}
		if(!otmp->cursed) heal_legs();
		break;
	    case EGG:
		if (touch_petrifies(&mons[otmp->corpsenm])) {
		    if (!Stone_resistance &&
			!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
			static char kbuf[BUFSIZ];

			if (!Stoned) Stoned = 5;
			killer_format = KILLED_BY_AN;
/*JP			Sprintf(kbuf, "%s egg", mons[otmp->corpsenm].mname);*/
			Sprintf(kbuf, "%s�����", jtrns_mon(mons[otmp->corpsenm].mname,-1));
			killer = kbuf;
		    }
		}
		break;
	    case EUCALYPTUS_LEAF:
		if (Sick && !otmp->cursed)
		    make_sick(0L, (char *)0, TRUE, SICK_ALL);
		if (Vomiting && !otmp->cursed)
		    make_vomiting(0L, TRUE);
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
		pline("©��Ǥ��ʤ��Τˡ��ɤ���äƿ��٤��餤���������");
		return 0;
	}
/*JP	if (!(otmp = floorfood("eat", 0))) return 0;*/
	if (!(otmp = floorfood("���٤�", 0))) return 0;
	if (check_capacity((char *)0)) return 0;

	/* We have to make non-foods take 1 move to eat, unless we want to
	 * do ridiculous amounts of coding to deal with partly eaten plate
	 * mails, players who polymorph back to human in the middle of their
	 * metallic meal, etc....
	 */
	if (!is_edible(otmp)) {
/*JP	    You("cannot eat that!");*/
	    You("����򿩤٤��ʤ���");
	    return 0;
	} else if ((otmp->owornmask & (W_ARMOR|W_TOOL|W_AMUL
#ifdef STEED
			|W_SADDLE
#endif
			)) != 0) {
	    /* let them eat rings */
/*JP	    You_cant("eat %s you're wearing.", something);*/
	    You("�ȤˤĤ��Ƥ���֤Ͽ��٤�ʤ���");
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
		pline("����������%s���ɻ�����Ƥ��롪", xname(otmp));
		/* The regurgitated object's rustproofing is gone now */
		otmp->oerodeproof = 0;
		make_stunned(HStun + rn2(10), TRUE);
/*JP		pline("You spit %s out onto the %s.", the(xname(otmp)),*/
		pline("���ʤ���%s��%s���Ǥ��Ф�����", the(xname(otmp)),
			surface(u.ux, u.uy));
		if (carried(otmp)) {
			freeinv(otmp);
			dropy(otmp);
		}
		stackobj(otmp);
		return 1;
	}
	/* KMH -- Slow digestion is... undigestable */
	if (otmp->otyp == RIN_SLOW_DIGESTION) {
/*JP		pline("This ring is undigestable!");*/
		pline("���λ��ؤϾò����ˤ�����");
		(void) rottenfood(otmp);
		docall(otmp);
		return (1);
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
		pline("����������ͭ�Ǥ��ä��˰㤤�ʤ���");  
		if(!Poison_resistance) {
		    losestr(rnd(4));
/*JP*/
/*JP		    losehp(rnd(15), xname(otmp), KILLED_BY_AN);*/
		    {
		      char jbuf[BUFSIZ];
		      Strcpy(jbuf,xname(otmp));
		      Strcat(jbuf,"��");
		      losehp(rnd(15), jbuf, KILLED_BY_AN);
		    }
		} else
/*JP		    You("seem unaffected by the poison.");*/
		    You("�Ǥαƶ�������ʤ��褦����");
	    } else if (!otmp->cursed)
/*JP		pline("This %s is delicious!",*/
		pline("����%s�ϻݤ���",
		      otmp->oclass == GOLD_CLASS ? foodword(otmp) :
		      singular(otmp, xname));
	    eatspecial();
	    return 1;
	}

	/* KMH, conduct */
	u.uconduct.food++;

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
	    You("������Ƴ�������");
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
STATIC_OVL int
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
		&& (carnivorous(youmonst.data) || herbivorous(youmonst.data))
		&& !Slow_digestion)
	    u.uhunger--;		/* ordinary food consumption */

	if (moves % 2) {	/* odd turns */
	    /* Regeneration uses up food, unless due to an artifact */
	    if (HRegeneration || ((ERegeneration & (~W_ART)) &&
				(ERegeneration != W_WEP || !uwep->oartifact)))
			u.uhunger--;
	    if (near_capacity() > SLT_ENCUMBER) u.uhunger--;
	} else {		/* even turns */
	    if (Hunger) u.uhunger--;
	    /* Conflict uses up food too */
	    if (HConflict || (EConflict & (~W_ARTI))) u.uhunger--;
	    /* +0 charged rings don't do anything, so don't affect hunger */
	    /* Slow digestion still uses ring hunger */
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
			choke(tin.tin);	/* may be null */
		/* no reset_eat() */
	    }
	} else {
	    /* Have lesshungry() report when you're nearly full so all eating
	     * warns when you're about to choke.
	     */
	    if (u.uhunger >= 1500) {
		if (!victual.eating || (victual.eating && !victual.fullwarn)) {
/*JP		    pline("You're having a hard time getting all of it down.");
		    nomovemsg = "You're finally finished.";*/
		    pline("���Ƥ���ߤ���ˤϻ��֤������롥");
		    nomovemsg = "��äȿ��ٽ�������";
		    if (!victual.eating)
			multi = -2;
		    else {
			victual.fullwarn = TRUE;
			if (victual.canchoke && victual.reqtime > 1) {
			    /* a one-gulp food will not survive a stop */
/*JP			    if (yn_function("Stop eating?",ynchars,'y')=='y') {*/
			    if (yn_function("���٤�Τ����Ǥ��ޤ�����",ynchars,'y')=='y') {
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
		nomovemsg = "���ʤ��������Ť�����";
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
				You("ʢ�����ä��ݤ줿��");
				flags.soundok = 0;
				nomul(-10+(u.uhunger/10));
/*JP				nomovemsg = "You regain consciousness.";*/
				nomovemsg = "���ʤ��������Ť�����";
				afternmv = unfaint;
				newhs = FAINTED;
			}
#ifdef NEWBIE
			if(!newbie.fainted){
			     pline("�ҥ��: �ԥ���ΤȤ���M-p�ǵ���Ȥ�����⤢�롥���ꤹ������ա�");
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
			You("��ष����");
			killer_format = KILLED_BY;
			killer = "������­�ǲ�ष��";
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
				"�ϥ�إ꤬���ä���":
				"�ϥ�إ�إ�ϥ顥");
			} else
/*JP			    You((!incr) ? "only feel hungry now." :
				  (u.uhunger < 145) ? "feel hungry." :
				   "are beginning to feel hungry.");*/
			    You((!incr) ? "ñ��ʢ�ڥ����֤ˤʤä���" :
				  (u.uhunger < 145) ? "��ʢ���򴶤�����" :
				   "��ʢ���򤪤ܤ��Ϥ��᤿��");
			if (incr && occupation &&
			    (occupation != eatfood && occupation != opentin))
			    stop_occupation();
			break;
		case WEAK:
			if (Hallucination)
			    pline((!incr) ?
/*JP				  "You still have the munchies." :
      "The munchies are interfering with your motor capabilities.");*/
				  "�ϥ�إ꤬����ʤ���":
 				  "�ϥ�إ꤬�⡼������ǽ��˸�����Ƥ��롥");
			else if (incr &&
				(Role_if(PM_WIZARD) || Race_if(PM_ELF) || Role_if(PM_VALKYRIE)))
/*JP			    pline("%s needs food, badly!",
			    		(Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE)) ?
			    		urole.name.m : "Elf");
*/
			    pline("%s�ˤϻ�޿�����ɬ�פ���",
			    		(Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE)) ?
			    		urole.name.m : "�����");
			else
/*JP			    You((!incr) ? "feel weak now." :
				  (u.uhunger < 45) ? "feel weak." :
				   "are beginning to feel weak.");
*/
			    You((!incr) ? "�����֤ˤʤä���":
				  (u.uhunger < 45) ? "��夷�Ƥ�����" :
				   "�夯�ʤäƤ����褦�˴�������");
			if (incr && occupation &&
			    (occupation != eatfood && occupation != opentin))
			    stop_occupation();
			break;
		}
		u.uhs = newhs;
		flags.botl = 1;
		bot();
		if ((Upolyd ? u.mh : u.uhp) < 1) {
/*JP			You("die from hunger and exhaustion.");*/
			You("��ʢ�ȿ��ǻ�����");
			killer_format = KILLED_BY;
/*JP			killer = "exhaustion";*/
			killer = "��ʢ�ȿ���";
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
	boolean feeding = (!strcmp(verb, "���٤�"));

	if (feeding && metallivorous(youmonst.data)) {
	    struct obj *gold;
	    struct trap *ttmp = t_at(u.ux, u.uy);

	    if (ttmp && ttmp->tseen && ttmp->ttyp == BEAR_TRAP) {
		/* If not already stuck in the trap, perhaps there should
		   be a chance to becoming trapped?  Probably not, because
		   then the trap would just get eaten on the _next_ turn... */
/*JP		Sprintf(qbuf, "There is a bear trap here (%s); eat it?",*/
		Sprintf(qbuf, "�����ˤϷ����(%s)������",
			(u.utrap && u.utraptype == TT_BEARTRAP) ?
/*JP				"holding you" : "armed");*/
				"���ʤ����Ϥޤ��Ƥ���" : "�Ϥޤ��Ƥ���");
		if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
		    u.utrap = u.utraptype = 0;
		    deltrap(ttmp);
		    return mksobj(BEARTRAP, TRUE, FALSE);
		} else if (c == 'q') {
		    return (struct obj *)0;
		}
	    }

	    if (
#ifdef STEED
	    	!u.usteed && 
#endif
		(gold = g_at(u.ux, u.uy)) != 0) {
		if (gold->quan == 1L)
/*JP		    Sprintf(qbuf, "There is 1 gold piece here; eat it?");*/
		    Sprintf(qbuf, "�����ˤ�1������ɤ��롥���٤ޤ�����");
		else
/*JP		    Sprintf(qbuf, "There are %ld gold pieces here; eat them?",*/
		    Sprintf(qbuf, "�����ˤ�%ld������ɤ��롥���٤ޤ�����",
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
	if (
#ifdef STEED
	    !u.usteed && 
#endif
	    !(Levitation && !Is_airlevel(&u.uz)  && !Is_waterlevel(&u.uz))
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
			Sprintf(qbuf, "�����ˤ�%s�����롥%s��",
				doname(otmp), jconj(verb,"�ޤ���"));
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
		You_cant("�����%s���ȤϤǤ��ʤ���", verb);
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
