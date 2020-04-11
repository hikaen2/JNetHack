/*	SCCS Id: @(#)uhitm.c	3.3	1999/08/16	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

STATIC_DCL boolean FDECL(known_hitum, (struct monst *,int *,struct attack *));
STATIC_DCL void FDECL(steal_it, (struct monst *, struct attack *));
STATIC_DCL boolean FDECL(hitum, (struct monst *,int,struct attack *));
STATIC_DCL boolean FDECL(hmon_hitmon, (struct monst *,struct obj *,int));
STATIC_DCL boolean FDECL(m_slips_free, (struct monst *mtmp,struct attack *mattk));
STATIC_DCL int FDECL(explum, (struct monst *,struct attack *));
STATIC_DCL void FDECL(start_engulf, (struct monst *));
STATIC_DCL void NDECL(end_engulf);
STATIC_DCL int FDECL(gulpum, (struct monst *,struct attack *));
STATIC_DCL boolean FDECL(hmonas, (struct monst *,int));
STATIC_DCL void FDECL(nohandglow, (struct monst *));

extern boolean notonhead;	/* for long worms */
/* The below might become a parameter instead if we use it a lot */
static int dieroll;
/* Used to flag attacks caused by Stormbringer's maliciousness. */
static boolean override_confirmation = FALSE;

#define PROJECTILE(obj)	((obj) && is_ammo(obj))

/* modified from hurtarmor() in mhitu.c */
/* This is not static because it is also used for monsters rusting monsters */
void
hurtmarmor(mdat, mdef, attk)
struct permonst *mdat;
struct monst *mdef;
int attk;
{
	boolean getbronze, rusting;
	int	hurt;
	struct obj *target;

	rusting = (attk == AD_RUST);
	if (rusting) {
		hurt = 1;
		target = which_armor(mdef, W_ARM);
		getbronze = (mdat == &mons[PM_BLACK_PUDDING] &&
			     target && is_corrodeable(target));
	} else {
		hurt=2;
		getbronze = FALSE;
	}
	/* What the following code does: it keeps looping until it
	 * finds a target for the rust monster.
	 * Head, feet, etc... not covered by metal, or covered by
	 * rusty metal, are not targets.  However, your body always
	 * is, no matter what covers it.
	 */
	while (1) {
	    switch(rn2(5)) {
	    case 0:
		target = which_armor(mdef, W_ARMH);
/*JP		if (!rust_dmg(target, rusting ? "helmet" : "leather helmet",
				     hurt, FALSE, mdef))
*/
		if (!rust_dmg(target, rusting ? "兜" : "皮の兜",
				     hurt, FALSE, mdef))
			continue;
		break;
	    case 1:
		target = which_armor(mdef, W_ARMC);
		if (target) {
		    if (!rusting)
/*JP			(void)rust_dmg(target, "cloak", hurt, TRUE, mdef);*/
			(void)rust_dmg(target, "クローク", hurt, TRUE, mdef);
		    break;
		}
		if (getbronze) {
		    target = which_armor(mdef, W_ARM);
/*JP		    (void)rust_dmg(target, "bronze armor", 3, TRUE, mdef);*/
		    (void)rust_dmg(target, "青銅の鎧", 3, TRUE, mdef);
		} else if ((target = which_armor(mdef, W_ARM)) != (struct obj *)0) {
		    (void)rust_dmg(target, xname(target), hurt, TRUE, mdef);
#ifdef TOURIST
		} else if ((target = which_armor(mdef, W_ARMU)) != (struct obj *)0) {
/*JP		    (void)rust_dmg(target, "shirt", hurt, TRUE, mdef);*/
		    (void)rust_dmg(target, "シャツ", hurt, TRUE, mdef);
#endif
		}
		break;
	    case 2:
		target = which_armor(mdef, W_ARMS);
/*JP		if (!rust_dmg(target, rusting ? "shield" : "wooden shield",
				     hurt, FALSE, mdef))
*/
		if (!rust_dmg(target, rusting ? "盾" : "木の盾",
				     hurt, FALSE, mdef))
			continue;
		break;
	    case 3:
		target = which_armor(mdef, W_ARMG);
/*JP		if (!rust_dmg(target, rusting ? "metal gauntlets" : "gloves",
				     hurt, FALSE, mdef))
*/
		if (!rust_dmg(target, rusting ? "金属の小手" : "小手",
				     hurt, FALSE, mdef))
			continue;
		break;
	    case 4:
		target = which_armor(mdef, W_ARMF);
/*JP		if (!rust_dmg(target, rusting ? "metal boots" : "boots",
				     hurt, FALSE, mdef))
*/
		if (!rust_dmg(target, rusting ? "金属の靴" : "靴",
				     hurt, FALSE, mdef))
			continue;
		break;
	    }
	    break; /* Out of while loop */
	}
}

boolean
attack_checks(mtmp, wep)
register struct monst *mtmp;
struct obj *wep;	/* uwep for attack(), null for kick_monster() */
{
	char qbuf[QBUFSZ];

	/* if you're close enough to attack, alert any waiting monster */
	mtmp->mstrategy &= ~STRAT_WAITMASK;

	if (u.uswallow && mtmp == u.ustuck) return FALSE;

	if (flags.forcefight) {
		/* Do this in the caller, after we checked that the monster
		 * didn't die from the blow.  Reason: putting the 'I' there
		 * causes the hero to forget the square's contents since
		 * both 'I' and remembered contents are stored in .glyph.
		 * If the monster dies immediately from the blow, the 'I' will
		 * not stay there, so the player will have suddenly forgotten
		 * the square's contents for no apparent reason.
		if (!canspotmon(mtmp) &&
		    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph))
			map_invisible(u.ux+u.dx, u.uy+u.dy);
		 */
		return FALSE;
	}

	/* Put up an invisible monster marker, but one exception is for
	 * monsters that hide.  That already prints a warning message and
	 * prevents you from hitting the monster just via the hidden monster
	 * code below; if we also did that here, similar behavior would be
	 * happening two turns in a row.
	 */
	if (!canspotmon(mtmp) &&
		    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph) &&
		    !(!Blind && mtmp->mundetected && hides_under(mtmp->data))) {
/*JP		pline("Wait!  There's %s there you can't see!",
			something);
*/
	    	pline("ちょっと待った！姿の見えない%sがいる！",
			something);
		map_invisible(u.ux+u.dx, u.uy+u.dy);
		/* if it was an invisible mimic, treat it as if we stumbled
		 * onto a visible mimic
		 */
		if(mtmp->m_ap_type && !Protection_from_shape_changers) {
		    if(!u.ustuck && !mtmp->mflee && dmgtype(mtmp->data,AD_STCK))
			u.ustuck = mtmp;
		}
		wakeup(mtmp); /* always necessary; also un-mimics mimics */
		return TRUE;
	}

	if(mtmp->m_ap_type && !Protection_from_shape_changers
						&& !sensemon(mtmp)) {
		/* If a hidden mimic was in a square where a player remembers
		 * some (probably different) unseen monster, the player is in
		 * luck--he attacks it even though it's hidden.
		 */
		if (glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph)) {
		    seemimic(mtmp);
		    return(FALSE);
		}
		stumble_onto_mimic(mtmp);
		return TRUE;
	}

	if (mtmp->mundetected && !canseemon(mtmp) &&
		(hides_under(mtmp->data) || mtmp->data->mlet == S_EEL)) {
	    mtmp->mundetected = mtmp->msleeping = 0;
	    newsym(mtmp->mx, mtmp->my);
	    if (glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph)) {
		seemimic(mtmp);
		return(FALSE);
	    }
	    if (!(Blind ? Blind_telepat : Unblind_telepat)) {
		struct obj *obj;

		if (Blind || (is_pool(mtmp->mx,mtmp->my) && !Underwater))
/*JP		    pline("Wait!  There's a hidden monster there!");*/
		    pline("待て！怪物が隠れている！");
		else if ((obj = level.objects[mtmp->mx][mtmp->my]) != 0)
/*JP		    pline("Wait!  There's %s hiding under %s!",
			  an(l_monnam(mtmp)), doname(obj));*/
		    pline("待て！%sの下に%sが隠れている！",
			  doname(obj), l_monnam(mtmp));

		return TRUE;
	    }
	}

	if (flags.confirm && mtmp->mpeaceful
	    && !Confusion && !Hallucination && !Stunned) {
		/* Intelligent chaotic weapons (Stormbringer) want blood */
		if (wep && wep->oartifact == ART_STORMBRINGER) {
			override_confirmation = TRUE;
			return(FALSE);
		}
		if (canspotmon(mtmp)) {
/*JP			Sprintf(qbuf, "Really attack %s?", mon_nam(mtmp));*/
			Sprintf(qbuf, "本当に%sを攻撃するの？", mon_nam(mtmp));
			if (yn(qbuf) != 'y') {
				flags.move = 0;
				return(TRUE);
			}
		}
	}

	return(FALSE);
}

schar
find_roll_to_hit(mtmp)
register struct monst *mtmp;
{
	schar tmp;
	int tmp2;

	tmp = 1 + Luck + abon() + find_mac(mtmp) + u.uhitinc +
		maybe_polyd(youmonst.data->mlevel, u.ulevel);

/*	it is unchivalrous to attack the defenseless or from behind */
	if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL &&
	    (!mtmp->mcanmove || mtmp->msleeping || mtmp->mflee) &&
	    u.ualign.record > -10) {
/*JP	    You("caitiff!");*/
	    pline("やっていることがまるで蛮族だ！");
	    adjalign(-1);
	}

/*	attacking peaceful creatures is bad for the samurai's giri */
	if (Role_if(PM_SAMURAI) && mtmp->mpeaceful &&
	    u.ualign.record > -10) {
/*JP	    You("dishonorably attack the innocent!");*/
	    pline("無実の者を攻撃するのは不名誉だ！");
	    adjalign(-1);
	}

/*	Adjust vs. (and possibly modify) monster state.		*/

	if(mtmp->mstun) tmp += 2;
	if(mtmp->mflee) tmp += 2;

	if (mtmp->msleeping) {
		mtmp->msleeping = 0;
		tmp += 2;
	}
	if(!mtmp->mcanmove) {
		tmp += 4;
		if(!rn2(10)) {
			mtmp->mcanmove = 1;
			mtmp->mfrozen = 0;
		}
	}
	if (is_orc(mtmp->data) && maybe_polyd(is_elf(youmonst.data),
			Race_if(PM_ELF)))
	    tmp++;
	if(Role_if(PM_MONK) && !Upolyd) {
	    if (uarm) {
/*JP	    	Your("armor is rather cumbersome...");*/
	    	pline("鎧が邪魔だ．．．");
	    	tmp -= urole.spelarmr;
	    } else if (!uwep)
	    	tmp += (u.ulevel / 3) + 2;
	}

/*	with a lot of luggage, your agility diminishes */
	if ((tmp2 = near_capacity()) != 0) tmp -= (tmp2*2) - 1;
	if (u.utrap) tmp -= 3;
/*	Some monsters have a combination of weapon attacks and non-weapon
 *	attacks.  It is therefore wrong to add hitval to tmp; we must add
 *	it only for the specific attack (in hmonas()).
 */
	if (uwep && !Upolyd) {
		tmp += hitval(uwep, mtmp);
		tmp += weapon_hit_bonus(uwep);
	}
	return tmp;
}

