/*	SCCS Id: @(#)trap.c	3.2	96/10/21	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

STATIC_DCL void FDECL(dofiretrap, (struct obj *));
STATIC_DCL void NDECL(domagictrap);
STATIC_DCL boolean FDECL(emergency_disrobe,(boolean *));
STATIC_DCL int FDECL(untrap_prob, (struct trap *ttmp));
STATIC_DCL void FDECL(cnv_trap_obj, (int, int, struct trap *));
STATIC_DCL void FDECL(move_into_trap, (struct trap *));
STATIC_DCL int FDECL(try_disarm, (struct trap *,BOOLEAN_P));
STATIC_DCL void FDECL(reward_untrap, (struct trap *, struct monst *));
STATIC_DCL int FDECL(disarm_beartrap, (struct trap *));
STATIC_DCL int FDECL(disarm_landmine, (struct trap *));
STATIC_DCL int FDECL(disarm_squeaky_board, (struct trap *));
STATIC_DCL int FDECL(disarm_shooting_trap, (struct trap *, int));
STATIC_DCL int FDECL(try_lift, (struct monst *, struct trap *, int, BOOLEAN_P));
STATIC_DCL int FDECL(help_monster_out, (struct monst *, struct trap *));
STATIC_DCL boolean FDECL(thitm, (int, struct monst *, struct obj *, int));
STATIC_DCL int FDECL(mkroll_launch,
			(struct trap *,XCHAR_P,XCHAR_P,SHORT_P,long));
STATIC_DCL boolean FDECL(isclearpath,(coord *, int, SCHAR_P, SCHAR_P));
STATIC_DCL void FDECL(blow_up_landmine, (struct trap *));

#ifndef OVLB
STATIC_VAR const char *a_your[2];
STATIC_VAR const char *A_Your[2];
STATIC_VAR const char *the_your[2];
STATIC_VAR const char tower_of_flame[];
STATIC_VAR const char *A_gush_of_water_hits;
STATIC_VAR const char *set_you[2];
STATIC_VAR const char *dig_you[2];
STATIC_VAR const char *web_you[2];

#else

#if 0 /*JP*/
STATIC_VAR const char *a_your[2] = { "a", "your" };
STATIC_VAR const char *A_Your[2] = { "A", "Your" };
STATIC_VAR const char *the_your[2] = { "the", "your" };
STATIC_VAR const char tower_of_flame[] = "tower of flame";
STATIC_VAR const char *A_gush_of_water_hits = "A gush of water hits";
#endif /*JP*/
STATIC_VAR const char *set_you[2] = { "", "あなたの仕掛けた" };
STATIC_VAR const char *dig_you[2] = { "", "あなたが掘った" };
STATIC_VAR const char *web_you[2] = { "", "あなたが張った" };

#endif /* OVLB */

#ifdef OVLB

/* called when you're hit by fire (dofiretrap,buzz,zapyourself,explode) */
boolean			/* returns TRUE if hit on torso */
burnarmor()
{
#define burn_dmg(obj,descr) rust_dmg(obj, descr, 0, FALSE)
    while (1) {
	switch (rn2(5)) {
	case 0:
/*JP	    if (!burn_dmg(uarmh, "leather helmet")) continue;*/
	    if (!burn_dmg(uarmh, "革の兜")) continue;
	    break;
	case 1:
	    if (uarmc)
/*JP		(void) burn_dmg(uarmc, "cloak");*/
		(void) burn_dmg(uarmc, "クローク");
	    else if (uarm)
		(void) burn_dmg(uarm, xname(uarm));
#ifdef TOURIST
	    else if (uarmu)
/*JP		(void) burn_dmg(uarmu, "shirt");*/
		(void) burn_dmg(uarmu, "シャツ");
#endif
	    return TRUE;
	case 2:
/*JP	    if (!burn_dmg(uarms, "wooden shield")) continue;*/
	    if (!burn_dmg(uarms, "木の盾")) continue;
	    break;
	case 3:
/*JP	    if (!burn_dmg(uarmg, "gloves")) continue;*/
	    if (!burn_dmg(uarmg, "小手")) continue;
	    break;
	case 4:
/*JP	    if (!burn_dmg(uarmf, "boots")) continue;*/
	    if (!burn_dmg(uarmf, "靴")) continue;
	    break;
	}
	break; /* Out of while loop */
    }
    return FALSE;
#undef burn_dmg
}

/* Generic rust-armor function.  Returns TRUE if a message was printed;
 * "print", if set, means to print a message (and thus to return TRUE) even
 * if the item could not be rusted; otherwise a message is printed and TRUE is
 * returned only for rustable items.
 */
boolean
rust_dmg(otmp, ostr, type, print)
register struct obj *otmp;
register const char *ostr;
int type;
boolean print;
{
#if 0 /*JP*/
	static NEARDATA const char *action[] = { "smoulder", "rust", "rot", "corrode" };
	static NEARDATA const char *msg[] =  { "burnt", "rusted", "rotten", "corroded" };
#endif
	static NEARDATA const char *action[] = { "くすぶった","錆びた","腐った","腐食した" };
	static NEARDATA const char *msg[] =  { "焦げた","錆びた","腐った","腐食した" };
	boolean vulnerable = FALSE;
	boolean plural;
	boolean grprot = FALSE;

	if (!otmp) return(FALSE);
	switch(type) {
		case 0:
		case 2: vulnerable = is_flammable(otmp); break;
		case 1: vulnerable = is_rustprone(otmp); grprot = TRUE; break;
		case 3: vulnerable = is_corrodeable(otmp); grprot = TRUE; break;
	}

	if (!print && (!vulnerable || otmp->oerodeproof || otmp->oeroded == MAX_ERODE))
		return FALSE;

	plural = is_gloves(otmp) || is_boots(otmp);

	if (!vulnerable) {
		if (flags.verbose)
/*JP		    Your("%s %s not affected.", ostr, plural ? "are" : "is");*/
		    Your("%sは影響を受けなかった．",ostr);
	} else if (otmp->oeroded < MAX_ERODE) {
		if (grprot && otmp->greased) {
			grease_protect(otmp,ostr,plural);
		} else if (otmp->oerodeproof || (otmp->blessed && !rnl(4))) {
			if (flags.verbose)
/*JP				pline("Somehow, your %s %s not affected.",
					ostr, plural ? "are" : "is");*/
				pline("何故か，%sは影響を受けなかった．",ostr);
		} else {
/*JP			Your("%s %s%s%s!", ostr, action[type],
				plural ? "" : "s",
				otmp->oeroded+1 == MAX_ERODE ? " completely" :
				otmp->oeroded ? " further" : "");*/
			Your("%sは%s%s！", ostr, 
			        otmp->oeroded+1 == MAX_ERODE ? "完全に" :
				otmp->oeroded ? "さらに" : "",
			        action[type]);
			otmp->oeroded++;
		}
	} else {
		if (flags.verbose)
/*JP			Your("%s %s%s completely %s.", ostr,
			     Blind ? "feel" : "look",
			     plural ? "" : "s", msg[type]);*/
			Your("%sは完全に%s%s.", ostr,msg[type],
			     Blind ? "ようだ" : "" );
	}
	return(TRUE);
}

void
grease_protect(otmp,ostr,plu)
register struct obj *otmp;
register const char *ostr;
register boolean plu;
{
/*JP	static const char txt[] = "protected by the layer of grease!";*/
	static const char txt[] = "油の塗りこみによって守られている！";

	if (ostr)
/*JP		Your("%s %s %s",ostr,plu ? "are" : "is",txt);*/
		Your("%sは%s",ostr,txt);
	else
/*JP		Your("%s %s",aobjnam(otmp,"are"),txt);*/
		Your("%sは%s",xname(otmp),txt);
	if (!rn2(2)) {
/*JP		pline("The grease dissolves.");*/
		pline("油は溶けてしまった．");
		otmp->greased = 0;
	}
}

struct trap *
maketrap(x,y,typ)
register int x, y, typ;
{
	register struct trap *ttmp;
	register struct rm *lev;
	register boolean oldplace;

	if ((ttmp = t_at(x,y)) != 0) {
	    if (ttmp->ttyp == MAGIC_PORTAL) return (struct trap *)0;
	    oldplace = TRUE;
	    if (u.utrap && (x == u.ux) && (y == u.uy) &&
	      ((u.utraptype == TT_BEARTRAP && typ != BEAR_TRAP) ||
	      (u.utraptype == TT_WEB && typ != WEB) ||
	      (u.utraptype == TT_PIT && typ != PIT && typ != SPIKED_PIT)))
		    u.utrap = 0;
	} else {
	    oldplace = FALSE;
	    ttmp = newtrap();
	    ttmp->tx = x;
	    ttmp->ty = y;
	    ttmp->launch.x = -1;	/* force error if used before set */
	    ttmp->launch.y = -1;
	}
	ttmp->ttyp = typ;
	switch(typ) {
	    case STATUE_TRAP:	    /* create a "living" statue */
	      { struct monst *mtmp;
		struct obj *otmp, *statue;

		statue = mkcorpstat(STATUE, (struct monst *)0,
					&mons[rndmonnum()], x, y, FALSE);
		mtmp = makemon(&mons[statue->corpsenm], 0, 0, NO_MM_FLAGS);
		if (!mtmp) break; /* should never happen */
		while(mtmp->minvent) {
		    otmp = mtmp->minvent;
		    otmp->owornmask = 0;
		    obj_extract_self(otmp);
		    add_to_container(statue, otmp);
		}
		mongone(mtmp);
		break;
	      }
	    case ROLLING_BOULDER_TRAP:	/* boulder will roll towards trigger */
		(void) mkroll_launch(ttmp, x, y, BOULDER, 1L);
		break;
	    case HOLE:
	    case PIT:
	    case SPIKED_PIT:
	    case TRAPDOOR:
		lev = &levl[x][y];
		if (*in_rooms(x, y, SHOPBASE) &&
			((typ == HOLE || typ == TRAPDOOR) || IS_DOOR(lev->typ)))
		    add_damage(x, y,		/* schedule repair */
			(IS_DOOR(lev->typ) && !flags.mon_moving) ? 200L : 0L);
		lev->doormask = 0;	/* subsumes altarmask, icedpool... */
		if (IS_ROOM(lev->typ)) /* && !IS_AIR(lev->typ) */
		    lev->typ = ROOM;

		/*
		 * some cases which can happen when digging
		 * down while phazing thru solid areas
		 */
		else if (lev->typ == STONE || lev->typ == SCORR)
		    lev->typ = CORR;
		else if (IS_WALL(lev->typ) || lev->typ == SDOOR)
		    lev->typ = level.flags.is_maze_lev ? ROOM :
			       level.flags.is_cavernous_lev ? CORR : DOOR;

		unearth_objs(x, y);
		break;
	}
	if (ttmp->ttyp == HOLE) ttmp->tseen = 1;  /* You can't hide a hole */
	else ttmp->tseen = 0;
	ttmp->once = 0;
	ttmp->madeby_u = 0;
	ttmp->dst.dnum = -1;
	ttmp->dst.dlevel = -1;
	if (!oldplace) {
	    ttmp->ntrap = ftrap;
	    ftrap = ttmp;
	}
	return(ttmp);
}

void
fall_through(td)
boolean td;	/* td == TRUE : trapdoor or hole */
{
	d_level dtmp;
	char msgbuf[BUFSZ];
	const char *dont_fall = 0;
	register int newlevel = dunlev(&u.uz);

	if(Blind && Levitation) return;

	do {
	    newlevel++;
	} while(!rn2(4) && newlevel < dunlevs_in_dungeon(&u.uz));

	if(td) {
		struct trap *t=t_at(u.ux,u.uy);
		if (t->ttyp == TRAPDOOR)
/*JP			pline("A trap door opens up under you!");*/
			pline("落し扉があなたの足元に開いた！");
		else
/*JP			pline("There's a gaping hole under you!");*/
			pline("あなたの足下にぽっかりと穴が開いている！");
/*JP	} else pline_The("%s opens up under you!", surface(u.ux,u.uy));*/
	} else pline("足元の%sに穴が開いた！", surface(u.ux,u.uy));

	if(Levitation || u.ustuck || !Can_fall_thru(&u.uz)
	   || is_flyer(uasmon) || is_clinger(uasmon)
	   || (Inhell && !u.uevent.invoked &&
					newlevel == dunlevs_in_dungeon(&u.uz))
		) {
/*JP	    dont_fall = "don't fall in.";*/
	    dont_fall = "しかしあなたは落ちなかった．";
	} else if (uasmon->msize >= MZ_HUGE) {
/*JP	    dont_fall = "don't fit through.";*/
	    dont_fall = "通り抜けるにはサイズが合わない．";
	} else if (!next_to_u()) {
/*JP	    dont_fall = "are jerked back by your pet!";*/
	    dont_fall = "あなたはペットによって引っぱられた！";
	}
	if (dont_fall) {
/*JP	    You(dont_fall);*/
	    pline(dont_fall);
	    /* hero didn't fall through, but any objects here might */
	    impact_drop((struct obj *)0, u.ux, u.uy, 0);
	    if (!td) {
		display_nhwindow(WIN_MESSAGE, FALSE);
/*JP		pline_The("opening under you closes up.");*/
		pline_The("足下に開いていたものは閉じた．");
	    }
	    return;
	}

	if(*u.ushops) shopdig(1);
	if (Is_stronghold(&u.uz)) {
	    find_hell(&dtmp);
	} else {
	    dtmp.dnum = u.uz.dnum;
	    dtmp.dlevel = newlevel;
	}
	if (!td)
/*JP	    Sprintf(msgbuf, "The hole in the %s above you closes up.",*/
	    Sprintf(msgbuf, "%sに開いた穴は閉じた．",
		    ceiling(u.ux,u.uy));
	schedule_goto(&dtmp, FALSE, TRUE, 0,
		      (char *)0, !td ? msgbuf : (char *)0);
}

/* you've either stepped onto a statue trap's location
   or you've triggered a statue trap by searching next to it
   or by trying to break it with a wand or pick-axe */
