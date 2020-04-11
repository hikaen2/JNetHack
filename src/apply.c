/*	SCCS Id: @(#)apply.c	3.3	1999/10/10	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "edog.h"

#ifdef OVLB

static const char tools[] = { TOOL_CLASS, WEAPON_CLASS, WAND_CLASS, 0 };
static const char tools_too[] = { ALL_CLASSES, TOOL_CLASS, POTION_CLASS,
					  WEAPON_CLASS, WAND_CLASS, 0 };

#ifdef TOURIST
STATIC_DCL int FDECL(use_camera, (struct obj *));
#endif
STATIC_DCL int FDECL(use_towel, (struct obj *));
STATIC_DCL boolean FDECL(its_dead, (int,int,int *));
STATIC_DCL int FDECL(use_stethoscope, (struct obj *));
STATIC_DCL void FDECL(use_whistle, (struct obj *));
STATIC_DCL void FDECL(use_magic_whistle, (struct obj *));
STATIC_DCL void FDECL(use_leash, (struct obj *));
STATIC_DCL int FDECL(use_mirror, (struct obj *));
STATIC_DCL void FDECL(use_bell, (struct obj *));
STATIC_DCL void FDECL(use_candelabrum, (struct obj *));
STATIC_DCL void FDECL(use_candle, (struct obj *));
STATIC_DCL void FDECL(use_lamp, (struct obj *));
STATIC_DCL void FDECL(light_cocktail, (struct obj *));
STATIC_DCL void FDECL(use_tinning_kit, (struct obj *));
STATIC_DCL void FDECL(use_figurine, (struct obj *));
STATIC_DCL void FDECL(use_grease, (struct obj *));
STATIC_DCL void FDECL(use_trap, (struct obj *));
STATIC_PTR int NDECL(set_trap);		/* occupation callback */
STATIC_DCL int FDECL(use_whip, (struct obj *));
STATIC_DCL int FDECL(use_pole, (struct obj *));
STATIC_DCL int FDECL(use_grapple, (struct obj *));
STATIC_DCL int FDECL(do_break_wand, (struct obj *));
STATIC_DCL boolean FDECL(figurine_location_checks,
				(struct obj *, coord *, BOOLEAN_P));

#ifdef	AMIGA
void FDECL( amii_speaker, ( struct obj *, char *, int ) );
#endif

/*JPstatic char no_elbow_room[] = "don't have enough elbow-room to maneuver.";*/
static char no_elbow_room[] = "それをするだけのゆとりがない．";

#ifdef TOURIST
STATIC_OVL int
use_camera(obj)
	struct obj *obj;
{
	register struct monst *mtmp;

	if(Underwater) {
/*JP		pline("Using your camera underwater would void the warranty.");*/
		pline("そんなことをしたら保証が効かなくなる．");
		return(0);
	}
	if(!getdir((char *)0)) return(0);

	if (obj->spe <= 0) {
		pline(nothing_happens);
		return (1);
	}
	obj->spe--;
	if (obj->cursed && !rn2(2)) {
		(void) zapyourself(obj, TRUE);
	} else if (u.uswallow) {
/*JP		You("take a picture of %s %s.", s_suffix(mon_nam(u.ustuck)),
		    is_animal(u.ustuck->data) ? "stomach" : "interior");*/
		You("%sの%sの写真を撮った．", mon_nam(u.ustuck),
		    is_animal(u.ustuck->data) ? "胃" : "内部");
	} else if (u.dz) {
/*JP		You("take a picture of the %s.",*/
		You("%sの写真を撮った",
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
	} else if (!u.dx && !u.dy) {
		(void) zapyourself(obj, TRUE);
	} else if ((mtmp = bhit(u.dx, u.dy, COLNO, FLASHED_LIGHT,
				(int FDECL((*),(MONST_P,OBJ_P)))0,
				(int FDECL((*),(OBJ_P,OBJ_P)))0,
				obj)) != 0) {
		obj->ox = u.ux,  obj->oy = u.uy;
		(void) flash_hits_mon(mtmp, obj);
	}
	return 1;
}
#endif

STATIC_OVL int
use_towel(obj)
	struct obj *obj;
{
	if(!freehand()) {
/*JP		You("have no free %s!", body_part(HAND));*/
		You("%sの自由が効かない！", body_part(HAND));
		return 0;
	} else if (obj->owornmask) {
/*JP		You("cannot use it while you're wearing it!");*/
		You("それを身につけているので使用できない！");
		return 0;
	} else if (obj->cursed) {
		long old;
		switch (rn2(3)) {
		case 2:
		    old = Glib;
		    Glib += rn1(10, 3);
/*JP		    Your("%s %s!", makeplural(body_part(HAND)),
			(old ? "are filthier than ever" : "get slimy"));
*/
		    Your("%sは%s！", makeplural(body_part(HAND)),
			(old ? "ますます汚なくなった" : "ぬるぬるになった"));
		    return 1;
		case 1:
		    if (!ublindf) {
			old = u.ucreamed;
			u.ucreamed += rn1(10, 3);
/*JP			pline("Yecch! Your %s %s gunk on it!", body_part(FACE),
			      (old ? "has more" : "now has"));
*/
			pline("ゲェー！あなたの%sは%sべとべとになった！", body_part(FACE),
			      (old ? "もっと" : ""));
			make_blinded(Blinded + (long)u.ucreamed - old, TRUE);
		    } else {
/*JP			const char *what = (ublindf->otyp == LENSES) ?
					    "lenses" : "blindfold";*/
			const char *what = (ublindf->otyp == LENSES) ?
					    "レンズ" : "目隠し";
			if (ublindf->cursed) {
/*JP			    You("push your %s %s.", what,
				rn2(2) ? "cock-eyed" : "crooked");
*/
			    pline("%sがずれた．", what);
			} else {
			    struct obj *saved_ublindf = ublindf;
/*JP			    You("push your %s off.", what);*/
			    You("%sがずり落ちた．", what);
			    Blindf_off(ublindf);
			    dropx(saved_ublindf);
			}
		    }
		    return 1;
		case 0:
		    break;
		}
	}

	if (Glib) {
		Glib = 0;
/*JP		You("wipe off your %s.", makeplural(body_part(HAND)));*/
		You("%sを拭いた．", makeplural(body_part(HAND)));
		return 1;
	} else if(u.ucreamed) {
		Blinded -= u.ucreamed;
		u.ucreamed = 0;

		if (!Blinded) {
/*JP			pline("You've got the glop off.");*/
			You("さっぱりした．");
			Blinded = 1;
			make_blinded(0L,TRUE);
		} else {
			pline("%sの汚れを拭きとった．", body_part(FACE));
		}
		return 1;
	}

/*JP	Your("%s and %s are already clean.",*/
	Your("%sや%sは汚れていない．",
		body_part(FACE), makeplural(body_part(HAND)));

	return 0;
}

/* maybe give a stethoscope message based on floor objects */
STATIC_OVL boolean
its_dead(rx, ry, resp)
int rx, ry, *resp;
{
	struct obj *otmp;
	struct trap *ttmp;

	/* additional stethoscope messages from jyoung@apanix.apana.org.au */
	if (Hallucination && sobj_at(CORPSE, rx, ry)) {
	    /* (a corpse doesn't retain the monster's sex,
	       so we're forced to use generic pronoun here) */
/*JP	    You_hear("a voice say, \"It's dead, Jim.\"");*/
	    You("声が聞こえた「それは死んでるぜ，ジム」");
	    *resp = 1;
	    return TRUE;
	} else if (Role_if(PM_HEALER) && ((otmp = sobj_at(CORPSE, rx, ry)) != 0 ||
				    (otmp = sobj_at(STATUE, rx, ry)) != 0)) {
	    /* possibly should check uppermost {corpse,statue} in the pile
	       if both types are present, but it's not worth the effort */
	    if (vobj_at(rx, ry)->otyp == STATUE) otmp = vobj_at(rx, ry);
	    if (otmp->otyp == CORPSE) {
/*JP		You("determine that %s unfortunate being is dead.",
		    (rx == u.ux && ry == u.uy) ? "this" : "that");*/
		You("%s不幸な生き物は死んでいると結論した．",
		    (rx == u.ux && ry == u.uy) ? "この" : "その");
	    } else {
		ttmp = t_at(rx, ry);
/*JP		pline("%s appears to be in %s health for a statue.",
		      The(mons[otmp->corpsenm].mname),
		      (ttmp && ttmp->ttyp == STATUE_TRAP) ?
			"extraordinary" : "excellent");*/
		pline("彫像としての%sは%s作品だ．",
		      jtrns_mon(The(mons[otmp->corpsenm].mname), -1),
		      (ttmp && ttmp->ttyp == STATUE_TRAP) ?
			"健康的な" : "躍動的な");
	    }
	    return TRUE;
	}
	return FALSE;
}
/*JPstatic char hollow_str[] = "a hollow sound.  This must be a secret %s!";*/
static char hollow_str[] = "うつろな音を聞いた．秘密の%sに違いない！";

/* Strictly speaking it makes no sense for usage of a stethoscope to
   not take any time; however, unless it did, the stethoscope would be
   almost useless.  As a compromise, one use per turn is free, another
   uses up the turn; this makes curse status have a tangible effect. */
STATIC_OVL int
use_stethoscope(obj)
	register struct obj *obj;
{
	static long last_used = 0;
	struct monst *mtmp;
	struct rm *lev;
	int rx, ry, res;

	if (nohands(youmonst.data)) {	/* should also check for no ears and/or deaf */
/*JP		You("have no hands!");*//* not `body_part(HAND)' */
		pline("あなたには手がない！");
		return 0;
	} else if (!freehand()) {
/*JP		You("have no free %s.", body_part(HAND));*/
		You("%sの自由が効かない．", body_part(HAND));
		return 0;
	}
	if (!getdir((char *)0)) return 0;

	res = (moves + monstermoves == last_used);
	last_used = moves + monstermoves;

	if (u.uswallow && (u.dx || u.dy || u.dz)) {
		mstatusline(u.ustuck);
		return res;
#ifdef STEED
	} else if (u.usteed && u.dz > 0) {
		mstatusline(u.usteed);
		return res;
#endif
	} else if (u.dz) {
		if (Underwater)
/*JP		    You_hear("faint splashing.");*/
		    You("かすかにバシャバシャという音を聞いた．");
		else if (u.dz < 0 || !can_reach_floor())
/*JP		    You_cant("reach the %s.",
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));*/
		    You("%sに届かない．",
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
		else if (its_dead(u.ux, u.uy, &res))
		    ;	/* message already given */
		else if (Is_stronghold(&u.uz))
/*JP		    You_hear("the crackling of hellfire.");*/
		    You("地獄の炎がパチパチ燃えている音を聞いた．");
		else
/*JP		    pline_The("%s seems healthy enough.", surface(u.ux,u.uy));*/
		    pline("%sは充分健康のようだ．", surface(u.ux,u.uy));
		return res;
	} else if (obj->cursed && !rn2(2)) {
/*JP		You_hear("your heart beat.");*/
		You("自分の心臓の鼓動を聞いた．");
		return res;
	}
	if (Stunned || (Confusion && !rn2(5))) confdir();
	if (!u.dx && !u.dy) {
		ustatusline();
		return res;
	}
	rx = u.ux + u.dx; ry = u.uy + u.dy;
	if (!isok(rx,ry)) {
/*JP		You_hear("a faint typing noise.");*/
		You("かすかにだれかがタイピングしている音を聞いた．");
		return 0;
	}
	if ((mtmp = m_at(rx,ry)) != 0) {
		mstatusline(mtmp);
		if (mtmp->mundetected) {
			mtmp->mundetected = 0;
			if (cansee(rx,ry)) newsym(mtmp->my,mtmp->my);
		}
		if (!canspotmon(mtmp))
			map_invisible(rx,ry);
		return res;
	}
	if (glyph_is_invisible(levl[rx][ry].glyph)) {
		unmap_object(rx, ry);
		newsym(rx, ry);
/*JP		pline_The("invisible monster must have moved.");*/
		pline_The("見えない怪物は移動してしまった．");
	}

	lev = &levl[rx][ry];
	switch(lev->typ) {
	case SDOOR:
/*JP
		You_hear(hollow_str, "door");
*/
		You_hear(hollow_str, "扉");
		cvt_sdoor_to_door(lev);		/* ->typ = DOOR */
		if (Blind) feel_location(rx,ry);
		else newsym(rx,ry);
		return res;
	case SCORR:
/*JP
		You_hear(hollow_str, "passage");
*/
		You_hear(hollow_str, "通路");
		lev->typ = CORR;
		if (Blind) feel_location(rx,ry);
		else newsym(rx,ry);
		return res;
	}

	if (!its_dead(rx, ry, &res))
/*JP	    You("hear nothing special.");*/	/* not You_hear()  */
	    pline("別に何も聞こえない．");
	return res;
}
/*JP
static char whistle_str[] = "produce a %s whistling sound.";
*/
static char whistle_str[] = "笛を吹いて%s音をたてた．";

STATIC_OVL void
use_whistle(obj)
struct obj *obj;
{
/*JP	You(whistle_str, obj->cursed ? "shrill" : "high");
 */
	You(whistle_str, obj->cursed ? "不気味な" : "かん高い");
	wake_nearby();
}

STATIC_OVL void
use_magic_whistle(obj)
struct obj *obj;
{
	register struct monst *mtmp, *nextmon;

	if(obj->cursed && !rn2(2)) {
/*JP		You("produce a high-pitched humming noise.");*/
		You("調子の高いうなるような音をたてた．");
		wake_nearby();
	} else {
		int pet_cnt = 0;
/*JP		You(whistle_str, Hallucination ? "normal" : "strange");*/
		You(whistle_str, Hallucination ? "笛のような" : "奇妙な");
		for(mtmp = fmon; mtmp; mtmp = nextmon) {
		    nextmon = mtmp->nmon; /* trap might kill mon */
		    if (mtmp->mtame) {
			if (mtmp->mtrapped) {
			    /* no longer in previous trap (affects mintrap) */
			    mtmp->mtrapped = 0;
			    fill_pit(mtmp->mx, mtmp->my);
			}
			mnexto(mtmp);
			if (canspotmon(mtmp)) ++pet_cnt;
			if (mintrap(mtmp) == 2) change_luck(-1);
		    }
		}
		if (pet_cnt > 0) makeknown(MAGIC_WHISTLE);
	}
}

boolean
um_dist(x,y,n)
register xchar x, y, n;
{
	return((boolean)(abs(u.ux - x) > n  || abs(u.uy - y) > n));
}

int
number_leashed()
{
	register int i = 0;
	register struct obj *obj;

	for(obj = invent; obj; obj = obj->nobj)
		if(obj->otyp == LEASH && obj->leashmon != 0) i++;
	return(i);
}

void
o_unleash(otmp)		/* otmp is about to be destroyed or stolen */
register struct obj *otmp;
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(mtmp->m_id == (unsigned)otmp->leashmon)
			mtmp->mleashed = 0;
	otmp->leashmon = 0;
}

void
m_unleash(mtmp)		/* mtmp is about to die, or become untame */
register struct monst *mtmp;
{
	register struct obj *otmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == LEASH &&
				otmp->leashmon == (int)mtmp->m_id)
			otmp->leashmon = 0;
	mtmp->mleashed = 0;
}

