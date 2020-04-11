/*	SCCS Id: @(#)dokick.c	3.1	93/06/15	*/
/* Copyright (c) Izchak Miller, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "eshk.h"

#ifndef POLYSELF
# define martial()	(pl_character[0] == 'S' || pl_character[0] == 'P')
#else
# define is_bigfoot(x)	((x) == &mons[PM_SASQUATCH])
# define martial()	(pl_character[0] == 'S' || pl_character[0] == 'P' \
			 || is_bigfoot(uasmon))
#endif

static NEARDATA struct rm *maploc;

extern boolean notonhead;	/* for long worms */

static void FDECL(kickdmg, (struct monst *, BOOLEAN_P));
static void FDECL(kick_monster, (XCHAR_P, XCHAR_P));
static int FDECL(kick_object, (XCHAR_P, XCHAR_P));
static char *NDECL(kickstr);
static void FDECL(otransit_msg, (struct obj *, XCHAR_P, BOOLEAN_P, int));
static const char *FDECL(gate_str, (XCHAR_P));
static void FDECL(drop_to, (coord *, XCHAR_P));

static NEARDATA struct obj *kickobj;

#define IS_SHOP(x)	(rooms[x].rtype >= SHOPBASE)

static void
kickdmg(mon, clumsy)
register struct monst *mon;
register boolean clumsy;
{
	register int mdx, mdy;
	register int dmg = ( ACURRSTR + ACURR(A_DEX) + ACURR(A_CON) )/ 15;

	/* excessive wt affects dex, so it affects dmg */
	if(clumsy) dmg = dmg/2;

	/* kicking a dragon or an elephant will not harm it */
	if(thick_skinned(mon->data)) dmg = 0;

	/* a good kick exercises your dex */
	exercise(A_DEX, TRUE);

/*	it is unchivalrous to attack the defenseless or from behind */
	if (pl_character[0] == 'K' &&
		u.ualign.type == A_LAWFUL && u.ualign.record > -10 &&
		(!mon->mcanmove || mon->msleep || mon->mflee))
		adjalign(-1);

	/* squeeze some guilt feelings... */
	if(mon->mtame) {
	    abuse_dog(mon);
	    mon->mflee = mon->mtame ? 1 : 0;
#ifdef HISX
	    mon->mfleetim = mon->mfleetim + (dmg ? rnd(dmg) : 1);
#else
	    mon->mfleetim += (dmg ? rnd(dmg) : 1);
#endif
	}

	if (dmg > 0)
		mon->mhp -= (!martial() ? rnd(dmg) :
			rnd(dmg)+rnd(ACURR(A_DEX)/2));
	if(mon->mhp < 1) {
		(void) passive(mon, TRUE, 0, TRUE);
		killed(mon);
		return;
	}
	if(martial() && !bigmonst(mon->data) && !rn2(3) && mon->mcanmove
	   && mon != u.ustuck) {
		/* see if the monster has a place to move into */
		mdx = mon->mx + u.dx;
		mdy = mon->my + u.dy;
		if(goodpos(mdx, mdy, mon, mon->data)) {
/*JP			pline("%s reels from the blow.", Monnam(mon));*/
			pline("%sは強打されよろめいた．", Monnam(mon));
			remove_monster(mon->mx, mon->my);
			newsym(mon->mx, mon->my);
			place_monster(mon, mdx, mdy);
			newsym(mon->mx, mon->my);
			set_apparxy(mon);
		}
	}
	(void) passive(mon, FALSE, 1, TRUE);

}

static void
kick_monster(x, y)
register xchar x, y;
{
	register boolean clumsy = FALSE;
	register struct monst *mon = m_at(x, y);
	register int i, j;

	bhitpos.x = x;
	bhitpos.y = y;
	if(special_case(mon)) return;
	setmangry(mon);
#ifdef POLYSELF
	/* Kick attacks by kicking monsters are normal attacks, not special.
	 * If you have >1 kick attack, you get all of them.
	 */
	if (attacktype(uasmon, AT_KICK)) {
	    schar tmp = find_roll_to_hit(mon);
	    for(i=0; i<NATTK; i++) {
		if (uasmon->mattk[i].aatyp == AT_KICK && multi >= 0) {
		    /* check multi; maybe they had 2 kicks and the first */
		    /* was a kick against a floating eye */
		    if (tmp > rnd(20)) {
			int sum;

/*JP			You("kick %s.", mon_nam(mon));*/
			You("%sを蹴った．", mon_nam(mon));
			sum = damageum(mon, &(uasmon->mattk[i]));
			if (sum == 2)
				(void)passive(mon, 1, 0, TRUE);
			else (void)passive(mon, sum, 1, TRUE);
		    } else {
			missum(mon, &(uasmon->mattk[i]));
			(void)passive(mon, 0, 1, TRUE);
		    }
		}
	    }
	    return;
	}
#endif

	/* no need to check POLYSELF since only ghosts, which you can't turn */
	/* into, are noncorporeal */
	if(noncorporeal(mon->data)) {
/*JP		Your("kick passes through!");*/
		Your("蹴りは空をきった！");
		return;
	}

	if(Levitation && !rn2(3) && verysmall(mon->data) &&
	   !is_flyer(mon->data)) {
/*JP		pline("Floating in the air, you miss wildly!");*/
		pline("空中に浮いているので，大きく外した！");
		exercise(A_DEX, FALSE);
		(void) passive(mon, FALSE, 1, TRUE);
		return;
	}

	i = -inv_weight();
	j = weight_cap();

	if(i < (j*3)/10) {
		if(!rn2((i < j/10) ? 2 : (i < j/5) ? 3 : 4)) {
			if(martial() && !rn2(2)) goto doit;
/*JP			Your("clumsy kick does no damage.");*/
			Your("不器用な蹴りはダメージを与えない．");
			(void) passive(mon, FALSE, 1, TRUE);
			return;
		}
		if(i < j/10) clumsy = TRUE;
		else if(!rn2((i < j/5) ? 2 : 3)) clumsy = TRUE;
	}

	if(Fumbling) clumsy = TRUE;

	else if(uarm && objects[uarm->otyp].oc_bulky && ACURR(A_DEX) < rnd(25))
		clumsy = TRUE;
doit:
/*JP	You("kick %s.", mon_nam(mon));*/
	You("%sを蹴った．", mon_nam(mon));
	if(!rn2(clumsy ? 3 : 4) && (clumsy || !bigmonst(mon->data)) &&
	   mon->mcansee && !mon->mtrapped && !thick_skinned(mon->data) &&
	   mon->data->mlet != S_EEL && haseyes(mon->data) && mon->mcanmove &&
	   !mon->mstun && !mon->mconf && !mon->msleep &&
	   mon->data->mmove >= 12) {
		if(!nohands(mon->data) && !rn2(martial() ? 5 : 3)) {
/*JP		    pline("%s blocks your %skick.", Monnam(mon),
				clumsy ? "clumsy " : "");*/
		    pline("%sはあなたの%s蹴りを防いだ．", Monnam(mon),
				clumsy ? "不器用な" : "");
		    (void) passive(mon, FALSE, 1, TRUE);
		    return;
		} else {
		    mnexto(mon);
		    if(mon->mx != x || mon->my != y) {
/*JP			pline("%s %s, %s evading your %skick.", Monnam(mon),
				(can_teleport(mon->data) ? "teleports" :
				 is_floater(mon->data) ? "floats" :
				 is_flyer(mon->data) ? "flutters" :
				 nolimbs(mon->data) ? "slides" :
				 "jumps"),
				clumsy ? "easily" : "nimbly",
				clumsy ? "clumsy " : "");*/
			pline("%sは%s，%sあなたの%s蹴りをたくみに避けた．", Monnam(mon),
				(can_teleport(mon->data) ? "瞬間移動し" :
				 is_floater(mon->data) ? "浮き" :
				 is_flyer(mon->data) ? "はばたき" :
				 nolimbs(mon->data) ? "横に滑り" :
				 "跳ね"),
				clumsy ? "楽々と" : "素早く",
				clumsy ? "不器用な" : "");
			(void) passive(mon, FALSE, 1, TRUE);
			return;
		    }
		}
	}
	kickdmg(mon, clumsy);
}

