/*	SCCS Id: @(#)muse.c	3.2	96/06/10	*/
/*	Copyright (C) 1990 by Ken Arromdee			   */
/* NetHack may be freely redistributed.  See license for details.  */

/*
 * Monster item usage routines.
 */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "edog.h"

extern int monstr[];

boolean m_using = FALSE;

/* Let monsters use magic items.  Arbitrary assumptions: Monsters only use
 * scrolls when they can see, monsters know when wands have 0 charges, monsters
 * cannot recognize if items are cursed are not, monsters which are confused
 * don't know not to read scrolls, etc....
 */

static int FDECL(precheck, (struct monst *,struct obj *));
static void FDECL(mzapmsg, (struct monst *,struct obj *,BOOLEAN_P));
static void FDECL(mreadmsg, (struct monst *,struct obj *));
static void FDECL(mquaffmsg, (struct monst *,struct obj *));
STATIC_PTR int FDECL(mbhitm, (struct monst *,struct obj *));
static void FDECL(mbhit,
	(struct monst *,int,int FDECL((*),(MONST_P,OBJ_P)),
	int FDECL((*),(OBJ_P,OBJ_P)),struct obj *));
static void FDECL(you_aggravate, (struct monst *));

static struct musable {
	struct obj *offensive;
	struct obj *defensive;
	struct obj *misc;
	int has_offense, has_defense, has_misc;
	/* =0, no capability; otherwise, different numbers.
	 * If it's an object, the object is also set (it's 0 otherwise).
	 */
} m;
static int trapx, trapy;
static boolean zap_oseen;
	/* for wands which use mbhitm and are zapped at players.  We usually
	 * want an oseen local to the function, but this is impossible since the
	 * function mbhitm has to be compatible with the normal zap routines,
	 * and those routines don't remember who zapped the wand.
	 */

/* Any preliminary checks which may result in the monster being unable to use
 * the item.  Returns 0 if nothing happened, 2 if the monster can't do anything
 * (i.e. it teleported) and 1 if it's dead.
 */
static int
precheck(mon, obj)
struct monst *mon;
struct obj *obj;
{
	boolean vis;

	if (!obj) return 0;
	vis = cansee(mon->mx, mon->my);

	if (obj->oclass == POTION_CLASS) {
	    coord cc;
/*JP	    static const char *empty = "The potion turns out to be empty.";*/
	    static const char *empty = "薬は空っぽであることがわかった．";
	    const char *potion_descr;
	    struct monst *mtmp;
#define POTION_OCCUPANT_CHANCE(n) (13 + 2*(n))	/* also in potion.c */

	    potion_descr = OBJ_DESCR(objects[obj->otyp]);
	    if (potion_descr && !strcmp(potion_descr, "milky") &&
		    flags.ghost_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.ghost_count))) {
		if (!enexto(&cc, mon->mx, mon->my, &mons[PM_GHOST])) return 0;
		mquaffmsg(mon, obj);
		m_useup(mon, obj);
		mtmp = makemon(&mons[PM_GHOST], cc.x, cc.y, NO_MM_FLAGS);
		if (!mtmp) {
		    if (vis) pline(empty);
		} else {
		    if (vis) {
/*JP			pline("As %s opens the bottle, an enormous %s emerges!",*/
			pline("%sが瓶を開けると，巨大な%sが出てきた！",
			   mon_nam(mon),
/*JP			   Hallucination ? rndmonnam() : (const char *)"ghost");*/
			   jtrns_mon(Hallucination ? rndmonnam() : (const char *)"幽霊", -1));
/*JP			pline("%s is frightened to death, and unable to move.",*/
			pline("%sはまっさおになって驚き，動けなくなった．",
				Monnam(mon));
		    }
		    mon->mcanmove = 0;
		    mon->mfrozen = 3;
		}
		return 2;
	    }
	    if (potion_descr && !strcmp(potion_descr, "smoky") &&
		    flags.djinni_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.djinni_count))) {
		if (!enexto(&cc, mon->mx, mon->my, &mons[PM_DJINNI])) return 0;
		mquaffmsg(mon, obj);
		m_useup(mon, obj);
		mtmp = makemon(&mons[PM_DJINNI], cc.x, cc.y, NO_MM_FLAGS);
		if (!mtmp) {
		    if (vis) pline(empty);
		} else {
		    if (vis)
/*JP			pline("In a cloud of smoke, %s emerges!",*/
			pline("煙の中から，%sが現われた！",
							a_monnam(mtmp));
/*JP		    pline("%s speaks.", vis ? Monnam(mtmp) : Something);*/
		    pline("%sは話した．", vis ? Monnam(mtmp) : Something);
		/* I suspect few players will be upset that monsters */
		/* can't wish for wands of death here.... */
		    if (rn2(2)) {
/*JP			verbalize("You freed me!");*/
		        verbalize("解放してくれたのはお前か！");
			mtmp->mpeaceful = 1;
			set_malign(mtmp);
		    } else {
/*JP			verbalize("It is about time.");*/
			verbalize("さらばだ！");
/*JP			if (vis) pline("%s vanishes.", Monnam(mtmp));*/
			if (vis) pline("%sは消えた．", Monnam(mtmp));
			mongone(mtmp);
		    }
		}
		return 2;
	    }
	}
	if (obj->oclass == WAND_CLASS && obj->cursed && !rn2(100)) {
	    int dam = d(obj->spe+2, 6);

	    if (flags.soundok) {
/*JP		if (vis) pline("%s zaps %s, which suddenly explodes!",*/
		if (vis) pline("%sは%sをふりかざした，それは突然爆発した！",
			Monnam(mon), an(xname(obj)));
/*JP		else You_hear("a zap and an explosion in the distance.");*/
		else You("遠方の杖の音と爆発音を聞いた．");
	    }
	    m_useup(mon, obj);
	    if (mon->mhp <= dam) {
		monkilled(mon, "", AD_RBRE);
		return 1;
	    }
	    else mon->mhp -= dam;
	    m.has_defense = m.has_offense = m.has_misc = 0;
	    /* Only one needed to be set to 0 but the others are harmless */
	}
	return 0;
}

static void
mzapmsg(mtmp, otmp, self)
struct monst *mtmp;
struct obj *otmp;
boolean self;
{
	if (!canseemon(mtmp)) {
		if (flags.soundok)
/*JP			You_hear("a distant zap.");*/
			You("遠方での杖の音を聞いた．");
	} else if (self)
/*JP		pline("%s zaps %sself with %s!",
		      Monnam(mtmp), him[pronoun_gender(mtmp)], doname(otmp));*/
		pline("%sは自分自身に%sをふりかざした！",
		      Monnam(mtmp),  doname(otmp));
	else {
/*JP		pline("%s zaps %s!", Monnam(mtmp), an(xname(otmp)));*/
		pline("%sは%sをふりかざした！", Monnam(mtmp), an(xname(otmp)));
		stop_occupation();
	}
}