void
unleash_all()		/* player is about to die (for bones) */
{
	register struct obj *otmp;
	register struct monst *mtmp;

	for(otmp = invent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == LEASH) otmp->leashmon = 0;
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(mtmp->mtame) mtmp->mleashed = 0;
}

#define MAXLEASHED	2

/* ARGSUSED */
STATIC_OVL void
use_leash(obj)
struct obj *obj;
{
	register int x, y;
	register struct monst *mtmp;
	int spotmon;

	if(!obj->leashmon && number_leashed() >= MAXLEASHED) {
/*JP		You("cannot leash any more pets.");*/
		You("これ以上ペットに紐をかけられない．");
		return;
	}

	if(!getdir((char *)0)) return;

	x = u.ux + u.dx;
	y = u.uy + u.dy;

	if((x == u.ux) && (y == u.uy)) {
/*JP		pline("Leash yourself?  Very funny...");*/
		pline("自分を縛る？変なの．．．");
		return;
	}

	if(!(mtmp = m_at(x, y))) {
/*JP		pline("There is no creature there.");*/
		pline("そこには生き物はいない．");
		return;
	}

	spotmon = canspotmon(mtmp);

	if(!mtmp->mtame) {
	    if(!spotmon)
/*JP		pline("There is no creature there.");*/
		pline("そこには生き物はいない．");
	    else
/*JP		pline("%s %s leashed!", Monnam(mtmp), (!obj->leashmon) ?
				"cannot be" : "is not");
*/
		pline("%sは紐で%s！", Monnam(mtmp), (!obj->leashmon) ?
				"結べない" : "結ばれていない");
	    return;
	}
	if(!obj->leashmon) {
		if(mtmp->mleashed) {
/*JP			pline("This %s is already leashed.",
			      spotmon ? l_monnam(mtmp) : "monster");*/
			pline("この%sはもう結んである．",
			      spotmon ? l_monnam(mtmp) : "怪物");
			return;
		}
/*JP		You("slip the leash around %s%s.",
		    spotmon ? "your " : "", l_monnam(mtmp));*/
		You("%sを紐で結びつけた．",
		    l_monnam(mtmp));
		mtmp->mleashed = 1;
		obj->leashmon = (int)mtmp->m_id;
		mtmp->msleeping = 0;
		return;
	}
	if(obj->leashmon != (int)mtmp->m_id) {
/*JP		pline("This leash is not attached to that creature.");*/
		pline("この紐はそれには結ばれていない．");
		return;
	} else {
		if(obj->cursed) {
/*JP			pline_The("leash would not come off!");*/
			pline("紐がはずれない！");
			obj->bknown = TRUE;
			return;
		}
		mtmp->mleashed = 0;
		obj->leashmon = 0;
/*JP		You("remove the leash from %s%s.",
		    spotmon ? "your " : "", l_monnam(mtmp));*/
		You("%sから紐をはずした．",
		    l_monnam(mtmp));
	}
	return;
}

struct obj *
get_mleash(mtmp)	/* assuming mtmp->mleashed has been checked */
register struct monst *mtmp;
{
	register struct obj *otmp;

	otmp = invent;
	while(otmp) {
		if(otmp->otyp == LEASH && otmp->leashmon == (int)mtmp->m_id)
			return(otmp);
		otmp = otmp->nobj;
	}
	return((struct obj *)0);
}

#endif /* OVLB */
#ifdef OVL1

boolean
next_to_u()
{
	register struct monst *mtmp;
	register struct obj *otmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(mtmp->mleashed) {
			if (distu(mtmp->mx,mtmp->my) > 2) mnexto(mtmp);
			if (distu(mtmp->mx,mtmp->my) > 2) {
			    for(otmp = invent; otmp; otmp = otmp->nobj)
				if(otmp->otyp == LEASH &&
					otmp->leashmon == (int)mtmp->m_id) {
				    if(otmp->cursed) return(FALSE);
/*JP				    You_feel("%s leash go slack.",
					(number_leashed() > 1) ? "a" : "the");*/
				    You("紐がたるんだような気がした．");
				    mtmp->mleashed = 0;
				    otmp->leashmon = 0;
				}
			}
		}
	return(TRUE);
}

#endif /* OVL1 */
#ifdef OVL0

void
check_leash(x, y)
register xchar x, y;
{
	register struct obj *otmp;
	register struct monst *mtmp = fmon;

	for(otmp = invent; otmp; otmp = otmp->nobj)
	    if(otmp->otyp == LEASH && otmp->leashmon != 0) {
		while(mtmp) {
		    if((int)mtmp->m_id == otmp->leashmon &&
			    (dist2(u.ux,u.uy,mtmp->mx,mtmp->my) >
				dist2(x,y,mtmp->mx,mtmp->my))
			) {
			if(otmp->cursed && !breathless(mtmp->data)) {
			    if(um_dist(mtmp->mx, mtmp->my, 5)) {
/*JP				pline("%s chokes to death!",Monnam(mtmp));*/
				pline("%sは絞め殺された！",Monnam(mtmp));
				mondied(mtmp);
			    } else
				if(um_dist(mtmp->mx, mtmp->my, 3))
/*JP					pline("%s chokes on the leash!",*/
					pline("%sは紐で首を絞められた！",
						Monnam(mtmp));
			} else {
			    if(um_dist(mtmp->mx, mtmp->my, 5)) {
/*JP				pline("%s leash snaps loose!",*/
				pline("%sの紐はパチンと外れた！",
					s_suffix(Monnam(mtmp)));
				m_unleash(mtmp);
			    } else {
				if(um_dist(mtmp->mx, mtmp->my, 3)) {
/*JP				    You("pull on the leash.");*/
				    You("紐を引っぱった．");
				    if (mtmp->data->msound != MS_SILENT)
					switch(rn2(3)) {
					    case 0:  growl(mtmp);	break;
					    case 1:  yelp(mtmp);	break;
					    default: whimper(mtmp); break;
					}
				}
			    }
			}
		    }
		    mtmp = mtmp->nmon;
		}
	    }
}

#endif /* OVL0 */
#ifdef OVLB

boolean
wield_tool(obj)
struct obj *obj;
{
	if(welded(uwep)) {
		/* Andreas Bormann - ihnp4!decvax!mcvax!unido!ab */
		if(flags.verbose) {
/*JP			pline("Since your weapon is welded to your %s,",
				bimanual(uwep) ?
				(const char *)makeplural(body_part(HAND))
				: body_part(HAND));
			pline("you cannot wield that %s.", xname(obj));
*/
			pline("武器を%sに装備しているので，",
				body_part(HAND));
			You("%sを装備できない．", xname(obj));
		}
		return(FALSE);
	}
	if (cantwield(youmonst.data)) {
/*JP		You_cant("hold it strongly enough.");*/
		You("それを持つほど力がない．");
		return(FALSE);
	}
	/* Check shield */
	if (uarms && bimanual(obj)) {
/*JP		You("cannot wield a two-handed tool while wearing a shield.");*/
		You("盾を装備しているときに両手持ちの道具を装備できない．");
		return(FALSE);
	}
	if(uquiver == obj) setuqwep((struct obj *)0);
	if(uswapwep == obj) {
	    (void) doswapweapon();
	    /* If doswapweapon failed... */
	    if(uswapwep == obj) return (FALSE);
	} else {
/*JP	    You("now wield %s.", doname(obj));*/
	    You("%sを装備した．", doname(obj));
	    setuwep(obj);
	}
	if (uwep != obj) return(FALSE); /* rewielded old object after dying */
	if (!can_twoweapon())
		untwoweapon();
	if (obj->oclass != WEAPON_CLASS)
		unweapon = TRUE;
	return(TRUE);
}

#define WEAK	3	/* from eat.c */

/*JPstatic char look_str[] = "look %s.";*/
static char look_str[] = "%s見える．";

STATIC_OVL int
use_mirror(obj)
struct obj *obj;
{
	register struct monst *mtmp;
	register char mlet;
	boolean vis;