/* try to attack; return FALSE if monster evaded */
/* u.dx and u.dy must be set */
boolean
attack(mtmp)
register struct monst *mtmp;
{
	schar tmp;
	register struct permonst *mdat = mtmp->data;

	/* This section of code provides protection against accidentally
	 * hitting peaceful (like '@') and tame (like 'd') monsters.
	 * Protection is provided as long as player is not: blind, confused,
	 * hallucinating or stunned.
	 * changes by wwp 5/16/85
	 * More changes 12/90, -dkh-. if its tame and safepet, (and protected
	 * 07/92) then we assume that you're not trying to attack. Instead,
	 * you'll usually just swap places if this is a movement command
	 */
	/* Intelligent chaotic weapons (Stormbringer) want blood */
	if (is_safepet(mtmp)) {
	    if (!uwep || uwep->oartifact != ART_STORMBRINGER) {
		/* there are some additional considerations: this won't work
		 * if in a shop or Punished or you miss a random roll or
		 * if you can walk thru walls and your pet cannot (KAA) or
		 * if your pet is a long worm (unless someone does better).
		 * there's also a chance of displacing a "frozen" monster.
		 * sleeping monsters might magically walk in their sleep.
		 */
		unsigned int foo = (Punished ||
				    !rn2(7) || is_longworm(mtmp->data));

		if (*in_rooms(mtmp->mx, mtmp->my, SHOPBASE) || foo
			|| (IS_ROCK(levl[u.ux][u.uy].typ) &&
					!passes_walls(mtmp->data))) {
		    char buf[BUFSZ];

		    mtmp->mflee = 1;
		    mtmp->mfleetim = rnd(6);
		    Strcpy(buf, y_monnam(mtmp));
		    buf[0] = highc(buf[0]);
/*JP		    You("stop.  %s is in the way!", buf);*/
		    You("止まった．%sが道にいる！", buf);
		    return(TRUE);
		} else if ((mtmp->mfrozen || (! mtmp->mcanmove)
				|| (mtmp->data->mmove == 0)) && rn2(6)) {
/*JP		    pline("%s doesn't seem to move!", Monnam(mtmp));*/
		    pline("%sは動けないようだ．", Monnam(mtmp));
		    return(TRUE);
		} else return(FALSE);
	    }
	}

	/* possibly set in attack_checks;
	   examined in known_hitum, called via hitum or hmonas below */
	override_confirmation = FALSE;
	if (attack_checks(mtmp, uwep)) return(TRUE);

	if (Upolyd) {
		/* certain "pacifist" monsters don't attack */
		if(noattacks(youmonst.data)) {
/*JP			You("have no way to attack monsters physically.");*/
			You("物理的に怪物を攻撃するすべがない．");
			mtmp->mstrategy &= ~STRAT_WAITMASK;
			goto atk_done;
		}
	}

/*JP	if(check_capacity("You cannot fight while so heavily loaded."))*/
	if(check_capacity("あなたは物を沢山持ちすぎて戦えない．"))
	    goto atk_done;

	if (u.twoweap && !can_twoweapon())
		untwoweapon();

	if(unweapon) {
	    unweapon = FALSE;
	    if(flags.verbose){
		if(uwep)
/*JP		    You("begin bashing monsters with your %s.",
			aobjnam(uwep, (char *)0));*/
		    You("%sで怪物をなぐりつけた．",
			xname(uwep));
		else if (!cantwield(youmonst.data))
/*JP		    You("begin bashing monsters with your %s %s.",
			uarmg ? "gloved" : "bare",	/ * Del Lamb * /
			makeplural(body_part(HAND)));
*/
		    You("%sで怪物をなぐりつけた．",
			uarmg ? "グローブ" : "素手");
	    }
	}
	exercise(A_STR, TRUE);		/* you're exercising muscles */
	/* andrew@orca: prevent unlimited pick-axe attacks */
	u_wipe_engr(3);

	/* Is the "it died" check actually correct? */
	if(mdat->mlet == S_LEPRECHAUN && !mtmp->mfrozen && !mtmp->msleeping &&
	   !mtmp->mconf && mtmp->mcansee && !rn2(7) &&
	   (m_move(mtmp, 0) == 2 ||			    /* it died */
	   mtmp->mx != u.ux+u.dx || mtmp->my != u.uy+u.dy)) /* it moved */
		return(FALSE);

	tmp = find_roll_to_hit(mtmp);
	if (Upolyd)
		(void) hmonas(mtmp, tmp);
	else
		(void) hitum(mtmp, tmp, youmonst.data->mattk);
	mtmp->mstrategy &= ~STRAT_WAITMASK;

atk_done:
	/* see comment in attack_checks() */
	/* we only need to check for this if we did an attack_checks()
	 * and it returned 0 (it's okay to attack), and the monster didn't
	 * evade.
	 */
	if (flags.forcefight && mtmp->mhp > 0 && !canspotmon(mtmp) &&
	    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph) &&
	    !(u.uswallow && mtmp == u.ustuck))
		map_invisible(u.ux+u.dx, u.uy+u.dy);

	return(TRUE);
}

STATIC_OVL boolean
known_hitum(mon, mhit, uattk)	/* returns TRUE if monster still lives */
register struct monst *mon;
register int *mhit;
struct attack *uattk;
{
	register boolean malive = TRUE;

	if (override_confirmation) {
	    /* this may need to be generalized if weapons other than
	       Stormbringer acquire similar anti-social behavior... */
/*JP	    if (flags.verbose) Your("bloodthirsty blade attacks!");*/
	    if (flags.verbose) Your("武器は血に飢えている！");
	}

	if(!*mhit) {
	    missum(mon, uattk);
	} else {
	    int oldhp = mon->mhp;

		/* KMH, conduct */
		if (uwep) u.uconduct.weaphit++;

	    /* we hit the monster; be careful: it might die! */
	    notonhead = (mon->mx != u.ux+u.dx || mon->my != u.uy+u.dy);
	    malive = hmon(mon, uwep, 0);
	    if (malive && u.twoweap) malive = hmon(mon, uswapwep, 0);
	    if (malive) {
		/* monster still alive */
		if(!rn2(25) && mon->mhp < mon->mhpmax/2
			    && !(u.uswallow && mon == u.ustuck)) {
		    /* maybe should regurgitate if swallowed? */
		    mon->mflee = 1;
		    if(!rn2(3)) {
			mon->mfleetim = rnd(100);
/*JP			if (!Blind) pline("%s turns to flee!", (Monnam(mon)));*/
			if (!Blind) pline("%sは逃げた！", (Monnam(mon)));
		    }
		    if(u.ustuck == mon && !u.uswallow && !sticks(youmonst.data))
			u.ustuck = 0;
		}
		/* Vorpal Blade hit converted to miss */
		/* could be headless monster or worm tail */
		if (mon->mhp == oldhp)
			*mhit = 0;
		if (mon->wormno && *mhit)
			cutworm(mon, u.ux+u.dx, u.uy+u.dy, uwep);
	    }
	}
	return(malive);
}

STATIC_OVL boolean
hitum(mon, tmp, uattk)		/* returns TRUE if monster still lives */
struct monst *mon;
int tmp;
struct attack *uattk;
{
	boolean malive;
	int mhit = (tmp > (dieroll = rnd(20)) || u.uswallow);

	if(tmp > dieroll) exercise(A_DEX, TRUE);
	malive = known_hitum(mon, &mhit, uattk);
	(void) passive(mon, mhit, malive, AT_WEAP);
	return(malive);
}

boolean			/* general "damage monster" routine */
hmon(mon, obj, thrown)		/* return TRUE if mon still alive */
struct monst *mon;
struct obj *obj;
int thrown;
{
	boolean result, anger_guards;

	anger_guards = (mon->mpeaceful &&
			    (mon->ispriest || mon->isshk ||
			     mon->data == &mons[PM_WATCHMAN] ||
			     mon->data == &mons[PM_WATCH_CAPTAIN]));
	result = hmon_hitmon(mon, obj, thrown);
	if (mon->ispriest && !rn2(2)) ghod_hitsu(mon);
	if (anger_guards) (void)angry_guards(!flags.soundok);
	return result;
}

