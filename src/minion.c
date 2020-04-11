/*	SCCS Id: @(#)minion.c	3.1	92/11/01	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
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
	register int dtype = 0, cnt = 0, atyp = sgn(ptr->maligntyp);

	if (is_dprince(ptr) || (ptr == &mons[PM_WIZARD_OF_YENDOR])) {

	    dtype = (!rn2(20)) ? dprince(atyp) :
				 (!rn2(4)) ? dlord(atyp) : ndemon(atyp);
	    cnt = (!rn2(4) && !is_dprince(&mons[dtype])) ? 2 : 1;

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

	/*
	 * If this daemon is unique and being re-summoned (the only way we
	 * could get this far with an extinct dtype), try another.
	 */
	if (mons[dtype].geno & (G_EXTINCT | G_GENOD)) {
	    dtype = ndemon(atyp);
	    if (!dtype) return;
	}

	while (cnt > 0) {

	    (void)makemon(&mons[dtype], u.ux, u.uy);
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
	mon = makemon(pm, u.ux, u.uy);
	pm->pxlth = 0;
	if (mon) {
	    mon->isminion = TRUE;
	    EMIN(mon)->min_align = alignment;
	}
    } else if (mnum == PM_ANGEL) {
	mon = makemon(&mons[mnum], u.ux, u.uy);
	if (mon) {
	    mon->isminion = TRUE;
	    EPRI(mon)->shralign = alignment;	/* always A_LAWFUL here */
	}
    } else
	mon = makemon(&mons[mnum], u.ux, u.uy);
    if (mon) {
	if (talk) {
/*JP	    pline("The voice of %s booms:", align_gname(alignment));*/
	    pline("%s������������:", align_gname(alignment));
/*JP	    verbalize("Thou shalt pay for thy indiscretion!");*/
	    verbalize("��̵ʬ�̤ʹ�ư��ȳ������衪");
	    if (!Blind)
/*JP		pline("%s appears before you.", Amonnam(mon));*/
		pline("%s�����ʤ������˸���줿��", Amonnam(mon));
	}
	mon->mpeaceful = FALSE;
	/* don't call set_malign(); player was naughty */
    }
}

#define	Athome	(Inhell && !mtmp->cham)

int
demon_talk(mtmp)		/* returns 1 if it won't attack. */
register struct monst *mtmp;
{
	long	demand, offer;

	if (uwep && uwep->oartifact == ART_EXCALIBUR) {
/*JP	    pline("%s looks very angry.", Amonnam(mtmp));*/
	    pline("%s�ϤȤƤ��ܤäƤ���褦�˸����롥", Amonnam(mtmp));
	    mtmp->mpeaceful = mtmp->mtame = 0;
	    newsym(mtmp->mx, mtmp->my);
	    return 0;
	}

	/* Slight advantage given. */
	if (is_dprince(mtmp->data) && mtmp->minvis) {
	    mtmp->minvis = 0;
/*JP	    if (!Blind) pline("%s appears before you.", Amonnam(mtmp));*/
	    if (!Blind) pline("%s���ܤ����˸���줿��", Amonnam(mtmp));
	    newsym(mtmp->mx,mtmp->my);
	}
	if (u.usym == S_DEMON) {	/* Won't blackmail their own. */

/*JP	    pline("%s says, \"Good hunting, %s.\" and vanishes.",
		  Amonnam(mtmp), flags.female ? "Sister" : "Brother");*/
	    pline("%s�ϸ��ä��֤褦%s�衪�ס������ƾä�����",
		  Amonnam(mtmp), flags.female ? "��" : "��");
	    rloc(mtmp);
	    return(1);
	}
	demand = (u.ugold * (rnd(80) + 20 * Athome)) /
		 100 * (1 + (sgn(u.ualign.type) == sgn(mtmp->data->maligntyp)));
	if (!demand)  		/* you have no gold */
	    return mtmp->mpeaceful = 0;
	else {
/*JP	    pline("%s demands %ld zorkmid%s for safe passage.",
		  Amonnam(mtmp), demand, plur(demand));*/
	    pline("%s���̹����Ȥ���%ld��������׵ᤷ����",
		  Amonnam(mtmp), demand);

	    if ((offer = bribe(mtmp)) >= demand) {
/*JP		pline("%s vanishes, laughing about cowardly mortals.",*/
		pline("�ह�٤��ܤ�����Τ�Ф��ʤ��顤%s�Ͼä�����",
		      Amonnam(mtmp));
	    } else {
		if ((long)rnd(40) > (demand - offer)) {
/*JP		    pline("%s scowls at you menacingly, then vanishes.",*/
		    pline("%s�Ϥ��ʤ���ҳŤ����ä�����",
			  Amonnam(mtmp));
		} else {
/*JP		    pline("%s gets angry...", Amonnam(mtmp));*/
		    pline("%s���ܤä�������", Amonnam(mtmp));
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
	char buf[80];
	long offer;

/*JP	getlin("How much will you offer?", buf);*/
	getlin("���������Ϳ���롩", buf);
	(void) sscanf(buf, "%ld", &offer);

	/*Michael Paddon -- fix for negative offer to monster*/
	/*JAR880815 - */
 	if (offer < 0L) {
/*JP 		You("try to shortchange %s, but fumble.",*/
 		You("%s����ޤ����Ȥ����������Ԥ�����",
 			mon_nam(mtmp));
 		offer = 0L;
 	} else if (offer == 0L) {
/*JP		You("refuse.");*/
		You("������");
 	} else if (offer >= u.ugold) {
/*JP		You("give %s all your gold.", mon_nam(mtmp));*/
		You("%s�ˤ��������Ϳ������", mon_nam(mtmp));
		offer = u.ugold;
/*JP	} else You("give %s %ld zorkmid%s.", mon_nam(mtmp), offer,
		   plur(offer));*/
	} else You("%s��%ld�������Ϳ������", mon_nam(mtmp), offer);

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
	    if (!(mons[pm].geno & (G_GENOD | G_EXTINCT)) &&
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
	    if (!(mons[pm].geno & (G_GENOD | G_EXTINCT)) &&
		    (atyp == A_NONE || sgn(mons[pm].maligntyp) == sgn(atyp)))
		return(pm);
	}
	return(ndemon(atyp));	/* approximate */
}

/* create lawful (good) lord */
int
llord()
{
	if (!(mons[PM_ARCHON].geno & (G_GENOD | G_EXTINCT)))
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