	if(!getdir((char *)0)) return 0;
	if(obj->cursed && !rn2(2)) {
		if (!Blind)
/*JP			pline_The("mirror fogs up and doesn't reflect!");*/
			pline("鏡は曇り，映らなくなった！");
		return 1;
	}
	if(!u.dx && !u.dy && !u.dz) {
		if(!Blind && !Invisible) {
		    if (u.umonnum == PM_FLOATING_EYE) {
			if (!Free_action) {
			pline(Hallucination ?
/*JP			      "Yow!  The mirror stares back!" :
			      "Yikes!  You've frozen yourself!");
*/
			      "おゎ！鏡があなたをにらみ返した！" :
			      "おゎ！あなたは動けなくなった！");
			nomul(-rnd((MAXULEV+6) - u.ulevel));
/*JP			} else pline("You stiffen momentarily under your gaze.");*/
			} else pline("一瞬あなたのにらみで硬直した．");
		    } else if (youmonst.data->mlet == S_VAMPIRE)
/*JP			You("don't have a reflection.");*/
			You("鏡に映らなかった．");
		    else if (u.umonnum == PM_UMBER_HULK) {
/*JP			pline("Huh?  That doesn't look like you!");*/
			pline("ほえ？写ってるのはあなたじゃないみたいだ！");
			make_confused(HConfusion + d(3,4),FALSE);
		    } else if (Hallucination)
			You(look_str, hcolor((char *)0));
		    else if (Sick)
/*JP			You(look_str, "peaked");*/
			You(look_str, "顔色が悪く");
		    else if (u.uhs >= WEAK)
/*JP			You(look_str, "undernourished");*/
			You(look_str, "栄養失調のように");
/*JP		    else You("look as %s as ever.",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "beautiful" : "handsome") :
				"ugly");
*/
		    else You("あいかわらず%s見える．",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "美しく" : "りりしく") :
				"醜く");
		} else {
/*JP			You_cant("see your %s %s.",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "beautiful" : "handsome") :
				"ugly",
				body_part(FACE));
*/
			You("自分の%s%sを見ることができない．",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "美しい" : "りりしい") :
				"醜い",
				body_part(FACE));
		}
		return 1;
	}
	if(u.uswallow) {
/*JP		if (!Blind) You("reflect %s %s.", s_suffix(mon_nam(u.ustuck)),
		    is_animal(u.ustuck->data)? "stomach" : "interior");*/
		if (!Blind) You("%sの%sを映した．", mon_nam(u.ustuck),
		    is_animal(u.ustuck->data)? "胃" : "内部");
		return 1;
	}
	if(Underwater) {
		pline(Hallucination ?
/*JP		    "give the fish a chance to fix their makeup." :
		    "reflect the murky water.");*/
		    "魚がやってきて化粧直しをした．":
		    "あなたは陰気な水を映した．");
		return 1;
	}
	if(u.dz) {
		if (!Blind)
/*JP		    You("reflect the %s.",*/
		    You("%sを映した．",
			(u.dz > 0) ? surface(u.ux,u.uy) : ceiling(u.ux,u.uy));
		return 1;
	}
	mtmp = bhit(u.dx, u.dy, COLNO, INVIS_BEAM,
		    (int FDECL((*),(MONST_P,OBJ_P)))0,
		    (int FDECL((*),(OBJ_P,OBJ_P)))0,
		    obj);
	if (!mtmp || !haseyes(mtmp->data))
		return 1;

	vis = canseemon(mtmp);
	mlet = mtmp->data->mlet;
	if (mtmp->msleeping) {
		if (vis)
/*JP		    pline ("%s is too tired to look at your mirror.",*/
		    pline ("%sはとても疲れていて鏡を見ることができない．",
			    Monnam(mtmp));
	} else if (!mtmp->mcansee) {
	    if (vis)
/*JP		pline("%s can't see anything right now.", Monnam(mtmp));*/
		pline("%sは幻覚に襲われていて，ちゃんと見ることができない．", Monnam(mtmp));
	/* some monsters do special things */
	} else if (mlet == S_VAMPIRE || mlet == S_GHOST) {
	    if (vis)
/*JP		pline ("%s doesn't have a reflection.", Monnam(mtmp));*/
		pline ("%sは鏡に写らない．", Monnam(mtmp));
	} else if(!mtmp->mcan && mtmp->data == &mons[PM_MEDUSA]) {
/*JP		if (mon_reflects(mtmp, "The gaze is reflected away by %s %s!"))*/
		if (mon_reflects(mtmp, "にらみは%sの%sで反射した！"))
			return 1;
		if (vis)
/*JP			pline("%s is turned to stone!", Monnam(mtmp));*/
			pline("%sは石になった！", Monnam(mtmp));
		stoned = TRUE;
		killed(mtmp);
	} else if(!mtmp->mcan && !mtmp->minvis &&
					mtmp->data == &mons[PM_FLOATING_EYE]) {
		int tmp = d((int)mtmp->m_lev, (int)mtmp->data->mattk[0].damd);
		if (!rn2(4)) tmp = 120;
	/* Note: floating eyes cannot use their abilities while invisible,
	 * but Medusa and umber hulks can.
	 */
		if (vis)
/*JP			pline("%s is frozen by its reflection.", Monnam(mtmp));*/
			pline("%sは自分の姿を見て動けなくなった．", Monnam(mtmp));

/*JP		else You_hear("%s stop moving.",something);*/
		else You_hear("何かが動けなくなった音を聞いた．");
		mtmp->mcanmove = 0;
		if ( (int) mtmp->mfrozen + tmp > 127)
			mtmp->mfrozen = 127;
		else mtmp->mfrozen += tmp;
	} else if(!mtmp->mcan && mtmp->data == &mons[PM_UMBER_HULK]) {
		if (vis)
/*JP			pline ("%s confuses itself!", Monnam(mtmp));*/
			pline ("%sは混乱した！", Monnam(mtmp));
		mtmp->mconf = 1;
	} else if(!mtmp->mcan && !mtmp->minvis && (mlet == S_NYMPH
				     || mtmp->data==&mons[PM_SUCCUBUS])) {
		if (vis) {
/*JP		    pline ("%s admires herself in your mirror.", Monnam(mtmp));
		    pline ("She takes it!");*/
		    pline ("%sは自分の姿にうっとりした．", Monnam(mtmp));
		    pline ("彼女はそれを奪った！");
/*JP		} else pline ("It steals your mirror!");*/
		} else pline ("何者かがあなたの鏡を盗んだ！");
		setnotworn(obj); /* in case mirror was wielded */
		freeinv(obj);
		mpickobj(mtmp,obj);
		rloc(mtmp);
	} else if (!is_unicorn(mtmp->data) && !humanoid(mtmp->data) &&
			(!mtmp->minvis || perceives(mtmp->data)) && rn2(5)) {
		if (vis)
/*JP			pline ("%s is frightened by its reflection.",*/
			pline ("%sは自分の姿を見て怖がった．",
				Monnam(mtmp));
		mtmp->mflee = 1;
		mtmp->mfleetim += d(2,4);
	} else if (!Blind) {
		if (mtmp->minvis && !See_invisible)
		    ;
		else if ((mtmp->minvis && !perceives(mtmp->data))
			 || !haseyes(mtmp->data))
/*JP		    pline("%s doesn't seem to notice its reflection.",*/
		    pline("%sは自分の姿に気がついてないようだ．",
			Monnam(mtmp));
		else
/*JP		    pline("%s ignores %s reflection.",
			  Monnam(mtmp), his[pronoun_gender(mtmp)]);*/
		    pline("%sは自分の姿を無視した．",
			  Monnam(mtmp));
	}
	return 1;
}

STATIC_OVL void
use_bell(obj)
register struct obj *obj;
{
	struct monst *mtmp;
	boolean wakem = FALSE, learno = FALSE,
		ordinary = (obj->otyp != BELL_OF_OPENING || !obj->spe),
		invoking = (obj->otyp == BELL_OF_OPENING &&
			 invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy));

/*JP	You("ring %s.", the(xname(obj)));*/
	You("%sを鳴らした．", the(xname(obj)));

	if (Underwater || (u.uswallow && ordinary)) {
#ifdef	AMIGA
	    amii_speaker( obj, "AhDhGqEqDhEhAqDqFhGw", AMII_MUFFLED_VOLUME );
#endif
/*JP	    pline("But the sound is muffled.");*/
	    pline("しかし音はかき消された．");

	} else if (invoking && ordinary) {
	    /* needs to be recharged... */
/*JP	    pline("But it makes no sound.");(*/
	    pline("しかし，音は鳴らなかった．");
	    learno = TRUE;	/* help player figure out why */

	} else if (ordinary) {
#ifdef	AMIGA
	    amii_speaker( obj, "ahdhgqeqdhehaqdqfhgw", AMII_MUFFLED_VOLUME );
#endif
	    if (obj->cursed && !rn2(4) &&
		    /* note: once any of them are gone, we stop all of them */
		    !(mvitals[PM_WOOD_NYMPH].mvflags & G_GONE) &&
		    !(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) &&
		    !(mvitals[PM_MOUNTAIN_NYMPH].mvflags & G_GONE) &&
		    (mtmp = makemon(mkclass(S_NYMPH, 0),
					u.ux, u.uy, NO_MINVENT)) != 0) {
/*JP		You("summon %s!", a_monnam(mtmp));*/
		You("%sを召喚した！", a_monnam(mtmp));
		if (!obj_resists(obj, 93, 100)) {
/*JP		    pline("%s has shattered!", The(xname(obj)));*/
		    pline("%sは粉々になった！", The(xname(obj)));
		    useup(obj);
		} else switch (rn2(3)) {
			default:
				break;
			case 1: mon_adjust_speed(mtmp, 2);
				break;
			case 2: /* no explanation; it just happens... */
				nomovemsg = "";
				nomul(-rnd(2));
				break;
		}
	    }
	    wakem = TRUE;

	} else {
	    /* charged Bell of Opening */
	    check_unpaid(obj);
	    obj->spe--;

	    if (u.uswallow) {
		if (!obj->cursed)
		    (void) openit();
		else
		    pline(nothing_happens);

	    } else if (obj->cursed) {
		coord mm;

		mm.x = u.ux;
		mm.y = u.uy;
		mkundead(&mm, FALSE, NO_MINVENT);
		wakem = TRUE;

	    } else  if (invoking) {
/*JP		pline("%s issues an unsettling shrill sound...",*/
	        pline("%sは不気味な鋭い音を出した．．．",
		      The(xname(obj)));
#ifdef	AMIGA
		amii_speaker( obj, "aefeaefeaefeaefeaefe", AMII_LOUDER_VOLUME );
#endif
		obj->age = moves;
		learno = TRUE;
		wakem = TRUE;

	    } else if (obj->blessed) {
#ifdef	AMIGA
		amii_speaker( obj, "ahahahDhEhCw", AMII_SOFT_VOLUME );
#endif
		switch (openit()) {
		  case 0:  pline(nothing_happens); break;
/*JP		  case 1:  pline("%s opens...", Something);*/
		  case 1:  pline("何かが開いた");
			   learno = TRUE; break;
/*JP		  default: pline("Things open around you...");*/
		  default: pline("まわりの物が開いた．．．");
			   learno = TRUE; break;
		}

	    } else {  /* uncursed */
#ifdef	AMIGA
		amii_speaker( obj, "AeFeaeFeAefegw", AMII_OKAY_VOLUME );
#endif
		if (findit() != 0) learno = TRUE;
		else pline(nothing_happens);
	    }

	}	/* charged BofO */

	if (learno) {
	    makeknown(BELL_OF_OPENING);
	    obj->known = 1;
	}
	if (wakem) wake_nearby();
}

STATIC_OVL void
use_candelabrum(obj)
register struct obj *obj;
{
	if(Underwater) {
/*JP		You("cannot make fire under water.");*/
		You("水中で火はおこせない．");
		return;
	}
	if(obj->lamplit) {
/*JP		You("snuff the candle%s.", obj->spe > 1 ? "s" : "");*/
		You("ろうそくを吹き消した．");
		end_burn(obj, TRUE);
		return;
	}
	if(obj->spe <= 0) {
/*JP		pline("This %s has no candles.", xname(obj));*/
		pline("この%sにはろうそくがない．", xname(obj));
		return;
	}
	if(u.uswallow || obj->cursed) {
/*JP		pline_The("candle%s flicker%s for a moment, then die%s."
			obj->spe > 1 ? "s" : "",
			obj->spe > 1 ? "" : "s",
			obj->spe > 1 ? "" : "s");*/
		pline("ろうそくの炎はしばらく点滅し，消えた．");
		return;
	}
	if(obj->spe < 7) {
/*JP		pline("There %s only %d candle%s in %s.",
		       obj->spe == 1 ? "is" : "are",
		       obj->spe,
		       obj->spe > 1 ? "s" : "",
		       the(xname(obj)));*/
		pline("%sにはたった%d本のろうそくしかない．",
		       the(xname(obj)),
		       obj->spe);
		if (!Blind)
/*JP		    pline("%s lit.  %s shines dimly.",
		       obj->spe == 1 ? "It is" : "They are", The(xname(obj)));*/
		    pline("%sに火をつけた．%sはほのかに輝いた．",
		       The(xname(obj)),xname(obj));
	} else {
/*JP		pline("%s's candles burn%s", The(xname(obj)),
			(Blind ? "." : " brightly!"));*/
		pline("%sのろうそくは%s燃えあがった！", The(xname(obj)),
			(Blind ? "" : "明るく"));
	}
	if (!invocation_pos(u.ux, u.uy)) {
/*JP		pline_The("candle%s being rapidly consumed!",
			(obj->spe > 1 ? "s are" : " is"));*/
		pline("ろうそくはあっというまに燃えつきた！");
		obj->age /= 2;
	} else {
		if(obj->spe == 7) {
		    if (Blind)
/*JP		      pline("%s radiates a strange warmth!", The(xname(obj)));*/
		      pline("奇妙な暖かさを%sに感じた！", The(xname(obj)));
		    else
/*JP		      pline("%s glows with a strange light!", The(xname(obj)));*/
		      pline("%sは奇妙な光を発している！", The(xname(obj)));
		}
		obj->known = 1;
	}
	begin_burn(obj, FALSE);
}

STATIC_OVL void
use_candle(obj)
register struct obj *obj;
{
	register struct obj *otmp;
	char qbuf[QBUFSZ];

	if(u.uswallow) {
		You(no_elbow_room);
		return;
	}
	if(Underwater) {
/*JP		pline("Sorry, fire and water don't mix.");*/
	  	pline("残念ながら，火と水はまざらない．");
		return;
	}

	otmp = carrying(CANDELABRUM_OF_INVOCATION);
	if(!otmp || otmp->spe == 7) {
		use_lamp(obj);
		return;
	}

/*JP	Sprintf(qbuf, "Attach %s", the(xname(obj)));
	Sprintf(eos(qbuf), " to %s?", the(xname(otmp)));*/
	Sprintf(qbuf, "%sを", the(xname(obj)));
	Sprintf(eos(qbuf), "%sに取りつけますか？", the(xname(otmp)));
	if(yn(qbuf) == 'n') {
		if (!obj->lamplit)
/*JP		    You("try to light %s...", the(xname(obj)));*/
		    You("%sに火をつけようとした．．．", the(xname(obj)));
		use_lamp(obj);
		return;
	} else {
		if ((long)otmp->spe + obj->quan > 7L)
		    (void)splitobj(obj, 7L - (long)otmp->spe);
/*JP		You("attach %ld%s candle%s to %s.",
		    obj->quan, !otmp->spe ? "" : " more",
		    plur(obj->quan), the(xname(otmp)));
*/
		You("%ld本のろうそくを%s%sへ取りつけた．",
		    obj->quan, !otmp->spe ? "" : "さらに",
		    the(xname(otmp)));
		if (!otmp->spe || otmp->age > obj->age)
		    otmp->age = obj->age;
		otmp->spe += (int)obj->quan;
		if (otmp->lamplit && !obj->lamplit)
/*JP		    pline_The("new candle%s magically ignite%s!",
			      plur(obj->quan),
			      (obj->quan > 1L) ? "" : "s");
*/
		    pline("新しいろうそくは不思議な炎をあげた！");
		else if (!otmp->lamplit && obj->lamplit)
/*JP		    pline("%s out.", (obj->quan > 1L) ? "They go" : "It goes");*/
		    pline("炎は消えた");
		if (obj->unpaid)
/*JP		    verbalize("You %s %s, you bought %s!",
			      otmp->lamplit ? "burn" : "use",
			      (obj->quan > 1L) ? "them" : "it",
			      (obj->quan > 1L) ? "them" : "it");
*/
		    verbalize("火をつけたな，買ってもらおう！");
		if (obj->quan < 7L && otmp->spe == 7)
/*JP		    pline("%s now has seven%s candles attached.",
			  The(xname(otmp)), otmp->lamplit ? " lit" : "");
*/
		    pline("%sにはもう7本の%sろうそくが取りつけられている．",
		  The(xname(otmp)), otmp->lamplit ? "火のついた" : "");
		/* candelabrum's light range might increase */
		if (otmp->lamplit) obj_merge_light_sources(otmp, otmp);
		/* candles are no longer a separate light source */
		if (obj->lamplit) end_burn(obj, TRUE);
		/* candles are now gone */
		useupall(obj);
	}
}

