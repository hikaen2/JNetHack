/*	SCCS Id: @(#)wizard.c	3.3	99/03/29	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* wizard code - inspired by rogue code from Merlyn Leroy (digi-g!brian) */
/*	       - heavily modified to give the wiz balls.  (genat!mike)   */
/*	       - dewimped and given some maledictions. -3. */
/*	       - generalized for 3.1 (mike@bullns.on01.bull.ca) */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "qtext.h"

#ifdef OVLB

STATIC_DCL short FDECL(which_arti, (int));
STATIC_DCL boolean FDECL(mon_has_arti, (struct monst *,SHORT_P));
STATIC_DCL struct monst *FDECL(other_mon_has_arti, (struct monst *,SHORT_P));
STATIC_DCL struct obj *FDECL(on_ground, (SHORT_P));
STATIC_DCL boolean FDECL(you_have, (int));
STATIC_DCL long FDECL(target_on, (int,struct monst *));
STATIC_DCL long FDECL(strategy, (struct monst *));

static NEARDATA const int nasties[] = {
	PM_COCKATRICE, PM_ETTIN, PM_STALKER, PM_MINOTAUR, PM_RED_DRAGON,
	PM_BLACK_DRAGON, PM_GREEN_DRAGON, PM_OWLBEAR, PM_PURPLE_WORM,
	PM_ROCK_TROLL, PM_XAN, PM_GREMLIN, PM_UMBER_HULK, PM_VAMPIRE_LORD,
	PM_XORN, PM_ZRUTY, PM_ELF_LORD, PM_ELVENKING, PM_YELLOW_DRAGON,
	PM_LEOCROTTA, PM_BALUCHITHERIUM, PM_CARNIVOROUS_APE, PM_FIRE_GIANT,
	PM_COUATL, PM_CAPTAIN, PM_WINGED_GARGOYLE, PM_MASTER_MIND_FLAYER,
	PM_FIRE_ELEMENTAL, PM_JABBERWOCK, PM_ARCH_LICH, PM_OGRE_KING,
	PM_OLOG_HAI, PM_IRON_GOLEM, PM_OCHRE_JELLY, PM_GREEN_SLIME,
	PM_DISENCHANTER
	};

static NEARDATA const unsigned wizapp[] = {
	PM_HUMAN, PM_WATER_DEMON, PM_VAMPIRE,
	PM_RED_DRAGON, PM_TROLL, PM_UMBER_HULK,
	PM_XORN, PM_XAN, PM_COCKATRICE,
	PM_FLOATING_EYE,
	PM_GUARDIAN_NAGA,
	PM_TRAPPER
};

#endif /* OVLB */
#ifdef OVL0

/* If you've found the Amulet, make the Wizard appear after some time */
/* Also, give hints about portal locations, if amulet is worn/wielded -dlc */
void
amulet()
{
	struct monst *mtmp;
	struct trap *ttmp;
	struct obj *amu;

#if 0		/* caller takes care of this check */
	if (!u.uhave.amulet)
		return;
#endif
	if ((((amu = uamul) != 0 && amu->otyp == AMULET_OF_YENDOR) ||
	     ((amu = uwep) != 0 && amu->otyp == AMULET_OF_YENDOR))
	    && !rn2(15)) {
	    for(ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
		if(ttmp->ttyp == MAGIC_PORTAL) {
		    int du = distu(ttmp->tx, ttmp->ty);
		    if (du <= 9)
/*JP			pline("%s feels hot!", The(xname(amu)));*/
			pline("%s��Ǯ����������", The(xname(amu)));
		    else if (du <= 64)
/*JP			pline("%s feels very warm.", The(xname(amu)));*/
			pline("%s�ϤȤƤ��Ȥ�����������", The(xname(amu)));
		    else if (du <= 144)
/*JP			pline("%s feels warm", The(xname(amu)));*/
			pline("%s���Ȥ�����������", The(xname(amu)));
		    /* else, the amulet feels normal */
		    break;
		}
	    }
	}

	if (!flags.no_of_wizards)
		return;
	/* find Wizard, and wake him if necessary */
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (mtmp->iswiz && mtmp->msleeping && !rn2(40)) {
		mtmp->msleeping = 0;
		if (distu(mtmp->mx,mtmp->my) > 2)
		    You(
/*JP "get the creepy feeling that somebody noticed your taking the Amulet."*/
 "���ʤ������������äƤ��뤳�Ȥ�ï���ˤ狼�ä��ȴ����Ƥ�������������"
		    );
		return;
	    }
}

