/*	SCCS Id: @(#)mhitm.c	3.2	96/05/25	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
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

static const char brief_feeling[] =
/*JP	"have a %s feeling for a moment, then it passes.";*/
	"%s気持におそわれたが，すぐに過ぎさった．";

static char *FDECL(mon_nam_too, (char *,struct monst *,struct monst *));
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

/* returns mon_nam(mon) relative to other_mon; normal name unless they're
   the same, in which case the reference is to {him|her|it} self */
static char *
mon_nam_too(outbuf, mon, other_mon)
char *outbuf;
struct monst *mon, *other_mon;
{
	Strcpy(outbuf, mon_nam(mon));
	if (mon == other_mon)
	    Strcpy(outbuf, "自分自身");
/*JP	    switch (pronoun_gender(mon)) {
	    case 0:	Strcpy(outbuf, "himself");  break;
	    case 1:	Strcpy(outbuf, "herself");  break;
	    default:	Strcpy(outbuf, "itself"); break;
	    }*/
	return outbuf;
}

static void
noises(magr, mattk)
	register struct monst *magr;
	register struct	attack *mattk;
{
	boolean farq = (distu(magr->mx, magr->my) > 15);

	if(flags.soundok && (farq != far_noise || moves-noisetime > 10)) {
		far_noise = farq;
		noisetime = moves;
/*JP		You_hear("%s%s.",
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
	char buf[BUFSZ], mdef_name[BUFSZ];

	if (vis) {
		if (mdef->m_ap_type) seemimic(mdef);
		if (magr->m_ap_type) seemimic(magr);
		fmt = (could_seduce(magr,mdef,mattk) && !magr->mcan) ?
/*JP
			"%s pretends to be friendly to" : "%s misses";
*/
			"%sは%%sに友好的なふりをした．" : "%sの%%sへの攻撃は外れた．";
		Sprintf(buf, fmt, Monnam(magr));
/*JP
		pline("%s %s.", buf, mon_nam_too(mdef_name, mdef, magr));
*/
		pline(buf, mon_nam_too(mdef_name, mdef, magr));
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

	if(u.ustuck == mtmp) {
	    /* perhaps we're holding it... */
	    if(itsstuck(mtmp))
		return(0);
	}
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
    if (!magr->mcanmove) return(MM_MISS);		/* riv05!a3 */
    pa = magr->data;  pd = mdef->data;

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
	if(canseemon(mdef) && !sensemon(mdef))
/*JP	    pline("Suddenly, you notice %s.", a_monnam(mdef));*/
	    pline("あなたは%sに気がついた．", a_monnam(mdef));
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
		if (magr->weapon_check == NEED_WEAPON || !MON_WEP(magr)) {
			magr->weapon_check = NEED_HTH_WEAPON;
			if (mon_wield_item(magr) != 0) return 0;
		}
		possibly_unwield(magr);
		otmp = MON_WEP(magr);

		if (otmp) {
		    if (vis) mswingsm(magr, mdef, otmp);
		    tmp += hitval(otmp, mdef);
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
		char buf[BUFSZ], mdef_name[BUFSZ];

		if(mdef->m_ap_type) seemimic(mdef);
		if(magr->m_ap_type) seemimic(magr);
		if((compat = could_seduce(magr,mdef,mattk)) && !magr->mcan) {
#if 0 /*JP*/
			Sprintf(buf, "%s %s", Monnam(magr),
				mdef->mcansee ? "smiles at" : "talks to");
			pline("%s %s %s.", buf, mon_nam(mdef),
				compat == 2 ?
					"engagingly" : "seductively");
#endif
			Sprintf(buf, "%sは%%sに%%s%s．", Monnam(magr),
				mdef->mcansee ? "微笑みかけた" : "話しかけた");
			pline(buf, mon_nam(mdef),
				compat == 2 ?
					"魅力的に" : "誘惑的に");
		} else {
		    char magr_name[BUFSZ];

		    Strcpy(magr_name, Monnam(magr));
		    switch (mattk->aatyp) {
			case AT_BITE:
/*JP				Sprintf(buf,"%s bites", magr_name);*/
				Sprintf(buf,"%sは%%sに噛みついた．", magr_name);
				break;
			case AT_STNG:
/*JP				Sprintf(buf,"%s stings", magr_name);*/
				Sprintf(buf,"%sは%%sを突きさした．", magr_name);
				break;
			case AT_BUTT:
/*JP				Sprintf(buf,"%s butts", magr_name);*/
				Sprintf(buf,"%sは%%sに頭突きをくらわした．", magr_name);
				break;
			case AT_TUCH:
/*JP				Sprintf(buf,"%s touches", magr_name);*/
				Sprintf(buf,"%sは%%sに触れた．", magr_name);
				break;
			case AT_TENT:
/*JP				Sprintf(buf, "%s tentacles suck",*/
				Sprintf(buf, "%sの触手が%%sの体液を吸いとった．",
					s_suffix(magr_name));
				break;
			case AT_HUGS:
				if (magr != u.ustuck) {
/*JP				    Sprintf(buf,"%s squeezes", magr_name);*/
				    Sprintf(buf,"%sは%%sを絞めた．", magr_name);
				    break;
				}
			default:
/*JP				Sprintf(buf,"%s hits", magr_name);*/
				Sprintf(buf,"%sの%%sへの攻撃は命中した．", magr_name);
		    }
		    pline(buf, mon_nam_too(mdef_name, mdef, magr));
		}
/*JP
		pline("%s %s.", buf, mon_nam_too(mdef_name, mdef, magr));
*/
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
	struct obj *obj;

	if (mdef->data->msize >= MZ_HUGE) return MM_MISS;

	if (vis) {
/*JP		Sprintf(buf,"%s swallows", Monnam(magr));
		pline("%s %s.", buf, mon_nam(mdef));*/
		Sprintf(buf,"%sは%%sをぐっと飲みこんだ．", Monnam(magr));
		pline(buf, mon_nam(mdef));
	}
	for (obj = mdef->minvent; obj; obj = obj->nobj)
	    (void) snuff_lit(obj);

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
	int result;

	if(cansee(magr->mx, magr->my))
/*JP		pline("%s explodes!", Monnam(magr));*/
		pline("%sは爆発した！", Monnam(magr));
	else	noises(magr, mattk);

	result = mdamagem(magr, mdef, mattk);

	/* Kill off agressor if it didn't die. */
	if (!(result & MM_AGR_DIED)) {
	    mondead(magr);
	    if (magr->mhp > 0) return result;	/* life saved */
	    result |= MM_AGR_DIED;
	}
	if (magr->mtame){	/* give this one even if it was visible */
/*JP	    You(brief_feeling, "melancholy");*/
	  if(!Hallucination)
	    You(brief_feeling, "もの悲しい");
	  else{
	    switch(rn2(3)){
	    case 0:
/*By Shigehiro Miyashita*/
	      You(brief_feeling, 
		  rn2(2) ? "横山智佐のサイン会に参加してるような" :
		  "西原久美子のサイン会に参加しているような");
	      break;
	    default:
/*By Issei Numata*/
	      You(brief_feeling, "十三連鎖をくらったような");
	      break;
	    }
	  }
	}

	return result;
}

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

	if (pd == &mons[PM_COCKATRICE] && !resists_ston(magr) &&
	   (mattk->aatyp != AT_WEAP || !otmp) &&
	   (mattk->aatyp != AT_GAZE && mattk->aatyp != AT_EXPL) &&
	   !(magr->misc_worn_check & W_ARMG)) {
		if (poly_when_stoned(pa)) {
		    mon_to_stone(magr);
		    return MM_HIT; /* no damage during the polymorph */
		}
/*JP		if (vis) pline("%s turns to stone!", Monnam(magr));*/
		if (vis) pline("%sは石になった！", Monnam(magr));
		monstone(magr);
		if (magr->mhp > 0) return 0;
		else if (magr->mtame && !vis)
/*JP		    You(brief_feeling, "peculiarly sad");*/
		    You(brief_feeling, "とても悲しい");
		return MM_AGR_DIED;
	}

	switch(mattk->adtyp) {
	    case AD_DGST:
		/* eating a Rider or its corpse is fatal */
		if (is_rider(mdef->data)) {
		    if (vis)
/*JP			pline("%s %s!", Monnam(magr),
			      mdef->data == &mons[PM_FAMINE] ?
				"belches feebly, shrivels up and dies" :
			      mdef->data == &mons[PM_PESTILENCE] ?
				"coughs spasmodically and collapses" :
				"vomits violently and drops dead");*/
			pline("%s%s！", Monnam(magr),
			      mdef->data == &mons[PM_FAMINE] ?
				"弱々しく吐きもどしたかと思うと，体がしぼみ死んでしまった" :
			      mdef->data == &mons[PM_PESTILENCE] ?
				"痙攣したようにせきこみ倒れた" :
				"激しく嘔吐し死んだ");
		    mondied(magr);
		    if (magr->mhp > 0) return 0;	/* lifesaved */
		    else if (magr->mtame && !vis)
/*JP			You(brief_feeling, "queasy");*/
			You(brief_feeling, "不安な");
		    return MM_AGR_DIED;
		}
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
			if (otmp->otyp == CORPSE &&
				otmp->corpsenm == PM_COCKATRICE)
			    goto do_stone_goto_label;
			tmp += dmgval(otmp, mdef);
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
		if (vis)
/*JP		    pline("%s is %s!", Monnam(mdef),
			  mattk->aatyp == AT_HUGS ?
				"being roasted" : "on fire");*/
		    pline("%sは%sになった！", Monnam(mdef),
			  mattk->aatyp == AT_HUGS ?
				"丸焼け" : "火だるま");
		tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
		tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
		if (resists_fire(mdef)) {
		    if (vis)
/*JP			pline_The("fire doesn't seem to burn %s!",*/
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
/*JP		if (vis) pline("%s is covered in frost!", Monnam(mdef));*/
		if (vis) pline("%sは氷で覆われた！", Monnam(mdef));
		if (resists_cold(mdef)) {
		    if (vis)
/*JP			pline_The("frost doesn't seem to chill %s!",*/
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
/*JP		if (vis) pline("%s gets zapped!", Monnam(mdef));*/
		if (vis) pline("%sは衝撃をくらった！", Monnam(mdef));
		tmp += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
		if (resists_elec(mdef)) {
/*JP		    if (vis) pline_The("zap doesn't shock %s!", mon_nam(mdef));*/
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
		if (resists_acid(mdef)) {
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
			mondied(mdef);
			if (mdef->mhp > 0) return 0;
			else if (mdef->mtame && !vis)
			    pline("May %s rust in peace.", mon_nam(mdef));
/*JP			    pline("May %s rust in peace.", mon_nam(mdef));*/
			    pline("%sはバラバラになって錆びた．", mon_nam(mdef));
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
			mondied(mdef);
			if (mdef->mhp > 0) return 0;
			else if (mdef->mtame && !vis)
/*JP		            pline("May %s rot in peace.", mon_nam(mdef));*/
			    pline("%sはバラバラになって腐った．", mon_nam(mdef));
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		tmp = 0;
		break;
	    case AD_STON:
do_stone_goto_label:
		/* may die from the acid if it eats a stone-curing corpse */
		if (munstone(mdef, FALSE)) goto label2;
		if (poly_when_stoned(pd)) {
			mon_to_stone(mdef);
			tmp = 0;
			break;
		}
		if (!resists_ston(mdef)) {
/*JP			if (vis) pline("%s turns to stone!", Monnam(mdef));*/
			if (vis) pline("%sは石になった！", Monnam(mdef));
			monstone(mdef);
label2:			if (mdef->mhp > 0) return 0;
			else if (mdef->mtame && !vis)
			    You(brief_feeling, "もの悲しい");
			return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		tmp = (mattk->adtyp == AD_STON ? 0 : 1);
		break;
	    case AD_TLPT:
		if (!magr->mcan && tmp < mdef->mhp && !tele_restrict(mdef)) {
		    char mdef_Monnam[BUFSZ];
		    /* save the name before monster teleports, otherwise
		       we'll get "it" in the suddenly disappears message */
		    if (vis) Strcpy(mdef_Monnam, Monnam(mdef));
		    rloc(mdef);
		    if (vis && !cansee(mdef->mx, mdef->my))
/*JP			pline("%s suddenly disappears!", mdef_Monnam);*/
			pline("%sは突然消えた！", Monnam(mdef));
		}
		break;
	    case AD_SLEE:
		if (!magr->mcan && !mdef->msleep &&
			sleep_monst(mdef, rnd(10), -1)) {
		    if (vis) {
			Strcpy(buf, Monnam(mdef));
/*JP			pline("%s is put to sleep by %s.", buf, mon_nam(magr));*/
			pline("%sは%sによって眠らされた．", buf, mon_nam(magr));
		    }
		    slept_monst(mdef);
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
		if (!magr->mcan && !mdef->mconf && !magr->mspec_used) {
/*JP		    if (vis) pline("%s looks confused.", Monnam(mdef));*/
 		    if (vis) pline("%sは混乱しているように見える．", Monnam(mdef));
		    mdef->mconf = 1;
		}
		break;
	    case AD_BLND:
		if (!magr->mcan && !resists_blnd(mdef)) {
		    register unsigned rnd_tmp;

		    if (vis)
/*JP			pline("%s is blinded.", Monnam(mdef));*/
			pline("%sは目が見えなくなった．", Monnam(mdef));
		    rnd_tmp = d((int)mattk->damn, (int)mattk->damd);
		    if ((rnd_tmp += mdef->mblinded) > 127) rnd_tmp = 127;
		    mdef->mblinded = rnd_tmp;
		    mdef->mcansee = 0;
		}
		tmp = 0;
		break;
	    case AD_HALU:
		if (!magr->mcan && haseyes(pd) && mdef->mcansee) {
/*JP		    if (vis) pline("%s looks %sconfused.",
				    Monnam(mdef), mdef->mconf ? "more " : "");*/
		    if (vis) pline("%sは%s混乱しているように見える．",
				    Monnam(mdef), mdef->mconf ? "ますます" : "");
		    mdef->mconf = 1;
		}
		tmp = 0;
		break;
	    case AD_CURS:
		if (!night() && (pa == &mons[PM_GREMLIN])) break;
		if (!magr->mcan && !rn2(10)) {
		    mdef->mcan = 1;	/* cancelled regardless of lifesave */
		    if (is_were(pd) && pd->mlet != S_HUMAN)
			were_change(mdef);
		    if (pd == &mons[PM_CLAY_GOLEM]) {
			    if (vis) {
/*JP				pline("Some writing vanishes from %s head!",*/
				pline("何かの文字が%sの頭から消えた！",
				    s_suffix(mon_nam(mdef)));
/*JP				pline("%s is destroyed!", Monnam(mdef));*/
				pline("%sは破壊された！", Monnam(mdef));
			    }
			    mondied(mdef);
			    if (mdef->mhp > 0) return 0;
			    else if (mdef->mtame && !vis)
/*JP				You(brief_feeling, "strangely sad");*/
				You(brief_feeling, "妙に悲しい");
			    return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		    }
		    if (flags.soundok) {
/*JP			    if (!vis) You_hear("laughter.");
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
		if (rn2(2) && !resists_drli(mdef)) {
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
			char onambuf[BUFSZ];

			otmp = mdef->minvent;
			obj_extract_self(otmp);
			if (otmp->owornmask) {
				mdef->misc_worn_check &= ~otmp->owornmask;
				otmp->owornmask = 0L;
				update_mon_intrinsics(mdef, otmp, FALSE);
			}
			/* add_to_minv() might free otmp [if it merges] */
			if (vis)
				Strcpy(onambuf, doname(otmp));
			add_to_minv(magr, otmp);
			if (vis) {
				Strcpy(buf, Monnam(magr));
/*JP				pline("%s steals %s from %s!", buf,*/
				pline("%sは%sを%sから盗んだ！", buf,
				      onambuf, mon_nam(mdef));
			}
			possibly_unwield(mdef);
			mselftouch(mdef, (const char *)0, FALSE);
			if (mdef->mhp <= 0)
				return (MM_DEF_DIED | (grow_up(magr,mdef) ?
							0 : MM_AGR_DIED));
		}
		tmp = 0;
		break;
	    case AD_DRST:
	    case AD_DRDX:
	    case AD_DRCO:
		if (!magr->mcan && !rn2(8)) {
		    if (vis)
/*JP			pline("%s %s was poisoned!", s_suffix(Monnam(magr)),*/
			pline("%sの%sは毒されている！", s_suffix(Monnam(magr)),
			      mpoisons_subj(magr, mattk));
		    if (resists_poison(mdef)) {
			if (vis)
/*JP			    pline_The("poison doesn't seem to affect %s.",*/
 			    pline("%sは毒の影響を与えない．",
				mon_nam(mdef));
		    } else {
			if (rn2(10)) tmp += rn1(10,6);
			else {
/*JP			    if (vis) pline_The("poison was deadly...");*/
			    if (vis) pline("毒で死にかけている．．．");
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
		if ((mdef->misc_worn_check & W_ARMH) && rn2(8)) {
		    if (vis) {
			Strcpy(buf, s_suffix(Monnam(mdef)));
/*JP			pline("%s helmet blocks %s attack to his head.",*/
			pline("%sの兜は%sの頭への攻撃を防いだ．",
				buf, s_suffix(mon_nam(magr)));
		    }
		    break;
		}
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
/*JP		    pline("%s last thought fades away...",*/
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

/* `mon' is hit by a sleep attack; return 1 if it's affected, 0 otherwise */
int
sleep_monst(mon, amt, how)
struct monst *mon;
int amt, how;
{
	if (resists_sleep(mon) ||
		(how >= 0 && resist(mon, (char)how, 0, NOTELL))) {
	    shieldeff(mon->mx, mon->my);
	} else if (mon->mcanmove) {
	    amt += (int) mon->mfrozen;
	    if (amt > 0) {	/* sleep for N turns */
		mon->mcanmove = 0;
		mon->mfrozen = min(amt, 127);
	    } else {		/* sleep until awakened */
		mon->msleep = 1;
	    }
	    return 1;
	}
	return 0;
}

/* sleeping grabber releases, engulfer doesn't; don't use for paralysis! */
void
slept_monst(mon)
struct monst *mon;
{
	if ((mon->msleep || !mon->mcanmove) && mon == u.ustuck &&
		!sticks(uasmon) && !u.uswallow) {
/*JP	    pline("%s grip relaxes.", s_suffix(Monnam(mon)));*/
	    pline("%sはうつろになった．", s_suffix(Monnam(mon)));
	    unstuck(mon);
	}
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
/*JP			pline("%s weapon is not affected.",*/
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
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "thrusts" : "swings",
	      his[pronoun_gender(magr)], xname(otemp), buf);*/
	pline("%sは%s%s%sを%s", Monnam(magr),
	      xname(otemp),
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "で" : "を振りまわし",
	      buf,
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "突いた" : "攻撃した");
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
		    if (resists_acid(magr)) {
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
/*JP			Sprintf(buf, "%s gaze is reflected by %%s %%s.",*/
			Sprintf(buf, "%sのにらみは%%sによって%%s．",
				s_suffix(mon_nam(mdef)));
			if (mon_reflects(magr,
					 canseemon(magr) ? buf : (char *)0))
				return(mdead|mhit);
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
		if (resists_cold(magr)) {
		    if (canseemon(magr)) {
/*JP			pline("%s is mildly chilly.", Monnam(magr));*/
			pline("%sは冷えた．", Monnam(magr));
			golemeffects(magr, AD_COLD, tmp);
		    }
		    tmp = 0;
		    break;
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
		if (resists_fire(magr)) {
		    if (canseemon(magr)) {
/*JP			pline("%s is mildly warmed.", Monnam(magr));*/
			pline("%sは暖かくなった．", Monnam(magr));
			golemeffects(magr, AD_FIRE, tmp);
		    }
		    tmp = 0;
		    break;
		}
		if(canseemon(magr))
/*JP		    pline("%s is suddenly very hot!", Monnam(magr));*/
		    pline("%sは突然とても熱くなった！", Monnam(magr));
		break;
	    case AD_ELEC:
		if (resists_elec(magr)) {
		    if (canseemon(magr)) {
/*JP			pline("%s is mildly tingled.", Monnam(magr));*/
		        pline("%sはピリピリしている．", Monnam(magr));
			golemeffects(magr, AD_ELEC, tmp);
		    }
		    tmp = 0;
		    break;
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