struct monst *
activate_statue_trap(trap, x, y, shatter)
struct trap *trap;
xchar x, y;
boolean shatter;
{
	struct monst *mtmp = 0;
	struct permonst *mptr = 0;
	struct obj *item, *otmp = sobj_at(STATUE, x, y);

	/* Guard against someone wishing for a statue of a unique monster
	   (which is allowed in normal play) and then tossing it onto the
	   [detected or guessed] location of a statue trap.  Normally the
	   uppermost statue is the one which would be activated. */
	while (otmp) {
	    mptr = &mons[otmp->corpsenm];
	    if (!(mptr->geno & G_UNIQ)) break;	/* not unique => use it */
	    while ((otmp = otmp->nexthere) != 0)
		if (otmp->otyp == STATUE) break;
	}
	deltrap(trap);
	if (otmp && (mtmp = makemon(mptr, x, y, NO_MINVENT)) != 0) {
	    /* if statue has been named, give same name to the monster */
	    if (otmp->onamelth)
		mtmp = christen_monst(mtmp, ONAME(otmp));
	    /* transfer any statue contents to monster's inventory */
	    while ((item = otmp->cobj) != 0) {
		obj_extract_self(item);
		add_to_minv(mtmp, item);
	    }
	    m_dowear(mtmp, TRUE);
	    delobj(otmp);
	    /* mimic statue becomes seen mimic; other hiders won't be hidden */
	    if (mtmp->m_ap_type) seemimic(mtmp);
	    else mtmp->mundetected = FALSE;
	    if (x == u.ux && y == u.uy)
/*JP
		pline_The("statue comes to life!");
*/
	        pline("彫像は生命を帯びた！");
	    else if (shatter)
/*JP
		pline("Instead of shattering, the statue suddenly comes alive!");
*/
		pline("砕けるかわりに，彫像は生命を帯びた！");
	    else
/*JP
		You("find %s posing as a statue.", a_monnam(mtmp));
*/
		pline("%sが彫像のふりをしているのを見つけた．", a_monnam(mtmp));
	    /* avoid hiding under nothing */
	    if (x == u.ux && y == u.uy &&
		    Upolyd && hides_under(uasmon) && !OBJ_AT(x, y))
		u.uundetected = 0;
	}
	if (Blind) feel_location(x, y);
	else newsym(x, y);
	return mtmp;
}

void
dotrap(trap)
register struct trap *trap;
{
	register int ttype = trap->ttyp;
	register struct monst *mtmp;
	register struct obj *otmp;
	boolean already_seen = trap->tseen;

	nomul(0);
	if(already_seen) {
	    if ((Levitation || is_flyer(uasmon)) &&
		    (ttype == PIT || ttype == SPIKED_PIT || ttype == HOLE ||
		    ttype == BEAR_TRAP)) {
#if 0 /*JP*/
		You("%s over %s %s.",
		    Levitation ? "float" : "fly",
		    a_your[trap->madeby_u],
		    defsyms[trap_to_defsym(ttype)].explanation);
#endif
		You("%s%sの上%s．",
		    set_you[trap->madeby_u],
		    jtrns_obj('^', defsyms[trap_to_defsym(ttype)].explanation),
		    Levitation ? "を見下ろした" : "を飛んでいる");
		return;
	    }
	    if(!Fumbling && ttype != MAGIC_PORTAL && ttype != ANTI_MAGIC &&
		(!rn2(5) ||
	    ((ttype == PIT || ttype == SPIKED_PIT) && is_clinger(uasmon)))) {
#if 0 /*JP*/
		You("escape %s %s.",
		    (ttype == ARROW_TRAP && !trap->madeby_u) ? "an" :
			a_your[trap->madeby_u],
		    defsyms[trap_to_defsym(ttype)].explanation);
#endif
		You("%s%sをするりと避けた．",
		    set_you[trap->madeby_u],
		    jtrns_obj('^', defsyms[trap_to_defsym(ttype)].explanation));
		return;
	    }
	}
		
	switch(ttype) {
	    case ARROW_TRAP:
		seetrap(trap);
/*JP		pline("An arrow shoots out at you!");*/
		pline("矢が飛んできた！");
		otmp = mksobj(ARROW, TRUE, FALSE);
		otmp->quan = 1L;
		otmp->owt = weight(otmp);
		if (thitu(8, dmgval(otmp, &youmonst), otmp, "矢")) {
		    obfree(otmp, (struct obj *)0);
		} else {
		    place_object(otmp, u.ux, u.uy);
		    if (!Blind) otmp->dknown = 1;
		    stackobj(otmp);
		    newsym(u.ux, u.uy);
		}
		break;
	    case DART_TRAP:
		seetrap(trap);
/*JP		pline("A little dart shoots out at you!");*/
		pline("小さな投げ矢があなたに飛んできた！");
		otmp = mksobj(DART, TRUE, FALSE);
		otmp->quan = 1L;
		otmp->owt = weight(otmp);
		if (!rn2(6)) otmp->opoisoned = 1;
/*JP		if (thitu(7, dmgval(otmp, &youmonst), otmp, "little dart")) {*/
		if (thitu(7, dmgval(otmp, &youmonst), otmp, "投げ矢")) {
		    if (otmp->opoisoned)
/*JP			poisoned("dart",A_CON,"poison dart",10);*/
			poisoned("投げ矢",A_CON,"毒矢",10);
		    obfree(otmp, (struct obj *)0);
		} else {
		    place_object(otmp, u.ux, u.uy);
		    if (!Blind) otmp->dknown = 1;
		    stackobj(otmp);
		    newsym(u.ux, u.uy);
		}
		break;
	    case ROCKTRAP:
		{
		    int dmg = d(2,6); /* should be std ROCK dmg? */

		    seetrap(trap);
		    otmp = mksobj_at(ROCK, u.ux, u.uy, TRUE);
		    otmp->quan = 1L;
		    otmp->owt = weight(otmp);

/*JP    pline("A trap door in the %s opens and a rock falls on your %s!",*/
		    pline("落し扉が%sに開き，石があなたの%sに落ちてきた！",
			    ceiling(u.ux,u.uy),
			    body_part(HEAD));

		    if (uarmh) {
			if(is_metallic(uarmh)) {
/*JP			    pline("Fortunately, you are wearing a hard helmet.");*/
			    pline("幸運にも，あなたは固い兜を身につけていた．");
			    dmg = 2;
			} else if (flags.verbose) {
/*JP			    Your("%s does not protect you.", xname(uarmh));*/
			    Your("%sでは防げない．", xname(uarmh));
			}
		    }

		    if (!Blind) otmp->dknown = 1;
		    stackobj(otmp);
		    newsym(u.ux,u.uy);	/* map the rock */

/*JP		    losehp(dmg, "falling rock", KILLED_BY_AN);*/
		    losehp(dmg, "落岩で", KILLED_BY_AN);
		    exercise(A_STR, FALSE);
		}
		break;

	    case SQKY_BOARD:	    /* stepped on a squeaky board */
		if (Levitation || is_flyer(uasmon)) {
		    if (!Blind) {
			seetrap(trap);
			if (Hallucination)
/*JP				You("notice a crease in the linoleum.");*/
				You("床の仕上材のしわに気がついた．");
			else
/*JP				You("notice a loose board below you.");*/
				You("足元の緩んだ板に気がついた．");
		    }
		} else {
		    seetrap(trap);
/*JP		    pline("A board beneath you squeaks loudly.");*/
		    pline("足元の板が大きくきしんだ．");
		    wake_nearby();
		}
		break;

	    case BEAR_TRAP:
		if(Levitation || is_flyer(uasmon)) break;
		seetrap(trap);
		if(amorphous(uasmon) || is_whirly(uasmon) ||
						    unsolid(uasmon)) {
/*JP		    pline("%s bear trap closes harmlessly through you.",
			    A_Your[trap->madeby_u]);*/
		    pline("%s熊の罠は噛みついたが，するっと通り抜けた．",
			    set_you[trap->madeby_u]);
		    break;
		}
		if(uasmon->msize <= MZ_SMALL) {
/*JP		    pline("%s bear trap closes harmlessly over you.",
			    A_Your[trap->madeby_u]);*/
		    pline("%s熊の罠は遥か上方で噛みついた．",
			    set_you[trap->madeby_u]);
		    break;
		}
		u.utrap = rn1(4, 4);
		u.utraptype = TT_BEARTRAP;
/*JP		pline("%s bear trap closes on your %s!",
			    A_Your[trap->madeby_u], body_part(FOOT));*/
		pline("%s熊の罠があなたの%sに噛みついた！",
			    set_you[trap->madeby_u], body_part(FOOT));
		if(u.umonnum == PM_OWLBEAR || u.umonnum == PM_BUGBEAR)
/*JP		    You("howl in anger!");*/
		    You("怒りの咆哮をあげた！");
		exercise(A_DEX, FALSE);
		break;

	    case SLP_GAS_TRAP:
		seetrap(trap);
		if(Sleep_resistance) {
/*JP		    You("are enveloped in a cloud of gas!");*/
		    You("ガス雲につつまれた！");
		    break;
		}
/*JP		pline("A cloud of gas puts you to sleep!");*/
		pline("あなたはガス雲で眠ってしまった！");
		flags.soundok = 0;
		fall_asleep(-rnd(25), TRUE);
		afternmv = Hear_again;
		break;

	    case RUST_TRAP:
		seetrap(trap);
		if (u.umonnum == PM_IRON_GOLEM) {
/*JP		    pline("%s you!", A_gush_of_water_hits);*/
		    pline("水が噴出してあなたに命中した！");
/*JP		    You("are covered with rust!");*/
		    You("錆に覆われた！");
		    rehumanize();
		    break;
		} else if (u.umonnum == PM_GREMLIN && rn2(3)) {
/*JP		    pline("%s you!", A_gush_of_water_hits);*/
		    pline("水が噴出してあなたに命中した！");
		    if ((mtmp = cloneu()) != 0) {
			mtmp->mhpmax = (u.mhmax /= 2);
/*JP			You("multiply.");*/
			You("分裂した．");
		    }
		    break;
		}

	    /* Unlike monsters, traps cannot aim their rust attacks at
	     * you, so instead of looping through and taking either the
	     * first rustable one or the body, we take whatever we get,
	     * even if it is not rustable.
	     */
		switch (rn2(5)) {
		    case 0:
/*JP			pline("%s you on the %s!", A_gush_of_water_hits,*/
			pline("水が噴出してあなたの%sに命中した！",
				    body_part(HEAD));
/*JP			(void) rust_dmg(uarmh, "helmet", 1, TRUE);*/
			(void) rust_dmg(uarmh, "兜", 1, TRUE);
			break;
		    case 1:
/*JP			pline("%s your left %s!", A_gush_of_water_hits,*/
			pline("水が噴出してあなたの左%sに命中した！",
				    body_part(ARM));
/*JP			if (rust_dmg(uarms, "shield", 1, TRUE)) break;*/
			if (rust_dmg(uarms, "盾", 1, TRUE)) break;
			if (uwep && bimanual(uwep))
			    goto two_hand;
			/* Two goto statements in a row--aaarrrgggh! */
/*JPglovecheck:		    (void) rust_dmg(uarmg, "gauntlets", 1, TRUE);*/
glovecheck:		    (void) rust_dmg(uarmg, "小手", 1, TRUE);
			/* Not "metal gauntlets" since it gets called
			 * even if it's leather for the message
			 */
			break;
		    case 2:
/*JP			pline("%s your right %s!", A_gush_of_water_hits,*/
			pline("水が噴出してあなたの右%sに命中した！",
				    body_part(ARM));
two_hand:		    erode_weapon(FALSE);
			goto glovecheck;
		    default:
/*JP			pline("%s you!", A_gush_of_water_hits);*/
			pline("水が噴出してあなたに命中した！");
			for (otmp=invent; otmp; otmp = otmp->nobj)
				    (void) snuff_lit(otmp);
/*JP			if (uarmc) (void) rust_dmg(uarmc, "cloak", 1, TRUE);*/
			if (uarmc) (void) rust_dmg(uarmc, "クローク", 1, TRUE);
			else if (uarm)
/*JP			    (void) rust_dmg(uarm, "armor", 1, TRUE);*/
			    (void) rust_dmg(uarm, "鎧", 1, TRUE);
#ifdef TOURIST
			else if (uarmu)
/*JP			    (void) rust_dmg(uarmu, "shirt", 1, TRUE);*/
			    (void) rust_dmg(uarmu, "シャツ", 1, TRUE);
#endif
		}
		break;

	    case FIRE_TRAP:
		seetrap(trap);
		dofiretrap((struct obj *)0);
		break;

	    case PIT:
	    case SPIKED_PIT:
		if (Levitation || is_flyer(uasmon)) break;
		seetrap(trap);
		if (is_clinger(uasmon)) {
		    if(trap->tseen) {
/*JP			You("see %s %spit below you.", a_your[trap->madeby_u],
			    ttype == SPIKED_PIT ? "spiked " : "");*/
			pline("足元に%s%s落し穴を発見した．", 
			    dig_you[trap->madeby_u],
			    ttype == SPIKED_PIT ? "トゲだらけの" : "");
		    } else {
/*JP			pline("%s pit %sopens up under you!",
			    A_Your[trap->madeby_u],
			    ttype == SPIKED_PIT ? "full of spikes " : "");*/
			    pline("%s落し穴が足元に開いた！",
				  dig_you[trap->madeby_u]);
/*JP			You("don't fall in!");*/
			pline("しかし，あなたは落ちなかった！");
		    }
		    break;
		}
/*JP		You("fall into %s pit!", a_your[trap->madeby_u]);*/
		You("%s落し穴に落ちた！", dig_you[trap->madeby_u]); 
		if (ttype == SPIKED_PIT)
/*JP		    You("land on a set of sharp iron spikes!");*/
		    You("鋭い鉄のトゲトゲの上に落ちた！");
		if (!passes_walls(uasmon))
		    u.utrap = rn1(6,2);
		u.utraptype = TT_PIT;
		if (ttype == SPIKED_PIT) {
/*JP		    losehp(rnd(10),"fell into a pit of iron spikes",*/
		    losehp(rnd(10),"トゲだらけの落し穴に落ちて",
/*JP			NO_KILLER_PREFIX);*/
			KILLED_BY);
		    if (!rn2(6))
/*JP			poisoned("spikes", A_STR, "fall onto poison spikes", 8);*/
			poisoned("トゲ", A_STR, "落し穴のトゲ", 8);
		} else
/*JP		    losehp(rnd(6),"fell into a pit", NO_KILLER_PREFIX);*/
		    losehp(rnd(6),"落し穴に落ちて", KILLED_BY);
		if (Punished && !carried(uball)) {
		    unplacebc();
		    ballfall();
		    placebc();
		}
/*JP		selftouch("Falling, you");*/
		selftouch("落下中あなたは");
		vision_full_recalc = 1;	/* vision limits change */
		exercise(A_STR, FALSE);
		exercise(A_DEX, FALSE);
		break;
	    case HOLE:
	    case TRAPDOOR:
		seetrap(trap);
		if (!Can_fall_thru(&u.uz)) {
		    impossible("dotrap: %ss cannot exist on this level.",
			       defsyms[trap_to_defsym(ttype)].explanation);
		    break;		/* don't activate it after all */
		}
		fall_through(TRUE);
		break;

	    case TELEP_TRAP:
		seetrap(trap);
		tele_trap(trap);
		break;
	    case LEVEL_TELEP:
		seetrap(trap);
		level_tele_trap(trap);
		break;

	    case WEB: /* Our luckless player has stumbled into a web. */
		seetrap(trap);
		if (amorphous(uasmon) || is_whirly(uasmon) ||
						    unsolid(uasmon)) {
		    if (acidic(uasmon) || u.umonnum == PM_GELATINOUS_CUBE ||
			u.umonnum == PM_FIRE_ELEMENTAL) {
/*JP			You("%s %s spider web!",
			    (u.umonnum == PM_FIRE_ELEMENTAL) ? "burn" : "dissolve",
			    a_your[trap->madeby_u]);*/
			You("%s蜘蛛の巣を%s！",
			    web_you[trap->madeby_u],
			    (u.umonnum == PM_FIRE_ELEMENTAL) ? "焼いた" : "こなごなにした");
			deltrap(trap);
			newsym(u.ux,u.uy);
			break;
		    }
/*JP		    You("flow through %s spider web.",
			    a_your[trap->madeby_u]);*/
		    You("%s蜘蛛の巣をするりと通り抜けた．",
			    web_you[trap->madeby_u]);
		    break;
		}
		if (uasmon->mlet == S_SPIDER) {
/*JP		    pline(trap->madeby_u ? "You take a walk on your web."
					 : "There is a spider web here.");*/
		    pline(trap->madeby_u ? 
			  "自分で張った蜘蛛の巣の上を歩いた．" : 
			  "ここには蜘蛛の巣がある");
		    break;
		}
/*JP		You("%s into %s spider web!",
		      Levitation ? (const char *)"float" :
		      locomotion(uasmon, "stumble"),
		      a_your[trap->madeby_u]);*/
		You("%s%s蜘蛛の巣にひっかかった",
		    Levitation ? (const char *)"浮きながら" : 
		    jconj(locomotion(uasmon, "つまずく"), "て"),
		    web_you[trap->madeby_u]);
		u.utraptype = TT_WEB;

		/* Time stuck in the web depends on your strength. */
		{
		    register int str = ACURR(A_STR);

		    if (str == 3) u.utrap = rn1(6,6);
		    else if (str < 6) u.utrap = rn1(6,4);
		    else if (str < 9) u.utrap = rn1(4,4);
		    else if (str < 12) u.utrap = rn1(4,2);
		    else if (str < 15) u.utrap = rn1(2,2);
		    else if (str < 18) u.utrap = rnd(2);
		    else if (str < 69) u.utrap = 1;
		    else {
			u.utrap = 0;
/*JP			You("tear through %s web!", a_your[trap->madeby_u]);*/
			You("%s蜘蛛の巣を引き裂いた！",
			    web_you[trap->madeby_u]);
			deltrap(trap);
			newsym(u.ux,u.uy);	/* get rid of trap symbol */
		    }
		}
		break;

	    case STATUE_TRAP:
		(void) activate_statue_trap(trap, u.ux, u.uy, FALSE);
		break;

	    case MAGIC_TRAP:	    /* A magic trap. */
		seetrap(trap);
		if (!rn2(30)) {
		    deltrap(trap);
		    newsym(u.ux,u.uy);	/* update position */
/*JP		    You("are caught in a magical explosion!");*/
		    You("魔法の爆発を浴びた！");
/*JP		    losehp(rnd(10), "magical explosion", KILLED_BY_AN);*/
		    losehp(rnd(10), "魔法の爆発を浴びて", KILLED_BY_AN);
/*JP		    Your("body absorbs some of the magical energy!");*/
		    Your("体は魔法のエネルギーを少し吸いとった！");
		    u.uen = (u.uenmax += 2);
		} else domagictrap();
		break;

	    case ANTI_MAGIC:
		seetrap(trap);
		if(Antimagic) {
		    shieldeff(u.ux, u.uy);
/*JP		    You_feel("momentarily lethargic.");*/
		    You("一瞬無気力感を感じた．");
		} else drain_en(rnd(u.ulevel) + 1);
		break;

	    case POLY_TRAP:
		seetrap(trap);
/*JP		You("%s onto a polymorph trap!",
		    Levitation ? (const char *)"float" :
		    locomotion(uasmon, "step"));*/
		You("変化の罠に%s！",
		    jconj(Levitation ? (const char *)"浮きながら飛びこむ" : 
		    locomotion(uasmon, "踏み込む"), "た"));
		if(Antimagic) {
		    shieldeff(u.ux, u.uy);
/*JP		    You_feel("momentarily different.");(*/
		    You("一瞬無気力感を感じた．");
		    /* Trap did nothing; don't remove it --KAA */
		} else {
		    deltrap(trap);	/* delete trap before polymorph */
		    newsym(u.ux,u.uy);	/* get rid of trap symbol */
/*JP		    You_feel("a change coming over you.");*/
		    You("変化が訪れたような気がした．");
		    polyself();
		}
		break;

	    case LANDMINE:
		if (Levitation || is_flyer(uasmon)) {
		    if (!already_seen && rn2(3)) break;
		    seetrap(trap);
/*JP		    pline("%s %s in a pile of soil below you.",
			    already_seen ? "There is" : "You discover",
			    trap->madeby_u ? "the trigger of your mine" :
					     "a trigger");*/
		    if(already_seen)
		      pline("ここには%s地雷の起爆スイッチがある．",
			    set_you[trap->madeby_u]);
		    else
		      You("足下の土の山に%s地雷の起爆スイッチをみつけた．",
			    set_you[trap->madeby_u]);
		    if (already_seen && rn2(3)) break;
/*JP		    pline("KAABLAMM!!!  The air currents set %s%s off!",
			    already_seen ? a_your[trap->madeby_u] : "",
			    already_seen ? " land mine" : "it");*/
		    pline("ちゅどーん！！空気の流れで%s地雷のスイッチが入った！",
			    set_you[trap->madeby_u]);
		} else {
		    seetrap(trap);
/*JP		    pline("KAABLAMM!!!  You triggered %s land mine!",
					    a_your[trap->madeby_u]);*/
		    pline("ちゅどーん！！%s地雷の起爆スイッチを踏んだ！",
			    set_you[trap->madeby_u]);
		    set_wounded_legs(LEFT_SIDE, rn1(35, 41));
		    set_wounded_legs(RIGHT_SIDE, rn1(35, 41));
		    exercise(A_DEX, FALSE);
		}
		blow_up_landmine(trap);
		newsym(u.ux,u.uy);		/* update trap symbol */
/*JP		losehp(rnd(16), "land mine", KILLED_BY_AN);*/
		losehp(rnd(16), "地雷を踏んで", KILLED_BY_AN);
		/* fall recursively into the pit... */
		if ((trap = t_at(u.ux, u.uy)) != 0) dotrap(trap);
		break;

	    case ROLLING_BOULDER_TRAP:
		seetrap(trap);
/*JP		pline("Click! You trigger a rolling boulder trap!");*/
		pline("カチッ！あなたは落岩のスイッチを踏んだ！");
		if(!launch_obj(BOULDER, trap->launch.x, trap->launch.y,
		      trap->launch2.x,trap->launch2.y, ROLL)) {
		    deltrap(trap);
		    newsym(u.ux,u.uy);	/* get rid of trap symbol */
/*JP		    pline("Fortunately for you, no boulder was released.");*/
		    pline("運のよいことに岩は転がってこなかった．");
		}
		break;

	    case MAGIC_PORTAL:
		seetrap(trap);
		domagicportal(trap);
		break;

	    default:
		seetrap(trap);
		impossible("You hit a trap of type %u", trap->ttyp);
	}
}