/* guts of hmon() */
STATIC_OVL boolean
hmon_hitmon(mon, obj, thrown)
struct monst *mon;
struct obj *obj;
int thrown;
{
	int tmp;
	struct permonst *mdat = mon->data;
	int barehand_silver_rings = 0;
	/* The basic reason we need all these booleans is that we don't want
	 * a "hit" message when a monster dies, so we have to know how much
	 * damage it did _before_ outputting a hit message, but any messages
	 * associated with the damage don't come out until _after_ outputting
	 * a hit message.
	 */
	boolean hittxt = FALSE, destroyed = FALSE;
	boolean get_dmg_bonus = TRUE;
	boolean ispoisoned = FALSE, needpoismsg = FALSE, poiskilled = FALSE;
	boolean silvermsg = FALSE;
	boolean valid_weapon_attack = FALSE;
	int wtype;
	struct obj *monwep;
	char yourbuf[BUFSZ];

	wakeup(mon);
	if(!obj) {	/* attack with bare hands */
	    if (mdat == &mons[PM_SHADE])
		tmp = 0;
	    else if (martial_bonus())
		tmp = rnd(4);	/* bonus for martial arts */
	    else
		tmp = rnd(2);
	    valid_weapon_attack = (tmp > 1);
	    /* blessed gloves give bonuses when fighting 'bare-handed' */
	    if (uarmg && uarmg->blessed && (is_undead(mdat) || is_demon(mdat)))
		tmp += rnd(4);
	    /* So do silver rings.  Note: rings are worn under gloves, so you
	     * don't get both bonuses.
	     */
	    if (!uarmg) {
		if (uleft && objects[uleft->otyp].oc_material == SILVER)
		    barehand_silver_rings++;
		if (uright && objects[uright->otyp].oc_material == SILVER)
		    barehand_silver_rings++;
		if (barehand_silver_rings && hates_silver(mdat)) {
		    tmp += rnd(20);
		    silvermsg = TRUE;
		}
	    }
	} else {
	    if(obj->oclass == WEAPON_CLASS || is_weptool(obj) ||
	       obj->oclass == GEM_CLASS) {

		/* is it not a melee weapon? */
		if (/* if you strike with a bow... */
		    is_launcher(obj) ||
		    /* or strike with a missile in your hand... */
		    (!thrown && (is_missile(obj) || is_ammo(obj))) ||
		    /* or use a pole at short range... */
		    (!thrown && is_pole(obj)) ||
		    /* or throw a missile without the proper bow... */
		    (is_ammo(obj) && !ammo_and_launcher(obj, uwep))) {
		    /* then do only 1-2 points of damage */
		    if (mdat == &mons[PM_SHADE] && obj->otyp != SILVER_ARROW)
			tmp = 0;
		    else
			tmp = rnd(2);
		} else {
		    tmp = dmgval(obj, mon);
		    /* a minimal hit doesn't exercise proficiency */
		    valid_weapon_attack = (tmp > 1);
		    if (!valid_weapon_attack || mon == u.ustuck) {
			;	/* no special bonuses */
		    } else if (mon->mflee && Role_if(PM_ROGUE) && !Upolyd) {
/*JP			You("strike %s from behind!", mon_nam(mon));*/
			You("%sを背後から攻撃した！", mon_nam(mon));
			tmp += rnd(u.ulevel);
			hittxt = TRUE;
		    } else if (dieroll == 2 && obj == uwep &&
			  obj->oclass == WEAPON_CLASS &&
			  (bimanual(obj) ||
			    (Role_if(PM_SAMURAI) && obj->otyp == KATANA && !uarms)) &&
			  ((wtype = weapon_type(obj)) != P_NONE &&
			    P_SKILL(wtype) >= P_SKILLED) &&
			  ((monwep = MON_WEP(mon)) != 0 &&
			    weapon_type(monwep) != P_WHIP &&
			    !obj_resists(monwep,
				 50 + 15 * greatest_erosion(monwep), 100))) {
			/*
			 * 2.5% chance of shattering defender's weapon when
			 * using a two-handed weapon; less if uwep is rusted.
			 * [dieroll == 2 is most successful non-beheading or
			 * -bisecting hit, in case of special artifact damage;
			 * the percentage chance is (1/20)*(50/100).]
			 */
			monwep->owornmask &= ~W_WEP;
			MON_NOWEP(mon);
			mon->weapon_check = NEED_WEAPON;
/*JP
			pline("%s %s shatter%s from the force of your blow!",
			      s_suffix(Monnam(mon)), xname(monwep),
			      (monwep->quan) == 1L ? "s" : "");
*/
			pline("%sの%sはあなたの一撃で粉々になった！",
			      s_suffix(Monnam(mon)), xname(monwep));
			m_useup(mon, monwep);
			/* If someone just shattered MY weapon, I'd flee! */
			if (rn2(4) && !mon->mflee) {
			    mon->mflee = 1;
			    mon->mfleetim = d(2,3);
			}
			hittxt = TRUE;
		    }

		    if (obj->oartifact &&
			artifact_hit(&youmonst, mon, obj, &tmp, dieroll)) {
			if(mon->mhp <= 0) /* artifact killed monster */
			    return FALSE;
			if (tmp == 0) return TRUE;
			hittxt = TRUE;
		    }
		    if (objects[obj->otyp].oc_material == SILVER
				&& hates_silver(mdat))
			silvermsg = TRUE;
		    if(!thrown && obj == uwep && obj->otyp == BOOMERANG &&
		       !rnl(3)) {
/*JP			pline("As you hit %s, %s breaks into splinters.",*/
			pline("%sを攻撃すると, %sはこっぱみじんになった．",
			      mon_nam(mon), the(xname(obj)));
			useup(obj);
			obj = (struct obj *) 0;
			hittxt = TRUE;
			if (mdat != &mons[PM_SHADE])
			    tmp++;
		    } else if(thrown && (is_ammo(obj) || is_missile(obj))) {
			if (ammo_and_launcher(obj, uwep)) {
			    /* Elves and Samurai do extra damage using
			     * their bows&arrows; they're highly trained.
			     */
			    if (Role_if(PM_SAMURAI) &&
				obj->otyp == YA && uwep->otyp == YUMI)
				tmp++;
			    else if (Race_if(PM_ELF) &&
				     obj->otyp == ELVEN_ARROW &&
				     uwep->otyp == ELVEN_BOW)
				tmp++;
			}
			if(obj->opoisoned && is_poisonable(obj))
			    ispoisoned = TRUE;
		    }
		}
	    } else if(obj->oclass == POTION_CLASS) {
		if (obj->quan > 1L)
		    setworn(splitobj(obj, 1L), W_WEP);
		else
		    setuwep((struct obj *)0);
		freeinv(obj);
		potionhit(mon, obj, TRUE);
		if (mon->mhp <= 0) return FALSE;	/* killed */
		hittxt = TRUE;
		/* in case potion effect causes transformation */
		mdat = mon->data;
		tmp = (mdat == &mons[PM_SHADE]) ? 0 : 1;
	    } else {
		boolean shade_aware = FALSE;

		switch(obj->otyp) {
		    case BOULDER:		/* 1d20 */
		    case HEAVY_IRON_BALL:	/* 1d25 */
		    case IRON_CHAIN:		/* 1d4+1 */
			tmp = dmgval(obj, mon);
			shade_aware = TRUE;	/* dmgval handles it */
			break;
		    case MIRROR:
			if (breaktest(obj)) {
/*JP			    You("break %s mirror.  That's bad luck!",
				shk_your(yourbuf, obj));
*/
			    You("%s鏡を壊してしまった．こりゃまいった！",
				shk_your(yourbuf, obj));
			    change_luck(-2);
			    useup(obj);
			    obj = (struct obj *) 0;
			    hittxt = TRUE;
			}
			tmp = 1;
			break;
#ifdef TOURIST
		    case EXPENSIVE_CAMERA:
/*JP
			You("succeed in destroying %s camera.  Congratulations!",
*/
			You("%sカメラを壊すことができた．おめでとう！",
			    shk_your(yourbuf, obj));
			useup(obj);
			return(TRUE);
#endif
		    case CORPSE:		/* fixed by polder@cs.vu.nl */
			if (touch_petrifies(&mons[obj->corpsenm])) {
			    tmp = 1;
			    hittxt = TRUE;
/*JP			    You("hit %s with %s corpse.", mon_nam(mon),
				obj->dknown ? the(mons[obj->corpsenm].mname) :
				an(mons[obj->corpsenm].mname));
*/
			    You("%sを%sの死体で攻撃した．", mon_nam(mon),
				jtrns_mon(mons[obj->corpsenm].mname,-1));

			    if (!munstone(mon, TRUE))
				minstapetrify(mon, TRUE);
			    if (resists_ston(mon)) break;
			    /* note: hp may be <= 0 even if munstoned==TRUE */
			    return (boolean) (mon->mhp > 0);
#if 0
			} else if (touch_petrifies(mdat)) {
			    /* maybe turn the corpse into a statue? */
#endif
			}
			tmp = (obj->corpsenm >= LOW_PM ?
					mons[obj->corpsenm].msize : 0) + 1;
			break;
		    case EGG:
		      {
			/* setting quantity to 1 forces complete `useup' */
#define useup_eggs(o)	{ o->quan = 1L; \
			  if (thrown) obfree(o,(struct obj *)0); \
			  else useup(o); \
			  o = (struct obj *)0; }	/* now gone */
			/*JP long cnt = obj->quan;*/

			tmp = 1;		/* nominal physical damage */
			get_dmg_bonus = FALSE;
			hittxt = TRUE;		/* message always given */
			/* egg is always either used up or transformed, so next
			   hand-to-hand attack should yield a "bashing" mesg */
			if (obj == uwep) unweapon = TRUE;
			if (obj->spe && obj->corpsenm >= LOW_PM){
			    if (obj->quan < 5)
				change_luck((schar) -(obj->quan));
			    else
				change_luck(-5);
			}

			if (touch_petrifies(&mons[obj->corpsenm])) {
			    /*learn_egg_type(obj->corpsenm);*/
/*JP			    You("hit %s with %s %s egg%s.  Splat!",
				mon_nam(mon),
				obj->known ? "the" : cnt > 1L ? "some" : "a",
				obj->known ? mons[obj->corpsenm].mname : "petrifying",
				plur(cnt));
*/
			    You("%sに%sの卵を投げつけた．ビチャッ！",
				mon_nam(mon),
				jtrns_mon(mons[obj->corpsenm].mname,-1));

			    obj->known = 1;	/* (not much point...) */
			    useup_eggs(obj);
			    if (!munstone(mon, TRUE))
				minstapetrify(mon, TRUE);
			    if (resists_ston(mon)) break;
			    return (boolean) (mon->mhp > 0);
			} else {	/* ordinary egg(s) */
/*JP			    const char *eggp =
				     (obj->corpsenm != NON_PM && obj->known) ?
					      the(mons[obj->corpsenm].mname) :
					      (cnt > 1L) ? "some" : "an";
*/
			    const char *eggp =
				     (obj->corpsenm != NON_PM && obj->known) ?
					      jtrns_mon(mons[obj->corpsenm].mname, -1) : "";
/*JP			    You("hit %s with %s egg%s.",
				mon_nam(mon), eggp, plur(cnt));
*/
			    You("%sに%s%s卵を投げつけた．",
				mon_nam(mon), eggp, *eggp ? "の" : "");
			    if (touch_petrifies(mdat)) {
/*JP				pline_The("egg%s %s alive any more...",
				      plur(cnt),
				      (cnt == 1L) ? "isn't" : "aren't");
*/
				pline("もう卵が孵化することはないだろう．．．");
				if (obj->timed) obj_stop_timers(obj);
				obj->otyp = ROCK;
				obj->oclass = GEM_CLASS;
				obj->oartifact = 0;
				obj->spe = 0;
				obj->known = obj->dknown = obj->bknown = 0;
				obj->owt = weight(obj);
				if (thrown) place_object(obj, mon->mx, mon->my);
			    } else {
/*JP				pline("Splat!");*/
				pline("ビチャッ！");
				useup_eggs(obj);
				exercise(A_WIS, FALSE);
			    }
			}
			break;
#undef useup_eggs
		      }
		    case CLOVE_OF_GARLIC:	/* no effect against demons */
			if (is_undead(mdat)) {
			    mon->mflee = 1;
			    mon->mfleetim += d(2,4);
/*JP			    pline("%s turns to flee!", Monnam(mon));*/
			    pline("%sは逃走した！", Monnam(mon));
			}
			tmp = 1;
			break;
		    case CREAM_PIE:
		    case BLINDING_VENOM:
			/* note: resists_blnd() does not apply here */
			if (Blind || !haseyes(mdat) || u.uswallow) {
/*JP
			    pline(obj->otyp==CREAM_PIE ? "Splat!" : "Splash!");
*/
			    pline(obj->otyp==CREAM_PIE ? "ビシャッ！" : "ピチャッ！");
			} else if (obj->otyp == BLINDING_VENOM) {
/*JP
			    pline_The("venom blinds %s%s!", mon_nam(mon),
					mon->mcansee ? "" : " further");
*/
			    pline("毒液で%sは%s目が見えなくなった！", mon_nam(mon),
					mon->mcansee ? "" : "さらに");
			} else {
			    char *whom = mon_nam(mon);
			    /* note: s_suffix returns a modifiable buffer */
			    if (haseyes(mdat) && mdat != &mons[PM_FLOATING_EYE])
/*JP
				whom = strcat(s_suffix(whom), " face");
			    pline_The("cream pie splashes over %s!", whom);
*/
				whom = strcat(s_suffix(whom), "の顔");
			    pline("クリームパイは%sにぶちまけられた！", whom);
			}
			if (mon->msleeping) mon->msleeping = 0;
			setmangry(mon);
			if (haseyes(mon->data) && !u.uswallow) {
			    mon->mcansee = 0;
			    tmp = rn1(25, 21);
			    if(((int) mon->mblinded + tmp) > 127)
				mon->mblinded = 127;
			    else mon->mblinded += tmp;
			}
			if (thrown) obfree(obj, (struct obj *)0);
			else useup(obj);
			hittxt = TRUE;
			get_dmg_bonus = FALSE;
			tmp = 0;
			break;
		    case ACID_VENOM: /* thrown (or spit) */
			if (resists_acid(mon)) {
/*JP				Your("venom hits %s harmlessly.",*/
				pline("毒液は%sには効果がなかった．",
					mon_nam(mon));
				tmp = 0;
			} else {
/*JP				Your("venom burns %s!", mon_nam(mon));*/
				Your("毒液は%sを焼いた！", mon_nam(mon));
				tmp = dmgval(obj, mon);
			}
			if (thrown) obfree(obj, (struct obj *)0);
			else useup(obj);
			hittxt = TRUE;
			get_dmg_bonus = FALSE;
			break;
		    default:
			/* non-weapons can damage because of their weight */
			/* (but not too much) */
			tmp = obj->owt/100;
			if(tmp < 1) tmp = 1;
			else tmp = rnd(tmp);
			if(tmp > 6) tmp = 6;
		}

		if (!shade_aware && mdat == &mons[PM_SHADE] && obj &&
				objects[obj->otyp].oc_material != SILVER)
		    tmp = 0;
	    }
	}

	/****** NOTE: perhaps obj is undefined!! (if !thrown && BOOMERANG)
	 *      *OR* if attacking bare-handed!! */

	if (get_dmg_bonus && tmp > 0) {
		tmp += u.udaminc;
		/* If you throw using a propellor, you don't get a strength
		 * bonus but you do get an increase-damage bonus.
		 */
		if(!thrown || !obj || !uwep || !ammo_and_launcher(obj, uwep))
		    tmp += dbon();
	}

	if (valid_weapon_attack) {
	    struct obj *wep;

	    /* to be valid a projectile must have had the correct projector */
	    wep = PROJECTILE(obj) ? uwep : obj;
	    tmp += weapon_dam_bonus(wep);
	    use_skill(weapon_type(wep), 1);
	}

	if (ispoisoned) {
	    int nopoison = (10 - (obj->owt/10));            
	    if(nopoison < 2) nopoison = 2;
	    if Role_if(PM_SAMURAI) {
/*JP		You("dishonorably use a poisoned weapon!");*/
		You("不名誉にも毒の武器を使用した！");
		adjalign(-sgn(u.ualign.type));
	    } else if ((u.ualign.type == A_LAWFUL) && (u.ualign.record > -10)) {
/*JP		You_feel("like an evil coward for using a poisoned weapon.");*/
		You("毒の武器を使用するのは卑怯だと感じた．");
		adjalign(-1);
	    }
	    if (obj && !rn2(nopoison)) {
		obj->opoisoned = FALSE;
/*JP		Your("%s%s no longer poisoned.", xname(obj),
		     (obj->quan == 1L) ? " is" : "s are");	/ **FIXME**/
		Your("%sはもう毒が塗られていない．", xname(obj));
	    }
	    if (resists_poison(mon))
		needpoismsg = TRUE;
	    else if (rn2(10))
		tmp += rnd(6);
	    else poiskilled = TRUE;
	}
	if (tmp < 1) {
	    /* make sure that negative damage adjustment can't result
	       in inadvertently boosting the victim's hit points */
	    tmp = 0;
	    if (mdat == &mons[PM_SHADE]) {
		if (!hittxt) {
/*JP
		    Your("attack passes harmlessly through %s.",
*/
		    Your("攻撃は%sをすっと通りぬけた．",
			mon_nam(mon));
		    hittxt = TRUE;
		}
	    } else {
		if (get_dmg_bonus) tmp = 1;
	    }
	}

	/* VERY small chance of stunning opponent if unarmed. */
	if (tmp > 1 && !thrown && !obj && !uwep && !uarm && !uarms && !Upolyd) {
	    if (rnd(100) < P_SKILL(P_BARE_HANDED_COMBAT) &&
			!bigmonst(mdat) && !thick_skinned(mdat)) {
		if (canspotmon(mon))
/*JP		    pline("%s staggers from your powerful strike!",*/
		    pline("%sはあなたの会心の一撃でよろめいた！",
			  Monnam(mon));
		mon->mstun = 1;
		hittxt = TRUE;
		if (mon->mcanmove && mon != u.ustuck) {
		    xchar mdx, mdy;

		    /* see if the monster has a place to move into */
		    mdx = mon->mx + u.dx;
		    mdy = mon->my + u.dy;
		    if (goodpos(mdx, mdy, mon)) {
			remove_monster(mon->mx, mon->my);
			newsym(mon->mx, mon->my);
			place_monster(mon, mdx, mdy);
			newsym(mon->mx, mon->my);
			set_apparxy(mon);
		    }
		}
	    }
	}

	mon->mhp -= tmp;
	if(mon->mhp < 1)
		destroyed = TRUE;
	if (mon->mtame && (!mon->mflee || mon->mfleetim) && tmp > 0) {
		unsigned fleetim;

		abuse_dog(mon);
		mon->mflee = TRUE;		/* Rick Richardson */
		fleetim = mon->mfleetim + (unsigned)(10 * rnd(tmp));
		mon->mfleetim = min(fleetim,127);
	}
	if((mdat == &mons[PM_BLACK_PUDDING] || mdat == &mons[PM_BROWN_PUDDING])
		   && obj && obj == uwep
		   && objects[obj->otyp].oc_material == IRON
		   && mon->mhp > 1 && !thrown && !mon->mcan
		   /* && !destroyed  -- guaranteed by mhp > 1 */ ) {
		if (clone_mon(mon)) {
/*JP			pline("%s divides as you hit it!", Monnam(mon));*/
			pline("あなたの攻撃で%sは分裂した！", Monnam(mon));
			hittxt = TRUE;
		}
	}

	if(!hittxt && !destroyed) {
		if(thrown)
		    /* thrown => obj exists */
		    hit(xname(obj), mon, exclam(tmp) );
/*JP		else if(!flags.verbose) You("hit it.");*/
		else if(!flags.verbose) pline("攻撃は命中した．");
/*JP		else You("%s %s%s", Role_if(PM_BARBARIAN) ? "smite" : "hit",
			 mon_nam(mon), canseemon(mon) ? exclam(tmp) : ".");
*/
		else Your("%sへの攻撃は命中した%s", mon_nam(mon), canseemon(mon)
			? exclam(tmp) : "．");
	}

	if (silvermsg) {
		const char *fmt;
		char *whom = mon_nam(mon);

		if (canspotmon(mon)) {
		    if (barehand_silver_rings == 1)
/*JP			fmt = "Your silver ring sears %s!";*/
			fmt = "%sは銀の指輪で焼かれた！";
		    else if (barehand_silver_rings == 2)
/*JP			fmt = "Your silver rings sear %s!";*/
			fmt = "%sは銀の指輪で焼かれた！";
		    else
/*JP			fmt = "The silver sears %s!";*/
			fmt = "%sは銀で焼かれた！";
		} else {
		    *whom = highc(*whom);	/* "it" -> "It" */
/*JP		    fmt = "%s is seared!";*/
		    fmt = "%sは焼かれた．";
		}
		/* note: s_suffix returns a modifiable buffer */
		if (!noncorporeal(mdat))
/*JP		    whom = strcat(s_suffix(whom), " flesh");*/
		    whom = strcat(s_suffix(whom), "の肉は");
		pline(fmt, whom);
	}

	if (needpoismsg)
