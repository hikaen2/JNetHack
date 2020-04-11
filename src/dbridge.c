/*	SCCS Id: @(#)dbridge.c	3.1	93/05/15	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet		  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the drawbridge manipulation (create, open, close,
 * destroy).
 *
 * Added comprehensive monster-handling, and the "entity" structure to 
 * deal with players as well. - 11/89
 */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

#ifdef OVLB
static void FDECL(get_wall_for_db, (int *, int *));
static struct entity *FDECL(e_at, (int, int));
static void FDECL(m_to_e, (struct monst *, XCHAR_P, XCHAR_P, struct entity *));
static void FDECL(u_to_e, (struct entity *));
static void FDECL(set_entity, (int, int, struct entity *));
static const char *FDECL(e_nam, (struct entity *));
#ifdef D_DEBUG
static const char *FDECL(Enam, (struct entity *)); /* unused */
#endif
static const char *FDECL(E_phrase, (struct entity *, const char *));
static boolean FDECL(e_survives_at, (struct entity *, int, int));
static void FDECL(e_died, (struct entity *, int, int));
static boolean FDECL(automiss, (struct entity *));
static boolean FDECL(e_missed, (struct entity *, BOOLEAN_P));
static boolean FDECL(e_jumps, (struct entity *));
static void FDECL(do_entity, (struct entity *));
#endif /* OVLB */

#ifdef OVL0

boolean
is_pool(x,y)
int x,y;
{
       register schar ltyp = levl[x][y].typ;
       if(ltyp == POOL || ltyp == MOAT || ltyp == WATER) return TRUE;
       if(ltyp == DRAWBRIDGE_UP &&
               (levl[x][y].drawbridgemask & DB_UNDER) == DB_MOAT) return TRUE;
       return FALSE;
}

boolean
is_lava(x,y)
int x,y;
{
       register schar ltyp = levl[x][y].typ;
       if(ltyp == LAVAPOOL ||
	  (ltyp == DRAWBRIDGE_UP &&
	   (levl[x][y].drawbridgemask & DB_UNDER) == DB_LAVA)) return TRUE;
       return FALSE;
}

boolean
is_ice(x,y)
int x,y;
{
	register schar ltyp = levl[x][y].typ;
	if (ltyp == ICE ||
	    (ltyp == DRAWBRIDGE_UP &&
		(levl[x][y].drawbridgemask & DB_UNDER) == DB_ICE)) return TRUE;
	return FALSE;
}

#endif /* OVL0 */

#ifdef OVL1

/* 
 * We want to know whether a wall (or a door) is the portcullis (passageway)
 * of an eventual drawbridge.
 *
 * Return value:  the direction of the drawbridge.
 */

int
is_drawbridge_wall(x,y)
int x,y;
{
	struct rm *lev;

	lev = &levl[x][y];
	if (lev->typ != DOOR && lev->typ != DBWALL)
		return (-1);

	if (IS_DRAWBRIDGE(levl[x+1][y].typ) &&
	    (levl[x+1][y].drawbridgemask & DB_DIR) == DB_WEST)
		return (DB_WEST);
	if (IS_DRAWBRIDGE(levl[x-1][y].typ) && 
	    (levl[x-1][y].drawbridgemask & DB_DIR) == DB_EAST)
		return (DB_EAST);
	if (IS_DRAWBRIDGE(levl[x][y-1].typ) && 
	    (levl[x][y-1].drawbridgemask & DB_DIR) == DB_SOUTH)
		return (DB_SOUTH);
	if (IS_DRAWBRIDGE(levl[x][y+1].typ) && 
	    (levl[x][y+1].drawbridgemask & DB_DIR) == DB_NORTH)
		return (DB_NORTH);

	return (-1);
}

/*
 * Use is_db_wall where you want to verify that a
 * drawbridge "wall" is UP in the location x, y
 * (instead of UP or DOWN, as with is_drawbridge_wall). 
 */ 
boolean
is_db_wall(x,y)
int x,y;
{
	return((boolean)( levl[x][y].typ == DBWALL ));
}


/*
 * Return true with x,y pointing to the drawbridge if x,y initially indicate
 * a drawbridge or drawbridge wall.
 */