/* some actions common to both player and monsters for triggered landmine */
STATIC_OVL void
blow_up_landmine(trap)
struct trap *trap;
{
	scatter(trap->tx, trap->ty, 4,
		MAY_DESTROY | MAY_HIT | MAY_FRACTURE | VIS_EFFECTS);
	del_engr_at(trap->tx, trap->ty);
	wake_nearto(trap->tx, trap->ty, 400);
	if (IS_DOOR(levl[trap->tx][trap->ty].typ))
	    levl[trap->tx][trap->ty].doormask = D_BROKEN;
	/* TODO: destroy drawbridge if present;
		 sometimes delete trap instead of always leaving a pit */
	trap->ttyp = PIT;		/* explosion creates a pit */
	trap->madeby_u = FALSE;		/* resulting pit isn't yours */
}

#endif /* OVLB */
#ifdef OVL3

/*
 * Move obj from (x1,y1) to (x2,y2)
 *
 * Return 0 if no object was launched.
 *        1 if an object was launched and placed somewhere.
 *        2 if an object was launched, but used up.
 */
int
launch_obj(otyp, x1, y1, x2, y2, style)
short otyp;
register int x1,y1,x2,y2;
int style;
{
	register struct monst *mtmp;
	register struct obj *otmp;
	register int dx,dy;
	struct obj *singleobj;
	boolean used_up = FALSE;
	boolean otherside = FALSE;
	int dist;
	int tmp;
	int delaycnt = 0;

	otmp = sobj_at(otyp, x1, y1);
	/* Try the other side too, for rolling boulder traps */
	if (!otmp && otyp == BOULDER) {
		otherside = TRUE;
		otmp = sobj_at(otyp, x2, y2);
	}
	if (!otmp) return 0;
	if (otherside) {	/* swap 'em */
		int tx, ty;

		tx = x1; ty = y1;
		x1 = x2; y1 = y2;
		x2 = tx; y2 = ty;
	}

	if (otmp->quan == 1L) {
	    obj_extract_self(otmp);
	    singleobj = otmp;
	    otmp = (struct obj *) 0;
	} else {
	    singleobj = splitobj(otmp, otmp->quan - 1L);
	    obj_extract_self(singleobj);
	}
	newsym(x1,y1);
	/* in case you're using a pick-axe to chop the boulder that's being
	   launched (perhaps a monster triggered it), destroy context so that
	   next dig attempt never thinks you're resuming previous effort */
	if ((otyp == BOULDER || otyp == STATUE) &&
	    singleobj->ox == digging.pos.x && singleobj->oy == digging.pos.y)
	    (void) memset((genericptr_t)&digging, 0, sizeof digging);

	dist = distmin(x1,y1,x2,y2);
	bhitpos.x = x1;
	bhitpos.y = y1;
	dx = sgn(x2 - x1);
	dy = sgn(y2 - y1);
	switch (style) {
	    case ROLL:
			delaycnt = 2;
			/* fall through */
	    default:
			if (!delaycnt) delaycnt = 1;
			if (!cansee(bhitpos.x,bhitpos.y)) curs_on_u();
			tmp_at(DISP_FLASH, obj_to_glyph(singleobj));
			tmp_at(bhitpos.x, bhitpos.y);
	}
	while(dist-- > 0 && !used_up) {
		tmp_at(bhitpos.x, bhitpos.y);
		tmp = delaycnt;
		while (tmp-- > 0) delay_output();
		bhitpos.x += dx;
		bhitpos.y += dy;

		if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
			if (otyp == BOULDER && throws_rocks(mtmp->data)) {
			    if (rn2(3)) {
/*JP				pline("%s snatches the boulder.",*/
				pline("%sは岩をもぎとった．",
					Monnam(mtmp));
				mpickobj(mtmp, singleobj);
				used_up = TRUE;
				break;
			    }
			}
			if (ohitmon(mtmp,singleobj,
					(style==ROLL) ? -1 : dist, FALSE)) {
				used_up = TRUE;
				break;
			}
		} else if (bhitpos.x==u.ux && bhitpos.y==u.uy) {
			int hitvalu, hitu;
			if (multi) nomul(0);
			hitvalu = 9 + singleobj->spe;
			hitu = thitu(hitvalu,
				dmgval(singleobj, &youmonst),
				singleobj,
				xname(singleobj));
			if (hitu) stop_occupation();
		}
		if (style == ROLL) {
		    if (down_gate(bhitpos.x, bhitpos.y) != -1) {
		       if (ship_object(singleobj, bhitpos.x, bhitpos.y, FALSE)){
				used_up = TRUE;
				break;
			}
		    }
/*JP		    if (flooreffects(singleobj, bhitpos.x, bhitpos.y, "fall")) {*/
		    if (flooreffects(singleobj, bhitpos.x, bhitpos.y, "落ちた")) {
			used_up = TRUE;
			break;
		    }
		}
		if (otyp == BOULDER && closed_door(bhitpos.x,bhitpos.y)) {
			if (cansee(bhitpos.x, bhitpos.y))
/*JP				pline_The("boulder crashes through a door.");*/
				pline("岩は扉を破壊した．");
			levl[bhitpos.x][bhitpos.y].doormask = D_BROKEN;
		}
	}
	tmp_at(DISP_END, 0);
	if (!used_up) {
		place_object(singleobj, x2,y2);
		newsym(x2,y2);
		return 1;
	} else
		return 2;
}
#endif /* OVL3 */
#ifdef OVLB

void
seetrap(trap)
	register struct trap *trap;
{
	if(!trap->tseen) {
	    trap->tseen = 1;
	    newsym(trap->tx, trap->ty);
	}
}

#endif /* OVLB */
#ifdef OVL3

STATIC_OVL int
mkroll_launch(ttmp, x, y, otyp, ocount)
struct trap *ttmp;
xchar x,y;
short otyp;
long ocount;
{
	struct obj *otmp;
	register int tmp;
	schar dx,dy;
	int distance;
	coord cc;
	coord bcc;
	int trycount = 0;
	boolean success = FALSE;
	int mindist = 4;

	if (ttmp->ttyp == ROLLING_BOULDER_TRAP) mindist = 2;
	distance = rn1(5,4);    /* 4..8 away */
	tmp = rn2(8);		/* randomly pick a direction to try first */
	while (distance >= mindist) {
		dx = xdir[tmp];
		dy = ydir[tmp];
		cc.x = x; cc.y = y;
		/* Prevent boulder from being placed on water */
		if (ttmp->ttyp == ROLLING_BOULDER_TRAP
				&& is_pool(x+distance*dx,y+distance*dy))
			success = FALSE;
		else success = isclearpath(&cc, distance, dx, dy);
		if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
			boolean success_otherway;
			bcc.x = x; bcc.y = y;
			success_otherway = isclearpath(&bcc, distance,
						-(dx), -(dy));
			if (!success_otherway) success = FALSE;
		}
		if (success) break;
		if (++tmp > 7) tmp = 0;
		if ((++trycount % 8) == 0) --distance;
	}
	if (!success) {
	    /* create the trap without any ammo, launch pt at trap location */
		cc.x = bcc.x = x;
		cc.y = bcc.y = y;
	} else {
		otmp = mksobj(otyp, TRUE, FALSE);
		otmp->quan = ocount;
		otmp->owt = weight(otmp);
		place_object(otmp, cc.x, cc.y);
		stackobj(otmp);
	}
	ttmp->launch.x = cc.x;
	ttmp->launch.y = cc.y;
	if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
		ttmp->launch2.x = bcc.x;
		ttmp->launch2.y = bcc.y;
	} else
		ttmp->launch_otyp = otyp;
	newsym(ttmp->launch.x, ttmp->launch.y);
	return 1;
}

STATIC_OVL boolean
isclearpath(cc,distance,dx,dy)
coord *cc;
int distance;
schar dx,dy;
{
	uchar typ;
	xchar x, y;

	x = cc->x;
	y = cc->y;
	while (distance-- > 0) {
		x += dx;
		y += dy;
		typ = levl[x][y].typ;
		if (!isok(x,y) || !ZAP_POS(typ) || closed_door(x,y))
			return FALSE;
	}
	cc->x = x;
	cc->y = y;
	return TRUE;
}
#endif /* OVL3 */
#ifdef OVL1

