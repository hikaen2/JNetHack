/*	SCCS Id: @(#)were.c	3.2	96/08/09	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

#ifdef OVL0

void
were_change(mon)
register struct monst *mon;
{
	if (!is_were(mon->data))
	    return;

	if (is_human(mon->data)) {
	    if (!Protection_from_shape_changers &&
		    !rn2(night() ? (flags.moonphase == FULL_MOON ?  3 : 30)
				 : (flags.moonphase == FULL_MOON ? 10 : 50))) {
		new_were(mon);		/* change into animal form */
		if (flags.soundok) {
		    const char *howler;

		    switch (monsndx(mon->data)) {
/*JP		    case PM_HUMAN_WEREWOLF:	  howler = "wolf";    break;
		    case PM_HUMAN_WEREJACKAL: howler = "jackal";  break;*/
		    case PM_HUMAN_WEREWOLF:	  howler = "狼";    break;
		    case PM_HUMAN_WEREJACKAL: howler = "ジャッカル";  break;
		    default:		  howler = (char *)0; break;
		    }
		    if (howler)
/*JP			You_hear("a %s howling at the moon.", howler);*/
			You_hear("月夜に%sが吠える声を聞いた．", howler);
		}
	    }
	} else if (!rn2(30) || Protection_from_shape_changers) {
	    new_were(mon);		/* change back into human form */
	}
}

#endif /* OVL0 */
#ifdef OVLB

static int FDECL(counter_were,(int));

static int
counter_were(pm)
int pm;
{
	switch(pm) {
	    case PM_WEREWOLF:	      return(PM_HUMAN_WEREWOLF);
	    case PM_HUMAN_WEREWOLF:   return(PM_WEREWOLF);
	    case PM_WEREJACKAL:	      return(PM_HUMAN_WEREJACKAL);
	    case PM_HUMAN_WEREJACKAL: return(PM_WEREJACKAL);
	    case PM_WERERAT:	      return(PM_HUMAN_WERERAT);
	    case PM_HUMAN_WERERAT:    return(PM_WERERAT);
	    default:		      return(0);
	}
}

void
new_were(mon)
register struct monst *mon;
{
	register int pm;

	pm = counter_were(monsndx(mon->data));
	if(!pm) {
	    impossible("unknown lycanthrope %s.", mon->data->mname);
	    return;
	}

	if(canseemon(mon))
/*JP	    pline("%s changes into a %s.", Monnam(mon),
			Hallucination ? rndmonnam() :
			is_human(&mons[pm]) ? "human" :
			mons[pm].mname+4);*/
	    pline("%sは%sの姿になった．", Monnam(mon),
			Hallucination ? rndmonnam() :
			is_human(&mons[pm]) ? "人間" :
			jtrns_mon(mons[pm].mname+4, mon->female));

	set_mon_data(mon, &mons[pm], 0);
	if (mon->msleep || !mon->mcanmove) {
	    /* transformation wakens and/or revitalizes */
	    mon->msleep = 0;
	    mon->mfrozen = 0;	/* not asleep or paralyzed */
	    mon->mcanmove = 1;
	}
	/* regenerate by 1/4 of the lost hit points */
	mon->mhp += (mon->mhpmax - mon->mhp) / 4;
	newsym(mon->mx,mon->my);
	mon_break_armor(mon);
	possibly_unwield(mon);
}

boolean
were_summon(ptr,yours)	/* were-creature (even you) summons a horde */
register struct permonst *ptr;
register boolean yours;
{
	register int i, typ, pm = monsndx(ptr);
	register struct monst *mtmp;
	boolean success = FALSE;

	if(Protection_from_shape_changers && !yours)
		return FALSE;
	for(i = rnd(5); i > 0; i--) {
	   switch(pm) {

		case PM_WERERAT:
		case PM_HUMAN_WERERAT:
			typ = rn2(3) ? PM_SEWER_RAT : rn2(3) ? PM_GIANT_RAT : PM_RABID_RAT ;
			break;
		case PM_WEREJACKAL:
		case PM_HUMAN_WEREJACKAL:
			typ = PM_JACKAL;
			break;
		case PM_WEREWOLF:
		case PM_HUMAN_WEREWOLF:
			typ = rn2(5) ? PM_WOLF : PM_WINTER_WOLF ;
			break;
		default:
			continue;
	    }
	    mtmp = makemon(&mons[typ], u.ux, u.uy, NO_MM_FLAGS);
	    if (mtmp) success = TRUE;
	    if (yours && mtmp)
		(void) tamedog(mtmp, (struct obj *) 0);
	}
	return success;
}

void
you_were()
{
	char qbuf[QBUFSZ];

	if(u.umonnum == u.ulycn) return;
	if(Polymorph_control) {
	    /* `+4' => skip "were" prefix to get name of beast */
/*JP	    Sprintf(qbuf,"Do you want to change into a %s? ",*/
	    Sprintf(qbuf,"%sに変化しますか？",
		jtrns_mon(mons[u.ulycn].mname+4, -1));
	    if(yn(qbuf) == 'n') return;
	}
	(void) polymon(u.ulycn);
}

void
you_unwere(purify)
boolean purify;
{
	if (purify) {
/*JP
	    You_feel("purified.");
*/
	    You("浄められたような気がした．");
	    u.ulycn = NON_PM;	/* cure lycanthropy */
	}
	if (is_were(uasmon) &&
/*JP
		(!Polymorph_control || yn("Remain in beast form?") == 'n'))
*/
		(!Polymorph_control || yn("元の姿に戻る？") == 'y'))
	    rehumanize();
}

#endif /* OVLB */

/*were.c*/