static void
mreadmsg(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;
{
	boolean vismon = canseemon(mtmp);
	char saverole, onambuf[BUFSZ];
	unsigned savebknown;

	if (!vismon && !flags.soundok)
	    return;		/* no feedback */

	otmp->dknown = 1;  /* seeing or hearing it read reveals its label */
	/* shouldn't be able to hear curse/bless status of unseen scrolls;
	   for priest characters, bknown will always be set during naming */
	savebknown = otmp->bknown;
	saverole = u.role;
	if (!vismon) {
	    otmp->bknown = 0;
	    if (Role_is('P')) u.role = '@';	/* '@' => not 'P' */
	}
	Strcpy(onambuf, singular(otmp, doname));
	otmp->bknown = savebknown;
	u.role = saverole;

	if (vismon)
/*JP	    pline("%s reads %s!", Monnam(mtmp), onambuf);*/
	    pline("%sは%sを読んだ！", Monnam(mtmp), onambuf);
	else
/*JP	    You_hear("%s reading %s.",*/
	    You_hear("%sが%sを読んでいる声を聞いた．",
/*JP		     an(Hallucination ? rndmonnam() : mtmp->data->mname),*/
		     jtrns_mon(Hallucination ? rndmonnam() : mtmp->data->mname, mtmp->female),
		     onambuf);

	if (mtmp->mconf)
/*JP	    pline("Being confused, %s mispronounces the magic words...",*/
	    pline("混乱しているので，%sは呪文を間違って唱えてしまった．．．",
		  vismon ? mon_nam(mtmp) : he[pronoun_gender(mtmp)]);
}

static void
mquaffmsg(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;
{
	if (canseemon(mtmp)) {
		otmp->dknown = 1;
/*JP		pline("%s drinks %s!", Monnam(mtmp), singular(otmp, doname));*/
		pline("%sは%sを飲んだ！", Monnam(mtmp), singular(otmp, doname));
	} else
		if (flags.soundok)
/*JP			You_hear("a chugging sound.");*/
			You_hear("ゴクッ！という音を聞いた．");
}

/* Defines for various types of stuff.  The order in which monsters prefer
 * to use them is determined by the order of the code logic, not the
 * numerical order in which they are defined.
 */
#define MUSE_SCR_TELEPORTATION 1
#define MUSE_WAN_TELEPORTATION_SELF 2
#define MUSE_POT_HEALING 3
#define MUSE_POT_EXTRA_HEALING 4
#define MUSE_WAN_DIGGING 5
#define MUSE_TRAPDOOR 6
#define MUSE_TELEPORT_TRAP 7
#define MUSE_UPSTAIRS 8
#define MUSE_DOWNSTAIRS 9
#define MUSE_WAN_CREATE_MONSTER 10
#define MUSE_SCR_CREATE_MONSTER 11
#define MUSE_UP_LADDER 12
#define MUSE_DN_LADDER 13
#define MUSE_SSTAIRS 14
#define MUSE_WAN_TELEPORTATION 15
#define MUSE_BUGLE 16
#define MUSE_UNICORN_HORN 17
/*
#define MUSE_INNATE_TPT 9999
 * We cannot use this.  Since monsters get unlimited teleportation, if they
 * were allowed to teleport at will you could never catch them.  Instead,
 * assume they only teleport at random times, despite the inconsistency that if
 * you polymorph into one you teleport at will.
 */

/* Select a defensive item/action for a monster.  Returns TRUE iff one is
 * found.
 */
boolean
find_defensive(mtmp)
struct monst *mtmp;
{
	register struct obj *obj = 0;
	struct trap *t;
	int x=mtmp->mx, y=mtmp->my;
	boolean stuck = (mtmp == u.ustuck);
	boolean immobile = (mtmp->data->mmove == 0);
	int fraction;

	if (is_animal(mtmp->data) || mindless(mtmp->data))
		return FALSE;
	if(dist2(x, y, mtmp->mux, mtmp->muy) > 25)
		return FALSE;
	if (u.uswallow && stuck) return FALSE;

	m.defensive = (struct obj *)0;
	m.has_defense = 0;

	/* since unicorn horns don't get used up, the monster would look
	 * silly trying to use the same cursed horn round after round
	 */
	if (mtmp->mconf || mtmp->mstun || !mtmp->mcansee) {
	    if (mtmp->data->mlet != S_UNICORN && !nohands(mtmp->data)) {
		for(obj = mtmp->minvent; obj; obj = obj->nobj)
		    if (obj->otyp == UNICORN_HORN && !obj->cursed)
			break;
	    }
	    if (obj || (mtmp->data->mlet == S_UNICORN)) {
		m.defensive = obj;
		m.has_defense = MUSE_UNICORN_HORN;
		return TRUE;
	    }
	}

	/* It so happens there are two unrelated cases when we might want to
	 * check specifically for healing alone.  The first is when the monster
	 * is blind (healing cures blindness).  The second is when the monster
	 * is peaceful; then we don't want to flee the player, and by
	 * coincidence healing is all there is that doesn't involve fleeing.
	 * These would be hard to combine because of the control flow.
	 */
	if (!mtmp->mcansee && !nohands(mtmp->data)) {
	    if ((obj = m_carrying(mtmp, POT_EXTRA_HEALING)) != 0) {
		m.defensive = obj;
		m.has_defense = MUSE_POT_EXTRA_HEALING;
		return TRUE;
	    }
	    if ((obj = m_carrying(mtmp, POT_HEALING)) != 0) {
		m.defensive = obj;
		m.has_defense = MUSE_POT_HEALING;
		return TRUE;
	    }
	}

	fraction = u.ulevel < 10 ? 5 : u.ulevel < 14 ? 4 : 3;
	if(mtmp->mhp >= mtmp->mhpmax ||
			(mtmp->mhp >= 10 && mtmp->mhp*fraction >= mtmp->mhpmax))
		return FALSE;

	if (mtmp->mpeaceful) {
	    if (!nohands(mtmp->data)) {
		if ((obj = m_carrying(mtmp, POT_EXTRA_HEALING)) != 0) {
		    m.defensive = obj;
		    m.has_defense = MUSE_POT_EXTRA_HEALING;
		    return TRUE;
		}
		if ((obj = m_carrying(mtmp, POT_HEALING)) != 0) {
		    m.defensive = obj;
		    m.has_defense = MUSE_POT_HEALING;
		    return TRUE;
		}
	    }
	    return FALSE;
	}

	if (levl[x][y].typ == STAIRS && !stuck && !immobile) {
		if (x == xdnstair && y == ydnstair && !is_floater(mtmp->data))
			m.has_defense = MUSE_DOWNSTAIRS;
		if (x == xupstair && y == yupstair && ledger_no(&u.uz) != 1)
	/* Unfair to let the monsters leave the dungeon with the Amulet */
	/* (or go to the endlevel since you also need it, to get there) */
			m.has_defense = MUSE_UPSTAIRS;
	} else if (levl[x][y].typ == LADDER && !stuck && !immobile) {
		if (x == xupladder && y == yupladder)
			m.has_defense = MUSE_UP_LADDER;
		if (x == xdnladder && y == ydnladder && !is_floater(mtmp->data))
			m.has_defense = MUSE_DN_LADDER;
	} else if (sstairs.sx && sstairs.sx == x && sstairs.sy == y) {
		m.has_defense = MUSE_SSTAIRS;
	} else if (!stuck && !immobile) {
	/* Note: trapdoors take precedence over teleport traps. */
		int xx, yy;

		for(xx = x-1; xx <= x+1; xx++) for(yy = y-1; yy <= y+1; yy++)
		if (isok(xx,yy))
		if (xx != u.ux && yy != u.uy)
		if (mtmp->data != &mons[PM_GRID_BUG] || xx == x || yy == y)
		if ((xx==x && yy==y) || !level.monsters[xx][yy])
		if ((t = t_at(xx,yy)) != 0)
	  	if ((verysmall(mtmp->data) || throws_rocks(mtmp->data) ||
		     passes_walls(mtmp->data)) || !sobj_at(BOULDER, xx, yy))
		if (!onscary(xx,yy,mtmp)) {
			if ((t->ttyp == TRAPDOOR || t->ttyp == HOLE)
				&& !is_floater(mtmp->data)
				&& !mtmp->isshk && !mtmp->isgd
				&& !mtmp->ispriest
				&& Can_fall_thru(&u.uz)
						) {
				trapx = xx;
				trapy = yy;
				m.has_defense = MUSE_TRAPDOOR;
			} else if (t->ttyp == TELEP_TRAP && m.has_defense != MUSE_TRAPDOOR) {
				trapx = xx;
				trapy = yy;
				m.has_defense = MUSE_TELEPORT_TRAP;
			}
		}
	}

	if (nohands(mtmp->data))	/* can't use objects */
		goto botm;

	if (is_mercenary(mtmp->data) && (obj = m_carrying(mtmp, BUGLE))) {
		int xx, yy;
		struct monst *mon;

		/* Distance is arbitrary.  What we really want to do is
		 * have the soldier play the bugle when it sees or
		 * remembers soldiers nearby...
		 */
		for(xx = x-3; xx <= x+3; xx++) for(yy = y-3; yy <= y+3; yy++)
		if (isok(xx,yy))
		if ((mon = m_at(xx,yy)) && is_mercenary(mon->data) &&
				mon->data != &mons[PM_GUARD] &&
				(mon->msleep || (!mon->mcanmove))) {
			m.defensive = obj;
			m.has_defense = MUSE_BUGLE;
		}
	}

	/* use immediate physical escape prior to attempting magic */
	if (m.has_defense)	/* stairs, trapdoor or tele-trap, bugle alert */
		goto botm;

	/* kludge to cut down on trap destruction (particularly portals) */
	t = t_at(x,y);
	if (t && (t->ttyp == PIT || t->ttyp == SPIKED_PIT ||
		  t->ttyp == WEB || t->ttyp == BEAR_TRAP))
		t = 0;		/* ok for monster to dig here */

#define nomore(x) if(m.has_defense==x) continue;
	for (obj = mtmp->minvent; obj; obj = obj->nobj) {
		/* don't always use the same selection pattern */
		if (m.has_defense && !rn2(3)) break;

		/* nomore(MUSE_WAN_DIGGING); */
		if (m.has_defense == MUSE_WAN_DIGGING) break;
		if (obj->otyp == WAN_DIGGING && obj->spe > 0 && !stuck && !t
		    && !mtmp->isshk && !mtmp->isgd && !mtmp->ispriest
		    && !is_floater(mtmp->data)
		    /* digging wouldn't be effective; assume they know that */
		    && !(levl[x][y].wall_info & W_NONDIGGABLE)
		    && !(Is_botlevel(&u.uz) || In_endgame(&u.uz))
		    && !(is_ice(x,y) || is_pool(x,y) || is_lava(x,y))) {
			m.defensive = obj;
			m.has_defense = MUSE_WAN_DIGGING;
		}
		nomore(MUSE_WAN_TELEPORTATION_SELF);
		nomore(MUSE_WAN_TELEPORTATION);
		if(obj->otyp == WAN_TELEPORTATION && obj->spe > 0) {
			m.defensive = obj;
			m.has_defense = (mon_has_amulet(mtmp))
				? MUSE_WAN_TELEPORTATION
				: MUSE_WAN_TELEPORTATION_SELF;
		}
		nomore(MUSE_SCR_TELEPORTATION);
		if(obj->otyp == SCR_TELEPORTATION && mtmp->mcansee
		   && haseyes(mtmp->data)
		   && (!obj->cursed ||
		       (!(mtmp->isshk && inhishop(mtmp))
			    && !mtmp->isgd && !mtmp->ispriest))) {
			m.defensive = obj;
			m.has_defense = MUSE_SCR_TELEPORTATION;
		}
		nomore(MUSE_POT_EXTRA_HEALING);
		if(obj->otyp == POT_EXTRA_HEALING) {
			m.defensive = obj;
			m.has_defense = MUSE_POT_EXTRA_HEALING;
		}
		nomore(MUSE_WAN_CREATE_MONSTER);
		if(obj->otyp == WAN_CREATE_MONSTER && obj->spe > 0) {
			m.defensive = obj;
			m.has_defense = MUSE_WAN_CREATE_MONSTER;
		}
		nomore(MUSE_POT_HEALING);
		if(obj->otyp == POT_HEALING) {
			m.defensive = obj;
			m.has_defense = MUSE_POT_HEALING;
		}
		nomore(MUSE_SCR_CREATE_MONSTER);
		if(obj->otyp == SCR_CREATE_MONSTER) {
			m.defensive = obj;
			m.has_defense = MUSE_SCR_CREATE_MONSTER;
		}
	}
botm:	return((boolean)(!!m.has_defense));
#undef nomore
}

/* Perform a defensive action for a monster.  Must be called immediately
 * after find_defensive().  Return values are 0: did something, 1: died,
 * 2: did something and can't attack again (i.e. teleported).
 */
int
use_defensive(mtmp)
struct monst *mtmp;
{
	int i, fleetim, how = 0;
	struct obj *otmp = m.defensive;
	boolean vis, vismon, oseen;
/*JP	const char *mcsa = "%s can see again.";*/
	const char *mcsa = "%sはまた見えるようになった．";

	if ((i = precheck(mtmp, otmp)) != 0) return i;
	vis = cansee(mtmp->mx, mtmp->my);
	vismon = canseemon(mtmp);
	oseen = otmp && vismon;

	/* when using defensive choice to run away, we want monster to avoid
	   rushing right straight back; don't override if already scared */
	fleetim = !mtmp->mflee ? (33 - (30 * mtmp->mhp / mtmp->mhpmax)) : 0;
#define m_flee(m)	if (fleetim && !m->iswiz) \
			{ m->mflee = 1; m->mfleetim = fleetim; }

	switch(m.has_defense) {
	case MUSE_UNICORN_HORN:
		if (vismon) {
		    if (otmp)
/*JP
			pline("%s uses a unicorn horn!", Monnam(mtmp));
*/
			pline("%sはユニコーンの角を使った！", Monnam(mtmp));
		    else
/*JP			pline("The tip of %s's horn glows!", mon_nam(mtmp));*/
			pline("%sの角の先端が輝いた！", mon_nam(mtmp));
		}
		if (!mtmp->mcansee) {
		    mtmp->mcansee = 1;
		    mtmp->mblinded = 0;
		    if (vismon) pline(mcsa, Monnam(mtmp));
		} else if (mtmp->mconf || mtmp->mstun) {
		    mtmp->mconf = mtmp->mstun = 0;
		    if (vismon)
/*JP			pline("%s seems steadier now.", Monnam(mtmp));*/
			pline("%sは平静をとりもどした．", Monnam(mtmp));
		} else impossible("No need for unicorn horn?");
		return 2;
	case MUSE_BUGLE:
		if (vismon)
/*JP			pline("%s plays %s!", Monnam(mtmp), doname(otmp));*/
			pline("%sは%sを吹いた！", Monnam(mtmp), doname(otmp));
		else if (flags.soundok)
/*JP			You_hear("a bugle playing reveille!");*/
			You("起床ラッパの音を聞いた！");
		awaken_soldiers();
		return 2;
	case MUSE_WAN_TELEPORTATION_SELF:
		if ((mtmp->isshk && inhishop(mtmp))
		       || mtmp->isgd || mtmp->ispriest) return 2;
		m_flee(mtmp);
		mzapmsg(mtmp, otmp, TRUE);
		otmp->spe--;
		how = WAN_TELEPORTATION;
mon_tele:
		if (tele_restrict(mtmp)) {	/* mysterious force... */
		    if (vismon && how)		/* mentions 'teleport' */
			makeknown(how);
		    return 2;
		}
		if ((
#if 0
			mon_has_amulet(mtmp) ||
#endif
			On_W_tower_level(&u.uz)) && !rn2(3)) {
		    if (vismon)
/*JP			pline("%s seems disoriented for a moment.",*/
			pline("%sは一瞬方向感覚を失った．",
				Monnam(mtmp));
		    return 2;
		}
		if (oseen && how) makeknown(how);
		rloc(mtmp);
		return 2;
	case MUSE_WAN_TELEPORTATION:
		zap_oseen = oseen;
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		m_using = TRUE;
		mbhit(mtmp,rn1(8,6),mbhitm,bhito,otmp);
		m_using = FALSE;
		return 2;
	case MUSE_SCR_TELEPORTATION:
	    {
		int obj_is_cursed = otmp->cursed;

		if (mtmp->isshk || mtmp->isgd || mtmp->ispriest) return 2;
		m_flee(mtmp);
		mreadmsg(mtmp, otmp);
		m_useup(mtmp, otmp);	/* otmp might be free'ed */
		how = SCR_TELEPORTATION;
		if (obj_is_cursed || mtmp->mconf) {
			int nlev;
			d_level flev;

			if (mon_has_amulet(mtmp) || In_endgame(&u.uz)) {
			    if (vismon)
/*JP				pline("%s seems very disoriented for a moment.",*/
				pline("%sは一瞬方向感覚を失った．",
					Monnam(mtmp));
			    return 2;
			}
			nlev = random_teleport_level();
			if (nlev == depth(&u.uz)) {
			    if (vismon)
/*JP				pline("%s shudders for a moment.",*/
				pline("%sは一瞬震えた．",
								Monnam(mtmp));
			    return 2;
			}
			get_level(&flev, nlev);
			migrate_to_level(mtmp, ledger_no(&flev), MIGR_RANDOM,
				(coord *)0);
			if (oseen) makeknown(SCR_TELEPORTATION);
		} else goto mon_tele;
		return 2;
	    }
	case MUSE_WAN_DIGGING:
	    {	struct trap *ttmp;

		m_flee(mtmp);
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		if (oseen) makeknown(WAN_DIGGING);
		if (IS_FURNITURE(levl[mtmp->mx][mtmp->my].typ) ||
		    IS_DRAWBRIDGE(levl[mtmp->mx][mtmp->my].typ) ||
		    (is_drawbridge_wall(mtmp->mx, mtmp->my) >= 0) ||
		    (sstairs.sx && sstairs.sx == mtmp->mx &&
				   sstairs.sy == mtmp->my)) {
/*JP			pline_The("digging ray is ineffective.");*/
			pline("掘削の光は効果がない．");
			return 2;
		}
		if (!Can_dig_down(&u.uz)) {
		    if(canseemon(mtmp))
/*JP			pline_The("floor here is too hard to dig in.");*/
			pline("ここの床は固くて掘れない．");
		    return 2;
		}
		ttmp = maketrap(mtmp->mx, mtmp->my, HOLE);
		if (!ttmp) return 2;
		seetrap(ttmp);
		if (vis) {
/*JP		    pline("%s has made a hole in the floor.", Monnam(mtmp));
		    pline("%s %s through...", Monnam(mtmp),
			  is_flyer(mtmp->data) ? "dives" : "falls");*/
		    pline("%sは床に穴を開けた．", Monnam(mtmp));
		    pline("%sは通り抜けて%s．．．", Monnam(mtmp),
			  is_flyer(mtmp->data) ? "飛びこんだ" : "落ちた");
		} else if (flags.soundok)
/*JP			You_hear("%s crash through the floor.", something);*/
			You_hear("%sが床を通り抜けて落ちる音を聞いた．．．", something);
		/* we made sure that there is a level for mtmp to go to */
		migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				 MIGR_RANDOM, (coord *)0);
		return 2;
	    }
	case MUSE_WAN_CREATE_MONSTER:
	    {	coord cc;
		    /* pm: 0 => random, eel => aquatic, croc => amphibious */
		struct permonst *pm = !is_pool(mtmp->mx, mtmp->my) ? 0 :
			     &mons[u.uinwater ? PM_GIANT_EEL : PM_CROCODILE];
		struct monst *mon;

		if (!enexto(&cc, mtmp->mx, mtmp->my, pm)) return 0;
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		mon = makemon((struct permonst *)0, cc.x, cc.y, NO_MM_FLAGS);
		if (mon && canspotmon(mon) && oseen)
		    makeknown(WAN_CREATE_MONSTER);
		return 2;
	    }
	case MUSE_SCR_CREATE_MONSTER:
	    {	coord cc;
		struct permonst *pm = 0, *fish = 0;
		int cnt = 1;
		struct monst *mon;
		boolean known = FALSE;

		if (!rn2(73)) cnt += rnd(4);
		if (mtmp->mconf || otmp->cursed) cnt += 12;
		if (mtmp->mconf) pm = fish = &mons[PM_ACID_BLOB];
		else if (is_pool(mtmp->mx, mtmp->my))
		    fish = &mons[u.uinwater ? PM_GIANT_EEL : PM_CROCODILE];
		mreadmsg(mtmp, otmp);
		while(cnt--) {
		    /* `fish' potentially gives bias towards water locations;
		       `pm' is what to actually create (0 => random) */
		    if (!enexto(&cc, mtmp->mx, mtmp->my, fish)) break;
		    mon = makemon(pm, cc.x, cc.y, NO_MM_FLAGS);
		    if (mon && canspotmon(mon)) known = TRUE;
		}
		/* The only case where we don't use oseen.  For wands, you
		 * have to be able to see the monster zap the wand to know
		 * what type it is.  For teleport scrolls, you have to see
		 * the monster to know it teleported.
		 */
		if (known)
		    makeknown(SCR_CREATE_MONSTER);
		else if (!objects[SCR_CREATE_MONSTER].oc_name_known
			&& !objects[SCR_CREATE_MONSTER].oc_uname)
		    docall(otmp);
		m_useup(mtmp, otmp);
		return 2;
	    }
	case MUSE_TRAPDOOR:
		/* trapdoors on "bottom" levels of dungeons are rock-drop
		 * trapdoors, not holes in the floor.  We check here for
		 * safety.
		 */
		if (Is_botlevel(&u.uz)) return 0;
		m_flee(mtmp);
		if (vis) {
			struct trap *t;
			t = t_at(trapx,trapy);
/*JP			pline("%s %s into a %s!", Monnam(mtmp),
			makeplural(locomotion(mtmp->data, "jump")),
			t->ttyp == TRAPDOOR ? "trapdoor" : "hole");*/
			pline("%sは%sに%s入った！", Monnam(mtmp),
			      t->ttyp == TRAPDOOR ? "落し扉" : "穴",
			      jconj(locomotion(mtmp->data, "飛ぶ"), "て"));
			seetrap(t_at(trapx,trapy));
		}

		/*  don't use rloc_to() because worm tails must "move" */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);	/* update old location */
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);

		migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				 MIGR_RANDOM, (coord *)0);
		return 2;
	case MUSE_UPSTAIRS:
		/* Monsters without amulets escape the dungeon and are
		 * gone for good when they leave up the up stairs.
		 * Monsters with amulets would reach the endlevel,
		 * which we cannot allow since that would leave the
		 * player stranded.
		 */
		if (ledger_no(&u.uz) == 1) {
			if (mon_has_special(mtmp))
				return 0;
			if (vismon)
/*JP			    pline("%s escapes the dungeon!", Monnam(mtmp));*/
			    pline("%sは迷宮から逃走した！", Monnam(mtmp));
			mongone(mtmp);
			return 2;
		}
		m_flee(mtmp);
		if (Inhell && mon_has_amulet(mtmp) && !rn2(4) &&
			(dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz) - 3)) {
		    if (vismon) pline(
/*JP "As %s climbs the stairs, a mysterious force momentarily surrounds %s...",
				     mon_nam(mtmp), him[pronoun_gender(mtmp)]);*/
     "%sが階段をのぼろうとすると奇妙な力が%sをつつんだ．．．",
				     mon_nam(mtmp), mon_nam(mtmp));
		    /* simpler than for the player; this will usually be
		       the Wizard and he'll immediately go right to the
		       upstairs, so there's not much point in having any
		       chance for a random position on the current level */
		    migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				     MIGR_RANDOM, (coord *)0);
		} else {
/*JP		    if (vismon) pline("%s escapes upstairs!", Monnam(mtmp));*/
		    if (vismon) pline("%sは階段をのぼって逃げた！", Monnam(mtmp));
		    migrate_to_level(mtmp, ledger_no(&u.uz) - 1,
				     MIGR_STAIRS_DOWN, (coord *)0);
		}
		return 2;
	case MUSE_DOWNSTAIRS:
		m_flee(mtmp);