boolean
snuff_candle(otmp)  /* call in drop, throw, and put in box, etc. */
register struct obj *otmp;
{
	register boolean candle = Is_candle(otmp);

	if ((candle || otmp->otyp == CANDELABRUM_OF_INVOCATION) &&
		otmp->lamplit) {
	    char buf[BUFSZ];
	    xchar x, y;
/*JP	    register boolean many = candle ? otmp->quan > 1L : otmp->spe > 1;*/

	    (void) get_obj_location(otmp, &x, &y, 0);
	    if (otmp->where == OBJ_MINVENT ? cansee(x,y) : !Blind)
/*JP		pline("%s %scandle%s flame%s extinguished.",
		      Shk_Your(buf, otmp),
		      (candle ? "" : "candelabrum's "),
		      (many ? "s'" : "'s"), (many ? "s are" : " is"));*/
		pline("%s%sろうそくの炎は消えた",
		      Shk_Your(buf, otmp),
		      candle ? "" : "燭台の");
	   end_burn(otmp, TRUE);
	   return(TRUE);
	}
	return(FALSE);
}

/* called when lit lamp is hit by water or put into a container or
   you've been swallowed by a monster; obj might be in transit while
   being thrown or dropped so don't assume that its location is valid */
boolean
snuff_lit(obj)
struct obj *obj;
{
	xchar x, y;

	if (obj->lamplit) {
	    if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
		    obj->otyp == BRASS_LANTERN || obj->otyp == POT_OIL) {
		(void) get_obj_location(obj, &x, &y, 0);
		if (obj->where == OBJ_MINVENT ? cansee(x,y) : !Blind)
/*JP
		    pline("%s goes out!", Yname2(obj));
*/
		    pline("%sは消えた！", Yname2(obj));
		end_burn(obj, TRUE);
		return TRUE;
	    }
	    if (snuff_candle(obj)) return TRUE;
	}
	return FALSE;
}

STATIC_OVL void
use_lamp(obj)
struct obj *obj;
{
	char buf[BUFSZ];

	if(Underwater) {
/*JP
		pline("This is not a diving lamp.");
*/
		pline("これは潜水用のランプじゃない．");
		return;
	}
	if(obj->lamplit) {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN)
/*JP
		    pline("%s lamp is now off.", Shk_Your(buf, obj));
*/
		    pline("%sランプの灯は消えた．", Shk_Your(buf, obj));
		else
/*JP
		    You("snuff out %s.", yname(obj));
*/
		    pline("%sを吹き消した．", yname(obj));
		end_burn(obj, TRUE);
		return;
	}
	/* magic lamps with an spe == 0 (wished for) cannot be lit */
	if ((!Is_candle(obj) && obj->age == 0)
			|| (obj->otyp == MAGIC_LAMP && obj->spe == 0)) {
		if (obj->otyp == BRASS_LANTERN)
/*JP  			Your("lamp has run out of power.");
*/
			Your("ランプを使い切ってしまった！");
/*JP  		else pline("This %s has no oil.", xname(obj));
*/
		else pline("この%sにはもうオイルがない．", xname(obj));
		return;
	}
	if (obj->cursed && !rn2(2)) {
/*JP		pline("%s flicker%s for a moment, then die%s.",
		       The(xname(obj)),
		       obj->quan > 1L ? "" : "s",
		       obj->quan > 1L ? "" : "s");
*/
		pline("%sはしばらくの間点滅し，消えた",
		       The(xname(obj)));
	} else {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN) {
		    check_unpaid(obj);
/*JP		    pline("%s lamp is now on.", Shk_Your(buf, obj));*/
		    pline("%sランプに灯が灯った．", Shk_Your(buf, obj));
		} else {	/* candle(s) */
/*JP		    pline("%s flame%s burn%s%s",
			s_suffix(Yname2(obj)),
			plur(obj->quan),
			obj->quan > 1L ? "" : "s",
			Blind ? "." : " brightly!");
*/
		    pline("%sは%s燃えあがった！",
			s_suffix(Yname2(obj)),
			Blind ? "" : "明るく");
		    if (obj->unpaid &&
			  obj->age == 20L * (long)objects[obj->otyp].oc_cost) {
/*JP			const char *ithem = obj->quan > 1L ? "them" : "it";
			verbalize("You burn %s, you bought %s!", ithem, ithem);*/
			verbalize("灯をつけたな，買ってもらおう！");
			bill_dummy_object(obj);
		    }
		}
		begin_burn(obj, FALSE);
	}
}

STATIC_OVL void
light_cocktail(obj)
	struct obj *obj;	/* obj is a potion of oil */
{
/*JP	char buf[BUFSZ];*/

	if (u.uswallow) {
	    You(no_elbow_room);
	    return;
	}

	if (obj->lamplit) {
/*JP	    You("snuff the lit potion.");*/
	    You("油瓶の火を吹き消した．");
	    end_burn(obj, TRUE);
	    /*
	     * Free & add to re-merge potion.  This will average the
	     * age of the potions.  Not exactly the best solution,
	     * but its easy.
	     */
	    freeinv(obj);
	    (void) addinv(obj);
	    return;
	} else if (Underwater) {
/*JP	    pline("There is not enough oxygen to sustain a fire.");*/
	    pline("火をつけるのに十分な酸素がない．");
	    return;
	}

/*JP	You("light %s potion.  It gives off a dim light.", shk_your(buf, obj));*/
	You("油瓶に火をつけた．油瓶は暗い光をはなった．");
	if (obj->unpaid) {
	    check_unpaid(obj);		/* surcharge for use of unpaid item */
	    bill_dummy_object(obj);	/* treat it as having been used up    */
	    obj->no_charge = 1;		/* you're now obligated to pay for it */
	    obj->unpaid = 0;
	}
	makeknown(obj->otyp);

	if (obj->quan > 1L) {
	    (void) splitobj(obj, 1L);
	    begin_burn(obj, FALSE);	/* burn before free to get position */
	    obj_extract_self(obj);	/* free from inv */

	    /* shouldn't merge */
/*JP	    obj = hold_another_object(obj, "You drop %s!",*/
	    obj = hold_another_object(obj, "あなたは%sを落した！",
				      doname(obj), (const char *)0);
	} else
	    begin_burn(obj, FALSE);
}

static NEARDATA const char cuddly[] = { TOOL_CLASS, 0 };

int
dorub()
{
/*JP	struct obj *obj = getobj(cuddly, "rub");*/
	struct obj *obj = getobj(cuddly, "こする");

	if(!obj || (obj != uwep && !wield_tool(obj))) return 0;

	/* now uwep is obj */
	if (uwep->otyp == MAGIC_LAMP) {
	    if (uwep->spe > 0 && !rn2(3)) {
		check_unpaid_usage(uwep, TRUE);		/* unusual item use */
		djinni_from_bottle(uwep);
		makeknown(MAGIC_LAMP);
		uwep->otyp = OIL_LAMP;
		uwep->spe = 0; /* for safety */
		uwep->age = rn1(500,1000);
		if (uwep->lamplit) begin_burn(uwep, TRUE);
	    } else if (rn2(2) && !Blind)
/*JP		You("see a puff of smoke.");*/
		pline("けむりが舞いあがった．");
	    else pline(nothing_happens);
	} else if (obj->otyp == BRASS_LANTERN) {
	    /* message from Adventure */
/*JP	    pline("Rubbing the electric lamp is not particularly rewarding.");*/
	    pline("電気ランプをこすっても意味はないと思うが．．．");
/*JP	    pline("Anyway, nothing exciting happens.");*/
	    pline("やっぱり，何も起きなかった．");
	} else pline(nothing_happens);
	return 1;
}

int
dojump()
{
	/* Physical jump */
	return jump(0);
}

int
jump(magic)
int magic; /* 0=Physical, otherwise skill level */
{
	coord cc;
	struct monst *mtmp;

	if (!magic && (nolimbs(youmonst.data) || slithy(youmonst.data))) {
		/* normally (nolimbs || slithy) implies !Jumping,
		   but that isn't necessarily the case for knights */
/*JP		You_cant("jump; you have no legs!");*/
		pline("足が無くては跳べない！");
		return 0;
	} else if (!magic && !Jumping) {
/*JP		You_cant("jump very far.");*/
		You_cant("そんな遠くまで跳べない．");
		return 0;
	} else if (u.uswallow) {
		if (magic) {
/*JP			You("bounce around a little.");*/
			pline("反動をつけた．");
			return 1;
		}
/*JP		pline("You've got to be kidding!");*/
		pline("冗談はよしこさん！");
		return 0;
	} else if (u.uinwater) {
		if (magic) {
/*JP			You("swish around a little.");*/
			pline("スイスイと泳いだ．");
			return 1;
		}
/*JP		pline("This calls for swimming, not jumping!");*/
		pline("それは『泳ぐ』であって，『跳ねる』じゃない！");
		return 0;
	} else if (u.ustuck) {
		if (magic) {
/*JP			You("writhe a little in the grasp of %s!", mon_nam(u.ustuck));*/
			You("%sから逃れようとジタバタした！", mon_nam(u.ustuck));
			return 1;
		}
/*JP		You("cannot escape from %s!", mon_nam(u.ustuck));*/
		You("%sから逃れられない！", mon_nam(u.ustuck));
		return 0;
	} else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
		if (magic) {
/*JP			You("flail around a little.");*/
			You("バタバタ跳んだ．");
			return 1;
		}
/*JP		You("don't have enough traction to jump.");*/
		You("跳ぶための反動がつけられない．");
		return 0;
	} else if (!magic && near_capacity() > UNENCUMBERED) {
/*JP		You("are carrying too much to jump!");*/
		You("たくさん物を持ちすぎて跳べない！");
		return 0;
	} else if (!magic && (u.uhunger <= 100 || ACURR(A_STR) < 6)) {
/*JP		You("lack the strength to jump!");*/
		You("跳べるだけの力がない！");
		return 0;
	} else if (Wounded_legs) {
		/* note: dojump() has similar code */
		long wl = (Wounded_legs & BOTH_SIDES);
		const char *bp = body_part(LEG);

		if (wl == BOTH_SIDES) bp = makeplural(bp);
#ifdef STEED
		if (u.usteed)
/*JP	 	    pline("%s is in no shape for jumping.", Monnam(u.usteed));*/
	 	    pline("%sは跳ぶような形をしていない．", Monnam(u.usteed));
		else
#endif
/*JP		Your("%s%s %s in no shape for jumping.",
		     (wl == LEFT_SIDE) ? "left " :
			(wl == RIGHT_SIDE) ? "right " : "",
		     bp, (wl == BOTH_SIDES) ? "are" : "is");
*/
		Your("%s%sを怪我をしており跳べない．",
		     (wl == LEFT_SIDE) ? "左" :
		     (wl == RIGHT_SIDE) ? "右" : "", bp);

		return 0;
	}
#ifdef STEED
	else if (u.usteed && u.utrap) {
/*JP		pline("%s is stuck in a trap.", Monnam(u.usteed));*/
		pline("%sは罠にひっかかっている．", Monnam(u.usteed));
		return (0);
	}
#endif

/*JP	pline("Where do you want to jump?");*/
	pline("どこに跳びますか？");
	cc.x = u.ux;
	cc.y = u.uy;
