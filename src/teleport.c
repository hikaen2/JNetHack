/*	SCCS Id: @(#)teleport.c	3.2	96/05/03	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

static boolean FDECL(tele_jump_ok, (int,int,int,int));
static boolean FDECL(teleok, (int,int,BOOLEAN_P));
static void NDECL(vault_tele);
static boolean FDECL(rloc_pos_ok, (int,int,struct monst *));
static void FDECL(mvault_tele, (struct monst *));

boolean
goodpos(x, y, mtmp, mdat)
int x,y;
struct monst *mtmp;	/* existing monster being moved, if any */
struct permonst *mdat;
{
	struct monst *mtmp2;

	if (!isok(x, y)) return FALSE;

	/* in many cases, we're trying to create a new monster, which
	 * can't go on top of the player or any existing monster.
	 * however, occasionally we are relocating engravings or objects,
	 * which could be co-located and thus get restricted a bit too much.
	 * oh well.
	 */
	if ((!mdat || mdat != &playermon) && x == u.ux && y == u.uy)
		return FALSE;
	if ((mtmp || mdat) && (mtmp2 = m_at(x, y)) != 0 && mtmp != mtmp2)
		return FALSE;

	if (mdat) {
	    if (is_pool(x,y)) {
		if (mdat == &playermon)
			return !!(HLevitation || Wwalking || Amphibious);
		else	return (is_flyer(mdat) || is_swimmer(mdat) ||
							is_clinger(mdat));
	    } else if (mdat->mlet == S_EEL && rn2(13)) {
		return FALSE;
	    } else if (is_lava(x,y)) {
		if (mdat == &playermon)
		    return !!HLevitation;
		else
		    return (is_flyer(mdat) || likes_lava(mdat));
	    }
	    if (passes_walls(mdat) && may_passwall(x,y)) return TRUE;
	}
	if (!ACCESSIBLE(levl[x][y].typ)) return FALSE;
	if (closed_door(x, y) && (!mdat || !amorphous(mdat)))
		return FALSE;
	if (sobj_at(BOULDER, x, y) && (!mdat || !throws_rocks(mdat)))
		return FALSE;
	return TRUE;
}

/*
 * "entity next to"
 *
 * Attempt to find a good place for the given monster type in the closest
 * position to (xx,yy).  Do so in successive square rings around (xx,yy).
 * If there is more than one valid positon in the ring, choose one randomly.
 * Return TRUE and the position chosen when successful, FALSE otherwise.
 */
boolean
enexto(cc, xx, yy, mdat)
coord *cc;
register xchar xx, yy;
struct permonst *mdat;
{
#define MAX_GOOD 15
    coord good[MAX_GOOD], *good_ptr;
    int x, y, range, i;
    int xmin, xmax, ymin, ymax;

    good_ptr = good;
    range = 1;
    /*
     * Walk around the border of the square with center (xx,yy) and
     * radius range.  Stop when we find at least one valid position.
     */
    do {
	xmin = max(1, xx-range);
	xmax = min(COLNO-1, xx+range);
	ymin = max(0, yy-range);
	ymax = min(ROWNO-1, yy+range);

	for (x = xmin; x <= xmax; x++)
	    if (goodpos(x, ymin, (struct monst *)0, mdat)) {
		good_ptr->x = x;
		good_ptr->y = ymin ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[MAX_GOOD-1]) goto full;
	    }
	for (x = xmin; x <= xmax; x++)
	    if (goodpos(x, ymax, (struct monst *)0, mdat)) {
		good_ptr->x = x;
		good_ptr->y = ymax ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[MAX_GOOD-1]) goto full;
	    }
	for (y = ymin+1; y < ymax; y++)
	    if (goodpos(xmin, y, (struct monst *)0, mdat)) {
		good_ptr->x = xmin;
		good_ptr-> y = y ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[MAX_GOOD-1]) goto full;
	    }
	for (y = ymin+1; y < ymax; y++)
	    if (goodpos(xmax, y, (struct monst *)0, mdat)) {
		good_ptr->x = xmax;
		good_ptr->y = y ;
		/* beware of accessing beyond segment boundaries.. */
		if (good_ptr++ == &good[MAX_GOOD-1]) goto full;
	    }
	range++;

	/* return if we've grown too big (nothing is valid) */
	if (range > ROWNO && range > COLNO) return FALSE;
    } while (good_ptr == good);

full:
    i = rn2((int)(good_ptr - good));
    cc->x = good[i].x;
    cc->y = good[i].y;
    return TRUE;
}

/*
 * Check for restricted areas present in some special levels.  (This might
 * need to be augmented to allow deliberate passage in wizard mode, but
 * only for explicitly chosen destinations.)
 */
