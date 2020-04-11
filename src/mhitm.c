/*	SCCS Id: @(#)mhitm.c	3.1	93/05/26	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "artifact.h"
#include "edog.h"

#ifdef OVLB

static NEARDATA boolean vis, far_noise;
static NEARDATA long noisetime;
static NEARDATA struct obj *otmp;

static void FDECL(mrustm, (struct monst *, struct monst *, struct obj *));
static int FDECL(hitmm, (struct monst *,struct monst *,struct attack *));
static int FDECL(gazemm, (struct monst *,struct monst *,struct attack *));
static int FDECL(gulpmm, (struct monst *,struct monst *,struct attack *));
static int FDECL(explmm, (struct monst *,struct monst *,struct attack *));
static int FDECL(mdamagem, (struct monst *,struct monst *,struct attack *));
static void FDECL(mswingsm, (struct monst *, struct monst *, struct obj *));
static void FDECL(noises,(struct monst *,struct attack *));
static void FDECL(missmm,(struct monst *,struct monst *,struct attack *));
static int FDECL(passivemm, (struct monst *, struct monst *, BOOLEAN_P, int));

/* Needed for the special case of monsters wielding vorpal blades (rare).
 * If we use this a lot it should probably be a parameter to mdamagem()
 * instead of a global variable.
 */
static int dieroll;

static void
noises(magr, mattk)
	register struct monst *magr;
	register struct	attack *mattk;
{
	boolean farq = (distu(magr->mx, magr->my) > 15);

	if(flags.soundok && (farq != far_noise || moves-noisetime > 10)) {
		far_noise = farq;
		noisetime = moves;
/*JP		You("hear %s%s.",
			(mattk->aatyp == AT_EXPL) ? "an explosion" : "some noises",
			farq ? " in the distance" : "");*/
		You("%s%sを聞いた．",
		        farq ? "遠くで" : "",
			(mattk->aatyp == AT_EXPL) ? "爆発音" : "何かが戦う音"
			);
	}
}

static
void
missmm(magr, mdef, mattk)
	register struct monst *magr, *mdef;
	struct attack *mattk;
{
	const char *fmt;
	char buf[BUFSZ];

	if (vis) {
		if (mdef->m_ap_type) seemimic(mdef);
		if (magr->m_ap_type) seemimic(magr);
/*JP		fmt = (could_seduce(magr,mdef,mattk) && !magr->mcan) ?
			"%s pretends to be friendly to" : "%s misses";*/
		fmt = (could_seduce(magr,mdef,mattk) && !magr->mcan) ?
			"%sは%%sに友好的なふりをした" : "%sの%%sへの攻撃は外れた";
/*JP		Sprintf(buf, fmt, Monnam(magr));*/
		Sprintf(buf, fmt, Monnam(magr));
/*JP		pline("%s %s.", buf, mon_nam(mdef));*/
		pline( buf,mon_nam(mdef));
	} else  noises(magr, mattk);
}

/*
 *  fightm()  -- fight some other monster
 *
 *  Returns:
 *	0 - Monster did nothing.
 *	1 - If the monster made an attack.  The monster might have died.
 *
 *  There is an exception to the above.  If mtmp has the hero swallowed,
 *  then we report that the monster did nothing so it will continue to
 *  digest the hero.
 */
int
fightm(mtmp)		/* have monsters fight each other */
	register struct monst *mtmp;
{
	register struct monst *mon, *nmon;
	int result, has_u_swallowed;
#ifdef LINT
	nmon = 0;
#endif
	/* perhaps the monster will resist Conflict */
	if(resist(mtmp, RING_CLASS, 0, 0))
	    return(0);
#ifdef POLYSELF
	if(u.ustuck == mtmp) {
	    /* perhaps we're holding it... */
	    if(itsstuck(mtmp))
		return(0);
	}
#endif
	has_u_swallowed = (u.uswallow && (mtmp == u.ustuck));

	for(mon = fmon; mon; mon = nmon) {
	    nmon = mon->nmon;
	    if(nmon == mtmp) nmon = mtmp->nmon;
	    if(mon != mtmp) {
		if(monnear(mtmp,mon->mx,mon->my)) {
		    if(!u.uswallow && (mtmp == u.ustuck)) {
			if(!rn2(4)) {
/*JP			    pline("%s releases you!", Monnam(mtmp));*/
			    pline("%sはあなたを解放した！", Monnam(mtmp));
			    u.ustuck = 0;
			} else
			    break;
		    }

		    /* mtmp can be killed */
		    bhitpos.x = mon->mx;
		    bhitpos.y = mon->my;
		    result = mattackm(mtmp,mon);

		    if (result & MM_AGR_DIED) return 1;	/* mtmp died */
		    /*
		     *  If mtmp has the hero swallowed, lie and say there
		     *  was no attack (this allows mtmp to digest the hero).
		     */
		    if (has_u_swallowed) return 0;

		    return ((result & MM_HIT) ? 1 : 0);
		}
	    }
	}
	return 0;
}

/*
 * mattackm() -- a monster attacks another monster.
 *
 * This function returns a result bitfield:
 *	   
 *	    --------- agressor died
 *	   /  ------- defender died
 *	  /  /  ----- defender was hit
 *	 /  /  /
 *	x  x  x
 *
 *	0x4	MM_AGR_DIED
 *	0x2	MM_DEF_DIED
 *	0x1	MM_HIT
 *	0x0	MM_MISS
 *
 * Each successive attack has a lower probability of hitting.  Some rely on the
 * success of previous attacks.  ** this doen't seem to be implemented -dl **
 *
 * In the case of exploding monsters, the monster dies as well.
 */