boolean
find_drawbridge(x,y)
int *x,*y;
{
	int dir;

	if (IS_DRAWBRIDGE(levl[*x][*y].typ))
		return TRUE;
	dir = is_drawbridge_wall(*x,*y);
	if (dir >= 0) {
		switch(dir) {
			case DB_NORTH: (*y)++; break;
			case DB_SOUTH: (*y)--; break;
			case DB_EAST:  (*x)--; break;
			case DB_WEST:  (*x)++; break;
		}
		return TRUE;
	}
	return FALSE;
}

#endif /* OVL1 */
#ifdef OVLB

/* 
 * Find the drawbridge wall associated with a drawbridge.
 */
static void
get_wall_for_db(x,y)
int *x,*y;
{
	switch (levl[*x][*y].drawbridgemask & DB_DIR) {
		case DB_NORTH: (*y)--; break;
		case DB_SOUTH: (*y)++; break;
		case DB_EAST:  (*x)++; break;
		case DB_WEST:  (*x)--; break;
	}
}

/*
 * Creation of a drawbridge at pos x,y.
 *     dir is the direction.
 *     flag must be put to TRUE if we want the drawbridge to be opened.
 */

boolean
create_drawbridge(x,y,dir,flag)
int x,y,dir;
boolean flag;
{
	int x2,y2;
	boolean horiz;
	boolean lava = levl[x][y].typ == LAVAPOOL; /* assume initialized map */

	x2 = x; y2 = y;
	switch(dir) {
		case DB_NORTH:
			horiz = TRUE;
			y2--;
			break;
		case DB_SOUTH:
			horiz = TRUE;
			y2++;
			break;
		case DB_EAST:
			horiz = FALSE;
			x2++;
			break;
		default:
			impossible("bad direction in create_drawbridge");
			/* fall through */
		case DB_WEST:
			horiz = FALSE;
			x2--;
			break;
	}
	if (!IS_WALL(levl[x2][y2].typ))
		return(FALSE);
	if (flag) {             /* We want the bridge open */
		levl[x][y].typ = DRAWBRIDGE_DOWN;
		levl[x2][y2].typ = DOOR;
		levl[x2][y2].doormask = D_NODOOR;
	} else {
		levl[x][y].typ = DRAWBRIDGE_UP;
		levl[x2][y2].typ = DBWALL;
		/* Drawbridges are non-diggable. */
		levl[x2][y2].diggable = W_NONDIGGABLE;
	}
	levl[x][y].horizontal = !horiz;
	levl[x2][y2].horizontal = horiz;
	levl[x][y].drawbridgemask = dir;
	if(lava) levl[x][y].drawbridgemask |= DB_LAVA;
	return(TRUE);           
}

struct entity {
	struct monst *emon;	  /* youmonst for the player */
	struct permonst *edata;   /* must be non-zero for record to be valid */
	int ex, ey;
};

#define ENTITIES 2

static NEARDATA struct entity occupants[ENTITIES];

static
struct entity *
e_at(x, y)
int x, y;
{
	int entitycnt;
	
	for (entitycnt = 0; entitycnt < ENTITIES; entitycnt++)
		if ((occupants[entitycnt].edata) && 
		    (occupants[entitycnt].ex == x) &&
		    (occupants[entitycnt].ey == y))
			break;
#ifdef D_DEBUG
	pline("entitycnt = %d", entitycnt);
	wait_synch();
#endif
	return((entitycnt == ENTITIES)? 
	       (struct entity *)0 : &(occupants[entitycnt]));
}

static void
m_to_e(mtmp, x, y, etmp)
struct monst *mtmp;
xchar x, y;
struct entity *etmp;
{
	etmp->emon = mtmp;
	if (mtmp) {
		etmp->ex = x;
		etmp->ey = y;
		if (mtmp->wormno && (x != mtmp->mx || y != mtmp->my))
			etmp->edata = &mons[PM_LONG_WORM_TAIL];
		else
			etmp->edata = mtmp->data;
	} else
		etmp->edata = (struct permonst *)0;
}

static void
u_to_e(etmp)
struct entity *etmp;
{
	etmp->emon = &youmonst;
	etmp->ex = u.ux;
	etmp->ey = u.uy;
	etmp->edata = uasmon;
}