/*
 *  Return TRUE if caught (the gold taken care of), FALSE otherwise.
 *  The gold object is *not* attached to the fobj chain!
 */
boolean
ghitm(mtmp, gold)
register struct monst *mtmp;
register struct obj *gold;
{
	if(!likes_gold(mtmp->data) && !mtmp->isshk && !mtmp->ispriest
#ifdef ARMY
		&& !is_mercenary(mtmp->data)
#endif
		) {
		wakeup(mtmp);
	} else if (!mtmp->mcanmove) {
		/* too light to do real damage */
		if (canseemon(mtmp))
/*JP		    pline("The gold hits %s.", mon_nam(mtmp));*/
		    pline("ゴールドは%sに命中した．", mon_nam(mtmp));
	} else {
		mtmp->msleep = 0;
		mtmp->meating = 0;
		if(!rn2(4)) setmangry(mtmp); /* not always pleasing */

		/* greedy monsters catch gold */
		if (cansee(mtmp->mx, mtmp->my))
/*JP		    pline("%s catches the gold.", Monnam(mtmp));*/
		    pline("%sはゴールドを受けとった．", Monnam(mtmp));
		mtmp->mgold += gold->quan;
		if (mtmp->isshk) {
			long robbed = ESHK(mtmp)->robbed;

			if (robbed) {
				robbed -= gold->quan;
				if (robbed < 0) robbed = 0;
/*JP				pline("The amount %scovers %s recent losses.",
				      !robbed ? "" : "partially ",*/
				pline("%s%sの損失を補填するのに使われた．",
				      !robbed ? "" : "金の一部は",
				      his[mtmp->female]);
				ESHK(mtmp)->robbed = robbed;
				if(!robbed)
					make_happy_shk(mtmp, FALSE);
			} else {
				if(mtmp->mpeaceful) {
				    ESHK(mtmp)->credit += gold->quan;
/*JP				    You("have %ld zorkmid%s in credit.",*/
				    You("%ldゴールドをクレジットにした．",
					ESHK(mtmp)->credit);
/*JP				} else verbalize("Thanks, scum!");*/
				} else verbalize("ありがとよ！くそったれ！");
			}
		}
		else if(mtmp->ispriest) {
			if(mtmp->mpeaceful)
/*JP			    verbalize("Thank you for your contribution.");*/
			    verbalize("寄付をどうもありがとう．");
			else verbalize("ありがとよ！くそったれ！");
		}
#ifdef ARMY
		else if (is_mercenary(mtmp->data)) {
		    long goldreqd = 0L;

		    if (rn2(3)) {
			if (mtmp->data == &mons[PM_SOLDIER])
			   goldreqd = 100L;
			else if (mtmp->data == &mons[PM_SERGEANT])
			   goldreqd = 250L;
			else if (mtmp->data == &mons[PM_LIEUTENANT])
			   goldreqd = 500L;
			else if (mtmp->data == &mons[PM_CAPTAIN])
			   goldreqd = 750L;

			if (goldreqd) {
			   if (gold->quan > goldreqd +
				(u.ugold + u.ulevel*rn2(5))/ACURR(A_CHA))
			    mtmp->mpeaceful = TRUE;
			}
		     }
		     if (mtmp->mpeaceful)
/*JP			    verbalize("That should do.  Now beat it!");*/
			    verbalize("なんだい？これは？");
/*JP		     else verbalize("That's not enough, coward!");*/
		     else verbalize("そんなもので済むか，卑怯者！");
		 }
#endif

		dealloc_obj(gold);
		return(1);
	}
	return(0);
}

