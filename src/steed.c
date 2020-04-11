/*	SCCS Id: @(#)steed.c	3.3	1999/08/16	*/
/* Copyright (c) Kevin Hugo, 1998-1999. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (00/3/10)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"


#ifdef STEED

/* Monsters that might be ridden */
static NEARDATA const char steeds[] = {
	S_QUADRUPED, S_UNICORN, S_ANGEL, S_CENTAUR, S_DRAGON, S_JABBERWOCK, '\0'
};


/*** Putting the saddle on ***/

/* Can this monster wear a saddle? */
boolean
can_saddle(mtmp)
	struct monst *mtmp;
{
	struct permonst *ptr = mtmp->data;

	return (index(steeds, ptr->mlet) &&
			!humanoid(ptr) && (ptr->msize >= MZ_MEDIUM) &&
			!amorphous(ptr) && !noncorporeal(ptr) &&
			!is_whirly(ptr) && !unsolid(ptr));
}


int
use_saddle(otmp)
	struct obj *otmp;
{
	struct monst *mtmp;
	struct permonst *ptr;
	int chance;
	const char *s;


	/* Can you use it? */
	if (nohands(youmonst.data)) {
#if 0
		You("have no hands!");	/* not `body_part(HAND)' */
#endif
		You("手がない！");	/* not `body_part(HAND)' */
		return 0;
	} else if (!freehand()) {
/*JP		You("have no free %s.", body_part(HAND));*/
		You("自由な%sがない！", body_part(HAND));
		return 0;
	}

	/* Select an animal */
	if (u.uswallow || Underwater || !getdir((char *)0)) {
/*JP	    pline("Never mind.");*/
	    pline("そんなことは考えるな．");
	    return 0;
	}
	if (!u.dx && !u.dy) {
/*JP	    pline("Saddle yourself?  Very funny...");*/
	    pline("自分自身に鞍？おもしろい．．．");
	    return 0;
	}
	if (!isok(u.ux+u.dx, u.uy+u.dy) ||
			!(mtmp = m_at(u.ux+u.dx, u.uy+u.dy)) ||
			!canspotmon(mtmp)) {
/*JP	    pline("I see nobody there.");*/
	    pline("そこには何もいないようだ．");
	    return 1;
	}

	/* Is this a valid monster? */
	if (mtmp->misc_worn_check & W_SADDLE ||
			which_armor(mtmp, W_SADDLE)) {
/*JP	    pline("%s is already saddled.", Monnam(mtmp));*/
	    pline("%sはもう鞍が取りつけられている．", Monnam(mtmp));
	    return 1;
	}
	ptr = mtmp->data;
	if (touch_petrifies(ptr) && !Stone_resistance) {
	    char kbuf[BUFSZ];

/*JP	    You("touch %s.", mon_nam(mtmp));
 	    if (!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
			Sprintf(kbuf, "attempting to saddle %s", a_monnam(mtmp));
			instapetrify(kbuf);
 	    }
*/
	    You("%sに触れた．", mon_nam(mtmp));
 	    if (!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
			Sprintf(kbuf, "%sに鞍を取りつけようとして", a_monnam(mtmp));
			instapetrify(kbuf);
 	    }
	}
	if (ptr == &mons[PM_INCUBUS] || ptr == &mons[PM_SUCCUBUS]) {
/*JP	    pline("Shame on you!");*/
	    pline("みっともないぞ！");
	    exercise(A_WIS, FALSE);
	    return 1;
	}
	if (mtmp->isminion || mtmp->isshk || mtmp->ispriest ||
			mtmp->isgd || mtmp->iswiz) {
/*JP	    pline("I think %s would mind.", mon_nam(mtmp));*/
	    pline("%sは鞍をつけられることを気にしているようだ．", mon_nam(mtmp));
	    return 1;
	}
	if (!can_saddle(mtmp)) {
/*JP		You_cant("saddle such a creature.");*/
		You("その生き物に鞍はとりつけられない．");
		return 1;
	}