static void
set_entity(x, y, etmp)
int x, y;
struct entity *etmp;
{
	if ((x == u.ux) && (y == u.uy))
		u_to_e(etmp);
	else
		if (MON_AT(x, y))
			m_to_e(m_at(x, y), x, y, etmp);
		else
			etmp->edata = (struct permonst *)0;
}

#define is_u(etmp) (etmp->emon == &youmonst)
#define e_canseemon(etmp) (is_u(etmp) ? (boolean)TRUE : canseemon(etmp->emon)) 

/*
 * e_strg is a utility routine which is not actually in use anywhere, since 
 * the specialized routines below suffice for all current purposes. 
 */

/* #define e_strg(etmp, func) (is_u(etmp)? (char *)0 : func(etmp->emon)) */

static const char *
e_nam(etmp)
struct entity *etmp;
{
/*JP	return(is_u(etmp)? "you" : mon_nam(etmp->emon));*/
	return(is_u(etmp)? "あなた" : mon_nam(etmp->emon));
}

#ifdef D_DEBUG
/*
 * Enam is another unused utility routine:  E_phrase is preferable.
 */

static const char *
Enam(etmp)
struct entity *etmp;
{
/*JP	return(is_u(etmp)? "You" : Monnam(etmp->emon));*/
	return(is_u(etmp)? "あなた" : Monnam(etmp->emon));
}
#endif /* D_DEBUG */

/*
 * Generates capitalized entity name, makes 2nd -> 3rd person conversion on 
 * verb, where necessary.
 */

static const char *
E_phrase(etmp, verb)
struct entity *etmp;
const char *verb;
{
	static char wholebuf[80];
	char verbbuf[30];

	if (is_u(etmp)) 
/*JP		Strcpy(wholebuf, "You");*/
		Strcpy(wholebuf, "あなた");
	else
		Strcpy(wholebuf, Monnam(etmp->emon));
	if (!*verb)
		return(wholebuf);
/*JP	Strcat(wholebuf, " ");*/
	Strcat(wholebuf, "は");
	verbbuf[0] = '\0';
#if 0
	if (is_u(etmp)) 
		Strcpy(verbbuf, verb);
	else {
		if (!strcmp(verb, "are"))
			Strcpy(verbbuf, "is");
		if (!strcmp(verb, "have"))
			Strcpy(verbbuf, "has");
		if (!verbbuf[0]) {
			Strcpy(verbbuf, verb);
			switch (verbbuf[strlen(verbbuf) - 1]) {
				case 'y':
					verbbuf[strlen(verbbuf) - 1] = '\0';
					Strcat(verbbuf, "ies");
					break;
				case 'h':
				case 'o':
				case 's':
					Strcat(verbbuf, "es");
					break;
				default:
					Strcat(verbbuf, "s");
					break;
			}
		}
	}
#endif
	Strcat(wholebuf, verbbuf);
	return(wholebuf);
}

/*
 * Simple-minded "can it be here?" routine
 */

static boolean
e_survives_at(etmp, x, y)
struct entity *etmp;
int x, y;
{
	if (noncorporeal(etmp->edata))
		return(TRUE);
	if (is_pool(x, y))
		return((boolean)((is_u(etmp) && (Wwalking || Amphibious || Levitation)) ||
		       is_swimmer(etmp->edata) || is_flyer(etmp->edata) ||
		       is_floater(etmp->edata)));
	/* must force call to lava_effects in e_died if is_u */
	if (is_lava(x, y))
		return((boolean)(is_u(etmp) ? !!Levitation : resists_fire(etmp->edata)));
	if (is_db_wall(x, y))
		return((boolean)(passes_walls(etmp->edata)));
	return(TRUE);
}