/*JP	if (getpos(&cc, TRUE, "the desired position") < 0)*/
	if (getpos(&cc, TRUE, "跳びたい場所") < 0)
		return 0;	/* user pressed ESC */
	if (!magic && !(HJumping & ~INTRINSIC) && !EJumping &&
			distu(cc.x, cc.y) != 5) {
/*JP		pline("Illegal move!");*/
		pline("その移動は桂馬飛びじゃない！");
		return 0;
	} else if (distu(cc.x, cc.y) > (magic ? 6+magic*3 : 9)) {
/*JP		pline("Too far!");*/
		pline("遠すぎる！");
		return 0;
	} else if (!cansee(cc.x, cc.y)) {
/*JP		You("cannot see where to land!");*/
		You("着地点が見えない！");
		return 0;
	} else if ((mtmp = m_at(cc.x, cc.y)) != 0) {
/*JP		You("cannot trample %s!", mon_nam(mtmp));*/
		You("%sには乗っかれない！", mon_nam(mtmp));
		return 0;
	} else if (!isok(cc.x, cc.y) ||
		   ((IS_ROCK(levl[cc.x][cc.y].typ) ||
		     sobj_at(BOULDER, cc.x, cc.y) || closed_door(cc.x, cc.y))
		    && !(Passes_walls && may_passwall(cc.x, cc.y)))) {
/*JP  			You("cannot jump there!");
*/
			You("そこには飛べない！");
			return 0;
	} else {
	    if(u.utrap)
		switch(u.utraptype) {
		case TT_BEARTRAP: {
		    register long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;
/*JP		    You("rip yourself free of the bear trap!  Ouch!");
		    losehp(rnd(10), "jumping out of a bear trap", KILLED_BY);
*/
		    You("自分を熊の罠からひきはがした，いてっ！");
		    losehp(rnd(10), "熊の罠から飛び出ようとして", KILLED_BY);
		    set_wounded_legs(side, rn1(1000,500));
		    break;
		  }
		case TT_PIT:
/*JP		    You("leap from the pit!");*/
		    You("落し穴から飛び出た！");
		    break;
		case TT_WEB:
/*JP		    You("tear the web apart as you pull yourself free!");*/
		    You("蜘蛛の巣をひきさき，自由になった！");
		    deltrap(t_at(u.ux,u.uy));
		    break;
		case TT_LAVA:
/*JP		    You("pull yourself above the lava!");*/
		    You("溶岩から飛び出た！");
		    u.utrap = 0;
		    return 1;
		case TT_INFLOOR:
/*JP		    You("strain your %s, but you're still stuck in the floor.",*/
		    You("%sを引っぱった，しかし，あなたはまだ床にうまっている．",
			makeplural(body_part(LEG)));
		    set_wounded_legs(LEFT_SIDE, rn1(10, 11));
		    set_wounded_legs(RIGHT_SIDE, rn1(10, 11));
		    return 1;
		}

		/* A little Sokoban guilt... */
		if (In_sokoban(&u.uz))
		    change_luck(-1);

	    teleds(cc.x, cc.y);
	    nomul(-1);
	    nomovemsg = "";
	    morehungry(rnd(25));
	    return 1;
	}
}

boolean
tinnable(corpse)
struct obj *corpse;
{
	if (corpse->oeaten) return 0;
	if (!mons[corpse->corpsenm].cnutrit) return 0;
	return 1;
}

STATIC_OVL void
use_tinning_kit(obj)
register struct obj *obj;
{
	register struct obj *corpse, *can;

	/* This takes only 1 move.  If this is to be changed to take many
	 * moves, we've got to deal with decaying corpses...
	 */
	if (obj->spe <= 0) {
/*JP		You("seem to be out of tins.");*/
		You("缶詰を作るための缶が切れたようだ．");
		return;
	}
/*JP	if (!(corpse = floorfood("tin", 2))) return;*/
	if (!(corpse = floorfood("缶詰めにする", 2))) return;
	if (corpse->oeaten) {
/*JP		You("cannot tin %s which is partly eaten.",something);*/
		You("食べかけのものを缶詰にすることはできない．");
		return;
	}
	if (touch_petrifies(&mons[corpse->corpsenm])
		&& !Stone_resistance && !uarmg) {
	    char kbuf[BUFSZ];

	    if (poly_when_stoned(youmonst.data))
/*JP		You("tin %s without wearing gloves.",
			an(mons[corpse->corpsenm].mname));
*/
		You("小手なしで%sを缶詰にしようとした．",
			jtrns_mon(mons[corpse->corpsenm].mname,-1));
	    else {
/*JP		pline("Tinning %s without wearing gloves is a fatal mistake...",
			an(mons[corpse->corpsenm].mname));
*/
		pline("%sを小手なしで缶詰にするのは致命的な間違いだ．．．",
			jtrns_mon(mons[corpse->corpsenm].mname,-1));
/*JP		Sprintf(kbuf, "trying to tin %s without gloves",
			an(mons[corpse->corpsenm].mname));
*/
		Sprintf(kbuf, "小手をつけずに%sを缶詰にしようとして",
			jtrns_mon(mons[corpse->corpsenm].mname,-1));
	    }
	    instapetrify(kbuf);
	}
	if (is_rider(&mons[corpse->corpsenm])) {
		(void) revive_corpse(corpse);
/*JP		verbalize("Yes...  But War does not preserve its enemies...");*/
		verbalize("そうだ．．．「戦争」は敵に安らぎを与えぬ．．．");
		return;
	}
	if (mons[corpse->corpsenm].cnutrit == 0) {
/*JP		pline("That's too insubstantial to tin.");*/
		pline("実体がないので缶詰にできない．");
		return;
	}
	obj->spe--;
	if ((can = mksobj(TIN, FALSE, FALSE)) != 0) {
/*JP	    static const char you_buy_it[] = "You tin it, you bought it!";*/
	    static const char you_buy_it[] = "缶詰にしたな！買ってもらおう！";

	    can->corpsenm = corpse->corpsenm;
	    can->cursed = obj->cursed;
	    can->blessed = obj->blessed;
	    can->owt = weight(can);
	    can->known = 1;
	    can->spe = -1;  /* Mark tinned tins. No spinach allowed... */
	    if (carried(corpse)) {
		if (corpse->unpaid)
		    verbalize(you_buy_it);
		useup(corpse);
	    } else {
		if (costly_spot(corpse->ox, corpse->oy) && !corpse->no_charge)
		    verbalize(you_buy_it);
		useupf(corpse, 1L);
	    }
/*JP	    can = hold_another_object(can, "You make, but cannot pick up, %s.",*/
	    can = hold_another_object(can, "缶詰にできたが，%sを持つことができない．",
				      doname(can), (const char *)0);
	} else impossible("Tinning failed.");
}

void
use_unicorn_horn(obj)
struct obj *obj;
{
#define PROP_COUNT 6		/* number of properties we're dealing with */
#define ATTR_COUNT (A_MAX*3)	/* number of attribute points we might fix */
	int idx, val, val_limit,
	    trouble_count, unfixable_trbl, did_prop, did_attr;
	int trouble_list[PROP_COUNT + ATTR_COUNT];

	if (obj && obj->cursed) {
	    long lcount = (long) rnd(100);

	    switch (rn2(6)) {
	    case 0: make_sick(Sick ? Sick/3L + 1L : (long)rn1(ACURR(A_CON),20),
			xname(obj), TRUE, SICK_NONVOMITABLE);
		    break;
	    case 1: make_blinded(Blinded + lcount, TRUE);
		    break;
	    case 2: if (!Confusion)
/*JP			You("suddenly feel %s.",
			    Hallucination ? "trippy" : "confused");*/
			You("突然%s．",
			    Hallucination ? "へろへろになった" : "混乱した");
		    make_confused(HConfusion + lcount, TRUE);
		    break;
	    case 3: make_stunned(HStun + lcount, TRUE);
		    break;
	    case 4: (void) adjattrib(rn2(A_MAX), -1, FALSE);
		    break;
	    case 5: make_hallucinated(HHallucination + lcount, TRUE, 0L);
		    break;
	    }
	    return;
	}

/*
 * Entries in the trouble list use a very simple encoding scheme.
 */
#define prop2trbl(X)	((X) + A_MAX)
#define attr2trbl(Y)	(Y)
#define prop_trouble(X) trouble_list[trouble_count++] = prop2trbl(X)
#define attr_trouble(Y) trouble_list[trouble_count++] = attr2trbl(Y)

	trouble_count = unfixable_trbl = did_prop = did_attr = 0;

	/* collect property troubles */
	if (Sick) prop_trouble(SICK);
	if (Blinded > (long)(u.ucreamed + 1)) prop_trouble(BLINDED);
	if (HHallucination) prop_trouble(HALLUC);
	if (Vomiting) prop_trouble(VOMITING);
	if (HConfusion) prop_trouble(CONFUSION);
	if (HStun) prop_trouble(STUNNED);
	/* keep track of unfixed trouble, for message adjustment below
	   (can't "feel great" with these problems present) */
	if (Stoned) unfixable_trbl++;
	if (Strangled) unfixable_trbl++;
	if (Wounded_legs) unfixable_trbl++;

	/* collect attribute troubles */
	for (idx = 0; idx < A_MAX; idx++) {
	    val_limit = AMAX(idx);
	    /* don't recover strength lost from hunger */
	    if (idx == A_STR && u.uhs >= WEAK) val_limit--;
	    /* don't recover more than 3 points worth of any attribute */
	    if (val_limit > ABASE(idx) + 3) val_limit = ABASE(idx) + 3;

	    for (val = ABASE(idx); val < val_limit; val++)
		attr_trouble(idx);
	    /* keep track of unfixed trouble, for message adjustment below */
	    unfixable_trbl += (AMAX(idx) - val_limit);
	}

	if (trouble_count == 0) {
	    pline(nothing_happens);
	    return;
	} else if (trouble_count > 1) {		/* shuffle */
	    int i, j, k;

	    for (i = trouble_count - 1; i > 0; i--)
		if ((j = rn2(i + 1)) != i) {
		    k = trouble_list[j];
		    trouble_list[j] = trouble_list[i];
		    trouble_list[i] = k;
		}
	}

	/*
	 *		Chances for number of troubles to be fixed
	 *		 0	1      2      3      4	    5	   6	  7
	 *   blessed:  22.7%  22.7%  19.5%  15.4%  10.7%   5.7%   2.6%	 0.8%
	 *  uncursed:  35.4%  35.4%  22.9%   6.3%    0	    0	   0	  0
	 */
	val_limit = rn2( d(2, (obj && obj->blessed) ? 4 : 2) );
	if (val_limit > trouble_count) val_limit = trouble_count;

	/* fix [some of] the troubles */
	for (val = 0; val < val_limit; val++) {
	    idx = trouble_list[val];

	    switch (idx) {
	    case prop2trbl(SICK):
		make_sick(0L, (char *) 0, TRUE, SICK_ALL);
		did_prop++;
		break;
	    case prop2trbl(BLINDED):
		make_blinded(u.ucreamed ? (long)(u.ucreamed+1) : 0L, TRUE);
		did_prop++;
		break;
	    case prop2trbl(HALLUC):
		make_hallucinated(0L, TRUE, 0L);
		did_prop++;
		break;
	    case prop2trbl(VOMITING):
		make_vomiting(0L, TRUE);
		did_prop++;
		break;
	    case prop2trbl(CONFUSION):
		make_confused(0L, TRUE);
		did_prop++;
		break;
	    case prop2trbl(STUNNED):
		make_stunned(0L, TRUE);
		did_prop++;
		break;
	    default:
		if (idx >= 0 && idx < A_MAX) {
		    ABASE(idx) += 1;
		    did_attr++;
		} else
		    panic("use_unicorn_horn: bad trouble? (%d)", idx);
		break;
	    }
	}

	if (did_attr)
/*JP	    pline("This makes you feel %s!",
		  (did_prop + did_attr) == (trouble_count + unfixable_trbl) ?
		  "great" : "better");*/
	    pline("気分が%sよくなった！",
		  (did_prop + did_attr) == (trouble_count + unfixable_trbl) ?
		  "とても" : "より");
	else if (!did_prop)
/*JP	    pline("Nothing seems to happen.");*/
	    pline("何も起きたような気がしない．");

	flags.botl = (did_attr || did_prop);
#undef PROP_COUNT
#undef ATTR_COUNT
#undef prop2trbl
#undef attr2trbl
#undef prop_trouble
#undef attr_trouble
}

/*
 * Timer callback routine: turn figurine into monster
 */
void
fig_transform(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *figurine = (struct obj *)arg;
	struct monst *mtmp;
	coord cc;
	boolean cansee_spot, silent, okay_spot;
	boolean redraw = FALSE;
	char monnambuf[BUFSZ], carriedby[BUFSZ];

	if (!figurine) {
#ifdef DEBUG
	    pline("null figurine in fig_transform()");
#endif
	    return;
	}
	silent = (timeout != monstermoves); /* happened while away */
	okay_spot = get_obj_location(figurine, &cc.x, &cc.y, 0);
	if (figurine->where == OBJ_INVENT ||
	    figurine->where == OBJ_MINVENT) 
	        okay_spot = enexto(&cc, cc.x, cc.y,
				   &mons[figurine->corpsenm]);
	if (!okay_spot ||
	    !figurine_location_checks(figurine,&cc, TRUE)) {
	    	/* reset the timer to try again later */
		(void) start_timer((long)rnd(5000), TIMER_OBJECT,
				FIG_TRANSFORM, (genericptr_t)figurine);
		return;
	}
	
	cansee_spot = cansee(cc.x, cc.y);
	mtmp = make_familiar(figurine, cc.x, cc.y, TRUE);
	if (mtmp) {
	    Sprintf(monnambuf, "%s",a_monnam(mtmp));
	    switch (figurine->where) {
		case OBJ_INVENT:
		    if (Blind)
/*JP			You_feel("%s %s from your pack!", something,
			    locomotion(mtmp->data,"drop"));
*/
			You_feel("%sがあなたの鞄から%s！", something,
				 jconj(locomotion(mtmp->data,"落ちる"), "た"));
		    else
/*JP			You("see %s %s out of your pack!",
			    monnambuf,
			    locomotion(mtmp->data,"drop"));
*/
			You("%sがあなたの鞄から%s！",
			    monnambuf,
			    jconj(locomotion(mtmp->data,"落ちる"), "た"));
		    break;

		case OBJ_FLOOR:
		    if (cansee_spot && !silent) {
/*JP			You("suddenly see a figurine transform into %s!",
				monnambuf);
*/
			You("突然人形は%sになった！",
				monnambuf);
			redraw = TRUE;	/* update figurine's map location */
		    }
		    break;

		case OBJ_MINVENT:
		    if (cansee_spot && !silent) {
		    	struct monst *mon;
		    	mon = figurine->ocarry;
			/* figurine carring monster might be invisible */
			if (canseemon(figurine->ocarry)) {
/*JP			    Sprintf(carriedby, "%s pack",
				     s_suffix(a_monnam(mon)));
*/
			    Sprintf(carriedby, "%sの鞄",
				     s_suffix(a_monnam(mon)));
			}
			else if (is_pool(mon->mx, mon->my))
/*JP			    Strcpy(carriedby, "empty water");*/
			    Strcpy(carriedby, "何もない水中");
			else
/*JP			    Strcpy(carriedby, "thin air");*/
			    Strcpy(carriedby, "何もない空中");
/*JP			You("see %s %s out of %s!", monnambuf,
			    locomotion(mtmp->data, "drop"), carriedby);
*/
			You("%sが%sからの%sのを見た！", monnambuf,
			    carriedby, locomotion(mtmp->data, "落ちる"));
		    }
		    break;
#if 0
		case OBJ_MIGRATING:
		    break;
#endif

		default:
		    impossible("figurine came to life where? (%d)",
				(int)figurine->where);
		break;
	    }
	}
	/* free figurine now */
	obj_extract_self(figurine);
	obfree(figurine, (struct obj *)0);
	if (redraw) newsym(cc.x, cc.y);
}

