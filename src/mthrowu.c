/*	SCCS Id: @(#)mthrowu.c	3.2	96/06/26	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

STATIC_DCL int FDECL(drop_throw,(struct obj *,BOOLEAN_P,int,int));

#define URETREATING(x,y) (distmin(u.ux,u.uy,x,y) > distmin(u.ux0,u.uy0,x,y))

#ifndef OVLB

STATIC_DCL const char *breathwep[];

#else /* OVLB */

/*
 * Keep consistent with breath weapons in zap.c, and AD_* in monattk.h.
 */
STATIC_OVL NEARDATA const char *breathwep[] = {
/*JP				"fragments",
				"fire",
				"frost",
				"sleep gas",
				"a disintegration blast",
				"lightning",
				"poison gas",
				"acid",
				"strange breath #8",
				"strange breath #9"*/
				"破片",
				"炎",
				"冷気",
				"睡眠ガス",
				"死の光線",
				"稲妻",
				"毒の息",
				"酸",
				"strange breath #8",
				"strange breath #9"
};

int
thitu(tlev, dam, obj, name)	/* u is hit by sth, but not a monster */
	register int tlev, dam;
	struct obj *obj;
	register const char *name;
{
	const char *onm = (obj && obj_is_pname(obj)) ? the(name) :
			    (obj && obj->quan > 1) ? name : an(name);
	boolean is_acid = (obj && obj->otyp == ACID_VENOM);
	static char jbuf[256];

	if(u.uac + tlev <= rnd(20)) {
/*JP		if(Blind || !flags.verbose) pline("It misses.");*/
		if(Blind || !flags.verbose) pline("それははずれた．");
/*JP		else You("are almost hit by %s!", onm);*/
		else pline("もう少しで%sに命中するところだった！",onm);
		return(0);
	} else {
/*JP		if(Blind || !flags.verbose) You("are hit!");
		else You("are hit by %s!", onm);*/
		if(Blind || !flags.verbose) pline("何かがあなたに命中した！");
		else pline("%sがあなたに命中した！", onm);

		if (obj && objects[obj->otyp].oc_material == SILVER
				&& hates_silver(uasmon)) {
			dam += rnd(20);
/*JP			pline_The("silver sears your flesh!");*/
			pline("あなたの体は銀で焼かれた！");
			exercise(A_CON, FALSE);
		}
		if (is_acid && resists_acid(&youmonst))
/*JP			pline("It doesn't seem to hurt you.");*/
			pline("あなたは傷つかなかった．");
		else {
/*JP			if (is_acid) pline("It burns!");*/
			if (is_acid) pline("酸で焼かれた！");
			if (Half_physical_damage) dam = (dam+1) / 2;
			Strcpy(jbuf, name);
			Strcat(jbuf, "で");
/*JP			losehp(dam, name, (obj && obj_is_pname(obj)) ?*/
			losehp(dam, jbuf, (obj && obj_is_pname(obj)) ?
			       KILLED_BY : KILLED_BY_AN);
			exercise(A_STR, FALSE);
		}
		return(1);
	}
}

/* Be sure this corresponds with what happens to player-thrown objects in
 * dothrow.c (for consistency). --KAA
 * Returns 0 if object still exists (not destroyed).
 */

STATIC_OVL int
drop_throw(obj, ohit, x, y)
register struct obj *obj;
boolean ohit;
int x,y;
{
	int retvalu = 1;
	int create;
	struct monst *mtmp;
	struct trap *t;

	if (obj->otyp == CREAM_PIE || obj->oclass == VENOM_CLASS ||
		    (ohit && obj->otyp == EGG))
		create = 0;
	else if (ohit &&
		 ((obj->otyp >= ARROW && obj->otyp <= SHURIKEN) ||
		  obj->otyp == ROCK))
		create = !rn2(3);
	else create = 1;

	if (create && !((mtmp = m_at(x, y)) && (mtmp->mtrapped) &&
			(t = t_at(x, y)) && ((t->ttyp == PIT) ||
			(t->ttyp == SPIKED_PIT)))) {
		int objgone = 0;

		if (down_gate(x, y) != -1)
			objgone = ship_object(obj, x, y, FALSE);
		if (!objgone) {
/*JP			if (!flooreffects(obj,x,y,"fall")) {*/ /* don't double-dip on damage */
			if (!flooreffects(obj,x,y,"落ちる")) { /* don't double-dip on damage */
			    place_object(obj, x, y);
			    stackobj(obj);
			    retvalu = 0;
			}
		}
	} else obfree(obj, (struct obj*) 0);
	return retvalu;
}