int
mintrap(mtmp)
register struct monst *mtmp;
{
	register struct trap *trap = t_at(mtmp->mx, mtmp->my);
	boolean trapkilled = FALSE;
	struct permonst *mptr = mtmp->data;
	struct obj *otmp;

	if (!trap) {
	    mtmp->mtrapped = 0;	/* perhaps teleported? */
	} else if (mtmp->mtrapped) {	/* is currently in the trap */
	    if (!rn2(40)) {
		if (sobj_at(BOULDER, mtmp->mx, mtmp->my) &&
			(trap->ttyp == PIT || trap->ttyp == SPIKED_PIT)) {
		    if (!rn2(2)) {
			mtmp->mtrapped = 0;
			if (canseemon(mtmp))
/*JP
			    pline("%s pulls free...", Monnam(mtmp));
*/
			    pline("%sは助け上げられた．", Monnam(mtmp));
			fill_pit(mtmp->mx, mtmp->my);
		    }
		} else {
		    mtmp->mtrapped = 0;
		}
	    } else if (trap->ttyp == BEAR_TRAP && metallivorous(mptr)) {
		if (canseemon(mtmp))
/*JP
		    pline("%s eats a bear trap!", Monnam(mtmp));
*/
		    pline("%sは熊の罠を食べた！", Monnam(mtmp));
		deltrap(trap);
		mtmp->meating = 5;
		mtmp->mtrapped = 0;
	    }
	} else {
	    register int tt = trap->ttyp;
	    boolean in_sight, tear_web, see_it;

	    if ((mtmp->mtrapseen & (1 << (tt-1))) != 0 ||
		    (tt == HOLE && !mindless(mtmp->data))) {
		/* it has been in such a trap - perhaps it escapes */
		if(rn2(4)) return(0);
	    } else {
		mtmp->mtrapseen |= (1 << (tt-1));
	    }
	    /* Monster is aggravated by being trapped by you.
	       Recognizing who made the trap isn't completely
	       unreasonable; everybody has their own style. */
	    if (trap->madeby_u && rnl(5)) setmangry(mtmp);

	    /* bug?  `in_sight' ought to be split to distinguish between
	       trap_in_sight and can_see_victim to handle invisible monsters */
	    in_sight = canseemon(mtmp);
	    switch (tt) {
		case ARROW_TRAP:
			otmp = mksobj(ARROW, TRUE, FALSE);
			otmp->quan = 1L;
			otmp->owt = weight(otmp);
			if (in_sight) seetrap(trap);
			if(thitm(8, mtmp, otmp, 0)) trapkilled = TRUE;
			break;
		case DART_TRAP:
			otmp = mksobj(DART, TRUE, FALSE);
			otmp->quan = 1L;
			otmp->owt = weight(otmp);
			if (!rn2(6)) otmp->opoisoned = 1;
			if (in_sight) seetrap(trap);
			if(thitm(7, mtmp, otmp, 0)) trapkilled = TRUE;
			break;
		case ROCKTRAP:
			otmp = mksobj(ROCK, TRUE, FALSE);
			otmp->quan = 1L;
			otmp->owt = weight(otmp);
			if (in_sight) seetrap(trap);
			if (thitm(0, mtmp, otmp, d(2, 6)))
			    trapkilled = TRUE;
			break;

		case SQKY_BOARD:
			if(is_flyer(mptr)) break;
			/* stepped on a squeaky board */
			if (in_sight) {
/*JP			    pline("A board beneath %s squeaks loudly.", mon_nam(mtmp));*/
			    pline("%sの足元の板が大きくきしんだ．", mon_nam(mtmp));
			    seetrap(trap);
			} else
/*JP			   You_hear("a distant squeak.");*/
			   You_hear("遠くできしむ音を聞いた．");
			/* wake up nearby monsters */
			wake_nearto(mtmp->mx, mtmp->my, 40);
			break;

		case BEAR_TRAP:
			if(mptr->msize > MZ_SMALL &&
				!amorphous(mptr) && !is_flyer(mptr) &&
				!is_whirly(mptr) && !unsolid(mptr)) {
			    mtmp->mtrapped = 1;
			    if(in_sight) {
/*JP				pline("%s is caught in %s bear trap!",
				      Monnam(mtmp), a_your[trap->madeby_u]);*/
				pline("%sは%s熊の罠につかまった！",
				      Monnam(mtmp), set_you[trap->madeby_u]);
				seetrap(trap);
			    } else {
				if((mptr == &mons[PM_OWLBEAR]
				    || mptr == &mons[PM_BUGBEAR])
				   && flags.soundok)
/*JP				    You_hear("the roaring of an angry bear!");*/
				    You_hear("怒りの咆哮を聞いた！");
			    }
			}
			break;

		case SLP_GAS_TRAP:
			if (!resists_sleep(mtmp) &&
				!mtmp->msleep && mtmp->mcanmove) {
			    mtmp->mcanmove = 0;
			    mtmp->mfrozen = rnd(25);
			    if (in_sight) {
/*JP				pline("%s suddenly falls asleep!",*/
				pline("%sは突然眠りに落ちた！",
				      Monnam(mtmp));
				seetrap(trap);
			    }
			}
			break;

		case RUST_TRAP:
			if (in_sight) {
/*JP			    pline("%s %s!", A_gush_of_water_hits,*/
			    pline("水が噴出して，%sに命中した！", 
				  mon_nam(mtmp));
			    seetrap(trap);
			}
			if (mptr == &mons[PM_IRON_GOLEM]) {
				if (in_sight)
/*JP				    pline("%s falls to pieces!", Monnam(mtmp));*/
				    pline("%sはくだけちった！", Monnam(mtmp));
				else if(mtmp->mtame)
/*JP				    pline("May %s rust in peace.",*/
				    pline("%sよ永遠に破片なれ．",
								mon_nam(mtmp));
				mondied(mtmp);
				if (mtmp->mhp <= 0)
					trapkilled = TRUE;
			} else if (mptr == &mons[PM_GREMLIN] && rn2(3)) {
				struct monst *mtmp2 = clone_mon(mtmp);

				if (mtmp2) {
				    mtmp2->mhpmax = (mtmp->mhpmax /= 2);
				    if(in_sight)
/*JP					pline("%s multiplies.", Monnam(mtmp));*/
					pline("%sは分裂した．", Monnam(mtmp));
				}
			}
			if (rn2(2))
			    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
				(void) snuff_lit(otmp);
			break;

		case FIRE_TRAP:
 mfiretrap:
			see_it = cansee(mtmp->mx, mtmp->my);
			if (in_sight)
/*JP			    pline("A %s erupts from the %s under %s!",
				  tower_of_flame,
				  surface(mtmp->mx,mtmp->my), mon_nam(mtmp));*/
			    pline("火柱が%sの足元の%sから立ちのぼった！",
				  mon_nam(mtmp), surface(mtmp->mx,mtmp->my));
			else if (see_it)  /* evidently `mtmp' is invisible */
/*JP			    You("see a %s erupt from the %s!",
				tower_of_flame, surface(mtmp->mx,mtmp->my));*/
			    You("火柱が%sから生じるのを見た！",
				surface(mtmp->mx,mtmp->my));

			if (resists_fire(mtmp)) {
			    if (in_sight) {
				shieldeff(mtmp->mx,mtmp->my);
/*JP				pline("%s is uninjured.", Monnam(mtmp));*/
				pline("が，%sは傷つかない．", Monnam(mtmp));
			    }
			} else {
			    int num = d(2,4);

			    if (thitm(0, mtmp, (struct obj *)0, num))
				trapkilled = TRUE;
			    else
				/* we know mhp is at least `num' below mhpmax,
				   so no (mhp > mhpmax) check is needed here */
				mtmp->mhpmax -= rn2(num + 1);
			}
			(void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			(void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
			(void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			if (burn_floor_paper(mtmp->mx, mtmp->my, see_it) &&
				!see_it && distu(mtmp->mx, mtmp->my) <= 3*3)
/*JP			    You("smell smoke.");*/
			    pline("煙の匂いがした．");
			if (is_ice(mtmp->mx,mtmp->my))
			    melt_ice(mtmp->mx,mtmp->my);
			if (see_it) seetrap(trap);
			break;

		case PIT:
		case SPIKED_PIT:
			if ( !is_flyer(mptr) &&
			     (!mtmp->wormno || (count_wsegs(mtmp) < 6)) &&
			     !is_clinger(mptr) ) {
				if (!passes_walls(mptr))
				    mtmp->mtrapped = 1;
				if(in_sight) {
/*JP				    pline("%s falls into %s pit!",
					Monnam(mtmp), a_your[trap->madeby_u]);*/
				    pline("%sは%s落し穴に落ちた！",
					Monnam(mtmp), set_you[trap->madeby_u]);
				    seetrap(trap);
				}
/*JP				mselftouch(mtmp, "Falling, ", FALSE);*/
				mselftouch(mtmp, "落下中，", FALSE);
				if(mtmp->mhp <= 0 ||
					thitm(0, mtmp, (struct obj *)0,
					 rnd((tt==PIT) ? 6 : 10)))
				    trapkilled = TRUE;
			}
			break;
		case HOLE:
		case TRAPDOOR:
			if (!Can_fall_thru(&u.uz)) {
			 impossible("mintrap: %ss cannot exist on this level.",
				    defsyms[trap_to_defsym(tt)].explanation);
			    break;	/* don't activate it after all */
			}
			if (is_flyer(mptr) || mptr == &mons[PM_WUMPUS] ||
			    (mtmp->wormno && count_wsegs(mtmp) > 5) ||
			    mptr->msize >= MZ_HUGE) break;
			/* Fall through */
		case LEVEL_TELEP:
		case MAGIC_PORTAL:
			{
			    int mlev_res;
			    mlev_res = mlevel_tele_trap(mtmp, trap, in_sight);
			    if (mlev_res) return(mlev_res);
			}
			break;

		case TELEP_TRAP:
			mtele_trap(mtmp, trap, in_sight);
			break;

		case WEB:
			/* Monster in a web. */
			if (mptr->mlet == S_SPIDER) break;
			if (amorphous(mptr) || is_whirly(mptr) || unsolid(mptr)){
			    if(acidic(mptr) ||
			       mptr == &mons[PM_GELATINOUS_CUBE] ||
			       mptr == &mons[PM_FIRE_ELEMENTAL]) {
				if (in_sight)
/*JP				    pline("%s %s %s spider web!",
					  Monnam(mtmp),
					  (mptr == &mons[PM_FIRE_ELEMENTAL]) ?
					    "burns" : "dissolves",
					  a_your[trap->madeby_u]);*/
				    pline("%s%s蜘蛛の巣を%s！",
					  Monnam(mtmp),
					  web_you[trap->madeby_u],
					  (mptr == &mons[PM_FIRE_ELEMENTAL]) ?
					    "焼いた" : "こなごなにした");
				deltrap(trap);
				newsym(mtmp->mx, mtmp->my);
				break;
			    }
			    if (in_sight) {
/*JP				pline("%s flows through %s spider web.",
				      Monnam(mtmp),
				      a_your[trap->madeby_u]);*/
				pline("%sは%s蜘蛛の巣をするりと通り抜けた．",
				      Monnam(mtmp),
				      web_you[trap->madeby_u]);
				seetrap(trap);
			    }
			    break;
			}
			tear_web = FALSE;
			switch (monsndx(mptr)) {
			    case PM_OWLBEAR: /* Eric Backus */
			    case PM_BUGBEAR:
				if (!in_sight) {
/*JP				    You_hear("the roaring of a confused bear!");*/
				    You_hear("混乱の咆哮を聞いた！");
				    mtmp->mtrapped = 1;
				    break;
				}
				/* fall though */
			    default:
				if (mptr->mlet == S_GIANT ||
				    (mptr->mlet == S_DRAGON &&
					extra_nasty(mptr)) || /* excl. babies */
				    (mtmp->wormno && count_wsegs(mtmp) > 5)) {
				    tear_web = TRUE;
				} else if (in_sight) {
/*JP				    pline("%s is caught in %s spider web.",
					  Monnam(mtmp),
					  a_your[trap->madeby_u]);*/
				    pline("%sは%s蜘蛛の巣につかまった．",
					  Monnam(mtmp),
					  web_you[trap->madeby_u]);
				    seetrap(trap);
				}
				mtmp->mtrapped = tear_web ? 0 : 1;
				break;
			    /* this list is fairly arbitrary; it deliberately
			       excludes wumpus & giant/ettin zombies/mummies */
			    case PM_TITANOTHERE:
			    case PM_BALUCHITHERIUM:
			    case PM_PURPLE_WORM:
			    case PM_JABBERWOCK:
			    case PM_IRON_GOLEM:
			    case PM_BALROG:
			    case PM_KRAKEN:
				tear_web = TRUE;
				break;
			}
			if (tear_web) {
			    if (in_sight)
/*JP				pline("%s tears through %s spider web!",
				      Monnam(mtmp), a_your[trap->madeby_u]);*/
				pline("%sは%s蜘蛛の巣を引き裂いた！",
				      Monnam(mtmp), web_you[trap->madeby_u]);
			    deltrap(trap);
			    newsym(mtmp->mx, mtmp->my);
			}
			break;

		case STATUE_TRAP:
			break;

		case MAGIC_TRAP:
			/* A magic trap.  Monsters usually immune. */
			if (!rn2(21)) goto mfiretrap;
			break;
		case ANTI_MAGIC:
			break;

		case LANDMINE:
			if(rn2(3))
				break; /* monsters usually don't set it off */
			if(is_flyer(mptr)) {
				boolean already_seen = trap->tseen;
				if (in_sight && !already_seen) {
/*JP	pline("A trigger appears in a pile of soil below %s.", mon_nam(mtmp));*/
	pline("%sの足元の土の山に起爆スイッチが現われた．", mon_nam(mtmp));
					seetrap(trap);
				}
				if (rn2(3)) break;
				if (in_sight) {
					newsym(mtmp->mx, mtmp->my);
/*JP					pline_The("air currents set %s off!",
					  already_seen ? "a land mine" : "it");*/
					pline("空気の流れでスイッチが入った！");
				}
			} else if(in_sight) {
			    newsym(mtmp->mx, mtmp->my);
/*JP			    pline("KAABLAMM!!!  %s triggers %s land mine!",
				Monnam(mtmp), a_your[trap->madeby_u]);*/
			    pline("ちゅどーん！！%sは%s地雷の起爆スイッチを踏んだ！",
				Monnam(mtmp), set_you[trap->madeby_u]);
			}
			if (!in_sight)
/*JP				pline("Kaablamm!  You hear an explosion in the distance!");*/
				pline("ちゅどーん！あなたは遠方の爆発音を聞いた");
			blow_up_landmine(trap);
			if(thitm(0, mtmp, (struct obj *)0, rnd(16)))
				trapkilled = TRUE;
			else {
				/* monsters recursively fall into new pit */
				if (mintrap(mtmp) == 2) trapkilled=TRUE;
			}
			if (unconscious()) {
				multi = -1;
/*JP				nomovemsg="The explosion awakens you!";*/
				nomovemsg="爆発であなたは起きた！";
			}
			break;

		case POLY_TRAP:
		    if (resists_magm(mtmp)) {
			shieldeff(mtmp->mx, mtmp->my);
		    } else if (!resist(mtmp, WAND_CLASS, 0, NOTELL)) {
			(void) newcham(mtmp, (struct permonst *)0);
			if (in_sight) seetrap(trap);
		    }
		    break;

		case ROLLING_BOULDER_TRAP:
		    if (!is_flyer(mptr)) {
		        newsym(mtmp->mx,mtmp->my);
			if (in_sight)
/*JP
			  pline("Click! %s triggers %s.", Monnam(mtmp),
				  trap->tseen ?
				  "a rolling boulder trap" :
				  something);
*/
			  pline("カチッ！%sは%sのスイッチを踏んだ！", Monnam(mtmp),
				  trap->tseen ?
				  "落岩の罠" : "何か");
			if (launch_obj(BOULDER, trap->launch.x, trap->launch.y,
				       trap->launch2.x, trap->launch2.y, ROLL)) {
			  if (in_sight) trap->tseen = TRUE;
/*JP
			  else You_hear(Hallucination ?
					"someone bowling." :
					"rumbling in the distance.");
*/
			  else You_hear(Hallucination ?
					"誰かがボーリングをしている音を聞いた" :
					"遠くのゴロゴロという音を聞いた");
			  if (mtmp->mhp <= 0) trapkilled = TRUE;
			} else {
			  deltrap(trap);
			  newsym(mtmp->mx,mtmp->my);
			}
		      }
		    break;

		default:
			impossible("Some monster encountered a strange trap of type %d.", tt);
	    }
	}
	if(trapkilled) return 2;
	return mtmp->mtrapped;
}

#endif /* OVL1 */
#ifdef OVLB

/* Combine cockatrice checks into single functions to avoid repeating code. */
void
instapetrify(str)
const char *str;
{
	if (resists_ston(&youmonst)) return;
	if (poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM))
	    return;
/*JP	You("turn to stone...");*/
	You("石化した．．．");
	killer_format = KILLED_BY;
	killer = str;
	done(STONING);
}

void
minstapetrify(mon,byplayer)
struct monst *mon;
boolean byplayer;
{
	if (resists_ston(mon)) return;
	if (cansee(mon->mx, mon->my))
/*JP		pline("%s turns to stone.", Monnam(mon));*/
		pline("%sは石化した．", Monnam(mon));
	if (poly_when_stoned(mon->data)) {
		mon_to_stone(mon);
		return;
	}
	if (byplayer) {
		stoned = TRUE;
		xkilled(mon,0);
	} else monstone(mon);
}

void
selftouch(arg)
const char *arg;
{
	if (uwep && uwep->otyp == CORPSE && uwep->corpsenm == PM_COCKATRICE) {
/*JP		pline("%s touch the cockatrice corpse.", arg);*/
		pline("%sコカトリスの死体に触った．",arg);
/*JP		instapetrify("cockatrice corpse");*/
		instapetrify("コカトリスの死体に触れて");
	}
}

void
mselftouch(mon,arg,byplayer)
struct monst *mon;
const char *arg;
boolean byplayer;
{
	struct obj *mwep = MON_WEP(mon);

	if (mwep && mwep->otyp == CORPSE && mwep->corpsenm == PM_COCKATRICE) {
		if (cansee(mon->mx, mon->my)) {
/*JP			pline("%s%s touches the cockatrice corpse.",
			    arg ? arg : "", arg ? mon_nam(mon) : Monnam(mon));*/
			pline("%s%sはコカトリスの死体に触った．",
			    arg ? arg : "", mon_nam(mon));
		}
		minstapetrify(mon, byplayer);
	}
}

void
float_up()
{
	if(u.utrap) {
		if(u.utraptype == TT_PIT) {
			u.utrap = 0;
/*JP			You("float up, out of the pit!");*/
			You("落し穴から浮き出た！");
			vision_full_recalc = 1;	/* vision limits change */
			fill_pit(u.ux, u.uy);
		} else if (u.utraptype == TT_INFLOOR) {
/*JP			Your("body pulls upward, but your %s are still stuck.",*/
			Your("体は引き上げられた．しかし%sはまだはまっている．",
			     makeplural(body_part(LEG)));
		} else {
/*JP			You("float up, only your %s is still stuck.",*/
			You("浮き出た．%sだけがはまっている",
				body_part(LEG));
		}
	}
	else if(Is_waterlevel(&u.uz))
/*JP		pline("It feels as though you'd lost some weight.");*/
		You("まるで体重が減ったように感じた．");
	else if(u.uinwater)
		spoteffects();
	else if(u.uswallow)
/*JP		You(is_animal(u.ustuck->data) ?
			"float away from the %s."  :
			"spiral up into %s.",
		    is_animal(u.ustuck->data) ?
			surface(u.ux, u.uy) :
			mon_nam(u.ustuck));*/
		You(is_animal(u.ustuck->data) ?
			"%sの中で浮いた．" :
			"%sの中でぐるぐる回転した．",
		    is_animal(u.ustuck->data) ?
			surface(u.ux, u.uy) :
			mon_nam(u.ustuck));
	else if (Hallucination)
/*JP		pline("Up, up, and awaaaay!  You're walking on air!");*/
		pline("上れ，上れ，上れぇぇぇぇ！あなたは空中を歩いている！");
	else if(Is_airlevel(&u.uz))
/*JP		You("gain control over your movements.");*/
		You("うまく歩けるようになった．");
	else
/*JP		You("start to float in the air!");*/
		You("空中に浮きはじめた！");
}

void
fill_pit(x, y)
int x, y;
{
	struct obj *otmp;
	struct trap *t;

	if ((t = t_at(x, y)) &&
	    ((t->ttyp == PIT) || (t->ttyp == SPIKED_PIT)) &&
	    (otmp = sobj_at(BOULDER, x, y))) {
		obj_extract_self(otmp);
/*JP		(void) flooreffects(otmp, x, y, "settle");*/
		(void) flooreffects(otmp, x, y, "はまった");
	}
}

int
float_down(override_mask)
long override_mask;	/* might cancel timeout */
{
	register struct trap *trap = (struct trap *)0;
	d_level current_dungeon_level;
	boolean no_msg = FALSE;

	HLevitation &= ~override_mask;
	if(Levitation) return(0); /* maybe another ring/potion/boots */

	if (Punished && !carried(uball) &&
	    (is_pool(uball->ox, uball->oy) ||
	     ((trap = t_at(uball->ox, uball->oy)) &&
	      ((trap->ttyp == PIT) || (trap->ttyp == SPIKED_PIT) ||
	       (trap->ttyp == TRAPDOOR) || (trap->ttyp == HOLE))))) {
			u.ux0 = u.ux;
			u.uy0 = u.uy;
			u.ux = uball->ox;
			u.uy = uball->oy;
			movobj(uchain, uball->ox, uball->oy);
			newsym(u.ux0, u.uy0);
			vision_full_recalc = 1;	/* in case the hero moved. */
	}
	/* check for falling into pool - added by GAN 10/20/86 */
	if(!is_flyer(uasmon)) {
		/* kludge alert:
		 * drown() and lava_effects() print various messages almost
		 * every time they're called which conflict with the "fall
		 * into" message below.  Thus, we want to avoid printing
		 * confusing, duplicate or out-of-order messages.
		 * Use knowledge of the two routines as a hack -- this
		 * should really handled differently -dlc
		 */
		if(is_pool(u.ux,u.uy) && !Wwalking && !u.uinwater)
			no_msg = drown();

		if(is_lava(u.ux,u.uy)) {
			(void) lava_effects();
			no_msg = TRUE;
		}
	}
	if (!trap) {
		if(Is_airlevel(&u.uz))
/*JP			You("begin to tumble in place.");*/
			You("その場で転落しはじめた．");
		else if (Is_waterlevel(&u.uz) && !no_msg)
/*JP			You_feel("heavier.");*/
			You("重くなったような気がした．");
		/* u.uinwater msgs already in spoteffects()/drown() */
		else if (!u.uinwater && !no_msg) {
			if (Hallucination)
/*JP				pline("Bummer!  You've %s.",
				      is_pool(u.ux,u.uy) ?
					"splashed down" : "hit the ground");*/
				pline("やめてぇ！あなたは%s．",
				      is_pool(u.ux,u.uy) ?
					"ザブンと落ちた" : "地面にたたきつけられた");
			else
/*JP				You("float gently to the %s.",*/
				You("静かに%sまで辿りついた．",
				    surface(u.ux, u.uy));
		}
		trap = t_at(u.ux,u.uy);
	}

	/* can't rely on u.uz0 for detecting trap door-induced level change;
	   it gets changed to reflect the new level before we can check it */
	assign_level(&current_dungeon_level, &u.uz);

	if(trap)
		switch(trap->ttyp) {
		case STATUE_TRAP:
			break;
		case HOLE:
		case TRAPDOOR:
			if(!Can_fall_thru(&u.uz) || u.ustuck)
				break;
			/* fall into next case */
		default:
			dotrap(trap);
	}

	if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) && !u.uswallow &&
		/* falling through trap door calls goto_level,
		   and goto_level does its own pickup() call */
		on_level(&u.uz, &current_dungeon_level))
	    pickup(1);
	return 1;
}