	/* Calculate your chance */
	chance = ACURR(A_DEX) + ACURR(A_CHA)/2 + 2*mtmp->mtame;
	chance += u.ulevel * (mtmp->mtame ? 20 : 5);
	if (!mtmp->mtame) chance -= 10*mtmp->m_lev;
	if (Role_if(PM_KNIGHT))
	    chance += 20;
	switch (P_SKILL(P_RIDING)) {
	case P_ISRESTRICTED:
	case P_UNSKILLED:
	default:
	    chance -= 20;	break;
	case P_BASIC:
	    break;
	case P_SKILLED:
	    chance += 15;	break;
	case P_EXPERT:
	    chance += 30;	break;
	}
	if (Confusion || Fumbling || Glib)
	    chance -= 20;
	else if (uarmg &&
		(s = OBJ_DESCR(objects[uarmg->otyp])) != (char *)0 &&
		!strncmp(s, "riding ", 7))
	    /* Bonus for wearing "riding" (but not fumbling) gloves */
	    chance += 10;
	else if (uarmf &&
		(s = OBJ_DESCR(objects[uarmf->otyp])) != (char *)0 &&
		!strncmp(s, "riding ", 7))
	    /* ... or for "riding boots" */
	    chance += 10;
	if (otmp->cursed)
	    chance -= 50;

	/* Make the attempt */
	if (rn2(100) < chance) {
/*JP	    You("put the saddle on %s.", mon_nam(mtmp));*/
	    You("鞍を%sに取りつけた．", mon_nam(mtmp));
	    freeinv(otmp);
	    mpickobj(mtmp, otmp);
	    mtmp->misc_worn_check |= W_SADDLE;
	    otmp->owornmask = W_SADDLE;
	    otmp->leashmon = mtmp->m_id;
	    update_mon_intrinsics(mtmp, otmp, TRUE);
	} else
/*JP	    pline("%s resists!", Monnam(mtmp));*/
	    pline("%sは拒否した！", Monnam(mtmp));
	return 1;
}


/*** Riding the monster ***/

/* Can we ride this monster?  Caller should also check can_saddle() */
boolean
can_ride(mtmp)
	struct monst *mtmp;
{
	return (mtmp->mtame && humanoid(youmonst.data) &&
			!verysmall(youmonst.data) && !bigmonst(youmonst.data) &&
			(!Underwater || is_swimmer(mtmp->data)));
}


int
doride()
{
	boolean forcemount = FALSE;
	if (u.usteed)
	    dismount_steed(DISMOUNT_BYCHOICE);
	else if(getdir((char *)0) && isok(u.ux+u.dx, u.uy+u.dy)) {
#ifdef WIZARD
/*JP	if (wizard && yn("Force the mount to succeed?") == 'y')
		forcemount = TRUE;
*/
	if (wizard && yn("無理矢理成功させますか？") == 'y')
		forcemount = TRUE;
#endif
	    (void) mount_steed(m_at(u.ux+u.dx, u.uy+u.dy), forcemount);
	}
	return 1;
}