#endif /* OVLB */
#ifdef OVL1

/* an object launched by someone/thing other than player attacks a monster;
   return 1 if the object has stopped moving (hit or its range used up) */
int
ohitmon(mtmp, otmp, range, verbose)
struct monst *mtmp;	/* accidental target */
struct obj *otmp;	/* missile; might be destroyed by drop_throw */
int range;		/* how much farther will object travel if it misses */
			/* Use -1 to signify to keep going even after hit, */
			/* unless its gone (used for rolling_boulder_traps) */
boolean verbose;  /* give message(s) even when you can't see what happened */
{
	int damage, tmp;
	boolean vis, ismimic;
	int objgone = 1;

	ismimic = mtmp->m_ap_type && mtmp->m_ap_type != M_AP_MONSTER;
	vis = cansee(bhitpos.x, bhitpos.y);

	tmp = 5 + find_mac(mtmp) + omon_adj(mtmp, otmp, FALSE);
	if (tmp < rnd(20)) {
	    if (!ismimic) {
		if (vis) miss(distant_name(otmp, xname), mtmp);
/*JP		else if (verbose) pline("It is missed.");*/
		else if (verbose) pline("何かがかすめた．");
	    }
	    if (!range) { /* Last position; object drops */
		(void) drop_throw(otmp, 0, mtmp->mx, mtmp->my);
		return 1;
	    }
	} else if (otmp->oclass == POTION_CLASS) {
	    if (ismimic) seemimic(mtmp);
	    mtmp->msleep = 0;
	    if (vis) otmp->dknown = 1;
	    potionhit(mtmp, otmp);
	    return 1;
	} else {
	    damage = dmgval(otmp, mtmp);
	    if (otmp->otyp == ACID_VENOM && resists_acid(mtmp))
		damage = 0;
	    if (ismimic) seemimic(mtmp);
	    mtmp->msleep = 0;
	    if (vis) hit(distant_name(otmp,xname), mtmp, exclam(damage));
/*JP	    else if (verbose) pline("It is hit%s", exclam(damage));*/
	    else if (verbose) pline("何かが命中した%s", exclam(damage));

	    if (otmp->opoisoned) {
		if (resists_poison(mtmp)) {
/*JP		    if (vis) pline_The("poison doesn't seem to affect %s.",*/
		    if (vis) pline("%sは毒の影響を受けない．",
				   mon_nam(mtmp));
		} else {
		    if (rn2(30)) {
			damage += rnd(6);
		    } else {
/*JP			if (vis) pline_The("poison was deadly...");*/
			if (vis) pline("毒は致死量だった．．．");
			damage = mtmp->mhp;
		    }
		}
	    }
	    if (objects[otmp->otyp].oc_material == SILVER &&
		    hates_silver(mtmp->data)) {
/*JP		if (vis) pline_The("silver sears %s flesh!",*/
	        if (vis) pline("%sの体は銀で焼かれた！",
				s_suffix(mon_nam(mtmp)));
/*JP		else if (verbose) pline("Its flesh is seared!");*/
		else if (verbose) pline("何者かの体は焼かれた！");
	    }
	    if (otmp->otyp == ACID_VENOM && cansee(mtmp->mx,mtmp->my)) {
		if (resists_acid(mtmp)) {
		    if (vis || verbose)
/*JP			pline("%s is unaffected.", Monnam(mtmp));*/
			pline("%sは影響を受けない．", Monnam(mtmp));
		    damage = 0;
		} else {
/*JP		    if (vis) pline_The("acid burns %s!", mon_nam(mtmp));
		    else if (verbose) pline("It is burned!");*/
		    if (vis) pline("%sは酸で焼かれた！", mon_nam(mtmp));
		    else if (verbose) pline("何物かは焼かれた！");
		}
	    }
	    mtmp->mhp -= damage;
	    if (mtmp->mhp < 1) {
		if (vis || verbose)
/*JP		    pline("%s is %s!", Monnam(mtmp),
			(nonliving(mtmp->data) || !vis)
			? "destroyed" : "killed");*/
		    pline("%sは%s！", Monnam(mtmp),
			(nonliving(mtmp->data) || !vis)
			? "倒された" : "死んだ");
		mondied(mtmp);
	    }

	    if ((otmp->otyp == CREAM_PIE || otmp->otyp == BLINDING_VENOM) &&
		   haseyes(mtmp->data)) {
		/* note: resists_blnd() doesn't apply here */
/*JP		if (vis) pline("%s is blinded by %s.",
				Monnam(mtmp), the(xname(otmp)));*/
		if (vis) pline("%sは%sによって目が見えなくなった．",
				Monnam(mtmp), the(xname(otmp)));
		mtmp->mcansee = 0;
		tmp = (int)mtmp->mblinded + rnd(25) + 20;
		if (tmp > 127) tmp = 127;
		mtmp->mblinded = tmp;
	    }

	    objgone = drop_throw(otmp, 1, bhitpos.x, bhitpos.y);
	    if (!objgone && range == -1) {  /* special case */
		    obj_extract_self(otmp); /* free it for motion again */
		    return 0;
	    }
	    return 1;
	}
	return 0;
}