static boolean
tele_jump_ok(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
	if (dndest.nlx > 0) {
	    /* if inside a restricted region, can't teleport outside */
	    if (within_bounded_area(x1, y1, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy) &&
		!within_bounded_area(x2, y2, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy))
		return FALSE;
	    /* and if outside, can't teleport inside */
	    if (!within_bounded_area(x1, y1, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy) &&
		within_bounded_area(x2, y2, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy))
		return FALSE;
	}
	if (updest.nlx > 0) {		/* ditto */
	    if (within_bounded_area(x1, y1, updest.nlx, updest.nly,
						updest.nhx, updest.nhy) &&
		!within_bounded_area(x2, y2, updest.nlx, updest.nly,
						updest.nhx, updest.nhy))
		return FALSE;
	    if (!within_bounded_area(x1, y1, updest.nlx, updest.nly,
						updest.nhx, updest.nhy) &&
		within_bounded_area(x2, y2, updest.nlx, updest.nly,
						updest.nhx, updest.nhy))
		return FALSE;
	}
	return TRUE;
}

static boolean
teleok(x, y, trapok)
register int x, y;
boolean trapok;
{
	if (!trapok && t_at(x, y)) return FALSE;
	if (!goodpos(x, y, (struct monst *)0, &playermon)
			&& !goodpos(x, y, (struct monst *)0, uasmon))
		return FALSE;
	if (!tele_jump_ok(u.ux, u.uy, x, y)) return FALSE;
	return TRUE;
}

void
teleds(nux, nuy)
register int nux,nuy;
{
	if (Punished) unplacebc();
	u.utrap = 0;
	u.ustuck = 0;
	u.ux0 = u.ux;
	u.uy0 = u.uy;
	u.ux = nux;
	u.uy = nuy;
	fill_pit(u.ux0, u.uy0); /* do this now so that cansee() is correct */

	if (hides_under(uasmon))
		u.uundetected = OBJ_AT(nux, nuy);
	else if (uasmon->mlet == S_EEL)
		u.uundetected = is_pool(u.ux, u.uy);
	else
		u.uundetected = 0;
	if (u.usym == S_MIMIC_DEF) u.usym = S_MIMIC;

	if (u.uswallow) {
		u.uswldtim = u.uswallow = 0;
		docrt();
	}
	if (Punished) placebc();
	initrack(); /* teleports mess up tracking monsters without this */
	/*
	 *  Make sure the hero disappears from the old location.  This will
	 *  not happen if she is teleported within sight of her previous
	 *  location.  Force a full vision recalculation because the hero
	 *  is now in a new location.
	 */
	newsym(u.ux0,u.uy0);
	vision_full_recalc = 1;
	nomul(0);
	spoteffects();
	invocation_message();
}

boolean
safe_teleds()
{
	register int nux, nuy, tcnt = 0;

	do {
		nux = rnd(COLNO-1);
		nuy = rn2(ROWNO);
	} while (!teleok(nux, nuy, (boolean)(tcnt > 200)) && ++tcnt <= 400);

	if (tcnt <= 400) {
		teleds(nux, nuy);
		return TRUE;
	} else
		return FALSE;
}

static void
vault_tele()
{
	register struct mkroom *croom = search_special(VAULT);
	coord c;

	if (croom && somexy(croom, &c) && teleok(c.x,c.y,FALSE)) {
		teleds(c.x,c.y);
		return;
	}
	tele();
}

boolean
teleport_pet(mtmp)
register struct monst *mtmp;
{
	register struct obj *otmp;

	if (mtmp->mleashed) {
	    otmp = get_mleash(mtmp);
	    if (!otmp) {
		impossible("%s is leashed, without a leash.", Monnam(mtmp));
		goto release_it;
	    }
	    if (otmp->cursed) {
		yelp(mtmp);
		return FALSE;
	    } else {
/*JP		Your("leash goes slack.");*/
		Your("紐はたるんだ．");
 release_it:
		m_unleash(mtmp);
		return TRUE;
	    }
	}
	return TRUE;
}