/*JP		if (vismon) pline("%s escapes downstairs!", Monnam(mtmp));*/
		if (vismon) pline("%sは階段を降りて逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				 MIGR_STAIRS_UP, (coord *)0);
		return 2;
	case MUSE_UP_LADDER:
		m_flee(mtmp);
/*JP		if (vismon) pline("%s escapes up the ladder!", Monnam(mtmp));*/
		if (vismon) pline("%sは梯子をのぼって逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&u.uz) - 1,
				 MIGR_LADDER_DOWN, (coord *)0);
		return 2;
	case MUSE_DN_LADDER:
		m_flee(mtmp);
/*JP		if (vismon) pline("%s escapes down the ladder!", Monnam(mtmp));*/
		if (vismon) pline("%sは梯子を降りて逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				 MIGR_LADDER_UP, (coord *)0);
		return 2;
	case MUSE_SSTAIRS:
		m_flee(mtmp);
		/* the stairs leading up from the 1st level are */
		/* regular stairs, not sstairs.			*/
		if (sstairs.up) {
			if (vismon)
/*JP			    pline("%s escapes upstairs!", Monnam(mtmp));*/
			    pline("%sはのぼり階段に逃げた！", Monnam(mtmp));
			if(Inhell) {
			    migrate_to_level(mtmp, ledger_no(&sstairs.tolev),
					     MIGR_RANDOM, (coord *)0);
			    return 2;
			}
		} else	if (vismon)
/*JP		    pline("%s escapes downstairs!", Monnam(mtmp));*/
		    pline("%sは降り階段に逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&sstairs.tolev),
				 MIGR_SSTAIRS, (coord *)0);
		return 2;
	case MUSE_TELEPORT_TRAP:
		m_flee(mtmp);
		if (vis) {
/*JP			pline("%s %s onto a teleport trap!", Monnam(mtmp),
				makeplural(locomotion(mtmp->data, "jump")));*/
		if (vis) pline("%sは瞬間移動の罠に%s入った！", Monnam(mtmp),
			jconj(locomotion(mtmp->data, "飛ぶ"),"て"));
			seetrap(t_at(trapx,trapy));
		}
		/*  don't use rloc_to() because worm tails must "move" */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);	/* update old location */
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);

		goto mon_tele;
	case MUSE_POT_HEALING:
		mquaffmsg(mtmp, otmp);
		i = d(6 + 2 * bcsign(otmp), 4);
		mtmp->mhp += i;
		if (mtmp->mhp > mtmp->mhpmax) mtmp->mhp = ++mtmp->mhpmax;
		if (!otmp->cursed && !mtmp->mcansee) {
			mtmp->mcansee = 1;
			mtmp->mblinded = 0;
			if (vismon) pline(mcsa, Monnam(mtmp));
		}	
