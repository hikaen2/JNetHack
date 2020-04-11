/*	SCCS Id: @(#)apply.c	3.1	93/06/24	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "edog.h"

#ifdef OVLB

static NEARDATA const char tools[] = { TOOL_CLASS, 0 };

static NEARDATA boolean did_dig_msg;

#ifdef TOURIST
static int FDECL(use_camera, (struct obj *));
#endif
static int FDECL(use_towel, (struct obj *));
static void FDECL(use_stethoscope, (struct obj *));
static void FDECL(use_whistle, (struct obj *));
static void FDECL(use_magic_whistle, (struct obj *));
#ifdef WALKIES
static void FDECL(use_leash, (struct obj *));
#endif
STATIC_DCL int NDECL(dig);
#ifdef OVLB
STATIC_DCL schar FDECL(fillholetyp, (int, int));
#endif
static boolean FDECL(wield_tool, (struct obj *));
static int FDECL(use_pick_axe, (struct obj *));
static int FDECL(use_mirror, (struct obj *));
static void FDECL(use_bell, (struct obj *));
static void FDECL(use_candelabrum, (struct obj *));
static void FDECL(use_candle, (struct obj *));



static void FDECL(use_lamp, (struct obj *));
static void FDECL(use_tinning_kit, (struct obj *));
static void FDECL(use_figurine, (struct obj *));
static void FDECL(use_grease, (struct obj *));
static boolean NDECL(rm_waslit);
static void FDECL(mkcavepos, (XCHAR_P,XCHAR_P,int,BOOLEAN_P,BOOLEAN_P));
static void FDECL(mkcavearea, (BOOLEAN_P));
static void FDECL(digactualhole, (int));

#ifdef	AMIGA
void FDECL( amii_speaker, ( struct obj *, char *, int ) );
#endif

#ifdef TOURIST
static int
use_camera(obj)
	struct obj *obj;
{
	register struct monst *mtmp;

	if(Underwater) {
/*JP		pline("Using your camera underwater would void the warranty.");*/
		pline("そんなことしたら保証が効かなくなる．");
		return(0);
	}
	if(!getdir(NULL)) return(0);
	if(u.uswallow) {
/*JP		You("take a picture of %s's %s.", mon_nam(u.ustuck),
		    is_animal(u.ustuck->data) ? "stomach" : "interior");*/
		You("%sの%sの写真を撮った．", mon_nam(u.ustuck),
		    is_animal(u.ustuck->data) ? "胃" : "内部");
	} else if(obj->cursed && !rn2(2)) goto blindu;
	else if(u.dz) {
/*JP		You("take a picture of the %s.",
			(u.dz > 0) ? surface(u.ux,u.uy) : "ceiling");*/
		You("%sの写真を撮った",
			(u.dz > 0) ? surface(u.ux,u.uy) : "天井");
	} else if(!u.dx && !u.dy) {
blindu:
		if(!Blind) {
/*JP			You("are blinded by the flash!");*/
			You("フラッシュで目がくらんだ！");
			make_blinded((long)rnd(25),FALSE);
		}
	} else if ((mtmp = bhit(u.dx,u.dy,COLNO,FLASHED_LIGHT,
				(int(*)())0,(int(*)())0,obj)) != 0) {
		if(mtmp->msleep) {
		    mtmp->msleep = 0;
		    if(cansee(mtmp->mx,mtmp->my))
/*JP			pline("The flash awakens %s.", mon_nam(mtmp)); /* a3 */
			pline("フラッシュで%sの目が覚めた．", mon_nam(mtmp));
		} else if (mtmp->data->mlet != S_LIGHT)
		    if(mtmp->mcansee && haseyes(mtmp->data)) {
			register int tmp = distu(mtmp->mx,mtmp->my);

			if(cansee(mtmp->mx,mtmp->my))
/*JP			    pline("%s is blinded by the flash!", Monnam(mtmp));*/
			    pline("%sはフラッシュで目がくらんだ！", Monnam(mtmp));
			if(mtmp->data == &mons[PM_GREMLIN]) {
			    /* Rule #1: Keep them out of the light. */
/*JP			    pline("%s cries out in pain!", Monnam(mtmp));*/
			    pline("%sは苦痛の叫びをあげた！", Monnam(mtmp));
			    if (mtmp->mhp > 1) mtmp->mhp--;
			}
			setmangry(mtmp);
			if(tmp < 9 && !mtmp->isshk && rn2(4)) {
				mtmp->mflee = 1;
				if(rn2(4)) mtmp->mfleetim = rnd(100);
			}
			mtmp->mcansee = 0;
			if(tmp < 3) {
				mtmp->mblinded = 0;
			} else {
				mtmp->mblinded = rnd(1 + 50/tmp);
			}
		    }
	}
	return 1;
}
#endif