#endif /* OVL0 */
#ifdef OVLB

int
mon_has_amulet(mtmp)
register struct monst *mtmp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == AMULET_OF_YENDOR) return(1);
	return(0);
}

int
mon_has_special(mtmp)
register struct monst *mtmp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
		if(otmp->otyp == AMULET_OF_YENDOR ||
			is_quest_artifact(otmp) ||
			otmp->otyp == BELL_OF_OPENING ||
			otmp->otyp == CANDELABRUM_OF_INVOCATION ||
			otmp->otyp == SPE_BOOK_OF_THE_DEAD) return(1);
	return(0);
}

/*
 *	New for 3.1  Strategy / Tactics for the wiz, as well as other
 *	monsters that are "after" something (defined via mflag3).
 *
 *	The strategy section decides *what* the monster is going
 *	to attempt, the tactics section implements the decision.
 */
#define STRAT(w, x, y, typ) (w | ((long)(x)<<16) | ((long)(y)<<8) | (long)typ)

#define M_Wants(mask)	(mtmp->data->mflags3 & (mask))

STATIC_OVL short
which_arti(mask)
	register int mask;
{
	switch(mask) {
	    case M3_WANTSAMUL:	return(AMULET_OF_YENDOR);
	    case M3_WANTSBELL:	return(BELL_OF_OPENING);
	    case M3_WANTSCAND:	return(CANDELABRUM_OF_INVOCATION);
	    case M3_WANTSBOOK:	return(SPE_BOOK_OF_THE_DEAD);
	    default:		break;	/* 0 signifies quest artifact */
	}
	return(0);
}

/*
 *	If "otyp" is zero, it triggers a check for the quest_artifact,
 *	since bell, book, candle, and amulet are all objects, not really
 *	artifacts right now.	[MRS]
 */
STATIC_OVL boolean
mon_has_arti(mtmp, otyp)
	register struct monst *mtmp;
	register short	otyp;
{
	register struct obj *otmp;

	for(otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
	    if(otyp) {
		if(otmp->otyp == otyp)
			return(1);
	    }
	     else if(is_quest_artifact(otmp)) return(1);
	}
	return(0);

}

STATIC_OVL struct monst *
other_mon_has_arti(mtmp, otyp)
	register struct monst *mtmp;
	register short	otyp;
{
	register struct monst *mtmp2;

	for(mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon)
	    if(mtmp2 != mtmp)
		if(mon_has_arti(mtmp2, otyp)) return(mtmp2);

	return((struct monst *)0);
}

STATIC_OVL struct obj *
on_ground(otyp)
	register short	otyp;
{
	register struct obj *otmp;

	for (otmp = fobj; otmp; otmp = otmp->nobj)
	    if (otyp) {
		if (otmp->otyp == otyp)
		    return(otmp);
	    } else if (is_quest_artifact(otmp))
		return(otmp);
	return((struct obj *)0);
}

STATIC_OVL boolean
you_have(mask)
	register int mask;
{
	switch(mask) {
	    case M3_WANTSAMUL:	return(boolean)(u.uhave.amulet);
	    case M3_WANTSBELL:	return(boolean)(u.uhave.bell);
	    case M3_WANTSCAND:	return(boolean)(u.uhave.menorah);
	    case M3_WANTSBOOK:	return(boolean)(u.uhave.book);
	    case M3_WANTSARTI:	return(boolean)(u.uhave.questart);
	    default:		break;
	}
	return(0);
}