STATIC_OVL boolean
figurine_location_checks(obj, cc, quietly)
struct obj *obj;
coord *cc;
boolean quietly;
{
	xchar x,y;
	
	x = cc->x; y = cc->y;
	if (!isok(x,y)) {
		if (!quietly)
/*JP			You("cannot put the figurine there.");*/
		    You("ここに人形を置けない．");
		return FALSE;
	}
	if (IS_ROCK(levl[x][y].typ) &&
	    !(passes_walls(&mons[obj->corpsenm]) && may_passwall(x,y))) {
		if (!quietly)
/*JP			You("cannot place a figurine in solid rock!");*/
		    You("固い石の中には人形を置けない！");
		return FALSE;
	}
	if (sobj_at(BOULDER,x,y) && !passes_walls(&mons[obj->corpsenm])
			&& !throws_rocks(&mons[obj->corpsenm])) {
		if (!quietly)
/*JP			You("cannot fit the figurine on the boulder.");*/
		    You("岩の上に人形を置けない．");
		return FALSE;
	}
	return TRUE;
}

STATIC_OVL void
use_figurine(obj)
register struct obj *obj;
{
	xchar x, y;
	coord cc;
	
	if(!getdir((char *)0)) {
		flags.move = multi = 0;
		return;
	}
	x = u.ux + u.dx; y = u.uy + u.dy;
	cc.x = x; cc.y = y;
	/* Passing FALSE arg here will result in messages displayed */
	if (!figurine_location_checks(obj, &cc, FALSE)) return;
/*JP	You("%s and it transforms.",
	    (u.dx||u.dy) ? "set the figurine beside you" :
	    (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) ?
		"release the figurine" :
	    (u.dz < 0 ?
		"toss the figurine into the air" :
		"set the figurine on the ground"));
*/
	You("%s．するとそれは変形した．",
	    (u.dx||u.dy) ? "そばに人形を置いた" :
	    (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) ?
		"人形を放った" :
	    (u.dz < 0 ?
		"人形を空中に投げた" :
		"人形を地面に置いた"));
	(void) make_familiar(obj, u.ux+u.dx, u.uy+u.dy, FALSE);
	(void) stop_timer(FIG_TRANSFORM, (genericptr_t)obj);
	useup(obj);
}

static NEARDATA const char lubricables[] = { ALL_CLASSES, ALLOW_NONE, 0 };
static NEARDATA const char need_to_remove_outer_armor[] =
/*JP			"need to remove your %s to grease your %s.";*/
			"%sに油を塗るには%sをはずす必要がある．";

STATIC_OVL void
use_grease(obj)
struct obj *obj;
{
	struct obj *otmp;
	char buf[BUFSZ];

	if (Glib) {
	    dropx(obj);
/*JP	    pline("%s slips from your %s.", The(xname(obj)),*/
	    pline("%sはあなたの%sから滑り落ちた．", The(xname(obj)),
		  makeplural(body_part(FINGER)));
	    return;
	}

	if (obj->spe > 0) {
		if ((obj->cursed || Fumbling) && !rn2(2)) {
			check_unpaid(obj);
			obj->spe--;
			dropx(obj);
/*JP			pline("%s slips from your %s.", The(xname(obj)),*/
			pline("%sはあなたの%sから滑り落ちた．", The(xname(obj)),
			      makeplural(body_part(FINGER)));
			return;
		}
/*P		otmp = getobj(lubricables, "grease");*/
		otmp = getobj(lubricables, "に塗る");
		if (!otmp) return;
		if ((otmp->owornmask & WORN_ARMOR) && uarmc) {
			Strcpy(buf, xname(uarmc));
/*JP			You(need_to_remove_outer_armor, buf, xname(otmp));*/
			You(need_to_remove_outer_armor, xname(otmp), buf);
			return;
		}
#ifdef TOURIST
/*JP		if ((otmp->owornmask & WORN_SHIRT) && (uarmc || uarm)) {
			Strcpy(buf, uarmc ? xname(uarmc) : "");
			if (uarmc && uarm) Strcat(buf, " and ");
			Strcat(buf, uarm ? xname(uarm) : "");
			You(need_to_remove_outer_armor, buf, xname(otmp));*/
		if ((otmp->owornmask & WORN_SHIRT) && (uarmc || uarm)) {
			Strcpy(buf, uarmc ? xname(uarmc) : "");
			if (uarmc && uarm) Strcat(buf, "と");
			Strcat(buf, uarm ? xname(uarm) : "");
			You(need_to_remove_outer_armor, xname(otmp), buf);
			return;
		}
#endif
		check_unpaid(obj);
		obj->spe--;
		if (otmp != &zeroobj) {
/*JP			You("cover %s with a thick layer of grease.",*/
			You("%sに脂を丹念に塗った．",
			    yname(otmp));
			otmp->greased = 1;
			if (obj->cursed && !nohands(youmonst.data)) {
			    incr_itimeout(&Glib, rnd(15));
/*JP			    pline("Some of the grease gets all over your %s.",*/
			    pline("あぶらが少し%sについた．",
				makeplural(body_part(HAND)));
			}
		} else {
			Glib += rnd(15);
/*JP			You("coat your %s with grease.",*/
			You("%sに脂を塗った．",
			    makeplural(body_part(FINGER)));
		}
	} else {
/*JP		pline("%s %s empty.", The(xname(obj)),
			obj->known ? "is" : "seems to be");
*/
		pline("%sは空っぽ%s．", The(xname(obj)),
		      obj->known ? "だ" : "に見える");

	}
}

static struct trapinfo {
	struct obj *tobj;
	xchar tx, ty;
	int time_needed;
} trapinfo;

void
reset_trapset()
{
	trapinfo.tobj = 0;
}

/* Place a landmine/bear trap.  Helge Hafting */
STATIC_OVL void
use_trap(otmp)
struct obj *otmp;
{
	int ttyp, tmp;
	const char *what = (char *)0;
/*JP	char buf[BUFSZ];*/
/*JP	const char *occutext = "setting the trap";*/
	const char *occutext = "罠を仕掛けている";