void
tele()
{
	coord cc;

	/* Disable teleportation in stronghold && Vlad's Tower */
	if (level.flags.noteleport) {
#ifdef WIZARD
		if (!wizard) {
#endif
/*JP		    pline("A mysterious force prevents you from teleporting!");*/
		    pline("奇妙な力が瞬間移動を防いだ！");
		    return;
#ifdef WIZARD
		}
#endif
	}

	/* don't show trap if "Sorry..." */
	if (!Blinded) make_blinded(0L,FALSE);

	if ((u.uhave.amulet || On_W_tower_level(&u.uz)) && !rn2(3)) {
/*JP	    You_feel("disoriented for a moment.");*/
	    You("一瞬方向感覚を失ったような気がした．");
	    return;
	}
	if (Teleport_control
#ifdef WIZARD
			    || wizard
#endif
					) {
	    if (unconscious()) {
/*JP		pline("Being unconscious, you cannot control your teleport.");*/
		pline("意識がないので，あなたは瞬間移動を制御できない．");
	    } else {
/*JP		    pline("To what position do you want to be teleported?");*/
		    pline("どの位置に瞬間移動しますか？");
		    cc.x = u.ux;
		    cc.y = u.uy;
		    getpos(&cc, TRUE, "the desired position");/* force valid*/
		    if (cc.x == -10) return; /* abort */
		    /* possible extensions: introduce a small error if
		       magic power is low; allow transfer to solid rock */
		    if (teleok(cc.x, cc.y, FALSE)) {
			teleds(cc.x, cc.y);
			return;
		    }
/*JP		    pline("Sorry...");*/
		    pline("おっと．．．");
		}
	}

	(void) safe_teleds();
}

int
dotele()
{
	struct trap *trap;

	trap = t_at(u.ux, u.uy);
	if (trap && (!trap->tseen || trap->ttyp != TELEP_TRAP))
		trap = 0;

	if (trap) {
		if (trap->once) {
/*JP			pline("This is a vault teleport, usable once only.");*/
			pline("一度かぎりの倉庫への瞬間移動の罠だ．");
/*JP			if (yn("Jump in?") == 'n')*/
			if (yn("飛び込む？") == 'n')
				trap = 0;
			else {
				deltrap(trap);
				newsym(u.ux, u.uy);
			}
		}
		if (trap)
		  {
		    char buf[BUFSIZ];
/*JP
			You("%s onto the teleportation trap.",
			    locomotion(uasmon, "jump"));*/
		    Sprintf(buf,jconj(locomotion(uasmon,"飛ぶ"),"ま"));
		    buf[strlen(buf)-2] = '\0';
			You("瞬間移動の罠に%sこんだ．",buf);
		  }
	}
	if (!trap) {
	    boolean castit = FALSE;
	    register int sp_no = 0, energy = 0;

	    if (!Teleportation || (u.ulevel < (Role_is('W') ? 8 : 12)
					&& !can_teleport(uasmon))) {
		/* Try to use teleport away spell. */
		if (objects[SPE_TELEPORT_AWAY].oc_name_known && !Confusion)
		    for (sp_no = 0; sp_no < MAXSPELL; sp_no++)
			if (spl_book[sp_no].sp_id == SPE_TELEPORT_AWAY) {
				castit = TRUE;
				break;
			}
#ifdef WIZARD
		if (!wizard) {
#endif
		    if (!castit) {
			if (!Teleportation)
/*JP			    You("don't know that spell.");*/
			    You("そんな魔法は知らない．");
/*JP			else You("are not able to teleport at will.");*/
			else You("自分の意思で瞬間移動できない．");
			return(0);
		    }
#ifdef WIZARD
		}
#endif
	    }

	    if (u.uhunger <= 100 || ACURR(A_STR) < 6) {
#ifdef WIZARD
		if (!wizard) {
#endif
/*JP			You("lack the strength %s.",
			    castit ? "for a teleport spell" : "to teleport");*/
			You("%sだけの力がない．",
			    castit ? "瞬間移動の魔法を唱える" : "瞬間移動する");

			return 1;
#ifdef WIZARD
		}
#endif
	    }

	    energy = objects[SPE_TELEPORT_AWAY].oc_level * 7 / 2 - 2;
	    if (u.uen <= energy) {
#ifdef WIZARD
		if (wizard)
			energy = u.uen;
		else
#endif
		{
/*JP			You("lack the energy %s.",
			    castit ? "for a teleport spell" : "to teleport");*/
			You("%sだけのエネルギーがない．",
			    castit ? "瞬間移動の魔法を唱える" : "瞬間移動する");
			return 1;
		}
	    }

	    if (check_capacity(
/*JP			"Your concentration falters from carrying so much."))*/
			"沢山ものを持ちすぎて，あなたは集中できない．"))
		return 1;

	    if (castit) {
		exercise(A_WIS, TRUE);
		if (spelleffects(sp_no, TRUE))
			return(1);
		else
#ifdef WIZARD
		    if (!wizard)
#endif
			return(0);
	    } else {
		u.uen -= energy;
		flags.botl = 1;
	    }
	}

	if (next_to_u()) {
		if (trap && trap->once) vault_tele();
		else tele();
		(void) next_to_u();
	} else {
		You(shudder_for_moment);
		return(0);
	}
	if (!trap) morehungry(100);
	return(1);
}


