/*	SCCS Id: @(#)minion.c	3.2	95/12/20	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "emin.h"
#include "epri.h"

void
msummon(ptr)		/* ptr summons a monster */
register struct permonst *ptr;
{
	register int dtype = 0, cnt = 0;
	aligntyp atyp = sgn(ptr->maligntyp);

	if (is_dprince(ptr) || (ptr == &mons[PM_WIZARD_OF_YENDOR])) {
	    dtype = (!rn2(20)) ? dprince(atyp) :
				 (!rn2(4)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && !is_ndemon(&mons[dtype])) ? 2 : 1;
	} else if (is_dlord(ptr)) {
	    dtype = (!rn2(50)) ? dprince(atyp) :
				 (!rn2(20)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && is_ndemon(&mons[dtype])) ? 2 : 1;
	} else if (is_ndemon(ptr)) {
	    dtype = (!rn2(20)) ? dlord(atyp) :
				 (!rn2(6)) ? ndemon(atyp) : monsndx(ptr);
	    cnt = 1;
	} else if (is_lminion(ptr)) {
	    dtype = (is_lord(ptr) && !rn2(20)) ? llord() :
		     (is_lord(ptr) || !rn2(6)) ? lminion() : monsndx(ptr);
	    cnt = (!rn2(4) && !is_lord(&mons[dtype])) ? 2 : 1;
	}

	if (!dtype) return;

	/* sanity checks */
	if (cnt > 1 && (mons[dtype].geno & G_UNIQ)) cnt = 1;
	/*
	 * If this daemon is unique and being re-summoned (the only way we
	 * could get this far with an extinct dtype), try another.
	 */
	if (mvitals[dtype].mvflags & G_GONE) {
	    dtype = ndemon(atyp);
	    if (!dtype) return;
	}

	while (cnt > 0) {
	    (void)makemon(&mons[dtype], u.ux, u.uy, NO_MM_FLAGS);
	    cnt--;
	}
	return;
}

void
summon_minion(alignment, talk)
aligntyp alignment;
boolean talk;
{
    register struct monst *mon;
    int mnum;

    switch ((int)alignment) {
	case A_LAWFUL:
	    mnum = lminion();
	    break;
	case A_NEUTRAL:
	    mnum = PM_AIR_ELEMENTAL + rn2(4);
	    break;
	case A_CHAOTIC:
	case A_NONE:
	    mnum = ndemon(alignment);
	    break;
	default:
	    impossible("unaligned player?");
	    mnum = ndemon(A_NONE);
	    break;
    }
    if (mons[mnum].pxlth == 0) {
	struct permonst *pm = &mons[mnum];
	pm->pxlth = sizeof(struct emin);
	mon = makemon(pm, u.ux, u.uy, NO_MM_FLAGS);
	pm->pxlth = 0;
	if (mon) {
	    mon->isminion = TRUE;
	    EMIN(mon)->min_align = alignment;
	}
    } else if (mnum == PM_ANGEL) {
	mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
	if (mon) {
	    mon->isminion = TRUE;
	    EPRI(mon)->shralign = alignment;	/* always A_LAWFUL here */
	}
    } else
	mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
    if (mon) {
	if (talk) {
/*JP	    pline_The("voice of %s booms:", align_gname(alignment));*/
	    pline("%sの声が響いた:", align_gname(alignment));
/*JP	    verbalize("Thou shalt pay for thy indiscretion!");*/
	    verbalize("汝，無分別な行動の罰を受けよ！");
	    if (!Blind)
/*JP		pline("%s appears before you.", Amonnam(mon));*/
		pline("%sがあなたの前に現われた．", Amonnam(mon));
	}
	mon->mpeaceful = FALSE;
	/* don't call set_malign(); player was naughty */
    }
}

#define Athome	(Inhell && !mtmp->cham)