static void
e_died(etmp, dest, how)
struct entity *etmp;
int dest, how;
{
	if (is_u(etmp)) {
		if (how == DROWNING)
			(void) drown();
		if (how == BURNING)
			(void) lava_effects();
		else {
			coord xy;

			killer_format = KILLED_BY_AN;
/*JP			killer = "falling drawbridge";*/
			killer = "降りてきた跳ね橋で";
			done(how);
			/* So, you didn't die */
			if (!e_survives_at(etmp, etmp->ex, etmp->ey)) {
			    if (enexto(&xy, etmp->ex, etmp->ey,
							    etmp->edata)) {
/*JP				pline("A %s force teleports you away...",
				      Hallucination ? "normal" : "strange");*/
				pline("%s力があなたを遠くに運んだ．．．",
				      Hallucination ? "普通の" : "奇妙な");
				teleds(xy.x, xy.y);
			    }
			    /* otherwise on top of the drawbridge is the
			     * only viable spot in the dungeon, so stay there
			     */
			}
		}
	} else {
		xkilled(etmp->emon, dest);
		etmp->edata = (struct permonst *)0;	
	}
}


/*
 * These are never directly affected by a bridge or portcullis.
 */

static boolean
automiss(etmp)
struct entity *etmp;
{
	return((boolean)(passes_walls(etmp->edata) || noncorporeal(etmp->edata)));
}

/*
 * Does falling drawbridge or portcullis miss etmp?
 */

static boolean
e_missed(etmp, chunks)
struct entity *etmp;
boolean chunks;
{
	int misses;

#ifdef D_DEBUG
	if (chunks)
		pline("Do chunks miss?");
#endif
	if (automiss(etmp))
		return(TRUE);	

	if (is_flyer(etmp->edata) && 
	    (is_u(etmp)? !Sleeping : 
	     (etmp->emon->mcanmove && !etmp->emon->msleep)))
						 /* flying requires mobility */
		misses = 5;	/* out of 8 */	
	else
		if (is_floater(etmp->edata) ||
		    (is_u(etmp) && Levitation))	 /* doesn't require mobility */
			misses = 3;
		else
			if (chunks && is_pool(etmp->ex, etmp->ey))
				misses = 2; 		    /* sitting ducks */
			else
				misses = 0;	  

	if (is_db_wall(etmp->ex, etmp->ey))
		misses -= 3;				    /* less airspace */

#ifdef D_DEBUG
	pline("Miss chance = %d (out of 8)", misses);
#endif

	return((boolean)((misses >= rnd(8))? TRUE : FALSE));
}

/*
 * Can etmp jump from death?
 */ 

static boolean
e_jumps(etmp)
struct entity *etmp;
{
	int tmp = 4; 		/* out of 10 */

	if (is_u(etmp)? (Sleeping || Fumbling) : 
		        (!etmp->emon->mcanmove || etmp->emon->msleep || 
			 !etmp->edata->mmove   || etmp->emon->wormno))
		return(FALSE);

	if (is_u(etmp)? Confusion : etmp->emon->mconf)
		tmp -= 2;

	if (is_u(etmp)? Stunned : etmp->emon->mstun)
		tmp -= 3;

	if (is_db_wall(etmp->ex, etmp->ey))
		tmp -= 2;			    /* less room to maneuver */
	
#ifdef D_DEBUG
	pline("%s to jump (%d chances in 10)", E_phrase(etmp, "try"), tmp);
#endif
	return((boolean)((tmp >= rnd(10))? TRUE : FALSE));
}

static void
do_entity(etmp)
struct entity *etmp;
{
	int newx, newy, at_portcullis, oldx, oldy;
	boolean must_jump = FALSE, relocates = FALSE, e_inview;
	struct rm *crm;

	if (!etmp->edata)
		return;

	e_inview = e_canseemon(etmp);

	oldx = etmp->ex;
	oldy = etmp->ey;

	at_portcullis = is_db_wall(oldx, oldy);

	crm = &levl[oldx][oldy];

