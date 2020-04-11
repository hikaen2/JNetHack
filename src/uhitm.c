/*	SCCS Id: @(#)uhitm.c	3.1	93/02/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

static boolean FDECL(known_hitum, (struct monst *,int));
static boolean FDECL(hitum, (struct monst *,int));
#ifdef POLYSELF
static int FDECL(explum, (struct monst *,struct attack *));
static int FDECL(gulpum, (struct monst *,struct attack *));
static boolean FDECL(hmonas, (struct monst *,int));
#endif
static void FDECL(nohandglow, (struct monst *));

extern boolean notonhead;	/* for long worms */
/* The below might become a parameter instead if we use it a lot */
static int dieroll;

struct monst *
clone_mon(mon)
struct monst *mon;
{
	coord mm;
	struct monst *m2;

	mm.x = mon->mx;
	mm.y = mon->my;
	if (!enexto(&mm, mm.x, mm.y, mon->data)) return (struct monst *)0;
	if (MON_AT(mm.x, mm.y) || mon->mhp <= 1) return (struct monst *)0;
	/* may have been extinguished for population control */
	if(mon->data->geno & G_EXTINCT) return((struct monst *) 0);
	m2 = newmonst(0);
	*m2 = *mon;			/* copy condition of old monster */
	m2->nmon = fmon;
	fmon = m2;
	m2->m_id = flags.ident++;
	m2->mx = mm.x;
	m2->my = mm.y;

	m2->minvent = (struct obj *) 0; /* objects don't clone */
	m2->mleashed = FALSE;
	m2->mgold = 0L;
	/* Max HP the same, but current HP halved for both.  The caller
	 * might want to override this by halving the max HP also.
	 */
	m2->mhpmax = mon->mhpmax;
	m2->mhp = mon->mhp /= 2;

	/* since shopkeepers and guards will only be cloned if they've been
	 * polymorphed away from their original forms, the clone doesn't have
	 * room for the extra information.  we also don't want two shopkeepers
	 * around for the same shop.
	 * similarly, clones of named monsters don't have room for the name,
	 * so we just make the clone unnamed instead of bothering to create
	 * a clone with room and copying over the name from the right place
	 * (which changes if the original was a shopkeeper or guard).
	 */
	if (mon->isshk) m2->isshk = FALSE;
	if (mon->isgd) m2->isgd = FALSE;
	if (mon->ispriest) m2->ispriest = FALSE;
	m2->mxlth = 0;
	m2->mnamelth = 0;
	place_monster(m2, m2->mx, m2->my);
	newsym(m2->mx,m2->my);	/* display the new monster */
	if (mon->mtame) {
	    struct monst *m3;

	    /* because m2 is a copy of mon it is tame but not init'ed.
	     * however, tamedog will not re-tame a tame dog, so m2
	     * must be made non-tame to get initialized properly.
	     */
	    m2->mtame = 0;
	    if ((m3 = tamedog(m2, (struct obj *)0)) != 0)
		m2 = m3;
	}
	return m2;
}