void
m_throw(mon, x, y, dx, dy, range, obj)
	register struct monst *mon;
	register int x,y,dx,dy,range;		/* direction and range */
	register struct obj *obj;
{
	register struct monst *mtmp;
	struct obj *singleobj;
	char sym = obj->oclass;
	int hitu, blindinc = 0;

	bhitpos.x = x;
	bhitpos.y = y;

	if (obj->quan == 1L) {
	    /*
	     * Remove object from minvent.  This cannot be done later on;
	     * what if the player dies before then, leaving the monster
	     * with 0 daggers?  (This caused the infamous 2^32-1 orcish
	     * dagger bug).
	     *
	     * VENOM is not in minvent - it should already be OBJ_FREE.
	     * The extract below does nothing.
	     */

	    /* not possibly_unwield, which checks the object's */
	    /* location, not its existence */
	    if (MON_WEP(mon) == obj) {
		    obj->owornmask &= ~W_WEP;
		    MON_NOWEP(mon);
	    }
	    obj_extract_self(obj);
	    singleobj = obj;
	    obj = (struct obj *) 0;
	} else {
	    singleobj = splitobj(obj, obj->quan - 1L);
	    obj_extract_self(singleobj);
	}

	singleobj->owornmask = 0; /* threw one of multiple weapons in hand? */

	if (singleobj->cursed && (dx || dy) && !rn2(7)) {
	    if(canseemon(mon) && flags.verbose) {
		if((singleobj->oclass == WEAPON_CLASS ||
						singleobj->oclass == GEM_CLASS)
		   && objects[singleobj->otyp].w_propellor)
/*JP		    pline("%s misfires!", Monnam(mon));*/
		    pline("%sははずした！", Monnam(mon));
		else
/*JP		    pline("%s slips as %s throws it!",*/
		    pline("%sが投げようとしたとたん%sが滑った！",
			  mon_nam(mon),The(xname(singleobj)));
	    }
	    dx = rn2(3)-1;
	    dy = rn2(3)-1;
	    /* pre-check validity of new direction */
	    if((!dx && !dy)
	       || !isok(bhitpos.x+dx,bhitpos.y+dy)
	       /* missile hits the wall */
	       || IS_ROCK(levl[bhitpos.x+dx][bhitpos.y+dy].typ)) {
		(void) drop_throw(singleobj, 0, bhitpos.x, bhitpos.y);
		return;
	    }
	}

	/* Note: drop_throw may destroy singleobj.  Since obj must be destroyed
	 * early to avoid the dagger bug, anyone who modifies this code should
	 * be careful not to use either one after it's been freed.
	 */
	if (sym) tmp_at(DISP_FLASH, obj_to_glyph(singleobj));
	while(range-- > 0) { /* Actually the loop is always exited by break */
		bhitpos.x += dx;
		bhitpos.y += dy;
		if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
		    if (ohitmon(mtmp, singleobj, range, TRUE))
			break;
		} else if (bhitpos.x == u.ux && bhitpos.y == u.uy) {
		    if (multi) nomul(0);

		    if (singleobj->oclass == GEM_CLASS &&
			    singleobj->otyp <= LAST_GEM+6 /* 6 glass colors */
			    && u.usym == S_UNICORN) {
			if (singleobj->otyp > LAST_GEM) {
/*JP			    You("catch the %s.", xname(singleobj));*/
			    You("%sをつかまえた．", xname(singleobj));
/*JP			    You("are not interested in %s junk.",*/
			    You("%sのガラクタに興味はない．",
				s_suffix(mon_nam(mon)));
			    makeknown(singleobj->otyp);
			    dropy(singleobj);
			} else {
/*JP			    You("accept %s gift in the spirit in which it was intended.",*/
			    You("これが欲しかったんだと思いながら%sの贈り物を受けとった．",
				s_suffix(mon_nam(mon)));
			    (void)hold_another_object(singleobj,
/*JP				"You catch, but drop, %s.", xname(singleobj),
				"You catch:");*/
				"あなたは%sをつかまえたが，落した．", xname(singleobj),
				"をつかまえた");
			}
			break;
		    }
		    if (singleobj->oclass == POTION_CLASS) {
			if (!Blind) singleobj->dknown = 1;
			potionhit(&youmonst, singleobj);
			break;
		    }
		    switch(singleobj->otyp) {
			int dam, hitv;
			case CREAM_PIE:
			case BLINDING_VENOM:
			    hitu = thitu(8, 0, singleobj, xname(singleobj));
			    break;
			default:
			    dam = dmgval(singleobj, &youmonst);
			    hitv = 3 - distmin(u.ux,u.uy, mon->mx,mon->my);
			    if (hitv < -4) hitv = -4;
			    if (is_elf(mon->data) &&
				objects[singleobj->otyp].w_propellor
							    == WP_BOW) {
				hitv++;
				if (MON_WEP(mon) &&
				    MON_WEP(mon)->otyp == ELVEN_BOW)
				    hitv++;
				if(singleobj->otyp == ELVEN_ARROW) dam++;
			    }
			    if (bigmonst(uasmon)) hitv++;
			    hitv += 8+singleobj->spe;

			    if (dam < 1) dam = 1;
			    hitu = thitu(hitv, dam,
				    singleobj, xname(singleobj));
		    }
		    if (hitu && singleobj->opoisoned) {
/*JP
			char *singlename = xname(singleobj);
*/
			char singlename[BUFSZ];
/*JP			Sprintf(singlename, "%sの", xname(singleobj));*/
			Sprintf(singlename, "%s", xname(singleobj));
			poisoned(singlename, A_STR, singlename, 10);
		    }
		    if(hitu && (singleobj->otyp == CREAM_PIE ||
				 singleobj->otyp == BLINDING_VENOM)) {
			blindinc = rnd(25);
			if(singleobj->otyp == CREAM_PIE) {
/*JP
			    if(!Blind) pline("Yecch!  You've been creamed.");
*/
			    if(!Blind) pline("ウェー．クリームをかぶった．");
/*JP
			    else	pline("There's %s sticky all over your %s.",
					    something,
*/
			    else	pline("あなたは%sにべとつくものを感じた．",
					    body_part(FACE));
			} else {	/* venom in the eyes */
			    if(Blindfolded) /* nothing */ ;
/*JP			    else if(!Blind) pline_The("venom blinds you.");*/
			    else if(!Blind) pline("毒で目が見えなくなった．");
/*JP			    else	Your("%s sting.",*/
			    else	Your("%sはちくちくした．",
				    makeplural(body_part(EYE)));
			}
		    }
		    stop_occupation();
		    if (hitu || !range) {
			(void) drop_throw(singleobj, hitu, u.ux, u.uy);
			break;
		    }
		} else if (!range	/* reached end of path */
			/* missile hits edge of screen */
			|| !isok(bhitpos.x+dx,bhitpos.y+dy)
			/* missile hits the wall */
			|| IS_ROCK(levl[bhitpos.x+dx][bhitpos.y+dy].typ)
#ifdef SINKS
			/* Thrown objects "sink" */
			|| IS_SINK(levl[bhitpos.x][bhitpos.y].typ)
#endif
								) {
		    (void) drop_throw(singleobj, 0, bhitpos.x, bhitpos.y);
		    break;
		}
		tmp_at(bhitpos.x, bhitpos.y);
		delay_output();
	}
	tmp_at(bhitpos.x, bhitpos.y);
	delay_output();
	tmp_at(DISP_END, 0);
	/* blindfold keeps substances out of your eyes */
	if (blindinc && !Blindfolded) {
		u.ucreamed += blindinc;
		make_blinded(Blinded + blindinc,FALSE);
	}
}