	if (automiss(etmp) && e_survives_at(etmp, oldx, oldy)) {
		char edifice[20];

		if (e_inview) {
			*edifice = '\0';
			if ((crm->typ == DRAWBRIDGE_DOWN) ||
		    	    (crm->typ == DRAWBRIDGE_UP))
/*JP				Strcpy(edifice, "drawbridge");*/
				Strcpy(edifice, "跳ね橋");
			else
				if (at_portcullis) 
/*JP					Strcpy(edifice, "portcullis");*/
					Strcpy(edifice, "落し格子");
			if (*edifice)
/*JP				pline("The %s passes through %s!", edifice, */
				pline("%sは%sを通り抜けた！", edifice, 
				      e_nam(etmp));
		}
		return;
	}
	if (e_missed(etmp, FALSE)) { 
		if (at_portcullis)
/*JP			pline("The portcullis misses %s!",*/
			pline("落し格子が%sに命中した！",
			      e_nam(etmp));
#ifdef D_DEBUG
		else
			pline("The drawbridge misses %s!", 
			      e_nam(etmp));
#endif
		if (e_survives_at(etmp, oldx, oldy)) 
			return;
		else {
#ifdef D_DEBUG
			pline("Mon can't survive here");
#endif
			if (at_portcullis)
				must_jump = TRUE;
			else
				relocates = TRUE; /* just ride drawbridge in */
		}
	} else {
		if (crm->typ == DRAWBRIDGE_DOWN) {
/*JP			pline("%s crushed underneath the drawbridge.",*/
			pline("%s跳ね橋の下敷になった．",
			      E_phrase(etmp, "are"));		  /* no jump */
			e_died(etmp, e_inview? 3 : 2, CRUSHING);/* no corpse */
			return;   /* Note: Beyond this point, we know we're  */
		}		  /* not at an opened drawbridge, since all  */
		must_jump = TRUE; /* *missable* creatures survive on the     */
	}			  /* square, and all the unmissed ones die.  */
	if (must_jump) {
	    if (at_portcullis) {
		if (e_jumps(etmp)) {
		    relocates = TRUE;
#ifdef D_DEBUG
		    pline("Jump succeeds!");
#endif
		} else {
		    if (e_inview)
/*JP			pline("%s crushed by the falling portcullis!",*/
			pline("%s落ちてきた落し格子に潰された！",
			      E_phrase(etmp, "are"));
		    else if (flags.soundok)
/*JP			You("hear a crushing sound.");*/
			You("何かが潰れる音を聞いた．");
		    e_died(etmp, e_inview? 3 : 2, CRUSHING);
		    /* no corpse */
		    return;
		}
	    } else { /* tries to jump off bridge to original square */
		relocates = !e_jumps(etmp); 
#ifdef D_DEBUG
		pline("Jump %s!", (relocates)? "fails" : "succeeds");
#endif
	    }
	}

/*
 * Here's where we try to do relocation.  Assumes that etmp is not arriving
 * at the portcullis square while the drawbridge is falling, since this square
 * would be inaccessible (i.e. etmp started on drawbridge square) or 
 * unnecessary (i.e. etmp started here) in such a situation.
 */
#ifdef D_DEBUG
	pline("Doing relocation.");
#endif
	newx = oldx;
	newy = oldy;
	(void)find_drawbridge(&newx, &newy);
	if ((newx == oldx) && (newy == oldy))
		get_wall_for_db(&newx, &newy);
#ifdef D_DEBUG
	pline("Checking new square for occupancy.");
#endif
	if (relocates && (e_at(newx, newy))) { 

/* 
 * Standoff problem:  one or both entities must die, and/or both switch 
 * places.  Avoid infinite recursion by checking first whether the other 
 * entity is staying put.  Clean up if we happen to move/die in recursion.
 */
		struct entity *other;

		other = e_at(newx, newy);
#ifdef D_DEBUG
		pline("New square is occupied by %s", e_nam(other));
#endif
		if (e_survives_at(other, newx, newy) && automiss(other)) {
			relocates = FALSE;     	      /* "other" won't budge */
#ifdef D_DEBUG
			pline("%s suicide.", E_phrase(etmp, "commit"));
#endif
		} else {

#ifdef D_DEBUG
			pline("Handling %s", e_nam(other));
#endif
			while ((e_at(newx, newy)) && 
			       (e_at(newx, newy) != etmp))
		       		do_entity(other);
#ifdef D_DEBUG
			pline("Checking existence of %s", e_nam(etmp));
			wait_synch();
#endif
			if (e_at(oldx, oldy) != etmp) {
#ifdef D_DEBUG
			    pline("%s moved or died in recursion somewhere",
				  E_phrase(etmp, "have"));
			    wait_synch();
#endif
			    return;
			}
		}
	}
	if (relocates && !e_at(newx, newy)) {/* if e_at() entity = worm tail */
#ifdef D_DEBUG
		pline("Moving %s", e_nam(etmp));
#endif
		if (!is_u(etmp)) {
			remove_monster(etmp->ex, etmp->ey);
			place_monster(etmp->emon, newx, newy);
		} else {
			u.ux = newx;
			u.uy = newy;
		}
		etmp->ex = newx;
		etmp->ey = newy;
		e_inview = e_canseemon(etmp);
	}
#ifdef D_DEBUG
	pline("Final disposition of %s", e_nam(etmp));
	wait_synch();
#endif
	if (is_db_wall(etmp->ex, etmp->ey)) {
#ifdef D_DEBUG
		pline("%s in portcullis chamber", E_phrase(etmp, "are"));
		wait_synch();
#endif
		if (e_inview) {
			if (is_u(etmp)) {
/*JP				You("tumble towards the closed portcullis!"); */
				You("閉まりかけの落し格子をころぶようにすりぬけた！");
				if (automiss(etmp))
/*JP					You("pass through it!");*/
					You("通りぬけた！");
				else
/*JP					pline("The drawbridge closes in...");*/
					pline("跳ね橋は閉じた．．．");
			} else
/*JP				pline("%s behind the drawbridge.",*/
				pline("%s跳ね橋の裏に消えた．",
				      E_phrase(etmp, "disappear"));
		}
		if (!e_survives_at(etmp, etmp->ex, etmp->ey)) {
			killer_format = KILLED_BY_AN;
/*JP			killer = "closing drawbridge";*/
			killer = "閉じる跳ね橋で";
			e_died(etmp, 0, CRUSHING); 	       /* no message */
			return;
		}
#ifdef D_DEBUG
		pline("%s in here", E_phrase(etmp, "survive"));
#endif
	} else {
#ifdef D_DEBUG
		pline("%s on drawbridge square", E_phrase(etmp, "are"));
#endif
		if (is_pool(etmp->ex, etmp->ey) && !e_inview)
			if (flags.soundok)
/*JP				You("hear a splash.");*/
				You("パシャパシャと言う音を聞いた．");
		if (e_survives_at(etmp, etmp->ex, etmp->ey)) {
			if (e_inview && !is_flyer(etmp->edata) &&
			    !is_floater(etmp->edata))
/*JP				pline("%s from the bridge.",*/
				pline("%s橋から落ちた",
				      E_phrase(etmp, "fall"));
			return;
		}
#ifdef D_DEBUG
		pline("%s cannot survive on the drawbridge square",Enam(etmp));
#endif
		if (is_pool(etmp->ex, etmp->ey) || is_lava(etmp->ex, etmp->ey))
		    if (e_inview && !is_u(etmp)) {
			/* drown() will supply msgs if nec. */
			boolean lava = is_lava(etmp->ex, etmp->ey);

			if (Hallucination)
/*JP			    pline("%s the %s and disappears.",
				  E_phrase(etmp, "drink"),
				  lava ? "lava" : "moat");*/
			    pline("%s%sを飲み，消えた．",
				  E_phrase(etmp, "drink"),
				  lava ? "溶岩" : "堀");
			else
/*JP			    pline("%s into the %s.",
				  E_phrase(etmp, "fall"),
				  lava ? "lava" : "moat");*/
			    pline("%s%sの中に落ちた．",
				  E_phrase(etmp, "fall"),
				  lava ? "溶岩" : "堀");
		    }
		killer_format = NO_KILLER_PREFIX;
/*JP		killer = "fell from a drawbridge";*/
		killer = "跳ね橋から落ちて";
		e_died(etmp, e_inview ? 3 : 2,      /* CRUSHING is arbitrary */
		       (is_pool(etmp->ex, etmp->ey)) ? DROWNING :
		       (is_lava(etmp->ex, etmp->ey)) ? BURNING :
						       CRUSHING); /*no corpse*/
		return;
	}
}