STATIC_OVL long
target_on(mask, mtmp)
	register int mask;
	register struct monst *mtmp;
{
	register short	otyp;
	register struct obj *otmp;
	register struct monst *mtmp2;

	if(!M_Wants(mask))	return(STRAT_NONE);

	otyp = which_arti(mask);
	if(!mon_has_arti(mtmp, otyp)) {
	    if(you_have(mask))
		return(STRAT(STRAT_PLAYER, u.ux, u.uy, mask));
	    else if((otmp = on_ground(otyp)))
		return(STRAT(STRAT_GROUND, otmp->ox, otmp->oy, mask));
	    else if((mtmp2 = other_mon_has_arti(mtmp, otyp)))
		return(STRAT(STRAT_MONSTR, mtmp2->mx, mtmp2->my, mask));
	}
	return(STRAT_NONE);
}

STATIC_OVL long
strategy(mtmp)
	register struct monst *mtmp;
{
	long strat, dstrat;

	if(!is_covetous(mtmp->data)) return(STRAT_NONE);

	switch((mtmp->mhp*3)/mtmp->mhpmax) {	/* 0-3 */

	   default:
	    case 0:	/* panic time - mtmp is almost snuffed */
			return(STRAT_HEAL);

	    case 1:	/* the wiz is less cautious */
			if(mtmp->data != &mons[PM_WIZARD_OF_YENDOR])
			    return(STRAT_HEAL);
			/* else fall through */

	    case 2:	dstrat = STRAT_HEAL;
			break;

	    case 3:	dstrat = STRAT_NONE;
			break;
	}

	if(flags.made_amulet)
	    if((strat = target_on(M3_WANTSAMUL, mtmp)) != STRAT_NONE)
		return(strat);

	if(u.uevent.invoked) {		/* priorities change once gate opened */

	    if((strat = target_on(M3_WANTSARTI, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSBOOK, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSBELL, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSCAND, mtmp)) != STRAT_NONE)
		return(strat);
	} else {

	    if((strat = target_on(M3_WANTSBOOK, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSBELL, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSCAND, mtmp)) != STRAT_NONE)
		return(strat);
	    if((strat = target_on(M3_WANTSARTI, mtmp)) != STRAT_NONE)
		return(strat);
	}
	return(dstrat);
}

int
tactics(mtmp)
	register struct monst *mtmp;
{
	long strat = strategy(mtmp);

	mtmp->mstrategy = (mtmp->mstrategy & STRAT_WAITMASK) | strat;

	switch (strat) {
	    case STRAT_HEAL:	/* hide and recover */
		/* if wounded, hole up on or near the stairs (to block them) */
		/* unless, of course, there are no stairs (e.g. endlevel) */
		if (In_W_tower(mtmp->mx, mtmp->my, &u.uz) ||
			(mtmp->iswiz && !xupstair && !mon_has_amulet(mtmp))) {
		    if (!rn2(3 + mtmp->mhp/10)) rloc(mtmp);
		} else if (xupstair &&
			 (mtmp->mx != xupstair || mtmp->my != yupstair)) {
		    (void) mnearto(mtmp, xupstair, yupstair, TRUE);
		}
		/* if you're not around, cast healing spells */
		if (distu(mtmp->mx,mtmp->my) > (BOLT_LIM * BOLT_LIM))
		    if(mtmp->mhp <= mtmp->mhpmax - 8) {
			mtmp->mhp += rnd(8);
			return(1);
		    }
		/* fall through :-) */

	    case STRAT_NONE:	/* harrass */
	        if(!rn2(5)) mnexto(mtmp);
		return(0);

	    default:		/* kill, maim, pillage! */
	    {
		long  where = (strat & STRAT_STRATMASK);
		xchar tx = STRAT_GOALX(strat),
		      ty = STRAT_GOALY(strat);
		int   targ = strat & STRAT_GOAL;
		struct obj *otmp;

		if(!targ) { /* simply wants you to close */
		    return(0);
		}
		if((u.ux == tx && u.uy == ty) || where == STRAT_PLAYER) {
		    /* player is standing on it (or has it) */
		    mnexto(mtmp);
		    return(0);
		}
		if(where == STRAT_GROUND) {
		  if(!MON_AT(tx, ty) || (mtmp->mx == tx && mtmp->my == ty)) {
		    /* teleport to it and pick it up */
		    rloc_to(mtmp, tx, ty);	/* clean old pos */

		    if ((otmp = on_ground(which_arti(targ))) != 0) {
			if (cansee(mtmp->mx, mtmp->my))
			    pline("%s��%s�򽦤ä���",
				  Monnam(mtmp),
				  (distu(mtmp->my, mtmp->my) <= 5) ?
				    doname(otmp) : distant_name(otmp, doname));
			obj_extract_self(otmp);
			mpickobj(mtmp, otmp);
			return(1);
		    } else return(0);
		  }
	        } else { /* a monster has it - 'port beside it. */
		    (void) mnearto(mtmp, tx, ty, TRUE);
		    return(0);
		}
	    }
	}
	/* NOTREACHED */
	return(0);
}