static int
kick_object(x, y)
xchar x, y;
{
	int range;
	register struct monst *mon, *shkp;
	register struct obj *otmp;
	struct trap *trap;
	boolean costly, insider, shipit;
	boolean isgold;

	/* if a pile, the "top" object gets kicked */
	kickobj = level.objects[x][y];

	/* kickobj should always be set due to conditions of call */
	if(!kickobj || kickobj->otyp == BOULDER
			|| kickobj == uball || kickobj == uchain)
		return(0);

	if((trap = t_at(x,y)) && trap->tseen) {
		if (((trap->ttyp == PIT || trap->ttyp == SPIKED_PIT)
#ifdef POLYSELF
			&& !passes_walls(uasmon)
#endif
			) || trap->ttyp == WEB) {
/*JP			You("can't kick something that's in a %s!",
				trap->ttyp == WEB ? "web" : "pit");*/
			You("%sの中では蹴ることができない",
				trap->ttyp == WEB ? "くもの巣" : "落し穴");
			return(1);
		}
	}

	if(Fumbling && !rn2(3)) {
/*JP		Your("clumsy kick missed.");*/
		Your("不器用な蹴りは外れた．");
		return(1);
	}

	/* range < 2 means the object will not move.	*/
	/* maybe dexterity should also figure here.     */
	range = (int)((ACURRSTR)/2 - kickobj->owt/40);

	if(martial()) range += rnd(3);

	/* Mjollnir is magically too heavy to kick */
	if(kickobj->oartifact == ART_MJOLLNIR) range = 1;

	/* see if the object has a place to move into */
	if(!ZAP_POS(levl[x+u.dx][y+u.dy].typ) || closed_door(x+u.dx, y+u.dy))
		range = 1;

	costly = ((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) && 
		                     costly_spot(x, y));
	insider = (*u.ushops && inside_shop(u.ux, u.uy) &&
				    *in_rooms(x, y, SHOPBASE) == *u.ushops);

	/* a box gets a chance of breaking open here */
	if(Is_box(kickobj)) {
		boolean otrp = kickobj->otrapped;
		struct obj *otmp2, *probj = (struct obj *) 0, *temp;
		long loss = 0L;

/*JP		if(range < 2) pline("THUD!");*/
		if(range < 2) pline("ガン！");

		for(otmp = kickobj->cobj; otmp; otmp = otmp2) {
			otmp2 = otmp->nobj;
			if (objects[otmp->otyp].oc_material == GLASS
			    && !obj_resists(otmp, 33, 100)) {
/*JP				You("hear a muffled shatter.");*/
				You("ガチャンと言う音を聞いた．");
				if(costly) loss += stolen_value(otmp, x, y, 
					    (boolean)shkp->mpeaceful, TRUE);
				if (otmp->quan > 1L)
					useup(otmp);
				else {
					temp = otmp;
					if (otmp == kickobj->cobj) {
						kickobj->cobj = otmp->nobj;
						otmp = (struct obj *) 0;
					} else {
						probj->nobj = otmp->nobj;
						otmp = probj;
					}
					obfree(temp, (struct obj *) 0);
				}
			}
			probj = otmp;
		}
		if(costly && loss) {
		    if(!insider) {
/*JP  		        You("caused %ld zorkmids worth of damage!", loss);*/
  		        You("%ldゴールド分のダメージをくらった！", loss);
			make_angry_shk(shkp, x, y);
		    } else
/*JP		        You("owe %s %ld zorkmids for objects destroyed.",*/
		        You("器物破損で%sに%ldゴールドの借りをつくった．",
			         mon_nam(shkp), loss);
		}

		if (kickobj->olocked) {
		    if (!rn2(5) || (martial() && !rn2(2))) {
/*JP			You("break open the lock!");*/
			You("鍵を壊し開けた！");
			kickobj->olocked = 0;
			kickobj->obroken = 1;
			if (otrp) (void) chest_trap(kickobj, LEG, FALSE);
			return(1);
		    }
		} else {
		    if (!rn2(3) || (martial() && !rn2(2))) {
/*JP			pline("The lid slams open, then falls shut.");*/
			pline("蓋がばたんと開き，閉じた．");
			if (otrp) (void) chest_trap(kickobj, LEG, FALSE);
			return(1);
		    }
		}
		if(range < 2) return(1);
		/* else let it fall through to the next cases... */
	}

	/* fragile objects should not be kicked */
	if (breaks(kickobj, FALSE)) return(1);

	/* potions get a chance of breaking here */
	if(kickobj->oclass == POTION_CLASS) {
		if(rn2(2)) {
/*JP		    You("smash %s %s!",
			  kickobj->quan == 1L ? "the" : "a", xname(kickobj));*/
		    You("%sを打ち割った！",xname(kickobj));
		    potionbreathe(kickobj);
		    if(costly) {
		        long loss = stolen_value(kickobj, kickobj->ox,
				   kickobj->oy, (boolean)shkp->mpeaceful, TRUE);
			if(loss) {
			    if(insider)
/*JP			      You("owe %ld zorkmids for objects destroyed.",*/
			      You("器物破損のため%ldゴールドの借りをつくった．",
				                              loss);
			    else {
/*JP  		           You("caused %ld zorkmids worth of damage!", loss);*/
  		           You("%ldゴールド分のダメージをくらった！", loss);
			          make_angry_shk(shkp, kickobj->ox, 
						                kickobj->oy);
			    }
			}
		    }
		    useupf(kickobj);
		    return(1);
		}
	}

	if(IS_ROCK(levl[x][y].typ)) {
		if ((!martial() && rn2(20) > ACURR(A_DEX))
#ifdef POLYSELF
				|| IS_ROCK(levl[u.ux][u.uy].typ)
#endif
								) {
/*JP			if (Blind) pline("It doesn't come loose.");*/
			if (Blind) pline("びくともしない．");
/*JP			else pline("%s do%sn't come loose.",
				The(distant_name(kickobj, xname)),
				(kickobj->quan == 1L) ? "es" : "");*/
			else pline("%sはびくともしない．",
				The(distant_name(kickobj, xname)));
			return(!rn2(3) || martial());
		}
/*JP		if (Blind) pline("It comes loose.");*/
		if (Blind) pline("ヒビが入ってきた．");
/*JP		else pline("%s come%s loose.",
			   The(distant_name(kickobj, xname)),
			   (kickobj->quan == 1L) ? "s" : "");*/
		else pline("%sにヒビが入ってきた．",
			   The(distant_name(kickobj, xname)));
		freeobj(kickobj);
		newsym(x, y);
		if (costly && (!costly_spot(u.ux, u.uy)
			       || !index(u.urooms, *in_rooms(x, y, SHOPBASE))))
			addtobill(kickobj, FALSE, FALSE, FALSE);
/*JP		if(!flooreffects(kickobj,u.ux,u.uy,"fall")) {*/
		if(!flooreffects(kickobj,u.ux,u.uy,"落ちる")) {
		    kickobj->nobj = fobj;
		    fobj = kickobj;
		    place_object(kickobj, u.ux, u.uy);
		    stackobj(kickobj);
		    newsym(u.ux, u.uy);
		}
		return(1);
	}

	isgold = (kickobj->otyp == GOLD_PIECE);

	/* too heavy to move.  range is calculated as potential distance from
	 * player, so range == 2 means the object may move up to one square
	 * from its current position
	 */
	if(range < 2 || (isgold && kickobj->quan > 300L)) {
/*JP	    if(!Is_box(kickobj)) pline("Thump!");*/
	    if(!Is_box(kickobj)) pline("ゴツン！");
	    return(!rn2(3) || martial());
	}

	if (kickobj->quan > 1L && !isgold) (void) splitobj(kickobj, 1L);

	freeobj(kickobj);
	newsym(x, y);
	mon = bhit(u.dx, u.dy, range, KICKED_WEAPON,
		   (int (*)()) 0, (int (*)()) 0, kickobj);

	/* a flag to "drop" the object to the next level */
	shipit = (!mon && down_gate(bhitpos.x, bhitpos.y) != -1);

	if(mon) {
	    notonhead = (mon->mx != bhitpos.x || mon->my != bhitpos.y);
	    /* awake monster if sleeping */
	    wakeup(mon);
	    if(isgold ? ghitm(mon, kickobj) :	/* caught? */
	       thitmonst(mon, kickobj))		/* hit? */
		return(1);
	}
	if(costly &&
	   (!costly_spot(bhitpos.x,bhitpos.y) || shipit ||
	    *in_rooms(bhitpos.x, bhitpos.y, 0) != *in_rooms(x, y, 0))) {

	    if(shipit && ship_object(kickobj, bhitpos.x, bhitpos.y, costly))
		return(1);

	    if(isgold)
		costly_gold(x, y, kickobj->quan);
	    else if(costly_spot(u.ux, u.uy) &&
		    index(u.urooms, *in_rooms(x, y, 0)))
		addtobill(kickobj, FALSE, FALSE, FALSE);
	    else (void)stolen_value(kickobj, x, y, FALSE, FALSE);
	}

	if(shipit && ship_object(kickobj, bhitpos.x, bhitpos.y, costly))
		return(1);
/*JP	if(flooreffects(kickobj,bhitpos.x,bhitpos.y,"fall")) return(1);*/
	if(flooreffects(kickobj,bhitpos.x,bhitpos.y,"落ちる")) return(1);
	kickobj->nobj = fobj;
	fobj = kickobj;
	place_object(kickobj, bhitpos.x, bhitpos.y);
	stackobj(kickobj);
	newsym(kickobj->ox, kickobj->oy);
	return(1);
}