/*
 * Close the drawbridge located at x,y
 */

void
close_drawbridge(x,y)
int x,y;
{
	register struct rm *lev1, *lev2;
	int x2, y2;

	lev1 = &levl[x][y];
	if (lev1->typ != DRAWBRIDGE_DOWN) return;
	x2 = x; y2 = y;
	get_wall_for_db(&x2,&y2);
	if (cansee(x,y))  /* change msgs if you are a w-walker at portcullis */
/*JP		You("see a drawbridge %s up!", 
		    ((u.ux == x2) && (u.uy == y2))? "coming" : "going");*/
		You("跳ね橋が閉じていくのを見た！");
	lev1->typ = DRAWBRIDGE_UP;
	lev2 = &levl[x2][y2];
	lev2->typ = DBWALL;
	switch (lev1->drawbridgemask & DB_DIR) {
		case DB_NORTH:
		case DB_SOUTH:
			lev2->horizontal = TRUE;
			break;
		case DB_WEST:
		case DB_EAST:
			lev2->horizontal = FALSE;
			break;
	}
	lev2->diggable = W_NONDIGGABLE;
	set_entity(x, y, &(occupants[0]));
	set_entity(x2, y2, &(occupants[1]));
	do_entity(&(occupants[0]));		/* Do set_entity after first */
	set_entity(x2, y2, &(occupants[1]));	/* do_entity for worm tail */
	do_entity(&(occupants[1]));
	if(OBJ_AT(x,y) && flags.soundok)
/*JP	    You("hear smashing and crushing.");*/
	    You("ガシャン，ガランと言う音を聞いた．");
	(void) revive_nasty(x,y,NULL);
	(void) revive_nasty(x2,y2,NULL);
	delallobj(x, y);
	newsym(x, y);
	delallobj(x2, y2);
	newsym(x2, y2);
	block_point(x2,y2);	/* vision */
}