int
mattackm(magr, mdef)
    register struct monst *magr,*mdef;
{
    int		    i,		/* loop counter */
		    tmp,	/* amour class difference */
		    strike,	/* hit this attack */
		    attk,	/* attack attempted this time */
		    struck = 0,	/* hit at least once */
		    res[NATTK];	/* results of all attacks */
    struct attack   *mattk;
    struct permonst *pa, *pd;

    if (!magr || !mdef) return(MM_MISS);		/* mike@genat */
    pa = magr->data; pd = mdef->data;
    if (!magr->mcanmove) return(MM_MISS);		/* riv05!a3 */

    /* Grid bugs cannot attack at an angle. */
    if (pa == &mons[PM_GRID_BUG] && magr->mx != mdef->mx
						&& magr->my != mdef->my)
	return(MM_MISS);

    /* Calculate the armour class differential. */
    tmp = find_mac(mdef) + magr->m_lev;
    if (mdef->mconf || !mdef->mcanmove || mdef->msleep){
	tmp += 4;
	if (mdef->msleep) mdef->msleep = 0;
    }

    /* undetect monsters become un-hidden if they are attacked */
    if (mdef->mundetected) {
	mdef->mundetected = 0;
	newsym(mdef->mx, mdef->my);
	if(canseemon(mdef))
/*JP	    pline("Suddenly, you notice %s.", a_monnam(mdef));*/
	    pline("突然あなたは%sに気がついた．", a_monnam(mdef));
    }

    /* Elves hate orcs. */
    if (is_elf(pa) && is_orc(pd)) tmp++;


    /* Set up the visibility of action */
    vis = (cansee(magr->mx,magr->my) && cansee(mdef->mx,mdef->my));

    /*	Set flag indicating monster has moved this turn.  Necessary since a
     *	monster might get an attack out of sequence (i.e. before its move) in
     *	some cases, in which case this still counts as its move for the round
     *	and it shouldn't move again.
     */
    magr->mlstmv = monstermoves;

    /* Now perform all attacks for the monster. */
    for (i = 0; i < NATTK; i++) {
	res[i] = MM_MISS;
	mattk = &(pa->mattk[i]);
	otmp = (struct obj *)0;
	attk = 1;
	switch (mattk->aatyp) {
	    case AT_WEAP:		/* "hand to hand" attacks */
#ifdef MUSE
		if (magr->weapon_check == NEED_WEAPON || !MON_WEP(magr)) {
			magr->weapon_check = NEED_HTH_WEAPON;
			if (mon_wield_item(magr) != 0) return 0;
		}
		remove_cadavers(&magr->minvent);
		possibly_unwield(magr);
		otmp = MON_WEP(magr);
#else
		otmp = select_hwep(magr);
#endif
		if (otmp) {
		    if (vis) mswingsm(magr, mdef, otmp);
		    tmp += hitval(otmp, pd);
		}
		/* fall through */
	    case AT_CLAW:
	    case AT_KICK:
	    case AT_BITE:
	    case AT_STNG:
	    case AT_TUCH:
	    case AT_BUTT:
	    case AT_TENT:
		dieroll = rnd(20 + i);
		strike = (tmp > dieroll);
		if (strike)
		    res[i] = hitmm(magr, mdef, mattk);
		else
		    missmm(magr, mdef, mattk);
		break;

	    case AT_HUGS:	/* automatic if prev two attacks succeed */
		strike = (i >= 2 && res[i-1] == MM_HIT && res[i-2] == MM_HIT);
		if (strike)
		    res[i] = hitmm(magr, mdef, mattk);

		break;

	    case AT_GAZE:
		strike = 0;	/* will not wake up a sleeper */
		res[i] = gazemm(magr, mdef, mattk);
		break;

	    case AT_EXPL:
		strike = 1;	/* automatic hit */
		res[i] = explmm(magr, mdef, mattk);
		break;

	    case AT_ENGL:
		/* Engulfing attacks are directed at the hero if
		 * possible. -dlc
		 */
		if (u.uswallow && magr == u.ustuck)
		    strike = 0;
		else {
		    if ((strike = (tmp > rnd(20+i))))
			res[i] = gulpmm(magr, mdef, mattk);
		    else
			missmm(magr, mdef, mattk);
		}
		break;

	    default:		/* no attack */
		strike = 0;
		attk = 0;
		break;
	}

	if (attk && !(res[i] & MM_AGR_DIED))
	    res[i] = passivemm(magr, mdef, strike, res[i] & MM_DEF_DIED);

	if (res[i] & MM_DEF_DIED) return res[i];

	/*
	 *  Wake up the defender.  NOTE:  this must follow the check
	 *  to see if the defender died.  We don't want to modify
	 *  unallocated monsters!
	 */
	if (strike) mdef->msleep = 0;

	if (res[i] & MM_AGR_DIED)  return res[i];
	/* return if aggressor can no longer attack */
	if (!magr->mcanmove || magr->msleep) return res[i];
	if (res[i] & MM_HIT) struck = 1;	/* at least one hit */
    }

    return(struck ? MM_HIT : MM_MISS);
}