STATIC_OVL void
dofiretrap(box)
struct obj *box;	/* null for floor trap */
{
	boolean see_it = !Blind;
	int num;

/* Bug: for box case, the equivalent of burn_floor_paper() ought
 * to be done upon its contents.
 */

	if ((box && !carried(box)) ? is_pool(box->ox, box->oy) : Underwater) {
/*JP	    pline("A cascade of steamy bubbles erupts from %s!",
		    the(box ? xname(box) : surface(u.ux,u.uy)));*/
	    pline("蒸気の泡が%sからしゅーっと発生した！",
		    box ? xname(box) : surface(u.ux,u.uy));
/*JP	    if (Fire_resistance) You("are uninjured.");*/
	    if (Fire_resistance) You("傷つかない．");
/*JP	    else losehp(rnd(3), "boiling water", KILLED_BY);*/
	    else losehp(rnd(3), "沸騰した水で", KILLED_BY);
	    return;
	}
/*JP	pline("A %s %s from %s!", tower_of_flame,
	      box ? "bursts" : "erupts",
	      the(box ? xname(box) : surface(u.ux,u.uy)));*/
	pline("火柱が%sから%s！",
	      box ? xname(box) : surface(u.ux,u.uy),
	      box ? "吹き出した" : "立ちのぼった");
	if (Fire_resistance) {
	    shieldeff(u.ux, u.uy);
	    num = rn2(2);
	} else {
	    num = d(2,4);
	    if (u.uhpmax > u.ulevel)
		u.uhpmax -= rn2(min(u.uhpmax,num + 1)), flags.botl = 1;
	}
	if (!num)
/*JP	    You("are uninjured.");*/
	    You("傷つかない．");
	else
	    losehp(num, "火柱で", KILLED_BY_AN);

	if (burnarmor() || rn2(3)) {
	    destroy_item(SCROLL_CLASS, AD_FIRE);
	    destroy_item(SPBOOK_CLASS, AD_FIRE);
	    destroy_item(POTION_CLASS, AD_FIRE);
	}
	if (!box && burn_floor_paper(u.ux, u.uy, see_it) && !see_it)
/*JP	    You("smell paper burning.");*/
	    You("紙のこげる匂いがした．");
	if (is_ice(u.ux, u.uy))
	    melt_ice(u.ux, u.uy);
}