static char *
kickstr()
{
	static NEARDATA char buf[BUFSZ];

/*JP	if (kickobj) Sprintf(buf, "kicking %s", doname(kickobj));*/
	if (kickobj) Sprintf(buf, "%sを蹴って", doname(kickobj));
	else {
/*JP	  Strcpy(buf, "kicking ");*/
	  Strcpy(buf, "");
/*JP	  if (IS_STWALL(maploc->typ)) Strcat(buf, "a wall");*/
	  if (IS_STWALL(maploc->typ)) Strcat(buf, "壁を");
/*JP	  else if (IS_ROCK(maploc->typ)) Strcat(buf, "a rock");*/
	  else if (IS_ROCK(maploc->typ)) Strcat(buf, "岩を");
/*JP	  else if (IS_THRONE(maploc->typ)) Strcat(buf, "a throne");*/
	  else if (IS_THRONE(maploc->typ)) Strcat(buf, "玉座を");
#ifdef SINKS
/*JP	  else if (IS_SINK(maploc->typ)) Strcat(buf, "a sink");*/
	  else if (IS_SINK(maploc->typ)) Strcat(buf, "流し台を");
#endif
/*JP	  else if (IS_ALTAR(maploc->typ)) Strcat(buf, "an altar");*/
	  else if (IS_ALTAR(maploc->typ)) Strcat(buf, "祭壇を");
/*JP	  else if (IS_DRAWBRIDGE(maploc->typ)) Strcat(buf, "the drawbridge");*/
	  else if (IS_DRAWBRIDGE(maploc->typ)) Strcat(buf, "跳ね橋を");
	  else {
		switch (maploc->typ) {
		case STAIRS:
/*JP			Strcat(buf, "the stairs");*/
			Strcat(buf, "階段を");
			break;
		case LADDER:
/*JP			Strcat(buf, "a ladder");*/
			Strcat(buf, "はしごを");
			break;
		default:
/*JP			Strcat(buf, "something wierd");*/
			Strcat(buf, "何か妙なものを");
			break;
		}
	  }
	  Strcat(buf, "蹴って");
	}
	return buf;
}