/* Start riding, with the given monster */
boolean
mount_steed(mtmp, force)
	struct monst *mtmp;	/* The animal */
	boolean force;		/* Quietly force this animal */
{
	struct obj *otmp;
	char buf[BUFSZ];
	struct permonst *ptr;


	/* Sanity checks */
	if (u.usteed) {
	    if (!force)
/*JP	    	You("are already riding %s.", mon_nam(u.usteed));*/
	    	You("もう%sに乗っている．", mon_nam(u.usteed));
	    return (FALSE);
	}

	/* Is the player in the right form? */
	if (Upolyd && (!humanoid(youmonst.data) || verysmall(youmonst.data) ||
			bigmonst(youmonst.data))) {
	    if (!force)
/*JP	    	You("won't fit on a saddle.");*/
	    	You("鞍に合わない．");
	    return (FALSE);
	}
	if(!force && (near_capacity() > SLT_ENCUMBER)) {
/*JP	    You_cant("do that while carrying so much stuff.");*/
	    You("沢山物を持ちすぎており出来ない．");
	    return (FALSE);
	}

	/* Can the player reach and see the monster? */
    if (u.uswallow || u.ustuck || u.utrap || Punished) {
        if (!force)
/*Jp        	You("are stuck here for now.");*/
        	You("はまっているので出来ない．");
        return (FALSE);
    }
	if (!mtmp || (!force && ((Blind && !Blind_telepat) ||
		mtmp->mundetected ||
		mtmp->m_ap_type == M_AP_FURNITURE ||
		mtmp->m_ap_type == M_AP_OBJECT))) {
	    if (!force)
/*JP	    	pline("I see nobody there.");*/
	    	pline("そこには何も見えない．");
	    return (FALSE);
	}

	/* Is this a valid monster? */
	otmp = which_armor(mtmp, W_SADDLE);
	if (!otmp) {
/*JP	    pline("%s is not saddled.", Monnam(mtmp));*/
	    pline("%sには鞍は取りつけられていない．", Monnam(mtmp));
	    return (FALSE);
	}
	ptr = mtmp->data;
	if (touch_petrifies(ptr) && !Stone_resistance) {
	    char kbuf[BUFSZ];

/*JP	    You("touch %s.", mon_nam(mtmp));
 	    Sprintf(kbuf, "attempting to ride %s", an(mtmp->data->mname));
*/
	    You("%sに触れた．", mon_nam(mtmp));
 	    Sprintf(kbuf, "%sに乗ろうとして", a_monnam(mtmp));
	    instapetrify(kbuf);
	}
	if (!mtmp->mtame || mtmp->isminion) {
	    if (!force)
/*JP	    	pline("I think %s would mind.", mon_nam(mtmp));*/
	    	pline("%sは鞍をつけられることを気にしているようだ．", mon_nam(mtmp));
	    return (FALSE);
	}
	if (!force && !Role_if(PM_KNIGHT) && !(--mtmp->mtame)) {
/*JP	    pline("%s resists!", Monnam(mtmp));*/
	    pline("%sは拒否した！", Monnam(mtmp));
	    return (FALSE);
	}
	if (!force && Underwater && !is_swimmer(ptr)) {
/*JP	    You_cant("ride that creature while under water.");*/
	    You("水中で乗ることはできない．");
	    return (FALSE);
	}
	if (!can_saddle(mtmp) || !can_ride(mtmp)) {
	    if (!force)
/*JP	    	You_cant("ride such a creature.");*/
	    	You("その生き物に乗ることはできない．");
	    return (0);
	}

	/* Is the player impaired? */
	if (!force && !is_floater(ptr) && !is_flyer(ptr) &&
			Levitation && !Lev_at_will) {
/*JP	    You("cannot reach %s.", mon_nam(mtmp));*/
	    You("%sに届かない．", mon_nam(mtmp));
	    return (FALSE);
	}
	if (!force && uarm && is_metallic(uarm) &&
			greatest_erosion(uarm)) {
/*JP	    Your("%s armor is too stiff to be able to mount %s.",
			uarm->oeroded ? "rusty" : "corroded",
*/
	    Your("%s鎧はギシギシいっており%sに乗れない",
			uarm->oeroded ? "錆びた" : "腐食した",
			mon_nam(mtmp));
	    return (FALSE);
	}
	if (!force && (Confusion || Fumbling || Glib || Wounded_legs ||
			otmp->cursed || (u.ulevel+mtmp->mtame < rnd(MAXULEV/2+5)))) {
/*JP	    You("slip while trying to get on %s.", mon_nam(mtmp));
	    Sprintf(buf, "slipped while mounting %s", a_monnam(mtmp));
*/
	    You("%sに乗ろうとしてすべった．", mon_nam(mtmp));
	    Sprintf(buf, "%sに乗ろうとしてすべって", a_monnam(mtmp));
	    losehp(rn1(5,10), buf, NO_KILLER_PREFIX);
	    return (FALSE);
	}

	/* Success */
	if (!force) {
	    if (Levitation && !is_floater(ptr) && !is_flyer(ptr))
	    	/* Must have Lev_at_will at this point */
/*JP	    	pline("%s magically floats up!", Monnam(mtmp));
	    You("mount %s.", mon_nam(mtmp));
*/
	    	pline("%sは魔法の力で浮いた！", Monnam(mtmp));
	    You("%sに乗った．", mon_nam(mtmp));
	}
	u.usteed = mtmp;
	remove_monster(mtmp->mx, mtmp->my);
	teleds(mtmp->mx, mtmp->my);
	return (TRUE);
}


