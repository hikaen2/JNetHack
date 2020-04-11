/*	SCCS Id: @(#)priest.c	3.1	93/05/15	*/
/* Copyright (c) Izchak Miller, Steve Linhart, 1989. 		  */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "mfndpos.h"
#include "eshk.h"
#include "epri.h"
#include "emin.h"

#ifdef OVLB

static boolean FDECL(histemple_at,(struct monst *,XCHAR_P,XCHAR_P));
static boolean FDECL(has_shrine,(struct monst *));

/*
 * Move for priests and shopkeepers.  Called from shk_move() and pri_move().
 * Valid returns are  1: moved  0: didn't  -1: let m_move do it  -2: died.
 */
int
move_special(mtmp,in_his_shop,appr,uondoor,avoid,omx,omy,gx,gy)
register struct monst *mtmp;
boolean in_his_shop;
schar appr;
boolean uondoor,avoid;
register xchar omx,omy,gx,gy;
{
	register xchar nx,ny,nix,niy;
	register schar i;
	schar chcnt,cnt;
	coord poss[9];
	long info[9];
	long allowflags;
	struct obj *ib = (struct obj *)0;

	if(omx == gx && omy == gy)
		return(0);
	if(mtmp->mconf) {
		avoid = FALSE;
		appr = 0;
	}

	nix = omx;
	niy = omy;
	if (mtmp->isshk) allowflags = ALLOW_SSM;
	else allowflags = ALLOW_SSM | ALLOW_SANCT;
	if (passes_walls(mtmp->data)) allowflags |= (ALLOW_ROCK|ALLOW_WALL);
	if (throws_rocks(mtmp->data)) allowflags |= ALLOW_ROCK;
	if (tunnels(mtmp->data) &&
		    (!needspick(mtmp->data) || m_carrying(mtmp, PICK_AXE)))
		allowflags |= ALLOW_DIG;
	if (!nohands(mtmp->data) && !verysmall(mtmp->data)) {
		allowflags |= OPENDOOR;
		if (m_carrying(mtmp, SKELETON_KEY)) allowflags |= BUSTDOOR;
	}
	if (is_giant(mtmp->data)) allowflags |= BUSTDOOR;
	cnt = mfndpos(mtmp, poss, info, allowflags);

	if(mtmp->isshk && avoid && uondoor) { /* perhaps we cannot avoid him */
		for(i=0; i<cnt; i++)
		    if(!(info[i] & NOTONL)) goto pick_move;
		avoid = FALSE;
	}

#define	GDIST(x,y)	(dist2(x,y,gx,gy))
pick_move:
	chcnt = 0;
	for(i=0; i<cnt; i++) {
		nx = poss[i].x;
		ny = poss[i].y;
		if(levl[nx][ny].typ == ROOM ||
			(mtmp->ispriest &&
			    levl[nx][ny].typ == ALTAR) ||
			(mtmp->isshk &&
			    (!in_his_shop || ESHK(mtmp)->following))) {
		    if(avoid && (info[i] & NOTONL))
			continue;
		    if((!appr && !rn2(++chcnt)) ||
			(appr && GDIST(nx,ny) < GDIST(nix,niy))) {
			    nix = nx;
			    niy = ny;
		    }
		}
	}
	if(mtmp->ispriest && avoid &&
			nix == omx && niy == omy && onlineu(omx,omy)) {
		/* might as well move closer as long it's going to stay
		 * lined up */
		avoid = FALSE;
		goto pick_move;
	}

	if(nix != omx || niy != omy) {
		remove_monster(omx, omy);
		place_monster(mtmp, nix, niy);
		newsym(nix,niy);
		if (mtmp->isshk && !in_his_shop && inhishop(mtmp))
		    check_special_room(FALSE);
		if(ib) {
			if (cansee(mtmp->mx,mtmp->my))
/*JP			    pline("%s picks up %s.", Monnam(mtmp),*/
			    pline("%sは%sを拾った．", Monnam(mtmp),
				distant_name(ib,doname));
			freeobj(ib);
			mpickobj(mtmp, ib);
		}
		return(1);
	}
	return(0);
}