void
level_tele()
{
	register int newlev;
	d_level newlevel;

	if ((u.uhave.amulet || In_endgame(&u.uz))
#ifdef WIZARD
						&& !wizard
#endif
							) {
/*JP	    You_feel("very disoriented for a moment.");*/
	    You("一瞬方向感覚を大きく失ったような気がした．");
	    return;
	}
	if (Teleport_control
#ifdef WIZARD
	   || wizard
#endif
		) {
	    char buf[BUFSZ], qbuf[BUFSZ];
	    int trycnt = 0;

	    Strcpy(qbuf, "To what level do you want to teleport?");
	    Strcpy(qbuf, "何階に移動しますか？");
	    do {
/*JP		if (++trycnt == 2) Strcat(qbuf, " [type a number]");*/
		if (++trycnt == 2) Strcat(qbuf, "[数字をいれてね]");
		getlin(qbuf, buf);
		if (!strcmp(buf,"\033"))	/* cancelled */
		    return;
		else if (!strcmp(buf,"*"))
		    goto random_levtport;
		if ((newlev = lev_by_name(buf)) == 0) newlev = atoi(buf);
	    } while (!newlev && !digit(buf[0]) &&
		     (buf[0] != '-' || !digit(buf[1])) &&
		     trycnt < 10);

	    /* no dungeon escape via this route */
	    if (newlev == 0) {
		if (trycnt >= 10)
		    goto random_levtport;
/*JP		if (ynq("Go to Nowhere.  Are you sure?") != 'y') return;*/
	        if(ynq("どことも知れぬ場所に行きます？よろしいですか？") != 'y') return;
/*JP		You("scream in agony as your body begins to warp...");*/
	        You("体が移動しはじめると，苦しみのあえぎ声を出した．．．");
		display_nhwindow(WIN_MESSAGE, FALSE);
/*JP	        You("cease to exist.");*/
	        You("抜けれなかった．");
/*JP		killer_format = NO_KILLER_PREFIX;*/
		killer_format = KILLED_BY;
/*JP		killer = "committed suicide";*/
		killer = "献身的自殺行為で";
		done(DIED);
		return;
	    }

	    /* if in Knox and the requested level > 0, stay put.
	     * we let negative values requests fall into the "heaven" loop.
	     */
	    if (Is_knox(&u.uz) && newlev > 0) {
		You(shudder_for_moment);
		return;
	    }
	    /* if in Quest, the player sees "Home 1", etc., on the status
	     * line, instead of the logical depth of the level.  controlled
	     * level teleport request is likely to be relativized to the
	     * status line, and consequently it should be incremented to
	     * the value of the logical depth of the target level.
	     *
	     * we let negative values requests fall into the "heaven" loop.
	     */
	    if (In_quest(&u.uz) && newlev > 0)
		newlev = newlev + dungeons[u.uz.dnum].depth_start - 1;
	} else { /* involuntary level tele */
 random_levtport:
	    newlev = random_teleport_level();
	    if (newlev == depth(&u.uz)) {
		You(shudder_for_moment);
		return;
	    }
	}

	if (!next_to_u()) {
		You(shudder_for_moment);
		return;
	}
#ifdef WIZARD
	if (In_endgame(&u.uz)) {	/* must already be wizard */
	    int llimit = dunlevs_in_dungeon(&u.uz);

	    if (newlev >= 0 || newlev <= -llimit) {
/*JP		You("can't get there from here.");*/
		You("ここからは行けない．");
		return;
	    }
	    newlevel.dnum = u.uz.dnum;
	    newlevel.dlevel = llimit + newlev;
	    schedule_goto(&newlevel, FALSE, FALSE, 0, (char *)0, (char *)0);
	    return;
	}
#endif

	if (newlev < 0) {
		if (newlev <= -10) {
/*JP			You("arrive in heaven.");*/
			You("天国に辿りついた．");
/*JP			verbalize("Thou art early, but we'll admit thee.");*/
			verbalize("汝，死ぬには早すぎるがそれもよかろう．");
/*JP			killer_format = NO_KILLER_PREFIX;*/
			killer_format = KILLED_BY;
/*JP			killer = "went to heaven prematurely";*/
			killer = "若くして天国に行き";
			done(DIED);
			return;
		} else if (newlev == -9) {
/*JP			You_feel("deliriously happy. ");*/
			You("狂ったような幸せを感じた．");
/*JP			pline("(In fact, you're on Cloud 9!) ");*/
			pline("(本当に苦を乗り越えた場所にいる！) ");
			display_nhwindow(WIN_MESSAGE, FALSE);
		} else
/*JP			You("are now high above the clouds...");*/
			You("雲の遥か上にいる．．．");

		if (Levitation || is_floater(uasmon)) {
/*JP		    You("float gently down to earth.");*/
		    You("ゆっくり地面に降りた．");
		    u.uz.dnum = 0; /* he might have been in another dgn */
		    newlev = 1;
		} else if (is_flyer(uasmon)) {
/*JP		    You("fly down to earth.");*/
		    You("地面に降りた．");
		    u.uz.dnum = 0; /* he might have been in another dgn */
		    newlev = 1;
		} else {
		    d_level save_dlevel;

		    assign_level(&save_dlevel, &u.uz);
/*JP		    pline("Unfortunately, you don't know how to fly.");
		    You("plummet a few thousand feet to your death.");*/
		    pline("不幸にも，あなたは飛びかたを知らない．");
		    pline("数千フィートの死のダイビングだ！");
		    u.uz.dnum = 0;
		    u.uz.dlevel = 0;
/*JP		    killer_format = NO_KILLER_PREFIX;*/
		    killer_format = KILLED_BY;
		    killer =
/*JP    self_pronoun("teleported out of the dungeon and fell to %s death","his");*/
		    "迷宮を飛びだし死のダイビングで";
		    done(DIED);
		    assign_level(&u.uz, &save_dlevel);
		    flags.botl = 1;
		    return;
		}
	}

	/* calls done(ESCAPED) if newlevel==0 */
	if (u.uz.dnum == medusa_level.dnum &&
	    newlev >= dungeons[u.uz.dnum].depth_start +
						dunlevs_in_dungeon(&u.uz)) {
	    find_hell(&newlevel);
	} else {
	    /* if invocation did not yet occur, teleporting into
	     * the last level of Gehennom is forbidden.
	     */
	    if (Inhell && !u.uevent.invoked &&
			newlev >= (dungeons[u.uz.dnum].depth_start +
					dunlevs_in_dungeon(&u.uz) - 1)) {
		newlev = dungeons[u.uz.dnum].depth_start +
					dunlevs_in_dungeon(&u.uz) - 2;
/*JP		pline("Sorry...");*/
		pline("おっと．．．");
	    }
	    /* no teleporting out of quest dungeon */
	    if (In_quest(&u.uz) && newlev < depth(&qstart_level))
		newlev = depth(&qstart_level);
	    /* the player thinks of levels purely in logical terms, so
	     * we must translate newlev to a number relative to the
	     * current dungeon.
	     */
	    get_level(&newlevel, newlev);
	}
	schedule_goto(&newlevel, FALSE, FALSE, 0, (char *)0, (char *)0);
}