boolean
special_case(mtmp)
/* Moved this code from attack() in order to 	*/
/* avoid having to duplicate it in dokick.	*/
register struct monst *mtmp;
{
	char qbuf[QBUFSZ];

	if(mtmp->m_ap_type && !Protection_from_shape_changers
						&& !sensemon(mtmp)) {
		stumble_onto_mimic(mtmp);
		mtmp->data->mflags3 &= ~M3_WAITMASK;
		return(1);
	}

	if(mtmp->mundetected && hides_under(mtmp->data) && !canseemon(mtmp)) {
		mtmp->mundetected = 0;
		if (!(Blind ? Telepat : (HTelepat & (W_ARMH|W_AMUL|W_ART)))) {
			register struct obj *obj;

			if(Blind)
/*JP			    pline("Wait!  There's a hidden monster there!");*/
			    pline("待て！怪物が隠れている！");
			else if ((obj = level.objects[mtmp->mx][mtmp->my]) != 0)
/*JP			    pline("Wait!  There's %s hiding under %s!",
					an(l_monnam(mtmp)), doname(obj));*/
			    pline("待て！%sの下に%sが隠れている！",
					doname(obj), an(l_monnam(mtmp)));
			mtmp->msleep = 0;
			mtmp->data->mflags3 &= ~M3_WAITMASK;
			return(TRUE);
		}
	}

	if (flags.confirm && mtmp->mpeaceful
	    && !Confusion && !Hallucination && !Stunned) {
		/* Intelligent chaotic weapons (Stormbringer) want blood */
		if (uwep && uwep->oartifact == ART_STORMBRINGER)
			return(FALSE);

		if (canspotmon(mtmp)) {
/*JP			Sprintf(qbuf, "Really attack %s?", mon_nam(mtmp));*/
			Sprintf(qbuf, "本当に%sを攻撃するの？", mon_nam(mtmp));
			if (yn(qbuf) != 'y') {
				flags.move = 0;
				mtmp->data->mflags3 &= ~M3_WAITMASK;
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
	struct permonst *mdat = mtmp->data;

	tmp = 1 + Luck + abon() +
		find_mac(mtmp) +
#ifdef POLYSELF
		((u.umonnum >= 0) ? uasmon->mlevel : u.ulevel);
#else
		u.ulevel;
#endif

/*	it is unchivalrous to attack the defenseless or from behind */
	if (pl_character[0] == 'K' && u.ualign.type == A_LAWFUL &&
	    (!mtmp->mcanmove || mtmp->msleep || mtmp->mflee) &&
	    u.ualign.record > -10) adjalign(-1);

/*	attacking peaceful creatures is bad for the samurai's giri */
	if (pl_character[0] == 'S' && mtmp->mpeaceful &&
	    u.ualign.record > -10) adjalign(-1);

/*	Adjust vs. (and possibly modify) monster state.		*/

	if(mtmp->mstun) tmp += 2;
	if(mtmp->mflee) tmp += 2;

	if(mtmp->msleep) {
		mtmp->msleep = 0;
		tmp += 2;
	}
	if(!mtmp->mcanmove) {
		tmp += 4;
		if(!rn2(10)) {
			mtmp->mcanmove = 1;
			mtmp->mfrozen = 0;
		}
	}
	if (is_orc(mtmp->data) && pl_character[0]=='E') tmp++;

/*	with a lot of luggage, your agility diminishes */
	if ((tmp2 = near_capacity()) != 0) tmp -= (tmp2*2) - 1;
	if(u.utrap) tmp -= 3;
#ifdef POLYSELF
/*	Some monsters have a combination of weapon attacks and non-weapon
 *	attacks.  It is therefore wrong to add hitval to tmp; we must add it
 *	only for the specific attack (in hmonas()).
 */
	if(uwep && u.umonnum == -1) tmp += hitval(uwep, mdat);
#else
	if(uwep) tmp += hitval(uwep, mdat);
#endif
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
	if (is_safepet(mtmp) &&
	    (!uwep || uwep->oartifact != ART_STORMBRINGER)) {
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
#ifdef POLYSELF
			|| (IS_ROCK(levl[u.ux][u.uy].typ) &&
					!passes_walls(mtmp->data))
#endif
			) {
		    mtmp->mflee = 1;
		    mtmp->mfleetim = rnd(6);
/*JP		    You("stop.  %s is in your way!",*/
		    You("止まった．%sが道にいる！",
			(mtmp->mnamelth ? NAME(mtmp) : Monnam(mtmp)));
		    return(TRUE);
		} else if ((mtmp->mfrozen || (! mtmp->mcanmove)
				|| (mtmp->data->mmove == 0)) && rn2(6)) {
/*JP		    pline("%s doesn't seem to move!", Monnam(mtmp));*/
		    pline("%sは動けないようだ．", Monnam(mtmp));
		    return(TRUE);
		} else return(FALSE);
	}

	/* moved code to a separate function to share with dokick */
	if(special_case(mtmp)) return(TRUE);

#ifdef POLYSELF
	if(u.umonnum >= 0) {	/* certain "pacifist" monsters don't attack */
		set_uasmon();
		if(noattacks(uasmon)) {
/*JP			You("have no way to attack monsters physically.");*/
			You("物理的に怪物を攻撃するすべがない．");
			mtmp->data->mflags3 &= ~M3_WAITMASK;
			return(TRUE);
		}
	}
#endif

/*JP	if(check_capacity("You cannot fight while so heavily loaded."))*/
	if(check_capacity("あなたは物を沢山持ちすぎて戦えない．"))
	    return (TRUE);

	if(unweapon) {
	    unweapon=FALSE;
	    if(flags.verbose)
		if(uwep)
/*JP		    You("begin bashing monsters with your %s.",
			aobjnam(uwep, NULL));*/
		    You("%sで怪物をなぐりつけた．",
			xname(uwep));
		else
#ifdef POLYSELF
		    if (!cantwield(uasmon))
#endif
/*JP		    You("begin bashing monsters with your %s hands.",
			uarmg ? "gloved" : "bare");		* Del Lamb */
		    You("%sで怪物をなぐりつけた．",
			uarmg ? "グローブ" : "素手");
	}
	exercise(A_STR, TRUE);		/* you're exercising muscles */
	/* andrew@orca: prevent unlimited pick-axe attacks */
	u_wipe_engr(3);

	if(mdat->mlet == S_LEPRECHAUN && mtmp->mfrozen && !mtmp->msleep &&
	   !mtmp->mconf && mtmp->mcansee && !rn2(7) &&
	   (m_move(mtmp, 0) == 2 ||			    /* it died */
	   mtmp->mx != u.ux+u.dx || mtmp->my != u.uy+u.dy)) /* it moved */
		return(FALSE);

	tmp = find_roll_to_hit(mtmp);
#ifdef POLYSELF
	if (u.umonnum >= 0) (void) hmonas(mtmp, tmp);
	else
#endif
	    (void) hitum(mtmp, tmp);

	mtmp->data->mflags3 &= ~M3_WAITMASK;
	return(TRUE);
}

static boolean
known_hitum(mon, mhit)	/* returns TRUE if monster still lives */
/* Made into a separate function because in some cases we want to know
 * in the calling function whether we hit.
 */
register struct monst *mon;
register int mhit;
{
	register boolean malive = TRUE, special;

	/* we need to know whether the special monster was peaceful */
	/* before the attack, to save idle calls to angry_guards()  */
	special = (mon->mpeaceful && (mon->data == &mons[PM_WATCHMAN] ||
				mon->data == &mons[PM_WATCH_CAPTAIN] ||
				      mon->ispriest || mon->isshk));

	if(!mhit) {
/*JP	    if(flags.verbose) You("miss %s.", mon_nam(mon));*/
	    if(flags.verbose) Your("%sへの攻撃ははずれた．", mon_nam(mon));
/*JP	    else			You("miss it.");*/
	    else			Your("攻撃ははずれた．");
	    if(!mon->msleep && mon->mcanmove)
		wakeup(mon);
#ifdef MUSE
	    else if (uwep && uwep->otyp == TSURUGI &&
		     MON_WEP(mon) && !rn2(20)) {
		/* 1/20 chance of shattering defender's weapon */
		struct obj *obj = MON_WEP(mon);

		obj->owornmask &= ~W_WEP;
		MON_NOWEP(mon);
		m_useup(mon, obj);
/*JP		pline("%s weapon shatters!", s_suffix(Monnam(mon)));*/
		pline("%sの武器はくだけちった！", s_suffix(Monnam(mon)));
		/* perhaps this will freak out the monster */
		if (!rn2(3)) {
		    mon->mflee = 1;
		    mon->mfleetim += rnd(20);
		}
	    }
#endif
	} else {
	    int oldhp = mon->mhp;

	    /* we hit the monster; be careful: it might die! */
	    notonhead = (mon->mx != u.ux+u.dx || mon->my != u.uy+u.dy);
	    if((malive = hmon(mon, uwep, 0)) == TRUE) {
		/* monster still alive */
		if(!rn2(25) && mon->mhp < mon->mhpmax/2) {
			mon->mflee = 1;
			if(!rn2(3)) mon->mfleetim = rnd(100);
			if(u.ustuck == mon && !u.uswallow
#ifdef POLYSELF
						&& !sticks(uasmon)
#endif
								)
				u.ustuck = 0;
		}
		/* If no damage was done (Vorpal Blade and not on head)
		 * do not cut the worm.  We lost the information long ago, so
		 * we must do this by checking the hit points.
		 */
		if (mon->wormno && mon->mhp < oldhp)
			cutworm(mon, u.ux+u.dx, u.uy+u.dy, uwep);
	    }
	    if(mon->ispriest && !rn2(2)) ghod_hitsu(mon);
	    if(special) (void) angry_guards(!flags.soundok);
	}
	return(malive);
}

static boolean
hitum(mon, tmp)		/* returns TRUE if monster still lives */
struct monst *mon;
int tmp;
{
	static NEARDATA boolean malive;
	boolean mhit = (tmp > (dieroll = rnd(20)) || u.uswallow);

	if(tmp > dieroll) exercise(A_DEX, TRUE);
	malive = known_hitum(mon, mhit);
	(void) passive(mon, mhit, malive, FALSE);
	return(malive);
}

boolean			/* general "damage monster" routine */
hmon(mon, obj, thrown)		/* return TRUE if mon still alive */
register struct monst *mon;
register struct obj *obj;
register int thrown;
{
	int tmp;
	struct permonst *mdat = mon->data;
	/* Why all these booleans?  This stuff has to be done in the
	 *      following order:
	 * 1) Know what we're attacking with, and print special hittxt for
	 *	unusual cases.
	 * 2a) Know whether we did damage (depends on 1)
	 * 2b) Know if it's poisoned (depends on 1)
	 * 2c) Know whether we get a normal damage bonus or not (depends on 1)
	 * 3a) Know what the value of the damage bonus is (depends on 2c)
	 * 3b) Know how much poison damage was taken (depends on 2b) and if the
	 *	poison instant-killed it
	 * 4) Know if it was killed (requires knowing 3a, 3b) except by instant-
	 *	kill poison
	 * 5) Print hit message (depends on 1 and 4)
	 * 6a) Print poison message (must be done after 5)
#if 0
	 * 6b) Rust weapon (must be done after 5)
#endif
	 * 7) Possibly kill monster (must be done after 6a, 6b)
	 * 8) Instant-kill from poison (can happen anywhere between 5 and 9)
	 * 9) Hands not glowing (must be done after 7 and 8)
	 * The major problem is that since we don't want a "hit" message
	 * when the monster dies, we have to know how much damage it did
	 * _before_ outputting a hit message, but any messages associated with
	 * the damage don't come out until _after_ outputting a hit message.
	 */
	boolean hittxt = FALSE, destroyed = FALSE;
	boolean get_dmg_bonus = TRUE;
	boolean ispoisoned = FALSE, needpoismsg = FALSE, poiskilled = FALSE;
	boolean silvermsg = FALSE;
	
	wakeup(mon);
	if(!obj) {	/* attack with bare hands */
	    if (mdat == &mons[PM_SHADE])
		tmp = 0;
	    else
		tmp = rnd(2);
#if 0
	    if(mdat == &mons[PM_COCKATRICE] && !uarmg
#ifdef POLYSELF
		&& !resists_ston(uasmon)
#endif
		) {

/*JP		You("hit %s with your bare %s.",*/
		You("%sで%sを攻撃した．",
			 makeplural(humanoid(uasmon)?"素手":body_part(HAND),mon_nam(mon)));
# ifdef POLYSELF
		if(poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM))
		    return TRUE;
# endif
/*JP		You("turn to stone...");*/
		You("石になった．．．");
		done_in_by(mon);
		hittxt = TRUE; /* maybe lifesaved */
	    }
#endif
	} else {
	    if(obj->oclass == WEAPON_CLASS || obj->otyp == PICK_AXE ||
	       obj->otyp == UNICORN_HORN || obj->oclass == ROCK_CLASS) {

		/* If not a melee weapon, and either not thrown, or thrown */
		/* and a bow (bows are >= BOW), or thrown and a missile */
		/* without a propellor (which means <DART), do 1-2 points */
		if((obj->otyp >= BOW || obj->otyp < BOOMERANG)
			&& obj->otyp != PICK_AXE && obj->otyp != UNICORN_HORN
			&& (!thrown ||
			    (obj->oclass != ROCK_CLASS &&
			    (obj->otyp >= BOW ||
				(obj->otyp < DART &&
				    (!uwep ||
				    objects[obj->otyp].w_propellor !=
				    -objects[uwep->otyp].w_propellor)
				))))) {
		    if (mdat == &mons[PM_SHADE] && obj->otyp != SILVER_ARROW)
			tmp = 0;
		    else
			tmp = rnd(2);
		} else {
		    tmp = dmgval(obj, mdat);
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
		    } else if(thrown &&
			      (obj->otyp >= ARROW && obj->otyp <= SHURIKEN)) {
			if(uwep && obj->otyp < DART &&
			   objects[obj->otyp].w_propellor ==
			   -objects[uwep->otyp].w_propellor) {
			    /* Elves and Samurai do extra damage using
			     * their bows&arrows; they're highly trained.
			     */
			    if (pl_character[0] == 'S' &&
				obj->otyp == YA && uwep->otyp == YUMI)
				tmp++;
			    else if (pl_character[0] == 'E' &&
				     obj->otyp == ELVEN_ARROW &&
				     uwep->otyp == ELVEN_BOW)
				tmp++;
			}
			if(((uwep && objects[obj->otyp].w_propellor ==
				-objects[uwep->otyp].w_propellor)
				|| obj->otyp==DART || obj->otyp==SHURIKEN) &&
				obj->opoisoned)
			    ispoisoned = TRUE;
		    }
		}
	    } else if(obj->oclass == POTION_CLASS) {
			if (obj->quan > 1L) setuwep(splitobj(obj, 1L));
			else setuwep((struct obj *)0);
			freeinv(obj);
			potionhit(mon,obj);
			hittxt = TRUE;
			if (mdat == &mons[PM_SHADE])
			    tmp = 0;
			else
			    tmp = 1;
	    } else {
		switch(obj->otyp) {
		    case HEAVY_IRON_BALL:
			tmp = rnd(25); break;
		    case BOULDER:
			tmp = rnd(20); break;
		    case MIRROR:
/*JP			You("break your mirror.  That's bad luck!");*/
			You("鏡を壊してしまった．こりゃまいった！");
			change_luck(-2);
			useup(obj);
			obj = (struct obj *) 0;
			hittxt = TRUE;
			tmp = 1;
			break;
#ifdef TOURIST
		    case EXPENSIVE_CAMERA:
/*JP	You("succeed in destroying your camera.  Congratulations!");*/
	You("カメラを壊すことができた．おめでとう！");
			useup(obj);
			return(TRUE);
#endif
		    case CORPSE:		/* fixed by polder@cs.vu.nl */
			if(obj->corpsenm == PM_COCKATRICE) {
/*JP			    You("hit %s with the cockatrice corpse.",*/
			    You("%sをコカトリスの死体で攻撃した．",
				  mon_nam(mon));
			    if(resists_ston(mdat)) {
				tmp = 1;
				hittxt = TRUE;
				break;
			    }
			    if(poly_when_stoned(mdat)) {
				mon_to_stone(mon);
				tmp = 1;
				hittxt = TRUE;
				break;
			    }
/*JP			    pline("%s turns to stone.", Monnam(mon));*/
			    pline("%sは石になった．", Monnam(mon));
			    stoned = TRUE;
			    xkilled(mon,0);
			    return(FALSE);
			}
			tmp = mons[obj->corpsenm].msize + 1;
			break;
		    case EGG: /* only possible if hand-to-hand */
			if(obj->corpsenm > -1
					&& obj->corpsenm != PM_COCKATRICE
					&& mdat == &mons[PM_COCKATRICE]) {
/*JP				You("hit %s with the %s egg%s.",*/
				You("%sに%sの卵を投げつけた．",
					mon_nam(mon),
					jtrns_mon(mons[obj->corpsenm].mname));
				hittxt = TRUE;
/*JP				pline("The egg%sn't live any more...",
					(obj->quan == 1L) ? " is" : "s are");*/
				pline("もう卵が孵化することはないだろう．．．");
				obj->otyp = ROCK;
				obj->oclass = GEM_CLASS;
				obj->known = obj->dknown = 0;
				obj->owt = weight(obj);
			}
			tmp = 1;
			break;
		    case CLOVE_OF_GARLIC:	/* no effect against demons */
			if(is_undead(mdat)) mon->mflee = 1;
			tmp = 1;
			break;
		    case CREAM_PIE:
#ifdef POLYSELF
		    case BLINDING_VENOM:
			if(Blind || !haseyes(mon->data))
/*JP			    pline(obj->otyp==CREAM_PIE ? "Splat!" : "Splash!");*/
			    pline(obj->otyp==CREAM_PIE ? "ピシャッ！" : "ビチャッ！");
			else if (obj->otyp == BLINDING_VENOM)
/*JP			    pline("The venom blinds %s%s!", mon_nam(mon),
					mon->mcansee ? "" : " further");*/
			    pline("毒液で%sは%s目が見えなくなった！", mon_nam(mon),
					mon->mcansee ? "" : "さらに");
#else
/*JP			if(Blind) pline("Splat!");*/
			if(Blind) pline("ピシャッ！");
#endif
			else
/*JP			    pline("The cream pie splashes over %s%s!",
				mon_nam(mon),
				(haseyes(mdat) &&
				    mdat != &mons[PM_FLOATING_EYE])
				? (*(eos(mon_nam(mon))-1) == 's' ? "' face" :
					 "'s face") : "");*/
			    pline("クリームパイは%sの顔にぶちまけられた！",
				mon_nam(mon));
			if(mon->msleep) mon->msleep = 0;
			setmangry(mon);
			if(haseyes(mon->data)) {
			    mon->mcansee = 0;
			    tmp = rn1(25, 21);
			    if(((int) mon->mblinded + tmp) > 127)
				mon->mblinded = 127;
			    else mon->mblinded += tmp;
			}
			hittxt = TRUE;
			get_dmg_bonus = FALSE;
			tmp = 0;
			break;
#ifdef POLYSELF
		    case ACID_VENOM: /* only possible if thrown */
			if(resists_acid(mdat)) {
/*JP				Your("venom hits %s harmlessly.",*/
				Your("毒液は%sには効果がなかった．",
					mon_nam(mon));
				tmp = 0;
			} else {
/*JP				Your("venom burns %s!", mon_nam(mon));*/
				Your("毒液は%sを焼いた！", mon_nam(mon));
				tmp = dmgval(obj, mdat);
			}
			hittxt = TRUE;
			get_dmg_bonus = FALSE;
			break;
#endif
		    default:
			/* non-weapons can damage because of their weight */
			/* (but not too much) */
			tmp = obj->owt/100;
			if(tmp < 1) tmp = 1;
			else tmp = rnd(tmp);
			if(tmp > 6) tmp = 6;
		}
		if (mdat == &mons[PM_SHADE] && obj &&
				objects[obj->otyp].oc_material != SILVER)
		    tmp = 0;
	    }
	}

	/****** NOTE: perhaps obj is undefined!! (if !thrown && BOOMERANG)
	 *      *OR* if attacking bare-handed!! */

	if (get_dmg_bonus && tmp) {
		tmp += u.udaminc;
		/* If you throw using a propellor, you don't get a strength
		 * bonus but you do get an increase-damage bonus.
		 */
		if(!thrown || !obj || !uwep ||
		   (obj->oclass != GEM_CLASS && obj->oclass != WEAPON_CLASS) ||
		   !objects[obj->otyp].w_propellor ||
		   (objects[obj->otyp].w_propellor !=
				-objects[uwep->otyp].w_propellor))
		    tmp += dbon();
	}

/* TODO:	Fix this up.  multiple engulf attacks now exist.
	if(u.uswallow) {
	    if((tmp -= u.uswldtim) <= 0) {
		Your("%s are no longer able to hit.",
			makeplural(body_part(ARM)));
		return(TRUE);
	    }
	}
 */
	if (ispoisoned) {
	    if(resists_poison(mdat))
		needpoismsg = TRUE;
	    else if (rn2(10))
		tmp += rnd(6);
	    else poiskilled = TRUE;
	}
	if(tmp < 1)
	    if (mdat == &mons[PM_SHADE]) {
/*JP		Your("attack passes harmlessly through %s.",*/
		Your("攻撃は%sに効果がなかった．",
			mon_nam(mon));
		hittxt = TRUE;
	    } else
		tmp = 1;

	mon->mhp -= tmp;
	if(mon->mhp < 1)
		destroyed = TRUE;
	if(mon->mtame && (!mon->mflee || mon->mfleetim)) {
		abuse_dog(mon);
		mon->mflee = TRUE;		/* Rick Richardson */
		mon->mfleetim += 10*rnd(tmp);
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
		else if(!flags.verbose) Your("攻撃は命中した！");
/*JP		else	You("hit %s%s", mon_nam(mon), canseemon(mon)
			? exclam(tmp) : ".");*/
		else	Your("%sへの攻撃は命中した%s", mon_nam(mon), canseemon(mon)
			? exclam(tmp) : "．");
	}

	if (silvermsg) {
		if (canseemon(mon) || sensemon(mon))
/*JP			pline("The silver sears %s%s!",
				mon_nam(mon),
				noncorporeal(mdat) ? "" : 
			          (*(eos(mon_nam(mon))-1) == 's' ?
				       "' flesh" : "'s flesh"));*/
			pline("銀は%sの体を焼いた！",
				mon_nam(mon));
		else
/*JP			pline("It%s is seared!",
				noncorporeal(mdat) ? "" : "s flesh");*/
			pline("何者かの体は焼かれた！");
	}

	if (needpoismsg)
/*JP		pline("The poison doesn't seem to affect %s.", mon_nam(mon));*/
		pline("毒は%sに効かなかったようだ．", mon_nam(mon));
	if (poiskilled) {
/*JP		pline("The poison was deadly...");*/
		pline("毒は致死量だった．．．");
		xkilled(mon, 0);
		return FALSE;
	} else if (destroyed) {
		killed(mon);	/* takes care of most messages */
	} else if(u.umconf && !thrown) {
		nohandglow(mon);
		if(!mon->mconf && !resist(mon, '+', 0, NOTELL)) {
			mon->mconf = 1;
			if(!mon->mstun && mon->mcanmove && !mon->msleep &&
			   !Blind)
/*JP				pline("%s appears confused.", Monnam(mon));*/
				pline("%sは混乱しているようだ．", Monnam(mon));
		}
	}
#if 0
	if(mdat == &mons[PM_RUST_MONSTER] && obj && obj == uwep &&
		is_rustprone(obj) && obj->oeroded < MAX_ERODE) {
	    if (obj->greased)
		grease_protect(obj,NULL,FALSE);
	    else if (obj->oerodeproof || (obj->blessed && !rnl(4))) {
	        if (flags.verbose)
/*JP			pline("Somehow, your %s is not affected.",
			      is_sword(obj) ? "sword" : "weapon");*/
			pline("どうも，あなたの%sには影響がなかったようだ．",
			      is_sword(obj) ? "剣" : "武器");
	    } else {
/*JP		Your("%s%s!", aobjnam(obj, "rust"),
		     obj->oeroded+1 == MAX_ERODE ? " completely" :
		     obj->oeroded ? " further" : "");*/
	        Your("%sは%s錆びた！",xname(obj),
		     obj->oeroded+1 == MAX_ERODE ? "完全に" :
		     obj->oeroded ? "さらに" : "");
		obj->oeroded++;
	    }
	}
#endif

	return((boolean)(destroyed ? FALSE : TRUE));
}
#ifdef POLYSELF