#endif /* OVLB */

#ifdef OVL0

char
temple_occupied(array)
register char *array;
{
	register char *ptr;

	for (ptr = array; *ptr; ptr++)
		if (rooms[*ptr - ROOMOFFSET].rtype == TEMPLE)
			return(*ptr);
	return('\0');
}

#endif /* OVL0 */
#ifdef OVLB

static boolean
histemple_at(priest, x, y)
register struct monst *priest;
register xchar x, y;
{
	return((boolean)((EPRI(priest)->shroom == *in_rooms(x, y, TEMPLE)) &&
	       on_level(&(EPRI(priest)->shrlevel), &u.uz)));
}

/*
 * pri_move: return 1: moved  0: didn't  -1: let m_move do it  -2: died
 */
int
pri_move(priest)
register struct monst *priest;
{
	register xchar gx,gy,omx,omy;
	schar temple;
	boolean avoid = TRUE;

	omx = priest->mx;
	omy = priest->my;

	if(!histemple_at(priest, omx, omy)) return(-1);

	temple = EPRI(priest)->shroom;

	gx = EPRI(priest)->shrpos.x;
	gy = EPRI(priest)->shrpos.y;

	gx += rn1(3,-1);	/* mill around the altar */
	gy += rn1(3,-1);

	if(!priest->mpeaceful ||
	   (Conflict && !resist(priest, RING_CLASS, 0, 0))) {
		if(monnear(priest, u.ux, u.uy)) {
			if(Displaced)
/*JP				Your("displaced image doesn't fool %s!",*/
			        Your("幻影は%sをだませなかった！",
					mon_nam(priest));
			(void) mattacku(priest);
			return(0);
		} else if(index(u.urooms, temple)) {
			/* chase player if inside temple & can see him */
			if(priest->mcansee && m_canseeu(priest)) {
				gx = u.ux;
				gy = u.uy;
			}
			avoid = FALSE;
		}
	} else if(Invis) avoid = FALSE;

	return(move_special(priest,FALSE,TRUE,FALSE,avoid,omx,omy,gx,gy));
}

/* exclusively for mktemple() */
void
priestini(lvl, sroom, sx, sy, sanctum)
d_level	*lvl;
struct mkroom *sroom;
int sx, sy;
boolean sanctum;   /* is it the seat of the high priest? */
{
	register struct monst *priest;
	register struct obj *otmp;
	register int cnt;

	if(MON_AT(sx+1, sy))
		rloc(m_at(sx+1, sy)); /* insurance */

	priest = (sanctum ? makemon(&mons[PM_HIGH_PRIEST], sx+1, sy)
			  : makemon(&mons[PM_ALIGNED_PRIEST], sx+1, sy));
	if (priest) {
		EPRI(priest)->shroom = (sroom - rooms) + ROOMOFFSET;
		EPRI(priest)->shralign = Amask2align(levl[sx][sy].altarmask);
		EPRI(priest)->shrpos.x = sx;
		EPRI(priest)->shrpos.y = sy;
		assign_level(&(EPRI(priest)->shrlevel), lvl);
		priest->mtrapseen = ~0;	/* traps are known */
		priest->mpeaceful = 1;
		priest->ispriest = 1;
		priest->msleep = 0;
		set_malign(priest); /* mpeaceful may have changed */

		/* now his/her goodies... */
		(void) mongets(priest, CHAIN_MAIL);
		(void) mongets(priest, SMALL_SHIELD);
#ifdef MUSE
		m_dowear(priest, TRUE);
#endif
		priest->mgold = (long)rn1(10,20);
		if(sanctum && EPRI(priest)->shralign == A_NONE &&
		     on_level(&sanctum_level, &u.uz))
			(void) mongets(priest, AMULET_OF_YENDOR);
		/* Do NOT put the rest in m_initinv.    */
		/* Priests created elsewhere than in a  */
		/* temple should not carry these items, */
		/* except for the mace.			*/
		cnt = rn1(2,3);
		while(cnt) {
		    otmp = mkobj(SPBOOK_CLASS, FALSE);
		    if(otmp) mpickobj(priest, otmp);
		    cnt--;
		}
		if(p_coaligned(priest))
		    (void) mongets(priest, rn2(2) ? CLOAK_OF_PROTECTION
						  : CLOAK_OF_MAGIC_RESISTANCE);
		else {
		    if(!rn2(5))
			otmp = mksobj(CLOAK_OF_MAGIC_RESISTANCE, TRUE, FALSE);
		    else otmp = mksobj(CLOAK_OF_PROTECTION, TRUE, FALSE);
		    if(otmp) {
			if(!rn2(2)) curse(otmp);
			mpickobj(priest, otmp);
		    }
		}

		otmp = mksobj(MACE, FALSE, FALSE);
		if(otmp) {
		    otmp->spe = rnd(3);
		    if(!rn2(2)) curse(otmp);
		    mpickobj(priest, otmp);
		}
	}
}