STATIC_OVL void
domagictrap()
{
	register int fate = rnd(20);

	/* What happened to the poor sucker? */

	if (fate < 10) {
	  /* Most of the time, it creates some monsters. */
	  register int cnt = rnd(4);

	  if (!resists_blnd(&youmonst)) {
/*JP		You("are momentarily blinded by a flash of light!");*/
		You("まばゆい光で一瞬目がくらんだ！");
		make_blinded((long)rn1(5,10),FALSE);
	  } else if (!Blind) {
/*JP		You("see a flash of light!");*/
		You("まばゆい光を浴びた！");
	  }  else
/*JP		You_hear("a deafening roar!");*/
		You_hear("耳をつんざくような咆哮を聞いた！");
	  while(cnt--)
		(void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
	}
	else
	  switch (fate) {

	     case 10:
	     case 11:
		      /* sometimes nothing happens */
			break;
	     case 12: /* a flash of fire */
			dofiretrap((struct obj *)0);
			break;

	     /* odd feelings */
/*JP	     case 13:	pline("A shiver runs up and down your %s!",*/
	     case 13:	pline("震えがあなたの%sを走った！",
			      body_part(SPINE));
			break;
/*JP	     case 14:	You_hear(Hallucination ?
				"the moon howling at you." :
				"distant howling.");*/
	     case 14:	You_hear(Hallucination ?
				"ふにゃ？月が吠えている．" :
				"遠方の遠吠を聞いた．");
			break;
	     case 15:	if (on_level(&u.uz, &qstart_level))
/*JP			    You_feel("%slike the prodigal son.",
			      (flags.female || (Upolyd && is_neuter(uasmon))) ?
				     "oddly " : "");*/
			    You_feel("%s悔い改めた罪人を好きになったような気がした．",
			      (flags.female || (Upolyd && is_neuter(uasmon))) ?
				     "異常に" : "");
			else
/*JP			    You("suddenly yearn for %s.",
				Hallucination ? "Cleveland" :
			    (In_quest(&u.uz) || at_dgn_entrance("The Quest")) ?
						"your nearby homeland" :
						"your distant homeland");*/
			    You("突然%sが恋しくなった．",
				Hallucination ? "青森" :
			    (In_quest(&u.uz) || at_dgn_entrance("The Quest")) ?
						"すぐそこにある故郷" :
						"はるかかなたの故郷");

			break;
/*JP	     case 16:   Your("pack shakes violently!");*/
	     case 16:   Your("袋は激しく揺れた！");
			break;
	     case 17:	You(Hallucination ?
/*JP				"smell hamburgers." :
				"smell charred flesh.");*/
				"ハンバーガーの匂いがした．" :
				"黒焦げの肉の匂いがした．");
			break;
/*JP	     case 18:	You_feel("tired.");*/
	     case 18:	You("疲れを感じた．");
			break;

	     /* very occasionally something nice happens. */

	     case 19:
		    /* tame nearby monsters */
		   {   register int i,j;
		       register struct monst *mtmp;

		       (void) adjattrib(A_CHA,1,FALSE);
		       for(i = -1; i <= 1; i++) for(j = -1; j <= 1; j++) {
			   if(!isok(u.ux+i, u.uy+j)) continue;
			   mtmp = m_at(u.ux+i, u.uy+j);
			   if(mtmp)
			       (void) tamedog(mtmp, (struct obj *)0);
		       }
		       break;
		   }

	     case 20:
		    /* uncurse stuff */
		   {  register struct obj *obj;

			/* below plines added by GAN 10/30/86 */
/*JP			You_feel(Hallucination ?
				"in touch with the Universal Oneness." :
				"like someone is helping you.");*/
			You(Hallucination ?
				"宇宙原理の調和に触れているような気がした．":
				"誰かがあなたを助けてくれているような気がした．");
			for(obj = invent; obj ; obj = obj->nobj)
			       if(obj->owornmask || obj->otyp == LOADSTONE)
					uncurse(obj);
		       if(Punished) unpunish();
		       break;
		   }
	     default: break;
	  }
}

void
water_damage(obj, force, here)
register struct obj *obj;
register boolean force, here;
{
	/* Scrolls, spellbooks, potions, weapons and
	   pieces of armor may get affected by the water */
	for (; obj; obj = (here ? obj->nexthere : obj->nobj)) {

		(void) snuff_lit(obj);

		if(obj->greased) {
			if (force || !rn2(2)) obj->greased = 0;
		} else if(Is_container(obj) && !Is_box(obj) &&
			(obj->otyp != OILSKIN_SACK || (obj->cursed && !rn2(3)))) {
			water_damage(obj->cobj, force, FALSE);
		} else if (!force && (Luck + 5) > rn2(20)) {
			/*  chance per item of sustaining damage:
			 *	max luck (full moon):	 5%
			 *	max luck (elsewhen):	10%
			 *	avg luck (Luck==0):	75%
			 *	awful luck (Luck<-4):  100%
			 */
			continue;
		} else if (obj->oclass == SCROLL_CLASS) {
#ifdef MAIL
		    if (obj->otyp != SCR_MAIL)
#endif
			obj->otyp = SCR_BLANK_PAPER;
			obj->spe = 0;
		} else if (obj->oclass == SPBOOK_CLASS) {
			if (obj->otyp == SPE_BOOK_OF_THE_DEAD)
/*JP				pline("Steam rises from %s.", the(xname(obj)));*/
				pline("蒸気が%sから立ちのぼった．", the(xname(obj)));
			else obj->otyp = SPE_BLANK_PAPER;
		} else if (obj->oclass == POTION_CLASS) {
			if (obj->odiluted) {
				obj->otyp = POT_WATER;
				obj->blessed = obj->cursed = 0;
				obj->odiluted = 0;
			} else if (obj->otyp != POT_WATER)
				obj->odiluted++;
		} else if (is_rustprone(obj) && obj->oeroded < MAX_ERODE &&
			  !(obj->oerodeproof || (obj->blessed && !rnl(4)))) {
			/* all metal stuff and armor except (body armor
			   protected by oilskin cloak) */
			if(obj->oclass != ARMOR_CLASS || obj != uarm ||
			   !uarmc || uarmc->otyp != OILSKIN_CLOAK ||
			   (uarmc->cursed && !rn2(3)))
				obj->oeroded++;
		}
	}
}

/*
 * This function is potentially expensive - rolling
 * inventory list multiple times.  Luckily it's seldom needed.
 * Returns TRUE if disrobing made player unencumbered enough to
 * crawl out of the current predicament.
 */
STATIC_OVL boolean
emergency_disrobe(lostsome)
boolean *lostsome;
{
	int invc = inv_cnt();

	/* [TODO: drop u.ugold separately or insert it into inventory chain] */
	if (invc == 0) return FALSE;	/* nothing to drop */

	while (near_capacity() > (Punished ? UNENCUMBERED : SLT_ENCUMBER)) {
		register struct obj *obj, *otmp = (struct obj *)0;
		register int i = rn2(invc);

		for (obj = invent; obj; obj = obj->nobj) {
			/*
			 * Undroppables are: body armor, boots, gloves,
			 * amulets, and rings because of the time and effort
			 * in removing them + loadstone and other cursed stuff
			 * for obvious reasons.
			 */
			if (!((obj->otyp == LOADSTONE && obj->cursed) ||
			      obj == uamul || obj == uleft || obj == uright ||
			      obj == ublindf || obj == uarm || obj == uarmc ||
			      obj == uarmg || obj == uarmf ||
#ifdef TOURIST
			      obj == uarmu ||
#endif
			      (obj->cursed && (obj == uarmh || obj == uarms)) ||
			      welded(obj)))
				otmp = obj;
			/* reached the mark and found some stuff to drop? */
			if (--i < 0 && otmp) break;

			/* else continue */
		}

		/* nothing to drop and still overweight */
		if (!otmp) return(FALSE);

		if (otmp == uarmh) (void) Helmet_off();
		else if (otmp == uarms) (void) Shield_off();
		else if (otmp == uwep) setuwep((struct obj *)0);
		*lostsome = TRUE;
		dropx(otmp);
		invc--;
	}
	return(TRUE);
}

/*
 *  return(TRUE) == player relocated
 */
boolean
drown()
{
	boolean inpool_ok = FALSE, crawl_ok;
	int i, x, y;

	/* happily wading in the same contiguous pool */
	if (u.uinwater && is_pool(u.ux-u.dx,u.uy-u.dy) &&
	    (is_swimmer(uasmon) || Amphibious)) {
		/* water effects on objects every now and then */
		if (!rn2(5)) inpool_ok = TRUE;
		else return(FALSE);
	}

	if (!u.uinwater) {
/*JP	    You("%s into the water%c",
		Is_waterlevel(&u.uz) ? "plunge" : "fall",
		Amphibious ? '.' : '!');*/
	    You("水の中に%s%s",
		Is_waterlevel(&u.uz) ? "飛びこんだ" : "落ちた",
		Amphibious ? "．" : "！");
	    if(!is_swimmer(uasmon))
		if (!Is_waterlevel(&u.uz))
/*JP		    You("sink like %s.",
			Hallucination ? "the Titanic" : "a rock");*/
		    You("%sのように沈んだ．",
			Hallucination ? "タイタニック号" : "岩");
	}

	water_damage(invent, FALSE, FALSE);

	if(u.umonnum == PM_GREMLIN && rn2(3)) {
		struct monst *mtmp;
		if ((mtmp = cloneu()) != 0) {
			mtmp->mhpmax = (u.mhmax /= 2);
/*JP			You("multiply.");*/
			You("分裂した．");
		}
	}
	if (inpool_ok) return(FALSE);

	if ((i = number_leashed()) > 0) {
/*JP		pline_The("leash%s slip%s loose.",
			(i > 1) ? "es" : "",
			(i > 1) ? "" : "s");*/
		pline("紐がゆるんだ");
		unleash_all();
	}

	if (Amphibious || is_swimmer(uasmon)) {
		if (Amphibious) {
			if (flags.verbose)
/*JP				pline("But you aren't drowning.");*/
				pline("しかし，あなたは溺れなかった．");
			if (!Is_waterlevel(&u.uz)){
				if (Hallucination)
/*JP					Your("keel hits the bottom.");*/
					You("底にニードロップを決めた．");
				else
/*JP					You("touch bottom.");*/
					You("底についた．");
			}
		}
		if (Punished) {
			unplacebc();
			placebc();
		}
		vision_recalc(2);	/* unsee old position */
		u.uinwater = 1;
		under_water(1);
		vision_full_recalc = 1;
		return(FALSE);
	}
	if((Teleportation || can_teleport(uasmon)) &&
	   (Teleport_control || rn2(3) < Luck+2)) {
/*JP		You("attempt a teleport spell.");*/	/* utcsri!carroll */
		You("瞬間移動の魔法を唱えようとした．");	/* utcsri!carroll */
		(void) dotele();
		if(!is_pool(u.ux,u.uy))
			return(TRUE);
	}
	crawl_ok = FALSE;
	/* look around for a place to crawl to */
	for (i = 0; i < 100; i++) {
		x = rn1(3,u.ux - 1);
		y = rn1(3,u.uy - 1);
		if (goodpos(x, y, (struct monst *)0, &playermon)
			|| goodpos(x, y, (struct monst *)0, uasmon)) {
			crawl_ok = TRUE;
			goto crawl;
		}
	}
	/* one more scan */
	for (x = u.ux - 1; x <= u.ux + 1; x++)
		for (y = u.uy - 1; y <= u.uy + 1; y++)
			if (goodpos(x, y, (struct monst *)0, &playermon)
				|| goodpos(x, y, (struct monst *)0, uasmon)) {
				crawl_ok = TRUE;
				goto crawl;
			}
crawl:;
	if (crawl_ok) {
		boolean lost = FALSE;
		/* time to do some strip-tease... */
		boolean succ = Is_waterlevel(&u.uz) ? TRUE :
				emergency_disrobe(&lost);

/*JP		You("try to crawl out of the water.");*/
		You("水からはいあがろうとした．");
		if (lost)
/*JP			You("dump some of your gear to lose weight...");*/
			You("体を軽くするためいくつか物を投げすてた．．．");
		if (succ) {
/*JP			pline("Pheew!  That was close.");*/
			pline("ハァハァ！よかった．");
			teleds(x,y);
			return(TRUE);
		}
		/* still too much weight */
/*JP		pline("But in vain.");*/
		pline("が，無駄だった．");
	}
	u.uinwater = 1;
/*JP	You("drown.");*/
	You("溺れた．");
	killer_format = KILLED_BY_AN;
	killer = (levl[u.ux][u.uy].typ == POOL || Is_medusa_level(&u.uz)) ?
/*JP	    "pool of water" : "moat";*/
	    "水たまりで" : "堀で";
	done(DROWNING);
	/* oops, we're still alive.  better get out of the water. */
	while (!safe_teleds()) {
/*JP		pline("You're still drowning.");*/
		You("溺れている．");
		done(DROWNING);
	}
	u.uinwater = 0;
/*JP	You("find yourself back %s.", Is_waterlevel(&u.uz) ?
		"in an air bubble" : "on land");*/
	You("いつのまにか%sにいるのに気がついた．", Is_waterlevel(&u.uz) ?
		"空気の泡の中" : "地面");
	return(TRUE);
}

void
drain_en(n)
register int n;
{
	if (!u.uenmax) return;
/*JP	You_feel("your magical energy drain away!");*/
	You("魔法のエネルギーが吸いとられたような気がした！");
	u.uen -= n;
	if(u.uen < 0)  {
		u.uenmax += u.uen;
		if(u.uenmax < 0) u.uenmax = 0;
		u.uen = 0;
	}
	flags.botl = 1;
}

int
dountrap()	/* disarm a trap */
{
	if (near_capacity() >= HVY_ENCUMBER) {
/*JP	    pline("You're too strained to do that.");*/
	    pline("罠を解除しようにも物を持ちすぎている．");
	    return 0;
	}
	if (nohands(uasmon) || !uasmon->mmove) {
/*JP	    pline("And just how do you expect to do that?");*/
	    pline("いったい何を期待しているんだい？");
	    return 0;
	} else if (u.ustuck && sticks(uasmon)) {
/*JP	    pline("You'll have to let go of %s first.", mon_nam(u.ustuck));*/
	    pline("%sを手離さないことにはできない．", mon_nam(u.ustuck));
	    return 0;
	}
	if (u.ustuck || (welded(uwep) && bimanual(uwep))) {
/*JP	    Your("%s seem to be too busy for that.",
		 makeplural(body_part(HAND)));*/
	    Your("そんなことをする余裕なんてない．");
	    return 0;
	}
	return untrap(FALSE);
}
#endif /* OVLB */
#ifdef OVL2

/* Probability of disabling a trap.  Helge Hafting */
STATIC_OVL int
untrap_prob(ttmp)
struct trap *ttmp;
{
	int chance = 3;

	if (Confusion || Hallucination) chance++;
	if (Blind) chance++;
	if (Stunned) chance += 2;
	if (Fumbling) chance *= 2;
	/* Your own traps are better known than others. */
	if (ttmp && ttmp->madeby_u) chance--;
	if (Role_is('R')) {
	    if (rn2(2 * MAXULEV) < u.ulevel) chance--;
	    if (u.uhave.questart && chance > 1) chance--;
	}
	return rn2(chance);
}

/* Replace trap with object(s).  Helge Hafting */
STATIC_OVL void
cnv_trap_obj(otyp, cnt, ttmp)
int otyp;
int cnt;
struct trap *ttmp;
{
	struct obj *otmp = mksobj(otyp, TRUE, FALSE);
	otmp->quan=cnt;
	otmp->owt = weight(otmp);
	place_object(otmp, ttmp->tx, ttmp->ty);
	/* Sell your own traps only... */
	if (ttmp->madeby_u) sellobj(otmp, ttmp->tx, ttmp->ty);
	stackobj(otmp);
	newsym(ttmp->tx, ttmp->ty);
	deltrap(ttmp);
}

/* while attempting to disarm an adjacent trap, we've fallen into it */
STATIC_OVL void
move_into_trap(ttmp)
struct trap *ttmp;
{
	int bc;
	xchar x = ttmp->tx, y = ttmp->ty, bx, by, cx, cy;
	boolean unused;

	/* we know there's no monster in the way, and we're not trapped */
	if (!Punished || drag_ball(x, y, &bc, &bx, &by, &cx, &cy, &unused)) {
	    u.ux0 = u.ux,  u.uy0 = u.uy;
	    u.ux = x,  u.uy = y;
	    u.umoved = TRUE;
	    newsym(u.ux0, u.uy0);
	    vision_recalc(1);
	    check_leash(u.ux0, u.uy0);
	    if (Punished) move_bc(0, bc, bx, by, cx, cy);
	    spoteffects();	/* dotrap() */
	    exercise(A_WIS, FALSE);
	}
}

/* 0: doesn't even try
 * 1: tries and fails
 * 2: succeeds
 */
STATIC_OVL int
try_disarm(ttmp, force_failure)
struct trap *ttmp;
boolean force_failure;
{
	struct monst *mtmp = m_at(ttmp->tx,ttmp->ty);
	int ttype = ttmp->ttyp;
	boolean under_u = (!u.dx && !u.dy);

	/* Test for monster first, monsters are displayed instead of trap. */
	if (mtmp && (!mtmp->mtrapped || ttype != BEAR_TRAP)) {
/*JP		pline("%s is in the way.", Monnam(mtmp));*/
		pline("そこには%sがいる．", Monnam(mtmp));
		return 0;
	}
	/* We might be forced to move onto the trap's location. */
	if (sobj_at(BOULDER, ttmp->tx, ttmp->ty)
				&& !passes_walls(uasmon) && !under_u) {
/*JP		pline("There is a boulder in your way.");*/
		pline("そこには岩がある．");
		return 0;
	}
	/* untrappable traps are located on the ground. */
	if (!can_reach_floor()) {
/*JP		You("are unable to reach the %s!",*/
		You("%sに届かない！",
			jtrns_obj('^', defsyms[trap_to_defsym(ttype)].explanation));
		return 0;
	}

	/* Will our hero succeed? */
	if (force_failure || untrap_prob(ttmp)) {
		if (rnl(5)) {
/*JP		    pline("Whoops...");*/
		    pline("うわっ．．．");
		    if (mtmp) {		/* must be a bear trap */
			if (mtmp->mtame) abuse_dog(mtmp);
			if ((mtmp->mhp -= rnd(4)) <= 0) killed(mtmp);
		    } else if (under_u) {
			dotrap(ttmp);
		    } else {
			move_into_trap(ttmp);
		    }
		} else {
/*JP		    pline("%s %s is difficult to disarm.",*/
		    pline("%s%sを解除するのは困難だ．",
			  ttmp->madeby_u ? "あなたの仕掛けた" : under_u ? "この" : "その",
			  jtrns_obj('^', defsyms[trap_to_defsym(ttype)].explanation));
		}
		return 1;
	}
	return 2;
}

STATIC_OVL void
reward_untrap(ttmp, mtmp)
struct trap *ttmp;
struct monst *mtmp;
{
	if (!ttmp->madeby_u) {
		if (rnl(10)<8 && !mtmp->mpeaceful &&
						mtmp->data->mlet != S_HUMAN) {
			mtmp->mpeaceful = 1;
/*JP			pline("%s is grateful.", Monnam(mtmp));*/
			pline("%sは喜んでいる．", Monnam(mtmp));
		}
		/* Helping someone out of a trap is a nice thing to do,
		 * A lawful may be rewarded, but not too often.  */
		if (!rn2(3) && !rnl(8) && u.ualign.type == A_LAWFUL) {
			adjalign(1);
/*JP			You_feel("that you did the right thing.");*/
			You("正しいことをしたような気がした．");
		}
	}
}

STATIC_OVL int
disarm_beartrap(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
	struct monst *mtmp;
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return fails;

	/* ok, disarm it. */

	/* untrap the monster, if any.
	   There's no need for a cockatrice test, only the trap is touched */
	if ((mtmp = m_at(ttmp->tx,ttmp->ty)) != 0) {
		mtmp->mtrapped = 0;
/*JP		You("remove %s bear trap from %s.", the_your[ttmp->madeby_u],
			mon_nam(mtmp));*/
		You("%s熊の罠を%sからはずした", set_you[ttmp->madeby_u],
			mon_nam(mtmp));
		reward_untrap(ttmp, mtmp);
/*JP	} else You("disarm %s bear trap.", the_your[ttmp->madeby_u]);*/
	} else You("%s熊の罠をはずした", set_you[ttmp->madeby_u]);
	cnv_trap_obj(BEARTRAP, 1, ttmp);
	return 1;
}

STATIC_OVL int
disarm_landmine(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return fails;
/*JP	You("disarm %s land mine.", the_your[ttmp->madeby_u]);*/
	You("%s地雷を解除した．", set_you[ttmp->madeby_u]);
	cnv_trap_obj(LAND_MINE, 1, ttmp);
	return 1;
}

/* getobj will filter down to cans of grease and known potions of oil */
static NEARDATA const char oil[] = { ALL_CLASSES, TOOL_CLASS, POTION_CLASS, 0 };

/* it may not make much sense to use grease on floor boards, but so what? */
STATIC_OVL int
disarm_squeaky_board(ttmp)
struct trap *ttmp;
{
	struct obj *obj;
	boolean bad_tool;
	int fails;

/*JP	obj = getobj(oil, "untrap with");*/
	obj = getobj(oil, "使って解除する");
	if (!obj) return 0;

	bad_tool = (obj->cursed ||
			((obj->otyp != POT_OIL || obj->lamplit) &&
			 (obj->otyp != CAN_OF_GREASE || !obj->spe)));

	fails = try_disarm(ttmp, bad_tool);
	if (fails < 2) return fails;

	/* successfully used oil or grease to fix squeaky board */
	if (obj->otyp == CAN_OF_GREASE) {
	    check_unpaid(obj);
	    obj->spe--;
	} else {
	    useup(obj);	/* oil */
	    makeknown(POT_OIL);
	}
/*JP	You("repair the squeaky board.");*/	/* no madeby_u */
	You("きしむ板を修理した．");	/* no madeby_u */
	deltrap(ttmp);
	newsym(u.ux + u.dx, u.uy + u.dy);
	more_experienced(1, 5);
	return 1;
}

/* removes traps that shoot arrows, darts, etc. */
STATIC_OVL int
disarm_shooting_trap(ttmp, otyp)
struct trap *ttmp;
int otyp;
{
	int fails = try_disarm(ttmp, FALSE);

	if (fails < 2) return fails;
/*JP	You("disarm %s trap.", the_your[ttmp->madeby_u]);*/
	pline("%s罠を解除した．", set_you[ttmp->madeby_u]);
	cnv_trap_obj(otyp, 50-rnl(50), ttmp);
	return 1;
}

/* Is the weight too heavy?
 * Formula as in near_capacity() & check_capacity() */
STATIC_OVL int
try_lift(mtmp, ttmp, wt, stuff)
struct monst *mtmp;
struct trap *ttmp;
int wt;
boolean stuff;
{
	int wc = weight_cap();

	if ((((wt<<1) / wc)+1) >= EXT_ENCUMBER) {
/*JP		pline("%s is %s for you to lift.", Monnam(mtmp),
			stuff ? "carrying too much" : "too heavy");*/
		pline("%sは%s持ちあげることができない．", Monnam(mtmp),
		  stuff ? "物を持ちすぎており" : "重すぎて");
		if (!ttmp->madeby_u && !mtmp->mpeaceful
			&& mtmp->data->mlet != S_HUMAN && rnl(10) < 3) {
		    mtmp->mpeaceful = 1;
/*JP		    pline("%s thinks it was nice of you to try.", Monnam(mtmp));*/
		    pline("%sはあなたの努力に感謝しているようだ．", Monnam(mtmp));
		}
		return 0;
	}
	return 1;
}

/* Help trapped monster (out of a (spiked) pit) */
STATIC_OVL int
help_monster_out(mtmp, ttmp)
struct monst *mtmp;
struct trap *ttmp;
{
	int wt;
	struct obj *otmp;

	/*
	 * This works when levitating too -- consistent with the ability
	 * to hit monsters while levitating.
	 *
	 * Should perhaps check that our hero has arms/hands at the
	 * moment.  Helping can also be done by engulfing...
	 *
	 * Test the monster first - monsters are displayed before traps.
	 */
	if (!mtmp->mtrapped) {
/*JP		pline("%s isn't trapped.", Monnam(mtmp));*/
		pline("%sは罠にかかっていない．", Monnam(mtmp));
		return 0;
	}
	/* Do you have the necessary capacity to lift anything? */
	if (check_capacity((char *)0)) return 1;

	/* Will our hero succeed? */
	if (untrap_prob(ttmp)) {
/*JP		You("try to reach out your %s, but %s backs away skeptically.",
			makeplural(body_part(ARM)),
			mon_nam(mtmp));*/
		You("%sを差し延べようとしたが%sは警戒している．",
			makeplural(body_part(ARM)),
			mon_nam(mtmp));
		return 1;
	}


	/* is it a cockatrice?... */
	if (mtmp->data == &mons[PM_COCKATRICE] && !uarmg
		    && !resists_ston(&youmonst)) {
/*JP
		You("grab the trapped cockatrice using your bare %s.",
*/
		You("罠にかかっているコカトリスを素%sで掴んでしまった．",
			makeplural(body_part(HAND)));

		if (poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM))
			display_nhwindow(WIN_MESSAGE, FALSE);
		else {
/*JP
		      instapetrify("trying to help a cockatrice out of a pit");
*/
		      instapetrify("罠にかかっているコカトリスを助けようとして");
			return 1;
		}
	}
/*JP
	You("reach out your %s and grab %s.",
	    makeplural(body_part(ARM)), mon_nam(mtmp));
*/
	You("%sに%sを差し延べた．",
	    mon_nam(mtmp), makeplural(body_part(ARM))); 

	/* is the monster too heavy? */
	wt = inv_weight() + mtmp->data->cwt;
	if (!try_lift(mtmp, ttmp, wt, FALSE)) return 1;

	/* is the monster with inventory too heavy? */
	for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		wt += otmp->owt;
	if (!try_lift(mtmp, ttmp, wt, TRUE)) return 1;

/*JP	You("pull %s out of the pit.", mon_nam(mtmp));*/
	You("%sを落し穴からひっぱった．", mon_nam(mtmp));
	mtmp->mtrapped = 0;
	fill_pit(mtmp->mx, mtmp->my);
	reward_untrap(ttmp, mtmp);
	return 1;
}