void
domagicportal(ttmp)
register struct trap *ttmp;
{
	struct d_level target_level;

	if (!next_to_u()) {
		You(shudder_for_moment);
		return;
	}

	/* if landed from another portal, do nothing */
	/* problem: level teleport landing escapes the check */
	if (!on_level(&u.uz, &u.uz0)) return;

/*JP	You("activated a magic portal!");*/
	pline("魔法の入口が動きだした！");

	/* prevent the poor shnook, whose amulet was stolen while in
	 * the endgame, from accidently triggering the portal to the
	 * next level, and thus losing the game
	 */
	if (In_endgame(&u.uz) && !u.uhave.amulet) {
/*JP	    You_feel("dizzy for a moment, but nothing happens...");*/
	    You("一瞬めまいを感じた，しかし何も起きなかった．．．");
	    return;
	}

	target_level = ttmp->dst;
	schedule_goto(&target_level, FALSE, FALSE, 1,
/*JP		      "You feel dizzy for a moment, but the sensation passes.",*/
		      "一瞬めまいを感じた，しかしその感覚は消えた．",
		      (char *)0);
}

void
tele_trap(trap)
struct trap *trap;
{
	if (In_endgame(&u.uz) || Antimagic) {
		if (Antimagic)
			shieldeff(u.ux, u.uy);
/*JP		You_feel("a wrenching sensation.");*/
		You("ねじられたような感覚を感じた．");
	} else if (!next_to_u()) {
		You(shudder_for_moment);
	} else if (trap->once) {
		deltrap(trap);
		newsym(u.ux,u.uy);	/* get rid of trap symbol */
		vault_tele();
	} else
		tele();
}