/*JP		if (vismon) pline("%s looks better.", Monnam(mtmp));*/
		if (vismon) pline("%sは気分がよくなったようだ．", Monnam(mtmp));

		if (oseen) makeknown(POT_HEALING);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_POT_EXTRA_HEALING:
		mquaffmsg(mtmp, otmp);
		i = d(6 + 2 * bcsign(otmp), 8);
		mtmp->mhp += i;
		if (mtmp->mhp > mtmp->mhpmax)
			mtmp->mhp = (mtmp->mhpmax += (otmp->blessed ? 5 : 2));
		if (!mtmp->mcansee) {
			mtmp->mcansee = 1;
			mtmp->mblinded = 0;
			if (vismon) pline(mcsa, Monnam(mtmp));
		}
/*JP		if (vismon) pline("%s looks much better.", Monnam(mtmp));*/
		if (vismon) pline("%sとても気分がよくなったようだ．", Monnam(mtmp));
		if (oseen) makeknown(POT_EXTRA_HEALING);
		m_useup(mtmp, otmp);
		return 2;
	case 0: return 0; /* i.e. an exploded wand */
	default: impossible("%s wanted to perform action %d?", Monnam(mtmp),
			m.has_defense);
		break;
	}
	return 0;
#undef m_flee
}

int
rnd_defensive_item(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless(mtmp->data)
			|| pm->mlet == S_GHOST
# ifdef KOPS
			|| pm->mlet == S_KOP
# endif
		) return 0;
	switch (rn2(8 + (difficulty > 3) + (difficulty > 6) +
				(difficulty > 8))) {
		case 6: case 9:
			if (!rn2(3)) return WAN_TELEPORTATION;
			/* else FALLTHRU */
		case 0: case 1:
			return SCR_TELEPORTATION;
		case 8: case 10:
			if (!rn2(3)) return WAN_CREATE_MONSTER;
			/* else FALLTHRU */
		case 2: return SCR_CREATE_MONSTER;
		case 3: case 4:
			return POT_HEALING;
		case 5: return POT_EXTRA_HEALING;
		case 7: if (is_floater(pm) || mtmp->isshk || mtmp->isgd
						|| mtmp->ispriest
									)
				return 0;
			else
				return WAN_DIGGING;
	}
	/*NOTREACHED*/
	return 0;
}