/* Returns the result of mdamagem(). */
static int
hitmm(magr, mdef, mattk)
	register struct monst *magr,*mdef;
	struct	attack *mattk;
{
	if(vis){
		int compat;
		char buf[BUFSZ];

		if(mdef->m_ap_type) seemimic(mdef);
		if(magr->m_ap_type) seemimic(magr);
		if((compat = could_seduce(magr,mdef,mattk)) && !magr->mcan) {
/*JP			Sprintf(buf, "%s %s", Monnam(magr),
				mdef->mcansee ? "smiles at" : "talks to");*/
			Sprintf(buf, "%sは%%sに%%s%s．", Monnam(magr),
				mdef->mcansee ? "微笑みかけた" : "話しかけた");
/*JP			pline("%s %s %s.", buf, mon_nam(mdef),
				compat == 2 ?
					"engagingly" : "seductively");*/
			pline(buf, mon_nam(mdef),
				compat == 2 ?
					"魅力的に" : "誘惑的に");
		} else {
		    char magr_name[BUFSZ];

		    Strcpy(magr_name, Monnam(magr));
		    switch (mattk->aatyp) {
			case AT_BITE:
/*JP				Sprintf(buf,"%s bites", magr_name);*/
				Sprintf(buf,"%sは%%sに噛みついた", magr_name);
				break;
			case AT_STNG:
/*JP				Sprintf(buf,"%s stings", magr_name);*/
				Sprintf(buf,"%sは%%sを突きさした", magr_name);
				break;
			case AT_BUTT:
/*JP				Sprintf(buf,"%s butts", magr_name);*/
				Sprintf(buf,"%sは%%sに頭突きをくらわした", magr_name);
				break;
			case AT_TUCH:
/*JP				Sprintf(buf,"%s touches", magr_name);*/
				Sprintf(buf,"%sは%%sに触れた", magr_name);
				break;
			case AT_TENT:
/*JP				Sprintf(buf, "%s tentacles suck",*/
				Sprintf(buf, "%sの触手が%%sの体液を吸いとった",
					s_suffix(magr_name));
				break;
			case AT_HUGS:
				if (magr != u.ustuck) {
/*JP				    Sprintf(buf,"%s squeezes", magr_name);*/
				    Sprintf(buf,"%sは%%sを絞めた", magr_name);
				    break;
				}
			default:
/*JP				Sprintf(buf,"%s hits", magr_name);*/
				Sprintf(buf,"%sの%%sへの攻撃は命中した", magr_name);
		    }
		    pline(buf, mon_nam(mdef));
		}
/*JP		pline("%s %s.", buf, mon_nam(mdef));*/
	} else  noises(magr, mattk);
	return(mdamagem(magr, mdef, mattk));
}

/* Returns the same values as mdamagem(). */
static int
gazemm(magr, mdef, mattk)
	register struct monst *magr, *mdef;
	struct attack *mattk;
{
	char buf[BUFSZ];

	if(vis) {
/*JP		Sprintf(buf,"%s gazes at", Monnam(magr));
		pline("%s %s...", buf, mon_nam(mdef));*/
		Sprintf(buf,"%sは%%sをにらみつけた．．．", Monnam(magr));
		pline(buf, mon_nam(mdef));
	}

	if (!mdef->mcansee || mdef->msleep) {
/*JP	    if(vis) pline("but nothing happens.");*/
	    if(vis) pline("しかし何もおこらなかった．");
	    return(MM_MISS);
	}

	return(mdamagem(magr, mdef, mattk));
}

/* Returns the same values as mattackm(). */
static int
gulpmm(magr, mdef, mattk)
	register struct monst *magr, *mdef;
	register struct	attack *mattk;
{
	xchar	ax, ay, dx, dy;
	int	status;
	char buf[BUFSZ];

	if (mdef->data->msize >= MZ_HUGE) return MM_MISS;

	if (vis) {
/*JP		Sprintf(buf,"%s swallows", Monnam(magr));
		pline("%s %s.", buf, mon_nam(mdef));*/
		Sprintf(buf,"%sは%%sをぐっと飲みこんだ．", Monnam(magr));
		pline(buf, mon_nam(mdef));
	}

	/*
	 *  All of this maniuplation is needed to keep the display correct.
	 *  There is a flush at the next pline().
	 */
	ax = magr->mx;
	ay = magr->my;
	dx = mdef->mx;
	dy = mdef->my;
	/*
	 *  Leave the defender in the monster chain at it's current position,
	 *  but don't leave it on the screen.  Move the agressor to the def-
	 *  ender's position.
	 */
	remove_monster(ax, ay);
	place_monster(magr, dx, dy);
	newsym(ax,ay);			/* erase old position */
	newsym(dx,dy);			/* update new position */

	status = mdamagem(magr, mdef, mattk);

	if ((status & MM_AGR_DIED) && (status & MM_DEF_DIED)) {
	    ;					/* both died -- do nothing  */
	}
	else if (status & MM_DEF_DIED) {	/* defender died */
	    /*
	     *  Note:  remove_monster() was called in relmon(), wiping out
	     *  magr from level.monsters[mdef->mx][mdef->my].  We need to
	     *  put it back and display it.	-kd
	     */
	    place_monster(magr, dx, dy);
	    newsym(dx, dy);
	}
	else if (status & MM_AGR_DIED) {	/* agressor died */
	    place_monster(mdef, dx, dy);
	    newsym(dx, dy);
	}
	else {					/* both alive, put them back */
	    if (cansee(dx, dy))
/*JP		pline("%s is regurgitated!", Monnam(mdef));*/
		pline("%sは吐き戻された！", Monnam(mdef));

	    place_monster(magr, ax, ay);
	    place_monster(mdef, dx, dy);
	    newsym(ax, ay);
	    newsym(dx, dy);
	}

	return status;
}