void
level_tele_trap(trap)
struct trap *trap;
{
/*JP	You("%s onto a level teleport trap!",
		      Levitation ? (const char *)"float" :
				  locomotion(uasmon, "step"));*/
	You("別の階への瞬間移動の罠を%s！",
		      Levitation ? (const char *)"見下ろした" :
				  jconj(locomotion(uasmon, "踏む"), "た"));
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	}
	if (Antimagic || In_endgame(&u.uz)) {
/*JP	    You_feel("a wrenching sensation.");*/
	    You("ねじられたような感覚を感じた．");
	    return;
	}
	if (!Blind)
/*JP	    You("are momentarily blinded by a flash of light.");*/
	    You("まばゆい光で一瞬目がくらんだ．");
	else
/*JP	    You("are momentarily disoriented.");*/
	    You("一瞬方向感覚を失った．");
	deltrap(trap);
	newsym(u.ux,u.uy);	/* get rid of trap symbol */
	level_tele();
}

/* check whether monster can arrive at location <x,y> via Tport (or fall) */
static boolean
rloc_pos_ok(x, y, mtmp)
register int x, y;		/* coordinates of candidate location */
struct monst *mtmp;
{
	register int xx, yy;

	if (!goodpos(x, y, mtmp, mtmp->data)) return FALSE;
	/*
	 * Check for restricted areas present in some special levels.
	 *
	 * `xx' is current column; if 0, then `yy' will contain flag bits
	 * rather than row:  bit #0 set => moving upwards; bit #1 set =>
	 * inside the Wizard's tower.
	 */
	xx = mtmp->mx;
	yy = mtmp->my;
	if (!xx) {
	    /* no current location (migrating monster arrival) */
	    if (dndest.nlx && On_W_tower_level(&u.uz))
		return ((yy & 2) != 0) ^	/* inside xor not within */
		       !within_bounded_area(x, y, dndest.nlx, dndest.nly,
						  dndest.nhx, dndest.nhy);
	    if (updest.lx && (yy & 1) != 0)	/* moving up */
		return (within_bounded_area(x, y, updest.lx, updest.ly,
						  updest.hx, updest.hy) &&
		       (!updest.nlx ||
			!within_bounded_area(x, y, updest.nlx, updest.nly,
						   updest.nhx, updest.nhy)));
	    if (dndest.lx && (yy & 1) == 0)	/* moving down */
		return (within_bounded_area(x, y, dndest.lx, dndest.ly,
						  dndest.hx, dndest.hy) &&
		       (!dndest.nlx ||
			!within_bounded_area(x, y, dndest.nlx, dndest.nly,
						   dndest.nhx, dndest.nhy)));
	} else {
	    /* current location is <xx,yy> */
	    if (!tele_jump_ok(xx, yy, x, y)) return FALSE;
	}
	/* <x,y> is ok */
	return TRUE;
}

/*
 * rloc_to()
 *
 * Pulls a monster from its current position and places a monster at
 * a new x and y.  If oldx is 0, then the monster was not in the levels.monsters
 * array.  However, if oldx is 0, oldy may still have a value because mtmp is a
 * migrating_mon.  Worm tails are always placed randomly around the head of
 * the worm.
 */
void
rloc_to(mtmp, x, y)
struct monst *mtmp;
register int x, y;
{
	register int oldx = mtmp->mx, oldy = mtmp->my;

	if (x == mtmp->mx && y == mtmp->my)	/* that was easy */
		return;

	if (oldx) {				/* "pick up" monster */
	    if (mtmp->wormno)
		remove_worm(mtmp);
	    else {
		remove_monster(oldx, oldy);
		newsym(oldx, oldy);		/* update old location */
	    }
	}

	place_monster(mtmp, x, y);		/* put monster down */

	if (mtmp->wormno)			/* now put down tail */
		place_worm_tail_randomly(mtmp, x, y);

	if (u.ustuck == mtmp) {
		if (u.uswallow) {
			u.ux = x;
			u.uy = y;
			docrt();
		} else	u.ustuck = 0;
	}

	newsym(x, y);				/* update new location */
	set_apparxy(mtmp);			/* orient monster */
}

/* place a monster at a random location, typically due to teleport */
void
rloc(mtmp)
struct monst *mtmp;	/* mx==0 implies migrating monster arrival */
{
	register int x, y, trycount;
	xchar omx = mtmp->mx, omy = mtmp->my;

	if (mtmp->iswiz && omx) {	/* Wizard, not just arriving */
	    if (!In_W_tower(u.ux, u.uy, &u.uz))
		x = xupstair,  y = yupstair;
	    else if (!xdnladder)	/* bottom level of tower */
		x = xupladder,  y = yupladder;
	    else
		x = xdnladder,  y = ydnladder;
	    /* if the wiz teleports away to heal, try the up staircase,
	       to block the player's escaping before he's healed
	       (deliberately use `goodpos' rather than `rloc_pos_ok' here) */
	    if (goodpos(x, y, mtmp, mtmp->data))
		goto found_xy;
	}