/*
 * Specially aligned monsters are named specially.
 *	- aligned priests with ispriest and high priests have shrines
 *		they retain ispriest and epri when polymorphed
 *	- aligned priests without ispriest and Angels are roamers
 *		they retain isminion and access epri as emin when polymorphed
 *		(coaligned Angels are also created as minions, but they
 *		use the same naming convention)
 *	- minions do not have ispriest but have isminion and emin
 */
char *
priestname(mon)
register struct monst *mon;
{
	static NEARDATA char pname[PL_NSIZ];

/*JP	Strcpy(pname, "the ");*/
	Strcpy(pname, "");
/*JP	if (mon->minvis) Strcat(pname, "invisible ");*/
	if (mon->minvis) Strcat(pname, "透明な");
	if (mon->ispriest || mon->data == &mons[PM_ALIGNED_PRIEST] ||
					mon->data == &mons[PM_ANGEL]) {
		/* use epri */
		Strcat(pname, align_gname((int)EPRI(mon)->shralign));
		Strcat(pname, "の");
		if (mon->mtame && mon->data == &mons[PM_ANGEL]){
/*JP			Strcat(pname, "guardian ");*/
			Strcat(pname, "警護の");
		}
		if (mon->data != &mons[PM_ALIGNED_PRIEST] &&
				mon->data != &mons[PM_HIGH_PRIEST]) {
			Strcat(pname, jtrns_mon(mon->data->mname));
/*JP			Strcat(pname, " ");*/
		}
		if (mon->data != &mons[PM_ANGEL]) {
			if (!mon->ispriest && EPRI(mon)->renegade)
/*JP				Strcat(pname, "renegade ");*/
				Strcat(pname, "裏切り者の");
			if (mon->data == &mons[PM_HIGH_PRIEST])
/*JP				Strcat(pname, "high ");*/
				Strcat(pname, "位の高い");
			if (mon->female)
/*JP				Strcat(pname, "priestess ");*/
				Strcat(pname, "尼僧");
			else
/*JP				Strcat(pname, "priest ");*/
				Strcat(pname, "僧侶");
		}
/*JP		Strcat(pname, "of ");*/
/*JP		Strcat(pname, align_gname((int)EPRI(mon)->shralign));*/
/*		Strcat(pname, jtrns_mon(mon->data->mname));*/
		return(pname);
	}
	/* use emin instead of epri */
/*JP	Strcat(pname, mon->data->mname);
	Strcat(pname, " of ");
	Strcat(pname, align_gname(EMIN(mon)->min_align));*/
	Strcat(pname, align_gname(EMIN(mon)->min_align));
	Strcat(pname, "の化身の");
	Strcat(pname, jtrns_mon(mon->data->mname));
	return(pname);
}