#endif /* OVL1 */
#ifdef OVLB

/* Remove an item from the monster's inventory and destroy it. */
void
m_useup(mon, obj)
struct monst *mon;
struct obj *obj;
{
	if (obj->quan > 1L) {
		obj->quan--;
	} else {
		obj_extract_self(obj);
		possibly_unwield(mon);
		if (obj->owornmask) {
		    mon->misc_worn_check &= ~obj->owornmask;
		    update_mon_intrinsics(mon, obj, FALSE);
		}
		dealloc_obj(obj);
	}
}

#endif /* OVLB */
#ifdef OVL1

void
thrwmu(mtmp)	/* monster throws item at you */
register struct monst *mtmp;
{
	struct obj *otmp;
	register xchar x, y;

	if(lined_up(mtmp)) {
	    if (mtmp->weapon_check == NEED_WEAPON || !MON_WEP(mtmp)) {
		mtmp->weapon_check = NEED_RANGED_WEAPON;
		/* mon_wield_item resets weapon_check as appropriate */
		if(mon_wield_item(mtmp) != 0) return;
	    }
	    otmp = select_rwep(mtmp);
	    if (otmp) {
		/* If you are coming toward the monster, the monster
		 * should try to soften you up with missiles.  If you are
		 * going away, you are probably hurt or running.  Give
		 * chase, but if you are getting too far away, throw.
		 */
		x = mtmp->mx;
		y = mtmp->my;
		if(!URETREATING(x,y) ||
		   !rn2(BOLT_LIM-distmin(x,y,mtmp->mux,mtmp->muy)))
		{
/*JP		    const char *verb = "throws";*/
		    const char *verb = "投げた";

		    if (otmp->otyp == ARROW
			|| otmp->otyp == ELVEN_ARROW
			|| otmp->otyp == ORCISH_ARROW
/*JP			|| otmp->otyp == CROSSBOW_BOLT) verb = "shoots";*/
			|| otmp->otyp == CROSSBOW_BOLT) verb = "撃った";
		    if (canseemon(mtmp)) {
/*JP			pline("%s %s %s!", Monnam(mtmp), verb,*/
			pline("%sは%sを%s！", Monnam(mtmp), 
			      obj_is_pname(otmp) ?
			      the(singular(otmp, xname)) :
			      an(singular(otmp, xname)),verb);
		    }
		    m_throw(mtmp, mtmp->mx, mtmp->my, sgn(tbx), sgn(tby),
			distmin(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy), otmp);
		    nomul(0);
		    return;
		}
	    }
	}
}