static int
use_towel(obj)
	struct obj *obj;
{
	if(!freehand()) {
/*JP		You("have no free %s!", body_part(HAND));*/
		You("空いている%sがない！", body_part(HAND));
		return 0;
	} else if (obj->owornmask) {
/*JP		You("cannot use it while you're wearing it!");*/
		You("それは身につけていては使用できない！");
		return 0;
	} else if (obj->cursed) {
		long old;
		switch (rn2(3)) {
		case 2:
		    old = Glib;
		    Glib += rn1(10, 3);
/*JP		    Your("%s %s!", makeplural(body_part(HAND)),
			(old ? "are filthier than ever" : "get slimy"));*/
		    Your("%sは%s！", makeplural(body_part(HAND)),
			(old ? "ますます汚なくなった" : "ぬるぬるになった"));
		    return 1;
		case 1:
		    if (!Blindfolded) {
			old = u.ucreamed;
			u.ucreamed += rn1(10, 3);
/*JP			pline("Yecch! Your %s %s gunk on it!", body_part(FACE),
			      (old ? "has more" : "now has"));*/
			pline("ゲェー！あなたの%sは%sべとべとになった！", body_part(FACE),
			      (old ? "もっと" : ""));
			make_blinded(Blinded + (long)u.ucreamed - old, TRUE);
		    } else {
			if (ublindf->cursed) {
/*JP			    You("push your blindfold %s.",
				rn2(2) ? "cock-eyed" : "crooked");*/
			    You("目隠しを%s．",
				rn2(2) ? "ゆがめた" : "ゆるめた");
			} else {
/*JP			    You("push your blindfold off.");*/
			    You("目隠しをはずした．");
			    Blindf_off(ublindf);
			    dropx(ublindf);
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

/*JPstatic char hollow_str[] = "hear a hollow sound.  This must be a secret %s!";*/
static char hollow_str[] = "虚ろな音を聞いた．秘密の%sに違いない！";

/* Strictly speaking it makes no sense for usage of a stethoscope to
   not take any time; however, unless it did, the stethoscope would be
   almost useless. */
static void
use_stethoscope(obj)
	register struct obj *obj;
{
	register struct monst *mtmp;
	register struct rm *lev;
	register int rx, ry;

	if(!freehand()) {
/*JP		You("have no free %s.", body_part(HAND));*/
		You("空いている%sはない．", body_part(HAND));
		return;
	}
	if (!getdir(NULL)) return;
	if (u.uswallow && (u.dx || u.dy || u.dz)) {
		mstatusline(u.ustuck);
		return;
	} else if (u.dz) {
		if (Underwater)
/*JP		    You("hear faint splashing.");*/
		    You("かすかにバシャバシャと言う音を聞いた．");
		else if (u.dz < 0 || Levitation)
/*JP		    You("can't reach the %s.",
			(u.dz > 0) ? surface(u.ux,u.uy) : "ceiling");*/
		    You("%sに届かない．",
			(u.dz > 0) ? surface(u.ux,u.uy) : "天井");
		else if (Is_stronghold(&u.uz))
/*JP		    You("hear the crackling of hellfire.");*/
		    You("地獄の炎がパチパチ燃えている音を聞いた．");
		else
/*JP		    pline("The %s seems healthy enough.", surface(u.ux,u.uy));*/
		    pline("%sは充分健康のようだ．", surface(u.ux,u.uy));
		return;
	} else if (obj->cursed && !rn2(2)) {
/*JP		You("hear your heart beat.");*/
		You("自分の心臓の鼓動を聞いた．");
		return;
	}
	if (Stunned || (Confusion && !rn2(5))) confdir();
	if (!u.dx && !u.dy) {
		ustatusline();
		return;
	}
	rx = u.ux + u.dx; ry = u.uy + u.dy;
	if (!isok(rx,ry)) {
/*JP		You("hear a faint typing noise.");*/
		You("かすかにだれかがタイピングしている音を聞いた．");
		return;
	}
	if ((mtmp = m_at(rx,ry)) != 0) {
		mstatusline(mtmp);
		if (mtmp->mundetected) {
			mtmp->mundetected = 0;
			if (cansee(rx,ry)) newsym(mtmp->my,mtmp->my);
		}
		return;
	}
	lev = &levl[rx][ry];
	switch(lev->typ) {
	case SDOOR:
/*JP		You(hollow_str, "door");*/
		You(hollow_str, "扉");
		lev->typ = DOOR;
		newsym(rx,ry);
		return;
	case SCORR:
/*JP		You(hollow_str, "passage");*/
		You(hollow_str, "通路");
		lev->typ = CORR;
		newsym(rx,ry);
		return;
	}
/*JP	You("hear nothing special.");*/
	pline("別に何も聞こえない．");
}

/*JPstatic char whistle_str[] = "produce a %s whistling sound.";*/
static char whistle_str[] = "笛を吹いて%s音をたてた．";

/*ARGSUSED*/
static void
use_whistle(obj)
struct obj *obj;
#if defined(applec)
# pragma unused(obj)
#endif
{
/*JP	You(whistle_str, "high");*/
	You(whistle_str, "かん高い");
	wake_nearby();
}

static void
use_magic_whistle(obj)
struct obj *obj;
{
	register struct monst *mtmp;

	if(obj->cursed && !rn2(2)) {
/*JP		You("produce a high-pitched humming noise.");*/
		You("調子の高いうなるような音をたてた．");
		wake_nearby();
	} else {
		makeknown(MAGIC_WHISTLE);
/*JP		You(whistle_str, Hallucination ? "normal" : "strange");*/
		You(whistle_str, Hallucination ? "普通の" : "奇妙な");
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			if(mtmp->mtame) mnexto(mtmp);
	}
}

boolean
um_dist(x,y,n)
register xchar x, y, n;
{
	return((boolean)(abs(u.ux - x) > n  || abs(u.uy - y) > n));
}

#endif /* OVLB */

#ifdef WALKIES
#define MAXLEASHED	2

#ifdef OVLB

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

/* ARGSUSED */
static void
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

	if(!getdir(NULL)) return;

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

	spotmon = canseemon(mtmp) || sensemon(mtmp);

	if(!mtmp->mtame) {
	    if(!spotmon)
/*JP		pline("There is no creature there.");*/
		pline("そこには生き物はいない．");
	    else
/*JP		pline("%s %s leashed!", Monnam(mtmp), (!obj->leashmon) ?
				"cannot be" : "is not");*/
		pline("%sは紐で%s！", Monnam(mtmp), (!obj->leashmon) ?
				"結べない" : "結ばれない");
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
		if(mtmp->msleep)  mtmp->msleep = 0;
		return;
	}
	if(obj->leashmon != (int)mtmp->m_id) {
/*JP		pline("This leash is not attached to that creature.");*/
		pline("この紐はその生物には結ばれていない．");
		return;
	} else {
		if(obj->cursed) {
/*JP			pline("The leash would not come off!");*/
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
/*JP				    You("feel %s leash go slack.",
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
#ifdef OVLB
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

#endif /* WALKIES */
#ifdef OVL0

#ifdef WALKIES
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
			if(otmp->cursed) {
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
# ifdef SOUNDS
				    if (mtmp->data->msound != MS_SILENT)
				    switch(rn2(3)) {
					case 0:  growl(mtmp);	break;
					case 1:  yelp(mtmp);	break;
					default: whimper(mtmp); break;
				    }
# endif
				}
			    }
			}
		    }
		    mtmp = mtmp->nmon;
		}
	    }
}

#endif /* WALKIES */

#endif /* OVL0 */
#ifdef OVLB

static boolean
rm_waslit() {
    register xchar x, y;

    if(levl[u.ux][u.uy].typ == ROOM && levl[u.ux][u.uy].waslit)
	return(TRUE);
    for(x = u.ux-2; x < u.ux+3; x++)
	for(y = u.uy-1; y < u.uy+2; y++)
	    if(isok(x,y) && levl[x][y].waslit) return(TRUE);
    return(FALSE);
}

/* Change level topology.  Messes with vision tables and ignores things like
 * boulders in the name of a nice effect.  Vision will get fixed up again
 * immediately after the effect is complete.
 */
static void
mkcavepos(x, y, dist, waslit, rockit)
    xchar x,y;
    int dist;
    boolean waslit, rockit;
{
    register struct rm *lev;

    if(!isok(x,y)) return;
    lev = &levl[x][y];

    if(rockit) {
	register struct monst *mtmp;

	if(IS_ROCK(lev->typ)) return;
	if(t_at(x, y)) return; /* don't cover the portal */
	if ((mtmp = m_at(x, y)) != 0)	/* make sure crucial monsters survive */
	    if(!passes_walls(mtmp->data)) rloc(mtmp);
    } else if(lev->typ == ROOM) return;

    unblock_point(x,y);	/* make sure vision knows this location is open */

    /* fake out saved state */
    lev->seen = FALSE;
    lev->doormask = 0;
    if(dist < 3) lev->lit = (rockit ? FALSE : TRUE);
    if(waslit) lev->waslit = (rockit ? FALSE : TRUE);
    lev->horizontal = FALSE;
    viz_array[y][x] = (dist < 3 ) ?
	(IN_SIGHT|COULD_SEE) : /* short-circuit vision recalc */
	COULD_SEE;
    lev->typ = (rockit ? STONE : ROOM);
    if(dist >= 3)
	impossible("mkcavepos called with dist %d", dist);
    if(Blind)
	feel_location(x, y);
    else newsym(x,y);
}

static void
mkcavearea(rockit)
register boolean rockit;
{
    int dist;
    xchar xmin = u.ux, xmax = u.ux;
    xchar ymin = u.uy, ymax = u.uy;
    register xchar i;
    register boolean waslit = rm_waslit();

/*JP    if(rockit) pline("Crash!  The ceiling collapses around you!");*/
    if(rockit) pline("げげん！あなたのまわりの天井が崩れた！");
/*JP    else pline("A mysterious force %s cave around you!",
	     (levl[u.ux][u.uy].typ == CORR) ? "creates a" : "extends the");*/
    else pline("神秘的な力によりあなたのまわり%s！",
	     (levl[u.ux][u.uy].typ == CORR) ? "に洞窟ができた" : "の洞窟が広がった");
    display_nhwindow(WIN_MESSAGE, TRUE);

    for(dist = 1; dist <= 2; dist++) {
	xmin--; xmax++;

	/* top and bottom */
	if(dist < 2) { /* the area is wider that it is high */
	    ymin--; ymax++;
	    for(i = xmin+1; i < xmax; i++) {
		mkcavepos(i, ymin, dist, waslit, rockit);
		mkcavepos(i, ymax, dist, waslit, rockit);
	    }
	}

	/* left and right */
	for(i = ymin; i <= ymax; i++) {
	    mkcavepos(xmin, i, dist, waslit, rockit);
	    mkcavepos(xmax, i, dist, waslit, rockit);
	}

	flush_screen(1);	/* make sure the new glyphs shows up */
	delay_output();
    }

    if(!rockit && levl[u.ux][u.uy].typ == CORR) {
	levl[u.ux][u.uy].typ = ROOM;
	if(waslit) levl[u.ux][u.uy].waslit = TRUE;
	newsym(u.ux, u.uy); /* in case player is invisible */
    }

    vision_full_recalc = 1;	/* everything changed */
}

STATIC_OVL int
dig()
{
	register struct rm *lev;
	register xchar dpx = dig_pos.x, dpy = dig_pos.y;

	lev = &levl[dpx][dpy];
	/* perhaps a nymph stole your pick-axe while you were busy digging */
	/* or perhaps you teleported away */
	if(u.uswallow || !uwep || uwep->otyp != PICK_AXE ||
	    !on_level(&dig_level, &u.uz) ||
	    ((dig_down && (dpx != u.ux || dpy != u.uy)) ||
	     (!dig_down && distu(dpx,dpy) > 2)))
		return(0);

	if (dig_down) {
	    struct trap *ttmp;

	    if (On_stairs(u.ux, u.uy)) {
		if (u.ux == xdnladder || u.ux == xupladder)
/*JP		     pline("The ladder resists your effort.");*/
		     pline("梯子が邪魔をした．");
/*JP		else pline("The stairs are too hard to dig in.");*/
		else pline("階段はとても固くて掘れない．");
		return(0);
	    } else if (IS_THRONE(levl[u.ux][u.uy].typ)) {
/*JP		pline("The throne is too hard to break apart.");*/
		pline("玉座はとても固くて砕けない．");
		return (0);
	    } else if (IS_ALTAR(levl[u.ux][u.uy].typ)) {
/*JP		pline("The altar is too hard to break apart.");*/
		pline("祭壇はとても固くて砕けない．");
		return (0);
	    } else if (Is_airlevel(&u.uz)) {
/*JP		You("cannot dig in thin air.");*/
		You("何もない空間は掘れない．");
		return(0);
	    } else if (Is_waterlevel(&u.uz)) {
/*JP		pline("The water splashes and subsides.");*/
		pline("水ははねて、静まった．");
		return(0);
	    } else if ((ttmp = t_at(dpx, dpy)) &&
			(ttmp->ttyp == MAGIC_PORTAL || !Can_dig_down(&u.uz))) {
/*JP		pline("The %s here is too hard to dig in.",*/
		pline("%sはとても固くて掘れない．",
			surface(dpx,dpy));
		return(0);
	    } else if (sobj_at(BOULDER, dpx, dpy)) {
/*JP		pline("There isn't enough room to dig here.");*/
		pline("ここには穴を掘れるだけの広さがない．");
		return(0);
	    }
	} else { /* !dig_down */
	    if(IS_ROCK(lev->typ) && !may_dig(dpx,dpy)) {
/*JP		pline("This wall is too hard to dig into.");*/
		pline("この壁はとても固くて掘れない．");
		return(0);
	    }
	}
	if(Fumbling && !rn2(3)) {
		switch(rn2(3)) {
		case 0:  if(!welded(uwep)) {
/*JP			     You("fumble and drop your %s.", xname(uwep));*/
			     You("手が滑り%sを落した．", xname(uwep));
			     dropx(uwep);
			     setuwep((struct obj *)0);
			 } else {
/*JP			     pline("Ouch!  Your %s bounces and hits you!",*/
			     pline("いてっ！%sは跳ねかえりあなたに命中した！",
				xname(uwep));
			     set_wounded_legs(RIGHT_SIDE, 5 + rnd(5));
			 }
			 break;
/*JP		case 1:  pline("Bang!  You hit with the broad side of %s!",*/
		case 1:  pline("バン！%sの幅広い方で打ってしまった！",
			       the(xname(uwep)));
			 break;
/*JP		default: Your("swing misses its mark.");*/
		default: You("狙いを定めたが，空振した．");
			 break;
		}
		return(0);
	}
	dig_effort += 10 + abon() + uwep->spe - uwep->oeroded + rn2(5);
	if(dig_down) {
		register struct trap *ttmp;

		if(dig_effort > 250) {
			(void) dighole(FALSE);
			dig_level.dnum = 0;
			dig_level.dlevel = -1;
			return(0);	/* done with digging */
		}

		if (dig_effort <= 50)
			return(1);

		if ((ttmp = t_at(dpx,dpy)) &&
		    ((ttmp->ttyp == PIT) || (ttmp->ttyp == SPIKED_PIT) ||
		     (ttmp->ttyp == TRAPDOOR)))
			return(1);

		if (IS_ALTAR(lev->typ)) {
			altar_wrath(dpx, dpy);
			angry_priest();
		}

		if (dighole(TRUE)) {	/* make pit at <u.ux,u.uy> */
		    dig_level.dnum = 0;
		    dig_level.dlevel = -1;
		}
		return(0);
	}
	if(dig_effort > 100) {
		register const char *digtxt, *dmgtxt = (const char*) 0;
		register struct obj *obj;
		register boolean shopedge = *in_rooms(dpx, dpy, SHOPBASE);

		if ((obj = sobj_at(STATUE, dpx, dpy)) != 0) {
			if (break_statue(obj))
/*JP				digtxt = "The statue shatters.";*/
				digtxt = "彫像はこなごなになった．";
			else
				/* it was a statue trap; break_statue()
				 * printed a message and updated the screen
				 */
				digtxt = NULL;
		} else if ((obj = sobj_at(BOULDER, dpx, dpy)) != 0) {
			fracture_rock(obj);
/*JP			digtxt = "The boulder falls apart.";*/
			digtxt = "岩はこなごなになった．";
		} else if(!lev->typ || lev->typ == SCORR) {
			if(Is_earthlevel(&u.uz)) {
			    if(uwep->blessed && !rn2(3)) {
				mkcavearea(FALSE);
				goto cleanup;
			    } else if((uwep->cursed && !rn2(4)) ||
					  (!uwep->blessed && !rn2(6))) {
				mkcavearea(TRUE);
				goto cleanup;
			    }
			}
			lev->typ = CORR;
/*JP			digtxt = "You succeed in cutting away some rock.";*/
			digtxt = "岩を少し切りとった．";
		} else if(IS_WALL(lev->typ)) {
			if(shopedge) {
			    add_damage(dpx, dpy, 10L * ACURRSTR);
/*JP			    dmgtxt = "damage";*/
			    dmgtxt = "傷つける";
			}
			if (level.flags.is_maze_lev) {
			    lev->typ = ROOM;
			} else if (level.flags.is_cavernous_lev) {
			    lev->typ = CORR;
			} else {
			    lev->typ = DOOR;
			    lev->doormask = D_NODOOR;
			}
/*JP			digtxt = "You make an opening in the wall.";*/
			digtxt = "壁に穴を空けた．";
		} else if(lev->typ == SDOOR) {
			lev->typ = DOOR;
/*JP			digtxt = "You break through a secret door!";*/
			digtxt = "秘密の扉を通り抜けた！";
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else if(closed_door(dpx, dpy)) {
/*JP			digtxt = "You break through the door.";*/
			digtxt = "扉を通り抜けた．";
			if(shopedge) {
			    add_damage(dpx, dpy, 400L);
/*JP			    dmgtxt = "break";*/
			    dmgtxt = "壊す";
			}
			if(!(lev->doormask & D_TRAPPED))
				lev->doormask = D_BROKEN;
		} else return(0); /* statue or boulder got taken */

		unblock_point(dpx,dpy);	/* vision:  can see through */
		if(Blind)
		    feel_location(dpx, dpy);
		else
		    newsym(dpx, dpy);
		if(digtxt) pline(digtxt);	/* after newsym */
		if(dmgtxt)
		    pay_for_damage(dmgtxt);

		if(Is_earthlevel(&u.uz) && !rn2(3)) {
		    register struct monst *mtmp;

		    switch(rn2(2)) {
		      case 0:
			mtmp = makemon(&mons[PM_EARTH_ELEMENTAL], dpx, dpy);
			break;
		      default:
			mtmp = makemon(&mons[PM_XORN], dpx, dpy);
			break;
		    }
/*JP		    if(mtmp) pline("The debris from your digging comes to life!");*/
		    if(mtmp) pline("岩の破片が生命を帯びた！");
		}
		if(IS_DOOR(lev->typ) && (lev->doormask & D_TRAPPED)) {
			lev->doormask = D_NODOOR;
/*JP			b_trapped("door", 0);*/
			b_trapped("扉", 0);
			newsym(dpx, dpy);
		}
cleanup:
		dig_level.dnum = 0;
		dig_level.dlevel = -1;
		return(0);
	} else {
		if(IS_WALL(lev->typ) || closed_door(dpx, dpy)) {
		    if(*in_rooms(dpx, dpy, SHOPBASE)) {
/*JP			pline("This %s seems too hard to dig into.",
			      IS_DOOR(lev->typ) ? "door" : "wall");*/
			pline("この%sはとても固くて掘れない．",
			      IS_DOOR(lev->typ) ? "扉" : "壁");
			return(0);
		    }
		} else if (!IS_ROCK(lev->typ) && !sobj_at(STATUE, dpx, dpy)
				&& !sobj_at(BOULDER, dpx, dpy))
			return(0); /* statue or boulder got taken */
		if(!did_dig_msg) {
/*JP		    You("hit the %s with all your might.",
			sobj_at(STATUE, dpx, dpy) ? "statue" :
			sobj_at(BOULDER, dpx, dpy) ? "boulder" :
			IS_DOOR(lev->typ) ? "door" : "rock");*/
		    You("%sを力一杯打ちつけた．",
			sobj_at(STATUE, dpx, dpy) ? "彫像" :
			sobj_at(BOULDER, dpx, dpy) ? "岩" :
			IS_DOOR(lev->typ) ? "扉" : "石");
		    did_dig_msg = TRUE;
		}
	}
	return(1);
}

/* When will hole be finished? Very rough indication used by shopkeeper. */
int
holetime() {
	if(occupation != dig || !*u.ushops) return(-1);
	return((250 - dig_effort)/20);
}

/* Return typ of liquid to fill a hole with, or ROOM, if no liquid nearby */
STATIC_OVL
schar
fillholetyp(x,y)
int x, y;
{
    register int x1, y1;
    int lo_x = max(1,x-1), hi_x = min(x+1,COLNO-1),
	lo_y = max(0,y-1), hi_y = min(y+1,ROWNO-1);
    int pool_cnt = 0, moat_cnt = 0, lava_cnt = 0;

    for (x1 = lo_x; x1 <= hi_x; x1++)
	for (y1 = lo_y; y1 <= hi_y; y1++)
	    if (levl[x1][y1].typ == POOL)
		pool_cnt++;
	    else if (levl[x1][y1].typ == MOAT ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_MOAT))
		moat_cnt++;
	    else if (levl[x1][y1].typ == LAVAPOOL ||
		    (levl[x1][y1].typ == DRAWBRIDGE_UP &&
			(levl[x1][y1].drawbridgemask & DB_UNDER) == DB_LAVA))
		lava_cnt++;
    pool_cnt /= 3;		/* not as much liquid as the others */

    if (lava_cnt > moat_cnt + pool_cnt && rn2(lava_cnt + 1))
	return LAVAPOOL;
    else if (moat_cnt > 0 && rn2(moat_cnt + 1))
	return MOAT;
    else if (pool_cnt > 0 && rn2(pool_cnt + 1))
	return POOL;
    else
	return ROOM;
}

static void
digactualhole(ttyp)
int ttyp;
{
	struct obj *oldobjs, *newobjs;
	register struct trap *ttmp;
	boolean wont_fall = !!Levitation;
#ifdef POLYSELF
		wont_fall |= !!is_flyer(uasmon);
#endif

	oldobjs = level.objects[u.ux][u.uy];
	ttmp = maketrap(u.ux, u.uy, ttyp);
	if (!ttmp) return;
	newobjs = level.objects[u.ux][u.uy];
	ttmp->tseen = 1;
	if (Invisible) newsym(ttmp->tx,ttmp->ty);

	if (ttyp == PIT) {
		if (!wont_fall) {
			u.utrap = rn1(4,2);
			u.utraptype = TT_PIT;
			vision_full_recalc = 1;	/* vision limits change */
		} else
			u.utrap = 0;
		if (oldobjs != newobjs)	/* something unearthed */
			pickup(1);	/* detects pit */
		else
/*JP			You("dig a pit in the %s.", surface(u.ux, u.uy));*/
			You("%sに落し穴を掘った．", surface(u.ux, u.uy));

	} else {	/* TRAPDOOR */
/*JP		pline("You dig a hole through the %s.", surface(u.ux,u.uy));*/
		You("%sに穴を空けた．", surface(u.ux,u.uy));

		if (*u.ushops)
			add_damage(u.ux, u.uy, 0L);

		/* floor objects get a chance of falling down.
		 * the case where the hero does NOT fall down
		 * is treated here.  the case where the hero
		 * does fall down is treated in goto_level().
		 */
		if (u.ustuck || wont_fall) {
			if (newobjs)
				impact_drop((struct obj *)0, u.ux, u.uy, 0);
			if (oldobjs != newobjs)
				pickup(1);
		} else {
			if (*u.ushops)
				shopdig(1);
#ifdef WALKIES
			if (!next_to_u()) {
/*JP			    You("are jerked back by your pet!");*/
			    You("ペットによって引き戻された！");
			    if (newobjs)
				impact_drop((struct obj *)0, u.ux, u.uy, 0);
			    if (oldobjs != newobjs)
				pickup(1);
			} else
#endif
			{
			    d_level newlevel;

/*JP			    You("fall through...");*/
			    You("落ちた．．．");

			    /* earlier checks must ensure that the
			     * destination level exists and is in the
			     * present dungeon.
			     */

			    newlevel.dnum = u.uz.dnum;
			    newlevel.dlevel = u.uz.dlevel + 1;
			    goto_level(&newlevel, FALSE, TRUE, FALSE);
			}
		}
	}
}

/* return TRUE if digging succeeded, FALSE otherwise */
boolean
dighole(pit_only)
boolean pit_only;
{
	register struct trap *ttmp = t_at(u.ux, u.uy);
	struct rm *lev = &levl[u.ux][u.uy];
	struct obj *boulder_here;
	schar typ;
	boolean nohole = !Can_dig_down(&u.uz);

	if (ttmp && (ttmp->ttyp == MAGIC_PORTAL || nohole)) {
/*JP		pline("The %s here is too hard to dig in.", surface(u.ux,u.uy));*/
		pline("%sはとても固くて掘れない．", surface(u.ux,u.uy));

	} else if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) {
/*JP		pline("The %s sloshes furiously for a moment, then subsides.",
			is_lava(u.ux, u.uy) ? "lava" : "water");*/
		pline("%sは一瞬はげしく波うった．",
			is_lava(u.ux, u.uy) ? "溶岩" : "水");
		wake_nearby();	/* splashing */

	} else if (lev->typ == DRAWBRIDGE_DOWN) {
		if (pit_only) {
/*JP		    pline("The drawbridge seems too hard to dig through.");*/
		    pline("跳ね橋はとても固くて掘れないようだ．");
		    return FALSE;
		} else {
		    destroy_drawbridge(u.ux, u.uy);
		    return TRUE;
		}

	} else if ((boulder_here = sobj_at(BOULDER, u.ux, u.uy)) != 0) {
		if (ttmp && (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT)) {
/*JP			pline("The boulder settles into the pit.");*/
			pline("岩は落し穴を埋めた．");
			ttmp->ttyp = PIT;	 /* crush spikes */
		} else {
			/*
			 * digging makes a hole, but the boulder immediately
			 * fills it.  Final outcome:  no hole, no boulder.
			 */
/*JP			pline("KADOOM! The boulder falls in!");*/
			pline("どどーん！岩は落ちた！");

			/* destroy traps that emanate from the floor */
			/* some of these are arbitrary -dlc */
			if (ttmp && ((ttmp->ttyp == SQKY_BOARD) ||
				     (ttmp->ttyp == BEAR_TRAP) ||
				     (ttmp->ttyp == LANDMINE) ||
				     (ttmp->ttyp == FIRE_TRAP) ||
				     (ttmp->ttyp == TRAPDOOR) ||
				     (ttmp->ttyp == TELEP_TRAP) ||
				     (ttmp->ttyp == LEVEL_TELEP) ||
				     (ttmp->ttyp == WEB) ||
				     (ttmp->ttyp == MAGIC_TRAP) ||
				     (ttmp->ttyp == ANTI_MAGIC))) {
				deltrap(ttmp);
				u.utrap = 0;
				u.utraptype = 0;
			}
		}
		delobj(boulder_here);
		return TRUE;

	} else if (lev->typ == DRAWBRIDGE_UP) {
		/* must be floor or ice, other cases handled above */
		/* dig "pit" and let fluid flow in (if possible) */
		typ = fillholetyp(u.ux,u.uy);

		if (typ == ROOM) {
			/*
			 * We can't dig a hole here since that will destroy
			 * the drawbridge.  The following is a cop-out. --dlc
			 */
/*JP			pline("The %s here is too hard to dig in.",*/
			pline("%sはとても固くて掘れない．",
			      surface(u.ux, u.uy));
			return FALSE;
		}

		lev->drawbridgemask &= ~DB_UNDER;
		lev->drawbridgemask |= (typ == LAVAPOOL) ? DB_LAVA : DB_MOAT;

 liquid_flow:
		newsym(u.ux,u.uy);

/*JP		pline("As you dig a pit, it fills with %s!",
		      typ == LAVAPOOL ? "lava" : "water");*/
		pline("落し穴を掘ると，%sが湧いてきた！",
		      typ == LAVAPOOL ? "溶岩" : "水");
		if (!Levitation
#ifdef POLYSELF
		   && !is_flyer(uasmon)
#endif
					) {
		    if (typ == LAVAPOOL)
			(void) lava_effects();
		    else if (!Wwalking)
			(void) drown();
		}
		return TRUE;

	} else if (IS_FOUNTAIN(lev->typ)) {
		dogushforth(FALSE);
		dryup(u.ux,u.uy);
		return TRUE;
#ifdef SINKS
	} else if (IS_SINK(lev->typ)) {
		breaksink(u.ux, u.uy);
		return TRUE;
#endif
	/* the following two are here for the wand of digging */
	} else if (IS_THRONE(lev->typ)) {
/*JP		pline("The throne is too hard to break apart.");*/
		pline("玉座はとても固くて砕けない．");

	} else if (IS_ALTAR(lev->typ)) {
/*JP		pline("The altar is too hard to break apart.");*/
		pline("祭壇はとても固くて砕けない．");

	} else {
		typ = fillholetyp(u.ux,u.uy);

		if (typ != ROOM) {
			lev->typ = typ;
			goto liquid_flow;
		}

		/* finally we get to make a hole */
		if (nohole || pit_only)
			digactualhole(PIT);
		else
			digactualhole(TRAPDOOR);

		return TRUE;
	}