boolean
p_coaligned(priest)
struct monst *priest;
{
	return((boolean)(u.ualign.type == ((int)EPRI(priest)->shralign)));
}

static boolean
has_shrine(pri)
struct monst *pri;
{
	struct rm *lev;

	if(!pri)
		return(FALSE);
	lev = &levl[EPRI(pri)->shrpos.x][EPRI(pri)->shrpos.y];
	if (!IS_ALTAR(lev->typ) || !(lev->altarmask & AM_SHRINE))
		return(FALSE);
	return((boolean)(EPRI(pri)->shralign == Amask2align(lev->altarmask & ~AM_SHRINE)));
}

struct monst *
findpriest(roomno)
char roomno;
{
	register struct monst *mtmp;
	extern struct monst *fdmon; /* from mon.c */

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if(mtmp->ispriest && (EPRI(mtmp)->shroom == roomno) &&
	       histemple_at(mtmp,mtmp->mx,mtmp->my))
		return(mtmp);
	for(mtmp = fdmon; mtmp; mtmp = mtmp->nmon)
	    if(mtmp->ispriest && (EPRI(mtmp)->shroom == roomno) &&
	       histemple_at(mtmp,mtmp->mx,mtmp->my))
		return(mtmp);
	return (struct monst *)0;
}

/* called from check_special_room() when the player enters the temple room */
void
intemple(roomno)
register int roomno;
{
	register struct monst *priest = findpriest((char)roomno);
	boolean tended = (priest != (struct monst *)0);
	boolean shrined = (tended && has_shrine(priest));
	boolean sanctum = (tended && priest->data == &mons[PM_HIGH_PRIEST] &&
			    (Is_sanctum(&u.uz) || In_endgame(&u.uz)));

	if(!temple_occupied(u.urooms0)) {
	    if(tended) {
/*JP		pline("%s intones:",
		      (!Blind ? Monnam(priest) : "A nearby voice"));*/
	        if(!Blind)
		  pline("%sは詠唱した:",Monnam(priest));
		else
		  pline("詠唱が聞こえた:");
		if(sanctum && Is_sanctum(&u.uz)) {
		    if(priest->mpeaceful) {
/*JP			  verbalize("Infidel, you entered Moloch's Sanctum!");
			  verbalize("Be gone!");*/
			  verbalize("異端者よ！ここは，モーロックの聖域だ！");
			  verbalize("立ちされ！");
			  priest->mpeaceful = 0;
			  set_malign(priest);
		    } else
/*JP		      verbalize("You desecrate this place by your presence!");*/
		      verbalize("おまえはこの神聖な場所を汚している！");
/*JP		} else verbalize("Pilgrim, you enter a%s place!",
			   !shrined ? " desecrated" :
			   " sacred");*/
		} else verbalize("巡礼者よ，おまえは%s地にいる！",
			   !shrined ? "不浄の" :
			   "神聖なる");
		if(!sanctum) {
		    /* !tended -> !shrined */
		    if(!shrined || !p_coaligned(priest) ||
						   u.ualign.record < -5)
/*JP			You("have a%s forbidding feeling...",
				(!shrined) ? "" : " strange");*/
			You("%s近づきがたい気持がした．．．",
				(!shrined) ? "" : "奇妙な");
/*JP		    else You("experience a strange sense of peace.");*/
		    else You("奇妙な秩序ある雰囲気を体験した．");
		}
	    } else {
		switch(rn2(3)) {
/*JP		  case 0: You("have an eerie feeling..."); break;*/
		  case 0: You("ぞっとした．．．"); break;
/*JP		  case 1: You("feel like you are being watched."); break;*/
		  case 1: You("見つめられているような気がした．"); break;
/*JP		  default: pline("A shiver runs down your %s.",*/
		  default: pline("震えがあなたの%sを走った．",
			body_part(SPINE)); break;
		}
		if(!rn2(5)) {
		    struct monst *mtmp;

		    if(!(mtmp = makemon(&mons[PM_GHOST],u.ux,u.uy))) return;
/*JP		    pline("An enormous ghost appears next to you!");*/
		    pline("巨大な幽霊があなたの隣に現われた！");
		    mtmp->mpeaceful = 0;
		    set_malign(mtmp);
		    if(flags.verbose)
/*JP			You("are frightened to death, and unable to move.");*/
			You("まっさおになって驚き，動けなくなった．");
		    nomul(-3);
/*JP		    nomovemsg = "You regain your composure.";*/
		    nomovemsg = "あなたは平静を取り戻した．";
	       }
	   }
       }
}