#define MUSE_WAN_DEATH 1
#define MUSE_WAN_SLEEP 2
#define MUSE_WAN_FIRE 3
#define MUSE_WAN_COLD 4
#define MUSE_WAN_LIGHTNING 5
#define MUSE_WAN_MAGIC_MISSILE 6
#define MUSE_WAN_STRIKING 7
#define MUSE_SCR_FIRE 8
#define MUSE_POT_PARALYSIS 9
#define MUSE_POT_BLINDNESS 10
#define MUSE_POT_CONFUSION 11
#define MUSE_FROST_HORN 12
#define MUSE_FIRE_HORN 13

/* Select an offensive item/action for a monster.  Returns TRUE iff one is
 * found.
 */
boolean
find_offensive(mtmp)
struct monst *mtmp;
{
	register struct obj *obj;
	boolean ranged_stuff = lined_up(mtmp);
	boolean reflection_skip = (Reflecting && rn2(2));

	m.offensive = (struct obj *)0;
	m.has_offense = 0;
	if (mtmp->mpeaceful || is_animal(mtmp->data) ||
				mindless(mtmp->data) || nohands(mtmp->data))
		return FALSE;
	if (u.uswallow) return FALSE;
	if (in_your_sanctuary(mtmp->mx, mtmp->my)) return FALSE;
	if (dmgtype(mtmp->data, AD_HEAL) && !uwep
#ifdef TOURIST
	    && !uarmu
#endif
	    && !uarm && !uarmh && !uarms && !uarmg && !uarmc && !uarmf)
		return FALSE;

	if (!ranged_stuff) return FALSE;
#define nomore(x) if(m.has_offense==x) continue;
	for(obj=mtmp->minvent; obj; obj=obj->nobj) {
		/* nomore(MUSE_WAN_DEATH); */
		if (!reflection_skip) {
		    if(obj->otyp == WAN_DEATH && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_DEATH;
		    }
		    nomore(MUSE_WAN_SLEEP);
		    if(obj->otyp == WAN_SLEEP && obj->spe > 0 && multi >= 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_SLEEP;
		    }
		    nomore(MUSE_WAN_FIRE);
		    if(obj->otyp == WAN_FIRE && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_FIRE;
		    }
		    nomore(MUSE_FIRE_HORN);
		    if(obj->otyp == FIRE_HORN && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_FIRE_HORN;
		    }
		    nomore(MUSE_WAN_COLD);
		    if(obj->otyp == WAN_COLD && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_COLD;
		    }
		    nomore(MUSE_FROST_HORN);
		    if(obj->otyp == FROST_HORN && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_FROST_HORN;
		    }
		    nomore(MUSE_WAN_LIGHTNING);
		    if(obj->otyp == WAN_LIGHTNING && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_LIGHTNING;
		    }
		    nomore(MUSE_WAN_MAGIC_MISSILE);
		    if(obj->otyp == WAN_MAGIC_MISSILE && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_MAGIC_MISSILE;
		    }
		}
		nomore(MUSE_WAN_STRIKING);
		if(obj->otyp == WAN_STRIKING && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_STRIKING;
		}
		nomore(MUSE_POT_PARALYSIS);
		if(obj->otyp == POT_PARALYSIS && multi >= 0) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_PARALYSIS;
		}
		nomore(MUSE_POT_BLINDNESS);
		if(obj->otyp == POT_BLINDNESS) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_BLINDNESS;
		}
		nomore(MUSE_POT_CONFUSION);
		if(obj->otyp == POT_CONFUSION) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_CONFUSION;
		}
#if 0
		nomore(MUSE_SCR_FIRE);
		/* even more restrictive than ranged_stuff */
		if (obj->otyp == SCR_FIRE && resists_fire(mtmp)
		   && distu(mtmp->mx,mtmp->my)==1
		   && mtmp->mcansee && haseyes(mtmp->data)) {
			m.offensive = obj;
			m.has_offense = MUSE_SCR_FIRE;
		}
#endif
	}
	return((boolean)(!!m.has_offense));
#undef nomore
}

STATIC_PTR
int
mbhitm(mtmp, otmp)
register struct monst *mtmp;
register struct obj *otmp;
{
	int tmp;

	if (mtmp != &youmonst) {
		mtmp->msleep = 0;
		if (mtmp->m_ap_type) seemimic(mtmp);
	}
	switch(otmp->otyp) {
	case WAN_STRIKING:
		if (mtmp == &youmonst) {
			if (zap_oseen) makeknown(WAN_STRIKING);
			if (rnd(20) < 10 + u.uac) {
/*JP				pline_The("wand hits you!");*/
				pline("杖はあなたに命中した！");
				tmp = d(2,12);
				if(Half_spell_damage) tmp = (tmp+1) / 2;
/*JP				losehp(tmp, "wand", KILLED_BY_AN);*/
				losehp(tmp, "衝撃の杖によって", KILLED_BY_AN);
/*JP			} else pline_The("wand misses you.");*/
			} else pline("杖ははずれた．");
			stop_occupation();
			nomul(0);
		} else if (rnd(20) < 10+find_mac(mtmp)) {
			tmp = d(2,12);
			if(Half_spell_damage) tmp = (tmp+1) / 2;
/*JP			hit("wand", mtmp, exclam(tmp));*/
			hit("杖", mtmp, exclam(tmp));
			(void) resist(mtmp, otmp->oclass, tmp, TELL);
			if (cansee(mtmp->mx, mtmp->my) && zap_oseen)
				makeknown(WAN_STRIKING);
		} else {
/*JP			miss("wand", mtmp);*/
			miss("杖", mtmp);
			if (cansee(mtmp->mx, mtmp->my) && zap_oseen)
				makeknown(WAN_STRIKING);
		}
		break;
	case WAN_TELEPORTATION:
		if (mtmp == &youmonst) {
			if (zap_oseen) makeknown(WAN_TELEPORTATION);
			tele();
		} else {
			/* for consistency with zap.c, don't identify */
			if (mtmp->ispriest &&
				*in_rooms(mtmp->mx, mtmp->my, TEMPLE)) {
			    if (cansee(mtmp->mx, mtmp->my))
/*JP				pline("%s resists the magic!", Monnam(mtmp));*/
				pline("%sは魔法を防いだ！", Monnam(mtmp));
			    mtmp->msleep = 0;
			    if(mtmp->m_ap_type) seemimic(mtmp);
			} else
			    rloc(mtmp);
		}
		break;
	case WAN_CANCELLATION:
	case SPE_CANCELLATION:
		cancel_monst(mtmp, otmp, FALSE, TRUE, FALSE);
		break;
	}
	return 0;
}

/* A modified bhit() for monsters.  Based on bhit() in zap.c.  Unlike
 * buzz(), bhit() doesn't take into account the possibility of a monster
 * zapping you, so we need a special function for it.  (Unless someone wants
 * to merge the two functions...)
 */
static void
mbhit(mon,range,fhitm,fhito,obj)
struct monst *mon;			/* monster shooting the wand */
register int range;			/* direction and range */
int FDECL((*fhitm),(MONST_P,OBJ_P));
int FDECL((*fhito),(OBJ_P,OBJ_P));	/* fns called when mon/obj hit */
struct obj *obj;			/* 2nd arg to fhitm/fhito */
{
	register struct monst *mtmp;
	register struct obj *otmp;
	register uchar typ;
	int ddx, ddy;

	bhitpos.x = mon->mx;
	bhitpos.y = mon->my;
	ddx = sgn(mon->mux - mon->mx);
	ddy = sgn(mon->muy - mon->my);