	return FALSE;
}

static boolean
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
			pline("you cannot wield that %s.", xname(obj));*/
			pline("武器を%sに装備しているので，",
				body_part(HAND));
			You("%sを装備できない．", xname(obj));
		}
		return(FALSE);
	}
# ifdef POLYSELF
	if(cantwield(uasmon)) {
/*JP		You("can't hold it strongly enough.");*/
		You("それを持つほど力がない．");
		return(FALSE);
	}
# endif
	unweapon = TRUE;
/*JP	You("now wield %s.", doname(obj));*/
	You("%sを装備した．", doname(obj));
	setuwep(obj);
	if (uwep != obj) return(FALSE); /* rewielded old object after dying */
	return(TRUE);
}

static int
use_pick_axe(obj)
struct obj *obj;
{
	char dirsyms[12];
	char qbuf[QBUFSZ];
	register char *dsp = dirsyms;
	register const char *sdp = flags.num_pad ? ndir : sdir;
	register struct rm *lev;
	register int rx, ry, res = 0;
	register boolean isclosedoor;

	if(obj != uwep)
	    if (!wield_tool(obj)) return(0);
	    else res = 1;

	while(*sdp) {
		(void) movecmd(*sdp);	/* sets u.dx and u.dy and u.dz */
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		if(u.dz > 0 || (u.dz == 0 && isok(rx, ry) &&
		    (IS_ROCK(levl[rx][ry].typ)
		    || sobj_at(STATUE, rx, ry)
		    || sobj_at(BOULDER, rx, ry))))
			*dsp++ = *sdp;
		sdp++;
	}
	*dsp = 0;
/*JP	Sprintf(qbuf, "In what direction do you want to dig? [%s]", dirsyms);*/
	Sprintf(qbuf, "どの方向を掘りますか？[%s]", dirsyms);
	if(!getdir(qbuf))
		return(res);
	if(u.uswallow && attack(u.ustuck)) /* return(1) */;
	else if(Underwater) {
/*JP		pline("Turbulence torpedoes your digging attempts.");*/
		pline("そりゃ，嵐のなかの魚雷のようだ．");
	} else if(u.dz < 0) {
		if(Levitation)
/*JP			You("don't have enough leverage.");*/
			You("浮いているのでふんばりがきかない．");
		else
/*JP			You("can't reach the ceiling.");*/
			You("天井に届かない．");
	} else if(!u.dx && !u.dy && !u.dz) {
		char buf[BUFSZ];
		int dam;

		dam = rnd(2) + dbon() + obj->spe;
		if (dam <= 0) dam = 1;
/*JP		You("hit yourself with your pick-axe.");*/
		You("自分をつるはしで叩いた．");
		/* self_pronoun() won't work twice in a sentence */
/*JP		Strcpy(buf, self_pronoun("killed %sself with %%s pick-axe",
			"him"));*/
		Strcpy(buf, "自分をつるはしで叩いて");
/*JP		losehpn(dam, self_pronoun(buf, "his"), NO_KILLER_PREFIX);*/
		losehp(dam, buf, KILLED_BY);
		flags.botl=1;
		return(1);
	} else if(u.dz == 0) {
		if(Stunned || (Confusion && !rn2(5))) confdir();
		rx = u.ux + u.dx;
		ry = u.uy + u.dy;
		if(!isok(rx, ry)) {
/*JP			pline("Clash!");*/
			pline("ガラガラ！");
			return(1);
		}
		lev = &levl[rx][ry];
		if(MON_AT(rx, ry) && attack(m_at(rx, ry)))
			return(1);
		isclosedoor = closed_door(rx, ry);
		if(!IS_ROCK(lev->typ)
		     && !isclosedoor
		     && !sobj_at(STATUE, rx, ry)
		     && !sobj_at(BOULDER, rx, ry)) {
			/* ACCESSIBLE or POOL */
/*JP			You("swing your %s through thin air.",
				aobjnam(obj, NULL));*/
			You("%sを何もない空間で振りまわした．",
				xname(obj));
		} else {
			if(dig_pos.x != rx || dig_pos.y != ry
			    || !on_level(&dig_level, &u.uz) || dig_down) {
				dig_down = FALSE;
				dig_pos.x = rx;
				dig_pos.y = ry;
				assign_level(&dig_level, &u.uz);
				dig_effort = 0;
/*JP				You("start %s.",
				   sobj_at(STATUE, rx, ry) ?
						"chipping the statue" :
				   sobj_at(BOULDER, rx, ry) ?
						"hitting the boulder" :
				   isclosedoor ? "chopping at the door" :
						"digging");*/
				You("%sはじめた．",
				   sobj_at(STATUE, rx, ry) ?
						"彫像を削り" :
				   sobj_at(BOULDER, rx, ry) ?
						"岩を打ちつけ" :
				   isclosedoor ? "扉を削り" :
						"掘り");
			} else
/*JP				You("continue %s.",
				   sobj_at(STATUE, rx, ry) ?
						"chipping the statue" :
				   sobj_at(BOULDER, rx, ry) ?
						"hitting the boulder" :
				   isclosedoor ? "chopping at the door" :
						"digging");*/
				You("%s再開した．",
				   sobj_at(STATUE, rx, ry) ?
						"彫像を削るのを" :
				   sobj_at(BOULDER, rx, ry) ?
						"岩を打ちつけるのを" :
				   isclosedoor ? "扉を削るのを" :
						"掘るのを");
			did_dig_msg = FALSE;
/*JP			set_occupation(dig, "digging", 0);*/
			set_occupation(dig, "掘る", 0);
		}
	} else if (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) {
		/* it must be air -- water checked above */
/*JP		You("swing your %s through thin air.", aobjnam(obj, NULL));*/
		You("%sを何もない空間で振りまわした",xname(obj));
	} else if(Levitation) {
/*JP		You("can't reach the %s.", surface(u.ux,u.uy));*/
		You("%sに届かない．", surface(u.ux,u.uy));
	} else if (is_pool(u.ux, u.uy)) {
		/* Monsters which swim also happen not to be able to dig */
/*JP		You("cannot stay underwater long enough.");*/
		You("水面下には充分長くいられない．");
	} else {
		if(dig_pos.x != u.ux || dig_pos.y != u.uy
		    || !on_level(&dig_level, &u.uz) || !dig_down) {
			dig_down = TRUE;
			dig_pos.x = u.ux;
			dig_pos.y = u.uy;
			assign_level(&dig_level, &u.uz);
			dig_effort = 0;
/*JP			You("start digging downward.");*/
			You("下に向って掘りはじめた．");
			if(*u.ushops)
				shopdig(0);
		} else
/*JP			You("continue digging downward.");*/
			You("下に向って掘るのを再開した．");
		did_dig_msg = FALSE;
/*JP		set_occupation(dig, "digging", 0);*/
		set_occupation(dig, "掘る", 0);
	}
	return(1);
}