#endif /* OVL1 */
#ifdef OVLB

int
spitmu(mtmp, mattk)		/* monster spits substance at you */
register struct monst *mtmp;
register struct attack *mattk;
{
	register struct obj *otmp;

	if(mtmp->mcan) {

	    if(flags.soundok)
/*JP		pline("A dry rattle comes from %s throat",*/
		pline("ガラガラ声が%sの喉から聞こえた",
		                      s_suffix(mon_nam(mtmp)));
	    return 0;
	}
	if(lined_up(mtmp)) {
		switch (mattk->adtyp) {
		    case AD_BLND:
		    case AD_DRST:
			otmp = mksobj(BLINDING_VENOM, TRUE, FALSE);
			break;
		    default:
			impossible("bad attack type in spitmu");
				/* fall through */
		    case AD_ACID:
			otmp = mksobj(ACID_VENOM, TRUE, FALSE);
			break;
		}
		if(!rn2(BOLT_LIM-distmin(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy))) {
		    if (canseemon(mtmp))
/*JP			pline("%s spits venom!", Monnam(mtmp));*/
			pline("%sは毒を吐いた！", Monnam(mtmp));
		    m_throw(mtmp, mtmp->mx, mtmp->my, sgn(tbx), sgn(tby),
			distmin(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy), otmp);
		    nomul(0);
		    return 0;
		}
	}
	return 0;
}