void
priest_talk(priest)
register struct monst *priest;
{
	boolean coaligned = p_coaligned(priest);
	boolean strayed = (u.ualign.record < 0);

	if(priest->mflee || (!priest->ispriest && coaligned && strayed)) {
/*JP	    pline("%s doesn't want anything to do with you!",*/
	    pline("%sはあなたにかまいたくないようだ！",
				Monnam(priest));
	    priest->mpeaceful = 0;
	    return;
	}

	/* priests don't chat unless peaceful and in their own temple */
	if(!histemple_at(priest,priest->mx,priest->my) ||
		 !priest->mpeaceful || !priest->mcanmove || priest->msleep) {
	    if(!priest->mcanmove || priest->msleep) {
/*JP		pline("%s breaks out of %s reverie!",*/
		pline("%sは%sの冥想を中断した！",
		      Monnam(priest), his[pronoun_gender(priest)]);
		priest->mfrozen = priest->msleep = 0;
		priest->mcanmove = 1;
	    }
	    priest->mpeaceful = 0;
	    switch(rn2(3)) {
		case 0:
/*JP		   verbalize("Thou wouldst have words, eh?  I'll give thee a word or two!");*/
		   verbalize("汝言葉を望むのか？");
		   break;
		case 1:
/*JP		   verbalize("Talk?  Here is what I have to say!");*/
		   verbalize("話す？何を言えばよいのだ！");
		   break;
		default:
/*JP		   verbalize("Pilgrim, I would speak no longer with thee.");*/
		   verbalize("巡礼者よ，汝に語ることなどない．");
		   break;
	    }
	    return;
	}

	/* you desecrated the temple and now you want to chat? */
	if(priest->mpeaceful && *in_rooms(priest->mx, priest->my, TEMPLE) &&
		  !has_shrine(priest)) {
/*JP	    verbalize("Begone!  Thou desecratest this holy place with thy presence.");*/

	  verbalize("立ち去れ！汝はこの神聖なる場所を汚している．");
	    priest->mpeaceful = 0;
	    return;
	}

	if(!u.ugold) {
	    if(coaligned && !strayed) {
		if (priest->mgold > 0L) {
		    /* Note: two bits is actually 25 cents.  Hmm. */
/*JP		    pline("%s gives you %s for an ale.", Monnam(priest),
			(priest->mgold == 1L) ? "one bit" : "two bits");*/
		    pline("%sはあなたに%sエール酒を与えた．", Monnam(priest),
			(priest->mgold == 1L) ? "一口" : "二口");
		    if (priest->mgold > 1L)
			u.ugold = 2L;
		    else
			u.ugold = 1L;
		    priest->mgold -= u.ugold;
		    flags.botl = 1;
		} else
/*JP		    pline("%s preaches the virtues of poverty.", Monnam(priest));*/
		    pline("%sは貧困の美徳について説教した．", Monnam(priest));
		exercise(A_WIS, TRUE);
	    } else
/*JP		pline("%s is not interested.", Monnam(priest));*/
		pline("%sは興味を示さない．", Monnam(priest));
	    return;
	} else {
	    long offer;

/*JP	    pline("%s asks you for a contribution for the temple.",*/
	    pline("%sはあなたに寺院への寄贈を求めた．",
			Monnam(priest));
	    if((offer = bribe(priest)) == 0) {
/*JP		verbalize("Thou shalt regret thine action!");*/
		verbalize("汝の行為は神を冒涜するものなり！");
		if(coaligned) u.ualign.record--;
	    } else if(offer < (u.ulevel * 200)) {
/*JP		if(u.ugold > (offer * 2L)) verbalize("Cheapskate.");*/
		if(u.ugold > (offer * 2L)) verbalize("ケチめ．");
		else {
/*JP		    verbalize("I thank thee for thy contribution.");*/
		    verbalize("汝の寄贈に報いようぞ．");
		    /*  give player some token  */
		    exercise(A_WIS, TRUE);
		}
	    } else if(offer < (u.ulevel * 400)) {
/*JP		verbalize("Thou art indeed a pious individual.");*/
		verbalize("汝，まさに敬虔な人物なり．");
		if(u.ugold < (offer * 2L)) {
		    if(coaligned && u.ualign.record < -5) u.ualign.record++;
/*JP		    verbalize("I bestow upon thee a blessing.");*/
		    verbalize("汝に祝福を．");
		    Clairvoyant += rn1(500,500);
		}
	    } else if(offer < (u.ulevel * 600) &&
		      u.ublessed < 20 &&
		      (u.ublessed < 9 || !rn2(u.ublessed))) {
/*JP		verbalize("Thy devotion has been rewarded.");*/
		verbalize("汝の献身に報わん．");
		if (!(Protection & INTRINSIC))  {
			Protection |= FROMOUTSIDE;
			if (!u.ublessed)  u.ublessed = rn1(3, 2);
		} else u.ublessed++;
	    } else {
/*JP		verbalize("Thy selfless generosity is deeply appreciated.");*/
		verbalize("汝自身の真価は大いに認められた．");
		if(u.ugold < (offer * 2L) && coaligned) {
		    if(strayed && (moves - u.ucleansed) > 5000L) {
			u.ualign.record = 0; /* cleanse thee */
			u.ucleansed = moves;
		    } else {
			u.ualign.record += 2;
		    }
		}
	    }
	}
}