#define WEAK	3	/* from eat.c */

/*JPstatic char look_str[] = "look %s.";*/
static char look_str[] = "%s見える．";

static int
use_mirror(obj)
struct obj *obj;
{
	register struct monst *mtmp;
	register char mlet;
	boolean vis;

	if(!getdir(NULL)) return 0;
	if(obj->cursed && !rn2(2)) {
		if (!Blind)
/*JP			pline("The mirror fogs up and doesn't reflect!");*/
			pline("鏡は曇り，映らなくなった！");
		return 1;
	}
	if(!u.dx && !u.dy && !u.dz) {
		if(!Blind && !Invisible) {
#ifdef POLYSELF
		    if(u.umonnum == PM_FLOATING_EYE) {
			pline(Hallucination ?
/*JP			      "Yow!  The mirror stares back!" :
			      "Yikes!  You've frozen yourself!");*/
			      "おゎ！鏡があなたを眈み返した！" :
			      "おゎ！あなたは動けなくなった！");
			nomul(-rnd((MAXULEV+6) - (int)u.ulevel));
		    } else if (u.usym == S_VAMPIRE)
/*JP			You("don't have a reflection.");*/
			pline("鏡には何も映ってない．");
		    else if(u.umonnum == PM_UMBER_HULK) {
/*JP			pline("Huh?  That doesn't look like you!");*/
			pline("ほえ？映っているのはあなたじゃないみたいだ！");
			make_confused(HConfusion + d(3,4),FALSE);
		    } else
#endif
			   if (Hallucination) You(look_str, hcolor());
		    else if (Sick)
/*JP			You(look_str, "peaked");*/
			You(look_str, "顔色が悪く");
		    else if (u.uhs >= WEAK)
/*JP			You(look_str, "undernourished");*/
			You(look_str, "栄養失調のように");
/*JP		    else You("look as %s as ever.",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "beautiful" : "handsome") :
				"ugly");*/
		    else You("あいかわらず%s見える．",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "美しく" : "りりしく") :
				"醜く");
		} else {
/*JP			You("can't see your %s %s.",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "beautiful" : "handsome") :
				"ugly",
				body_part(FACE));*/
			You("自分の%s%sを見ることができない．",
				ACURR(A_CHA) > 14 ?
				(poly_gender()==1 ? "美しい" : "りりしい") :
				"醜い",
				body_part(FACE));
		}
		return 1;
	}
	if(u.uswallow) {
/*JP		if (!Blind) You("reflect %s's %s.", mon_nam(u.ustuck),
		    is_animal(u.ustuck->data)? "stomach" : "interior");*/
		if (!Blind) You("%sの%sを映した．", mon_nam(u.ustuck),
		    is_animal(u.ustuck->data)? "胃" : "内部");
		return 1;
	}
	if(Underwater) {
		You(Hallucination ?
/*JP		    "give the fish a chance to fix their makeup." :
		    "reflect the murky water.");*/
		    "魚に化粧を直す機会を与えた．":
		    "陰気な水を映した．");
		return 1;
	}
	if(u.dz) {
		if (!Blind)
/*JP		    You("reflect the %s.",
			(u.dz > 0) ? surface(u.ux,u.uy) : "ceiling");*/
		    You("%sを映した．",
			(u.dz > 0) ? surface(u.ux,u.uy) : "天井");
		return 1;
	}
	if(!(mtmp = bhit(u.dx,u.dy,COLNO,INVIS_BEAM,
					(int(*)())0,(int(*)())0,obj)) ||
	   !haseyes(mtmp->data))
		return 1;

	vis = canseemon(mtmp);
	mlet = mtmp->data->mlet;
	if(mtmp->msleep) {
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
/*JP			pline("%s is frozen by its reflection.", Monnam(mtmp));
		else You("hear something stop moving.");*/
			pline("%sは自分の姿を見て動けなくなった．", Monnam(mtmp));
		else You("何かが動けなくなったのを聞いた．");
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
	} else if (mlet != S_UNICORN && !humanoid(mtmp->data) &&
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
/*JP		    pline("%s ignores %s reflection.",*/
		    pline("%sは%sの姿を無視した．",
			  Monnam(mtmp), his[pronoun_gender(mtmp)]);
	}
	return 1;
}