void
aggravate()
{
	register struct monst *mtmp;

	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		mtmp->msleeping = 0;
		if(!mtmp->mcanmove && !rn2(5)) {
			mtmp->mfrozen = 0;
			mtmp->mcanmove = 1;
		}
	}
}

void
clonewiz()
{
	register struct monst *mtmp2;

	if ((mtmp2 = makemon(&mons[PM_WIZARD_OF_YENDOR],
				u.ux, u.uy, NO_MM_FLAGS)) != 0) {
	    mtmp2->msleeping = mtmp2->mtame = mtmp2->mpeaceful = 0;
	    if (!u.uhave.amulet && rn2(2)) {  /* give clone a fake */
		add_to_minv(mtmp2, mksobj(FAKE_AMULET_OF_YENDOR, TRUE, FALSE));
	    }
	    mtmp2->m_ap_type = M_AP_MONSTER;
	    mtmp2->mappearance = wizapp[rn2(SIZE(wizapp))];
	    newsym(mtmp2->mx,mtmp2->my);
	}
}

/* also used by newcham() */
int
pick_nasty()
{
    /* To do?  Possibly should filter for appropriate forms when
       in the elemental planes or surrounded by water or lava. */
    return nasties[rn2(SIZE(nasties))];
}

/* create some nasty monsters, aligned or neutral with the caster */
/* a null caster defaults to a chaotic caster (e.g. the wizard) */
void
nasty(mcast)
	struct monst *mcast;
{
    register struct monst	*mtmp;
    register int	i, j, tmp;
    int castalign = (mcast ? mcast->data->maligntyp : -1);

    if(!rn2(10) && Inhell) msummon(&mons[PM_WIZARD_OF_YENDOR]);
    else {
	tmp = (u.ulevel > 3) ? u.ulevel/3 : 1; /* just in case -- rph */

	for(i = rnd(tmp); i > 0; --i)
	    for(j=0; j<20; j++) {
		if ((mtmp = makemon(&mons[pick_nasty()],
				    u.ux, u.uy, NO_MM_FLAGS)) != 0) {
		    mtmp->msleeping = mtmp->mpeaceful = mtmp->mtame = 0;
		    set_malign(mtmp);
		} else /* GENOD? */
		    mtmp = makemon((struct permonst *)0,
					u.ux, u.uy, NO_MM_FLAGS);
		if(mtmp && (mtmp->data->maligntyp == 0 ||
		            sgn(mtmp->data->maligntyp) == sgn(castalign)) )
		    break;
	    }
    }
    return;
}