struct monst *
mk_roamer(ptr, alignment, x, y, peaceful)
register struct permonst *ptr;
aligntyp alignment;
xchar x, y;
boolean peaceful;
{
	register struct monst *roamer;
	register boolean coaligned = (u.ualign.type == alignment);

	if (ptr != &mons[PM_ALIGNED_PRIEST] && ptr != &mons[PM_ANGEL])
		return((struct monst *)0);
	
	if (MON_AT(x, y)) rloc(m_at(x, y));	/* insurance */

	if (!(roamer = makemon(ptr, x, y)))
		return((struct monst *)0);

	EPRI(roamer)->shralign = alignment;
	if (coaligned && !peaceful)
		EPRI(roamer)->renegade = TRUE;
	/* roamer->ispriest == FALSE naturally */
	roamer->isminion = TRUE;	/* borrowing this bit */
	roamer->mtrapseen = ~0;		/* traps are known */
	roamer->mpeaceful = peaceful;
	roamer->msleep = 0;
	set_malign(roamer); /* peaceful may have changed */

	/* MORE TO COME */
	return(roamer);
}

void
reset_hostility(roamer)
register struct monst *roamer;
{
        if(!(roamer->isminion && (roamer->data == &mons[PM_ALIGNED_PRIEST] ||
				  roamer->data == &mons[PM_ANGEL])))
	        return;

        if(EPRI(roamer)->shralign != u.ualign.type) {
	    roamer->mpeaceful = roamer->mtame = 0;
	    set_malign(roamer);
	}
	newsym(roamer->mx, roamer->my);
}