static void
use_bell(obj)
register struct obj *obj;
{
/*JP	You("ring %s.", the(xname(obj)));*/
	You("%sを鳴らした．", the(xname(obj)));

	if(Underwater) {
#ifdef	AMIGA
	    amii_speaker( obj, "AhDhGqEqDhEhAqDqFhGw", AMII_MUFFLED_VOLUME );
#endif
/*JP	    pline("But the sound is muffled.");*/
	    pline("しかし音はおおい消された．");
	    return;
	}
	if(obj->otyp == BELL) {
	    if(u.uswallow) {
		pline(nothing_happens);
		return;
	    }
#ifdef	AMIGA
	    amii_speaker( obj, "ahdhgqeqdhehaqdqfhgw", AMII_MUFFLED_VOLUME );
#endif
	    if(obj->cursed && !rn2(3)) {
		register struct monst *mtmp;

		if ((mtmp = makemon(&mons[PM_WOOD_NYMPH], u.ux, u.uy)) != 0)
/*JP		   You("summon %s!", a_monnam(mtmp));*/
		   You("%sを召喚した！", a_monnam(mtmp));
	    }
	    wake_nearby();
	    return;
	}

	/* bell of opening */
	if(u.uswallow && !obj->blessed) {
	    pline(nothing_happens);
	    return;
	}
	if(obj->cursed) {
	    coord mm;
	    mm.x = u.ux;
	    mm.y = u.uy;
	    mkundead(&mm);
cursed_bell:
	    wake_nearby();
	    if(obj->spe > 0) obj->spe--;
	    return;
	}
	if(invocation_pos(u.ux, u.uy) &&
		     !On_stairs(u.ux, u.uy) && !u.uswallow) {
/*JP	    pline("%s issues an unsettling shrill sound...", The(xname(obj)));*/
	    pline("%sは不気味な鋭い音を出した．．．", The(xname(obj)));
#ifdef	AMIGA
	    amii_speaker( obj, "aefeaefeaefeaefeaefe", AMII_LOUDER_VOLUME );
#endif
	    obj->age = moves;
	    if(obj->spe > 0) obj->spe--;
	    wake_nearby();
	    obj->known = 1;
	    return;
	}
	if(obj->blessed) {
	    if(obj->spe > 0) {
		register int cnt = openit();
		if(cnt == -1) return; /* was swallowed */
#ifdef	AMIGA
		amii_speaker( obj, "ahahahDhEhCw", AMII_SOFT_VOLUME );
#endif
		switch(cnt) {
		  case 0:  pline(nothing_happens); break;
/*JP		  case 1:  pline("Something opens..."); break;
		  default: pline("Things open around you..."); break;*/
		  case 1:  pline("何かが開いた．．．"); break;
		  default: pline("あなたのまわりで何かが開いた．．．"); break;
		}
		if(cnt > 0) obj->known = 1;
		obj->spe--;
	    } else pline(nothing_happens);
	} else {  /* uncursed */
#ifdef	AMIGA
	    amii_speaker( obj, "AeFeaeFeAefegw", AMII_OKAY_VOLUME );
#endif
	    if(obj->spe > 0) {
		register int cnt = findit();
		if(cnt == 0) pline(nothing_happens);
		else obj->known = 1;
		obj->spe--;
	    } else {
		if(!rn2(3)) goto cursed_bell;
		else pline(nothing_happens);
	    }
	}
}