int
untrap(force)
boolean force;
{
	register struct obj *otmp;
	register boolean confused = (Confusion > 0 || Hallucination > 0);
	register int x,y;
	int ch;
	struct trap *ttmp;
	struct monst *mtmp;
	boolean trap_skipped = FALSE;

	if(!getdir((char *)0)) return(0);
	x = u.ux + u.dx;
	y = u.uy + u.dy;

	if ((ttmp = t_at(x,y)) && ttmp->tseen) {
		if (u.utrap) {
/*JP			You("cannot deal with traps while trapped!");*/
			pline("罠にかかっている間は罠を解除できない！");
			return 1;
		}
		switch(ttmp->ttyp) {
			case BEAR_TRAP:
				return disarm_beartrap(ttmp);
			case LANDMINE:
				return disarm_landmine(ttmp);
			case SQKY_BOARD:
				return disarm_squeaky_board(ttmp);
			case DART_TRAP:
				return disarm_shooting_trap(ttmp, DART);
			case ARROW_TRAP:
				return disarm_shooting_trap(ttmp, ARROW);
			case PIT:
			case SPIKED_PIT:
				if (!u.dx && !u.dy) {
/*JP				    You("are already on the edge of the pit.");*/
				    You("もう落し穴の端にいる．");
				    return 0;
				}
				if (!(mtmp = m_at(x,y))) {
/*JP				    pline("Try filling the pit instead.");*/
				    pline("なんとか埋めることを考えてみたら？");
				    return 0;
				}
				return help_monster_out(mtmp, ttmp);
			default:
/*JP				You("cannot disable %s trap.", (u.dx || u.dy) ? "that" : "this");*/
				pline("%s罠は解除できない．", (u.dx || u.dy) ? "その" : "この");
				return 0;
		} /* end switch */
	} /* end if */

	if(!u.dx && !u.dy) {
	    for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if(Is_box(otmp)) {
/*JP		    pline("There is %s here.", doname(otmp));*/
		    pline("ここには%sがある．", doname(otmp));

/*JP		    switch (ynq("Check for traps?")) {*/
		    switch (ynq("罠を調べますか？")) {
			case 'q': return(0);
			case 'n': continue;
		    }

		    if((otmp->otrapped && (force || (!confused
				&& rn2(MAXULEV + 1 - u.ulevel) < 10)))
		       || (!force && confused && !rn2(3))) {
/*JP			You("find a trap on %s!", the(xname(otmp)));*/
			pline("%sに罠を発見した！", the(xname(otmp)));
			exercise(A_WIS, TRUE);

/*JP			switch (ynq("Disarm it?")) {*/
			switch (ynq("解除しますか？")) {
			    case 'q': return(1);
			    case 'n': trap_skipped = TRUE;  continue;
			}

			if(otmp->otrapped) {
			    exercise(A_DEX, TRUE);
			    ch = ACURR(A_DEX) + u.ulevel;
			    if (Role_is('R')) ch *= 2;
			    if(!force && (confused || Fumbling ||
				rnd(75+level_difficulty()/2) > ch)) {
				(void) chest_trap(otmp, FINGER, TRUE);
			    } else {
/*JP				You("disarm it!");*/
				pline("解除した！");
				otmp->otrapped = 0;
			    }
/*JP			} else pline("That %s was not trapped.", doname(otmp));*/
			} else pline("その%sに罠はない．", doname(otmp));
			return(1);
		    } else {
/*JP			You("find no traps on %s.", the(xname(otmp)));*/
			pline("罠を発見できなかった．");
			return(1);
		    }
		}

/*JP	    You(trap_skipped ? "find no other traps here."
			     : "know of no traps here.");*/
	    You(trap_skipped ? "他の罠を見つけられなかった．"
			     : "ここに罠がないことを知っている．");
	    return(0);
	}

	if ((mtmp = m_at(x,y))				&&
		mtmp->m_ap_type == M_AP_FURNITURE	&&
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp);
	    return(1);
	}

	if (!IS_DOOR(levl[x][y].typ)) {
	    if ((ttmp = t_at(x,y)) && ttmp->tseen)
/*JP		You("cannot disable that trap.");*/
		You("罠を解除できなかった．");
	    else
/*JP		You("know of no traps there.");*/
		You("そこに罠がないことを知っている．");
	    return(0);
	}

	switch (levl[x][y].doormask) {
	    case D_NODOOR:
/*JP		You("%s no door there.", Blind ? "feel" : "see");*/
		pline("そこには扉がない%s．", Blind ? "ようだ" : "ように見える");
		return(0);
	    case D_ISOPEN:
/*JP		pline("This door is safely open.");*/
		pline("その扉は安全に開いている．");
		return(0);
	    case D_BROKEN:
/*JP		pline("This door is broken.");*/
		pline("その扉は壊れている．");
		return(0);
	}

	if ((levl[x][y].doormask & D_TRAPPED
	     && (force ||
		 (!confused && rn2(MAXULEV - u.ulevel + 11) < 10)))
	    || (!force && confused && !rn2(3))) {
/*JP		You("find a trap on the door!");*/
		pline("扉に罠を発見した！");
		exercise(A_WIS, TRUE);
/*JP		if (ynq("Disarm it?") != 'y') return(1);*/
		if (ynq("解除しますか？") != 'y') return(1);
		if (levl[x][y].doormask & D_TRAPPED) {
		    ch = 15 + (Role_is('R') ? u.ulevel*3 : u.ulevel);
		    exercise(A_DEX, TRUE);
		    if(!force && (confused || Fumbling ||
				     rnd(75+level_difficulty()/2) > ch)) {
/*JP			You("set it off!");*/
		        You("スイッチを入れてしまった！");
/*JP			b_trapped("door", FINGER);*/
			b_trapped("扉", FINGER);
			levl[x][y].doormask = D_NODOOR;
			unblock_point(x, y);
			newsym(x, y);
			/* (probably ought to charge for this damage...) */
			if (*in_rooms(x, y, SHOPBASE)) add_damage(x, y, 0L);
		    } else {
/*JP			You("disarm it!");*/
		        You("解除した！");
			levl[x][y].doormask &= ~D_TRAPPED;
		    }
/*JP		} else pline("This door was not trapped.");*/
		} else pline("扉に罠はなかった．");
		return(1);
	} else {
/*JP		You("find no traps on the door.");*/
		pline("扉に罠を発見できなかった．");
		return(1);
	}
}
#endif /* OVL2 */
#ifdef OVLB