/* You and your steed have moved */
void
exercise_steed()
{
	if (!u.usteed)
		return;

	/* It takes many turns of riding to exercise skill */
	if (u.urideturns++ >= 100) {
	    u.urideturns = 0;
	    use_skill(P_RIDING, 1);
	}
	return;
}


/* The player kicks or whips the steed */
void
kick_steed()
{
	if (!u.usteed)
	    return;

	/* Make the steed less tame and check if it resists */
	if (u.usteed->mtame) u.usteed->mtame--;
	if (!u.usteed->mtame || (u.ulevel+u.usteed->mtame < rnd(MAXULEV/2+5))) {
	    dismount_steed(DISMOUNT_THROWN);
	    return;
	}

/*JP	pline("%s gallops!", Monnam(u.usteed));*/
	pline("%sは速足になった！", Monnam(u.usteed));
	u.ugallop += rn1(20, 30);
	return;
}


/* Stop riding the current steed */
void
dismount_steed(reason)
	int reason;		/* Player was thrown off etc. */
{
	struct monst *mtmp;
	struct obj *otmp;
	coord cc;
/*JP	const char *verb = "fall";*/
	const char *verb = "落ちた";
	boolean repair_leg_damage = TRUE;
	unsigned save_utrap = u.utrap;
	
	/* Sanity checks */
	if (!(mtmp = u.usteed))
	    /* Just return silently */
	    return;

	/* Check the reason for dismounting */
	otmp = which_armor(mtmp, W_SADDLE);
	switch (reason) {
	    case DISMOUNT_THROWN:
/*JP		verb = "are thrown";*/
		verb = "ふり落された";
	    case DISMOUNT_FELL:
/*JP		You("%s off of %s!", verb, mon_nam(mtmp));
		losehp(rn1(10,10), "riding accident", KILLED_BY_AN);
*/
		You("%sから%s！", mon_nam(mtmp), verb);
		losehp(rn1(10,10), "乗りものから落ちて", KILLED_BY_AN);
		HWounded_legs += rn1(5, 5);
		EWounded_legs |= BOTH_SIDES;
		repair_leg_damage = FALSE;
		break;
	    case DISMOUNT_POLY:
/*JP		You("can no longer ride %s.", mon_nam(u.usteed));*/
		You("%sに乗ってられない．", mon_nam(u.usteed));
		break;
	    case DISMOUNT_ENGULFED:
		/* caller displays message */
		break;
	    case DISMOUNT_GENERIC:
		/* no messages, just make it so */
		break;
	    case DISMOUNT_BYCHOICE:
	    default:
		if (otmp && otmp->cursed) {
/*JP		    You("can't.  The saddle seems to be cursed.");*/
		    You("できない．鞍は呪われているようだ．");
		    otmp->bknown = TRUE;
		    return;
		}
		if (!mtmp->mnamelth) {
/*JP			pline("You've been through the dungeon on %s with no name.",*/
			pline("あなたは名無の%sと共にダンジョン内にいる．",
			      a_monnam(mtmp));
			if (Hallucination)
/*JP				pline("It felt good to get out of the rain.");*/
			    pline("雨のなかから抜け出るのによいと思った．");
		} else
/*JP			You("dismount %s.", mon_nam(mtmp));*/
			You("%sから降りた．", mon_nam(mtmp));
	}
 	/* While riding these refer to the steed's legs
	 * so after dismounting they refer to the player's
	 * legs once again.
	 */
	if (repair_leg_damage) HWounded_legs = EWounded_legs = 0;

	/* Release the steed and saddle */
	u.usteed = 0;
	u.ugallop = 0L;

	/* Set player and steed's position.  Try moving the player first */
	place_monster(mtmp, u.ux, u.uy);
	if (!u.uswallow && !u.ustuck && enexto(&cc, u.ux, u.uy, youmonst.data)) {
	    /* The steed may drop into water/lava */
	    if (mtmp->mhp > 0 && is_pool(u.ux,u.uy) &&
	    		!is_flyer(mtmp->data) && !is_floater(mtmp->data) &&
	    		!is_clinger(mtmp->data)) {
	    	if (!Underwater)
/*	    	    pline("%s falls into the %s!", Monnam(mtmp), surface(u.ux,u.uy));*/
	    	    pline("%sは%sに落ちた！", Monnam(mtmp), surface(u.ux,u.uy));
	    	if (!is_swimmer(mtmp->data) && !amphibious(mtmp->data)) {
	    	    killed(mtmp);
	    	    adjalign(-1);
	    	}
	    }
	    if (mtmp->mhp > 0 && is_lava(u.ux,u.uy) &&
	    		!is_flyer(mtmp->data) && !is_floater(mtmp->data) &&
	    		!is_clinger(mtmp->data)) {
/*JP	    	pline("%s is pulled into the lava!", Monnam(mtmp));*/
	    	pline("%sは溶岩の中にひっぱられた！", Monnam(mtmp));
	    	if (!likes_lava(mtmp->data)) {
	    	    killed(mtmp);
	    	    adjalign(-1);
	    	}
	    }

	    /* Steed dismounting consists of two steps: being moved to another
	     * square, and descending to the floor.  We have functions to do
	     * each of these activities, but they're normally called
	     * individually and include an attempt to look at or pick up the
	     * objects on the floor:
	     * teleds() --> spoteffects() --> pickup()
	     * float_down() --> pickup()
	     * We use this kludge to make sure there is only one such attempt.
	     *
	     * Clearly this is not the best way to do it.  A full fix would
	     * involve having these functions not call pickup() at all, instead
	     * calling them first and calling pickup() afterwards.  But it
	     * would take a lot of work to keep this change from having any
	     * unforseen side effects (for instance, you would no longer be
	     * able to walk onto a square with a hole, and autopickup before
	     * falling into the hole).
	     */
	    /* Keep steed here, move the player to cc; teleds() clears u.utrap */
	    in_steed_dismounting = TRUE;
	    teleds(cc.x, cc.y);
	    in_steed_dismounting = FALSE;
	    if (reason != DISMOUNT_ENGULFED) /* being swallowed anyway in that case */
		vision_full_recalc = 1;

	    /* Put your steed in your trap */
	    if (save_utrap && mtmp->mhp > 0)
	    	(void) mintrap(mtmp);

	/* Couldn't... try placing the steed */
	} else if (enexto(&cc, u.ux, u.uy, mtmp->data))
	    /* Keep player here, move the steed to cc */
	    rloc_to(mtmp, cc.x, cc.y);
	    /* Player stays put */
	/* Otherwise, kill the steed */
	else {
	    killed(mtmp);
	    adjalign(-1);
	}

	/* Return the player to the floor */
	(void) float_down(0L, W_SADDLE);
	flags.botl = 1;
	if (reason != DISMOUNT_ENGULFED) (void)encumber_msg();
	return;
}


#endif /* STEED */