/* 
 * Open the drawbridge located at x,y
 */

void
open_drawbridge(x,y)
int x,y;
{
	register struct rm *lev1, *lev2;
	int x2, y2;

	lev1 = &levl[x][y];
	if (lev1->typ != DRAWBRIDGE_UP) return;
	x2 = x; y2 = y;
	get_wall_for_db(&x2,&y2);
	if (cansee(x,y))  /* change msgs if you are a w-walker at portcullis */
/*JP		You("see a drawbridge %s down!",
		    ((x2 == u.ux) && (y2 == u.uy))? "going" : "coming");*/
		You("跳ね橋が開くのを見た！");
	lev1->typ = DRAWBRIDGE_DOWN;
	lev2 = &levl[x2][y2];
	lev2->typ = DOOR;
	lev2->doormask = D_NODOOR;
	set_entity(x, y, &(occupants[0]));
	set_entity(x2, y2, &(occupants[1]));
	do_entity(&(occupants[0]));		/* do set_entity after first */
	set_entity(x2, y2, &(occupants[1]));	/* do_entity for worm tails */
	do_entity(&(occupants[1]));
	spoteffects();  /* if underwater, hero is now on solid ground */
	(void) revive_nasty(x,y,NULL);
	delallobj(x, y);
	newsym(x, y);
	newsym(x2, y2);
	unblock_point(x2,y2);	/* vision */
	if (Is_stronghold(&u.uz)) u.uevent.uopened_dbridge = TRUE;
}

/*
 * Let's destroy the drawbridge located at x,y
 */