int
demon_talk(mtmp)		/* returns 1 if it won't attack. */
register struct monst *mtmp;
{
	long	demand, offer;

	if (uwep && uwep->oartifact == ART_EXCALIBUR) {
/*JP	    pline("%s looks very angry.", Amonnam(mtmp));*/
	    pline("%sはとても怒っているように見える．", Amonnam(mtmp));
	    mtmp->mpeaceful = mtmp->mtame = 0;
	    newsym(mtmp->mx, mtmp->my);
	    return 0;
	}

	/* Slight advantage given. */
	if (is_dprince(mtmp->data) && mtmp->minvis) {
	    mtmp->minvis = mtmp->perminvis = 0;
/*JP	    if (!Blind) pline("%s appears before you.", Amonnam(mtmp));*/
	    if (!Blind) pline("%sが目の前に現われた．", Amonnam(mtmp));
	    newsym(mtmp->mx,mtmp->my);
	}
	if (u.usym == S_DEMON) {	/* Won't blackmail their own. */
/*JP	    pline("%s says, \"Good hunting, %s.\" and vanishes.",
		  Amonnam(mtmp), flags.female ? "Sister" : "Brother");*/
	    pline("%sは言った「よう兄%s！」．そして消えた．",
		  Amonnam(mtmp), flags.female ? "妹" : "弟");
	    rloc(mtmp);
	    return(1);
	}
	demand = (u.ugold * (rnd(80) + 20 * Athome)) /
		 100 * (1 + (sgn(u.ualign.type) == sgn(mtmp->data->maligntyp)));
	if (!demand)		/* you have no gold */
	    return mtmp->mpeaceful = 0;
	else {
/*JP	    pline("%s demands %ld zorkmid%s for safe passage.",
		  Amonnam(mtmp), demand, plur(demand));*/
	    pline("%sは通行料として%ldゴールド要求した．",
		  Amonnam(mtmp), demand);

	    if ((offer = bribe(mtmp)) >= demand) {
/*JP		pline("%s vanishes, laughing about cowardly mortals.",*/
		pline("死すべき卑しきものを笑いながら，%sは消えた．",
		      Amonnam(mtmp));
	    } else {
		if ((long)rnd(40) > (demand - offer)) {
/*JP		    pline("%s scowls at you menacingly, then vanishes.",*/
		    pline("%sはあなたを威嚇し，消えた．",
			  Amonnam(mtmp));
		} else {
/*JP		    pline("%s gets angry...", Amonnam(mtmp));*/
		    pline("%sは怒った．．．", Amonnam(mtmp));
		    return mtmp->mpeaceful = 0;
		}
	    }
	}
	mongone(mtmp);
	return(1);
}

long
bribe(mtmp)
struct monst *mtmp;
{
	char buf[BUFSZ];
	long offer;

/*JP	getlin("How much will you offer?", buf);*/
	getlin("何ゴールド与える？", buf);
	(void) sscanf(buf, "%ld", &offer);

	/*Michael Paddon -- fix for negative offer to monster*/
	/*JAR880815 - */
	if (offer < 0L) {
/*JP		You("try to shortchange %s, but fumble.",*/
		You("%sをだまそうとしたが，失敗した．",
			mon_nam(mtmp));
		offer = 0L;
	} else if (offer == 0L) {
/*JP		You("refuse.");*/
		You("拒んだ．");
	} else if (offer >= u.ugold) {
/*JP		You("give %s all your gold.", mon_nam(mtmp));*/
		You("%sにお金を全て与えた．", mon_nam(mtmp));
		offer = u.ugold;
/*JP	} else You("give %s %ld zorkmid%s.", mon_nam(mtmp), offer,
		   plur(offer));*/
	} else You("%sに%ldゴールド与えた．", mon_nam(mtmp), offer);

	u.ugold -= offer;
	mtmp->mgold += offer;
	flags.botl = 1;
	return(offer);
}

int
dprince(atyp)
aligntyp atyp;
{
	int tryct, pm;

	for (tryct = 0; tryct < 20; tryct++) {
	    pm = rn1(PM_DEMOGORGON + 1 - PM_ORCUS, PM_ORCUS);
	    if (!(mvitals[pm].mvflags & G_GONE) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(dlord(atyp));	/* approximate */
}

int
dlord(atyp)
aligntyp atyp;
{
	int tryct, pm;

	for (tryct = 0; tryct < 20; tryct++) {
	    pm = rn1(PM_YEENOGHU + 1 - PM_JUIBLEX, PM_JUIBLEX);
	    if (!(mvitals[pm].mvflags & G_GONE) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(ndemon(atyp));	/* approximate */
}

/* create lawful (good) lord */
int
llord()
{
	if (!(mvitals[PM_ARCHON].mvflags & G_GONE))
		return(PM_ARCHON);

	return(lminion());	/* approximate */
}

int
lminion()
{
	int	tryct;
	struct	permonst *ptr;

	for (tryct = 0; tryct < 20; tryct++) {
	    ptr = mkclass(S_ANGEL,0);
	    if (ptr && !is_lord(ptr))
		return(monsndx(ptr));
	}

	return(0);
}

int
ndemon(atyp)
aligntyp atyp;
{
	int	tryct;
	struct	permonst *ptr;

	for (tryct = 0; tryct < 20; tryct++) {
	    ptr = mkclass(S_DEMON, 0);
	    if (is_ndemon(ptr) &&
		    (atyp == A_NONE || sgn(ptr->maligntyp) == sgn(atyp)))
		return(monsndx(ptr));
	}

	return(0);
}

/*minion.c*/