int
dokick()
{
	register int x, y;
	int avrg_attrib;
	register struct monst *mtmp;
	s_level *slev;
	boolean no_kick = FALSE;

#ifdef POLYSELF
	if (nolimbs(uasmon)) {
/*JP		You("have no legs to kick with.");*/
		You("何かを蹴ろうにも足がない．");
		no_kick = TRUE;
	} else if (verysmall(uasmon)) {
/*JP		You("are too small to do any kicking.");*/
		You("何かを蹴るには小さすぎる．");
		no_kick = TRUE;
	} else
#endif
	if (Wounded_legs) {
/*JP		Your("%s %s in no shape for kicking.",*/
		Your("%sは怪我をしており蹴れない．",
		      ((Wounded_legs & BOTH_SIDES)==BOTH_SIDES)
			? (const char *)makeplural(body_part(LEG)) : body_part(LEG));
/*JP		      ((Wounded_legs & BOTH_SIDES)==BOTH_SIDES) ? "are" : "is");*/
		no_kick = TRUE;
	} else if (near_capacity() > SLT_ENCUMBER) {
/*JP		Your("load is too heavy to balance yourself for a kick.");*/
		You("たくさんものを持ちすぎて蹴りのためのバランスがとれない．");
		no_kick = TRUE;
	} else if (u.uinwater && !rn2(2)) {
/*JP		Your("slow motion kick doesn't hit anything.");*/
		Your("遅い動きの蹴りでは命中しようがない．");
		no_kick = TRUE;
	} else if (u.utrap) {
		switch (u.utraptype) {
		    case TT_PIT:
/*JP			pline("There's not enough room to kick down here.");*/
			pline("落し穴にはまっているので，蹴れない．");
			break;
		    case TT_WEB:
		    case TT_BEARTRAP:
/*JP			You("can't move your %s!", body_part(LEG));*/
			You("%sを動かすことができない！", body_part(LEG));
			break;
		    default:
			break;
		}
		no_kick = TRUE;
	}

	if (no_kick) {
		/* discard direction typeahead, if any */
		display_nhwindow(WIN_MESSAGE, TRUE);	/* --More-- */
		return 0;
	}

	if(!getdir(NULL)) return(0);
	if(!u.dx && !u.dy) return(0);

	x = u.ux + u.dx;
	y = u.uy + u.dy;
	avrg_attrib = (ACURRSTR+ACURR(A_DEX)+ACURR(A_CON))/3;

	if(u.uswallow) {
		switch(rn2(3)) {
/*JP		case 0:  You("can't move your %s!", body_part(LEG));*/
		case 0:  You("%sを動かすことができない！", body_part(LEG));
			 break;
		case 1:  if (is_animal(u.ustuck->data)) {
/*JP				pline("%s burps loudly.", Monnam(u.ustuck));*/
				pline("%sは大きなゲップをした．", Monnam(u.ustuck));
				break;
			 }
/*JP		default: Your("feeble kick has no effect."); break;*/
		default: Your("弱々しい蹴りは効果がない"); break;
		}
		return(1);
	}

	wake_nearby();
	u_wipe_engr(2);

	maploc = &levl[x][y];

	/* The next four tests should stay in      */
	/* their present order: monsters, objects, */
	/* non-doors, doors.			   */

	if(MON_AT(x, y)) {
		struct permonst *mdat = m_at(x,y)->data;
		kick_monster(x, y);
		if((Is_airlevel(&u.uz) || Levitation) && flags.move) {
		    int range;

		    range = ((int)uasmon->cwt + (weight_cap() + inv_weight()));
		    if (range < 1) range = 1; /* divide by zero avoidance */
		    range = (3*(int)mdat->cwt) / range;

		    if(range < 1) range = 1;
		    hurtle(-u.dx, -u.dy, range);
		}
		return(1);
	}

	kickobj = (struct obj *)0;
	if (OBJ_AT(x, y) &&
	    (!Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)
	     || sobj_at(BOULDER,x,y))) {
		if(kick_object(x, y)) {
		    if(Is_airlevel(&u.uz))
			hurtle(-u.dx, -u.dy, 1); /* assume it's light */
		    return(1);
		}
		goto ouch;
	}

	if(!IS_DOOR(maploc->typ)) {
		if(maploc->typ == SDOOR) {
		    if(!Levitation && rn2(30) < avrg_attrib) {
/*JP			pline("Crash!  You kick open a secret door!");*/
			pline("ガシャン！あなたは秘密の扉を蹴りやぶった！");
			exercise(A_DEX, TRUE);
			maploc->typ = DOOR;
			if(maploc->doormask & D_TRAPPED) {
			    maploc->doormask = D_NODOOR;
/*JP			    b_trapped("door", FOOT);*/
			    b_trapped("扉", FOOT);
			} else
			    maploc->doormask = D_ISOPEN;
			if (Blind)
			    feel_location(x,y);	/* we know its gone */
			else
			    newsym(x,y);
			unblock_point(x,y);	/* vision */
			return(1);
		    } else goto ouch;
		}
		if(maploc->typ == SCORR) {
		    if(!Levitation && rn2(30) < avrg_attrib) {
/*JP			pline("Crash!  You kick open a secret passage!");*/
			pline("ガシャン！あなたは秘密の通路を蹴りやぶった！");
			exercise(A_DEX, TRUE);
			maploc->typ = CORR;
			if (Blind)
			    feel_location(x,y);	/* we known its gone */
			else
			    newsym(x,y);
			unblock_point(x,y);	/* vision */
			return(1);
		    } else goto ouch;
		}
		if(IS_THRONE(maploc->typ)) {
		    register int i;
		    if(Levitation) goto dumb;
		    if((Luck < 0 || maploc->doormask) && !rn2(3)) {
			maploc->typ = ROOM;
			maploc->doormask = 0; /* don't leave loose ends.. */
			mkgold((long)rnd(200), x, y);
			if (Blind)
/*JP			    pline("CRASH!  You destroy it.");*/
			    pline("ガシャン！あなたは何かを破壊した．");
			else {
/*JP			    pline("CRASH!  You destroy the throne.");*/
			    pline("ガシャン！あなたは玉座を破壊した．");
			    newsym(x, y);
			}
			exercise(A_DEX, TRUE);
			return(1);
		    } else if(Luck > 0 && !rn2(3) && !maploc->looted) {
			mkgold((long) rn1(201, 300), x, y);
			i = Luck + 1;
			if(i > 6) i = 6;
			while(i--) (void) mkobj_at(GEM_CLASS, x, y, TRUE);
			if (Blind)
/*JP			    You("kick something loose!");*/
			    You("なにかを蹴り散らした！");
			else {
/*JP			    You("kick loose some ornamental coins and gems!");*/
			    You("装飾用の金貨や宝石を蹴り散らした！");
			    newsym(x, y);
			}
			/* prevent endless milking */
			maploc->looted = T_LOOTED;
			return(1);
		    } else if (!rn2(4)) {
			if(dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz)) {
			    fall_through(FALSE);
			    return(1);
			} else goto ouch;
		    }
		    goto ouch;
		}
		if(IS_ALTAR(maploc->typ)) {
		    if(Levitation) goto dumb;
/*JP		    You("kick %s.",(Blind ? "something" : "the altar"));*/
		    You("%sを蹴った．",(Blind ? "何かを" : "祭壇を"));
		    if(!rn2(3)) goto ouch;
		    altar_wrath(x, y);
		    exercise(A_DEX, TRUE);
		    return(1);
		}
#ifdef SINKS
		if(IS_SINK(maploc->typ)) {
		    if(Levitation) goto dumb;
		    if(rn2(5)) {
			if(flags.soundok)
/*JP			    pline("Klunk!  The pipes vibrate noisily.");*/
			    pline("ガラン！パイプはうるさく振動した．");
/*JP			else pline("Klunk!");*/
			else pline("ガラン！");
			exercise(A_DEX, TRUE);
			return(1);
		    } else if(!(maploc->looted & S_LPUDDING) && !rn2(3) &&
			  !(mons[PM_BLACK_PUDDING].geno &
				(G_GENOD | G_EXTINCT))) {
			if (Blind)
/*JP			    You("hear a gushing sound.");*/
			    You("なにかが噴出する音を聞いた．");
			else
/*JP			    pline("A %s ooze gushes up from the drain!",*/
			    pline("%s液体が排水口からにじみ出た！",
					  Hallucination ? hcolor() : Black);
			(void) makemon(&mons[PM_BLACK_PUDDING], x, y);
			exercise(A_DEX, TRUE);
			newsym(x,y);
			maploc->looted |= S_LPUDDING;
			return(1);
		    } else if(!(maploc->looted & S_LDWASHER) && !rn2(3) &&
# ifndef POLYSELF
			      poly_gender() != 2 &&
# endif
			      !(mons[poly_gender() == 1 ?
				      PM_INCUBUS : PM_SUCCUBUS].geno &
				  (G_GENOD | G_EXTINCT))) {
			/* can't resist... */
/*JP			pline("%s returns!", (Blind ? "Something" :
							"The dish washer"));*/
			pline("%sは戻った！", (Blind ? "何か" :
							"皿洗い"));
			if (makemon(&mons[poly_gender() == 1 ?
				PM_INCUBUS : PM_SUCCUBUS], x, y)) newsym(x,y);
			maploc->looted |= S_LDWASHER;
			exercise(A_DEX, TRUE);
			return(1);
		    } else if(!rn2(3)) {
/*JP			pline("Flupp!  %s.", (Blind ?
				      "You hear a sloshing sound" :
				      "Muddy waste pops up from the drain"));*/
			pline("うわ！%s．", (Blind ?
				      "あなたは，バチャバチャする音を聞いた" :
				      "排水口から泥々の廃棄物が登ってくる．"));
			if(!(maploc->looted & S_LRING)) { /* once per sink */
			    if (!Blind)
/*JP				You("see a ring shining in its midst.");*/
				You("その中央に光る指輪を見つけた．");
			    (void) mkobj_at(RING_CLASS, x, y, TRUE);
			    newsym(x, y);
			    exercise(A_DEX, TRUE);
			    exercise(A_WIS, TRUE);	/* a discovery! */
			    maploc->looted |= S_LRING;
			}
			return(1);
		    }
		    goto ouch;
		}