	while(range-- > 0) {
		int x,y;

		bhitpos.x += ddx;
		bhitpos.y += ddy;
		x = bhitpos.x; y = bhitpos.y;

		if (!isok(x,y)) {
		    bhitpos.x -= ddx;
		    bhitpos.y -= ddy;
		    break;
		}
		if (find_drawbridge(&x,&y))
		    switch (obj->otyp) {
			case WAN_STRIKING:
			    destroy_drawbridge(x,y);
		    }
		if(bhitpos.x==u.ux && bhitpos.y==u.uy) {
			(*fhitm)(&youmonst, obj);
			range -= 3;
		} else if(MON_AT(bhitpos.x, bhitpos.y)){
			mtmp = m_at(bhitpos.x,bhitpos.y);
			(*fhitm)(mtmp, obj);
			range -= 3;
		}
		/* modified by GAN to hit all objects */
		if(fhito){
		    int hitanything = 0;
		    register struct obj *next_obj;

		    for(otmp = level.objects[bhitpos.x][bhitpos.y];
							otmp; otmp = next_obj) {
			/* Fix for polymorph bug, Tim Wright */
			next_obj = otmp->nexthere;
			hitanything += (*fhito)(otmp, obj);
		    }
		    if(hitanything)	range--;
		}
		typ = levl[bhitpos.x][bhitpos.y].typ;
		if(IS_DOOR(typ) || typ == SDOOR) {
		    switch (obj->otyp) {
			/* note: monsters don't use opening or locking magic
			   at present, but keep these as placeholders */
			case WAN_OPENING:
			case WAN_LOCKING:
			case WAN_STRIKING:
			    if (doorlock(obj, bhitpos.x, bhitpos.y)) {
				makeknown(obj->otyp);
				/* if a shop door gets broken, add it to
				   the shk's fix list (no cost to player) */
				if (levl[bhitpos.x][bhitpos.y].doormask ==
					D_BROKEN &&
				    *in_rooms(bhitpos.x, bhitpos.y, SHOPBASE))
				    add_damage(bhitpos.x, bhitpos.y, 0L);
			    }
			    break;
		    }
		}
		if(!ZAP_POS(typ) || (IS_DOOR(typ) &&
		   (levl[bhitpos.x][bhitpos.y].doormask & (D_LOCKED | D_CLOSED)))
		  ) {
			bhitpos.x -= ddx;
			bhitpos.y -= ddy;
			break;
		}
	}
}

/* Perform an offensive action for a monster.  Must be called immediately
 * after find_offensive().  Return values are same as use_defensive().
 */
int
use_offensive(mtmp)
struct monst *mtmp;
{
	int i;
	struct obj *otmp = m.offensive;
	boolean oseen;

	/* offensive potions are not drunk, they're thrown */
	if (otmp->oclass != POTION_CLASS && (i = precheck(mtmp, otmp)) != 0)
		return i;
	oseen = otmp && canseemon(mtmp);