static int
explmm(magr, mdef, mattk)
	register struct monst *magr, *mdef;
	register struct	attack *mattk;
{
	int result, was_tame;

	if(cansee(magr->mx, magr->my))
/*JP		pline("%s explodes!", Monnam(magr));*/
		pline("%sは爆発した！", Monnam(magr));
	else	noises(magr, mattk);

	was_tame = magr->mtame;
	result = mdamagem(magr, mdef, mattk);

	/* The attacker could have died . . */
	if (was_tame)
/*JP	    You("have a sad feeling for a moment, then it passes.");*/
	  {
	    if(!Hallucination)
	      You("悲しい気分におそわれたが，すぐに過ぎさった．");
	    else{
/*JP*	original joke*/
	      switch(rn2(4)){
	      case 0:
/*By Shigehiro Miyashita*/
		You("横山智佐のサイン会に参加してるような気分におそわれたが，すぐに過ぎさった．");
		break;
	      case 1:
/*By Issei Numata*/
		You("四連鎖同時消しをくらったような気分におそわれたが，すぐに過ぎさった．");
		break;
	      default:
		You("悲しい気分におそわれたが，すぐに過ぎさった．");
		break;
	      }
	    }
	  }

	/* Kill off agressor if it didn't die. */
	if (!(result & MM_AGR_DIED)) {
	    mondead(magr);
#ifdef MUSE
	    if (magr->mhp <= 0)
#endif
		result |= MM_AGR_DIED;
	}

	return result;
}

static const char psf[] =
/*JP	"have a peculiarly sad feeling for a moment, then it passes.";*/
	"異様に悲しい気分におそわれたが，すぐに過ぎさった．";

/*
 *  See comment at top of mattackm(), for return values.
 */