#endif
		if (maploc->typ == STAIRS || maploc->typ == LADDER ||
						    IS_STWALL(maploc->typ)) {
		    if(!IS_STWALL(maploc->typ) && maploc->ladder == LA_DOWN)
			goto dumb;
ouch:
/*JP		    pline("Ouch!  That hurts!");*/
		    pline("いてっ！怪我した！");
		    exercise(A_DEX, FALSE);
		    exercise(A_STR, FALSE);
		    if (Blind) feel_location(x,y); /* we know we hit it */
		    if(!rn2(3)) set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
		    losehp(rnd(ACURR(A_CON) > 15 ? 3 : 5), kickstr(),
			KILLED_BY);
		    if(Is_airlevel(&u.uz) || Levitation)
			hurtle(-u.dx, -u.dy, rn1(2,4)); /* assume it's heavy */
		    return(1);
		}
		if (is_drawbridge_wall(x,y) >= 0) {
/*JP		    pline("The drawbridge is unaffected.");*/
		    pline("跳ね橋には影響がない．");
		    if(Levitation)
			hurtle(-u.dx, -u.dy, rn1(2,4)); /* it's heavy */
		    return(1);
		}
		goto dumb;
	}

	if(maploc->doormask == D_ISOPEN ||
	   maploc->doormask == D_BROKEN ||
	   maploc->doormask == D_NODOOR) {
dumb:
		exercise(A_DEX, FALSE);
		if (martial() || ACURR(A_DEX) >= 16 || rn2(3)) {
/*JP			You("kick at empty space.");*/
			You("何もない空間を蹴った．");
		} else {
/*JP			pline("Dumb move!  You strain a muscle.");*/
			pline("ばかげた動きだ！筋肉を痛めた．");
			exercise(A_STR, FALSE);
			set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
		}
		if ((Is_airlevel(&u.uz) || Levitation) && rn2(2)) {
		    hurtle(-u.dx, -u.dy, 1);
		    return 1;		/* you moved, so use up a turn */
		}
		return(0);
	}

	/* not enough leverage to kick open doors while levitating */
	if(Levitation) goto ouch;

	exercise(A_DEX, TRUE);
	/* door is known to be CLOSED or LOCKED */
	if(rnl(35) < avrg_attrib + (!martial() ? 0 : ACURR(A_DEX))) {
		/* break the door */
		if(maploc->doormask & D_TRAPPED) {
/*JP		    if (flags.verbose) You("kick the door.");*/
		    if (flags.verbose) You("扉を蹴った．");
		    exercise(A_STR, FALSE);
		    maploc->doormask = D_NODOOR;
/*JP		    b_trapped("door", FOOT);*/
		    b_trapped("扉", FOOT);
		} else if(ACURR(A_STR) > 18 && !rn2(5) &&
			  !*in_rooms(x, y, SHOPBASE)) {
/*JP		    pline("As you kick the door, it shatters to pieces!");*/
		    pline("扉を蹴ると，こなごなにくだけた！");
		    exercise(A_STR, TRUE);
		    maploc->doormask = D_NODOOR;
		} else {
/*JP		    pline("As you kick the door, it crashes open!");*/
		    pline("扉を蹴ると，壊れて開いた！");
		    exercise(A_STR, TRUE);
		    if(*in_rooms(x, y, SHOPBASE)) {
			add_damage(x, y, 400L);
/*JP			pay_for_damage("break");*/
			pay_for_damage("破壊する");
		    }
		    maploc->doormask = D_BROKEN;
		}
		if (Blind)
		    feel_location(x,y);		/* we know we broke it */
		else
		    newsym(x,y);
		unblock_point(x,y);		/* vision */
		if ((slev = Is_special(&u.uz)) && slev->flags.town)
		  for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		    if((mtmp->data == &mons[PM_WATCHMAN] ||
			mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
			couldsee(mtmp->mx, mtmp->my) &&
			mtmp->mpeaceful) {
/*JP			pline("%s yells:", Amonnam(mtmp));*/
			pline("%sは叫んだ：", Amonnam(mtmp));
/*JP			verbalize("Halt, thief!  You're under arrest!");*/
			verbalize("止まれ泥棒！おまえを逮捕する！");
			(void) angry_guards(FALSE);
			break;
		    }
		  }
	} else {
	    if (Blind) feel_location(x,y);	/* we know we hit it */
	    exercise(A_STR, TRUE);
/*JP	    pline("WHAMMM!!!");*/
	    pline("ぐぁぁぁん");
	    if ((slev = Is_special(&u.uz)) && slev->flags.town)
	      for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	        if((mtmp->data == &mons[PM_WATCHMAN] ||
		  mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
	          couldsee(mtmp->mx, mtmp->my) &&
	          mtmp->mpeaceful) {
		
/*JP		  pline("%s yells:", Amonnam(mtmp));*/
		  pline("%sは叫んだ：", Amonnam(mtmp));
		  if(levl[x][y].looted & D_WARNED) {
/*JP			verbalize("Halt, vandal!  You're under arrest!");*/
			verbalize("止まれ野蛮人！おまえを逮捕する！");
			(void) angry_guards(FALSE);
		  } else {
/*JP			verbalize("Hey, stop damaging that door!");*/
			verbalize("おい，扉を破壊するのをやめろ！");
			levl[x][y].looted |= D_WARNED;
		  }
		  break;
	        }
	      }
	}
	return(1);
}

static const char *
gate_str(gate)
register xchar gate;
{
	const char *optr;

	switch(gate) {
	    case 0:
/*JP	    case 4:  optr = "through the trap door."; break;*/
	    case 4:  optr = "罠の扉を通り"; break;
	    case 1:
/*JP	    case 3:  optr = "down the stairs."; break;
	    case 2:  optr = "down the ladder."; break;
	    default: optr = "down out of sight."; break;*/
	    case 3:  optr = "階段から"; break;
	    case 2:  optr = "はしごから"; break;
	    default: optr = "視界から"; break;
	}
	return(optr);
}

