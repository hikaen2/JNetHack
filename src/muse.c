/*	SCCS Id: @(#)muse.c	3.1	93/05/25	*/
/* Monster item usage routine.  Copyright (C) 1990 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details.  */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

#ifdef MUSE

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
static int FDECL(mbhitm, (struct monst *,struct obj *));
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
	boolean vis = cansee(mon->mx, mon->my);

	if (!obj) return 0;
	if (obj->oclass == POTION_CLASS) {
	    coord cc;
/*JP	    static const char *empty = "The potion turns out to be empty.";*/
	    static const char *empty = "薬は空っぽであることがわかった．";
	    const char * potion_descr ;
	    struct monst *mtmp;

	    potion_descr = OBJ_DESCR(objects[obj->otyp]);
	    if (potion_descr && !strcmp(potion_descr, "milky") && !rn2(13)) {
		if (!enexto(&cc, mon->mx, mon->my, &mons[PM_GHOST])) return 0;
		mquaffmsg(mon, obj);
		m_useup(mon, obj);
		mtmp = makemon(&mons[PM_GHOST], cc.x, cc.y);
		if (!mtmp) {
		    if (vis) pline(empty);
		} else {
		    if (vis) {
/*JP			pline("As %s opens the bottle, an enormous %s emerges!",*/
			pline("%sが瓶を開けると，巨大な%sが出てきた！",
			   mon_nam(mon),
			   Hallucination ? rndmonnam() : (const char *)"ghost");
/*JP			pline("%s is frightened to death, and unable to move.",*/
			pline("%sはまっさおになって驚き，動けなくなった．",
				Monnam(mon));
		    }
		    mon->mcanmove = 0;
		    mon->mfrozen = 3;
		}
		return 2;
	    }
	    if (potion_descr && !strcmp(potion_descr, "smoky") && !rn2(13)) {
		if (!enexto(&cc, mon->mx, mon->my, &mons[PM_DJINNI])) return 0;
		mquaffmsg(mon, obj);
		m_useup(mon, obj);
		mtmp = makemon(&mons[PM_DJINNI], cc.x, cc.y);
		if (!mtmp) {
		    if (vis) pline(empty);
		} else {
		    if (vis)
/*JP			pline("In a cloud of smoke, %s emerges!",*/
			pline("煙の中から，%sが現われた！",
							a_monnam(mtmp));
/*JP		    pline("%s speaks.", vis ? Monnam(mtmp) : "Something");*/
		    pline("%sは話した．", vis ? Monnam(mtmp) : "何物か");
		/* I suspect few players will be upset that monsters */
		/* can't wish for wands of death here.... */
		    if (rn2(2)) {
/*JP			verbalize("You freed me!");*/
			verbalize("おまえは私を解放してくれた！");
			mtmp->mpeaceful = 1;
			set_malign(mtmp);
		    } else {
/*JP			verbalize("It is about time.");*/
			verbalize("おっと時間だ！．");
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

#ifdef SOUNDS
	    if (flags.soundok)
#endif
				{
/*JP		if (vis) pline("%s zaps %s, which suddenly explodes!",*/
		if (vis) pline("%sは%sをふりかざした，それは突然爆発した！",
			Monnam(mon), an(xname(obj)));
/*JP		else You("hear a zap and an explosion in the distance.");*/
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
#ifdef SOUNDS
		if (flags.soundok)
#endif
/*JP			You("hear a distant zap.");*/
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
	boolean vismon = (canseemon(mtmp));
#ifdef SOUNDS
	if (flags.soundok)
#endif
		otmp->dknown = 1;
	if (!vismon) {
#ifdef SOUNDS
		if (flags.soundok)
#endif
/*JP		    You("hear %s reading %s.",*/
		    You("%sが%sを読んでいる声を聞いた．",
			an(Hallucination ? rndmonnam() : jtrns_mon(mtmp->data->mname)),
			singular(otmp, doname));
/*JP	} else pline("%s reads %s!", Monnam(mtmp), singular(otmp,doname));*/
	} else pline("%sは%sを読んだ！", Monnam(mtmp), singular(otmp,doname));
	if (mtmp->mconf
#ifdef SOUNDS
		&& (vismon || flags.soundok)
#endif
					)
/*JP		pline("Being confused, %s mispronounces the magic words...",
		      vismon ? mon_nam(mtmp) : he[pronoun_gender(mtmp)]);*/
		pline("混乱のため，%s%s魔法の言葉を間違って発音した．．．",
		      vismon ? mon_nam(mtmp) : "誰か",
		      vismon ? "は" : "が");
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
#ifdef SOUNDS
		if (flags.soundok)
#endif
/*JP			You("hear a chugging sound.");*/
			You("ゴクンゴクンと言う音を聞いた．");
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
#ifdef ARMY
# define MUSE_BUGLE 16
#endif
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
	register struct obj *obj;
	struct trap *t;
	int x=mtmp->mx, y=mtmp->my;
	boolean stuck = (mtmp == u.ustuck);
	boolean immobile = (mtmp->data->mmove == 0);
	int fraction;

	if (mtmp->mpeaceful || is_animal(mtmp->data) || mindless(mtmp->data))
		return 0;
	if (u.uswallow && stuck) return 0;
	if(dist2(x, y, mtmp->mux, mtmp->muy) > 25)
		return 0;

	fraction = u.ulevel < 10 ? 5 : u.ulevel < 14 ? 4 : 3;
	if(mtmp->mhp >= mtmp->mhpmax ||
			(mtmp->mhp >= 10 && mtmp->mhp*fraction >= mtmp->mhpmax))
		return 0;

	m.defensive = (struct obj *)0;
	m.has_defense = 0;

	if (levl[x][y].typ == STAIRS && !stuck && !immobile) {
		if (x == xdnstair && y == ydnstair)
			m.has_defense = MUSE_DOWNSTAIRS;
		if (x == xupstair && y == yupstair && ledger_no(&u.uz) != 1)
	/* Unfair to let the monsters leave the dungeon with the Amulet */
	/* (or go to the endlevel since you also need it, to get there) */
			m.has_defense = MUSE_UPSTAIRS;
	} else if (levl[x][y].typ == LADDER && !stuck && !immobile) {
		if (x == xupladder && y == yupladder)
			m.has_defense = MUSE_UP_LADDER;
		if (x == xdnladder && y == ydnladder)
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
		if ((t = t_at(xx,yy)) != 0) {
			if (t->ttyp == TRAPDOOR
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

#ifdef ARMY
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
#endif
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
	int i;
	struct obj *otmp = m.defensive;
	boolean vis = cansee(mtmp->mx, mtmp->my);
	boolean vismon = canseemon(mtmp);
	boolean oseen = otmp && vismon;

	if ((i = precheck(mtmp, otmp)) != 0) return i;
	switch(m.has_defense) {
#ifdef ARMY
	case MUSE_BUGLE:
		if (canseemon(mtmp))
/*JP			pline("%s plays %s!", Monnam(mtmp), doname(otmp));*/
			pline("%sは%sを吹いた！", Monnam(mtmp), doname(otmp));
		else if (flags.soundok)
/*JP			You("hear a bugle playing reveille!");*/
			You("起床ラッパの音を聞いた！");
		awaken_soldiers();
		return 2;
#endif
	case MUSE_WAN_TELEPORTATION_SELF:
		if ((mtmp->isshk && inhishop(mtmp)) 
		       || mtmp->isgd || mtmp->ispriest) return 2;
		mzapmsg(mtmp, otmp, TRUE);
		otmp->spe--;
		if (oseen) makeknown(WAN_TELEPORTATION);
mon_tele:
		if (tele_restrict(mtmp))
		    return 2;
		if((/*mon_has_amulet(mtmp)||*/ Is_wiz1_level(&u.uz) ||
		      Is_wiz2_level(&u.uz) || Is_wiz3_level(&u.uz))
								&& !rn2(3)) {
		    if (vismon)
/*JP			pline("%s seems disoriented for a moment.",*/
			pline("%sは一瞬方向感覚を失った．",
				Monnam(mtmp));
		    return 2;
		}
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
		mreadmsg(mtmp, otmp);
		m_useup(mtmp, otmp);	/* otmp might be free'ed */
		if (oseen) makeknown(SCR_TELEPORTATION);
		if (obj_is_cursed) {
			if (mon_has_amulet(mtmp) || In_endgame(&u.uz)) {
			    if (vismon)
/*JP				pline("%s seems very disoriented for a moment.",*/
				pline("%sは一瞬方向感覚を失った．",
					Monnam(mtmp));
			    return 2;
			}
			if (Is_botlevel(&u.uz)) goto mon_tele;
			else {
			    int dpth = depth(&u.uz);
			    int nlev = max(dpth, 0);
			    d_level flev;

			    if (rn2(5)) nlev = rnd(nlev + 3);
			    else {
/*JP				pline("%s shudders for a moment.",*/
				pline("%sは一瞬震えた．",
							Monnam(mtmp));
				return 2; 
			    }
			    if (nlev == depth(&u.uz)) {
				if (u.uz.dlevel == 1) nlev++;
				else if (In_hell(&u.uz)) nlev--;
				else nlev++;
			    }
			    get_level(&flev, nlev);
			    migrate_to_level(mtmp, ledger_no(&flev), 0);
			}
		} else goto mon_tele;
		return 2;
	    }
	case MUSE_WAN_DIGGING:
	    {	struct trap *ttmp;

		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		if (oseen) makeknown(WAN_DIGGING);
		if(IS_FURNITURE(levl[mtmp->mx][mtmp->my].typ)
		   || (sstairs.sx && sstairs.sx == mtmp->mx &&
				sstairs.sy == mtmp->my)) {
/*JP			pline("The digging ray is ineffective.");*/
			pline("掘削の光は効果がない．");
			return 2;
		}
		if (!Can_dig_down(&u.uz)) {
		    if(canseemon(mtmp))
/*JP			pline("The floor here is too hard to dig in.");*/
			pline("ここの床は固くて掘れない．");
		    return 2;
		}
		ttmp = maketrap(mtmp->mx, mtmp->my, TRAPDOOR);
		if (!ttmp) return 2;
		seetrap(ttmp);
		if (vis) {
/*JP			pline("%s's made a hole in the floor.", Monnam(mtmp));
			pline("%s falls through...", Monnam(mtmp));*/
			pline("%sは床に穴を開けた．", Monnam(mtmp));
			pline("%sは通り抜けて落ちた．．．", Monnam(mtmp));
		} else
# ifdef SOUNDS
			if (flags.soundok)
# endif
/*JP			You("hear something crash through the floor.");*/
			You("何かが床を通り抜けて落る音を聞いた．");
		/* we made sure that there is a level for mtmp to go to */
		migrate_to_level(mtmp, ledger_no(&u.uz)+1, 0);
		return 2;
	    }
	case MUSE_WAN_CREATE_MONSTER:
	    {	coord cc;
		struct permonst *pm=rndmonst();

		if (!enexto(&cc, mtmp->mx, mtmp->my, pm)) return 0;
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		if (oseen) makeknown(WAN_CREATE_MONSTER);
		(void) makemon(pm, cc.x, cc.y);
		return 2;
	    }
	case MUSE_SCR_CREATE_MONSTER:
	    {	coord cc;
		struct permonst *pm=rndmonst();
		int cnt = 1;

		if (!rn2(73)) cnt += rnd(4);
		if (mtmp->mconf || otmp->cursed) cnt += 12;
		mreadmsg(mtmp, otmp);
		while(cnt--) {
			struct monst *mon;

			if (!enexto(&cc, mtmp->mx, mtmp->my, pm)) continue;
			mon = makemon(mtmp->mconf ? &mons[PM_ACID_BLOB]
				: rndmonst(), cc.x, cc.y);
			if (mon) newsym(mon->mx,mon->my);
		}
		/* flush monsters before asking for identification */
		flush_screen(0);
		if (oseen && !objects[SCR_CREATE_MONSTER].oc_name_known
			  && !objects[SCR_CREATE_MONSTER].oc_uname)
			docall(otmp); /* not makeknown(); be consistent */
		m_useup(mtmp, otmp);
		return 2;
	    }
	case MUSE_TRAPDOOR:
		/* trapdoors on "bottom" levels of dungeons are rock-drop
		 * trapdoors, not holes in the floor.  We check here for
		 * safety.
		 */
		if (Is_botlevel(&u.uz)) return 0;
/*JP		if (vis) pline("%s %s into a trapdoor!", Monnam(mtmp),
			makeplural(locomotion(mtmp->data, "jump")));*/
		if (vis) pline("%sは罠の扉に%s入った", Monnam(mtmp),
			jconj(locomotion(mtmp->data, "飛ぶ"),"て"));
		seetrap(t_at(trapx,trapy));

		/*  don't use rloc_to() because worm tails must "move" */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);	/* update old location */
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);

		migrate_to_level(mtmp, ledger_no(&u.uz)+1, 0);
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
/*JP			if (vismon) pline("%s escapes the dungeon!",*/
			if (vismon) pline("%sは迷宮を抜けた！",
				Monnam(mtmp));
			mongone(mtmp);
			return 2;
		}
/*JP		if (vismon) pline("%s escapes upstairs!", Monnam(mtmp));*/
		if (vismon) pline("%sは階段をのぼって逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&u.uz)-1, 2);
		return 2;
	case MUSE_DOWNSTAIRS:
/*JP		if (vismon) pline("%s escapes downstairs!", Monnam(mtmp));*/
		if (vismon) pline("%sは階段を降りて逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&u.uz)+1, 1);
		return 2;
	case MUSE_UP_LADDER:
/*JP		if (vismon) pline("%s escapes up the ladder!", Monnam(mtmp));*/
		if (vismon) pline("%sは梯子をのぼって逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&u.uz)-1, 4);
		return 2;
	case MUSE_DN_LADDER:
/*JP		if (vismon) pline("%s escapes down the ladder!", Monnam(mtmp));*/
		if (vismon) pline("%sは梯子を降りて逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&u.uz)+1, 3);
		return 2;
	case MUSE_SSTAIRS:
		/* the stairs leading up from the 1st level are */
		/* regular stairs, not sstairs.			*/
		if (sstairs.up) {
			if (vismon)
/*JP			    pline("%s escapes upstairs!", Monnam(mtmp));*/
			    pline("%sはのぼり階段に逃げた！", Monnam(mtmp));
			if(Inhell) {
			    migrate_to_level(mtmp,
					 ledger_no(&sstairs.tolev), 0);
			    return 2;
			}
		} else	if (vismon)
/*JP		    pline("%s escapes downstairs!", Monnam(mtmp));*/
		    pline("%s降り階段に逃げた！", Monnam(mtmp));
		migrate_to_level(mtmp, ledger_no(&sstairs.tolev), 5);
		return 2;
	case MUSE_TELEPORT_TRAP:
/*JP		if (vis) pline("%s %s onto a teleport trap!", Monnam(mtmp),*/
		if (vis) pline("%sは瞬間移動の罠に%s入った！", Monnam(mtmp),
			jconj(locomotion(mtmp->data, "飛ぶ"),"て"));
		seetrap(t_at(trapx,trapy));

		/*  don't use rloc_to() because worm tails must "move" */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);	/* update old location */
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);

		goto mon_tele;
	case MUSE_POT_HEALING:
		mquaffmsg(mtmp, otmp);
		i = d(5,2) + 5 * !!bcsign(otmp);
		mtmp->mhp += i;
		if (mtmp->mhp > mtmp->mhpmax) mtmp->mhp = ++mtmp->mhpmax;
		if (!otmp->cursed) mtmp->mcansee = 1;
/*JP		if (vismon) pline("%s begins to look better.", Monnam(mtmp));*/
		if (vismon) pline("%sは気分がよくなったようだ．", Monnam(mtmp));
		if (oseen) makeknown(POT_HEALING);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_POT_EXTRA_HEALING:
		mquaffmsg(mtmp, otmp);
		i = d(5,4) + 5 * !!bcsign(otmp);
		mtmp->mhp += i;
		if (mtmp->mhp > mtmp->mhpmax)
			mtmp->mhp = (mtmp->mhpmax += (otmp->blessed ? 5 : 2));
		mtmp->mcansee = 1;
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

/* Select an offensive item/action for a monster.  Returns TRUE iff one is
 * found.
 */
boolean
find_offensive(mtmp)
struct monst *mtmp;
{
	register struct obj *obj;
	boolean ranged_stuff = lined_up(mtmp);

	m.offensive = (struct obj *)0;
	m.has_offense = 0;
	if (mtmp->mpeaceful || is_animal(mtmp->data) ||
				mindless(mtmp->data) || nohands(mtmp->data))
		return 0;
	if (u.uswallow) return 0;
	if (in_your_sanctuary(mtmp->mx, mtmp->my)) return 0;
	if (dmgtype(mtmp->data, AD_HEAL) && !uwep
#ifdef TOURIST
	    && !uarmu
#endif
	    && !uarm && !uarmh && !uarms && !uarmg && !uarmc && !uarmf)
		return 0;

	if (!ranged_stuff) return 0;
#define nomore(x) if(m.has_offense==x) continue;
	for(obj=mtmp->minvent; obj; obj=obj->nobj) {
		/* nomore(MUSE_WAN_DEATH); */
		if (m.has_defense == WAN_DEATH) break;
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
		nomore(MUSE_WAN_COLD);
		if(obj->otyp == WAN_COLD && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_COLD;
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
		if(obj->otyp == SCR_FIRE && resists_fire(mtmp->data)
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

static int
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
/*JP				pline("The wand hits you!");*/
				pline("杖はあなたに命中した！");
				tmp = d(2,12);
				if(Half_spell_damage) tmp = (tmp+1) / 2;
/*JP				losehp(tmp, "wand", KILLED_BY_AN);*/
				losehp(tmp, "衝撃の杖によって", KILLED_BY_AN);
/*JP			} else pline("The wand misses you.");*/
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
			case WAN_STRIKING:
			    if (doorlock(obj, bhitpos.x, bhitpos.y))
				makeknown(obj->otyp);
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
	boolean oseen = otmp && canseemon(mtmp);

	if ((i = precheck(mtmp, otmp)) != 0) return i;
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
/*JP			    pline("The scroll erupts in a tower of flame!");*/
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
				if (resists_fire(mtmp2->data)) continue;
				mtmp2->mhp -= num;
				if(resists_cold(mtmp2->data))
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
#endif
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
boolean
find_misc(mtmp)
struct monst *mtmp;
{
	register struct obj *obj;
	int x=mtmp->mx, y=mtmp->my;
#ifdef POLYSELF
	struct trap *t;
	int xx, yy;
	boolean immobile = (mtmp->data->mmove == 0);
#endif
	boolean stuck = (mtmp == u.ustuck);

	m.misc = (struct obj *)0;
	m.has_misc = 0;
	if (is_animal(mtmp->data) || mindless(mtmp->data))
		return 0;
	if (u.uswallow && stuck) return 0;

	/* We arbitrarily limit to times when a player is nearby for the
	 * same reason as Junior Pac-Man doesn't have energizers eaten until
	 * you can see them...
	 */
	if(dist2(x, y, mtmp->mux, mtmp->muy) > 36)
		return 0;

#ifdef POLYSELF
	if (!stuck && !immobile &&
			!mtmp->cham && monstr[(monsndx(mtmp->data))] < 6)
	  for(xx = x-1; xx <= x+1; xx++)
	    for(yy = y-1; yy <= y+1; yy++)
		if (isok(xx,yy) && (xx != u.ux || yy != u.uy))
		    if (mtmp->data != &mons[PM_GRID_BUG] || xx == x || yy == y)
			if (/* (xx==x && yy==y) || */ !level.monsters[xx][yy])
			    if ((t = t_at(xx,yy)) != 0) {
				if (t->ttyp == POLY_TRAP) {
				    trapx = xx;
				    trapy = yy;
				    m.has_misc = MUSE_POLY_TRAP;
				    return TRUE;
				}
			    }
#endif
	if (nohands(mtmp->data))
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
		/* Note: peaceful/tame monsters won't make themselves
		 * invisible unless you can see them.  Not really right, but...
		 */
		nomore(MUSE_WAN_MAKE_INVISIBLE);
		if(obj->otyp == WAN_MAKE_INVISIBLE && obj->spe > 0 &&
		     !mtmp->minvis && (!mtmp->mpeaceful || See_invisible)
			&& (mtmp->data != &mons[PM_MEDUSA] || mtmp->mcan)) {
			m.misc = obj;
			m.has_misc = MUSE_WAN_MAKE_INVISIBLE;
		}
		nomore(MUSE_POT_INVISIBILITY);
		if(obj->otyp == POT_INVISIBILITY &&
		     !mtmp->minvis && (!mtmp->mpeaceful || See_invisible)) {
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
		if(obj->otyp == WAN_POLYMORPH && !mtmp->cham
				&& monstr[(monsndx(mtmp->data))] < 6) {
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
	boolean vis = cansee(mtmp->mx, mtmp->my);
	boolean vismon = canseemon(mtmp);
	boolean oseen = otmp && vismon;

	if ((i = precheck(mtmp, otmp)) != 0) return i;
	switch(m.has_misc) {
	case MUSE_POT_GAIN_LEVEL:
		mquaffmsg(mtmp, otmp);
		if (otmp->cursed) {
		    if (Can_rise_up(&u.uz)) {
			register int tolev = depth(&u.uz)-1;
			d_level tolevel;

			get_level(&tolevel, tolev);
			/* insurance against future changes... */
			if(on_level(&tolevel, &u.uz)) goto skipmsg;
			if (vismon) {
/*JP			    pline("%s rises up, through the ceiling!",*/
			    pline("%sは天井を突き抜けた！",
				Monnam(mtmp));
			    if(!objects[POT_GAIN_LEVEL].oc_name_known
			      && !objects[POT_GAIN_LEVEL].oc_uname)
				docall(otmp);
			}
			m_useup(mtmp, otmp);
			migrate_to_level(mtmp, ledger_no(&tolevel), 0);
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
		mtmp->minvis = 1;
		newsym(mtmp->mx,mtmp->my);
		if (mtmp->wormno) see_wsegs(mtmp);
		return 2;
	case MUSE_POT_INVISIBILITY:
		mquaffmsg(mtmp, otmp);
/*JP		if (vis) pline("Gee, all of a sudden %s can't see %sself.",
			       mon_nam(mtmp), him[pronoun_gender(mtmp)]);*/
		if (vis) pline("ゲ！突然%sは見えなくなった．",
			       mon_nam(mtmp));
		if (oseen) makeknown(POT_INVISIBILITY);
		mtmp->minvis = 1;
		newsym(mtmp->mx,mtmp->my);
		if (mtmp->wormno) see_wsegs(mtmp);
		if (otmp->cursed) {
			mtmp->minvis = 0;
/*JP			pline("For some reason, %s presence is known to you.",*/
			pline("なんらかの理由で，あなたは%sの存在に気がついた．",
				s_suffix(mon_nam(mtmp)));
			you_aggravate(mtmp);
			mtmp->minvis = 1;
			newsym(mtmp->mx,mtmp->my);
		}
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
		if (vismon) pline("%sは突然早く動けるようになった．",
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
#ifdef POLYSELF
	case MUSE_POLY_TRAP:
		if (vismon)
/*JP		    pline("%s deliberately goes onto a polymorph trap!",*/
		    pline("%sはわざと変化の罠に飛びこんだ！",
			  Monnam(mtmp));
		seetrap(t_at(trapx,trapy));

		/*  don't use rloc() due to worms */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);

		(void) newcham(mtmp, (struct permonst *)0);
		return 2;
#endif
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
	cls();
	show_glyph(mtmp->mx, mtmp->my, mon_to_glyph(mtmp));
	display_self();
/*JP	You("feel aggravated at %s.", mon_nam(mtmp));*/
	You("%sに腹がたった．", mon_nam(mtmp));
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	if (unconscious()) {
		multi = -1;
		nomovemsg =
/*JP		      "Aggravated, you are jolted into full consciousness.";*/
		      "腹がたっていて，ピリピリしている．";
	}
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
	
	if (!rn2(40)) return AMULET_OF_LIFE_SAVING;

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
	return((boolean)((obj->oclass == WAND_CLASS && objects[typ].oc_dir == RAY)
		|| typ == WAN_STRIKING
		|| (!mon->minvis &&
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
		|| typ == AMULET_OF_LIFE_SAVING
		|| ((mon->misc_worn_check & W_ARMG) && typ == CORPSE
			&& obj->corpsenm == PM_COCKATRICE)
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

#endif	/* MUSE */

/*muse.c*/