void
destroy_drawbridge(x,y)
int x,y;
{
	register struct rm *lev1, *lev2;
	int x2, y2;
	boolean e_inview;
	struct entity *etmp1 = &(occupants[0]), *etmp2 = &(occupants[1]);

	lev1 = &levl[x][y];
	if (!IS_DRAWBRIDGE(lev1->typ))
		return;
	x2 = x; y2 = y;
	get_wall_for_db(&x2,&y2);
	lev2 = &levl[x2][y2];
	if ((lev1->drawbridgemask & DB_UNDER) == DB_MOAT ||
	    (lev1->drawbridgemask & DB_UNDER) == DB_LAVA) {
		struct obj *otmp;
		boolean lava = (lev1->drawbridgemask & DB_UNDER) == DB_LAVA;
		if (lev1->typ == DRAWBRIDGE_UP) {
			if (cansee(x2,y2))
/*JP			    pline("The portcullis of the drawbridge falls into the %s!",
				  lava ? "lava" : "moat");*/
			    pline("跳ね橋の落し格子が%sに落ちた！",
				  lava ? "溶岩" : "堀");
			else if (flags.soundok)
/*JP				You("hear a loud *SPLASH*!");*/
				You("大きなバッシャーンと言う音を聞いた！");
		} else {
			if (cansee(x,y))
/*JP			    pline("The drawbridge collapses into the %s!",
				  lava ? "lava" : "moat");*/
			    pline("跳ね橋は%sにくずれ落ちた！",
				  lava ? "溶岩" : "堀");
			else if (flags.soundok)
/*JP				You("hear a loud *SPLASH*!");*/
				You("大きなバッシャーンと言う音を聞いた！");
		}
		lev1->typ = lava ? LAVAPOOL : MOAT;
		lev1->drawbridgemask = 0;
		if ((otmp = sobj_at(BOULDER,x,y)) != 0) {
		    freeobj(otmp);
/*JP		    (void) flooreffects(otmp,x,y,"fall");*/
		    (void) flooreffects(otmp,x,y,"落ちる");
		}
	} else {
		if (cansee(x,y))
/*JP			pline("The drawbridge disintegrates!");*/
			pline("跳ね橋はこなごなになった！");
		else
/*JP			You("hear a loud *CRASH*!");*/
			You("大きなガシャーンと言う音を聞いた！");
		lev1->typ =
			((lev1->drawbridgemask & DB_ICE) ? ICE : ROOM);
		lev1->icedpool =
			((lev1->drawbridgemask & DB_ICE) ? ICED_MOAT : 0);
	}
	wake_nearby();
	lev2->typ = DOOR;
	lev2->doormask = D_NODOOR;
	newsym(x,y);
	newsym(x2,y2);
	if (!does_block(x2,y2,lev2)) unblock_point(x2,y2);	/* vision */
	if (Is_stronghold(&u.uz)) u.uevent.uopened_dbridge = TRUE;

	set_entity(x2, y2, etmp2); /* currently only automissers can be here */
	if (etmp2->edata) {
		e_inview = e_canseemon(etmp2);
		if (!automiss(etmp2)) {
			if (e_inview)
/*JP				pline("%s blown apart by flying debris.",*/
				pline("%s飛び散った瓦礫の破片を浴びた．",
				      E_phrase(etmp2, "are"));
			killer_format = KILLED_BY_AN;
/*JP			killer = "exploding drawbridge";*/
			killer = "跳ね橋の爆発で";
			e_died(etmp2, e_inview? 3 : 2, CRUSHING); /*no corpse*/
		}	     /* nothing which is vulnerable can survive this */
	}
	set_entity(x, y, etmp1);
	if (etmp1->edata) {
		e_inview = e_canseemon(etmp1);
		if (e_missed(etmp1, TRUE)) {
#ifdef D_DEBUG
			pline("%s spared!", E_phrase(etmp1, "are"));
#endif
		} else {
			if (e_inview) {
			    if (!is_u(etmp1) && Hallucination)
/*JP				pline("%s into some heavy metal",*/
				pline("%s何か重金属になった",
				      E_phrase(etmp1, "get"));
			    else
/*JP				pline("%s hit by a huge chunk of metal!",
				      E_phrase(etmp1, "are"));*/
				pline("大きな金属のかたまりが%sに命中した！",
				      e_nam(etmp1));
			} else {
			    if (flags.soundok && !is_u(etmp1) && !is_pool(x,y))
/*JP				You("hear a crushing sound");*/
				You("ガシャンと言う音を聞いた");
#ifdef D_DEBUG
			    else
				pline("%s from shrapnel", 

				      E_phrase(etmp1, "die"));
#endif
			}
			killer_format = KILLED_BY_AN;
/*JP			killer = "collapsing drawbridge";*/
			killer = "跳ね橋の崩解で";
			e_died(etmp1, e_inview? 3 : 2, CRUSHING); /*no corpse*/
			if(lev1->typ == MOAT) do_entity(etmp1);
		}
	}
}


#endif /* OVLB */

/*dbridge.c*/