boolean
in_your_sanctuary(x, y)
xchar x, y;
{
	register char roomno;
	register struct monst *priest;

	if ((u.ualign.record < -5) || !(roomno = temple_occupied(u.urooms)) ||
	    (roomno != *in_rooms(x, y, TEMPLE)) ||
	    !(priest = findpriest(roomno)))
		return(FALSE);
	return((boolean)(has_shrine(priest) && p_coaligned(priest) && priest->mpeaceful));
}

void
ghod_hitsu(priest) 	/* when attacking "priest" in his temple */
struct monst *priest;
{
	int x, y, ax, ay, roomno = (int)temple_occupied(u.urooms);
	struct mkroom *troom;

	if (!roomno || !has_shrine(priest))
		return;

	ax = x = EPRI(priest)->shrpos.x;
	ay = y = EPRI(priest)->shrpos.y;
	troom = &rooms[roomno - ROOMOFFSET];

	if((u.ux == x && u.uy == y) || !linedup(u.ux, u.uy, x, y)) {
	    if(IS_DOOR(levl[u.ux][u.uy].typ)) {

		if(u.ux == troom->lx - 1) {
		    x = troom->hx;
		    y = u.uy;
		} else if(u.ux == troom->hx + 1) {
		    x = troom->lx;
		    y = u.uy;
		} else if(u.uy == troom->ly - 1) {
		    x = u.ux;
		    y = troom->hy;
		} else if(u.uy == troom->hy + 1) {
		    x = u.ux;
		    y = troom->ly;
		}
	    } else {
		switch(rn2(4)) {
		case 0:  x = u.ux; y = troom->ly; break;
		case 1:  x = u.ux; y = troom->hy; break;
		case 2:  x = troom->lx; y = u.uy; break;
		default: x = troom->hx; y = u.uy; break;
		}
	    }
	    if(!linedup(u.ux, u.uy, x, y)) return;
	}

	switch(rn2(3)) {
	case 0:
/*JP	    pline("%s roars in anger:  \"Thou shalt suffer!\"",*/
	    pline("%sは怒りのほえ声をあげた:「汝，苦しむがよい！」",
			a_gname_at(ax, ay));
	    break;
	case 1:
/*JP	    pline("%s voice booms:  \"How darest thou harm my servant!\"",*/
	    pline("%s声が響いた:「わが下僕に苦しむがよい！」",
			s_suffix(a_gname_at(ax, ay)));
	    break;
	default:
/*JP	    pline("%s roars:  \"Thou dost profane my shrine!\"",*/
	    pline("%sのほえ声が聞こえる:「汝，我が聖堂を汚したり」",
			a_gname_at(ax, ay));
	    break;
	}

	buzz(-10-(AD_ELEC-1), 6, x, y, sgn(tbx), sgn(tby)); /* bolt of lightning */
	exercise(A_WIS, FALSE);
}

void
angry_priest()
{
	register struct monst *priest;

	if ((priest = findpriest(temple_occupied(u.urooms))) != 0)
		wakeup(priest);
}

/*
 * When saving bones, find priests that aren't on their shrine level,
 * and remove them.   This avoids big problems when restoring bones.
 */
void
clearpriests()
{
    register struct monst *mtmp, *mtmp2;

    for(mtmp = fmon; mtmp; mtmp = mtmp2) {
	mtmp2 = mtmp->nmon;
	if (mtmp->ispriest && !on_level(&(EPRI(mtmp)->shrlevel), &u.uz))
	    mongone(mtmp);
    }
}

/* munge priest-specific structure when restoring -dlc */
void
restpriest(mtmp, ghostly)
register struct monst *mtmp;
boolean ghostly;
{
    if(u.uz.dlevel) {
	if (ghostly)
	    assign_level(&(EPRI(mtmp)->shrlevel), &u.uz);
    }
}

#endif /* OVLB */

/*priest.c*/
