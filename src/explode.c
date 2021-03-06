/*	SCCS Id: @(#)explode.c	3.2	96/05/01	*/
/*	Copyright (C) 1990 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

#ifdef OVL0

/* Note: Arrays are column first, while the screen is row first */
static int expl[3][3] = {
	{ S_explode1, S_explode4, S_explode7 },
	{ S_explode2, S_explode5, S_explode8 },
	{ S_explode3, S_explode6, S_explode9 }
};

/* Note: I had to choose one of three possible kinds of "type" when writing
 * this function: a wand type (like in zap.c), an adtyp, or an object type.
 * Wand types get complex because they must be converted to adtyps for
 * determining such things as fire resistance.  Adtyps get complex in that
 * they don't supply enough information--was it a player or a monster that
 * did it, and with a wand, spell, or breath weapon?  Object types share both
 * these disadvantages....
 */
void
explode(x, y, type, dam, olet)
int x, y;
int type; /* the same as in zap.c */
int dam;
char olet;
{
	int i, j, k, damu = dam;
	boolean starting = 1;
	boolean visible, any_shield;
	int uhurt = 0; /* 0=unhurt, 1=items damaged, 2=you and items damaged */
	const char *str;
	int idamres, idamnonres;
	struct monst *mtmp;
	uchar adtyp;
	int explmask[3][3];
		/* 0=normal explosion, 1=do shieldeff, 2=do nothing */
	boolean shopdamage = FALSE;

	if (olet == WAND_CLASS)		/* retributive strike */
		switch (u.role) {
			case 'P':
			case 'W': damu /= 5;
				  break;
			case 'H':
			case 'K': damu /= 2;
				  break;
			default:  break;
		}

	switch (abs(type) % 10) {
/*JP		case 0: str = "magical blast";*/
		case 0: str = "魔法の風";
			adtyp = AD_MAGM;
			break;
/*JP		case 1: str =   olet == BURNING_OIL ?	"burning oil" :
				olet == SCROLL_CLASS ?	"tower of flame" :
							"fireball";*/
		case 1: str =   olet == BURNING_OIL ?	"燃えている油" :
				olet == SCROLL_CLASS ?	"火柱" :
							"火の玉";
			adtyp = AD_FIRE;
			break;
/*JP		case 2: str = "ball of cold";*/
		case 2: str = "氷の玉";
			adtyp = AD_COLD;
			break;
/*JP		case 4: str = "disintegration field";*/
		case 4: str = "粉砕の風";
			adtyp = AD_DISN;
			break;
/*JP		case 5: str = "ball lightning";*/
		case 5: str = "雷球";
			adtyp = AD_ELEC;
			break;
		default: impossible("explosion base type %d?", type); return;
	}

	any_shield = visible = FALSE;
	for (i=0; i<3; i++) for (j=0; j<3; j++) {
		if (!isok(i+x-1, j+y-1)) {
			explmask[i][j] = 2;
			continue;
		} else
			explmask[i][j] = 0;

		if (i+x-1 == u.ux && j+y-1 == u.uy) {
		    switch(adtyp) {
			case AD_MAGM:
				explmask[i][j] = !!Antimagic;
				break;
			case AD_FIRE:
				explmask[i][j] = !!Fire_resistance;
				break;
			case AD_COLD:
				explmask[i][j] = !!Cold_resistance;
				break;
			case AD_DISN:
				explmask[i][j] = !!Disint_resistance;
				break;
			case AD_ELEC:
				explmask[i][j] = !!Shock_resistance;
				break;
			default:
				impossible("explosion type %d?", adtyp);
				break;
		    }
		}
		/* can be both you and mtmp if you're swallowed */
		if ((mtmp = m_at(i+x-1, j+y-1)) != 0) {
		    switch(adtyp) {
			case AD_MAGM:
				explmask[i][j] |= resists_magm(mtmp);
				break;
			case AD_FIRE:
				explmask[i][j] |= resists_fire(mtmp);
				break;
			case AD_COLD:
				explmask[i][j] |= resists_cold(mtmp);
				break;
			case AD_DISN:
				explmask[i][j] |= resists_disint(mtmp);
				break;
			case AD_ELEC:
				explmask[i][j] |= resists_elec(mtmp);
				break;
			default:
				impossible("explosion type %d?", adtyp);
				break;
		    }
		}

		if (cansee(i+x-1, j+y-1)) visible = TRUE;
		if (explmask[i][j] == 1) any_shield = TRUE;
	}

	if (visible) {
		/* Start the explosion */
		for (i=0; i<3; i++) for (j=0; j<3; j++) {
			if (explmask[i][j] == 2) continue;
			tmp_at(starting ? DISP_BEAM : DISP_CHANGE,
					    cmap_to_glyph(expl[i][j]));
			tmp_at(i+x-1, j+y-1);
			starting = 0;
		}
		curs_on_u();	/* will flush screen and output */

		if (any_shield) {	/* simulate a shield effect */
		    for (k = 0; k < SHIELD_COUNT; k++) {
			for (i=0; i<3; i++) for (j=0; j<3; j++) {
			    if (explmask[i][j] == 1)
				/*
				 * Bypass tmp_at() and send the shield glyphs
				 * directly to the buffered screen.  tmp_at()
				 * will clean up the location for us later.
				 */
				show_glyph(i+x-1, j+y-1,
					cmap_to_glyph(shield_static[k]));
			}
			curs_on_u();	/* will flush screen and output */
			delay_output();
		    }

		    /* Cover last shield glyph with blast symbol. */
		    for (i=0; i<3; i++) for (j=0; j<3; j++) {
			if (explmask[i][j] == 1)
			    show_glyph(i+x-1,j+y-1,cmap_to_glyph(expl[i][j]));
		    }

		} else {		/* delay a little bit. */
		    delay_output();
		    delay_output();
		}

	} else {
/*JP		if (flags.soundok) You_hear("a blast.");*/
		if (flags.soundok) You("爆風の音を聞いた！");
	}

    if (dam)
	for (i=0; i<3; i++) for (j=0; j<3; j++) {
		if (explmask[i][j] == 2) continue;
		if (i+x-1 == u.ux && j+y-1 == u.uy)
			uhurt = (explmask[i][j] == 1) ? 1 : 2;
		idamres = idamnonres = 0;
		(void)zap_over_floor((xchar)(i+x-1), (xchar)(j+y-1),
				     type, &shopdamage);

		mtmp = m_at(i+x-1, j+y-1);
		if (!mtmp) continue;
		if (u.uswallow && mtmp == u.ustuck) {
			if (is_animal(u.ustuck->data))
/*JP				pline("%s gets %s!",
				      Monnam(u.ustuck),
				      (adtyp == AD_FIRE) ? "heartburn" :
				      (adtyp == AD_COLD) ? "chilly" :
				      (adtyp == AD_DISN) ? "perforated" :
				      (adtyp == AD_ELEC) ? "shocked" :
				       "fried");*/
				pline("%sは%s！",
				      Monnam(u.ustuck),
				      (adtyp == AD_FIRE) ? "燃えた" :
				      (adtyp == AD_COLD) ? "凍らされた" :
				      (adtyp == AD_DISN) ? "穴をあけられた" :
				      (adtyp == AD_ELEC) ? "電撃をくらった" :
				       "fried");
			else
				pline("%sはわずかに%s！",
				      Monnam(u.ustuck),
				      (adtyp == AD_FIRE) ? "焦げた" :
				      (adtyp == AD_COLD) ? "凍らされた" :
				      (adtyp == AD_DISN) ? "穴をあけられた" :
				      (adtyp == AD_ELEC) ? "電撃をくらった" :
				       "fried");
		} else
/*JP		pline("%s is caught in the %s!",
			cansee(i+x-1, j+y-1) ? Monnam(mtmp) : "It", str);*/
		pline("%sは%sにつつまれた！",
			cansee(i+x-1, j+y-1) ? Monnam(mtmp) : "何者か", str);

		idamres += destroy_mitem(mtmp, SCROLL_CLASS, (int) adtyp);
		idamres += destroy_mitem(mtmp, SPBOOK_CLASS, (int) adtyp);
		idamnonres += destroy_mitem(mtmp, POTION_CLASS, (int) adtyp);
		idamnonres += destroy_mitem(mtmp, WAND_CLASS, (int) adtyp);
		idamnonres += destroy_mitem(mtmp, RING_CLASS, (int) adtyp);

		if (explmask[i][j] == 1) {
			golemeffects(mtmp, (int) adtyp, dam + idamres);
			mtmp->mhp -= idamnonres;
		} else {
		/* call resist with 0 and do damage manually so 1) we can
		 * get out the message before doing the damage, and 2) we can
		 * call mondied, not killed, if it's not your blast
		 */
			int mdam = dam;

			if (resist(mtmp, olet, 0, FALSE)) {
/*JP				pline("%s resists the magical blast!",*/
				pline("%sは魔法の風を無効化した！",
					cansee(i+x-1,j+y-1) ? Monnam(mtmp)
/*JP					: "It");*/
					: "何者か");
				mdam = dam/2;
			}
			if (mtmp == u.ustuck)
				mdam *= 2;
			if (resists_cold(mtmp) && adtyp == AD_FIRE)
				mdam *= 2;
			else if (resists_fire(mtmp) && adtyp == AD_COLD)
				mdam *= 2;
			mtmp->mhp -= mdam;
			mtmp->mhp -= (idamres + idamnonres);
		}
		if (mtmp->mhp <= 0) {
			if (type >= 0) killed(mtmp);
			else mondied(mtmp);
		}
	}

	if (visible) tmp_at(DISP_END, 0); /* clear the explosion */

	/* Do your injury last */
	if (uhurt) {
		if (type >= 0 && flags.verbose && olet != SCROLL_CLASS)
/*JP			You("are caught in the %s!", str);*/
			You("%sにつつまれた！", str);
		/* do property damage first, in case we end up leaving bones */
		if (adtyp == AD_FIRE) (void) burnarmor();
		destroy_item(SCROLL_CLASS, (int) adtyp);
		destroy_item(SPBOOK_CLASS, (int) adtyp);
		destroy_item(POTION_CLASS, (int) adtyp);
		destroy_item(RING_CLASS, (int) adtyp);
		destroy_item(WAND_CLASS, (int) adtyp);

		ugolemeffects((int) adtyp, damu);
		if (uhurt == 2) u.uhp -= damu, flags.botl = 1;

		if (u.uhp <= 0) {
			char buf[BUFSZ];

			if (type >= 0 && olet != SCROLL_CLASS) {
/*JP			    killer_format = NO_KILLER_PREFIX;*/
			    killer_format = KILLED_BY;
/*JP			    Sprintf(buf, "caught %sself in %s own %s.",
				    him[flags.female], his[flags.female], str);*/
			    Sprintf(buf, "自分自身の%sにつつまれて",
				    str);
			} else {
			    killer_format = KILLED_BY;
			    Strcpy(buf, str);
			}
			killer = buf;
			/* Known BUG: BURNING suppresses corpse in bones data,
			   but done does not handle killer reason correctly */
			/* done(adtyp == AD_FIRE ? BURNING : DIED); */
			done(BURNING);
		}
		exercise(A_STR, FALSE);
	}

	if (shopdamage) {
/*JP		pay_for_damage(adtyp == AD_FIRE ? "burn away" :
			       adtyp == AD_COLD ? "shatter" :
			       adtyp == AD_DISN ? "disintegrate" : "destroy");*/
		pay_for_damage(adtyp == AD_FIRE ? "焼く" :
			       adtyp == AD_COLD ? "こなごなにする" :
			       adtyp == AD_DISN ? "こなごなにする" : "破壊する");
	}
}
#endif /* OVL0 */
#ifdef OVL1