/*JP		pline_The("poison doesn't seem to affect %s.", mon_nam(mon));*/
		pline("毒は%sに効かなかったようだ．", mon_nam(mon));
	if (poiskilled) {
/*JP		pline_The("poison was deadly...");*/
		pline("毒は致死量だった．．．");
		xkilled(mon, 0);
		return FALSE;
	} else if (destroyed) {
		killed(mon);	/* takes care of most messages */
	} else if(u.umconf && !thrown) {
		nohandglow(mon);
		if(!mon->mconf && !resist(mon, '+', 0, NOTELL)) {
			mon->mconf = 1;
			if (!mon->mstun && mon->mcanmove && !mon->msleeping &&
				canseemon(mon))
/*JP			    pline("%s appears confused.", Monnam(mon));*/
			    pline("%sは混乱しているようだ．", Monnam(mon));
		}
	}

#if 0
	if(mdat == &mons[PM_RUST_MONSTER] && obj && obj == uwep &&
		is_rustprone(obj) && obj->oeroded < MAX_ERODE) {
	    if (obj->greased)
		grease_protect(obj,(char *)0,FALSE);
	    else if (obj->oerodeproof || (obj->blessed && !rnl(4))) {
	        if (flags.verbose)
			pline("Somehow, your %s is not affected.",
			      is_sword(obj) ? "sword" : "weapon");
	    } else {
		Your("%s%s!", aobjnam(obj, "rust"),
		     obj->oeroded+1 == MAX_ERODE ? " completely" :
		     obj->oeroded ? " further" : "");
		obj->oeroded++;
	    }
	}
#endif

	return((boolean)(destroyed ? FALSE : TRUE));
}

/* check whether slippery clothing protects from hug or wrap attack */
/* [currently assumes that you are the attacker] */
STATIC_OVL boolean
m_slips_free(mdef, mattk)
struct monst *mdef;
struct attack *mattk;
{
	struct obj *obj;

	if (mattk->adtyp == AD_DRIN) {
	    /* intelligence drain attacks the head */
	    obj = which_armor(mdef, W_ARMH);
	} else {
	    /* grabbing attacks the body */
	    obj = which_armor(mdef, W_ARMC);		/* cloak */
	    if (!obj) obj = which_armor(mdef, W_ARM);	/* suit */
#ifdef TOURIST
	    if (!obj) obj = which_armor(mdef, W_ARMU);	/* shirt */
#endif
	}

	/* if your cloak/armor is greased, monster slips off; this
	   protection might fail (33% chance) when the armor is cursed */
	if (obj && (obj->greased || obj->otyp == OILSKIN_CLOAK) &&
		(!obj->cursed || rn2(3))) {
#if 0 /*JP*/
	    You("%s %s %s %s!",
		mattk->adtyp == AD_WRAP ?
			"slip off of" : "grab, but cannot hold onto",
		s_suffix(mon_nam(mdef)),
		obj->greased ? "greased" : "slippery",
		/* avoid "slippery slippery cloak"
		   for undiscovered oilskin cloak */
		(obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : "cloak");
#endif
	    You("%sの%s%s%s！",
		mon_nam(mdef),
		obj->greased ? "油の塗られた" : "滑りやすい",
		(obj->greased || objects[obj->otyp].oc_name_known) ?
		xname(obj) : "クローク",
		mattk->adtyp == AD_WRAP ?
			"で滑った" : "をつかまようとした，しかしできなかった");
	    if (obj->greased && !rn2(2)) {
/*JP		pline_The("grease wears off.");*/
		pline("油は落ちてしまった．");
		obj->greased = 0;
	    }
	    return TRUE;
	}
	return FALSE;
}

STATIC_DCL void NDECL(demonpet);
/*
 * Send in a demon pet for the hero.  Exercise wisdom.
 *
 * This function used to be inline to damageum(), but the Metrowerks compiler
 * (DR4 and DR4.5) screws up with an internal error 5 "Expression Too Complex."
 * Pulling it out makes it work.
 */
STATIC_OVL void
demonpet()
{
	struct permonst *pm;
	struct monst *dtmp;

/*JP	pline("Some hell-p has arrived!");*/
	pline("地獄の仲間が現われた！");
	pm = !rn2(6) ? &mons[ndemon(u.ualign.type)] : youmonst.data;
	if ((dtmp = makemon(pm, u.ux, u.uy, NO_MM_FLAGS)) != 0)
	    (void)tamedog(dtmp, (struct obj *)0);
	exercise(A_WIS, TRUE);
}

/*
 * Player uses theft attack against monster.
 *
 * If the target is wearing body armor, take all of its possesions;
 * otherwise, take one object.  [Is this really the behavior we want?]
 *
 * This routine implicitly assumes that there is no way to be able to
 * resist petfication (ie, be polymorphed into a xorn or golem) at the
 * same time as being able to steal (poly'd into nymph or succubus).
 * If that ever changes, the check for touching a cockatrice corpse
 * will need to be smarter about whether to break out of the theft loop.
 */
STATIC_OVL void
steal_it(mdef, mattk)
struct monst *mdef;
struct attack *mattk;
{
	struct obj *otmp, *stealoid, **minvent_ptr;
	long unwornmask;

	if (!mdef->minvent) return;		/* nothing to take */