static
void
drop_to(cc, loc)
coord *cc;
register xchar loc;
{
	switch(loc) {
	    case 0: if(In_endgame(&u.uz) || (Is_botlevel(&u.uz) &&
			      !Is_stronghold(&u.uz))) {
			cc->y = 0;
			return;
		    }
		    if(Is_stronghold(&u.uz)) {
			cc->x = valley_level.dnum;
			cc->y = valley_level.dlevel;
			break;
		    } /* else fall to the next cases */
	    case 1:
	    case 2:
		    cc->x = u.uz.dnum;
		    cc->y = u.uz.dlevel + 1;
		    break;
	    case 3:
		    cc->x = sstairs.tolev.dnum;
		    cc->y = sstairs.tolev.dlevel;
		    break;
	    default:
		    cc->y = 0;
	}
}

void
impact_drop(missile, x, y, dlev)
register struct obj *missile;
register xchar x, y, dlev;
{
	xchar toloc;
	register struct obj *obj, *obj2;
	register struct monst *shkp;
	long oct, dct, price, debit, robbed;
	boolean angry, costly, isrock;
	coord cc;

	if(!OBJ_AT(x, y)) return;

	toloc = down_gate(x, y);
	drop_to(&cc, toloc);
	if (!cc.y) return;

	if (dlev) {
		/* send objects next to player falling through trap door.
		 * checked in obj_delivery().
		 */
		toloc = 4;
		cc.y = dlev;
	}

	costly = costly_spot(x, y);
	price = debit = robbed = 0L;
	angry = FALSE;
	shkp = (struct monst *) 0;
	/* if 'costly', we must keep a record of ESHK(shkp) before
	 * it undergoes changes through the calls to stolen_value.
	 * the angry bit must be reset, if needed, in this fn, since
	 * stolen_value is called under the 'silent' flag to avoid
	 * unsavory pline repetitions.
	 */
	if(costly) {
	    if((shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 
	                                           (struct monst *)0) {
		debit	= ESHK(shkp)->debit;
		robbed	= ESHK(shkp)->robbed;
		angry	= !shkp->mpeaceful;
	    }
	}

	isrock = (missile && missile->otyp == ROCK);
	oct = dct = 0L;
	for(obj = level.objects[x][y]; obj; obj = obj2) {
		obj2 = obj->nexthere;
		if(obj == missile) continue;
		/* number of objects in the pile */
		oct += obj->quan;
		if(obj == uball || obj == uchain) continue;
		/* boulders can fall too, but rarely & never due to rocks */
		if((isrock && obj->otyp == BOULDER) ||
		   rn2(obj->otyp == BOULDER ? 30 : 3)) continue;
		freeobj(obj);

		if(costly) {
		    price += stolen_value(obj, x, y,
				(costly_spot(u.ux, u.uy) &&
				 index(u.urooms, *in_rooms(x, y, SHOPBASE))),
				TRUE);
		    /* set obj->no_charge to 0 */
		    if (Has_contents(obj))
			picked_container(obj);	/* does the right thing */
		    if (obj->otyp != GOLD_PIECE)
			obj->no_charge = 0;
		}
		obj->nobj = migrating_objs;
		migrating_objs = obj;

		obj->ox = cc.x;
		obj->oy = cc.y;
		obj->owornmask = (long)toloc;

		/* number of fallen objects */
		dct += obj->quan;
	}

	if (dct) {	/* at least one object fell */
/*JP	    const char *what = (dct == 1L ? "object falls" : "objects fall");*/
	    const char *what = "物";
	    if (missile)
/*JP		pline("From the impact, %sother %s.",
			dct == oct ? "the " : dct == 1L ? "an" : "", what);*/
		pline("衝撃で，他の%sが落ちた．",what);
	    else
		if (oct == dct) {
/*JP		    pline("%s adjacent %s %s",*/
/*JP			    dct == 1L ? "The" : "All the",*/
		    pline("近くにあった%sが%s落ちた．",
			    what, gate_str(toloc));
		} else {
/*JP		    pline("%s adjacent %s %s",
			    dct == 1L ? "One of the" : "Some of the",
			    dct == 1L ? "objects falls" : what,
			    gate_str(toloc));*/
		    pline("近くにあった%s%s%s落ちた．",
			    what,
			    dct == 1L ? "は" : "のいくつかは",
			    gate_str(toloc));
		}
	}

	if(costly && shkp && price) {
		if(ESHK(shkp)->robbed > robbed) {
/*JP		    You("removed %ld zorkmids worth of goods!", price);*/
		    You("%ldゴールド分の品物を取りさった！",price);
		    if(cansee(shkp->mx, shkp->my)) {
			if(ESHK(shkp)->customer[0] == 0)
			    (void) strncpy(ESHK(shkp)->customer,
					   plname, PL_NSIZ);
			if(angry)
/*JP			    pline("%s is infuriated!", Monnam(shkp));*/
			    pline("%sは激怒した！", Monnam(shkp));
/*JP			else pline("\"%s, you are a thief!\"", plname);*/
			else pline("「%s，おまえは盗賊だな！」", plname);
/*JP		    } else  You("hear a scream, \"Thief!\"");*/
		    } else  You("金切り声を聞いた「泥棒！」");
		    hot_pursuit(shkp);
		    (void) angry_guards(FALSE);
		    return;
		}
		if(ESHK(shkp)->debit > debit)
/*JP		    You("owe %s %ld zorkmids for goods lost.",*/
		    You("品物消失のため%sに%ldゴールドの借りをつくった．",
			Monnam(shkp),
			(ESHK(shkp)->debit - debit));
	}

}

/* NOTE: ship_object assumes otmp was FREED from fobj or invent.
 * <x,y> is the point of drop.  otmp is _not_ an <x,y> resident:
 * otmp is either a kicked, dropped, or thrown object.
 */
boolean
ship_object(otmp, x, y, shop_floor_obj)
register xchar  x, y;
register struct obj *otmp;
register boolean shop_floor_obj;
{
	register xchar ox, oy;
	register xchar toloc = down_gate(x, y);
	/* toloc -- destination location: */
		/*	0: rnd loc,
		 *	1: <,
		 *	2: < ladder,
		 *	3: sstairs up
		 *	4: near player (trapdoor)
		 */
	coord cc;
	/* objects always fall down ladder, a chance of stay otherwise */
	register boolean nodrop = (toloc != 2 && rn2(3));
	register boolean unpaid, container, impact = FALSE;
	int n = 0;

	if(!otmp) return(FALSE);
	if(toloc == -1) return(FALSE);

	drop_to(&cc, toloc);
	if(!cc.y) return(FALSE);

	container = Has_contents(otmp);

	unpaid = (otmp->unpaid || (container && count_unpaid(otmp->cobj)));

	if(OBJ_AT(x, y)) {
	    register struct obj *obj;

	    for(obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if(obj != otmp) n++;
	    if(n) impact = TRUE;
	}

	otransit_msg(otmp, toloc, nodrop, n);

	if(nodrop) {
	    otmp->nobj = fobj;
	    fobj = otmp;
	    place_object(otmp, x, y);
	    stackobj(otmp);
	    newsym(otmp->ox, otmp->oy);
	    if(impact) goto chain_reaction;
	    else return(TRUE);
	}

	if(unpaid || shop_floor_obj) {
	    if(unpaid) {
		subfrombill(otmp, shop_keeper(*u.ushops));
		(void)stolen_value(otmp, u.ux, u.uy, TRUE, FALSE);
	    } else {
	        ox = otmp->ox;
		oy = otmp->oy;
		(void)stolen_value(otmp, ox, oy,
			  (costly_spot(u.ux, u.uy) &&
			      index(u.urooms, *in_rooms(ox, oy, SHOPBASE))),
			  FALSE);
	    }
	    /* set otmp->no_charge to 0 */
	    if(container)
	        picked_container(otmp); /* happens to do the right thing */
	    if(otmp->otyp != GOLD_PIECE)
	        otmp->no_charge = 0;
	}

	otmp->nobj = migrating_objs;
	migrating_objs = otmp;

	otmp->ox = cc.x;
	otmp->oy = cc.y;
	otmp->owornmask = (long)toloc;
chain_reaction:
	if(impact) {
	    /* the objs impacted may be in a shop other than
	     * the one in which the hero is located.  another
	     * check for a shk is made in impact_drop.  it is, e.g.,
	     * possible to kick/throw an object belonging to one
	     * shop into another shop through a gap in the wall,
	     * and cause objects belonging to the other shop to
	     * fall down a trapdoor--thereby getting two shopkeepers
	     * angry at the hero in one shot.
	     */
	    impact_drop(otmp, x, y, 0);
	    newsym(x,y);
	}
	return(TRUE);
}

void
obj_delivery()
{
	register struct obj *otmp, *otmp0 = (struct obj *)0, *otmp2;

	for(otmp = migrating_objs; otmp; otmp = otmp2) {
	    otmp2 = otmp->nobj;

	    if(otmp->ox == u.uz.dnum && otmp->oy == u.uz.dlevel) {
		if(otmp == migrating_objs)
		    migrating_objs = otmp->nobj;
		else
		    otmp0->nobj = otmp->nobj;
		otmp->nobj = fobj;
		fobj = otmp;

		switch((xchar)otmp->owornmask) {
		    xchar *xlocale, *ylocale;

		    case 1: xlocale = &xupstair; ylocale = &yupstair;
			    goto common;
		    case 2: xlocale = &xupladder; ylocale = &yupladder;
			    goto common;
		    case 3: xlocale = &sstairs.sx; ylocale = &sstairs.sy;
			    goto common;
		    case 4: { /* hero falls down trapdoor with objects */
			      xchar nx, ny;
			      int cnt = 0;

			      do {
				  nx = u.ux - 1 + rn2(3);
				  ny = u.uy - 1 + rn2(3);
			      } while((nx < 1 || nx > COLNO-2 ||
				       ny < 1 || ny > ROWNO-2 ||
				       is_pool(nx,ny) || is_lava(nx,ny) ||
				       !ACCESSIBLE(levl[nx][ny].typ) ||
				       closed_door(nx, ny)
				      ) && cnt++ <= 50);

			      if(cnt >= 50) goto scatter; /* safety */
			      xlocale = &nx;
			      ylocale = &ny;
			    }
common:
			    if (*xlocale && *ylocale) {
				place_object(otmp, *xlocale, *ylocale);
				stackobj(otmp);
				break;
			    } /* else fall through */
		    default:
scatter:
			    /* set dummy coordinates because there's no
			       current position for rloco() to update */
			    otmp->ox = otmp->oy = 0;
			    rloco(otmp);
			    break;
		}
		otmp->owornmask = 0L;
	    } else
		otmp0 = otmp;
	}
}

static void
otransit_msg(otmp, loc, nodrop, num)
register struct obj *otmp;
register xchar loc;
register boolean nodrop;
int num;
{
	char obuf[BUFSZ];

/*JP	Sprintf(obuf, "%s%s",
		 (otmp->otyp == CORPSE &&
			type_is_pname(&mons[otmp->corpsenm])) ? "" : "The ",*/
	Sprintf(obuf, "%sは",
		 xname(otmp));

	if(num) { /* means: other objects are impacted */
/*JP	    Sprintf(eos(obuf), " hit%s %s object%s",
		      otmp->quan == 1L ? "s" : "",
		      num == 1 ? "another" : "other",
		      num > 1 ? "s" : "");*/
	    Sprintf(eos(obuf), "他の物体に命中して");
	    if(nodrop)
/*JP		Sprintf(eos(obuf), " and stop%s.",
				 otmp->quan == 1L ? "s" : "");*/
		Sprintf(eos(obuf), "止った．");
	    else
/*JP		Sprintf(eos(obuf), " and fall%s %s",
				otmp->quan == 1L ? "s" : "", gate_str(loc));*/
		Sprintf(eos(obuf), "%s落ちた．",
				gate_str(loc));
	    pline(obuf);
	} else if(!nodrop)
/*JP	    pline("%s fall%s %s", obuf,
		  otmp->quan == 1L ? "s" : "",*/
	    pline("%s%s落ちた．", obuf,
		  gate_str(loc));
}

xchar
down_gate(x, y)
xchar x, y;
{
	register struct trap *ttmp = t_at(x, y);

#ifdef MULDGN	/* this matches the player restriction in goto_level() */
	if (on_level(&u.uz, &qstart_level) && !ok_to_quest()) return -1;
#endif
	if(ttmp && ttmp->ttyp == TRAPDOOR && ttmp->tseen) return 0;
	if(xdnstair == x && ydnstair == y) return 1;
	if(xdnladder == x && ydnladder == y) return 2;
	if(sstairs.sx == x && sstairs.sy == y && !sstairs.up) return 3;
	return -1;
}

/*dokick.c*/