/*	Let's resurrect the wizard, for some unexpected fun.	*/
void
resurrect()
{
	struct monst *mtmp, **mmtmp;
	long elapsed;
	const char *verb;

	if (!flags.no_of_wizards) {
	    /* make a new Wizard */
/*JP	    verb = "kill";*/
	    verb = "�򻦤���";
	    mtmp = makemon(&mons[PM_WIZARD_OF_YENDOR], u.ux, u.uy, MM_NOWAIT);
	} else {
	    /* look for a migrating Wizard */
/*JP	    verb = "elude";*/
	    verb = "����ƨ���";
	    mmtmp = &migrating_mons;
	    while ((mtmp = *mmtmp) != 0) {
		if (mtmp->iswiz &&
			/* if he has the Amulet, he won't bring it to you */
			!mon_has_amulet(mtmp) &&
			(elapsed = monstermoves - mtmp->mlstmv) > 0L) {
		    mon_catchup_elapsed_time(mtmp, elapsed);
		    if (elapsed >= LARGEST_INT) elapsed = LARGEST_INT - 1;
		    elapsed /= 50L;
		    if (mtmp->msleeping && rn2((int)elapsed + 1))
			mtmp->msleeping = 0;
		    if (mtmp->mfrozen == 1) /* would unfreeze on next move */
			mtmp->mfrozen = 0,  mtmp->mcanmove = 1;
		    if (mtmp->mcanmove && !mtmp->msleeping) {
			*mmtmp = mtmp->nmon;
			mon_arrive(mtmp, TRUE);
			/* note: there might be a second Wizard; if so,
			   he'll have to wait til the next resurrection */
			break;
		    }
		}
		mmtmp = &mtmp->nmon;
	    }
	}

	if (mtmp) {
		mtmp->msleeping = mtmp->mtame = mtmp->mpeaceful = 0;
		set_malign(mtmp);
/*JP		pline("A voice booms out...");*/
		pline("�����⤯�Ĥ������������");
/*JP		verbalize("So thou thought thou couldst %s me, fool.", verb);*/
		verbalize(";%s�Ȼפ����������Τ��Ĥ��Ԥᡥ", verb);
	}

}

/*	Here, we make trouble for the poor shmuck who actually	*/
/*	managed to do in the Wizard.				*/
void
intervene()
{
	int which = Is_astralevel(&u.uz) ? rnd(4) : rn2(6);
	/* cases 0 and 5 don't apply on the Astral level */
	switch (which) {
	    case 0:
/*JP	    case 1:	You_feel("vaguely nervous.");*/
	    case 1:	You("���Ȥʤ��԰¤ˤʤä���");
			break;
	    case 2:	if (!Blind)
/*JP			    You("notice a %s glow surrounding you.",*/
			    pline("%s�������ʤ���Ȥ�ޤ��Ƥ���Τ˵����Ĥ�����",
				  hcolor(Black));
			rndcurse();
			break;
	    case 3:	aggravate();
			break;
	    case 4:	nasty((struct monst *)0);
			break;
	    case 5:	resurrect();
			break;
	}
}

void
wizdead()
{
	flags.no_of_wizards--;
	if (!u.uevent.udemigod) {
		u.uevent.udemigod = TRUE;
		u.udg_cnt = rn1(250, 50);
	}
}

const char *random_insult[] = {
/*JP	"antic",*/
	"�դ�������Ϻ",
/*JP	"blackguard",*/
	"����",
/*JP	"caitiff",*/
	"�����ä���",
/*JP	"chucklehead",*/
	"�Τ��",
/*JP	"coistrel",*/
	"����ݤ󤿤�",
/*JP	"craven",*/
	"���¼�",
/*JP	"cretin",*/
	"����",
/*JP	"cur",*/
	"���Ǥʤ�",
/*JP	"dastard",*/
	"���Ĥ�",
/*JP	"demon fodder",*/
	"����α¿�",
/*JP	"dimwit",*/
	"�����Τ�",
/*JP	"dolt",*/
	"�ޤ̤�",
/*JP	"fool",*/
	"�ϼ�",
/*JP	"footpad",*/
	"�����Ϥ�",
/*JP	"imbecile",*/
	"�򤫼�",
/*JP	"knave",*/
	"�ʤ餺��",
/*JP	"maledict",*/
	"����",
/*JP	"miscreant",*/
	"�˰���",
/*JP	"niddering",*/
	"�ϼ�����",
/*JP	"poltroon",*/
	"�ܶ���",
/*JP	"rattlepate",*/
	"����Ƭ",
/*JP	"reprobate",*/
	"ƻ�ڼ�",
/*JP	"scapegrace",*/
	"����",
/*JP	"varlet",*/
	"��Ϻ",
/*JP	"villein",	*//* (sic.) */
	"����",	/* (sic.) */
/*JP	"wittol",*/
	"�դʤष",
/*JP	"worm",*/
	"����",
/*JP	"wretch",*/
	"�ͤǤʤ�",
};