struct scatter_chain {
	struct scatter_chain *next;	/* pointer to next scatter item	*/
	struct obj *obj;		/* pointer to the object	*/
	xchar ox;			/* location of			*/
	xchar oy;			/*	item			*/
	schar dx;			/* direction of			*/
	schar dy;			/*	travel			*/
	int range;			/* range of object		*/
	boolean stopped;		/* flag for in-motion/stopped	*/
};

/*
 * scflags:
 *	VIS_EFFECTS	Add visual effects to display
 *	MAY_HITMON	Objects may hit monsters
 *	MAY_HITYOU	Objects may hit hero
 *	MAY_HIT		Objects may hit you or monsters
 *	MAY_DESTROY	Objects may be destroyed at random
 *	MAY_FRACTURE	Stone objects can be fractured (statues, boulders)
 */

void
scatter(sx,sy,blastforce,scflags)
int sx,sy;				/* location of objects to scatter */
int blastforce;				/* force behind the scattering	*/
unsigned int scflags;
{
	register struct obj *otmp;
	register int tmp;
	int farthest = 0;
	uchar typ;
	long qtmp;
	boolean used_up;
	struct monst *mtmp;
	struct scatter_chain *stmp, *stmp2 = 0;
	struct scatter_chain *schain = (struct scatter_chain *)0;

	while ((otmp = level.objects[sx][sy]) != 0) {
	    if (otmp->quan > 1L) {
		qtmp = (long)rnd((int)otmp->quan - 1);
		(void) splitobj(otmp, qtmp);
	    }
	    obj_extract_self(otmp);
	    used_up = FALSE;

	    /* 9 in 10 chance of fracturing boulders or statues */
	    if ((scflags & MAY_FRACTURE)
			&& ((otmp->otyp == BOULDER) || (otmp->otyp == STATUE))
			&& rn2(10)) {
		if (otmp->otyp == BOULDER) {
/*JP		    pline("%s breaks apart.",The(xname(otmp)));*/
		    pline("%sは一部分が砕けた．",The(xname(otmp)));
		    fracture_rock(otmp);
		    place_object(otmp, sx, sy);	/* put fragments on floor */
		} else {
		    struct trap *trap;

		    if ((trap = t_at(sx,sy)) && trap->ttyp == STATUE_TRAP)
			    deltrap(trap);
/*JP		    pline("%s crumbles.",The(xname(otmp)));*/
		    pline("%sはこなごなになった．",The(xname(otmp)));
		    (void) break_statue(otmp);
		    place_object(otmp, sx, sy);	/* put fragments on floor */
		}
		used_up = TRUE;

	    /* 1 in 10 chance of destruction of obj; glass, egg destruction */
	    } else if ((scflags & MAY_DESTROY) && (!rn2(10)
			|| (objects[otmp->otyp].oc_material == GLASS
			|| otmp->otyp == EGG))) {
		if (breaks(otmp, (xchar)sx, (xchar)sy)) used_up = TRUE;
	    }

	    if (!used_up) {
		stmp = (struct scatter_chain *)
					alloc(sizeof(struct scatter_chain));
		stmp->next = (struct scatter_chain *)0;
		stmp->obj = otmp;
		stmp->ox = sx;
		stmp->oy = sy;
		tmp = rn2(8);		/* get the direction */
		stmp->dx = xdir[tmp];
		stmp->dy = ydir[tmp];
		tmp = blastforce - (otmp->owt/40);
		if (tmp < 1) tmp = 1;
		stmp->range = rnd(tmp); /* anywhere up to that determ. by wt */
		if (farthest < stmp->range) farthest = stmp->range;
		stmp->stopped = FALSE;
		if (!schain)
		    schain = stmp;
		else
		    stmp2->next = stmp;
		stmp2 = stmp;
	    }
	}

	while (farthest-- > 0) {
		for (stmp = schain; stmp; stmp = stmp->next) {
		   if ((stmp->range-- > 0) && (!stmp->stopped)) {
			bhitpos.x = stmp->ox + stmp->dx;
			bhitpos.y = stmp->oy + stmp->dy;
			typ = levl[bhitpos.x][bhitpos.y].typ;
			if(!isok(bhitpos.x, bhitpos.y)) {
				bhitpos.x -= stmp->dx;
				bhitpos.y -= stmp->dy;
				stmp->stopped = TRUE;
			} else if(!ZAP_POS(typ) ||
					closed_door(bhitpos.x, bhitpos.y)) {
				bhitpos.x -= stmp->dx;
				bhitpos.y -= stmp->dy;
				stmp->stopped = TRUE;
			} else if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
				if (scflags & MAY_HITMON) {
				    stmp->range--;
				    if (ohitmon(mtmp, stmp->obj, 1, FALSE)) {
					stmp->obj = (struct obj *)0;
					stmp->stopped = TRUE;
				    }
				}
			} else if (bhitpos.x==u.ux && bhitpos.y==u.uy) {
				if (scflags & MAY_HITYOU) {
				    int hitvalu, hitu;

				    if (multi) nomul(0);
				    hitvalu = 8 + stmp->obj->spe;
				    if (bigmonst(uasmon)) hitvalu++;
				    /* could just use doname all the time,
				     * except thitu adds "an" to the front
				     */
				    hitu = thitu(hitvalu,
					dmgval(stmp->obj, &youmonst),
					stmp->obj,
					(stmp->obj->quan>1) ? doname(stmp->obj)
						: xname(stmp->obj));
				    if (hitu) {
					stmp->range -= 3;
					stop_occupation();
				    }
				}
			} else {
				if (scflags & VIS_EFFECTS) {
				    /* tmp_at(bhitpos.x, bhitpos.y); */
				    /* delay_output(); */
				}
			}
			stmp->ox = bhitpos.x;
			stmp->oy = bhitpos.y;
		   }
		}
	}
	for (stmp = schain; stmp; stmp = stmp2) {
		int x,y;

		stmp2 = stmp->next;
		x = stmp->ox; y = stmp->oy;
		if (stmp->obj) {
			place_object(stmp->obj, x, y);
			stackobj(stmp->obj);
		}
		free((genericptr_t)stmp);
		newsym(x,y);
	}
}


/*
 * Splatter burning oil from x,y to the surrounding area.
 *
 * This routine should really take a how and direction parameters.
 * The how is how it was caused, e.g. kicked verses thrown.  The
 * direction is which way to spread the flaming oil.  Different
 * "how"s would give different dispersal patterns.  For example,
 * kicking a burning flask will splatter differently from a thrown
 * flask hitting the ground.
 *
 * For now, just perform a "regular" explosion.
 */
void
splatter_burning_oil(x, y)
    int x, y;
{
/* ZT_SPELL(ZT_FIRE) = ZT_SPELL(AD_FIRE-1) = 10+(2-1) = 11 */
#define ZT_SPELL_O_FIRE 11 /* value kludge, see zap.c */
    explode(x, y, ZT_SPELL_O_FIRE, d(4,4), BURNING_OIL);
}

#endif /* OVL1 */

/*explode.c*/