static void
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
		obj->lamplit = 0;
		check_lamps();
		return;
	}
	if(obj->spe <= 0) {
/*JP		pline("This %s has no candles.", xname(obj));*/
		pline("この%sにはろうそくがない．", xname(obj));
		return;
	}
	if(u.uswallow || obj->cursed) {
/*JP		pline("The candle%s flicker%s for a moment, then die%s."
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
/*JP		pline("The candle%s being rapidly consumed!",
			(obj->spe > 1 ? "s are" : " is"));*/
		pline("ろうそくはあっというまに燃えつきた！");
		obj->age /= 2;
	} else {
		if(obj->spe == 7) {
		    if (Blind)
/*JP		      pline("%s radiates a strange warmth!", The(xname(obj)));*/
		      pline("%sは奇妙な暖かさを放射している！", The(xname(obj)));
		    else
/*JP		      pline("%s glows with a strange light!", The(xname(obj)));*/
		      pline("%sは奇妙な光を発している！", The(xname(obj)));
		}
		obj->known = 1;
	}
	obj->lamplit = 1;
	check_lamps();
}

static void
use_candle(obj)
register struct obj *obj;
{

	register struct obj *otmp;
	char qbuf[QBUFSZ];

	if(obj->lamplit) {
		use_lamp(obj);
		return;
	}

	if(u.uswallow) {
/*JP		You("don't have enough elbow-room to maneuver.");*/
		You("それをするだけの広さがない．");
		return;
	}
	if(Underwater) {
/*JP		pline("Sorry, fire and water don't mix.");*/
		pline("残念ながら，火と水はまざらない．");
		return;
	}

	for(otmp = invent; otmp; otmp = otmp->nobj) {
		if(otmp->otyp == CANDELABRUM_OF_INVOCATION && otmp->spe < 7)
			break;
	}
	if(!otmp || otmp->spe == 7) {
		use_lamp(obj);
		return;
	}

/*JP	Sprintf(qbuf, "Attach %s", the(xname(obj)));
	Sprintf(eos(qbuf), " to %s?", the(xname(otmp)));*/
	Sprintf(qbuf, "%sを", the(xname(obj)));
	Sprintf(eos(qbuf), "%sに取りつけますか？", the(xname(otmp)));
	if(yn(qbuf) == 'n') {
/*JP		You("try to light %s...", the(xname(obj)));*/
		You("%sに火をつけようとした．．．", the(xname(obj)));
		use_lamp(obj);
		return;
	} else {
		register long needed = 7L - (long)otmp->spe;

/*JP		You("attach %ld%s candle%s to %s.",
			obj->quan >= needed ? needed : obj->quan,
			!otmp->spe ? "" : " more",
			(needed > 1L && obj->quan > 1L) ? "s" : "",
			the(xname(otmp)));*/
		You("%ld本のろうそくを%s%sへ取りつけた．",
			obj->quan >= needed ? needed : obj->quan,
			!otmp->spe ? "" : "さらに",
			the(xname(otmp)));
/*JP		if(otmp->lamplit)
			pline("The new candle%s magically ignite%s!",
			    (needed > 1L && obj->quan > 1L) ? "s" : "",
			    (needed > 1L && obj->quan > 1L) ? "" : "s");*/
		if(otmp->lamplit)
			pline("新しいろうそくは不思議な炎をあげた！");
		if(obj->unpaid)
/*JP			verbalize("You burn %s, you bought %s!",
                            (needed > 1L && obj->quan > 1L) ? "them" : "it",
                            (needed > 1L && obj->quan > 1L) ? "them" : "it");*/
			verbalize("火をつけたな，買ってもらおう！");

		if(!otmp->spe || otmp->age > obj->age)
			otmp->age = obj->age;
		if(obj->quan > needed) {
		    if(obj->unpaid) {
			/* this is a hack, until we re-write the billing */
			/* code to accommodate such cases directly. IM*/
			register long delta = obj->quan - needed;

			subfrombill(obj, shop_keeper(*u.ushops));
			obj->quan = needed;
			addtobill(obj, TRUE, FALSE, TRUE);
			bill_dummy_object(obj);
			obj->quan = delta;
			addtobill(obj, TRUE, FALSE, TRUE);
		     } else {
			obj->quan -= needed;
		     }
		     otmp->spe += (int)needed;
		} else {
		    otmp->spe += (int)obj->quan;
		    freeinv(obj);
		    obfree(obj, (struct obj *)0);
		}
		if(needed < 7L && otmp->spe == 7)
/*JP		    pline("%s has now seven%s candles attached.",
			The(xname(otmp)), otmp->lamplit ? " lit" : "");*/
		    pline("%sにはもう7本の%sろうそくが取りつけられている．",
			The(xname(otmp)), otmp->lamplit ? "燃えている" : "");
	}
}