/* only called when the player is doing something to the chest directly */
boolean
chest_trap(obj, bodypart, disarm)
register struct obj *obj;
register int bodypart;
boolean disarm;
{
	register struct obj *otmp = obj, *otmp2;
	char	buf[80];
	const char *msg;
	coord cc;

	if (get_obj_location(obj, &cc.x, &cc.y, 0))	/* might be carried */
	    obj->ox = cc.x,  obj->oy = cc.y;

	otmp->otrapped = 0;	/* trap is one-shot; clear flag first in case
				   chest kills you and ends up in bones file */
/*JP
	You(disarm ? "set it off!" : "trigger a trap!");
*/
	You(disarm ? "スイッチを入れてしまった！" : "罠にひっかかった！");
	display_nhwindow(WIN_MESSAGE, FALSE);
	if (Luck > -13 && rn2(13+Luck) > 7) {	/* saved by luck */
	    /* trap went off, but good luck prevents damage */
	    switch (rn2(13)) {
		case 12:
/*JP		case 11:  msg = "explosive charge is a dud";  break;*/
		case 11:  msg = "爆発は不発だった";  break;
		case 10:
/*JP		case  9:  msg = "electric charge is grounded";  break;*/
		case  9:  msg = "電撃が放出されたがアースされていた";  break;
		case  8:
/*JP		case  7:  msg = "flame fizzles out";  break;*/
		case  7:  msg = "炎はシューっと消えた";  break;
		case  6:
		case  5:
/*JP		case  4:  msg = "poisoned needle misses";  break;*/
		case  4:  msg = "毒針は刺さらなかった";  break;
		case  3:
		case  2:
		case  1:
/*JP		case  0:  msg = "gas cloud blows away";  break;*/
		case  0:  msg = "ガス雲は吹き飛んだ";  break;
		default:  impossible("chest disarm bug");  msg = (char *)0;
			  break;
	    }
/*JP
	    if (msg) pline("But luckily the %s!", msg);
*/
	    if (msg) pline("が運のよいことに%s！", msg);
	} else {
	    switch(rn2(20) ? ((Luck >= 13) ? 0 : rn2(13-Luck)) : rn2(26)) {
		case 25:
		case 24:
		case 23:
		case 22:
		case 21: {
			  struct monst *shkp = 0;
			  long loss = 0L;
			  boolean costly, insider;
			  register xchar ox = obj->ox, oy = obj->oy;

			  /* the obj location need not be that of player */
			  costly = (costly_spot(ox, oy) &&
				   (shkp = shop_keeper(*in_rooms(ox, oy,
				    SHOPBASE))) != (struct monst *)0);
			  insider = (*u.ushops && inside_shop(u.ux, u.uy) &&
				    *in_rooms(ox, oy, SHOPBASE) == *u.ushops);

/*JP			  pline("%s explodes!", The(xname(obj)));*/
			  pline("%sは爆発した！", The(xname(obj)));
/*JP			  Sprintf(buf, "exploding %s", xname(obj));*/
			  Sprintf(buf, "%sの爆発で", xname(obj));

			  if(costly)
			      loss += stolen_value(obj, ox, oy,
						(boolean)shkp->mpeaceful, TRUE);
			  delete_contents(obj);
			  for(otmp = level.objects[u.ux][u.uy];
							otmp; otmp = otmp2) {
			      otmp2 = otmp->nexthere;
			      if(costly)
				  loss += stolen_value(otmp, otmp->ox,
					  otmp->oy, (boolean)shkp->mpeaceful,
					  TRUE);
			      delobj(otmp);
			  }
			  wake_nearby();
			  losehp(d(6,6), buf, KILLED_BY_AN);
			  exercise(A_STR, FALSE);
			  if(costly && loss) {
			      if(insider)
/*JP			      You("owe %ld zorkmids for objects destroyed.",
							loss);*/
			      You("器物破損で%ldゴールドの借りをつくった．",
							loss);
			      else {
/*JP				  You("caused %ld zorkmids worth of damage!",*/
				  You("%ldゴールド分の損害を引きおこした！",
							loss);
				  make_angry_shk(shkp, ox, oy);
			      }
			  }
			  return TRUE;
			}
		case 20:
		case 19:
		case 18:
		case 17:
/*JP			pline("A cloud of noxious gas billows from %s.",*/
			pline("有毒ガスが%sから渦まいた．",
							the(xname(obj)));
/*JP			poisoned("gas cloud", A_STR, "cloud of poison gas",15);*/
			poisoned("ガス雲", A_STR, "ガス雲",15);
			exercise(A_CON, FALSE);
			break;
		case 16:
		case 15:
		case 14:
		case 13:
/*JP			You_feel("a needle prick your %s.",body_part(bodypart));*/
			You("%sチクッという痛みを感じた．", body_part(bodypart));
/*JP			poisoned("needle", A_CON, "poisoned needle",10);*/
			poisoned("針", A_CON, "毒針",10);
			exercise(A_CON, FALSE);
			break;
		case 12:
		case 11:
		case 10:
		case 9:
			dofiretrap(obj);
			break;
		case 8:
		case 7:
		case 6: {
			int dmg;

/*JP			You("are jolted by a surge of electricity!");*/
			You("電気ショックをくらった！");
			if(Shock_resistance)  {
			    shieldeff(u.ux, u.uy);
/*JP			    You("don't seem to be affected.");*/
			    pline("しかしあなたは影響を受けない．");
			    dmg = 0;
			} else
			    dmg = d(4, 4);
			destroy_item(RING_CLASS, AD_ELEC);
			destroy_item(WAND_CLASS, AD_ELEC);
/*JP			if (dmg) losehp(dmg, "electric shock", KILLED_BY_AN);*/
			if (dmg) losehp(dmg, "電気ショックで", KILLED_BY_AN);
			break;
		      }
		case 5:
		case 4:
		case 3:
/*JP			pline("Suddenly you are frozen in place!");*/
			pline("突然その場で動けなくなった！");
			nomul(-d(5, 6));
			exercise(A_DEX, FALSE);
/*JP			nomovemsg = You_can_move_again;*/
			nomovemsg = "また動けるようになった．";
			break;
		case 2:
		case 1:
		case 0:
/*JP			pline("A cloud of %s gas billows from %s.",
						hcolor((char *)0),
						the(xname(obj)));*/
			pline("%s光るガス雲が%sの底で渦まいた．",
						jconj_adj(hcolor((char *)0)),
						the(xname(obj)));
			if(!Stunned) {
			    if (Hallucination)
/*JP				pline("What a groovy feeling!");*/
				pline("なんて素敵なんだ！");
			    else if (Blind)
/*JP				You("stagger and get dizzy...");*/
				You("くらくらし，めまいがした．．．");
			    else
/*JP				You("stagger and your vision blurs...");*/
				You("くらくらし，景色がぼやけてきた．．．");
			}
			make_stunned(HStun + rn1(7, 16),FALSE);
			make_hallucinated(HHallucination + rn1(5, 16),FALSE,0L);
			break;
		default: impossible("bad chest trap");
			break;
	    }
	    bot();			/* to get immediate botl re-display */
	}

	return FALSE;
}

#endif /* OVLB */
#ifdef OVL0

struct trap *
t_at(x,y)
register int x, y;
{
	register struct trap *trap = ftrap;
	while(trap) {
		if(trap->tx == x && trap->ty == y) return(trap);
		trap = trap->ntrap;
	}
	return((struct trap *)0);
}

#endif /* OVL0 */
#ifdef OVLB

void
deltrap(trap)
register struct trap *trap;
{
	register struct trap *ttmp;

	if(trap == ftrap)
		ftrap = ftrap->ntrap;
	else {
		for(ttmp = ftrap; ttmp->ntrap != trap; ttmp = ttmp->ntrap) ;
		ttmp->ntrap = trap->ntrap;
	}
	dealloc_trap(trap);
}

boolean delfloortrap(ttmp)
register struct trap *ttmp;
{
	/* Destroy a trap that emanates from the floor. */
	/* some of these are arbitrary -dlc */
	if (ttmp && ((ttmp->ttyp == SQKY_BOARD) ||
		     (ttmp->ttyp == BEAR_TRAP) ||
		     (ttmp->ttyp == LANDMINE) ||
		     (ttmp->ttyp == FIRE_TRAP) ||
		     (ttmp->ttyp == PIT) ||
		     (ttmp->ttyp == SPIKED_PIT) ||
		     (ttmp->ttyp == HOLE) ||
		     (ttmp->ttyp == TRAPDOOR) ||
		     (ttmp->ttyp == TELEP_TRAP) ||
		     (ttmp->ttyp == LEVEL_TELEP) ||
		     (ttmp->ttyp == WEB) ||
		     (ttmp->ttyp == MAGIC_TRAP) ||
		     (ttmp->ttyp == ANTI_MAGIC))) {
	    register struct monst *mtmp;

	    if (ttmp->tx == u.ux && ttmp->ty == u.uy) {
		u.utrap = 0;
		u.utraptype = 0;
	    } else if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
		mtmp->mtrapped = 0;
	    }
	    deltrap(ttmp);
	    return TRUE;
	} else
	    return FALSE;
}

/* used for doors (also tins).  can be used for anything else that opens. */
void
b_trapped(item, bodypart)
register const char *item;
register int bodypart;
{
	register int lvl = level_difficulty();
	int dmg = rnd(5 + (lvl < 5 ? lvl : 2+lvl/2));

/*JP	pline("KABOOM!!  %s was booby-trapped!", The(item));*/
	pline("ちゅどーん！！%sにブービートラップが仕掛けられていた！", The(item));
	wake_nearby();
/*JP	losehp(dmg, "explosion", KILLED_BY_AN);*/
	losehp(dmg, "ブービートラップの爆発で", KILLED_BY_AN);
	exercise(A_STR, FALSE);
	if (bodypart) exercise(A_CON, FALSE);
	make_stunned(HStun + dmg, TRUE);
}

/* Monster is hit by trap. */
/* Note: doesn't work if both obj and d_override are null */
STATIC_OVL boolean
thitm(tlev, mon, obj, d_override)
register int tlev;
register struct monst *mon;
register struct obj *obj;
int d_override;
{
	register int strike;
	register boolean trapkilled = FALSE;

	if (d_override) strike = 1;
	else if (obj) strike = (find_mac(mon) + tlev + obj->spe <= rnd(20));
	else strike = (find_mac(mon) + tlev <= rnd(20));

	/* Actually more accurate than thitu, which doesn't take
	 * obj->spe into account.
	 */
	if(!strike) {
		if (cansee(mon->mx, mon->my))
/*JP			pline("%s is almost hit by %s!", Monnam(mon),
								doname(obj));*/
			pline("もう少しで%sが%sに命中するところだった！", doname(obj),
			      Monnam(mon));
	} else {
		int dam = 1;

		if (obj && cansee(mon->mx, mon->my))
/*JP			pline("%s is hit by %s!", Monnam(mon), doname(obj));*/
			pline("%sが%sに命中した！", doname(obj), Monnam(mon));
		if (d_override) dam = d_override;
		else if (obj) {
			dam = dmgval(obj, mon);
			if (dam < 1) dam = 1;
		}
		if ((mon->mhp -= dam) <= 0) {
			int xx = mon->mx;
			int yy = mon->my;

			monkilled(mon, "", AD_PHYS);
			if (mon->mhp <= 0) {
				newsym(xx, yy);
				trapkilled = TRUE;
			}
		}
	}
	if (obj && (!strike || d_override)) {
		place_object(obj, mon->mx, mon->my);
		stackobj(obj);
	} else if (obj) dealloc_obj(obj);

	return trapkilled;
}

boolean
unconscious()
{
/*
**	もう，かんべんしてよー．こんなコーディング．．．
**
**	You regain con  --  eat.c(2), potion.c(1)
**	You are consci  --  eat.c(2)
*/
	return((boolean)(multi < 0 && (!nomovemsg ||
		u.usleep ||
		!strncmp(nomovemsg,"あなたは正気づいた", 18) ||
		!strncmp(nomovemsg,"あなたはまた正気づ", 18) ||
		!strncmp(nomovemsg,"You regain con", 15) ||
		!strncmp(nomovemsg,"You are consci", 15))));
}

/*JPstatic char lava_killer[] = "molten lava";*/
static char lava_killer[] = "どろどろの溶岩で";

boolean
lava_effects()
{
    register struct obj *obj, *obj2;
    int dmg;

    if (likes_lava(uasmon)) return FALSE;

    if (!Fire_resistance) {
	if(Wwalking) {
	    dmg = d(6,6);
/*JP	    pline_The("lava here burns you!");*/
	    pline("溶岩があなたを焼きつくした！");
	    if(dmg < u.uhp) {
		losehp(dmg, lava_killer, KILLED_BY);
		goto burn_stuff;
	    }
	} else
/*JP	    You("fall into the lava!");*/
	    You("溶岩に落ちた！");

	for(obj = invent; obj; obj = obj2) {
	    obj2 = obj->nobj;
	    if(is_organic(obj) && !obj->oerodeproof) {
		if(obj->owornmask) {
		    if(obj == uarm) (void) Armor_gone();
		    else if(obj == uarmc) (void) Cloak_off();
		    else if(obj == uarmh) (void) Helmet_off();
		    else if(obj == uarms) (void) Shield_off();
		    else if(obj == uarmg) (void) Gloves_off();
		    else if(obj == uarmf) (void) Boots_off();
#ifdef TOURIST
		    else if(obj == uarmu) setnotworn(obj);
#endif
		    else if(obj == uleft) Ring_gone(obj);
		    else if(obj == uright) Ring_gone(obj);
		    else if(obj == ublindf) Blindf_off(obj);
		    else if(obj == uwep) uwepgone();
		    if(Lifesaved
#ifdef WIZARD
		       || wizard
#endif
/*JP		       ) Your("%s into flame!", aobjnam(obj, "burst"));*/
		       ) Your("%sは燃えた！", xname(obj));
		}
		useup(obj);
	    }
	}

	/* s/he died... */
	u.uhp = -1;
	killer_format = KILLED_BY;
	killer = lava_killer;
/*JP	You("burn to a crisp...");*/
	You("燃えてパリパリになった．．．");
	done(BURNING);
	while (!safe_teleds()) {
/*JP		pline("You're still burning.");*/
		You("まだ燃えている．");
		done(BURNING);
	}
/*JP	You("find yourself back on solid %s.", surface(u.ux, u.uy));*/
	You("いつのまにか固い%sに戻っていた．", surface(u.ux, u.uy));
	return(TRUE);
    }

    if (!Wwalking) {
	u.utrap = rn1(4, 4) + (rn1(4, 12) << 8);
	u.utraptype = TT_LAVA;
/*JP	You("sink into the lava, but it only burns slightly!");*/
	You("溶岩に沈んだが，ちょっと焦げただけだ！");
	if (u.uhp > 1)
	    losehp(1, lava_killer, KILLED_BY);
    }
    /* just want to burn boots, not all armor; destroy_item doesn't work on
       armor anyway */
burn_stuff:
    if(uarmf && !uarmf->oerodeproof && is_organic(uarmf)) {
	/* save uarmf value because Boots_off() sets uarmf to null */
	obj = uarmf;
/*JP	Your("%s burst into flame!", xname(obj));*/
	Your("%sは燃えた！", xname(obj));
	(void) Boots_off();
	useup(obj);
    }
    destroy_item(SCROLL_CLASS, AD_FIRE);
    destroy_item(SPBOOK_CLASS, AD_FIRE);
    destroy_item(POTION_CLASS, AD_FIRE);
    return(FALSE);
}

#endif /* OVLB */

/*trap.c*/