	/* look for worn body armor */
	stealoid = (struct obj *)0;
	if (could_seduce(&youmonst, mdef, mattk)) {
	    /* find armor, and move it to end of inventory in the process */
	    minvent_ptr = &mdef->minvent;
	    while ((otmp = *minvent_ptr) != 0)
		if (otmp->owornmask & W_ARM) {
		    if (stealoid) panic("steal_it: multiple worn suits");
		    *minvent_ptr = otmp->nobj;	/* take armor out of minvent */
		    stealoid = otmp;
		    stealoid->nobj = (struct obj *)0;
		} else {
		    minvent_ptr = &otmp->nobj;
		}
	    *minvent_ptr = stealoid;	/* put armor back into minvent */
	}

	if (stealoid) {		/* we will be taking everything */
	    if (gender(mdef) == (int) u.mfemale &&
			youmonst.data->mlet == S_NYMPH)
/*JP		You("charm %s.  She gladly hands over her possessions.",*/
		You("%sをうっとりさせた．彼女はそっと持ち物をさしだした．",
		    mon_nam(mdef));
	    else
/*JP		You("seduce %s and %s starts to take off %s clothes.",
		    mon_nam(mdef), he[pronoun_gender(mdef)],
		    his[pronoun_gender(mdef)]);
*/
		You("%sを誘惑した．%sは服を脱ぎはじめた．",
		    mon_nam(mdef), he[pronoun_gender(mdef)]);
	}