boolean
snuff_candle(otmp)  /* call in drop, throw, and put in box, etc. */
register struct obj *otmp;
{
	register boolean candle = Is_candle(otmp);

	if ((candle || otmp->otyp == CANDELABRUM_OF_INVOCATION) &&
		otmp->lamplit) {
/*JP	    register boolean many = candle ? otmp->quan > 1L : otmp->spe > 1;*/
	    if (!Blind)
/*JP		pline("The %scandle%s flame%s extinguished.",
		      (candle ? "" : "candelabrum's "),
		      (many ? "s'" : "'s"), (many ? "s are" : " is"));*/
		pline("%sろうそくの炎は消えた．",
		      (candle ? "" : "燭台の"));
	   otmp->lamplit = 0;
	   check_lamps();
	   return(TRUE);
	}
	return(FALSE);
}

boolean
snuff_lit(obj)
struct obj *obj;
{
	if(obj->lamplit) {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN) {
/*JP			if (!Blind) Your("lamp is now off.");*/
			if (!Blind) Your("ランプは消えた．");
			obj->lamplit = 0;
			check_lamps();
			return(TRUE);
		}

		if(snuff_candle(obj)) return(TRUE);
	}

	return(FALSE);
}

static void
use_lamp(obj)
struct obj *obj;
{
	if(Underwater) {
/*JP		pline("This is not a diving lamp.");*/
		pline("これは潜水用のランプじゃない．");
		return;
	}
	if(obj->lamplit) {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN)
/*JP		    Your("lamp is now off.");*/
		    Your("ランプは消えた．");
		else
/*JP		    You("snuff out %s.", the(xname(obj)));*/
		    You("%sを吹き消した．", the(xname(obj)));
		obj->lamplit = 0;
		check_lamps();
		return;
	}
	if (!Is_candle(obj) && obj->spe <= 0) {
		if (obj->otyp == BRASS_LANTERN)
/*JP			Your("lamp has run out of power.");*/
			You("ランプを使い切ってしまった！");
/*JP		else pline("This %s has no oil.", xname(obj));*/
		else pline("この%sにはもうオイルがない．", xname(obj));
		return;
	}
	if(obj->cursed && !rn2(2))
/*JP		pline("%s flicker%s for a moment, then die%s.",
		       The(xname(obj)),
		       obj->quan > 1L ? "" : "s",
		       obj->quan > 1L ? "" : "s");*/
		pline("しばらく%sの炎は点滅し，消えた．",
		       The(xname(obj)));
	else {
		if(obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP ||
				obj->otyp == BRASS_LANTERN)
/*JP		    Your("lamp is now on.");*/
		    Your("ランプに火が灯った．");
		else
/*JP		    pline("%s%s flame%s burn%s%s", The(xname(obj)),
			obj->quan > 1L ? "'" : "'s",
			obj->quan > 1L ? "s" : "",
			obj->quan > 1L ? "" : "s",
			Blind ? "." : " brightly!");*/
		    pline("%sの炎は%s燃えあがった！", The(xname(obj)),
			Blind ? "" : "明るく");
		obj->lamplit = 1;
		check_lamps();
		if (obj->unpaid && Is_candle(obj) &&
			obj->age == 20L * (long)objects[obj->otyp].oc_cost) {
/*JP		    const char *it_them = obj->quan > 1L ? "them" : "it";*/
/*JP		    verbalize("You burn %s, you bought %s!", it_them, it_them);*/
		    verbalize("火をつけたな，買ってもらおう！");
		    bill_dummy_object(obj);
		}
	}
}

void
check_lamps()
{
	register struct obj *obj;
	int lamps = 0;

	for(obj = invent; obj; obj = obj->nobj)
		if (obj->lamplit) {
			lamps++;
			break;
		}

	if (lamps && u.nv_range == 1) {
		u.nv_range = 3;
		vision_full_recalc = 1;
	} else if (!lamps && u.nv_range == 3) {
		u.nv_range = 1;
		vision_full_recalc = 1;
	}
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
		djinni_from_bottle(uwep);
		makeknown(MAGIC_LAMP);
		uwep->otyp = OIL_LAMP;
		uwep->spe = 1; /* for safety */
		uwep->age = rn1(500,1000);
	    } else if (rn2(2) && !Blind)
/*JP		You("see a puff of smoke.");*/
		pline("けむりが舞いあがった．");
	    else pline(nothing_happens);
	} else if (obj->otyp == BRASS_LANTERN) {
	    /* message from Adventure */
/*JP	    pline("Rubbing the electric lamp is not particularly rewarding.");*/
	    pline("電気のランプをこすっても意味はない．");
/*JP	    pline("Anyway, nothing exciting happens.");*/
	    pline("やっぱり，何も起きない．");
	} else pline(nothing_happens);
	return 1;
}