#endif /* OVLB */
#ifdef OVL1

int
breamu(mtmp, mattk)			/* monster breathes at you (ranged) */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	/* if new breath types are added, change AD_ACID to max type */
	int typ = (mattk->adtyp == AD_RBRE) ? rnd(AD_ACID) : mattk->adtyp ;

	if(lined_up(mtmp)) {

	    if(mtmp->mcan) {
		if(flags.soundok) {
		    if(canseemon(mtmp))
/*JP			pline("%s coughs.", Monnam(mtmp));*/
			pline("%sはせきをした．", Monnam(mtmp));
		    else
/*JP			You_hear("a cough.");*/
			You("せきの音を聞いた．");
		}
		return(0);
	    }
	    if(!mtmp->mspec_used && rn2(3)) {

		if((typ >= AD_MAGM) && (typ <= AD_ACID)) {

		    if(canseemon(mtmp))
/*JP			pline("%s breathes %s!", Monnam(mtmp),*/
			pline("%sは%sを吐いた！", Monnam(mtmp),
			      breathwep[typ-1]);
		    buzz((int) (-20 - (typ-1)), (int)mattk->damn,
			 mtmp->mx, mtmp->my, sgn(tbx), sgn(tby));
		    nomul(0);
		    /* breath runs out sometimes. Also, give monster some
		     * cunning; don't breath if the player fell asleep.
		     */
		    if(!rn2(3))
			mtmp->mspec_used = 10+rn2(20);
		    if(typ == AD_SLEE && !Sleep_resistance)
			mtmp->mspec_used += rnd(20);
		} else impossible("Breath weapon %d used", typ-1);
	    }
	}
	return(1);
}

boolean
linedup(ax, ay, bx, by)
register xchar ax, ay, bx, by;
{
	tbx = ax - bx;	/* These two values are set for use */
	tby = ay - by;	/* after successful return.	    */

	/* sometimes displacement makes a monster think that you're at its
	   own location; prevent it from throwing and zapping in that case */
	if (!tbx && !tby) return FALSE;

	if((!tbx || !tby || abs(tbx) == abs(tby)) /* straight line or diagonal */
	   && distmin(tbx, tby, 0, 0) < BOLT_LIM) {
	    if(ax == u.ux && ay == u.uy) return((boolean)(couldsee(bx,by)));
	    else if(clear_path(ax,ay,bx,by)) return TRUE;
	}
	return FALSE;
}

boolean
lined_up(mtmp)		/* is mtmp in position to use ranged attack? */
	register struct monst *mtmp;
{
	return(linedup(mtmp->mux,mtmp->muy,mtmp->mx,mtmp->my));
}

#endif /* OVL1 */
#ifdef OVL0

/* Check if a monster is carrying a particular item.
 */
struct obj *
m_carrying(mtmp, type)
struct monst *mtmp;
int type;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == type)
			return(otmp);
	return((struct obj *) 0);
}

#endif /* OVL0 */

/*mthrowu.c*/