	if (nohands(youmonst.data))
/*JP	    what = "without hands";*/
	    what = "手がないので";
	else if (Stunned)
/*JP	    what = "while stunned";*/
	    what = "くらくらしているので";
	else if (u.uswallow)
/*JP	    what = is_animal(u.ustuck->data) ? "while swallowed" :
			"while engulfed";
*/
	    what = is_animal(u.ustuck->data) ? "飲み込まれている間は" :
			"巻き込まれている間は";
	else if (Underwater)
/*JP	    what = "underwater";*/
	    what = "水面下では";
	else if (Levitation)
/*JP	    what = "while levitating";*/
	    what = "浮いている間は";
	else if (is_pool(u.ux, u.uy))
/*JP	    what = "in water";*/
	    what = "水中では";
	else if (is_lava(u.ux, u.uy))
/*JP	    what = "in lava";*/
	    what = "溶岩の中では";
	else if (On_stairs(u.ux, u.uy))
	    what = (u.ux == xdnladder || u.ux == xupladder) ?
/*JP			"on the ladder" : "on the stairs";*/
			"はしごの上では" : "階段の上では";
	else if (IS_FURNITURE(levl[u.ux][u.uy].typ) ||
		IS_ROCK(levl[u.ux][u.uy].typ) ||
		closed_door(u.ux, u.uy) || t_at(u.ux, u.uy))
/*JP	    what = "here";*/
	    what = "ここでは";
	if (what) {
/*JP	    You_cant("set a trap %s!",what);*/
	    pline("%s罠を仕掛けられない！",what);
	    reset_trapset();
	    return;
	}
	ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
	if (otmp == trapinfo.tobj &&
		u.ux == trapinfo.tx && u.uy == trapinfo.ty) {
/*JP	    You("resume setting %s %s.",
		shk_your(buf, otmp),
		defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
*/
	    You("%sを仕掛けるのを再開した．",
		jtrns_obj('^', defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
	    set_occupation(set_trap, occutext, 0);
	    return;
	}
	trapinfo.tobj = otmp;
	trapinfo.tx = u.ux,  trapinfo.ty = u.uy;
	tmp = ACURR(A_DEX);
	trapinfo.time_needed = (tmp > 17) ? 2 : (tmp > 12) ? 3 :
				(tmp > 7) ? 4 : 5;
	if (Blind) trapinfo.time_needed *= 2;
	tmp = ACURR(A_STR);
	if (ttyp == BEAR_TRAP && tmp < 18)
	    trapinfo.time_needed += (tmp > 12) ? 1 : (tmp > 7) ? 2 : 4;
	/*[fumbling and/or confusion and/or cursed object check(s)
	   should be incorporated here instead of in set_trap]*/

/*JP	You("begin setting %s %s.",
	    shk_your(buf, otmp),
	    defsyms[trap_to_defsym(what_trap(ttyp))].explanation);
*/
	You("%sを仕掛けはじめた．",
	    jtrns_obj('^', defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
	set_occupation(set_trap, occutext, 0);
	return;
}

STATIC_PTR
int
set_trap()
{
	struct obj *otmp = trapinfo.tobj;
	struct trap *ttmp;
	int ttyp;

	if (!otmp || otmp->where != OBJ_INVENT ||
		u.ux != trapinfo.tx || u.uy != trapinfo.ty) {
	    /* ?? */
	    reset_trapset();
	    return 0;
	}

	if (--trapinfo.time_needed > 0) return 1;	/* still busy */

	ttyp = (otmp->otyp == LAND_MINE) ? LANDMINE : BEAR_TRAP;
	ttmp = maketrap(u.ux, u.uy, ttyp);
	if (ttmp) {
	    ttmp->tseen = 1;
	    ttmp->madeby_u = 1;
	    newsym(u.ux, u.uy); /* if our hero happens to be invisible */
	    if (*in_rooms(u.ux,u.uy,SHOPBASE)) {
		add_damage(u.ux, u.uy, 0L);		/* schedule removal */
	    }
/*JP	    You("finish arming %s.",
		the(defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
*/
	    You("%sを仕掛け終えた．",
		jtrns_obj('^', defsyms[trap_to_defsym(what_trap(ttyp))].explanation));
	    if ((otmp->cursed || Fumbling) && (rnl(10) > 5)) dotrap(ttmp);
	} else {
	    /* this shouldn't happen */
/*JP	    Your("trap setting attempt fails.");*/
	    You("罠を仕掛けるのに失敗した．");
	}
	useup(otmp);
	reset_trapset();
	return 0;
}

STATIC_OVL int
use_whip(obj)
struct obj *obj;
{
	char buf[BUFSZ];
	struct monst *mtmp;
	register int rx, ry;
	int res = 0;
	int proficient = 0;
/*JP	const char *msg_slipsfree = "The bullwhip slips free.";
	const char *msg_snap = "Snap!";*/
	const char *msg_slipsfree = "鞭はほどけた．";
	const char *msg_snap = "ピシッ！";
	struct obj *otmp;

	if (obj != uwep) {
	    if (!wield_tool(obj)) return(0);
	    else res = 1;
	    /* prevent bashing msg */
	    unweapon = FALSE;
	}
	if(!getdir((char *)0)) return(res);
	if (Stunned || (Confusion && !rn2(5))) confdir();
	rx = u.ux + u.dx;
	ry = u.uy + u.dy;
	mtmp = m_at(rx, ry);

	/* fake some proficiency checks */
	proficient = 0;
	if (Role_if(PM_ARCHEOLOGIST)) ++proficient;
	if (ACURR(A_DEX) < 6) proficient--;
	else if (ACURR(A_DEX) >= 14) proficient += (ACURR(A_DEX) - 14);
	if (Fumbling) --proficient;
	if (proficient > 3) proficient = 3;
	if (proficient < 0) proficient = 0;

	if (u.uswallow && attack(u.ustuck))
/*JP		pline("There is not enough room to flick your bullwhip.");*/
		pline("鞭を打つほど広くない．");
	else if (Underwater)
/*JP		pline("There is too much resistance to flick your bullwhip.");*/
		pline("水の抵抗がありすぎて鞭を打つことができない．");
	else if (u.dz < 0)
/*JP		You("flick a bug off of the %s.",ceiling(u.ux,u.uy));*/
		You("%sの虫を打ち落した．",ceiling(u.ux,u.uy));
	else if((!u.dx && !u.dy) || (u.dz > 0)) {
		int dam;

#ifdef STEED
		/* Sometimes you hit your steed by mistake */
		if (u.usteed && !rn2(3)) {
/*JP			You("whip %s!", mon_nam(u.usteed));*/
			You("%sを鞭打った！", mon_nam(u.usteed));
			kick_steed();
			return (1);
		}
#endif
		if (Levitation
#ifdef STEED
			|| u.usteed
#endif
		   ) {
			/* Have a shot at snaring something on the floor */
			otmp = level.objects[u.ux][u.uy];
			if (otmp && proficient) {
/*JP				You("wrap your bullwhip around %s on the %s.",
					an(singular(otmp,xname)),
					surface(u.ux, u.uy));
*/
				You("鞭を%sの上の%sにからませた．",
					surface(u.ux, u.uy),
					an(singular(otmp,xname)));
				if (!rnl(6))
					if (pickup_object(otmp, 1L, TRUE) > 0)
						return 1;
				pline(msg_slipsfree);
				return 1;
			}
		}
		dam = rnd(2) + dbon() + obj->spe;
		if (dam <= 0) dam = 1;
/*JP		You("hit your %s with your bullwhip.", body_part(FOOT));*/
		You("自分の%sを自分で打ちつけた．", body_part(FOOT));
		/* self_pronoun() won't work twice in a sentence */
/*JP		Strcpy(buf, self_pronoun("killed %sself with %%s bullwhip",
			"him"));*/
		Strcpy(buf, "自分自身を鞭打って");
/*JP		losehp(dam, self_pronoun(buf, "his"), NO_KILLER_PREFIX);*/
		losehp(dam, buf, KILLED_BY);
		flags.botl=1;
		return(1);
	} else if ((Fumbling || Glib) && !rn2(5)) {
/*JP		pline_The("bullwhip slips out of your %s.",*/
		pline("あなたは%sから鞭を落した．",
			body_part(HAND));
		dropx(obj);
		setuwep((struct obj *)0);
	}
	/*
	 *     Assumptions:
	 *
	 *		if you're in a pit
	 *			- you are attempting to get out of the pit
	 *			- or, if you are applying it towards a small
	 *			  monster then it is assumed that you are
	 *			  trying to hit it.
	 *		else if the monster is wielding a weapon
	 *			- you are attempting to disarm a monster
	 *		else
	 *			- you are attempting to hit the monster
	 *
	 *		if you're confused (and thus off the mark)
	 *			- you only end up hitting.
	 *
	 */
	else if(u.utrap && u.utraptype == TT_PIT) {
		const char *wrapped_what = (char *)0;

		if (mtmp) {
			if (bigmonst(mtmp->data)) {
				Strcpy(buf, mon_nam(mtmp));
				wrapped_what = buf;
			} else if (proficient) {
				if (attack(mtmp)) return(1);
				else pline(msg_snap);
			}
		}
		if (!wrapped_what) {
			if (IS_FURNITURE(levl[rx][ry].typ))
				wrapped_what = something;
			else if (sobj_at(BOULDER, rx, ry))
/*JP				wrapped_what = "a boulder";*/
				wrapped_what = "岩";
		}
		if (wrapped_what) {
			coord cc;

			cc.x = rx; cc.y = ry;
/*JP			You("wrap your bullwhip around %s.", wrapped_what);*/
			You("鞭を%sにからませた", wrapped_what);
			if (proficient && rn2(proficient + 2)) {
				if (!mtmp || enexto(&cc, rx, ry, youmonst.data)) {
/*JP					You("yank yourself out of the pit!");*/
					You("ぐいと引っぱった！");
					teleds(cc.x, cc.y);
					u.utrap = 0;
					vision_full_recalc = 1;
				}
			} else
				pline(msg_slipsfree);
			if (mtmp) wakeup(mtmp);
		} else pline(msg_snap);
	} else if (mtmp) {
		if (!canspotmon(mtmp) &&
				    !glyph_is_invisible(levl[rx][ry].glyph)) {
/*JP			pline("A monster is there that you couldn't see.");*/
			pline("見えない怪物がいる．");
			map_invisible(rx, ry);
		}
		otmp = MON_WEP(mtmp);	/* can be null */
		if (otmp) {
			char onambuf[BUFSZ];
			const char *mon_hand;
			boolean gotit = proficient && (!Fumbling || !rn2(10));

			Strcpy(onambuf, xname(otmp));
			if (gotit) {
			    mon_hand = mbodypart(mtmp, HAND);
			    if (bimanual(otmp))
				mon_hand = makeplural(mon_hand);
			} else
			    mon_hand = 0;	/* lint suppression */

/*JP			You("wrap your bullwhip around %s %s.",*/
			You("鞭を%sの%sにからませた．",
				s_suffix(mon_nam(mtmp)), onambuf);
			if (gotit && otmp->cursed) {
/*JP			    pline("%s welded to %s %s%c",
				  (otmp->quan == 1L) ? "It is" : "They are",
				  his[pronoun_gender(mtmp)], mon_hand,
				  !otmp->bknown ? '!' : '.');
*/
			    pline("%sをがっちり掴んだ%s",
				  mon_hand,
				  !otmp->bknown ? "！" : "．");
			    otmp->bknown = 1;
			    gotit = FALSE;	/* can't pull it free */
			}
			if (gotit) {
			    obj_extract_self(otmp);
			    possibly_unwield(mtmp);
			    otmp->owornmask &= ~W_WEP;
			    switch(rn2(proficient + 1)) {
				case 2:
				    /* to floor near you */
/*JP				    You("yank %s %s to the %s!",*/
				    You("%sの%sを%sに引き落した！",
					s_suffix(mon_nam(mtmp)),
					onambuf,
					surface(u.ux, u.uy));
				    if (otmp->otyp == CRYSKNIFE &&
				    	(!otmp->oerodeproof || !rn2(10))) {
				    	otmp->otyp = WORM_TOOTH;
				    	otmp->oerodeproof = 0;
				    }
				    place_object(otmp,u.ux, u.uy);
				    break;
				case 3:
				    /* right into your inventory */
				    if (rn2(25)) {
/*JP					You("snatch %s %s!",
						s_suffix(mon_nam(mtmp)),
						onambuf);
*/
					You("%sの%sを奪った！",
						s_suffix(mon_nam(mtmp)),
						onambuf);
					otmp = hold_another_object(otmp,
/*JP						"You drop %s!", doname(otmp),*/
						"%sを落した！", doname(otmp),
						(const char *)0);
				    /* proficient with whip, but maybe not
				       so proficient at catching weapons */
				    }
#if 0
				    else {
					int hitu, hitvalu;

					hitvalu = 8 + otmp->spe;
					hitu = thitu(hitvalu,
						dmgval(otmp, &youmonst),
						otmp, onambuf);
					if (hitu) {
				You("The %s hits you as you try to snatch it!",
						the(onambuf));
					}
					place_object(otmp, u.ux, u.uy);
				    }
#endif /* 0 */
				    break;
				default:
				    /* to floor beneath mon */
/*JP				    You("yank %s from %s %s!",
					the(onambuf),
					s_suffix(mon_nam(mtmp)),
					mon_hand);
*/
				    You("%sを%sの%sからひっぱった！",
					the(xname(otmp)),
					s_suffix(mon_nam(mtmp)),
					body_part(HAND));
				    if (otmp->otyp == CRYSKNIFE &&
				    	(!otmp->oerodeproof || !rn2(10))) {
				    	otmp->otyp = WORM_TOOTH;
				    	otmp->oerodeproof = 0;
				    }
				    place_object(otmp, mtmp->mx, mtmp->my);
				    break;
			    }
			} else {
				pline(msg_slipsfree);
			}
			wakeup(mtmp);
		} else {
/*JP			You("flick your bullwhip towards %s.", mon_nam(mtmp));*/
			You("%sに向って鞭を打った．", mon_nam(mtmp));
			if (proficient) {
				if (attack(mtmp)) return(1);
				else pline(msg_snap);
			}
		}
	} else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
		/* it must be air -- water checked above */
/*JP		You("snap your whip through thin air.");*/
		You("何もないところでムチを打った．");
	} else
		pline(msg_snap);
	return(1);
}


static const char
/*JP	*not_enough_room = "There's not enough room here to use that.",
	*where_to_hit = "Where do you want to hit?",
	*cant_see_spot = "won't hit anything if you can't see that spot.";*/
	*not_enough_room = "それを使うだけの広さがない．",
	*where_to_hit = "どれを狙う？",
	*cant_see_spot = "場所が見えなければ狙えない．";

/* Distance attacks by pole-weapons */
STATIC_OVL int
use_pole (obj)
	struct obj *obj;
{
	int res = 0, typ, max_range = 4, min_range = 4;
	coord cc;
	struct monst *mtmp;


	/* Are you allowed to use the pole? */
	if (u.uswallow) {
	    pline(not_enough_room);
	    return (0);
	}
	if (obj != uwep) {
	    if (!wield_tool(obj)) return(0);
	    else res = 1;
	}

	/* Prompt for a location */
	pline(where_to_hit);
	cc.x = u.ux;
	cc.y = u.uy;
/*JP	if (getpos(&cc, TRUE, "the spot to hit") < 0)*/
	if (getpos(&cc, TRUE, "狙う場所") < 0)
	    return 0;	/* user pressed ESC */

	/* Calculate range */
	typ = weapon_type(obj);
	if (typ == P_NONE || P_SKILL(typ) <= P_BASIC) max_range = 4;
	else if (P_SKILL(typ) == P_SKILLED) max_range = 5;
	else max_range = 8;
	if (distu(cc.x, cc.y) > max_range) {
/*JP	    pline("Too far!");*/
	    pline("遠すぎる！");
	    return (res);
	} else if (distu(cc.x, cc.y) < min_range) {
/*JP	    pline("Too close!");*/
	    pline("近すぎる");
	    return (res);
	} else if (!cansee(cc.x, cc.y)) {
	    You(cant_see_spot);
	    return (res);
	}

	/* Attack the monster there */
	if ((mtmp = m_at(cc.x, cc.y)) != (struct monst *)0)
	    (void) thitmonst(mtmp, obj);
	else
	    /* Now you know that nothing is there... */
	    pline(nothing_happens);
	return (1);
}


STATIC_OVL int
use_grapple (obj)
	struct obj *obj;
{
	int res = 0, typ, max_range = 4;
	coord cc;
	struct monst *mtmp;
	struct obj *otmp;


	/* Are you allowed to use the hook? */
	if (u.uswallow) {
	    pline(not_enough_room);
	    return (0);
	}
	if (obj != uwep) {
	    if (!wield_tool(obj)) return(0);
	    else res = 1;
	}

	/* Prompt for a location */
	pline(where_to_hit);
	cc.x = u.ux;
	cc.y = u.uy;
/*JP	if (getpos(&cc, TRUE, "the spot to hit") < 0)*/
	if (getpos(&cc, TRUE, "狙う場所") < 0)
	    return 0;	/* user pressed ESC */

	/* Calculate range */
	typ = weapon_type(obj);
	if (typ == P_NONE || P_SKILL(typ) <= P_BASIC) max_range = 4;
	else if (P_SKILL(typ) == P_SKILLED) max_range = 5;
	else max_range = 8;
	if (distu(cc.x, cc.y) > max_range) {
/*JP		pline("Too far!");*/
		pline("遠すぎる！");
		return (res);
	} else if (!cansee(cc.x, cc.y)) {
		You(cant_see_spot);
		return (res);
	}

	/* What did you hit? */
	switch (rn2(5))
	{
	case 0:	/* Trap */
	    /* FIXME -- untrap needs to deal with non-adjacent traps */
	    break;
	case 1:	/* Object */
	    if ((otmp = level.objects[cc.x][cc.y]) !=
	    		(struct obj *)0) {
/*JP	    	You("snag an object from the %s!", surface(cc.x, cc.y));*/
	    	You("%sのものを引っ掛けた！", surface(cc.x, cc.y));
	    	(void) pickup_object(otmp, 1L, FALSE);
	    	/* If pickup fails, leave it alone */
	    	newsym(cc.x, cc.y);
	    	return (1);
	    }
	    break;
	case 2:	/* Monster */
	    if ((mtmp = m_at(cc.x, cc.y)) == (struct monst *)0) break;
	    if (verysmall(mtmp->data) && !rn2(4) &&
	    		enexto(&cc, u.ux, u.uy, (struct permonst *)0)) {
/*JP	    	You("pull in %s!", mon_nam(mtmp));*/
	    	You("%sを引っ張った！", mon_nam(mtmp));
	    	mtmp->mundetected = 0;
	    	rloc_to(mtmp, cc.x, cc.y);
	    	return (1);
	    } else if ((!bigmonst(mtmp->data) && !strongmonst(mtmp->data)) ||
		       rn2(4)) {
	    	(void) thitmonst(mtmp, obj);
	    	return (1);
	    }
	    /* FALL THROUGH */
	case 3:	/* Surface */
/*JP	    You("are yanked toward the %s!",*/
	    You("%sへ引っぱられた！",
	    		surface(cc.x, cc.y));
	    hurtle(sgn(cc.x-u.ux), sgn(cc.y-u.uy), 1, FALSE);
	    return (1);
	default:	/* Yourself (oops!) */
	    if (P_SKILL(typ) <= P_BASIC) {
/*JP	    	You("hook yourself!");
	    	losehp(rn1(10,10), "a grappling hook", KILLED_BY);
*/
	    	You("自分自身を引っ掛けた");
	    	losehp(rn1(10,10), "自分自身を引っ掛けて", KILLED_BY);
	    	return (1);
	    }
	    break;
	}
	pline(nothing_happens);
	return (1);
}


#define BY_OBJECT	((struct monst *)0)

/* return 1 if the wand is broken, hence some time elapsed */
STATIC_OVL int
do_break_wand(obj)
    struct obj *obj;
{
/*JP
    static const char nothing_else_happens[] = "But nothing else happens...";
*/
    static const char nothing_else_happens[] = "しかし，何も起きなかった．．．";
    register int i, x, y;
    register struct monst *mon;
    int dmg, damage;
    boolean affects_objects;
    char confirm[QBUFSZ], the_wand[BUFSZ];

    Strcpy(the_wand, yname(obj));
/*JP    Sprintf(confirm, "Are you really sure you want to break %s?", the_wand);*/
    Sprintf(confirm, "本当に%sを壊すの？", the_wand);
    if (yn(confirm) == 'n' ) return 0;

    if (nohands(youmonst.data)) {
/*JP	You_cant("break %s without hands!", the_wand);*/
 	You("手が無いので%sを壊せない！", the_wand);
	return 0;
    } else if (ACURR(A_STR) < 10) {
/*JP	You("don't have the strength to break %s!", the_wand);*/
	You("%sを壊すだけの力がない！", the_wand);
	return 0;
    }
/*JP
    pline("Raising %s high above your %s, you break it in two!",
	  the_wand, body_part(HEAD));
*/
    pline("%sを%sの上にあげ二つにへし折った！",
	  the_wand, body_part(HEAD));

    current_wand = obj;		/* destroy_item might reset this */
    freeinv(obj);		/* hide it from destroy_item instead... */

    if (obj->spe <= 0) {
	pline(nothing_else_happens);
	goto discard_broken_wand;
    }
    obj->ox = u.ux;
    obj->oy = u.uy;
    dmg = obj->spe * 4;
    affects_objects = FALSE;

    switch (obj->otyp) {
    case WAN_WISHING:
    case WAN_NOTHING:
    case WAN_LOCKING:
    case WAN_PROBING:
    case WAN_ENLIGHTENMENT:
    case WAN_OPENING:
    case WAN_SECRET_DOOR_DETECTION:
	pline(nothing_else_happens);
	goto discard_broken_wand;
    case WAN_DEATH:
    case WAN_LIGHTNING:
	dmg *= 2;
    case WAN_FIRE:
    case WAN_COLD:
	dmg *= 2;
    case WAN_MAGIC_MISSILE:
	explode(u.ux, u.uy, (obj->otyp - WAN_MAGIC_MISSILE), dmg, WAND_CLASS);
	makeknown(obj->otyp);	/* explode described the effect */
	goto discard_broken_wand;
    case WAN_STRIKING:
	/* we want this before the explosion instead of at the very end */
/*JP	pline("A wall of force smashes down around you!");*/
	pline("あなたは魔力の壁につつまれた！");
	dmg = d(1 + obj->spe,6);	/* normally 2d12 */
    case WAN_CANCELLATION:
    case WAN_POLYMORPH:
    case WAN_TELEPORTATION:
    case WAN_UNDEAD_TURNING:
	affects_objects = TRUE;
	break;
    default:
	break;
    }

    /* magical explosion and its visual effect occur before specific effects */
    explode(obj->ox, obj->oy, 0, rnd(dmg), WAND_CLASS);

    /* this makes it hit us last, so that we can see the action first */
    for (i = 0; i <= 8; i++) {
	bhitpos.x = x = obj->ox + xdir[i];
	bhitpos.y = y = obj->oy + ydir[i];
	if (!isok(x,y)) continue;

	if (obj->otyp == WAN_DIGGING) {
	    if(dig_check(BY_OBJECT, FALSE, x, y))
		digactualhole(x, y, BY_OBJECT,
			      (rn2(obj->spe) < 3 || !Can_dig_down(&u.uz)) ?
			       PIT : HOLE);
	    continue;
	} else if(obj->otyp == WAN_CREATE_MONSTER) {
	    (void) makemon((struct permonst *)0, x, y, NO_MM_FLAGS);
	    continue;
	} else {
	    if (x == u.ux && y == u.uy) {
		damage = zapyourself(obj, FALSE);
		if (damage)
		    losehp(damage,
/*JP			   self_pronoun("killed %sself by breaking a wand",
					"him"),*/
			   "自分自身で杖を壊してダメージを受け",
/*JP			   NO_KILLER_PREFIX);*/
			   KILLED_BY);
		if (flags.botl) bot();		/* blindness */
	    } else if ((mon = m_at(x, y)) != 0) {
		(void) bhitm(mon, obj);
	     /* if (flags.botl) bot(); */
	    }
	    if (affects_objects && level.objects[x][y]) {
		(void) bhitpile(obj, bhito, x, y);
		if (flags.botl) bot();		/* potion effects */
	    }
	}
    }

    if (obj->otyp == WAN_LIGHT)
	litroom(TRUE, obj);	/* only needs to be done once */

 discard_broken_wand:
    obj = current_wand;		/* [see dozap() and destroy_item()] */
    current_wand = 0;
    if (obj) {
	/* extra charge for _use_ prior to destruction */
	check_unpaid(obj);
	delobj(obj);
    }
    nomul(0);
    return 1;
}

int
doapply()
{
	register struct obj *obj;
	register int res = 1;

	if(check_capacity((char *)0)) return (0);
/*JP	obj = getobj(carrying(POT_OIL) ? tools_too : tools, "use or apply");*/
	obj = getobj(carrying(POT_OIL) ? tools_too : tools, "使う");
	if(!obj) return 0;

	if (obj->oclass == WAND_CLASS)
	    return do_break_wand(obj);

	switch(obj->otyp){
	case BLINDFOLD:
	case LENSES:
		if (obj == ublindf) {
		    if (!cursed(obj)) Blindf_off(obj);
		} else if (!ublindf)
		    Blindf_on(obj);
/*JP
		else You("are already %s.",
			ublindf->otyp == TOWEL ?     "covered by a towel" : 
			ublindf->otyp == BLINDFOLD ? "wearing a blindfold" :
						     "wearing lenses");
*/
		else You("もう%s．",
			ublindf->otyp == TOWEL ?     "タオルを巻いている" : 
			ublindf->otyp == BLINDFOLD ? "目隠しをつけている" :
						     "レンズをつけている");
		break;
	case BULLWHIP:
		res = use_whip(obj);
		break;
	case GRAPPLING_HOOK:
		res = use_grapple(obj);
		break;
	case LARGE_BOX:
	case CHEST:
	case ICE_BOX:
	case SACK:
	case BAG_OF_HOLDING:
	case OILSKIN_SACK:
		res = use_container(obj, 1);
		break;
	case BAG_OF_TRICKS:
		if(obj->spe > 0) {
			register int cnt = 1;

			check_unpaid(obj);
			obj->spe--;
			if(!rn2(23)) cnt += rn2(7) + 1;
			while(cnt--)
			   (void) makemon((struct permonst *) 0,
						u.ux, u.uy, NO_MM_FLAGS);
			makeknown(BAG_OF_TRICKS);
		} else
			pline(nothing_happens);
		break;
	case CAN_OF_GREASE:
		use_grease(obj);
		break;
	case LOCK_PICK:
#ifdef TOURIST
	case CREDIT_CARD:
#endif
	case SKELETON_KEY:
		(void) pick_lock(obj);
		break;
	case PICK_AXE:
	case DWARVISH_MATTOCK:
		res = use_pick_axe(obj);
		break;
	case TINNING_KIT:
		use_tinning_kit(obj);
		break;
	case LEASH:
		use_leash(obj);
		break;
#ifdef STEED
	case SADDLE:
		res = use_saddle(obj);
		break;
#endif
	case MAGIC_WHISTLE:
		use_magic_whistle(obj);
		break;
	case TIN_WHISTLE:
		use_whistle(obj);
		break;
	case STETHOSCOPE:
		res = use_stethoscope(obj);
		break;
	case MIRROR:
		res = use_mirror(obj);
		break;
	case BELL:
	case BELL_OF_OPENING:
		use_bell(obj);
		break;
	case CANDELABRUM_OF_INVOCATION:
		use_candelabrum(obj);
		break;
	case WAX_CANDLE:
	case TALLOW_CANDLE:
		use_candle(obj);
		break;
	case OIL_LAMP:
	case MAGIC_LAMP:
	case BRASS_LANTERN:
		use_lamp(obj);
		break;
	case POT_OIL:
		light_cocktail(obj);
		break;
#ifdef TOURIST
	case EXPENSIVE_CAMERA:
		res = use_camera(obj);
		break;
#endif
	case TOWEL:
		res = use_towel(obj);
		break;
	case CRYSTAL_BALL:
		use_crystal_ball(obj);
		break;
	case MAGIC_MARKER:
		res = dowrite(obj);
		break;
	case TIN_OPENER:
		if(!carrying(TIN)) {
/*JP			You("have no tin to open.");*/
			You("缶を持っていない．");
			goto xit;
		}
/*JP		You("cannot open a tin without eating or discarding its contents.");*/
		pline("中身を食べるか，捨てるかしないと缶を空にできない．");
		if(flags.verbose)
/*JP			pline("In order to eat, use the 'e' command.");*/
			pline("食べるには，'e'コマンドを使えばよい．");
		if(obj != uwep)
/*JP    pline("Opening the tin will be much easier if you wield the tin opener.");*/
    pline("缶切りを装備していれば，ずっと簡単に開けることができる．");
		goto xit;

	case FIGURINE:
		use_figurine(obj);
		break;
	case UNICORN_HORN:
		use_unicorn_horn(obj);
		break;
	case WOODEN_FLUTE:
	case MAGIC_FLUTE:
	case TOOLED_HORN:
	case FROST_HORN:
	case FIRE_HORN:
	case WOODEN_HARP:
	case MAGIC_HARP:
	case BUGLE:
	case LEATHER_DRUM:
	case DRUM_OF_EARTHQUAKE:
		res = do_play_instrument(obj);
		break;
	case HORN_OF_PLENTY:	/* not a musical instrument */
		if (obj->spe > 0) {
		    struct obj *otmp;
		    const char *what;

		    check_unpaid(obj);
		    obj->spe--;
		    if (!rn2(13)) {
			otmp = mkobj(POTION_CLASS, FALSE);
			if (objects[otmp->otyp].oc_magic) do {
			    otmp->otyp = rnd_class(POT_BOOZE, POT_WATER);
			} while (otmp->otyp == POT_SICKNESS);
/*JP			what = "A potion";*/
			what = "薬";
		    } else {
			otmp = mkobj(FOOD_CLASS, FALSE);
			if (otmp->otyp == FOOD_RATION && !rn2(7))
			    otmp->otyp = LUMP_OF_ROYAL_JELLY;
/*JP			what = "Some food";*/
			what = "食べ物";
		    }
/*JP		    pline("%s spills out.", what);*/
		    pline("%sが飛び出てきた．", what);
		    otmp->blessed = obj->blessed;
		    otmp->cursed = obj->cursed;
		    otmp->owt = weight(otmp);
		    otmp = hold_another_object(otmp,
					(u.uswallow || Is_airlevel(&u.uz) ||
					 u.uinwater || Is_waterlevel(&u.uz)) ?
/*JP					       "Oops!  %s away from you!" :
					       "Oops!  %s to the floor!",
					       The(aobjnam(otmp, "slip")),*/
					       "おっと！%sはあなたの手から滑り落ちた" :
					       "おっと！%sは床に滑り落ちた",
/*JP					       The(aobjnam(otmp, "slip")),*/
					       The(xname(otmp)),
					       (const char *)0);
		    makeknown(HORN_OF_PLENTY);
		} else
		    pline(nothing_happens);
		break;
	case LAND_MINE:
	case BEARTRAP:
		use_trap(obj);
		break;
	default:
		/* Pole-weapons can strike at a distance */
		if (is_pole(obj)) {
			res = use_pole(obj);
			break;
		} else if (is_pick(obj) /* || is_axe(obj) */) {
			res = use_pick_axe(obj);
			break;
		}
/*JP		pline("Sorry, I don't know how to use that.");*/
		pline("それをどうやって使うんだい？");
	xit:
		nomul(0);
		return 0;
	}
	nomul(0);
	return res;
}

#endif /* OVLB */

/*apply.c*/