int
dojump()
{
	coord cc;
	register struct monst *mtmp;
	if (!Jumping || Levitation) {
/*JP		You("can't jump very far.");*/
		You("たいして遠くまで飛べない．");
		return 0;
	} else if (u.uswallow) {
/*JP		pline("You've got to be kidding!");*/
		pline("冗談だろ！");
		return 0;
	} else if (u.uinwater) {
/*JP		pline("This calls for swimming, not jumping!");*/
		pline("それは『泳ぐ』であって，『飛ぶ』じゃない！");
		return 0;
	} else if (u.ustuck) {
/*JP		You("cannot escape from %s!", mon_nam(u.ustuck));*/
		You("%sから逃れられない！", mon_nam(u.ustuck));
		return 0;
	} else if (near_capacity() > UNENCUMBERED) {
/*JP		You("are carrying too much to jump!");*/
		You("たくさん物を持ちすぎて飛べない！");
		return 0;
	} else if (u.uhunger <= 100 || ACURR(A_STR) < 6) {
/*JP		You("lack the strength to jump!");*/
		You("飛べるだけの強さがない！");
		return 0;
	}
/*JP	pline("Where do you want to jump?");*/
	pline("どこに飛びますか？");
	cc.x = u.ux;
	cc.y = u.uy;
/*JP	getpos(&cc, TRUE, "the desired position");*/
	getpos(&cc, TRUE, "好きな位置");
	if(cc.x == -10) return 0; /* user pressed esc */
	if (!(Jumping & ~INTRINSIC) && distu(cc.x, cc.y) != 5) {
/*JP		pline("Illegal move!");*/
		pline("そこには行けない！");
		return 0;
	} else if (distu(cc.x, cc.y) > 9) {
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
#ifdef POLYSELF
		    && !(passes_walls(uasmon) && may_passwall(cc.x, cc.y))
#endif
		    )) {
/*JP			You("cannot jump there!");*/
			You("そこには飛べない！");
			return 0;
	} else {
	    if(u.utrap)
		switch(u.utraptype) {
		case TT_BEARTRAP: {
		    register long side = rn2(3) ? LEFT_SIDE : RIGHT_SIDE;
/*JP		    You("rip yourself free of the bear trap!  Ouch!");
		    losehp(rnd(10), "jumping out of a bear trap", KILLED_BY);*/
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

static void
use_tinning_kit(obj)
register struct obj *obj;
{
	register struct obj *corpse, *can;

	/* This takes only 1 move.  If this is to be changed to take many
	 * moves, we've got to deal with decaying corpses...
	 */
/*JP	if (!(corpse = floorfood("tin", 2))) return;*/
	if (!(corpse = floorfood("缶詰にする", 2))) return;
	if (corpse->oeaten) {
/*JP		You("cannot tin something which is partly eaten.");*/
		You("食べかけのものを缶詰にすることはできない．");
		return;
	}
	if ((corpse->corpsenm == PM_COCKATRICE)
#ifdef POLYSELF
		&& !resists_ston(uasmon)
#endif
		&& !uarmg) {
/*JPpline("Tinning a cockatrice without wearing gloves is a fatal mistake...");*/
pline("コカトリスを小手なしで缶詰にするのは致命的な間違いだ．．．");
#if defined(POLYSELF)
/* this will have to change if more monsters can poly */
		if(!(poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM)))
#endif
	    {
/*JP		You("turn to stone...");*/
		You("石になった．．．");
		killer_format = KILLED_BY;
/*JP		killer = "trying to tin a cockatrice without gloves";*/
		killer = "小手をつけずにコカトリスを缶詰にしようとして";
		done(STONING);
	    }
	}
	if (is_rider(&mons[corpse->corpsenm])) {
		revive_corpse(corpse, 0, FALSE);
/*JP		verbalize("Yes...  But War does not preserve its enemies...");*/
		verbalize("塩漬けにされてたまるか．．．");
		return;
	}
	if (mons[corpse->corpsenm].cnutrit == 0) {
/*JP		pline("That's too insubstantial to tin.");*/
		pline("実体がないので缶詰にできない．");
		return;
	}
	if ((can = mksobj(TIN, FALSE, FALSE)) != 0) {
	    can->corpsenm = corpse->corpsenm;
	    can->cursed = obj->cursed;
	    can->blessed = obj->blessed;
	    can->owt = weight(can);
	    can->known = 1;
	    can->spe = -1;  /* Mark tinned tins. No spinach allowed... */
	    if (carried(corpse)) {
		if(corpse->unpaid) {
/*JP		    verbalize("You tin it, you bought it!");*/
		    verbalize("缶詰にするのなら金を置いてってもらおう！");
		    bill_dummy_object(corpse);
		}
		useup(corpse);
	    } else {
		if(costly_spot(corpse->ox, corpse->oy) &&
		      !corpse->no_charge) {
/*JP		    verbalize("You tin it, you bought it!");*/
		    verbalize("缶詰にするのなら金を置いてってもらおう！");
		    bill_dummy_object(corpse);
		}
		useupf(corpse);
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
	boolean blessed = (obj && obj->blessed);
	boolean did_something = FALSE;

	if (obj && obj->cursed) {
		switch (rn2(6)) {
		    static char buf[BUFSZ];
		    case 0: make_sick(Sick ? Sick/4 + 1L : (long) rn1(ACURR(A_CON), 20), TRUE);
			    Strcpy(buf, xname(obj));
			    u.usick_cause = (const char *)buf;
			    break;
		    case 1: make_blinded(Blinded + (long) rnd(100), TRUE);
			    break;
		    case 2: if (!Confusion)
/*JP				You("suddenly feel %s.",
					Hallucination ? "trippy" : "confused");*/
				You("突然%s．",
					Hallucination ? "へろへろになった" : "混乱した");
			    make_confused(HConfusion + (long) rnd(100), TRUE);
			    break;
		    case 3: make_stunned(HStun + (long) rnd(100), TRUE);
			    break;
		    case 4: (void) adjattrib(rn2(6), -1, FALSE);
			    break;
		    case 5: make_hallucinated(HHallucination + (long) rnd(100),
				TRUE, 0L);
			    break;
		}
		return;
	}

	if (Sick) {
		make_sick(0L, TRUE);
		did_something++;
	}
	if (Blinded > (long)(u.ucreamed+1) && (!did_something || blessed)) {
		make_blinded(u.ucreamed ? (long)(u.ucreamed+1) : 0L, TRUE);
		did_something++;
	}
	if (Hallucination && (!did_something || blessed)) {
		make_hallucinated(0L, TRUE, 0L);
		did_something++;
	}
	if (Vomiting && (!did_something || blessed)) {
		make_vomiting(0L, TRUE);
		did_something++;
	}
	if (HConfusion && (!did_something || blessed)) {
		make_confused(0L, TRUE);
		did_something++;
	}
	if (HStun && (!did_something || blessed)) {
		make_stunned(0L, TRUE);
		did_something++;
	}
	if (!did_something || blessed) {
		register int j;
		int did_stat = 0;
		int i = rn2(A_MAX);
		for(j=0; j<A_MAX; j++) {
			/* don't recover strength lost while hungry */
			if ((blessed || j==i) &&
				((j != A_STR || u.uhs < WEAK)
				? (ABASE(j) < AMAX(j))
				: (ABASE(A_STR) < (AMAX(A_STR) - 1)))) {
				did_something++;
				/* They may have to use it several times... */
				if (!did_stat) {
					did_stat++;
/*JP					pline("This makes you feel good!");*/
					pline("気分がよくなった！");
				}
				ABASE(j)++;
				flags.botl = 1;
			}
		}
	}
	if (!did_something) pline(nothing_happens);
}

static void
use_figurine(obj)
register struct obj *obj;
{
	xchar x, y;

	if(!getdir(NULL)) {
		flags.move = multi = 0;
		return;
	}
	x = u.ux + u.dx; y = u.uy + u.dy;
	if (!isok(x,y)) {
/*JP		You("cannot put the figurine there.");*/
		You("ここに人形を置けない．");
		return;
	}
	if (IS_ROCK(levl[x][y].typ) &&
	    !(passes_walls(&mons[obj->corpsenm]) && may_passwall(x,y))) {
/*JP		You("cannot place a figurine in solid rock!");*/
		You("固い石の上には人形を置けない！");
		return;
	}
	if (sobj_at(BOULDER,x,y) && !passes_walls(&mons[obj->corpsenm])
			&& !throws_rocks(&mons[obj->corpsenm])) {
/*JP		You("cannot fit the figurine on the boulder.");*/
		You("岩に人形を置けない．");
		return;
	}
/*JP	You("%s and it transforms.",
	    (u.dx||u.dy) ? "set the figurine beside you" :
	    (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) ?
		"release the figurine" :
	    (u.dz < 0 ?
		"toss the figurine into the air" :
		"set the figurine on the ground"));*/
	You("%s．するとそれは変形した．",
	    (u.dx||u.dy) ? "そばに人形を置いた" :
	    (Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)) ?
		"人形を放った" :
	    (u.dz < 0 ?
		"人形を空中に投げた" :
		"人形を地面に置いた"));
	make_familiar(obj, u.ux+u.dx, u.uy+u.dy);
	useup(obj);
}

static NEARDATA const char lubricables[] = { ALL_CLASSES, ALLOW_NONE, 0 };

static void
use_grease(obj)
struct obj *obj;
{
	struct obj *otmp;

	if (Glib) {
	    dropx(obj);
/*JP	    pline("%s slips from your %s.", The(xname(obj)),*/
	    pline("%sはあなたの%sから滑り落ちた．", The(xname(obj)),
		  makeplural(body_part(FINGER)));
	    return;
	}

	if (obj->spe > 0) {
		if ((obj->cursed || Fumbling) && !rn2(2)) {
			obj->spe--;
			check_unpaid(obj);
			dropx(obj);
/*JP			pline("%s slips from your %s.", The(xname(obj)),*/
			pline("%sはあなたの%sから滑り落ちた．", The(xname(obj)),
			      makeplural(body_part(FINGER)));
			return;
		}
/*JP		otmp = getobj(lubricables, "grease");*/
/*
**JP		this is not a type miss
*/
		otmp = getobj(lubricables, "n塗る");
		if (!otmp) return;
		obj->spe--;
		check_unpaid(obj);
		if (otmp != &zeroobj) {
/*JP			You("cover your %s with a thick layer of grease.",*/
			You("%sに脂を丹念に塗った．",
			    xname(otmp));
			otmp->greased = 1;
		} else {
			Glib += rnd(15);
/*JP			You("coat your %s with grease.",*/
			You("%sに脂を塗った．",
			    makeplural(body_part(FINGER)));
		}
	} else {
/*JP		pline("%s %s empty.", The(xname(obj)),
			obj->known ? "is" : "seems to be");*/
		pline("%sは空っぽ%s．", The(xname(obj)),
			obj->known ? "だ" : "に見える");
	}
}

int
doapply()
{
	register struct obj *obj;
	register int res = 1;

	if(check_capacity(NULL)) return (0);
/*JP	obj = getobj(tools, "use or apply");*/
	obj = getobj(tools, "使う");
	if(!obj) return 0;

	check_unpaid(obj);

	switch(obj->otyp){
	case BLINDFOLD:
		if (obj == ublindf) {
		    if(cursed(obj)) break;
		    else Blindf_off(obj);
		}
		else if (!ublindf) Blindf_on(obj);
/*JP		else You("are already %s.", ublindf->otyp == TOWEL ?
			 "covered by a towel" : "wearing a blindfold");*/
		else You("もう%s．", ublindf->otyp == TOWEL ?
			 "タオルを巻いている" : "目隠しをつけている");
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

			obj->spe--;
			check_unpaid(obj);
			if(!rn2(23)) cnt += rn2(7) + 1;
			while(cnt--)
			    (void) makemon((struct permonst *) 0, u.ux, u.uy);
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
		res = use_pick_axe(obj);
		break;
	case TINNING_KIT:
		use_tinning_kit(obj);
		break;
#ifdef WALKIES
	case LEASH:
		use_leash(obj);
		break;
#endif
	case MAGIC_WHISTLE:
		use_magic_whistle(obj);
		break;
	case TIN_WHISTLE:
		use_whistle(obj);
		break;
	case STETHOSCOPE:
		res = 0;
		use_stethoscope(obj);
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

		    obj->spe--;
		    check_unpaid(obj);
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
	default:
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