	trycount = 0;
	do {
	    x = rn1(COLNO-3,2);
	    y = rn2(ROWNO);
	    if ((trycount < 500) ? rloc_pos_ok(x, y, mtmp)
				 : goodpos(x, y, mtmp, mtmp->data))
		goto found_xy;
	} while (++trycount < 1000);

	/* last ditch attempt to find a good place */
	for (x = 2; x < COLNO - 1; x++)
	    for (y = 0; y < ROWNO; y++)
		if (goodpos(x, y, mtmp, mtmp->data))
		    goto found_xy;

	/* level either full of monsters or somehow faulty */
	impossible("rloc(): couldn't relocate monster");
	return;

 found_xy:
	rloc_to(mtmp, x, y);
	/* shopkeepers will only teleport if you zap them with a wand of
	   teleportation or if they've been transformed into a jumpy monster;
	   the latter only happens if you've attacked them with polymorph */
	if (mtmp->isshk && !inhishop(mtmp)) make_angry_shk(mtmp, omx, omy);
}

static void
mvault_tele(mtmp)
struct monst *mtmp;
{
	register struct mkroom *croom = search_special(VAULT);
	coord c;

	if (croom && somexy(croom, &c) &&
				goodpos(c.x, c.y, mtmp, mtmp->data)) {
		rloc_to(mtmp, c.x, c.y);
		return;
	}
	rloc(mtmp);
}

boolean
tele_restrict(mon)
struct monst *mon;
{
	if (level.flags.noteleport) {
		if (canseemon(mon))
/*JP		    pline("A mysterious force prevents %s from teleporting!",
			mon_nam(mon));*/
		    pline("奇妙な力が%sの瞬間移動を防いだ！",
			mon_nam(mon));

		return TRUE;
	}
	return FALSE;
}

void
mtele_trap(mtmp, trap, in_sight)
struct monst *mtmp;
struct trap *trap;
int in_sight;
{
	char *monname;

	if (tele_restrict(mtmp)) return;
	if (teleport_pet(mtmp)) {
	    /* save name with pre-movement visibility */
	    monname = Monnam(mtmp);

	    /* Note: don't remove the trap if a vault.  Other-
	     * wise the monster will be stuck there, since
	     * the guard isn't going to come for it...
	     */
	    if (trap->once) mvault_tele(mtmp);
	    else rloc(mtmp);

	    if (in_sight) {
		if (canseemon(mtmp))
/*JP		    pline("%s seems disoriented.", monname);*/
		    pline("%sは一瞬方向間隔を失ったようだ．", monname);
		else
/*JP		    pline("%s suddenly disappears!", monname);*/
		    pline("%sは突然消えた！", monname);
		seetrap(trap);
	    }
	}
}

/* return 0 if still on level, 3 if not */
int
mlevel_tele_trap(mtmp, trap, in_sight)
struct monst *mtmp;
struct trap *trap;
int in_sight;
{
	int tt = trap->ttyp;
	struct permonst *mptr = mtmp->data;

	if (mtmp == u.ustuck)	/* probably a vortex */
	    return 0;		/* temporary? kludge */
	if (teleport_pet(mtmp)) {
	    d_level tolevel;
	    int migrate_typ = MIGR_RANDOM;

	    if ((tt == HOLE || tt == TRAPDOOR)) {
		if (Is_stronghold(&u.uz)) {
		    assign_level(&tolevel, &valley_level);
		} else if (Is_botlevel(&u.uz)) {
		    if (in_sight && trap->tseen)
/*JP			pline("%s avoids the %s.", Monnam(mtmp),
			(tt == HOLE) ? "hole" : "trap");*/
			pline("%sは%sを回避した．", Monnam(mtmp),
			(tt == HOLE) ? "穴" : "罠");
		    return 0;
		} else {
		    get_level(&tolevel, depth(&u.uz) + 1);
		}
	    } else if (tt == MAGIC_PORTAL) {
		if (In_endgame(&u.uz) &&
		    (mon_has_amulet(mtmp) || is_home_elemental(mptr))) {
		    if (in_sight && mptr->mlet != S_ELEMENTAL) {
/*JP			pline("%s seems to shimmer for a moment.",*/
		        pline("%sが一瞬輝いたように見えた",
							Monnam(mtmp));
			seetrap(trap);
		    }
		    return 0;
		} else {
		    assign_level(&tolevel, &trap->dst);
		    migrate_typ = MIGR_PORTAL;
		}
	    } else { /* (tt == LEVEL_TELEP) */
		int nlev;

		if (mon_has_amulet(mtmp) || In_endgame(&u.uz)) {
		    if (in_sight)
/*JP			pline("%s seems very disoriented for a moment.",*/
			pline("%sは一瞬方向感覚を大きく失ったようだ．",
				Monnam(mtmp));
		    return 0;
		}
		nlev = random_teleport_level();
		if (nlev == depth(&u.uz)) {
		    if (in_sight)
/*JP			pline("%s shudders for a moment.", Monnam(mtmp));*/
			pline("%sは一瞬震えた．", Monnam(mtmp));
		    return 0;
		}
		get_level(&tolevel, nlev);
	    }

	    if (in_sight) {
/*JP		pline("Suddenly, %s disappears out of sight.", mon_nam(mtmp));*/
		pline("突然%sが視界から消えた．", mon_nam(mtmp));
		seetrap(trap);
	    }
	    migrate_to_level(mtmp, ledger_no(&tolevel),
			     migrate_typ, (coord *)0);
	    return 3;	/* no longer on this level */
	}
	return 0;
}