	switch(m.has_offense) {
	case MUSE_WAN_DEATH:
	case MUSE_WAN_SLEEP:
	case MUSE_WAN_FIRE:
	case MUSE_WAN_COLD:
	case MUSE_WAN_LIGHTNING:
	case MUSE_WAN_MAGIC_MISSILE:
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		if (oseen) makeknown(otmp->otyp);
		m_using = TRUE;
		buzz((int)(-30 - (otmp->otyp - WAN_MAGIC_MISSILE)),
			(otmp->otyp == WAN_MAGIC_MISSILE) ? 2 : 6,
			mtmp->mx, mtmp->my,
			sgn(mtmp->mux-mtmp->mx), sgn(mtmp->muy-mtmp->my));
		m_using = FALSE;
		return (mtmp->mhp <= 0) ? 1 : 2;
	case MUSE_FIRE_HORN:
	case MUSE_FROST_HORN:
		if (oseen) {
			makeknown(otmp->otyp);
/*JP			pline("%s plays a %s!", Monnam(mtmp), xname(otmp));*/
			pline("%sは%sを吹いた！", Monnam(mtmp), xname(otmp));
		} else
/*JP			You_hear("a horn being played.");*/
			You_hear("ホルンの音を聞いた．");
		otmp->spe--;
		m_using = TRUE;
		buzz(-30 - ((otmp->otyp==FROST_HORN) ? AD_COLD-1 : AD_FIRE-1),
			rn1(6,6), mtmp->mx, mtmp->my,
			sgn(mtmp->mux-mtmp->mx), sgn(mtmp->muy-mtmp->my));
		m_using = FALSE;
		return (mtmp->mhp <= 0) ? 1 : 2;
	case MUSE_WAN_TELEPORTATION:
	case MUSE_WAN_STRIKING:
		zap_oseen = oseen;
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		m_using = TRUE;
		mbhit(mtmp,rn1(8,6),mbhitm,bhito,otmp);
		m_using = FALSE;
		return 2;
#if 0
	case MUSE_SCR_FIRE:
	      {
		boolean vis = cansee(mtmp->mx, mtmp->my);

		mreadmsg(mtmp, otmp);
		if (mtmp->mconf) {
			if (vis)
/*JP			    pline("Oh, what a pretty fire!");*/
			    pline("おや，なんて小さい火だ！"); 
		} else {
			struct monst *mtmp2;
			int num;

			if (vis)
/*JP			    pline_The("scroll erupts in a tower of flame!");*/
			    pline("巻物から火柱が立ちのぼった！");
			shieldeff(mtmp->mx, mtmp->my);
/*JP			pline("%s is uninjured.", Monnam(mtmp));*/
			pline("%sは傷つかない．", Monnam(mtmp));
			(void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			(void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
			(void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			num = (2*(rn1(3, 3) + 2 * bcsign(otmp)) + 1)/3;
			if (Fire_resistance)
/*JP			    You("are not affected.");*/
			    You("影響を受けない．");
			if (Half_spell_damage) num = (num+1) / 2;
			else losehp(num, "炎の巻物で", KILLED_BY_AN);
			for(mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon) {
			   if(mtmp == mtmp2) continue;
			   if(dist2(mtmp2->mx,mtmp2->my,mtmp->mx,mtmp->my) < 3){
				if (resists_fire(mtmp2)) continue;
				mtmp2->mhp -= num;
				if (resists_cold(mtmp2))
				    mtmp2->mhp -= 3*num;
				if(mtmp2->mhp < 1) {
				    mondied(mtmp2);
				    break;
				}
			    }
			}
		}
		return 2;
	      }
#endif	/* 0 */
	case MUSE_POT_PARALYSIS:
	case MUSE_POT_BLINDNESS:
	case MUSE_POT_CONFUSION:
		/* Note: this setting of dknown doesn't suffice.  A monster
		 * which is out of sight might throw and it hits something _in_
		 * sight, a problem not existing with wands because wand rays
		 * are not objects.  Also set dknown in mthrowu.c.
		 */
		if (cansee(mtmp->mx, mtmp->my)) {
			otmp->dknown = 1;
/*JP			pline("%s hurls %s!", Monnam(mtmp),*/
			pline("%sは%sを強く投げつけた！", Monnam(mtmp),
						singular(otmp, doname));
		}
		m_throw(mtmp, mtmp->mx, mtmp->my, sgn(mtmp->mux-mtmp->mx),
			sgn(mtmp->muy-mtmp->my),
			distmin(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy), otmp);
		return 2;
	case 0: return 0; /* i.e. an exploded wand */
	default: impossible("%s wanted to perform action %d?", Monnam(mtmp),
			m.has_offense);
		break;
	}
	return 0;
}

int
rnd_offensive_item(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless(mtmp->data)
			|| pm->mlet == S_GHOST
# ifdef KOPS
			|| pm->mlet == S_KOP
# endif
		) return 0;
	if (difficulty > 7 && !rn2(35)) return WAN_DEATH;
	switch (rn2(7 - (difficulty < 4) + 4 * (difficulty > 6))) {
		case 0: case 1:
			return WAN_STRIKING;
		case 2: return POT_CONFUSION;
		case 3: return POT_BLINDNESS;
		case 4: return POT_PARALYSIS;
		case 5: case 6:
			return WAN_MAGIC_MISSILE;
		case 7: return WAN_SLEEP;
		case 8: return WAN_FIRE;
		case 9: return WAN_COLD;
		case 10: return WAN_LIGHTNING;
	}
	/*NOTREACHED*/
	return 0;
}

#define MUSE_POT_GAIN_LEVEL 1
#define MUSE_WAN_MAKE_INVISIBLE 2
#define MUSE_POT_INVISIBILITY 3
#define MUSE_POLY_TRAP 4
#define MUSE_WAN_POLYMORPH 5
#define MUSE_POT_SPEED 6
#define MUSE_WAN_SPEED_MONSTER 7
#define MUSE_BULLWHIP 8

boolean
find_misc(mtmp)
struct monst *mtmp;
{
	register struct obj *obj;
	struct permonst *mdat = mtmp->data;
	int x = mtmp->mx, y = mtmp->my;
	struct trap *t;
	int xx, yy;
	boolean immobile = (mdat->mmove == 0);
	boolean stuck = (mtmp == u.ustuck);

	m.misc = (struct obj *)0;
	m.has_misc = 0;
	if (is_animal(mdat) || mindless(mdat))
		return 0;
	if (u.uswallow && stuck) return FALSE;

	/* We arbitrarily limit to times when a player is nearby for the
	 * same reason as Junior Pac-Man doesn't have energizers eaten until
	 * you can see them...
	 */
	if(dist2(x, y, mtmp->mux, mtmp->muy) > 36)
		return FALSE;

	if (!stuck && !immobile && !mtmp->cham && monstr[monsndx(mdat)] < 6) {
	  boolean ignore_boulders = (verysmall(mdat) ||
				     throws_rocks(mdat) ||
				     passes_walls(mdat));
	  for(xx = x-1; xx <= x+1; xx++)
	    for(yy = y-1; yy <= y+1; yy++)
		if (isok(xx,yy) && (xx != u.ux || yy != u.uy))
		    if (mdat != &mons[PM_GRID_BUG] || xx == x || yy == y)
			if (/* (xx==x && yy==y) || */ !level.monsters[xx][yy])
			    if ((t = t_at(xx, yy)) != 0 &&
			      (ignore_boulders || !sobj_at(BOULDER, xx, yy))
			      && !onscary(xx, yy, mtmp)) {
				if (t->ttyp == POLY_TRAP) {
				    trapx = xx;
				    trapy = yy;
				    m.has_misc = MUSE_POLY_TRAP;
				    return TRUE;
				}
			    }
	}
	if (nohands(mdat))
		return 0;

#define nomore(x) if(m.has_misc==x) continue;
	for(obj=mtmp->minvent; obj; obj=obj->nobj) {
		/* Monsters shouldn't recognize cursed items; this kludge is */
		/* necessary to prevent serious problems though... */
		if(obj->otyp == POT_GAIN_LEVEL && (!obj->cursed ||
			    (!mtmp->isgd && !mtmp->isshk && !mtmp->ispriest))) {
			m.misc = obj;
			m.has_misc = MUSE_POT_GAIN_LEVEL;
		}
		nomore(MUSE_BULLWHIP);
		if(obj->otyp == BULLWHIP && (MON_WEP(mtmp) == obj) &&
		   distu(mtmp->mx,mtmp->my)==1 && uwep) {
			m.misc = obj;
			m.has_misc = MUSE_BULLWHIP;
		}
		/* Note: peaceful/tame monsters won't make themselves
		 * invisible unless you can see them.  Not really right, but...
		 */
		nomore(MUSE_WAN_MAKE_INVISIBLE);
		if(obj->otyp == WAN_MAKE_INVISIBLE && obj->spe > 0 &&
		    !mtmp->minvis && !mtmp->invis_blkd &&
		    (!mtmp->mpeaceful || See_invisible) &&
		    (mdat != &mons[PM_MEDUSA] || mtmp->mcan)) {
			m.misc = obj;
			m.has_misc = MUSE_WAN_MAKE_INVISIBLE;
		}
		nomore(MUSE_POT_INVISIBILITY);
		if(obj->otyp == POT_INVISIBILITY &&
		    !mtmp->minvis && !mtmp->invis_blkd &&
		    (!mtmp->mpeaceful || See_invisible)) {
			m.misc = obj;
			m.has_misc = MUSE_POT_INVISIBILITY;
		}
		nomore(MUSE_WAN_SPEED_MONSTER);
		if(obj->otyp == WAN_SPEED_MONSTER && obj->spe > 0
				&& mtmp->mspeed != MFAST && !mtmp->isgd) {
			m.misc = obj;
			m.has_misc = MUSE_WAN_SPEED_MONSTER;
		}
		nomore(MUSE_POT_SPEED);
		if(obj->otyp == POT_SPEED && mtmp->mspeed != MFAST
							&& !mtmp->isgd) {
			m.misc = obj;
			m.has_misc = MUSE_POT_SPEED;
		}
		nomore(MUSE_WAN_POLYMORPH);
		if(obj->otyp == WAN_POLYMORPH && obj->spe > 0 && !mtmp->cham
				&& monstr[monsndx(mdat)] < 6) {
			m.misc = obj;
			m.has_misc = MUSE_WAN_POLYMORPH;
		}
	}
	return((boolean)(!!m.has_misc));
#undef nomore
}

int
use_misc(mtmp)
struct monst *mtmp;
{
	int i;
	struct obj *otmp = m.misc;
	boolean vis, vismon, oseen;

	if ((i = precheck(mtmp, otmp)) != 0) return i;
	vis = cansee(mtmp->mx, mtmp->my);
	vismon = canseemon(mtmp);
	oseen = otmp && vismon;

	switch(m.has_misc) {
	case MUSE_POT_GAIN_LEVEL:
		mquaffmsg(mtmp, otmp);
		if (otmp->cursed) {
		    if (Can_rise_up(mtmp->mx, mtmp->my, &u.uz)) {
			register int tolev = depth(&u.uz)-1;
			d_level tolevel;

			get_level(&tolevel, tolev);
			/* insurance against future changes... */
			if(on_level(&tolevel, &u.uz)) goto skipmsg;
			if (vismon) {
/*JP			    pline("%s rises up, through the %s!",*/
			    pline("%sは%sを突き抜けた！",
				  Monnam(mtmp), ceiling(mtmp->mx, mtmp->my));
			    if(!objects[POT_GAIN_LEVEL].oc_name_known
			      && !objects[POT_GAIN_LEVEL].oc_uname)
				docall(otmp);
			}
			m_useup(mtmp, otmp);
			migrate_to_level(mtmp, ledger_no(&tolevel),
					 MIGR_RANDOM, (coord *)0);
			return 2;
		    } else {
skipmsg:
			if (vismon) {
/*JP			    pline("%s looks uneasy.", Monnam(mtmp));*/
			    pline("%sは不安げに見える．", Monnam(mtmp));
			    if(!objects[POT_GAIN_LEVEL].oc_name_known
			      && !objects[POT_GAIN_LEVEL].oc_uname)
				docall(otmp);
			}
			m_useup(mtmp, otmp);
			return 2;
		    }
		}
/*JP		if (vismon) pline("%s seems more experienced.", Monnam(mtmp));*/
		if (vismon) pline("%sは経験を積んだように見える．", Monnam(mtmp));
		if (oseen) makeknown(POT_GAIN_LEVEL);
		m_useup(mtmp, otmp);
		if (!grow_up(mtmp,(struct monst *)0)) return 1;
			/* grew into genocided monster */
		return 2;
	case MUSE_WAN_MAKE_INVISIBLE:
		mzapmsg(mtmp, otmp, TRUE);
		otmp->spe--;
		mon_set_minvis(mtmp);
		return 2;
	case MUSE_POT_INVISIBILITY:
		mquaffmsg(mtmp, otmp);
		mon_set_minvis(mtmp);
		if (vismon && mtmp->minvis) {	/* was seen, now invisible */
		    if (See_invisible)
/*JP
			pline("%s body takes on a %s transparency.",
			      s_suffix(Monnam(mtmp)),
			      Hallucination ? "normal" : "strange");
*/
		        pline("%s%sの体は透過性をもった．",
			      Hallucination ? "あたりまえなことだが" : "奇妙なことに",
			      s_suffix(Monnam(mtmp)));
		    else
/*JP
			pline("Suddenly you cannot see %s.", mon_nam(mtmp));
*/
			pline("突然%sが見えなくなった．", mon_nam(mtmp));
		    if (oseen) makeknown(POT_INVISIBILITY);
		}
		if (otmp->cursed) you_aggravate(mtmp);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_WAN_SPEED_MONSTER:
		mzapmsg(mtmp, otmp, TRUE);
		otmp->spe--;
		if (mtmp->mspeed == MSLOW) mtmp->mspeed = 0;
		else mtmp->mspeed = MFAST;
		return 2;
	case MUSE_POT_SPEED:
		mquaffmsg(mtmp, otmp);
/*JP		if (vismon) pline("%s is suddenly moving much faster.",*/
		if (vismon) pline("%sは突然速く動けるようになった．",
			Monnam(mtmp));
		if (oseen) makeknown(POT_SPEED);
		if (mtmp->mspeed == MSLOW) mtmp->mspeed = 0;
		else mtmp->mspeed = MFAST;
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_WAN_POLYMORPH:
		mzapmsg(mtmp, otmp, TRUE);
		otmp->spe--;
		(void) newcham(mtmp, rndmonst());
		if (oseen) makeknown(WAN_POLYMORPH);
		return 2;
	case MUSE_POLY_TRAP:
		if (vismon)
/*JP		    pline("%s deliberately goes onto a polymorph trap!",*/
		    pline("%sはわざと変化の罠に飛びこんだ！",
			  Monnam(mtmp));
		if (vis) seetrap(t_at(trapx,trapy));

		/*  don't use rloc() due to worms */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);

		(void) newcham(mtmp, (struct permonst *)0);
		return 2;
	case MUSE_BULLWHIP:
		/* attempt to disarm hero */
		if (uwep) {
			if (!rn2(5)) {
				int where_to = rn2(4);
				struct obj *obj = uwep;
				
				if (vismon)
/*JP				  pline("%s flicks a bullwhip towards your %s!",*/
				  pline("%sはあなたの%sに向かって鞭を打った！",
					Monnam(mtmp),
					body_part(HAND));
				if (obj->otyp == HEAVY_IRON_BALL) {
/*JP				  pline_The("bullwhip does not wrap around %s.",*/
				  pline("鞭は%sにはからみつかなかった．.",
					the(xname(obj)));
					return 1;
				}
/*JP				pline_The("bullwhip wraps around %s %s!",
					the(xname(obj)),
					"you're wielding");*/
				pline("鞭はあなたの装備している%sにからみついた！",
					the(xname(obj)));
				if (!where_to) {
/*JP					pline_The("bullwhip slips free.");*/
					pline("鞭はほどけた");
					return 1;
				}
				freeinv(obj);
				setuwep((struct obj *)0);
				switch (where_to) {
				    case 1:	/* onto floor beneath mon */
/*JP					pline("%s yanks %s from your %s!",
						Monnam(mtmp),
						the(xname(obj)),
						body_part(HAND));*/
					pline("%sは%sをあなたの%sからぐいとひっぱった！",
						Monnam(mtmp),
						the(xname(obj)),
						body_part(HAND));
					if(obj->otyp == CRYSKNIFE)
						obj->otyp = WORM_TOOTH;
					place_object(obj, mtmp->mx, mtmp->my);
					break;
				    case 2:	/* onto floor beneath you */
/*JP					pline("%s yanks %s to the %s!",
						Monnam(mtmp),
						the(xname(obj)),
						surface(u.ux, u.uy));*/
					pline("%sは%sを%sに引き落した！",
						Monnam(mtmp),
						the(xname(obj)),
						surface(u.ux, u.uy));
					dropy(obj);
					break;
				    case 3:	/* into mon's inventory */
/*JP					pline("%s snatches %s!",
						Monnam(mtmp),
						the(xname(obj)));*/
					pline("%sは%sを奪った！",
						Monnam(mtmp),
						the(xname(obj)));
					mpickobj(mtmp,obj);
					break;
				    default:
					/* shouldn't happen */
/*JP					pline("Where's your weapon!");*/
					pline("武器はどこだ！");
					place_object(obj,u.ux, u.uy);
				}
				return 1;
			}
		}
		return 0;
	case 0: return 0; /* i.e. an exploded wand */
	default: impossible("%s wanted to perform action %d?", Monnam(mtmp),
			m.has_misc);
		break;
	}
	return 0;
}

static void
you_aggravate(mtmp)
struct monst *mtmp;
{
	killer = ""; /* hack: prevent "it" in names */
/*JP	pline("For some reason, %s presence is known to you.",*/
	pline("なんらかの理由で，あなたは%sの存在に気がついた．",
		s_suffix(mon_nam(mtmp)));
	cls();
#ifdef CLIPPING
	cliparound(mtmp->mx, mtmp->my);
#endif
	show_glyph(mtmp->mx, mtmp->my, mon_to_glyph(mtmp));
	display_self();
/*JP	You_feel("aggravated at %s.", mon_nam(mtmp));*/
	You("%sに腹がたった．", mon_nam(mtmp));
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	if (unconscious()) {
		multi = -1;
		nomovemsg =
/*JP		      "Aggravated, you are jolted into full consciousness.";*/
		      "腹がたっていて，ピリピリしている．";
	}
	killer = 0;
	newsym(mtmp->mx,mtmp->my);
}

int
rnd_misc_item(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless(mtmp->data)
			|| pm->mlet == S_GHOST
# ifdef KOPS
			|| pm->mlet == S_KOP
# endif
		) return 0;
	/* Unlike other rnd_item functions, we only allow _weak_ monsters
	 * to have this item; after all, the item will be used to strengthen
	 * the monster and strong monsters won't use it at all...
	 */
	if (difficulty < 6 && !rn2(30)) return WAN_POLYMORPH;
	