static int
mdamagem(magr, mdef, mattk)
	register struct monst	*magr, *mdef;
	register struct attack	*mattk;
{
	struct	permonst *pa = magr->data, *pd = mdef->data;
	int	tmp = d((int)mattk->damn,(int)mattk->damd);
	char buf[BUFSZ];

	if (pd == &mons[PM_COCKATRICE] && !resists_ston(pa) &&
	   (mattk->aatyp != AT_WEAP || !otmp) &&
	   (mattk->aatyp != AT_GAZE && mattk->aatyp != AT_EXPL) &&
#ifdef MUSE
	   (!(magr->misc_worn_check & W_ARMG))) {
#else
	   (!is_mercenary(pa) || !m_carrying(magr, LEATHER_GLOVES))) {
	   /* Note: other monsters may carry gloves, only soldiers have them */
	   /* as their "armor" and can be said to wear them */
#endif
		if (poly_when_stoned(pa)) {
		    mon_to_stone(magr);
		    return MM_HIT; /* no damage during the polymorph */
		}
/*JP		if (vis) pline("%s turns to stone!", Monnam(magr));*/
		if (vis) pline("%sは石になった！", Monnam(magr));
		else if (magr->mtame) You(psf);
		monstone(magr);
		return MM_AGR_DIED;
	}

	switch(mattk->adtyp) {
	    case AD_DGST:
/*JP		if(flags.verbose && flags.soundok) verbalize("Burrrrp!");*/
		if(flags.verbose && flags.soundok) verbalize("げっぷ！");
		tmp = mdef->mhp;
		break;
	    case AD_STUN:
		if (magr->mcan) break;
/*JP		if(vis) pline("%s staggers for a moment.", Monnam(mdef));*/
		if(vis) pline("%sは痺れた．", Monnam(mdef));
		mdef->mstun = 1;
		/* fall through */
	    case AD_WERE:
	    case AD_HEAL:
	    case AD_LEGS:
	    case AD_PHYS:
		if (mattk->aatyp == AT_KICK && thick_skinned(pd))
			tmp = 0;
		else if(mattk->aatyp == AT_WEAP) {
		    if(otmp) {
#ifdef MUSE
			if (otmp->otyp == CORPSE &&
				otmp->corpsenm == PM_COCKATRICE)
			    goto do_stone_goto_label;
#endif
			tmp += dmgval(otmp, pd);
			if (otmp->oartifact) {
			    (void)artifact_hit(magr,mdef, otmp, &tmp, dieroll);
			    if (mdef->mhp <= 0)
				return (MM_DEF_DIED |
					(grow_up(magr,mdef) ? 0 : MM_AGR_DIED));
			}
			if (tmp)
				mrustm(magr, mdef, otmp);
		    }
		}
		break;
	    case AD_FIRE:
		if (magr->mcan) {
		    tmp = 0;
		    break;
		}
/*JP		if(vis) pline("%s is on fire!", Monnam(mdef));*/
		if(vis) pline("%sは火だるまになった！", Monnam(mdef));
		tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
		tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
		if(resists_fire(pd)) {
		    if (vis)
/*JP			pline("The fire doesn't seem to burn %s!",*/
			pline("%sは炎で燃えないようだ！",
								mon_nam(mdef));
		    shieldeff(mdef->mx, mdef->my);
		    golemeffects(mdef, AD_FIRE, tmp);
		    tmp = 0;
		}
		/* only potions damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
		break;
	    case AD_COLD:
		if (magr->mcan) {
		    tmp = 0;
		    break;
		}
/*JP		if(vis) pline("%s is covered in frost!", Monnam(mdef));*/
		if(vis) pline("%sは氷で覆われた！", Monnam(mdef));
		if(resists_cold(pd)) {
		    if (vis)
/*JP			pline("The frost doesn't seem to chill %s!",*/
			pline("氷は%sを凍らすことができないようだ！",
								mon_nam(mdef));
		    shieldeff(mdef->mx, mdef->my);
		    golemeffects(mdef, AD_COLD, tmp);
		    tmp = 0;
		}
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
		break;
	    case AD_ELEC:
		if (magr->mcan) {
		    tmp = 0;
		    break;
		}
/*JP		if(vis) pline("%s gets zapped!", Monnam(mdef));*/
		if(vis) pline("%sは衝撃をくらった！", Monnam(mdef));
		tmp += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
		if(resists_elec(pd)) {
/*JP		    if (vis) pline("The zap doesn't shock %s!", mon_nam(mdef));*/
		    if (vis) pline("衝撃は%sに影響を与えない！", mon_nam(mdef));
		    shieldeff(mdef->mx, mdef->my);
		    golemeffects(mdef, AD_ELEC, tmp);
		    tmp = 0;
		}
		/* only rings damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
		break;
	    case AD_ACID:
		if (magr->mcan) {
		    tmp = 0;
		    break;
		}
		if(resists_acid(pd)) {
		    if (vis)
/*JP			pline("%s is covered in acid, but it seems harmless.",*/
			pline("%sは酸につつまれた．しかし傷つかない．",
							Monnam(mdef));
		    tmp = 0;
		} else if (vis) {
/*JP		    pline("%s is covered in acid!", Monnam(mdef));
		    pline("It burns %s!", mon_nam(mdef));*/
		    pline("%sは酸につつまれた！", Monnam(mdef));
		    pline("%sを燃やした！", mon_nam(mdef));
		}
		break;
	    case AD_RUST:
		if (!magr->mcan && pd == &mons[PM_IRON_GOLEM]) {
/*JP			if (vis) pline("%s falls to pieces!", Monnam(mdef));*/
			if (vis) pline("%sはバラバラになった！", Monnam(mdef));
			else if(mdef->mtame)
/*JP			     pline("May %s rust in peace.", mon_nam(mdef));*/
			     pline("%sはバラバラになって錆びた．", mon_nam(mdef));
			mondied(mdef);
#ifdef MUSE
			if (mdef->mhp > 0) return 0;
#endif
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		tmp = 0;
		break;
	    case AD_DCAY:
		if (!magr->mcan && (pd == &mons[PM_WOOD_GOLEM] ||
		    pd == &mons[PM_LEATHER_GOLEM])) {
/*JP			if (vis) pline("%s falls to pieces!", Monnam(mdef));*/
			if (vis) pline("%sはバラバラになった！", Monnam(mdef));
			else if(mdef->mtame)
/*JP			     pline("May %s rot in peace.", mon_nam(mdef));*/
			     pline("%sはバラバラになって腐った．", mon_nam(mdef));
			mondied(mdef);
#ifdef MUSE
			if (mdef->mhp > 0) return 0;
#endif
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		tmp = 0;
		break;
	    case AD_STON:
#ifdef MUSE
do_stone_goto_label:
#endif
		if(poly_when_stoned(pd)) {
		    mon_to_stone(mdef);
		    tmp = 0;
		    break;
		}
		if(!resists_ston(pd)) {
/*JP			if(vis) pline("%s turns to stone!", Monnam(mdef));*/
			if(vis) pline("%sは石になった！", Monnam(mdef));
			else if(mdef->mtame) You(psf);
			monstone(mdef);
#ifdef MUSE
			if (mdef->mhp > 0) return 0;
#endif
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		tmp = (mattk->adtyp == AD_STON ? 0 : 1);
		break;
	    case AD_TLPT:
		if(!magr->mcan && tmp < mdef->mhp) {
		    if (!tele_restrict(mdef)) rloc(mdef);
		    if(vis && !cansee(mdef->mx, mdef->my))
/*JP			pline("%s suddenly disappears!", Monnam(mdef));*/
			pline("%sは突然消えた！", Monnam(mdef));
		}
		break;
	    case AD_SLEE:
		if(!resists_sleep(pd) && !magr->mcan && !mdef->msleep
							&& mdef->mcanmove) {
		    if (vis) {
			Strcpy(buf, Monnam(mdef));
/*JP			pline("%s is put to sleep by %s.", buf, mon_nam(magr));*/
			pline("%sは%sによって眠らせれた．", buf, mon_nam(magr));
		    }
		    mdef->mcanmove = 0;
		    mdef->mfrozen = rnd(10);
		}
		break;
	    case AD_PLYS:
		if(!magr->mcan && mdef->mcanmove) {
		    if (vis) {
			Strcpy(buf, Monnam(mdef));
/*JP			pline("%s is frozen by %s.", buf, mon_nam(magr));*/
			pline("%sは%sによって動けなくなった．", buf, mon_nam(magr));
		    }
		    mdef->mcanmove = 0;
		    mdef->mfrozen = rnd(10);
		}
		break;
	    case AD_SLOW:
		if(!magr->mcan && vis && mdef->mspeed != MSLOW) {
/*JP		    if (vis) pline("%s slows down.", Monnam(mdef));*/
		    if (vis) pline("%sは動作がのろくなった．", Monnam(mdef));
		    if (mdef->mspeed == MFAST) mdef->mspeed = 0;
		    else mdef->mspeed = MSLOW;
		}
		break;
	    case AD_CONF:
		/* Since confusing another monster doesn't have a real time
		 * limit, setting spec_used would not really be right (though
		 * we still should check for it).
		 */
		if (!magr->mcan && vis && !mdef->mconf && !magr->mspec_used) {
/*JP		    pline("%s looks confused.", Monnam(mdef));*/
		    pline("%sは混乱しているようだ．", Monnam(mdef));
		    mdef->mconf = 1;
		}
		break;
	    case AD_BLND:
		if (!magr->mcan && haseyes(pd)) {
		    register unsigned rnd_tmp;

		    if (vis && mdef->mcansee)
/*JP			pline("%s is blinded.", Monnam(mdef));*/
			pline("%sは目が見えなくなった．", Monnam(mdef));
		    rnd_tmp = d((int)mattk->damn, (int)mattk->damd);
		    if ((rnd_tmp += mdef->mblinded) > 127) rnd_tmp = 127;
		    mdef->mblinded = rnd_tmp;
		    mdef->mcansee = 0;
		}
		tmp = 0;
		break;
	    case AD_CURS:
		if (!night() && (pa == &mons[PM_GREMLIN])) break;
		if (!magr->mcan && !rn2(10)) {
		    if (is_were(pd) && pd->mlet != S_HUMAN)
			were_change(mdef);
		    if (pd == &mons[PM_CLAY_GOLEM]) {
			    if (vis) {
/*JP				pline("Some writing vanishes from %s head!",*/
				pline("何かの文字が%sの頭から消えた！",
				    s_suffix(mon_nam(mdef)));
/*JP				pline("%s dies!", Monnam(mdef));*/
				pline("%sは死んだ！", Monnam(mdef));
			    }
			    else if (mdef->mtame)
/*JP	You("have a strangely sad feeling for a moment, then it passes.");*/
	You("とても悲しい気分におそわれたが，すぐに過ぎさった．");
			    mondied(mdef);
#ifdef MUSE
			    if (mdef->mhp > 0) return 0;
#endif
			    return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		      }
		    mdef->mcan = 1;
		    if (flags.soundok) {
/*JP			    if (!vis) You("hear laughter.");
			    else pline("%s chuckles.", Monnam(magr));*/
			    if (!vis) You("笑い声を聞いた．");
			    else pline("%sはくすくす笑った．", Monnam(magr));
		    }
		}
		break;
	    case AD_SGLD:
		tmp = 0;
		if (magr->mcan || !mdef->mgold) break;
		/* technically incorrect; no check for stealing gold from
		 * between mdef's feet...
		 */
		magr->mgold += mdef->mgold;
		mdef->mgold = 0;
		if (vis) {
			Strcpy(buf, Monnam(magr));
/*JP			pline("%s steals some gold from %s.", buf,*/
			pline("%sは%sから金を奪いとった．", buf,
								mon_nam(mdef));
		}
		break;
	    case AD_DRLI:
		if(rn2(2) && !resists_drli(pd)) {
			tmp = d(2,6);
			if (vis)
/*JP			    pline("%s suddenly seems weaker!", Monnam(mdef));*/
			    pline("%sは突然弱くなったように見えた！", Monnam(mdef));
			mdef->mhpmax -= tmp;
			if (mdef->m_lev == 0)
				tmp = mdef->mhp;
			else mdef->m_lev--;
			/* Automatic kill if drained past level 0 */
		}
		break;
#ifdef SEDUCE
	    case AD_SSEX:
#endif
	    case AD_SITM:	/* for now these are the same */
	    case AD_SEDU:
		if (!magr->mcan && mdef->minvent) {
		   	otmp = mdef->minvent;
			mdef->minvent = otmp->nobj;
			otmp->nobj = magr->minvent;
			magr->minvent = otmp;
			if (vis) {
				Strcpy(buf, Monnam(magr));
/*JP				pline("%s steals %s from %s!", buf,*/
				pline("%sは%sを%sから盗んだ！", buf,
						doname(otmp), mon_nam(mdef));
			}
#ifdef MUSE
			possibly_unwield(mdef);
			if (otmp->owornmask) {
				mdef->misc_worn_check &= ~otmp->owornmask;
				otmp->owornmask = 0;
			}
			mselftouch(mdef, (const char *)0, FALSE);
			if (mdef->mhp <= 0)
				return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
#endif
		}
		tmp = 0;
		break;
	    case AD_DRST:
	    case AD_DRDX:
	    case AD_DRCO:
		if (!magr->mcan && !rn2(8)) {
		    if (vis)
/*JP			pline("%s %s was poisoned!", s_suffix(Monnam(magr)),
				mattk->aatyp==AT_BITE ? "bite" : "sting");*/
			pline("%sの%sは毒されている！", s_suffix(Monnam(magr)),
				mattk->aatyp==AT_BITE ? "歯" : "針");
		    if (resists_poison(pd)) {
			if (vis)
/*JP			    pline("The poison doesn't seem to affect %s.",*/
			    pline("毒は%sに影響を与えない．",
				mon_nam(mdef));
		    } else {
			if (rn2(10)) tmp += rn1(10,6);
			else {
/*JP			    if (vis) pline("The poison was deadly...");*/
			    if (vis) pline("毒がまわってきた．．．");
			    tmp = mdef->mhp;
			}
		    }
		}
		break;
	    case AD_DRIN:
		if (!has_head(pd)) {
/*JP		    if (vis) pline("%s doesn't seem harmed.", Monnam(mdef));*/
		    if (vis) pline("%sは傷ついたようには見えない．", Monnam(mdef));
		    tmp = 0;
		    break;
		}
#ifdef MUSE
		if ((mdef->misc_worn_check & W_ARMH) && rn2(8)) {
		    if (vis) {
			Strcpy(buf, s_suffix(Monnam(mdef)));
/*JP			pline("%s helmet blocks %s attack to his head.",*/
			pline("%sの兜は%sの頭への攻撃を防いだ．",
				buf, s_suffix(mon_nam(magr)));
		    }
		    break;
		}
#endif
/*JP		if (vis) pline("%s brain is eaten!", s_suffix(Monnam(mdef)));*/
		if (vis) pline("%sの脳は食べられた！", s_suffix(Monnam(mdef)));
		if (mindless(pd)) {
/*JP		    if (vis) pline("%s doesn't notice.", Monnam(mdef));*/
		    if (vis) pline("%sは気がつかない．", Monnam(mdef));
		    break;
		}
		tmp += rnd(10); /* fakery, since monsters lack INT scores */
		if (magr->mtame && !magr->isminion) {
		    EDOG(magr)->hungrytime += rnd(60);
		    magr->mconf = 0;
		}
		if (tmp >= mdef->mhp && vis)
/*JP		    pline("%s last thought fades away...", */
		    pline("%sの最後の思いがよこぎる．．．",
			          s_suffix(Monnam(mdef)));
		break;
	    case AD_STCK:
	    case AD_WRAP: /* monsters cannot grab one another, it's too hard */
		break;
	    default:	tmp = 0;
			break;
	}
	if(!tmp) return(MM_MISS);

	if((mdef->mhp -= tmp) < 1) {
	    if (m_at(mdef->mx, mdef->my) == magr) {  /* see gulpmm() */
		remove_monster(mdef->mx, mdef->my);
		place_monster(mdef, mdef->mx, mdef->my);
	    }
	    monkilled(mdef, "", (int)mattk->adtyp);
	    if (mdef->mhp > 0) return 0; /* mdef lifesaved */
	    return (MM_DEF_DIED | (grow_up(magr,mdef) ? 0 : MM_AGR_DIED));
	}
	return(MM_HIT);
}

#endif /* OVLB */


#ifdef OVL0

int
noattacks(ptr)			/* returns 1 if monster doesn't attack */
	struct	permonst *ptr;
{
	int i;

	for(i = 0; i < NATTK; i++)
		if(ptr->mattk[i].aatyp) return(0);

	return(1);
}

#endif /* OVL0 */
#ifdef OVLB

static void
mrustm(magr, mdef, obj)
register struct monst *magr, *mdef;
register struct obj *obj;
{
	if (!magr || !mdef || !obj) return; /* just in case */
	if (mdef->data == &mons[PM_RUST_MONSTER] && !mdef->mcan &&
	    is_rustprone(obj) && obj->oeroded < MAX_ERODE) {
		if (obj->greased || obj->oerodeproof || (obj->blessed && rn2(3))) {
		    if (cansee(mdef->mx, mdef->my) && flags.verbose)
/*JP			pline("%s weapon is not affected.", */
			pline("%sの武器は影響を受けない．", 
			                 s_suffix(Monnam(magr)));
		    if (obj->greased && !rn2(2)) obj->greased = 0;
		} else {
		    if (cansee(mdef->mx, mdef->my)) {
/*JP			pline("%s %s%s!", s_suffix(Monnam(magr)),
			      aobjnam(obj, "rust"),
			      obj->oeroded ? " further" : "");*/
			pline("%sの%sは%s錆びた！", s_suffix(Monnam(magr)),
			      xname(obj),
			      obj->oeroded ? "さらに" : "");
		    }
		    obj->oeroded++;
		}
	}
}

static void
mswingsm(magr, mdef, otemp)
register struct monst *magr, *mdef;
register struct obj *otemp;
{
	char buf[BUFSZ];
	Strcpy(buf, mon_nam(mdef));
	if (!flags.verbose || Blind) return;
/*JP	pline("%s %s %s %s at %s.", Monnam(magr),
	      ((otemp->otyp >= SPEAR && otemp->otyp <= LANCE) ||
	       (otemp->otyp >= PARTISAN && otemp->otyp <= SPETUM) ||
	       otemp->otyp == TRIDENT) ? "thrusts" : "swings",
	      his[pronoun_gender(magr)], xname(otemp), buf);*/
	pline("%sは%s%s%sを%s．", Monnam(magr),
	      xname(otemp), 
	      ((otemp->otyp >= SPEAR && otemp->otyp <= LANCE) ||
	       (otemp->otyp >= PARTISAN && otemp->otyp <= SPETUM) ||
	       otemp->otyp == TRIDENT) ? "で" : "を振りまわし", buf,
	      ((otemp->otyp >= SPEAR && otemp->otyp <= LANCE) ||
	       (otemp->otyp >= PARTISAN && otemp->otyp <= SPETUM) ||
	       otemp->otyp == TRIDENT) ? "突いた" : "攻撃した" );
}

/*
 * Passive responses by defenders.  Does not replicate responses already
 * handled above.  Returns same values as mattackm.
 */
static int
passivemm(magr,mdef,mhit,mdead)
register struct monst *magr, *mdef;
boolean mhit;
int mdead;
{
	register struct permonst *mddat = mdef->data;
	register struct permonst *madat = magr->data;
	char buf[BUFSZ];
	int i, tmp;

	for(i = 0; ; i++) {
	    if(i >= NATTK) return (mdead | mhit); /* no passive attacks */
	    if(mddat->mattk[i].aatyp == AT_NONE) break;
	}
	if (mddat->mattk[i].damn)
	    tmp = d((int)mddat->mattk[i].damn, 
                                    (int)mddat->mattk[i].damd);
	else if(mddat->mattk[i].damd)
	    tmp = d((int)mddat->mlevel+1, (int)mddat->mattk[i].damd);
	else
	    tmp = 0;

	/* These affect the enemy even if defender killed */
	switch(mddat->mattk[i].adtyp) {
	    case AD_ACID:
		if (mhit && !rn2(2)) {
		    Strcpy(buf, Monnam(magr));
		    if(canseemon(magr))
/*JP			pline("%s is splashed by %s acid!",*/
			pline("%sは%sの酸を浴びた！",
			      buf, s_suffix(mon_nam(mdef)));
		    if(resists_acid(madat)) {
			if(canseemon(magr))
/*JP			    pline("%s is not affected.", Monnam(magr));*/
			    pline("%sは影響をうけない．", Monnam(magr));
			tmp = 0;
		    }
		} else tmp = 0;
		goto assess_dmg;
	    default:
		break;
	}
	if (mdead || mdef->mcan) return (mdead|mhit);

	/* These affect the enemy only if defender is still alive */
	if (rn2(3)) switch(mddat->mattk[i].adtyp) {
	    case AD_PLYS: /* Floating eye */
		if (tmp > 127) tmp = 127;
		if (mddat == &mons[PM_FLOATING_EYE]) {
		    if (!rn2(4)) tmp = 127;
		    if (magr->mcansee && haseyes(madat) && mdef->mcansee &&
			(perceives(madat) || !mdef->minvis)) {
#ifdef MUSE
/*JP			Sprintf(buf, "%s gaze is reflected by %%s %%s.",*/
			Sprintf(buf, "%sのにらみは%%sによって%%s．",
				s_suffix(mon_nam(mdef)));
			if (mon_reflects(magr, buf))
				return(mdead|mhit);
#endif
			Strcpy(buf, Monnam(magr));
			if(canseemon(magr))
/*JP			    pline("%s is frozen by %s gaze!",*/
			    pline("%sは%sのにらみで動けなくなった！",
				  buf, s_suffix(mon_nam(mdef)));
			magr->mcanmove = 0;
			magr->mfrozen = tmp;
			return (mdead|mhit);
		    }
		} else { /* gelatinous cube */
		    Strcpy(buf, Monnam(magr));
		    if(canseemon(magr))
/*JP			pline("%s is frozen by %s.", buf, mon_nam(mdef));*/
			pline("%sは%sによって動けなくなった．", buf, mon_nam(mdef));
		    magr->mcanmove = 0;
		    magr->mfrozen = tmp;
		    return (mdead|mhit);
		}
		return 1;
	    case AD_COLD:
		if (resists_cold(madat)) {
		    if (canseemon(magr)) {
/*JP			pline("%s is mildly chilly.", Monnam(magr));*/
			pline("%sは冷えた．", Monnam(magr));
			golemeffects(magr, AD_COLD, tmp);
			tmp = 0;
			break;
		    }
		}
		if(canseemon(magr))
/*JP		    pline("%s is suddenly very cold!", Monnam(magr));*/
		    pline("%sは突然凍りづけになった！", Monnam(magr));
		mdef->mhp += tmp / 2;
		if (mdef->mhpmax < mdef->mhp) mdef->mhpmax = mdef->mhp;
		if (mdef->mhpmax > ((int) (mdef->m_lev+1) * 8)) {
		    register struct monst *mtmp;

		    if ((mtmp = clone_mon(mdef)) != 0) {
			mtmp->mhpmax = mdef->mhpmax /= 2;
			if(canseemon(magr)) {
			    Strcpy(buf, Monnam(mdef));
/*JP			    pline("%s multiplies from %s heat!",*/
			    pline("%sは%sの熱で分裂した！",
				    buf, s_suffix(mon_nam(magr)));
			}
		    }
		}
		break;
	    case AD_STUN:
		if (!magr->mstun) {
		    magr->mstun = 1;
		    if (canseemon(magr))
/*JP			pline("%s staggers...", Monnam(magr));*/
			pline("%sはくらくらした．．．", Monnam(magr));
		}
		tmp = 0;
		break;
	    case AD_FIRE:
		if (resists_fire(madat)) {
		    if (canseemon(magr)) {
/*JP			pline("%s is mildly warmed.", Monnam(magr));*/
			pline("%sは暖かくなった．", Monnam(magr));
			golemeffects(magr, AD_FIRE, tmp);
			tmp = 0;
			break;
		    }
		}
		if(canseemon(magr))
/*JP		    pline("%s is suddenly very hot!", Monnam(magr));*/
		    pline("%sは突然熱くなった！", Monnam(magr));
		break;
	    case AD_ELEC:
		if (resists_elec(madat)) {
		    if (canseemon(magr)) {
/*JP			pline("%s is mildly tingled.", Monnam(magr));*/
			pline("%sはピリピリする．", Monnam(magr));
			golemeffects(magr, AD_ELEC, tmp);
			tmp = 0;
			break;
		    }
		}
		if(canseemon(magr))
/*JP		    pline("%s is jolted with electricity!", Monnam(magr));*/
		    pline("%sは電気ショックをうけた！", Monnam(magr));
		break;
	    default: tmp = 0;
		break;
	}
	else tmp = 0;

    assess_dmg:
	if((magr->mhp -= tmp) <= 0) {
		monkilled(magr, "", (int)mddat->mattk[i].adtyp);
		return (mdead | mhit | MM_AGR_DIED);
	}
	return (mdead | mhit);
}

#endif /* OVLB */

/*mhitm.c*/