	while ((otmp = mdef->minvent) != 0) {
	    /* take the object away from the monster */
	    obj_extract_self(otmp);
	    if ((unwornmask = otmp->owornmask) != 0L) {
		mdef->misc_worn_check &= ~unwornmask;
		otmp->owornmask = 0L;
		update_mon_intrinsics(mdef, otmp, FALSE);

		if (otmp == stealoid)	/* special message for final item */
/*JP		    pline("%s finishes taking off %s suit.",
			  Monnam(mdef), his[pronoun_gender(mdef)]);
*/
		    pline("%sは脱ぎ終えた．", Monnam(mdef));
	    }
	    /* give the object to the character */
/*JP	    otmp = hold_another_object(otmp, "You steal %s.",
				       doname(otmp), "You steal: ");
*/
	    otmp = hold_another_object(otmp, "あなたは%sを盗んだ．",
				       doname(otmp), "盗んだ物：");

	    if (otmp->otyp == CORPSE &&
		    touch_petrifies(&mons[otmp->corpsenm]) && !uarmg) {
		char kbuf[BUFSIZ];

/*JP		Sprintf(kbuf, "stolen %s corpse", mons[otmp->corpsenm].mname);*/
		Sprintf(kbuf, "%sの死体を盗んで", jtrns_mon(mons[otmp->corpsenm].mname,-1));
		instapetrify(kbuf);
		break;		/* stop the theft even if hero survives */
	    }
	    /* more take-away handling, after theft message */
	    if (unwornmask & W_WEP) {		/* stole wielded weapon */
		possibly_unwield(mdef);
	    } else if (unwornmask & W_ARMG) {	/* stole worn gloves */
		mselftouch(mdef, (const char *)0, TRUE);
		if (mdef->mhp <= 0)	/* it's now a statue */
		    return;		/* can't continue stealing */
	    }

	    if (!stealoid) break;	/* only taking one item */
	}
}

int
damageum(mdef, mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register struct permonst *pd = mdef->data;
	register int	tmp = d((int)mattk->damn, (int)mattk->damd);

	if (is_demon(youmonst.data) && !rn2(13) && !uwep
		&& u.umonnum != PM_SUCCUBUS && u.umonnum != PM_INCUBUS
		&& u.umonnum != PM_BALROG) {
	    demonpet();
	    return(0);
	}
	switch(mattk->adtyp) {
	    case AD_STUN:
		if(!Blind)
/*JP		    pline("%s staggers for a moment.", Monnam(mdef));*/
		    pline("%sは一瞬くらくらした．", Monnam(mdef));
		mdef->mstun = 1;
		/* fall through to next case */
	    case AD_WERE:	    /* no effect on monsters */
	    case AD_HEAL:
	    case AD_LEGS:
	    case AD_PHYS:
		if(mattk->aatyp == AT_WEAP) {
		    if(uwep) tmp = 0;
		} else if(mattk->aatyp == AT_KICK) {
		    if(thick_skinned(mdef->data)) tmp = 0;
		    if(mdef->data == &mons[PM_SHADE]) {
			if (!(uarmf && uarmf->blessed)) {
			    impossible("bad shade attack function flow?");
			    tmp = 0;
			} else
			    tmp = rnd(4); /* bless damage */
		    }
		}
		break;
	    case AD_FIRE:
		if (!Blind)
/*JP		    pline("%s is %s!", Monnam(mdef),
			  mattk->aatyp == AT_HUGS ?
			  "being roasted" : "on fire");
*/
		    pline("%sは%sになった", Monnam(mdef),
			  mattk->aatyp == AT_HUGS ?
			  "丸焼け" : "火だるま");
		if (pd == &mons[PM_STRAW_GOLEM] ||
		    pd == &mons[PM_PAPER_GOLEM]) {
		    if (!Blind)
/*JP		    	pline("%s burns completely!", Monnam(mdef));*/
		    	pline("%sは灰と化した！", Monnam(mdef));
		    xkilled(mdef,0);
		}
		tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
		tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
		if (resists_fire(mdef)) {
		    if (!Blind)
/*JP			pline_The("fire doesn't heat %s!", mon_nam(mdef));*/
		        pline("炎は%sに影響がない！", mon_nam(mdef));
		    golemeffects(mdef, AD_FIRE, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp = 0;
		}
		/* only potions damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
		break;
	    case AD_COLD:
/*JP		if (!Blind) pline("%s is covered in frost!", Monnam(mdef));*/
		if (!Blind) pline("%sは氷で覆われた！", Monnam(mdef));
		if (resists_cold(mdef)) {
		    shieldeff(mdef->mx, mdef->my);
		    if (!Blind)
/*JP			pline_The("frost doesn't chill %s!", mon_nam(mdef));*/
			pline("氷は%sを凍らすことができない！", mon_nam(mdef));
		    golemeffects(mdef, AD_COLD, tmp);
		    tmp = 0;
		}
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
		break;
	    case AD_ELEC:
/*JP		if (!Blind) pline("%s is zapped!", Monnam(mdef));*/
		if (!Blind) pline("%sは衝撃をくらった！", Monnam(mdef));
		tmp += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
		if (resists_elec(mdef)) {
		    if (!Blind)
/*JP			pline_The("zap doesn't shock %s!", mon_nam(mdef));*/
			pline("衝撃は%sに影響を与えない！", mon_nam(mdef));
		    golemeffects(mdef, AD_ELEC, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp = 0;
		}
		/* only rings damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
		break;
	    case AD_ACID:
		if (resists_acid(mdef)) tmp = 0;
		break;
	    case AD_STON:
		if (!munstone(mdef, TRUE))
		    minstapetrify(mdef, TRUE);
		tmp = 0;
		break;
#ifdef SEDUCE
	    case AD_SSEX:
#endif
	    case AD_SEDU:
	    case AD_SITM:
		steal_it(mdef, mattk);
		tmp = 0;
		break;
	    case AD_SGLD:
		if (mdef->mgold) {
		    u.ugold += mdef->mgold;
		    mdef->mgold = 0;
/*JP		    Your("purse feels heavier.");*/
		    You("財布が重くなったような気がした．");
		}
		exercise(A_DEX, TRUE);
		tmp = 0;
		break;
	    case AD_TLPT:
		if(tmp <= 0) tmp = 1;
		if(tmp < mdef->mhp) {
		    char nambuf[BUFSIZ];
		    boolean u_saw_mon = canseemon(mdef);
		    /* record the name before losing sight of monster */
		    Strcpy(nambuf, Monnam(mdef));
		    if (u_teleport_mon(mdef, FALSE) &&
			    u_saw_mon && !canseemon(mdef))
/*JP			pline("%s suddenly disappears!", nambuf);*/
			pline("%sは突然消えた！", Monnam(mdef));
		}
		break;
	    case AD_BLND:
		if (!resists_blnd(mdef)) {
/*JP		    if(!Blind) pline("%s is blinded.", Monnam(mdef));*/
		    if(!Blind) pline("%sは目が見えなくなった．", Monnam(mdef));
		    mdef->mcansee = 0;
		    mdef->mblinded += tmp;
		}
		tmp = 0;
		break;
	    case AD_CURS:
		if (night() && !rn2(10) && !mdef->mcan) {
		    if (mdef->data == &mons[PM_CLAY_GOLEM]) {
			if (!Blind)
/*JP			    pline("Some writing vanishes from %s head!",*/
			    pline("いくつかの文字が%sの頭から消えた！",
				s_suffix(mon_nam(mdef)));
			xkilled(mdef, 0);
			/* Don't return yet; keep hp<1 and tmp=0 for pet msg */
		    } else {
			mdef->mcan = 1;
/*JP			You("chuckle.");*/
			You("くすくす笑った．");
		    }
		}
		tmp = 0;
		break;
	    case AD_DRLI:
		if (rn2(2) && !resists_drli(mdef)) {
			int xtmp = d(2,6);
/*JP			pline("%s suddenly seems weaker!", Monnam(mdef));*/
			pline("%sは突然弱くなったように見えた！", Monnam(mdef));
			mdef->mhpmax -= xtmp;
			if ((mdef->mhp -= xtmp) <= 0 || !mdef->m_lev) {
/*JP				pline("%s dies!", Monnam(mdef));*/
				pline("%sは死んだ！", Monnam(mdef));
				xkilled(mdef,0);
			} else
				mdef->m_lev--;
		}
		tmp = 0;
		break;
	    case AD_RUST:
		if (pd == &mons[PM_IRON_GOLEM]) {
/*JP			pline("%s falls to pieces!", Monnam(mdef));*/
			pline("%sはバラバラになった！", Monnam(mdef));
			xkilled(mdef,0);
		}
		hurtmarmor(youmonst.data, mdef, AD_RUST);
		tmp = 0;
		break;
	    case AD_DCAY:
		if (pd == &mons[PM_WOOD_GOLEM] ||
		    pd == &mons[PM_LEATHER_GOLEM]) {
/*JP			pline("%s falls to pieces!", Monnam(mdef));*/
			pline("%sはバラバラになった！", Monnam(mdef));
			xkilled(mdef,0);
		}
		hurtmarmor(youmonst.data, mdef, AD_DCAY);
		tmp = 0;
		break;
	    case AD_DRST:
	    case AD_DRDX:
	    case AD_DRCO:
		if (!rn2(8)) {
/*JP		    Your("%s was poisoned!", mpoisons_subj(&youmonst, mattk));*/
		    Your("%sは毒されている！", mpoisons_subj(&youmonst, mattk));
		    if (resists_poison(mdef))
/*JP			pline_The("poison doesn't seem to affect %s.",*/
			pline("毒は%sに影響を与えない．",
				mon_nam(mdef));
		    else {
			if (!rn2(10)) {
/*JP			    Your("poison was deadly...");*/
			    Your("与えた毒は致死量だった．．．");
			    tmp = mdef->mhp;
			} else tmp += rn1(10,6);
		    }
		}
		break;
	    case AD_DRIN:
		if (notonhead || !has_head(mdef->data)) {
/*JP		    pline("%s doesn't seem harmed.", Monnam(mdef));*/
		    pline("%sは傷ついたようには見えない．", Monnam(mdef));
		    tmp = 0;
		    break;
		}
		if (m_slips_free(mdef, mattk)) break;

		if ((mdef->misc_worn_check & W_ARMH) && rn2(8)) {
/*JP		    pline("%s helmet blocks your attack to %s head.",
			  s_suffix(Monnam(mdef)), his[pronoun_gender(mdef)]);
*/
		    pline("%sの兜が頭への攻撃を防いだ．",
			  s_suffix(Monnam(mdef)));
		    break;
		}

/*JP		You("eat %s brain!", s_suffix(mon_nam(mdef)));*/
		You("%sの脳を食べた！", s_suffix(mon_nam(mdef)));
		if (mindless(mdef->data)) {
/*JP		    pline("%s doesn't notice.", Monnam(mdef));*/
		    pline("%sはぼーっとしている．", Monnam(mdef));
		    break;
		}
		tmp += rnd(10);
		morehungry(-rnd(30)); /* cannot choke */
		if (ABASE(A_INT) < AMAX(A_INT)) {
			ABASE(A_INT) += rnd(4);
			if (ABASE(A_INT) > AMAX(A_INT))
				ABASE(A_INT) = AMAX(A_INT);
			flags.botl = 1;
		}
		exercise(A_WIS, TRUE);
		break;
	    case AD_STCK:
		if (!sticks(mdef->data))
		    u.ustuck = mdef; /* it's now stuck to you */
		break;
	    case AD_WRAP:
		if (!sticks(mdef->data)) {
		    if (!u.ustuck && !rn2(10)) {
			if (m_slips_free(mdef, mattk)) {
			    tmp = 0;
			} else {
/*JP			    You("swing yourself around %s!",*/
			    You("%sの周囲をいったりきたりした！",
				  mon_nam(mdef));
			    u.ustuck = mdef;
			}
		    } else if(u.ustuck == mdef) {
			/* Monsters don't wear amulets of magical breathing */
			if (is_pool(u.ux,u.uy) && !is_swimmer(mdef->data)) {
/*JP			    You("drown %s...", mon_nam(mdef));*/
			    You("%sを溺れさせた．．．", mon_nam(mdef));
			    tmp = mdef->mhp;
			} else if(mattk->aatyp == AT_HUGS)
/*JP			    pline("%s is being crushed.", Monnam(mdef));*/
			    pline("%sは押しつぶされた．", Monnam(mdef));
		    } else {
			tmp = 0;
			if (flags.verbose)
/*JP			    You("brush against %s %s.",
				s_suffix(mon_nam(mdef)),
				mbodypart(mdef, LEG));
*/
			    You("%sの%sに触れた．",
				s_suffix(mon_nam(mdef)),
				mbodypart(mdef, LEG));
		    }
		} else tmp = 0;
		break;
	    case AD_PLYS:
		if (mdef->mcanmove && !rn2(3) && tmp < mdef->mhp) {
/*JP		    if (!Blind) pline("%s is frozen by you!", Monnam(mdef));*/
		    if (!Blind) pline("%sはあなたの眼力で動けなくなった！", Monnam(mdef));
		    mdef->mcanmove = 0;
		    mdef->mfrozen = rnd(10);
		}
		break;
	    case AD_SLEE:
		if (!mdef->msleeping && sleep_monst(mdef, rnd(10), -1)) {
		    if (!Blind)
/*JP			pline("%s is put to sleep by you!", Monnam(mdef));*/
			pline("%sは突然眠りにおちた！", Monnam(mdef));
		    slept_monst(mdef);
		}
		break;
	    case AD_SLIM:
	    	if (!rn2(4) && mdef->data != &mons[PM_FIRE_VORTEX] &&
	    			mdef->data != &mons[PM_FIRE_ELEMENTAL] &&
	    			mdef->data != &mons[PM_GREEN_SLIME]) {
/*JP	    	    You("turn %s into slime.", mon_nam(mdef));*/
	    	    You("%sは液状になった．", mon_nam(mdef));
	    	    (void) newcham(mdef, &mons[PM_GREEN_SLIME]);
	    	    tmp = 0;
	    	}
	    	break;
	    case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
			/* There's no msomearmor() function, so just do damage */
			break;
	    default:	tmp = 0;
			break;
	}

	if((mdef->mhp -= tmp) < 1) {
	    if (mdef->mtame && !cansee(mdef->mx,mdef->my)) {
/*JP		You_feel("embarrassed for a moment.");*/
		You("しばらく困惑した．");
		if (tmp) xkilled(mdef, 0); /* !tmp but hp<1: already killed */
	    } else if (!flags.verbose) {
/*JP		You("destroy it!");*/
		You("倒した！");
		if (tmp) xkilled(mdef, 0);
	    } else
		if (tmp) killed(mdef);
	    return(2);
	}
	return(1);
}

STATIC_OVL int
explum(mdef, mattk)
register struct monst *mdef;
register struct attack *mattk;
{
/*JP*/
	char *jad = "";    
	register int tmp = d((int)mattk->damn, (int)mattk->damd);

/*JP	You("explode!");*/
	You("爆発した！");
	switch(mattk->adtyp) {
	    boolean resistance; /* only for cold/fire/elec */

	    case AD_BLND:
		if (!resists_blnd(mdef)) {
/*JP		    pline("%s is blinded by your flash of light!", Monnam(mdef));*/
		    pline("%sはまばゆい光で目がくらんだ！", Monnam(mdef));
		    mdef->mblinded = min((int)mdef->mblinded + tmp, 127);
		    mdef->mcansee = 0;
		}
		break;
	    case AD_HALU:
		if (haseyes(mdef->data) && mdef->mcansee) {
/*JP		    pline("%s is affected by your flash of light!",*/
		    pline("%sはまばゆい光で影響をうけた！",
			  Monnam(mdef));
		    mdef->mconf = 1;
		}
		break;
	    case AD_COLD:
		jad = "氷";
		resistance = resists_cold(mdef);
		goto common;
	    case AD_FIRE:
		jad = "炎";
		resistance = resists_fire(mdef);
		goto common;
	    case AD_ELEC:
		jad = "電撃";
		resistance = resists_elec(mdef);
common:
		if (!resistance) {
/*JP		    pline("%s gets blasted!", Monnam(mdef));*/
		    pline("%sはを浴びた！", jad);
		    mdef->mhp -= tmp;
		    if (mdef->mhp <= 0) {
			 killed(mdef);
			 return(2);
		    }
		} else {
		    shieldeff(mdef->mx, mdef->my);
		    if (is_golem(mdef->data))
			golemeffects(mdef, (int)mattk->adtyp, tmp);
		    else
/*JP			pline_The("blast doesn't seem to affect %s.",*/
			pline("氷の風は%sに影響を与えなかったようだ．",
				mon_nam(mdef));
		}
		break;
	    default:
		break;
	}
	return(1);
}

STATIC_OVL void
start_engulf(mdef)
struct monst *mdef;
{
	if (!Invisible) {
		map_location(u.ux, u.uy, TRUE);
		tmp_at(DISP_ALWAYS, mon_to_glyph(&youmonst));
		tmp_at(mdef->mx, mdef->my);
	}
/*JP	You("engulf %s!", mon_nam(mdef));*/
	You("%sを飲み込んだ！", mon_nam(mdef));
	delay_output();
	delay_output();
}

STATIC_OVL void
end_engulf()
{
	if (!Invisible) {
		tmp_at(DISP_END, 0);
		newsym(u.ux, u.uy);
	}
}

STATIC_OVL int
gulpum(mdef,mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register int tmp;
	register int dam = d((int)mattk->damn, (int)mattk->damd);
	struct obj *otmp;
	/* Not totally the same as for real monsters.  Specifically, these
	 * don't take multiple moves.  (It's just too hard, for too little
	 * result, to program monsters which attack from inside you, which
	 * would be necessary if done accurately.)  Instead, we arbitrarily
	 * kill the monster immediately for AD_DGST and we regurgitate them
	 * after exactly 1 round of attack otherwise.  -KAA
	 */

	if(mdef->data->msize >= MZ_HUGE) return 0;

	if(u.uhunger < 1500 && !u.uswallow) {
	    for (otmp = mdef->minvent; otmp; otmp = otmp->nobj)
		(void) snuff_lit(otmp);

          if(0){
//	    if(!touch_petrifies(mdef->data) || Stone_resistance) {
#ifdef LINT	/* static char msgbuf[BUFSZ]; */
		char msgbuf[BUFSZ];
#else
		static char msgbuf[BUFSZ];
#endif
		start_engulf(mdef);
		switch(mattk->adtyp) {
		    case AD_DGST:
			/* eating a Rider or its corpse is fatal */
			if (is_rider(mdef->data)) {
/*JP			 pline("Unfortunately, digesting any of it is fatal.");*/
			 pline("それを食べるのは致命的な間違いだ．");
			    end_engulf();
/*JP			    Sprintf(msgbuf, "unwisely tried to eat %s",
				    mdef->data->mname);
*/
			    Sprintf(msgbuf, "愚かにも%sを食べて",
				    jtrns_mon(mdef->data->mname, -1));
			    killer = msgbuf;
			    killer_format = NO_KILLER_PREFIX;
			    done(DIED);
			    return 0;		/* lifesaved */
			}

			if (Slow_digestion) {
			    dam = 0;
			    break;
			}

			/* KMH, conduct */
			u.uconduct.food++;
			u.uconduct.flesh++;
			if (is_meaty(mdef->data)) u.uconduct.meat++;

			u.uhunger += mdef->data->cnutrit;
			newuhs(FALSE);
			xkilled(mdef,2);
/*JP			Sprintf(msgbuf, "You totally digest %s.",*/
			Sprintf(msgbuf, "あなたは%sを完全に消化した．",
					mon_nam(mdef));
			if ((tmp = 3 + (mdef->data->cwt >> 6)) != 0) {
			    /* setting afternmv = end_engulf is tempting,
			     * but will cause problems if the player is
			     * attacked (which uses his real location) or
			     * if his See_invisible wears off
			     */
/*JP			    You("digest %s.", mon_nam(mdef));*/
			    You("%sを消化している．", mon_nam(mdef));
			    nomul(-tmp);
			    nomovemsg = msgbuf;
			} else pline(msgbuf);
			end_engulf();
			exercise(A_CON, TRUE);
			return(2);
		    case AD_PHYS:
/*JP			pline("%s is pummeled with your debris!",Monnam(mdef));*/
			pline("%sは瓦礫で痛めつけられた！",Monnam(mdef));
			break;
		    case AD_ACID:
/*JP			pline("%s is covered with your goo!", Monnam(mdef));*/
			pline("%sはねばつくものでおわれた！", Monnam(mdef));
			if (resists_acid(mdef)) {
/*JP			    pline("It seems harmless to %s.", mon_nam(mdef));*/
			    pline("が，%sはなんともない．", mon_nam(mdef));
			    dam = 0;
			}
			break;
		    case AD_BLND:
			if (!resists_blnd(mdef)) {
			    if (mdef->mcansee)
/*JP				pline("%s can't see in there!", Monnam(mdef));*/
				pline("%sは目が見えなくなった！", Monnam(mdef));
			    mdef->mcansee = 0;
			    dam += mdef->mblinded;
			    if (dam > 127) dam = 127;
			    mdef->mblinded = dam;
			}
			dam = 0;
			break;
		    case AD_ELEC:
			if (rn2(2)) {
/*JP			    pline_The("air around %s crackles with electricity.", mon_nam(mdef));*/
			    pline("%sの回りの空気は静電気でピリピリしている．", mon_nam(mdef));

			    if (resists_elec(mdef)) {
/*JP				pline("%s seems unhurt.", Monnam(mdef));*/
				pline("が，%sは平気なようだ．", Monnam(mdef));
				dam = 0;
			    }
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		    case AD_COLD:
			if (rn2(2)) {
			    if (resists_cold(mdef)) {
/*JP				pline("%s seems mildly chilly.", Monnam(mdef));*/
				pline("%sは冷えたようだ．", Monnam(mdef));
				dam = 0;
			    } else
/*JP				pline("%s is freezing to death!",Monnam(mdef));*/
				pline("%sは凍死してしまった！",Monnam(mdef));
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		    case AD_FIRE:
			if (rn2(2)) {
			    if (resists_fire(mdef)) {
/*JP				pline("%s seems mildly hot.", Monnam(mdef));*/
				pline("%sは暖かくなったようだ．", Monnam(mdef));
				dam = 0;
			    } else
/*JP				pline("%s is burning to a crisp!",Monnam(mdef));*/
				pline("%sは燃えてカラカラになった！",Monnam(mdef));

			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		}
		end_engulf();
		if ((mdef->mhp -= dam) <= 0) {
		    killed(mdef);
		    return(2);
		}
/*JP		You("%s %s!", is_animal(youmonst.data) ? "regurgitate"
			: "expel", mon_nam(mdef));
*/
		You("%sを%s！", mon_nam(mdef),
		    is_animal(youmonst.data) ? "吐き戻した" : "吐出した");
		if (Slow_digestion || is_animal(youmonst.data)) {
/*JP		    pline("Obviously, you didn't like %s taste.",*/
		    pline("どうも%sの味は好きになれない．",
			  s_suffix(mon_nam(mdef)));
		}
	    } else {
		char kbuf[BUFSZ];

/*JP		You("bite into %s.", mon_nam(mdef));*/
		You("%sに噛みついた", mon_nam(mdef));
/*JP		Sprintf(kbuf, "swallowing %s whole", an(mdef->data->mname));*/
		Sprintf(kbuf, "%sを完全に飲み込んで", a_monnam(mdef));
		instapetrify(kbuf);
	    }
	}
	return(0);
}

void
missum(mdef,mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	if (could_seduce(&youmonst, mdef, mattk))
/*JP		You("pretend to be friendly to %s.", mon_nam(mdef));*/
		You("%sに友好的なふりをした．", mon_nam(mdef));
	else if(canspotmon(mdef) && flags.verbose)
/*JP		You("miss %s.", mon_nam(mdef));*/
		Your("%sへの攻撃は外れた．", mon_nam(mdef));
	else
/*JP		You("miss it.");*/
		Your("何者かへの攻撃は外れた．");
	if (!mdef->msleeping && mdef->mcanmove)
		wakeup(mdef);
}

STATIC_OVL boolean
hmonas(mon, tmp)		/* attack monster as a monster. */
register struct monst *mon;
register int tmp;
{
	register struct attack *mattk;
	int	i, sum[NATTK], hittmp = 0;
	int	nsum = 0;
	int	dhit = 0;

	for(i = 0; i < NATTK; i++) {

	    sum[i] = 0;
	    mattk = &(youmonst.data->mattk[i]);
	    switch(mattk->aatyp) {
		case AT_WEAP:
use_weapon:
	/* Certain monsters don't use weapons when encountered as enemies,
	 * but players who polymorph into them have hands or claws and thus
	 * should be able to use weapons.  This shouldn't prohibit the use
	 * of most special abilities, either.
	 */
	/* Potential problem: if the monster gets multiple weapon attacks,
	 * we currently allow the player to get each of these as a weapon
	 * attack.  Is this really desirable?
	 */
			if (uwep) {
			    hittmp = hitval(uwep, mon);
			    hittmp += weapon_hit_bonus(uwep);
			    tmp += hittmp;
			}
			dhit = (tmp > (dieroll = rnd(20)) || u.uswallow);
			/* KMH -- Don't accumulate to-hit bonuses */
			if (uwep) tmp -= hittmp;
			/* Enemy dead, before any special abilities used */
			if (!known_hitum(mon,&dhit,mattk)) {
			    sum[i] = 2;
			    break;
			} else sum[i] = 1;
			/* might be a worm that gets cut in half */
			if (m_at(u.ux+u.dx, u.uy+u.dy) != mon) return((boolean)(nsum != 0));
			/* Do not print "You hit" message, since known_hitum
			 * already did it.
			 */
			if (dhit && mattk->adtyp != AD_SPEL
				&& mattk->adtyp != AD_PHYS)
				sum[i] = damageum(mon,mattk);
			break;
		case AT_CLAW:
			if (i==0 && uwep && !cantwield(youmonst.data)) goto use_weapon;
#ifdef SEDUCE
			/* succubi/incubi are humanoid, but their _second_
			 * attack is AT_CLAW, not their first...
			 */
			if (i==1 && uwep && (u.umonnum == PM_SUCCUBUS ||
				u.umonnum == PM_INCUBUS)) goto use_weapon;
#endif
		case AT_KICK:
		case AT_BITE:
		case AT_STNG:
		case AT_TUCH:
		case AT_BUTT:
		case AT_TENT:
			if (i==0 && uwep && (youmonst.data->mlet==S_LICH)) goto use_weapon;
			if ((dhit = (tmp > rnd(20) || u.uswallow)) != 0) {
			    int compat;

			    if (!u.uswallow &&
				(compat=could_seduce(&youmonst, mon, mattk))) {
/*JP				You("%s %s %s.",
				    mon->mcansee && haseyes(mon->data)
				    ? "smile at" : "talk to",
				    mon_nam(mon),
				    compat == 2 ? "engagingly":"seductively");
*/
				You("%sへ%s%s．",
				    mon_nam(mon),
				    compat == 2 ? "魅力的に":"誘惑的に",
				    mon->mcansee && haseyes(mon->data)
				    ? "微笑みかけた" : "話しかけた");
				/* doesn't anger it; no wakeup() */
				sum[i] = damageum(mon, mattk);
				break;
			    }
			    wakeup(mon);
			    /* maybe this check should be in damageum()? */
			    if (mon->data == &mons[PM_SHADE] &&
					!(mattk->aatyp == AT_KICK &&
					    uarmf && uarmf->blessed)) {
/*JP				Your("attack passes harmlessly through %s.",*/
				Your("%sへの攻撃は失敗した．",
				    mon_nam(mon));
				break;
			    }
			    if (mattk->aatyp == AT_KICK)
/*JP				    You("kick %s.", mon_nam(mon));*/
				    You("%sを蹴った．", mon_nam(mon));
			    else if (mattk->aatyp == AT_BITE)
/*JP				    You("bite %s.", mon_nam(mon));*/
				    You("%sに噛みついた．", mon_nam(mon));
			    else if (mattk->aatyp == AT_STNG)
/*JP				    You("sting %s.", mon_nam(mon));*/
				    You("%sを突きさした．", mon_nam(mon));
			    else if (mattk->aatyp == AT_BUTT)
/*JP				    You("butt %s.", mon_nam(mon));*/
				    You("%sに頭突きをくらわした．", mon_nam(mon));
			    else if (mattk->aatyp == AT_TUCH)
/*JP				    You("touch %s.", mon_nam(mon));*/
				    You("%sに触れた．", mon_nam(mon));
			    else if (mattk->aatyp == AT_TENT)
/*JP				    Your("tentacles suck %s.", mon_nam(mon));*/
				    Your("触手が%sの体液を吸いとった．", mon_nam(mon));
/*JP			    else You("hit %s.", mon_nam(mon));*/
			    else Your("%sへの攻撃は命中した．", mon_nam(mon));
			    sum[i] = damageum(mon, mattk);
			} else
			    missum(mon, mattk);
			break;

		case AT_HUGS:
			/* automatic if prev two attacks succeed, or if
			 * already grabbed in a previous attack
			 */
			dhit = 1;
			wakeup(mon);
			if (mon->data == &mons[PM_SHADE])
/*JP			    Your("hug passes harmlessly through %s.",*/
			    You("%sを羽交い絞めにしようとしたが失敗した．",
				mon_nam(mon));
			else if (!sticks(mon->data) && !u.uswallow){
			    if (mon==u.ustuck) {
/*JP				pline("%s is being %s.", Monnam(mon),
				    u.umonnum==PM_ROPE_GOLEM ? "choked":
				    "crushed");*/
				pline("%sは%s．", Monnam(mon),
				    u.umonnum==PM_ROPE_GOLEM ? "首を絞められている":
				    "押しつぶされている");
				sum[i] = damageum(mon, mattk);
			    } else if(i >= 2 && sum[i-1] && sum[i-2]) {
/*JP				You("grab %s!", mon_nam(mon));*/
				You("%sをつかまえた！", mon_nam(mon));
				u.ustuck = mon;
				sum[i] = damageum(mon, mattk);
			    }
			}
			break;

		case AT_EXPL:	/* automatic hit if next to */
			dhit = -1;
			wakeup(mon);
			sum[i] = explum(mon, mattk);
			break;

		case AT_ENGL:
			if((dhit = (tmp > rnd(20+i)))) {
				wakeup(mon);
				if (mon->data == &mons[PM_SHADE])
/*JP				    Your("attempt to surround %s is harmless.",*/
				    You("%sを飲みこもうとしたが失敗した．",
					mon_nam(mon));
				else
				    sum[i]= gulpum(mon,mattk);
			} else
				missum(mon, mattk);
			break;

		case AT_MAGC:
			/* No check for uwep; if wielding nothing we want to
			 * do the normal 1-2 points bare hand damage...
			 */
			if (i==0 && (youmonst.data->mlet==S_KOBOLD
				|| youmonst.data->mlet==S_ORC
				|| youmonst.data->mlet==S_GNOME
				)) goto use_weapon;

		case AT_NONE:
		case AT_BOOM:
			continue;
			/* Not break--avoid passive attacks from enemy */

		case AT_BREA:
		case AT_SPIT:
		case AT_GAZE:	/* all done using #monster command */
			dhit = 0;
			break;

		default: /* Strange... */
			impossible("strange attack of yours (%d)",
				 mattk->aatyp);
	    }
	    if (dhit == -1)
		rehumanize();
	    if (sum[i] == 2)
		return((boolean)passive(mon, 1, 0, mattk->aatyp));
							/* defender dead */
	    else {
		(void) passive(mon, sum[i], 1, mattk->aatyp);
		nsum |= sum[i];
	    }
	    if (!Upolyd)
		break; /* No extra attacks if no longer a monster */
	    if (multi < 0)
		break; /* If paralyzed while attacking, i.e. floating eye */
	}
	return((boolean)(nsum != 0));
}

/*	Special (passive) attacks on you by monsters done here.		*/

int
passive(mon, mhit, malive, aatyp)
register struct monst *mon;
register boolean mhit;
register int malive;
uchar aatyp;
{
	register struct permonst *ptr = mon->data;
	register int i, tmp;

	for(i = 0; ; i++) {
	    if(i >= NATTK) return(malive | mhit);	/* no passive attacks */
	    if(ptr->mattk[i].aatyp == AT_NONE) break;	/* try this one */
	}
	/* Note: tmp not always used */
	if (ptr->mattk[i].damn)
	    tmp = d((int)ptr->mattk[i].damn, (int)ptr->mattk[i].damd);
	else if(ptr->mattk[i].damd)
	    tmp = d((int)mon->m_lev+1, (int)ptr->mattk[i].damd);
	else
	    tmp = 0;

/*	These affect you even if they just died */

	switch(ptr->mattk[i].adtyp) {

	  case AD_ACID:
	    if(mhit && rn2(2)) {
/*JP		if (Blind || !flags.verbose) You("are splashed!");*/
		if (Blind || !flags.verbose) You("何かを浴びせられた！");
/*JP		else	You("are splashed by %s acid!", */
		else	You("%sの酸を浴びせられた！", 
			                s_suffix(mon_nam(mon)));

		if (!Acid_resistance)
			mdamageu(mon, tmp);
		if(!rn2(30)) erode_armor(&youmonst, TRUE);
	    }
	    if(mhit && !rn2(6)) {
		if (aatyp == AT_KICK) {
		    if (uarmf)
			(void) rust_dmg(uarmf, xname(uarmf), 3, TRUE, &youmonst);
		} else if (aatyp == AT_WEAP || aatyp == AT_CLAW || aatyp == AT_MAGC || aatyp == AT_TUCH)
		    erode_weapon(&youmonst, TRUE);
	    }
	    exercise(A_STR, FALSE);
	    break;
	  case AD_STON:
	    if(mhit) {
	      /* mhit does not mean you physically hit; it just means the
	         attack was successful */
	      if ((aatyp == AT_KICK && !uarmf) ||
		    ((aatyp == AT_WEAP || aatyp == AT_CLAW || aatyp == AT_MAGC
				|| aatyp == AT_TUCH) && !uwep && !uarmg) ||
		    aatyp == AT_BITE || aatyp == AT_STNG || aatyp == AT_BUTT ||
		    aatyp == AT_TENT || aatyp == AT_HUGS || aatyp == AT_ENGL) {
		if (!Stone_resistance &&
		    !(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
/*JP			You("turn to stone...");*/
			You("石になった．．．");
			done_in_by(mon);
			return 2;
		}
	      }
	    }
	    break;
	  case AD_RUST:
	    if(mhit && !mon->mcan){
	      if (aatyp == AT_KICK) {
		if (uarmf)
		    (void) rust_dmg(uarmf, xname(uarmf), 1, TRUE, &youmonst);
	      } else if (aatyp == AT_WEAP || aatyp == AT_CLAW ||
			aatyp == AT_MAGC || aatyp == AT_TUCH)
		erode_weapon(&youmonst, FALSE);
	    }
	    break;
	  case AD_MAGM:
	    /* wrath of gods for attacking Oracle */
	    if(Antimagic) {
		shieldeff(u.ux, u.uy);
/*JP		pline("A hail of magic missiles narrowly misses you!");*/
		pline("魔法の矢の雨をなんとかかわした！");
	    } else {
/*JP		You("are hit by magic missiles appearing from thin air!");*/
		pline("突如空中に現われた魔法の矢が命中した！");
		mdamageu(mon, tmp);
	    }
	    break;
	  case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
	    {
	  		struct obj *obj = (aatyp == AT_KICK) ? uarmf :
	  				uwep ? uwep : uarmg;

	    	if (mhit && !mon->mcan && obj) {
	    	    if (drain_item(obj) && (obj->known ||
	    	    		obj->oclass == ARMOR_CLASS))
/*JP	    	    	Your("%s less effective.", aobjnam(obj, "seem"));*/
			Your("%sから魔力が消えた．", xname(obj));
	    	}
	    	break;
	    }
	  default:
	    break;
	}

/*	These only affect you if they still live */

	if(malive && !mon->mcan && rn2(3)) {

	    switch(ptr->mattk[i].adtyp) {

	      case AD_PLYS:
		if(ptr == &mons[PM_FLOATING_EYE]) {
		    if (!canseemon(mon)) {
			break;
		    }
		    if(mon->mcansee) {
/*JP		    if (ureflects("%s gaze is reflected by your %s.",*/
		    if (ureflects("%sのにらみは%sによって反射された．",
		    		s_suffix(Monnam(mon))))
		    	;
		    else if (Free_action)
/*JP		    	You("momentarily stiffen under %s gaze!",*/
		    	You("%sのにらみで一瞬硬直した！",
		    		s_suffix(mon_nam(mon)));
		    else {
/*JP			    You("are frozen by %s gaze!",*/
			    You("%sのにらみで動けなくなった！",
				  s_suffix(mon_nam(mon)));
			    nomul((ACURR(A_WIS) > 12 || rn2(4)) ? -tmp : -127);
			}
		    } else {
/*JP			pline("%s cannot defend itself.",
				Adjmonnam(mon,"blind"));
*/
			pline("%sは防御できない．",
				Adjmonnam(mon,"目の見えない"));
			if(!rn2(500)) change_luck(-1);
		    }
		} else if (Free_action) {
/*JP		    You("momentarily stiffen.");*/
		    You("一瞬硬直した．");
		} else { /* gelatinous cube */
/*JP		    You("are frozen by %s!", mon_nam(mon));*/
		    You("%sによって動けなくなった！", mon_nam(mon));
		    nomul(-tmp);
		    exercise(A_DEX, FALSE);
		}
		break;
	      case AD_COLD:		/* brown mold or blue jelly */
		if(monnear(mon, u.ux, u.uy)) {
		    if(Cold_resistance) {
			shieldeff(u.ux, u.uy);
/*JP			You_feel("a mild chill.");*/
			You("寒さを感じた．");
			ugolemeffects(AD_COLD, tmp);
			break;
		    }
/*JP		    You("are suddenly very cold!");*/
		    You("突然，猛烈に寒くなった！");
		    mdamageu(mon, tmp);
		/* monster gets stronger with your heat! */
		    mon->mhp += tmp / 2;
		    if (mon->mhpmax < mon->mhp) mon->mhpmax = mon->mhp;
		/* at a certain point, the monster will reproduce! */
		    if(mon->mhpmax > ((int) (mon->m_lev+1) * 8))
			(void)split_mon(mon, &youmonst);
		}
		break;
	      case AD_STUN:		/* specifically yellow mold */
		if(!Stunned)
		    make_stunned((long)tmp, TRUE);
		break;
	      case AD_FIRE:
		if(monnear(mon, u.ux, u.uy)) {
		    if(Fire_resistance) {
			shieldeff(u.ux, u.uy);
/*JP			You("feel mildly warm.");*/
			You("暖かさを感じた．");
			ugolemeffects(AD_FIRE, tmp);
			break;
		    }
/*JP		    You("are suddenly very hot!");*/
		    You("突然，猛烈に熱くなった！");
		    mdamageu(mon, tmp);
		}
		break;
	      case AD_ELEC:
		if(Shock_resistance) {
		    shieldeff(u.ux, u.uy);
/*JP		    You("feel a mild tingle.");*/
		    You("ピリピリと痺れを感じた．");
		    ugolemeffects(AD_ELEC, tmp);
		    break;
		}
/*JP		You("are jolted with electricity!");*/
		You("電気ショックをうけた！");
		mdamageu(mon, tmp);
		break;
	      default:
		break;
	    }
	}
	return(malive | mhit);
}

/* Note: caller must ascertain mtmp is mimicking... */
void
stumble_onto_mimic(mtmp)
struct monst *mtmp;
{
/*JP	const char *fmt = "Wait!  That's %s!",
		   *generic = "a monster",
		   *what = 0;
*/
	const char *fmt = "ちょっとまった！%sだ！",
		   *generic = "怪物",
		   *what = 0;

	if(!u.ustuck && !mtmp->mflee && dmgtype(mtmp->data,AD_STCK))
	    u.ustuck = mtmp;

	if (Blind) {
	    if (!Blind_telepat)
		what = generic;		/* with default fmt */
	    else if (mtmp->m_ap_type == M_AP_MONSTER)
		what = a_monnam(mtmp);	/* differs from what was sensed */
	} else {
	    int glyph = levl[u.ux+u.dx][u.uy+u.dy].glyph;

	    if (glyph_is_cmap(glyph) &&
		    (glyph_to_cmap(glyph) == S_hcdoor ||
		     glyph_to_cmap(glyph) == S_vcdoor))
/*JP		fmt = "The door actually was %s!";*/
		fmt = "扉は実際には%sだった！";
	    else if (glyph_is_object(glyph) &&
		    glyph_to_obj(glyph) == GOLD_PIECE)
/*JP		fmt = "That gold was %s!";*/
		fmt = "金塊は%sだった！";

	    /* cloned Wiz starts out mimicking some other monster and
	       might make himself invisible before being revealed */
	    if (mtmp->minvis && !See_invisible)
		what = generic;
	    else
		what = a_monnam(mtmp);
	}
	if (what) pline(fmt, what);

	wakeup(mtmp);	/* clears mimicking */
}

STATIC_OVL void
nohandglow(mon)
struct monst *mon;
{
	char *hands=makeplural(body_part(HAND));

	if (!u.umconf || mon->mconf) return;
	if (u.umconf == 1) {
		if (Blind)
/*JP			Your("%s stop tingling.", hands);*/
			Your("%sの痺れがとれた",hands);
		else
/*JP			Your("%s stop glowing %s.", hands, hcolor(red));*/
			Your("%sの%s輝きはなくなった．", hands, hcolor(red));
	} else {
		if (Blind)
/*JP			pline_The("tingling in your %s lessens.", hands);*/
			pline("%sの痺れがとれてきた．",hands);
		else
/*JP			Your("%s no longer glow so brightly %s.", hands,*/
			Your("%sの%s輝きがなくなってきた．",hands,
				hcolor(red));
	}
	u.umconf--;
}

int flash_hits_mon(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;	/* source of flash */
{
	int tmp, amt, res = 0, useeit = canseemon(mtmp);

	if (mtmp->msleeping) {
	    mtmp->msleeping = 0;
	    if (useeit) {
/*JP		pline_The("flash awakens %s.", mon_nam(mtmp));*/
		pline("閃光で%sが目を覚ました．", mon_nam(mtmp));

		res = 1;
	    }
	} else if (mtmp->data->mlet != S_LIGHT) {
	    if (!resists_blnd(mtmp)) {
		tmp = dist2(otmp->ox, otmp->oy, mtmp->mx, mtmp->my);
		if (useeit) {
/*JP		    pline("%s is blinded by the flash!", Monnam(mtmp));*/
		    pline("閃光で%sは目が見えなくなった．", Monnam(mtmp));
		    res = 1;
		}
		if (mtmp->data == &mons[PM_GREMLIN]) {
		    /* Rule #1: Keep them out of the light. */
		    amt = otmp->otyp == WAN_LIGHT ? d(1 + otmp->spe, 4) :
		          rn2(min(mtmp->mhp,4));
/*JP		    pline("%s %s!", Monnam(mtmp), amt > mtmp->mhp / 2 ?
			  "wails in agony" : "cries out in pain");*/
		    pline("%sは%s！", Monnam(mtmp), amt > mtmp->mhp / 2 ?
			  "苦痛の声をあげた" : "激痛で叫んだ");
		    if ((mtmp->mhp -= amt) <= 0) {
			if (flags.mon_moving)
			    monkilled(mtmp, (char *)0, AD_BLND);
			else
			    killed(mtmp);
		    } else if (cansee(mtmp->mx,mtmp->my) && !canspotmon(mtmp)){
			map_invisible(mtmp->mx, mtmp->my);
		    }
		}
		if (mtmp->mhp > 0) {
		    if (!flags.mon_moving) setmangry(mtmp);
		    if (tmp < 9 && !mtmp->isshk && rn2(4)) {
			mtmp->mflee = 1;
			if (rn2(4)) mtmp->mfleetim = rnd(100);
		    }
		    mtmp->mcansee = 0;
		    mtmp->mblinded = (tmp < 3) ? 0 : rnd(1 + 50/tmp);
		}
	    }
	}
	return res;
}

/*uhitm.c*/