	if (!rn2(40) && !nonliving(pm)) return AMULET_OF_LIFE_SAVING;

	switch (rn2(3)) {
		case 0:
			if (mtmp->isgd) return 0;
			return rn2(6) ? POT_SPEED : WAN_SPEED_MONSTER;
		case 1:
			if (mtmp->mpeaceful && !See_invisible) return 0;
			return rn2(6) ? POT_INVISIBILITY : WAN_MAKE_INVISIBLE;
		case 2:
			return POT_GAIN_LEVEL;
	}
	/*NOTREACHED*/
	return 0;
}

boolean
searches_for_item(mon, obj)
struct monst *mon;
struct obj *obj;
{
	int typ = obj->otyp;

	if (is_animal(mon->data) || mindless(mon->data)) return FALSE;
	/* No, the ghost doesn't pick up the bones pile */
	if (mon->data == &mons[PM_GHOST]) return FALSE;
	return((boolean)((obj->oclass == WAND_CLASS && objects[typ].oc_dir == RAY)
		|| typ == WAN_STRIKING
		|| (!mon->minvis && !mon->invis_blkd &&
			(typ == WAN_MAKE_INVISIBLE || typ == POT_INVISIBILITY))
		|| (mon->mspeed != MFAST &&
			(typ == WAN_SPEED_MONSTER || typ == POT_SPEED))
		|| typ == POT_HEALING
		|| typ == POT_EXTRA_HEALING
		|| typ == POT_GAIN_LEVEL
		|| (monstr[monsndx(mon->data)] < 6 && typ == WAN_POLYMORPH)
		|| (!is_floater(mon->data) && typ == WAN_DIGGING)
		|| typ == WAN_TELEPORTATION
		|| typ == SCR_TELEPORTATION
		|| typ == WAN_CREATE_MONSTER
		|| typ == SCR_CREATE_MONSTER
		|| typ == POT_PARALYSIS
		|| typ == POT_BLINDNESS
		|| typ == POT_CONFUSION
		|| (typ == PICK_AXE && needspick(mon->data))
		|| typ == AMULET_OF_REFLECTION
		|| (typ == AMULET_OF_LIFE_SAVING && !nonliving(mon->data))
		|| ((mon->misc_worn_check & W_ARMG) && typ == CORPSE
			&& obj->corpsenm == PM_COCKATRICE)
		|| (typ == CORPSE && !resists_ston(mon) &&
			(obj->corpsenm == PM_LIZARD ||
				acidic(&mons[obj->corpsenm])))
		|| (typ == UNICORN_HORN && !obj->cursed &&
			mon->data->mlet != S_UNICORN)
		|| typ == FROST_HORN
		|| typ == FIRE_HORN
	));
}

boolean
mon_reflects(mon,str)
struct monst *mon;
const char *str;
{
	struct obj *orefl = which_armor(mon, W_ARMS);

	if (orefl && orefl->otyp == SHIELD_OF_REFLECTION) {
	    if (str) {
/*JP		pline(str, s_suffix(mon_nam(mon)), "shield");*/
		pline(str, s_suffix(mon_nam(mon)), "盾");
		makeknown(SHIELD_OF_REFLECTION);
	    }
	    return TRUE;
	} else if ((orefl = which_armor(mon, W_AMUL)) &&
				orefl->otyp == AMULET_OF_REFLECTION) {
	    if (str) {
/*JP		pline(str, s_suffix(mon_nam(mon)), "amulet");*/
		pline(str, s_suffix(mon_nam(mon)), "魔除け");
		makeknown(AMULET_OF_REFLECTION);
	    }
	    return TRUE;
	}
	return FALSE;
}

/* TRUE if the monster ate something */
boolean
munstone(mon, by_you)
struct monst *mon;
boolean by_you;
{
	struct obj *obj;

	if (resists_ston(mon)) return FALSE;
	if (mon->meating || !mon->mcanmove || mon->msleep) return FALSE;

	for(obj = mon->minvent; obj; obj = obj->nobj) {
	    if (obj->otyp == CORPSE && (obj->corpsenm == PM_LIZARD ||
		    acidic(&mons[obj->corpsenm]))) {
		int nutrit = dog_nutrition(mon, obj); /* also sets meating */

		if (canseemon(mon)) {
		    long save_quan = obj->quan;

		    obj->quan = 1L;
/*JP		    pline("%s eats %s.", Monnam(mon), distant_name(obj,doname));*/
		    pline("%sは%sを食べた．", Monnam(mon), distant_name(obj,doname));
		    obj->quan = save_quan;
/*JP		} else if (flags.soundok) You("hear chewing.");*/
		} else if (flags.soundok) You_hear("クチャクチャという音を聞いた．");
		m_useup(mon, obj);
		if (acidic(&mons[obj->corpsenm]) && !resists_acid(mon)) {
		    mon->mhp -= rnd(15);
/*JP		    pline("%s has a very bad case of stomach acid.",*/
		    pline("%sは胃酸の調子がとても悪い．",
			Monnam(mon));
		}
		if (mon->mhp <= 0) {
/*JP		    pline("%s dies!", Monnam(mon));*/
		    pline("%sは死んだ！", Monnam(mon));
		    if (by_you) xkilled(mon, 0);
		    else mondead(mon);
		    return TRUE;
		}
		if (canseemon(mon)) {
		    if (Hallucination)
/*JP		pline("What a pity - %s just ruined a future piece of art!",*/
		pline("なんてことだ！%sは芸術作品になれたかもしれないのに！",
			mon_nam(mon));
		    else
			pline("%s seems limber!", Monnam(mon));
			pline("%sは体が柔らかくなったように見える！", Monnam(mon));
		}
		if (mon->mtame && !mon->isminion) {
		    struct edog *edog = EDOG(mon);

		    if (edog->hungrytime < moves) edog->hungrytime = moves;
		    edog->hungrytime += nutrit;
		    mon->mconf = 0;
		}
		mon->mlstmv = monstermoves; /* it takes a turn */
		return TRUE;
	    }
	}
	return FALSE;
}

/*muse.c*/