void
rloco(obj)
register struct obj *obj;
{
	register xchar tx, ty, otx, oty;
	boolean restricted_fall;
	int try_limit = 4000;

	obj_extract_self(obj);
	otx = obj->ox;
	oty = obj->oy;
	restricted_fall = (otx == 0 && dndest.lx);
	do {
	    tx = rn1(COLNO-3,2);
	    ty = rn2(ROWNO);
	    if (!--try_limit) break;
	} while (!goodpos(tx, ty, (struct monst *)0, (struct permonst *)0) ||
		/* bug: this lacks provision for handling the Wizard's tower */
		 (restricted_fall &&
		  (!within_bounded_area(tx, ty, dndest.lx, dndest.ly,
						dndest.hx, dndest.hy) ||
		   (dndest.nlx &&
		    within_bounded_area(tx, ty, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy)))));

/*JP	if (flooreffects(obj, tx, ty, "fall")) {*/
	if (flooreffects(obj, tx, ty, "落ちる")) {
	    return;
	} else if (otx == 0 && oty == 0) {
	    ;	/* fell through a trapdoor; no update of old loc needed */
	} else {
	    if (costly_spot(otx, oty)
	      && (!costly_spot(tx, ty) ||
		  !index(in_rooms(tx, ty, 0), *in_rooms(otx, oty, 0)))) {
		if (costly_spot(u.ux, u.uy) &&
			    index(u.urooms, *in_rooms(otx, oty, 0)))
		    addtobill(obj, FALSE, FALSE, FALSE);
		else (void)stolen_value(obj, otx, oty, FALSE, FALSE);
	    }
	    newsym(otx, oty);	/* update old location */
	}
	place_object(obj, tx, ty);
	newsym(tx, ty);
}

/* Returns an absolute depth */
int
random_teleport_level()
{
	int nlev, max_depth, min_depth;

	if (!rn2(5) || Is_knox(&u.uz))
		return (int)depth(&u.uz);

	/* Get a random value relative to the current dungeon */
	/* Range is 1 to current+3, current not counting */
	nlev = rnd((int)depth(&u.uz) + 2);
	if (nlev >= (int)depth(&u.uz)) nlev++;

	/* What I really want to do is as follows:
	 * -- If in a dungeon that goes down, the new level is to be restricted
	 *    to [top of parent, bottom of current dungeon]
	 * -- If in a dungeon that goes up, the new level is to be restricted
	 *    to [top of current dungeon, bottom of parent]
	 * -- If in a quest dungeon or similar dungeon entered by portals,
	 *    the new level is to be restricted to [top of current dungeon,
	 *    bottom of current dungeon]
	 * I don't know how to do this, and we need to get the release out, so
	 * I left in the old behavior (the one for players; there were two
	 * different versions, one for players and one for monsters, which is
	 * why it had to be changed to begin with).  With this behavior, we
	 * must explicitly check for special dungeons individually above.
	 * Currently we check for Knox; endgame is handled in the caller due
	 * to its different message ("disoriented").
	 * --KAA
	 */
	min_depth = 1;
	max_depth = dunlevs_in_dungeon(&u.uz) +
			(dungeons[u.uz.dnum].depth_start - 1);

	if (nlev > max_depth) {
	    nlev = max_depth;
	    /* teleport up if already on bottom */
	    if (Is_botlevel(&u.uz)) nlev -= rnd(3);
	}
	if (nlev < min_depth) {
	    nlev = min_depth;
	    if ((int)depth(&u.uz) == min_depth) {
		nlev += rnd(3);
		if (nlev > max_depth)
		    nlev = max_depth;
	    }
	}
	return nlev;
}
/*teleport.c*/