int
damageum(mdef, mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register struct permonst *pd = mdef->data;
	register int	tmp = d((int)mattk->damn, (int)mattk->damd);

	if (is_demon(uasmon) && !rn2(13) && !uwep
		&& u.umonnum != PM_SUCCUBUS && u.umonnum != PM_INCUBUS
		&& u.umonnum != PM_BALROG) {
	    struct monst *dtmp;
/*JP	    pline("Some hell-p has arrived!");*/
	    pline("地獄の仲間が現われた！");
	    if((dtmp = makemon(!rn2(6) ? &mons[ndemon(u.ualign.type)] :
					 uasmon, u.ux, u.uy)))
		(void)tamedog(dtmp, (struct obj *)0);
	    exercise(A_WIS, TRUE);
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
		} else if(mattk->aatyp == AT_KICK)
			if(thick_skinned(mdef->data)) tmp = 0;
		break;
	    case AD_FIRE:
/*JP		if(!Blind) pline("%s is on fire!", Monnam(mdef));*/
		if(!Blind) pline("%sは火だるまになった！", Monnam(mdef));
		tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
		tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
		if(resists_fire(pd)) {
		    if (!Blind)
/*JP			pline("The fire doesn't heat %s!", mon_nam(mdef));*/
			pline("炎は%sに影響がない！", mon_nam(mdef));
		    golemeffects(mdef, AD_FIRE, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp = 0;
		}
		/* only potions damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
		break;
	    case AD_COLD:
/*JP		if(!Blind) pline("%s is covered in frost!", Monnam(mdef));*/
		if(!Blind) pline("%sは氷で覆われた！", Monnam(mdef));
		if(resists_cold(pd)) {
		    shieldeff(mdef->mx, mdef->my);
		    if (!Blind)
/*JP			pline("The frost doesn't chill %s!", mon_nam(mdef));*/
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
		if(resists_elec(pd)) {
		    if (!Blind)
/*JP			pline("The zap doesn't shock %s!", mon_nam(mdef));*/
			pline("衝撃は%sに影響を与えない！", mon_nam(mdef));
		    golemeffects(mdef, AD_ELEC, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp = 0;
		}
		/* only rings damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
		break;
	    case AD_ACID:
		if(resists_acid(pd)) tmp = 0;
		break;
	    case AD_STON:
		if(poly_when_stoned(pd))
		   mon_to_stone(mdef);
		else if(!resists_ston(pd)) {
		    stoned = TRUE;
/*JP		    if(!Blind) pline("%s turns to stone.", Monnam(mdef));*/
		    if(!Blind) pline("%sは石になった．", Monnam(mdef));
		    xkilled(mdef, 0);
		    return(2);
		}
		tmp = 0;	/* no damage if this fails */
		break;
# ifdef SEDUCE
	    case AD_SSEX:
# endif
	    case AD_SEDU:
	    case AD_SITM:
		if(mdef->minvent) {
		    struct obj *otmp, *stealoid;

		    stealoid = (struct obj *)0;
		/* Without MUSE we can only change a monster's AC by stealing
		 * armor with the "unarmored soldier" kludge.  With it there
		 * are many monsters which wear armor, and all can be stripped.
		 */
		    if(
#ifndef MUSE
			is_mercenary(pd) &&
#endif
					could_seduce(&youmonst,mdef,mattk)){
			for(otmp = mdef->minvent; otmp; otmp=otmp->nobj)
#ifdef MUSE
			    if (otmp->owornmask & W_ARM) stealoid = otmp;
#else
			    if (otmp->otyp >= PLATE_MAIL && otmp->otyp
				<= ELVEN_CLOAK) stealoid = otmp;
#endif
		    }
		    if (stealoid) {
			boolean stolen = FALSE;
			if (gender(mdef) == u.mfemale &&
						uasmon->mlet == S_NYMPH)
/*JP	You("charm %s.  She gladly hands over her possessions.", mon_nam(mdef));*/
	You("%sをうっとりさせた．彼女はそっと持ち物をさしだした．", mon_nam(mdef));
			else
/*JP			You("seduce %s and %s starts to take off %s clothes.",*/
			You("%sを誘惑した．%sは%sの服を脱ぎはじめた．",
			    mon_nam(mdef), he[pronoun_gender(mdef)],
			    his[pronoun_gender(mdef)]);
			while(mdef->minvent) {
				otmp = mdef->minvent;
				mdef->minvent = otmp->nobj;
				/* set dknown to insure proper merge */
				if (!Blind) otmp->dknown = 1;
#ifdef MUSE
				otmp->owornmask = 0L;
#endif
				if (!stolen && otmp==stealoid) {
				    otmp = hold_another_object(otmp,
					      (const char *)0, (const char *)0,
							      (const char *)0);
				    stealoid = otmp;
				    stolen = TRUE;
				} else {
				    otmp = hold_another_object(otmp,
/*JP						 "You steal %s.", doname(otmp),
								"You steal: ");*/
						 "あなたは%sを盗んだ．", doname(otmp),
								"を盗んだ：");
				}
			}
			if (!stolen)
/*JP				impossible("Player steal fails!");*/
				impossible("盗みは失敗した！");
			else {
/*JP				pline("%s finishes taking off %s suit.",
				      Monnam(mdef), his[pronoun_gender(mdef)]);*/
				pline("%sは服を脱ぎ終えた",
				      Monnam(mdef));
/*JP				You("steal %s!", doname(stealoid));*/
				You("%sを盗んだ！", doname(stealoid));
# if defined(ARMY) && !defined(MUSE)
				mdef->data = &mons[PM_UNARMORED_SOLDIER];
# endif
			}
#ifdef MUSE
			possibly_unwield(mdef);
			mdef->misc_worn_check = 0L;
#endif
		   } else {
			otmp = mdef->minvent;
			mdef->minvent = otmp->nobj;
/*JP			otmp = hold_another_object(otmp, "You steal %s.",
						  doname(otmp), "You steal: ");*/
			otmp = hold_another_object(otmp, "あなたは%sを盗んだ．",
						  doname(otmp), "盗んだ：");
#ifdef MUSE
			possibly_unwield(mdef);
			otmp->owornmask = 0L;
			mselftouch(mdef, (const char *)0, TRUE);
			if (mdef->mhp <= 0) {
				tmp = 1; /* avoid early return from damageum */
				break;
			}
#endif
		   }
		}
		tmp = 0;
		break;
	    case AD_SGLD:
		if (mdef->mgold) {
		    u.ugold += mdef->mgold;
		    mdef->mgold = 0;
/*JP		    Your("purse feels heavier.");*/
		    Your("財布が重くなったような気がする．");
		}
		exercise(A_DEX, TRUE);
		tmp = 0;
		break;
	    case AD_TLPT:
		if(tmp <= 0) tmp = 1;
		if(tmp < mdef->mhp) {
		    rloc(mdef);
/*JP		    if(!Blind) pline("%s suddenly disappears!", Monnam(mdef));*/
		    if(!Blind) pline("%sは突然消えた！", Monnam(mdef));
		}
		break;
	    case AD_BLND:
		if(haseyes(pd)) {

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
			    pline("何かの文字が%sの頭から消えた！",
				s_suffix(mon_nam(mdef)));
			xkilled(mdef, 0);
			return 2;
		    }
		    mdef->mcan = 1;
/*JP		    You("chuckle.");*/
		    You("くすくす笑った．");
		}
		tmp = 0;
		break;
	    case AD_DRLI:
		if(rn2(2) && !resists_drli(pd)) {
			int xtmp = d(2,6);
/*JP			pline("%s suddenly seems weaker!", Monnam(mdef));*/
			pline("%sは突然弱くなったように見えた！", Monnam(mdef));
			mdef->mhpmax -= xtmp;
			if ((mdef->mhp -= xtmp) <= 0 || !mdef->m_lev) {
/*JP				pline("%s dies!", Monnam(mdef));*/
				pline("%sは死んだ！", Monnam(mdef));
				xkilled(mdef,0);
				return(2);
			}
			mdef->m_lev--;
		}
		tmp = 0;
		break;
	    case AD_RUST:
		if (pd == &mons[PM_IRON_GOLEM]) {
/*JP			pline("%s falls to pieces!", Monnam(mdef));*/
			pline("%sはバラバラになった！", Monnam(mdef));
			xkilled(mdef,0);
			return(2);
		}
		tmp = 0;
		break;
	    case AD_DCAY:
		if (pd == &mons[PM_WOOD_GOLEM] ||
		    pd == &mons[PM_LEATHER_GOLEM]) {
/*JP			pline("%s falls to pieces!", Monnam(mdef));*/
			pline("%sはバラバラになった！", Monnam(mdef));
			xkilled(mdef,0);
			return(2);
		}
	    case AD_DRST:
	    case AD_DRDX:
	    case AD_DRCO:
		if (!rn2(8)) {
/*JP		    Your("%s was poisoned!", mattk->aatyp==AT_BITE ?
			"bite" : "sting");*/
		    Your("%sは毒されている！", mattk->aatyp==AT_BITE ?
			"歯" : "針");
		    if (resists_poison(mdef->data))
/*JP			pline("The poison doesn't seem to affect %s.",*/
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
		if (!has_head(mdef->data)) {
/*JP		    pline("%s doesn't seem harmed.", Monnam(mdef));*/
		    pline("%sは傷ついたようには見えない．", Monnam(mdef));
		    tmp = 0;
		    break;
		}
#ifdef MUSE
		if ((mdef->misc_worn_check & W_ARMH) && rn2(8)) {
/*JP		    pline("%s helmet blocks your attack to %s head.",
			  s_suffix(Monnam(mdef)), his[pronoun_gender(mdef)]);*/
		    pline("%sの兜は頭への攻撃を防いだ．",
			  s_suffix(Monnam(mdef)));
		    break;
		}
#endif
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
	    case AD_WRAP:
	    case AD_STCK:
		if (!sticks(mdef->data))
		    u.ustuck = mdef; /* it's now stuck to you */
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
		if (!resists_sleep(mdef->data) && !mdef->msleep &&
							mdef->mcanmove) {
		    if (!Blind)
/*JP			pline("%s suddenly falls asleep!", Monnam(mdef));*/
			pline("%sは突然眠りにおちた！", Monnam(mdef));
		    mdef->mcanmove = 0;
		    mdef->mfrozen = rnd(10);
		}
		break;
	    default:	tmp = 0;
			break;
	}
	if(!tmp) return(1);

	if((mdef->mhp -= tmp) < 1) {

	    if (mdef->mtame && !cansee(mdef->mx,mdef->my)) {
/*JP		You("feel embarrassed for a moment.");*/
		You("恥ずかしい思いをした．");
		xkilled(mdef, 0);
	    } else if (!flags.verbose) {
/*JP		You("destroy it!");*/
		You("倒した！");
		xkilled(mdef, 0);
	    } else
		killed(mdef);
	    return(2);
	}
	return(1);
}

static int
explum(mdef, mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register int tmp = d((int)mattk->damn, (int)mattk->damd);

/*JP	You("explode!");*/
	You("爆発した！");
	switch(mattk->adtyp) {
	    case AD_BLND:
		if (haseyes(mdef->data)) {
/*JP		    pline("%s is blinded by your flash of light!", Monnam(mdef));*/
		    pline("%sはまばゆい光で目がくらんだ", Monnam(mdef));
		    if (mdef->mcansee) {
			mdef->mblinded += tmp;
			mdef->mcansee = 0;
		    }
		}
		break;
	    case AD_COLD:
		if (!resists_cold(mdef->data)) {
/*JP		    pline("%s gets blasted!", Monnam(mdef));*/
		    pline("%sは氷の風をうけた！", Monnam(mdef));
		    mdef->mhp -= tmp;
		    if (mdef->mhp <= 0) {
			 killed(mdef);
			 return(2);
		    }
		} else {
		    shieldeff(mdef->mx, mdef->my);
		    if (is_golem(mdef->data))
			golemeffects(mdef, AD_COLD, tmp);
		    else
/*JP			pline("The blast doesn't seem to affect %s.",*/
			pline("氷の風は%sに影響を与えなかったようだ．",
				mon_nam(mdef));
		}
		break;
	    default:
		break;
	}
	return(1);
}

static int
gulpum(mdef,mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register int tmp;
	register int dam = d((int)mattk->damn, (int)mattk->damd);
	/* Not totally the same as for real monsters.  Specifically, these
	 * don't take multiple moves.  (It's just too hard, for too little
	 * result, to program monsters which attack from inside you, which
	 * would be necessary if done accurately.)  Instead, we arbitrarily
	 * kill the monster immediately for AD_DGST and we regurgitate them
	 * after exactly 1 round of attack otherwise.  -KAA
	 */

	if(mdef->data->msize >= MZ_HUGE) return 0;

	if(u.uhunger < 1500 && !u.uswallow) {

	    if(mdef->data->mlet != S_COCKATRICE) {
# ifdef LINT	/* static char msgbuf[BUFSZ]; */
		char msgbuf[BUFSZ];
# else
		static char msgbuf[BUFSZ];
# endif
/* TODO: get the symbol display also to work (monster symbol is removed from
 * the screen and you moved onto it, then you get moved back and it gets
 * moved back if the monster survives--just like when monsters swallow you.
 */
/*JP		You("engulf %s!", mon_nam(mdef));*/
		You("%sを飲み込んだ！", mon_nam(mdef));
		switch(mattk->adtyp) {
		    case AD_DGST:
			u.uhunger += mdef->data->cnutrit;
			newuhs(FALSE);
			xkilled(mdef,2);
/*JP			Sprintf(msgbuf, "You totally digest %s.",*/
			Sprintf(msgbuf, "あなたは%sを完全に消化した．",
					mon_nam(mdef));
			if ((tmp = 3 + (mdef->data->cwt >> 6)) != 0) {
/*JP			    You("digest %s.", mon_nam(mdef));*/
			    You("%sを消化している．", mon_nam(mdef));
			    nomul(-tmp);
			    nomovemsg = msgbuf;
			} else pline(msgbuf);
			exercise(A_CON, TRUE);
			return(2);
		    case AD_PHYS:
/*JP			pline("%s is pummeled with your debris!",Monnam(mdef));*/
			pline("%sは瓦礫で痛めつけられた！",Monnam(mdef));
			break;
		    case AD_ACID:
/*JP			pline("%s is covered with your goo!", Monnam(mdef));*/
			pline("%sはねばつくものでおわれた！", Monnam(mdef));
			if (resists_acid(mdef->data)) {
/*JP			    pline("It seems harmless to %s.", mon_nam(mdef));*/
			    pline("が，%sはなんともない．", mon_nam(mdef));
			    dam = 0;
			}
			break;
		    case AD_BLND:
			if(haseyes(mdef->data)) {
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
/*JP			    pline("The air around %s crackles with electricity.", mon_nam(mdef));*/
			    pline("%sの回りの空気は静電気でピリピリしている．", mon_nam(mdef));
			    if (resists_elec(mdef->data)) {
/*JP				pline("%s seems unhurt.", Monnam(mdef));*/
				pline("が，%sは平気なようだ．", Monnam(mdef));
				dam = 0;
			    }
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		    case AD_COLD:
			if (rn2(2)) {
			    if (resists_cold(mdef->data)) {
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
			    if (resists_fire(mdef->data)) {
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
		if ((mdef->mhp -= dam) <= 0) {
		    killed(mdef);
		    return(2);
		}
/*JP		You("%s %s!", is_animal(uasmon) ? "regurgitate"
			: "expel", mon_nam(mdef));*/
		You("%sを%s！", mon_nam(mdef),
                     is_animal(uasmon) ? "吐き戻した" : "吐出した");
		if (is_animal(uasmon)) {
/*JP		    pline("Obviously, you didn't like %s taste.",*/
		    pline("どうも%sの味は好きになれない．",
			  s_suffix(mon_nam(mdef)));
		}
	    } else {
/*JP		You("bite into %s", mon_nam(mdef));*/
		You("%sに噛みついた", mon_nam(mdef));
/*JP		You("turn to stone...");*/
		You("石になった．．．");
		killer_format = KILLED_BY;
/*JP		killer = "swallowing a cockatrice whole";*/
		killer = "コカトリスをまるまる飲みこんで";
		done(STONING);
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
	else if(!Blind && flags.verbose)
/*JP		You("miss %s.", mon_nam(mdef));*/
		Your("%sへの攻撃は外れた．", mon_nam(mdef));
	else
/*JP		You("miss it.");*/
		Your("何者かへの攻撃は外れた．");
	wakeup(mdef);
}

static boolean
hmonas(mon, tmp)		/* attack monster as a monster. */
register struct monst *mon;
register int tmp;
{
	register struct attack *mattk;
	int	i, sum[NATTK];
	int	nsum = 0;
	schar	dhit;

#ifdef GCC_WARN
	dhit = 0;
#endif

	for(i = 0; i < NATTK; i++) {

	    sum[i] = 0;
	    mattk = &(uasmon->mattk[i]);
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
			if(uwep) tmp += hitval(uwep, mon->data);
			dhit = (tmp > (dieroll = rnd(20)) || u.uswallow);
			/* Enemy dead, before any special abilities used */
			if (!known_hitum(mon,dhit)) return 0;
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
			if (i==0 && uwep && !cantwield(uasmon)) goto use_weapon;
# ifdef SEDUCE
			/* succubi/incubi are humanoid, but their _second_
			 * attack is AT_CLAW, not their first...
			 */
			if (i==1 && uwep && (u.umonnum == PM_SUCCUBUS ||
				u.umonnum == PM_INCUBUS)) goto use_weapon;
# endif
		case AT_KICK:
		case AT_BITE:
		case AT_STNG:
		case AT_TUCH:
		case AT_BUTT:
		case AT_TENT:
			if (i==0 && uwep && (u.usym==S_LICH)) goto use_weapon;
			if ((dhit = (tmp > rnd(20) || u.uswallow)) != 0) {
			    int compat;

			    if (!u.uswallow &&
				(compat=could_seduce(&youmonst, mon, mattk))) {
/*JP				You("%s %s %s.",
				    mon->mcansee && haseyes(mon->data)
				    ? "smile at" : "talk to",
				    mon_nam(mon),
				    compat == 2 ? "engagingly":"seductively");*/
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
			    if (mon->data == &mons[PM_SHADE]) {
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
			else if (!sticks(mon->data) && !u.uswallow)
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
			if (i==0 && (u.usym==S_KOBOLD
				|| u.usym==S_ORC
				|| u.usym==S_GNOME
				)) goto use_weapon;

		case AT_NONE:
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
	    if(sum[i] == 2) return((boolean)passive(mon, 1, 0, (mattk->aatyp==AT_KICK)));
							/* defender dead */
	    else {
		(void) passive(mon, sum[i], 1, (mattk->aatyp==AT_KICK));
		nsum |= sum[i];
	    }
	    if (uasmon == &playermon)
		break; /* No extra attacks if no longer a monster */
	    if (multi < 0)
		break; /* If paralyzed while attacking, i.e. floating eye */
	}
	return((boolean)(nsum != 0));
}

#endif /* POLYSELF */

/*	Special (passive) attacks on you by monsters done here.		*/

int
passive(mon, mhit, malive, kicked)
register struct monst *mon;
register boolean mhit;
register int malive;
boolean kicked;
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

#ifdef POLYSELF
		if(!resists_acid(uasmon))
#endif
			mdamageu(mon, tmp);
		if(!rn2(30)) erode_armor(TRUE);
	    }
	    if(mhit && !rn2(6)) {
		if (kicked) {
		    if (uarmf)
			(void) rust_dmg(uarmf, xname(uarmf), 3, TRUE);
		} else erode_weapon(TRUE);
	    }
	    exercise(A_STR, FALSE);
	    break;
	  case AD_STON:
	    if(mhit)
	      if (!kicked)
		if (!uwep && !uarmg
#ifdef POLYSELF
		    && !resists_ston(uasmon)
		    && !(poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM))
#endif
		   ) {
/*JP		    You("turn to stone...");*/
		    You("石になった．．．");
		    done_in_by(mon);
		    return 2;
		}
	    break;
	  case AD_RUST:
	    if(mhit && !mon->mcan)
	      if (kicked) {
		if (uarmf)
		    (void) rust_dmg(uarmf, xname(uarmf), 1, TRUE);
	      } else
		erode_weapon(FALSE);
	    break;
	  case AD_MAGM:
	    /* wrath of gods for attacking Oracle */
	    if(Antimagic) {
		shieldeff(u.ux, u.uy);
/*JP		pline("A hail of magic missiles narrowly misses you!");*/
		pline("マジックミサイルの雨をなんとかかわした！");
	    } else {
/*JP		You("are hit by magic missiles appearing from thin air!");*/
		pline("突如空中に現われたマジックミサイルが命中した！");
		mdamageu(mon, tmp);
	    }
	    break;
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
			if(Reflecting & W_AMUL) {
			    makeknown(AMULET_OF_REFLECTION);
/*JP			    pline("%s gaze is reflected by your medallion.",*/
			    pline("%sのにらみは魔除けによって反射された．",
				  s_suffix(Monnam(mon)));
			} else if(Reflecting & W_ARMS) {
			    makeknown(SHIELD_OF_REFLECTION);
/*JP			    pline("%s gaze is reflected by your shield.",*/
			    pline("%sのにらみは盾によって反射された．",
				  s_suffix(Monnam(mon)));
			} else {
/*JP			    You("are frozen by %s gaze!", */
			    You("%sのにらみで動けなくなった．",
				  s_suffix(mon_nam(mon)));
			    nomul((ACURR(A_WIS) > 12 || rn2(4)) ? -tmp : -127);
			}
		    } else {
/*JP			pline("%s cannot defend itself.",
				Adjmonnam(mon,"blind"));*/
			pline("目の見えない%sは自分自身を守れない．",
				Monnam(mon));
			if(!rn2(500)) change_luck(-1);
		    }
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
/*JP			You("feel a mild chill.");*/
			You("寒さを感じた．");
#ifdef POLYSELF
			ugolemeffects(AD_COLD, tmp);
#endif
			break;
		    }
/*JP		    You("are suddenly very cold!");*/
		    You("突然，猛烈に寒くなった！");
		    mdamageu(mon, tmp);
		/* monster gets stronger with your heat! */
		    mon->mhp += tmp / 2;
		    if (mon->mhpmax < mon->mhp) mon->mhpmax = mon->mhp;
		/* at a certain point, the monster will reproduce! */
		    if(mon->mhpmax > ((int) (mon->m_lev+1) * 8)) {
			register struct monst *mtmp;

			if ((mtmp = clone_mon(mon)) != 0) {
			    mtmp->mhpmax = mon->mhpmax /= 2;
			    if(!Blind)
/*JP				pline("%s multiplies from your heat!",*/
				pline("%sは熱で分裂した！",
								Monnam(mon));
			}
		    }
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
#ifdef POLYSELF
			ugolemeffects(AD_FIRE, tmp);
#endif
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
#ifdef POLYSELF
		    ugolemeffects(AD_ELEC, tmp);
#endif
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

/* Note: caller must ascertain mtmp is mimicing... */
void
stumble_onto_mimic(mtmp)
register struct monst *mtmp;
{
	if(!u.ustuck && !mtmp->mflee && dmgtype(mtmp->data,AD_STCK))
	    u.ustuck = mtmp;
	if (Blind) {
	    if(!Telepat)
/*JP		pline("Wait!  That's a monster!");*/
		pline("ちょっとまった！怪物だ！");
	} else if (glyph_is_cmap(levl[u.ux+u.dx][u.uy+u.dy].glyph) &&
		(glyph_to_cmap(levl[u.ux+u.dx][u.uy+u.dy].glyph) == S_hcdoor ||
		 glyph_to_cmap(levl[u.ux+u.dx][u.uy+u.dy].glyph) == S_vcdoor))
/*JP	    pline("The door actually was %s!", a_monnam(mtmp));*/
	    pline("扉は実際には%sだった！", a_monnam(mtmp));
	else if (glyph_is_object(levl[u.ux+u.dx][u.uy+u.dy].glyph) &&
		glyph_to_obj(levl[u.ux+u.dx][u.uy+u.dy].glyph) == GOLD_PIECE)
/*JP	    pline("That gold was %s!", a_monnam(mtmp));*/
	    pline("金塊は%sだった！", a_monnam(mtmp));
	else {
/*JP	    pline("Wait!  That's %s!", a_monnam(mtmp));*/
	    pline("ちょっとまった！%sだ！", a_monnam(mtmp));
	}

	wakeup(mtmp);	/* clears mimicing */
}

static void
nohandglow(mon)
struct monst *mon;
{
	char *hands=makeplural(body_part(HAND));

	if (!u.umconf || mon->mconf) return;
	if (u.umconf == 1) {
		if (Blind)
/*JP			Your("%s stop tingling.", hands);*/
			Your("%sは痺れがとれた",hands);
		else
/*JP			Your("%s stop glowing %s.", hands,*/
			Your("%sの%s輝きはなくなった．", hands,
				Hallucination ? hcolor() : red);
	} else {
		if (Blind)
/*JP			pline("The tingling in your %s lessens.", hands);*/
			pline("%sの痺れがとれてきた．",hands);
		else
/*JP			Your("%s no longer glow so brightly %s.", hands,*/
			Your("%sの%s輝きがなくなってきた．",hands,
				Hallucination ? hcolor() : red);
	}
	u.umconf--;
}

/*uhitm.c*/