const char *random_malediction[] = {
/*JP	"Hell shall soon claim thy remains,",*/
	"�Ϲ��Ϥ��Ť졤��������Ĥ뤳�Ȥ��׵᤹��Ǥ�����",
/*JP	"I chortle at thee, thou pathetic",*/
	"����ʤ�Ĥ�Τ���;����­����",
/*JP	"Prepare to die, thou",*/
	"�򡤻��������",
/*JP	"Resistance is useless,",*/
	"�񹳤��Ƥ�̵�̤��㡤",
/*JP	"Surrender or die, thou",*/
	"�߻����补����ʤ��лस�㡥",
/*JP	"There shall be no mercy, thou",*/
	"�����̵�����",
/*JP	"Thou shalt repent of thy cunning,",*/
	"�򡤤���������٤���",
/*JP	"Thou art as a flea to me,",*/
	"���;�ˤȤäƥΥߤΤ褦�ʤ�Τ��㡤",
/*JP	"Thou art doomed,",*/
	"��ϼ����Ƥ��롤",
/*JP	"Thy fate is sealed,",*/
	"��α�̿����������Ƥ��롤",
/*JP	"Verily, thou shalt be one dead"*/
	"�ޤ��Ȥ���ϻ�ˤ���Ԥʤ�"
};

/* Insult or intimidate the player */
void
cuss(mtmp)
register struct monst	*mtmp;
{
	if (mtmp->iswiz) {
	    if (!rn2(5))  /* typical bad guy action */
/*JP		pline("%s laughs fiendishly.", Monnam(mtmp));*/
		pline("%s�ϰ���Τ褦�˾Фä���", Monnam(mtmp));
	    else
		if (u.uhave.amulet && !rn2(SIZE(random_insult)))
/*JP		    verbalize("Relinquish the amulet, %s!",*/
		    verbalize("��褱���������%s��",
			  random_insult[rn2(SIZE(random_insult))]);
		else if (u.uhp < 5 && !rn2(2))	/* Panic */
		    verbalize(rn2(2) ?
/*JP			  "Even now thy life force ebbs, %s!" :*/
			  "���ȤʤäƤ�ʤ����̿�Ϥ����ƿꤨ��Τ���%s��" :
/*JP			  "Savor thy breath, %s, it be thine last!",*/
			  "©��̣��äƤ�����%s����κǴ��λ�����",
			  random_insult[rn2(SIZE(random_insult))]);
		else if (mtmp->mhp < 5 && !rn2(2))	/* Parthian shot */
		    verbalize(rn2(2) ?
/*JP			      "I shall return." :*/
			      ";��ɬ�����äƤ��롥" :
/*JP			      "I'll be back.");*/
			      ";����äƤ��롥");
		else
/*JP		    verbalize("%s %s!",*/
		    verbalize("%s%s��",
			  random_malediction[rn2(SIZE(random_malediction))],
			  random_insult[rn2(SIZE(random_insult))]);
	} else if(is_lminion(mtmp->data)) {
		com_pager(rn2(QTN_ANGELIC - 1 + (Hallucination ? 1 : 0)) +
			      QT_ANGELIC);
	} else {
	    if (!rn2(5))
/*JP		pline("%s casts aspersions on your ancestry.", Monnam(mtmp));*/
		pline("%s�Ϥ��ʤ��β��������������", Monnam(mtmp));
	    else
	        com_pager(rn2(QTN_DEMONIC) + QT_DEMONIC);
	}
}

#endif /* OVLB */

/*wizard.c*/
