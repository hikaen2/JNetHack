/*	SCCS Id: @(#)mhitu.c	3.2	96/05/01	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-1996
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "artifact.h"

STATIC_VAR NEARDATA struct obj *otmp;

STATIC_DCL void FDECL(urustm, (struct monst *, struct obj *));
# ifdef OVL1
static boolean FDECL(u_slip_free, (struct monst *,struct attack *));
static int FDECL(passiveum, (struct permonst *,struct monst *,struct attack *));
# endif /* OVL1 */

#ifdef OVLB
# ifdef SEDUCE
static void FDECL(mayberem, (struct obj *, const char *));
# endif
#endif /* OVLB */

STATIC_DCL boolean FDECL(diseasemu, (struct permonst *));
STATIC_DCL int FDECL(hitmu, (struct monst *,struct attack *));
STATIC_DCL int FDECL(gulpmu, (struct monst *,struct attack *));
STATIC_DCL int FDECL(explmu, (struct monst *,struct attack *,BOOLEAN_P));
STATIC_DCL void FDECL(missmu,(struct monst *,BOOLEAN_P,struct attack *));
STATIC_DCL void FDECL(mswings,(struct monst *,struct obj *));
STATIC_DCL void FDECL(wildmiss, (struct monst *,struct attack *));

STATIC_DCL void FDECL(hurtarmor,(struct permonst *,int));
STATIC_DCL void FDECL(hitmsg,(struct monst *,struct attack *));

/* See comment in mhitm.c.  If we use this a lot it probably should be */
/* changed to a parameter to mhitu. */
static int dieroll;

#ifdef OVL1


STATIC_OVL void
hitmsg(mtmp, mattk)
register struct monst *mtmp;
register struct attack *mattk;
{
	int compat;

	/* Note: if opposite gender, "seductively" */
	/* If same gender, "engagingly" for nymph, normal msg for others */
	if((compat = could_seduce(mtmp, &youmonst, mattk))
			&& !mtmp->mcan && !mtmp->mspec_used) {
/*JP		pline("%s %s you %s.", Monnam(mtmp),
			Blind ? "talks to" : "smiles at",
			compat == 2 ? "engagingly" : "seductively");*/
	    	pline("%s�Ϥ��ʤ�%s%s��", Monnam(mtmp),
			compat == 2 ? "������Ĥ���褦��" : "�˹��դ�褻��褦��",
			Blind ? "�ä�������" : "���Ф��");
	  

	} else
	    switch (mattk->aatyp) {
		case AT_BITE:
/*JP			pline("%s bites!", Monnam(mtmp));*/
	                pline("%s�ϳ��ߤĤ�����", Monnam(mtmp));
			break;
		case AT_KICK:
/*JP			pline("%s kicks%c", Monnam(mtmp),
					thick_skinned(uasmon) ? '.' : '!');*/
			pline("%s�Ͻ��ȤФ���%s",Monnam(mtmp), 
					thick_skinned(uasmon) ? "��" : "��");
			break;
		case AT_STNG:
/*JP			pline("%s stings!", Monnam(mtmp));*/
			pline("%s���ͤ���������", Monnam(mtmp));
			break;
		case AT_BUTT:
/*JP			pline("%s butts!", Monnam(mtmp));*/
			pline("%s��Ƭ�ͤ��򤯤�路����", Monnam(mtmp));
			break;
		case AT_TUCH:
/*JP			pline("%s touches you!", Monnam(mtmp));*/
			pline("%s�Ϥ��ʤ��˿��줿��", Monnam(mtmp));
			break;
		case AT_TENT:
/*JP			pline("%s tentacles suck you!",
				        s_suffix(Monnam(mtmp)));*/
			pline("%s�ο��꤬���ʤ����αդ�ۤ��Ȥä���", 
					Monnam(mtmp));
			break;
		case AT_EXPL:
/*JP			pline("%s explodes!", Monnam(mtmp));*/
			pline("%s����ȯ������", Monnam(mtmp));
			break;
		default:
/*JP			pline("%s hits!", Monnam(mtmp));*/
			pline("%s�ι����̿�椷����", Monnam(mtmp));
	    }
}

STATIC_OVL void
missmu(mtmp, nearmiss, mattk)		/* monster missed you */
register struct monst *mtmp;
register boolean nearmiss;
register struct attack *mattk;
{
	if(could_seduce(mtmp, &youmonst, mattk) && !mtmp->mcan)
/*JP	    pline("%s pretends to be friendly.", Monnam(mtmp));*/
	    pline("%s��ͧ��Ū�ʤդ�򤷤Ƥ��롥",Monnam(mtmp));
	else {
	    if (!flags.verbose || !nearmiss)
/*JP		pline("%s misses.", Monnam(mtmp));*/
		pline("%s�ι���ϤϤ��줿��", Monnam(mtmp));
	    else
/*JP		pline("%s just misses!", Monnam(mtmp));*/
		pline("%s�ι���϶����ڤä���", Monnam(mtmp));
	}
}

STATIC_OVL void
mswings(mtmp, otemp)		/* monster swings obj */
register struct monst *mtmp;
register struct obj *otemp;
{
	if (!flags.verbose || Blind || !mon_visible(mtmp))
		return;
#if 0 /*JP*/
	pline("%s %s %s %s.", Monnam(mtmp),
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "thrusts" : "swings",
	      his[pronoun_gender(mtmp)], xname(otemp));
#endif
	pline("%s��%s%s��", Monnam(mtmp),
	      xname(otemp),
	      (objects[otemp->otyp].oc_dir & PIERCE) ? "���ͤ���" : "�򿶤�ޤ路��");
}

/* return how a poison attack was delivered */
const char *
mpoisons_subj(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
	if (mattk->aatyp == AT_WEAP) {
	    struct obj *mwep = (mtmp == &youmonst) ? uwep : MON_WEP(mtmp);
	    /* "Foo's attack was poisoned." is pretty lame, but at least
	       it's better than "sting" when not a stinging attack... */
/*JP	    return (!mwep || !mwep->opoisoned) ? "attack" : "weapon";*/
	    return (!mwep || !mwep->opoisoned) ? "����" : "���";
	} else {
/*JP	    return (mattk->aatyp == AT_TUCH) ? "contact" :
		   (mattk->aatyp == AT_BITE) ? "bite" : "sting";*/
	    return (mattk->aatyp == AT_TUCH) ? "�ܿ�" :
		   (mattk->aatyp == AT_BITE) ? "���ߤĤ�" : "�ͤ�����";
	}
}

/* called when your intrinsic speed is taken away */
void
u_slow_down()
{
	Fast &= ~(INTRINSIC|TIMEOUT);
	if (!Fast)
/*JP	    You("slow down.");*/
	    You("ư�����٤��ʤä���");
	else	/* speed boots */
/*JP	    Your("quickness feels less natural.");*/
	    You("®���ˤĤ��Ƥ����ʤ��ʤä���");
	exercise(A_DEX, FALSE);
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL void
wildmiss(mtmp, mattk)		/* monster attacked your displaced image */
	register struct monst *mtmp;
	register struct attack *mattk;
{
	int compat;

	if (!flags.verbose) return;
	if (!cansee(mtmp->mx, mtmp->my)) return;
		/* maybe it's attacking an image around the corner? */

	compat = (mattk->adtyp == AD_SEDU || mattk->adtyp == AD_SSEX) &&
		 could_seduce(mtmp, &youmonst, (struct attack *)0);

	if (!mtmp->mcansee || (Invis && !perceives(mtmp->data))) {
	    const char *swings =
/*JP		mattk->aatyp == AT_BITE ? "snaps" :
		mattk->aatyp == AT_KICK ? "kicks" :
		(mattk->aatyp == AT_STNG ||
		 mattk->aatyp == AT_BUTT ||
		 nolimbs(mtmp->data)) ? "lunges" : "swings";*/
		mattk->aatyp == AT_BITE ? "���ߤĤ�" :
		mattk->aatyp == AT_KICK ? "���ȤФ�" :
		(mattk->aatyp == AT_STNG ||
		 mattk->aatyp == AT_BUTT ||
		 nolimbs(mtmp->data)) ? "�Ϳʤ���" : "�����";

	    if (compat)
/*JP		pline("%s tries to touch you and misses!", Monnam(mtmp));*/
		pline("%s�Ϥ��ʤ��˿����Ȥ��������Ԥ�����", Monnam(mtmp));
	    else
		switch(rn2(3)) {
#if 0 /*JP*/
		case 0: pline("%s %s wildly and misses!", Monnam(mtmp),
			      swings);
		    break;
		case 1: pline("%s attacks a spot beside you.", Monnam(mtmp));
		    break;
		case 2: pline("%s strikes at %s!", Monnam(mtmp),
				Underwater ? "empty water" : "thin air");
		    break;
		default:pline("%s %s wildly!", Monnam(mtmp), swings);
		    break;
#endif /*JP*/
		case 0: pline("%s�Ϸ㤷��%s�����Ϥ�������", Monnam(mtmp),
			      jconj(swings, "��"));
		    break;
		case 1: pline("%s�ι���Ϥ��ʤ�����ʢ�򤫤��᤿��", Monnam(mtmp));
		    break;
		case 2: pline("%s��%s���Ǥ��Ĥ�����", Monnam(mtmp),
				Underwater ? "��" : "����ʤ��Ȥ���");
		    break;
		default:pline("%s�Ϸ㤷��%s��", Monnam(mtmp),
			      jconj(swings, "��"));
  		    break;

		}
	} else if (Displaced) {
	    if (compat)
/*JP		pline("%s smiles %s at your %sdisplaced image...",
			Monnam(mtmp),
			compat == 2 ? "engagingly" : "seductively",
			Invis ? "invisible " : "");*/
		pline("%s��%s���ʤ��θ��Ƥ��Ф���%s���Ф��������",
			Monnam(mtmp),
			Invis ? "Ʃ����" : "",
			compat == 2 ? "̥��Ū��" : "Ͷ��Ū��");
	    else
/*JP		pline("%s strikes at your %sdisplaced image and misses you!",*/
		pline("%s�Ϥ��ʤ���%s���Ƥ��Ǥ����Ϥ�������",
			/* Note: if you're both invisible and displaced,
			 * only monsters which see invisible will attack your
			 * displaced image, since the displaced image is also
			 * invisible.
			 */
			Monnam(mtmp),
/*JP			Invis ? "invisible " : "");*/
			Invis ? "Ʃ����" : "");

	} else if (Underwater) {
	    /* monsters may miss especially on water level where
	       bubbles shake the player here and there */
	    if (compat)
/*		pline("%s reaches towards your distorted image.",Monnam(mtmp));*/
		pline("%s�Ϥ��ʤ����Ĥ�����Ƥ����ظ����ä���",Monnam(mtmp));
	    else
/*JP		pline("%s is fooled by water reflections and misses!",Monnam(mtmp));*/
		pline("%s�Ͽ��ȿ�ͤˤ��ޤ��졤�Ϥ�������",Monnam(mtmp));

	} else impossible("%s attacks you without knowing your location?",
		Monnam(mtmp));
}

void
expels(mtmp, mdat, message)
register struct monst *mtmp;
register struct permonst *mdat; /* if mtmp is polymorphed, mdat != mtmp->data */
boolean message;
{
	if (message) {
		if (is_animal(mdat))
/*JP			You("get regurgitated!");*/
			You("�Ǥ������줿��");
		else {
			char blast[40];
			register int i;

			blast[0] = '\0';
			for(i = 0; i < NATTK; i++)
				if(mdat->mattk[i].aatyp == AT_ENGL)
					break;
			if (mdat->mattk[i].aatyp != AT_ENGL)
			      impossible("Swallower has no engulfing attack?");
			else {
				if (is_whirly(mdat)) {
					switch (mdat->mattk[i].adtyp) {
						case AD_ELEC:
							Strcpy(blast,
/*JP						      " in a shower of sparks");*/
						      "�βв֤α����椫��");
							break;
						case AD_COLD:
							Strcpy(blast,
/*JP							" in a blast of frost");*/
							"���䵤�������椫��");
							break;
/*By Hiramoto Kouji*/				default:
							Strcpy(blast,
							"����");
							break;
					}
				} else
/*JP					Strcpy(blast, " with a squelch");*/
					Strcpy(blast, "�����Ǥ��Ф����褦��");
/*JP				You("get expelled from %s%s!",*/
				You("%s%sæ�Ф�����", 
				    mon_nam(mtmp), blast);
			}
		}
	}
	unstuck(mtmp);	/* ball&chain returned in unstuck() */
	mnexto(mtmp);
	newsym(u.ux,u.uy);
	spoteffects();
	/* to cover for a case where mtmp is not in a next square */
	if(um_dist(mtmp->mx,mtmp->my,1))
/*JP		pline("Brrooaa...  You land hard at some distance.");*/
		pline("�֥������󤯤���Φ����Τ��񤷤���");
}

#endif /* OVLB */
#ifdef OVL0

/*
 * mattacku: monster attacks you
 *	returns 1 if monster dies (e.g. "yellow light"), 0 otherwise
 *	Note: if you're displaced or invisible the monster might attack the
 *		wrong position...
 *	Assumption: it's attacking you or an empty square; if there's another
 *		monster which it attacks by mistake, the caller had better
 *		take care of it...
 */
int
mattacku(mtmp)
	register struct monst *mtmp;
{
	struct	attack	*mattk;
	int	i, j, tmp, sum[NATTK];
	struct	permonst *mdat = mtmp->data;
	boolean ranged = (distu(mtmp->mx, mtmp->my) > 3);
		/* Is it near you?  Affects your actions */
	boolean range2 = !monnear(mtmp, mtmp->mux, mtmp->muy);
		/* Does it think it's near you?  Affects its actions */
	boolean foundyou = (mtmp->mux==u.ux && mtmp->muy==u.uy);
		/* Is it attacking you or your image? */
	boolean youseeit = canseemon(mtmp);
		/* Might be attacking your image around the corner, or
		 * invisible, or you might be blind....
		 */

	if(!ranged) nomul(0);
	if(mtmp->mhp <= 0 || (Underwater && !is_swimmer(mtmp->data)))
	    return(0);

	/* If swallowed, can only be affected by u.ustuck */
	if(u.uswallow) {
	    if(mtmp != u.ustuck)
		return(0);
	    u.ustuck->mux = u.ux;
	    u.ustuck->muy = u.uy;
	    range2 = 0;
	    foundyou = 1;
	    if(u.uinvulnerable) return (0); /* stomachs can't hurt you! */
	}

	if (u.uundetected && !range2 && foundyou && !u.uswallow) {
		u.uundetected = 0;
		if (is_hider(uasmon)) {
		    coord cc; /* maybe we need a unexto() function? */

/*JP		    You("fall from the %s!", ceiling(u.ux,u.uy));*/
		    You("%s�����������", ceiling(u.ux,u.uy));
		    if (enexto(&cc, u.ux, u.uy, &playermon)) {
			remove_monster(mtmp->mx, mtmp->my);
			newsym(mtmp->mx,mtmp->my);
			place_monster(mtmp, u.ux, u.uy);
			if(mtmp->wormno) worm_move(mtmp);
			teleds(cc.x, cc.y);
			set_apparxy(mtmp);
			newsym(u.ux,u.uy);
		    } else {
/*JP			pline("%s is killed by a falling %s (you)!",*/
			pline("%s������Ƥ���%s(���ʤ�)�ˤ�äƻ�����",
						Monnam(mtmp), uasmon->mname);
			killed(mtmp);
			newsym(u.ux,u.uy);
			if (mtmp->mhp > 0) return 0;
			else return 1;
		    }
		    if (u.usym != S_PIERCER)
			return(0);	/* trappers don't attack */

		    if (which_armor(mtmp, WORN_HELMET)) {
/*JP			Your("blow glances off %s helmet.",*/
			Your("�����%s�γ��򤫤��᤿��",
			               s_suffix(mon_nam(mtmp)));
		    } else {
			if (3 + find_mac(mtmp) <= rnd(20)) {
/*JP			    pline("%s is hit by a falling piercer (you)!",*/
			    pline("%s��������(���ʤ�)�ǽ��Ĥ�����",
								Monnam(mtmp));
			    if ((mtmp->mhp -= d(3,6)) < 1)
				killed(mtmp);
			} else
/*JP			  pline("%s is almost hit by a falling piercer (you)!",*/
			  pline("%s�Ϥ⤦������������(���ʤ�)�ǽ��Ĥ��Ȥ�����ä���",
								Monnam(mtmp));
		    }
		} else {
		    if (!youseeit)
/*JP			pline("It tries to move where you are hiding.");*/
			pline("���Ԥ������ʤ�������Ƥ���Ȥ�����ư���褦�Ȥ�����");
		    else {
			/* Ugly kludge for eggs.  The message is phrased so as
			 * to be directed at the monster, not the player,
			 * which makes "laid by you" wrong.  For the
			 * parallelism to work, we can't rephrase it, so we
			 * zap the "laid by you" momentarily instead.
			 */
			struct obj *obj = level.objects[u.ux][u.uy];

			if (obj ||
			      (uasmon->mlet == S_EEL && is_pool(u.ux, u.uy))) {
			    int save_spe = 0; /* suppress warning */
			    if (obj) {
				save_spe = obj->spe;
				if (obj->otyp == EGG) obj->spe = 0;
			    }
			    if (uasmon->mlet == S_EEL)
/*JP		pline("Wait, %s!  There's a hidden %s named %s there!",*/
/*JP				m_monnam(mtmp), uasmon->mname, plname);*/
	     pline("�Ԥơ�%s��%s�Ȥ���̾��%s������Ƥ��롪",
				m_monnam(mtmp), plname, jtrns_mon(uasmon->mname, flags.female));
			    else
#if 0 /*JP*/
	     pline("Wait, %s!  There's a %s named %s hiding under %s!",
				m_monnam(mtmp), uasmon->mname, plname,
				doname(level.objects[u.ux][u.uy]));
#endif /*JP*/
	     pline("�Ԥơ�%s��%s�Ȥ���̾��%s��%s�β��˱���Ƥ��롪",
				m_monnam(mtmp), plname, jtrns_mon(uasmon->mname, flags.female), 
				doname(level.objects[u.ux][u.uy]));

			    if (obj) obj->spe = save_spe;
			} else
			    impossible("hiding under nothing?");
		    }
		    newsym(u.ux,u.uy);
		}
		return(0);
	}
	if (u.usym == S_MIMIC_DEF && !range2 && foundyou && !u.uswallow) {
#if 0 /*JP*/
		if (!youseeit) pline("It gets stuck on you.");
		else pline("Wait, %s!  That's a %s named %s!",
			   m_monnam(mtmp), uasmon->mname, plname);
#endif /*JP*/
		if (!youseeit) pline("���Ԥ������ʤ��ξ�ˤΤ������ä�");
		    else pline("�Ԥơ�%s�������%s�Ȥ���̾��%s����",
			       m_monnam(mtmp), 
			       plname, jtrns_mon(uasmon->mname, flags.female));
		u.ustuck = mtmp;
		u.usym = S_MIMIC;
		newsym(u.ux,u.uy);
		return(0);
	}

	/* player might be mimicking gold */
	if (u.usym == 0 && !range2 && foundyou && !u.uswallow) {
	    if (!youseeit)
/*JP		 pline("%s %s!", Something, likes_gold(mtmp->data) ?
			"tries to pick you up" : "disturbs you");*/
		 pline("%s��%s��", Something, likes_gold(mtmp->data) ?
			"���ʤ��򽦤����Ȥ���" : "̵�뤷��");
/*JP	    else pline("Wait, %s!  That gold is really %s named %s!",
			m_monnam(mtmp),
			u.mtimedone ? an(uasmon->mname) :
			    an(player_mon()->mname),
			plname);*/
	    else pline("�Ԥơ�%s!���ζ����%s�Ȥ���̾��%s����",
			m_monnam(mtmp),
			plname,
			u.mtimedone ? an(uasmon->mname) :
			    an(player_mon()->mname));
	    if (multi < 0) {	/* this should always be the case */
		char buf[BUFSIZ];
/*JP		Sprintf(buf, "You appear to be %s again.",
			u.mtimedone ? (const char *) an(uasmon->mname) :
			    (const char *) "yourself");*/
		Sprintf(buf, "���ʤ���%s�Ȥʤä�",
			u.mtimedone ? (const char *) an(uasmon->mname) :
			    (const char *) "��ʬ����");
		unmul(buf);	/* immediately stop mimicking gold */
	    }
	    return 0;
	}

/*	Work out the armor class differential	*/
	tmp = AC_VALUE(u.uac) + 10;		/* tmp ~= 0 - 20 */
	tmp += mtmp->m_lev;
	if(multi < 0) tmp += 4;
	if((Invis && !perceives(mdat)) || !mtmp->mcansee)
		tmp -= 2;
	if(mtmp->mtrapped) tmp -= 2;
	if(tmp <= 0) tmp = 1;

	/* make eels visible the moment they hit/miss us */
	if(mdat->mlet == S_EEL && mtmp->minvis && cansee(mtmp->mx,mtmp->my)) {
		mtmp->minvis = 0;
		newsym(mtmp->mx,mtmp->my);
	}

/*	Special demon handling code */
	if(!mtmp->cham && is_demon(mdat) && !range2
	   && mtmp->data != &mons[PM_BALROG]
	   && mtmp->data != &mons[PM_SUCCUBUS]
	   && mtmp->data != &mons[PM_INCUBUS])
	    if(!mtmp->mcan && !rn2(13))	msummon(mdat);

/*	Special lycanthrope handling code */
	if(!mtmp->cham && is_were(mdat) && !range2) {

	    if(is_human(mdat)) {
		if(!rn2(5 - (night() * 2)) && !mtmp->mcan) new_were(mtmp);
	    } else if(!rn2(30) && !mtmp->mcan) new_were(mtmp);
	    mdat = mtmp->data;

	    if(!rn2(10) && !mtmp->mcan) {
		if(youseeit) {
/*JP			pline("%s summons help!", Monnam(mtmp));*/
			pline("%s�Ͻ�����Ƥ����", Monnam(mtmp));
		} else
/*JP			You_feel("hemmed in.");*/
			pline("��ʪ���˰Ϥޤ줿�褦�ʵ������롥");
		/* Technically wrong; we really should check if you can see the
		 * help, but close enough...
		 */
		if (!were_summon(mdat,FALSE) && youseeit)
/*JP		    pline("But none comes.");*/
		    pline("������������ʤ��ä���");
	    }
	}

	if(u.uinvulnerable) {
	    /* monster's won't attack you */
	    if(mtmp == u.ustuck)
/*JP		pline("%s loosens its grip slightly.", Monnam(mtmp));*/
		pline("%s�ϰ��ꤷ�᤿���μ��鷺���˴ˤ᤿��", Monnam(mtmp));
	    else if(!range2) {
		if(youseeit)
/*JP		    pline("%s starts to attack you, but pulls back.",*/
		    pline("%s�Ϥ��ʤ��򹶷⤷�����������Ҥä��᤿��",
			  Monnam(mtmp));
		else
/*JP		    You_feel("%s move nearby.", something);*/
		    pline("���Ԥ������ʤ��Τ��Ф��̤�̤����褦�ʵ���������");
	    }
	    return (0);
	}

	/* Unlike defensive stuff, don't let them use item _and_ attack. */
	if(find_offensive(mtmp)) {
		int foo = use_offensive(mtmp);

		if (foo != 0) return(foo==1);
	}

	for(i = 0; i < NATTK; i++) {

	    sum[i] = 0;
	    mattk = &(mdat->mattk[i]);
	    if (u.uswallow && (mattk->aatyp != AT_ENGL))
		continue;
	    switch(mattk->aatyp) {
		case AT_CLAW:	/* "hand to hand" attacks */
		case AT_KICK:
		case AT_BITE:
		case AT_STNG:
		case AT_TUCH:
		case AT_BUTT:
		case AT_TENT:
			if(!range2) {
			    if (foundyou) {
				if(tmp > (j = rnd(20+i))) {
				    if (mattk->aatyp != AT_KICK ||
					    !thick_skinned(uasmon))
					sum[i] = hitmu(mtmp, mattk);
				} else
				    missmu(mtmp, (tmp == j), mattk);
			    } else
				wildmiss(mtmp, mattk);
			}
			break;

		case AT_HUGS:	/* automatic if prev two attacks succeed */
			/* Note: if displaced, prev attacks never succeeded */
			if((!range2 && i>=2 && sum[i-1] && sum[i-2])
							|| mtmp == u.ustuck)
				sum[i]= hitmu(mtmp, mattk);
			break;

		case AT_GAZE:	/* can affect you either ranged or not */
			/* Medusa gaze already operated through m_respond in
			 * dochug(); don't gaze more than once per round.
			 */
			if (mdat != &mons[PM_MEDUSA])
				sum[i] = gazemu(mtmp, mattk);
			break;

		case AT_EXPL:	/* automatic hit if next to, and aimed at you */
			if(!range2) sum[i] = explmu(mtmp, mattk, foundyou);
			break;

		case AT_ENGL:
			if (!range2) {
			    if(foundyou) {
				if(u.uswallow || tmp > (j = rnd(20+i))) {
				    /* Force swallowing monster to be
				     * displayed even when player is
				     * moving away */
				    flush_screen(1);
				    sum[i] = gulpmu(mtmp, mattk);
				} else {
				    missmu(mtmp, (tmp == j), mattk);
				}
			   } else if (is_animal(mtmp->data))
/*JP					pline("%s gulps some air!", youseeit ?
					      Monnam(mtmp) : "It");*/
					pline("%s��©��ۤ����󤿡�", youseeit ?
					      Monnam(mtmp) : "���Ԥ�");
				  else
					if (youseeit)
/*JP					 pline("%s lunges forward and recoils!",*/
					 pline("%s���Ϳʤ���ä���",
					       Monnam(mtmp));
					else
						You_hear("%s�򤽤Ф�ʹ������",
						    is_whirly(mtmp->data)?
						    "�ͷ⤷�Ƥ��벻" :
						    "�ԥ���äȤ�����");
			}
			break;
		case AT_BREA:
			if(range2) sum[i] = breamu(mtmp, mattk);
			/* Note: breamu takes care of displacement */
			break;
		case AT_SPIT:
			if(range2) sum[i] = spitmu(mtmp, mattk);
			/* Note: spitmu takes care of displacement */
			break;
		case AT_WEAP:
			if(range2) {
#ifdef REINCARNATION
				if (!Is_rogue_level(&u.uz))
#endif
					thrwmu(mtmp);
			} else {
			    /* Rare but not impossible.  Normally the monster
			     * wields when 2 spaces away, but it can be
			     * teleported or whatever....
			     */
			    if (mtmp->weapon_check == NEED_WEAPON ||
							!MON_WEP(mtmp)) {
				mtmp->weapon_check = NEED_HTH_WEAPON;
				/* mon_wield_item resets weapon_check as
				 * appropriate */
				if (mon_wield_item(mtmp) != 0) break;
			    }
			    if (foundyou) {
				possibly_unwield(mtmp);
				otmp = MON_WEP(mtmp);
				if(otmp) {
				    tmp += hitval(otmp, &youmonst);
				    mswings(mtmp, otmp);
				}
				if(tmp > (j = dieroll = rnd(20+i)))
				    sum[i] = hitmu(mtmp, mattk);
				else
				    missmu(mtmp, (tmp == j), mattk);
			    } else
				wildmiss(mtmp, mattk);
			}
			break;
		case AT_MAGC:
			if (range2)
			    sum[i] = buzzmu(mtmp, mattk);
			else
			    if (foundyou)
				sum[i] = castmu(mtmp, mattk);
			    else
/*JP				pline("%s casts a spell at thin air!",
					youseeit ? Monnam(mtmp) : "It");*/
				pline("%s�ϲ���ʤ����֤���ˡ�򤫤�����",
					youseeit ? Monnam(mtmp) : "���Ԥ�");
				/* Not totally right since castmu allows other
				 * spells, such as the monster healing itself,
				 * that should work even when not next to you--
				 * but the previous code was just as wrong.
				 * --KAA
				 */
			break;

		default:		/* no attack */
			break;
	    }
	    if(flags.botl) bot();
	/* give player a chance of waking up before dying -kaa */
	    if(sum[i] == 1) {	    /* successful attack */
		if (u.usleep && u.usleep < monstermoves && !rn2(10)) {
		    multi = -1;
/*JP		    nomovemsg = "The combat suddenly awakens you.";*/
		    nomovemsg = "���ʤ��ϵ������줿��";
		}
	    }
	    if(sum[i] == 2) return 1;		/* attacker dead */
	    if(sum[i] == 3) break;  /* attacker teleported, no more attacks */
	    /* sum[i] == 0: unsuccessful attack */
	}
	return(0);
}

#endif /* OVL0 */
#ifdef OVLB

/*
 * helper function for some compilers that have trouble with hitmu
 */

STATIC_OVL void
hurtarmor(mdat, attk)
struct permonst *mdat;
int attk;
{
	boolean getbronze, rusting;
	int	hurt;

	rusting = (attk == AD_RUST);
	if (rusting) {
		hurt = 1;
		getbronze = (mdat == &mons[PM_BLACK_PUDDING] &&
			     uarm && is_corrodeable(uarm));
	}
	else {
		hurt=2;
		getbronze = FALSE;
	}
	/* What the following code does: it keeps looping until it
	 * finds a target for the rust monster.
	 * Head, feet, etc... not covered by metal, or covered by
	 * rusty metal, are not targets.  However, your body always
	 * is, no matter what covers it.
	 */
	while (1) {
	    switch(rn2(5)) {
	    case 0:
/*JP		if (!rust_dmg(uarmh, rusting ? "helmet" : "leather helmet",*/
		if (!rust_dmg(uarmh, rusting ? "��" : "��γ�",
					 hurt, FALSE))
			continue;
		break;
	    case 1:
		if (uarmc) {
		    if (!rusting)
/*JP			(void)rust_dmg(uarmc, "cloak", hurt, TRUE);*/
			(void)rust_dmg(uarmc, "������", hurt, TRUE);
		    break;
		}
		/* Note the difference between break and continue;
		 * break means it was hit and didn't rust; continue
		 * means it wasn't a target and though it didn't rust
		 * something else did.
		 */
		if (getbronze)
/*JP		    (void)rust_dmg(uarm, "bronze armor", 3, TRUE);*/
		    (void)rust_dmg(uarm, "��Ƽ�γ�", 3, TRUE);
		else if (uarm)
		    (void)rust_dmg(uarm, xname(uarm), hurt, TRUE);
#ifdef TOURIST
		else if (uarmu)
/*JP		    (void)rust_dmg(uarmu, "shirt", hurt, TRUE);*/
		    (void)rust_dmg(uarmu, "�����", hurt, TRUE);
#endif
		break;
	    case 2:
/*JP		if (!rust_dmg(uarms, rusting ? "shield" : "wooden shield",
					 hurt, FALSE))*/
		if (!rust_dmg(uarms, rusting ? "��" : "�ڤν�",
					 hurt, FALSE))
			continue;
		break;
	    case 3:
/*JP		if (!rust_dmg(uarmg, rusting ? "metal gauntlets" : "gloves",*/
		if (!rust_dmg(uarmg, rusting ? "��°�ξ���" : "����",
					 hurt, FALSE))
			continue;
		break;
	    case 4:
/*JP		if (!rust_dmg(uarmf, rusting ? "metal boots" : "boots",*/
		if (!rust_dmg(uarmf, rusting ? "��°�η�" : "��",
					 hurt, FALSE))
			continue;
		break;
	    }
	    break; /* Out of while loop */
	}
}

#endif /* OVLB */
#ifdef OVL1

STATIC_OVL boolean
diseasemu(mdat)
struct permonst *mdat;
{
	static char jbuf[BUFSZ];

	Strcpy(jbuf, jtrns_mon(mdat->mname, -1));
	Strcat(jbuf, "�ˤ�ä�");

	if (defends(AD_DISE,uwep) || u.usym == S_FUNGUS) {
/*JP		You_feel("a slight illness.");*/
		You("��������ʬ�������ʤä��褦�ʵ���������");
		return FALSE;
	} else {
		make_sick(Sick ? Sick/3L + 1L : (long)rn1(ACURR(A_CON), 20),
/*JP			mdat->mname, TRUE, SICK_NONVOMITABLE);*/
			jbuf, TRUE, SICK_NONVOMITABLE);
		return TRUE;
	}
}

/* check whether slippery clothing protects from hug or wrap attack */
static boolean
u_slip_free(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
	struct obj *obj = (uarmc ? uarmc : uarm);

#ifdef TOURIST
	if (!obj) obj = uarmu;
#endif
	/* if your cloak/armor is greased, monster slips off */
	if (obj && (obj->greased || obj->otyp == OILSKIN_CLOAK)) {
/*JP	    pline("%s %s your %s %s!",
		  Monnam(mtmp),
		  (mattk->adtyp == AD_WRAP) ?
			"slips off of" : "grabs you, but cannot hold onto",
		  obj->greased ? "greased" : "slippery",
		  * avoid "slippery slippery cloak"
		     for undiscovered oilskin cloak *
		  (obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : "cloak");*/
	    pline("%s��%s%s%s��",
		  Monnam(mtmp),
		  obj->greased ? "�����ɤ�줿" : "���䤹��",
		  (obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : "������",
		  (mattk->adtyp == AD_WRAP) ?
			"�ǳ�ä�" : "��Ĥ��ޤ��褦�Ȥ������������Ǥ��ʤ��ä�");
		  /* avoid "slippery slippery cloak"
		     for undiscovered oilskin cloak */

	    if (obj->greased && !rn2(2)) {
/*JP		pline_The("grease wears off.");*/
		pline("��������Ƥ��ޤä���");
		obj->greased = 0;
	    }
	    return TRUE;
	}
	return FALSE;
}

/*
 * hitmu: monster hits you
 *	  returns 2 if monster dies (e.g. "yellow light"), 1 otherwise
 *	  3 if the monster lives but teleported/paralyzed, so it can't keep
 *	       attacking you
 */
STATIC_OVL int
hitmu(mtmp, mattk)
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	register struct permonst *mdat = mtmp->data;
	register int uncancelled, ptmp;
	int dmg, armpro;
	char	 buf[BUFSZ];
	struct permonst *olduasmon = uasmon;
	int res;

/*	If the monster is undetected & hits you, you should know where
 *	the attack came from.
 */
	if(mtmp->mundetected && (hides_under(mdat) || mdat->mlet == S_EEL)) {
	    mtmp->mundetected = 0;
	    if (!(Blind ? Telepat : (HTelepat & ~INTRINSIC))) {
		struct obj *obj;
		const char *what;

		if ((obj = level.objects[mtmp->mx][mtmp->my]) != 0) {
		    if (Blind && !obj->dknown)
			what = something;
		    else if (is_pool(mtmp->mx, mtmp->my) && !Underwater)
/*JP			what = "the water";*/
		        pline("%s������˱���Ƥ��롪", Amonnam(mtmp));
		    else
/*JP			what = doname(obj);*/
		        pline("%s��%s�β��˱���Ƥ��롪", Amonnam(mtmp), doname(obj));

/*JP		    pline("%s was hidden under %s!", Amonnam(mtmp), what);*/
		}
		newsym(mtmp->mx, mtmp->my);
	    }
	}

/*	First determine the base damage done */
	dmg = d((int)mattk->damn, (int)mattk->damd);
	if(is_undead(mdat) && midnight())
		dmg += d((int)mattk->damn, (int)mattk->damd); /* extra damage */

/*	Next a cancellation factor	*/
/*	Use uncancelled when the cancellation factor takes into account certain
 *	armor's special magic protection.  Otherwise just use !mtmp->mcan.
 */
	armpro = 0;
	if (uarm && armpro < objects[uarm->otyp].a_can)
		armpro = objects[uarm->otyp].a_can;
	if (uarmc && armpro < objects[uarmc->otyp].a_can)
		armpro = objects[uarmc->otyp].a_can;
	if (uarmh && armpro < objects[uarmh->otyp].a_can)
		armpro = objects[uarmh->otyp].a_can;
	uncancelled = !mtmp->mcan && ((rn2(3) >= armpro) || !rn2(50));

/*	Now, adjust damages via resistances or specific attacks */
	switch(mattk->adtyp) {
	    case AD_PHYS:
		if (mattk->aatyp == AT_HUGS && !sticks(uasmon)) {
		    if(!u.ustuck && rn2(2)) {
			if (u_slip_free(mtmp, mattk)) {
			    dmg = 0;
			} else {
			    u.ustuck = mtmp;
/*JP			    pline("%s grabs you!", Monnam(mtmp));*/
			    pline("%s�ˤĤ��ޤ����Ƥ��롪", Monnam(mtmp));
			}
		    } else if(u.ustuck == mtmp) {
			exercise(A_STR, FALSE);
/*JP			You("are being %s.",
			      (mtmp->data == &mons[PM_ROPE_GOLEM])
			      ? "choked" : "crushed");*/
			You("%s�Ƥ��롥",
			      (mtmp->data == &mons[PM_ROPE_GOLEM])
			      ? "���ʤ���" : "�����Ĥ֤���");
		    }
		} else {			  /* hand to hand weapon */
		    if(mattk->aatyp == AT_WEAP && otmp) {
			if (otmp->otyp == CORPSE
				&& otmp->corpsenm == PM_COCKATRICE) {
			    dmg = 1;
/*JP			    pline("%s hits you with the cockatrice corpse.",
				Monnam(mtmp));*/
			    pline("%s�ϥ����ȥꥹ�λ��Τǹ��⤷����",
  				Monnam(mtmp));
			    if (!Stoned)
				goto do_stone;
			}
			dmg += dmgval(otmp, &youmonst);
			if (dmg <= 0) dmg = 1;
			if (!(otmp->oartifact &&
				artifact_hit(mtmp, &youmonst, otmp, &dmg,dieroll)))
			     hitmsg(mtmp, mattk);
			if (!dmg) break;
			if (u.mh > 1 && u.mh > ((u.uac>0) ? dmg : dmg+u.uac) &&
					(u.umonnum==PM_BLACK_PUDDING
					|| u.umonnum==PM_BROWN_PUDDING)) {
			    /* This redundancy necessary because you have to
			     * take the damage _before_ being cloned.
			     */
			    if (u.uac < 0) dmg += u.uac;
			    if (dmg < 1) dmg = 1;
			    if (dmg > 1) exercise(A_STR, FALSE);
			    u.mh -= dmg;
			    flags.botl = 1;
			    dmg = 0;
			    if(cloneu())
/*JP			    You("divide as %s hits you!",mon_nam(mtmp));*/
			    pline("%s�ι���ˤ�äƤ��ʤ���ʬ��������",mon_nam(mtmp));
			}
			urustm(mtmp, otmp);
		    } else if (mattk->aatyp != AT_TUCH || dmg != 0 ||
				mtmp != u.ustuck)
			hitmsg(mtmp, mattk);
		}
		break;
	    case AD_DISE:
		hitmsg(mtmp, mattk);
		if (!diseasemu(mdat)) dmg = 0;
		break;
	    case AD_FIRE:
		hitmsg(mtmp, mattk);
		if (uncancelled) {
/*JP		    pline("You're %s!",
			  mattk->aatyp == AT_HUGS ? "being roasted" :
			  "on fire");*/
		    You("%s�ˤʤä���",
			  mattk->aatyp == AT_HUGS ? "���Ǥ��ˤʤä�" :
			  "�Ф����");
		    if (Fire_resistance) {
/*JP			pline_The("fire doesn't feel hot!");*/
			pline("�ФϤ��󤼤�Ǯ���ʤ���");
			dmg = 0;
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(SCROLL_CLASS, AD_FIRE);
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(POTION_CLASS, AD_FIRE);
		    if((int) mtmp->m_lev > rn2(25))
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		} else dmg = 0;
		break;
	    case AD_COLD:
		hitmsg(mtmp, mattk);
		if (uncancelled) {
/*JP		    pline("You're covered in frost!");*/
		    You("ɹ��ʤ��줿��");
		    if (Cold_resistance) {
/*JP			pline_The("frost doesn't seem cold!");*/
			pline("ɹ���䤵�򴶤������ʤ���");
			dmg = 0;
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(POTION_CLASS, AD_COLD);
		} else dmg = 0;
		break;
	    case AD_ELEC:
		hitmsg(mtmp, mattk);
		if (uncancelled) {
/*JP		    You("get zapped!");*/
		    You("�ŷ�򤯤�ä���");
		    if (Shock_resistance) {
/*JP			pline_The("zap doesn't shock you!");*/
			pline("�ŷ�Ϥ��Ӥ�򴶤������ʤ���");
			dmg = 0;
		    }
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(WAND_CLASS, AD_ELEC);
		    if((int) mtmp->m_lev > rn2(20))
			destroy_item(RING_CLASS, AD_ELEC);
		} else dmg = 0;
		break;
	    case AD_SLEE:
		hitmsg(mtmp, mattk);
		if (uncancelled && multi >= 0 && !rn2(5)) {
		    if (Sleep_resistance) break;
		    fall_asleep(-rnd(10), TRUE);
/*JP		    if (Blind) You("are put to sleep!");*/
		    if (Blind) You("̲��ˤ�������");
/*JP		    else You("are put to sleep by %s!", mon_nam(mtmp));*/
		    else You("%s��̲�餵�줿��", mon_nam(mtmp));
		}
		break;
	    case AD_DRST:
		ptmp = A_STR;
		goto dopois;
	    case AD_DRDX:
		ptmp = A_DEX;
		goto dopois;
	    case AD_DRCO:
		ptmp = A_CON;
dopois:
		hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(8)) {
/*JP			Sprintf(buf, "%s %s",*/
			Sprintf(buf, "%s��%s",
/*JP				!canspotmon(mtmp) ? "Its" :*/
				!canspotmon(mtmp) ? "���Ԥ�" :
				Hallucination ? s_suffix(rndmonnam()) :
				                Monnam(mtmp),
				mpoisons_subj(mtmp, mattk));
/*JP			poisoned(buf, ptmp, jtrns_mon(mdat->mname, -1), 30);*/
			{
			  char jbuf[BUFSZ];
			  Sprintf(jbuf, "%s", jtrns_mon(mdat->mname, mtmp->female));
			  poisoned(buf, ptmp, jbuf, 30);
			}
		}
		break;
	    case AD_DRIN:
		hitmsg(mtmp, mattk);
		if (defends(AD_DRIN, uwep) || !has_head(uasmon)) {
/*JP		    You("don't seem harmed.");*/
		    You("���Ĥ��Ƥ��ʤ��褦����");
		    break;
		}
		if (uarmh && rn2(8)) {
/*JP		    Your("helmet blocks the attack to your head.");*/
		    Your("����Ƭ�ؤι�����ɤ�����");
		    break;
		}
		if (Half_physical_damage) dmg = (dmg+1) / 2;
		losehp(dmg, mon_nam(mtmp), KILLED_BY_AN);

		if (!uarmh || uarmh->otyp != DUNCE_CAP) {
/*JP		    Your("brain is eaten!");*/
		    Your("Ǿ�Ͽ��٤�줿��");
		    /* No such thing as mindless players... */
		    if (ABASE(A_INT) <= ATTRMIN(A_INT)) {
			int lifesaved = 0;
			struct obj *wore_amulet = uamul;

			while(1) {
			    /* avoid looping on "die(y/n)?" */
			    if (lifesaved && (discover || wizard)) {
				if (wore_amulet && !uamul) {
				    /* used up AMULET_OF_LIFE_SAVING; still
				       subject to dying from brainlessness */
				    wore_amulet = 0;
				} else {
				    /* explicitly chose not to die;
				       arbitrarily boost intelligence */
				    ABASE(A_INT) = ATTRMIN(A_INT) + 2;
/*JP				    You_feel("like a scarecrow.");*/
				    You("�������Τ褦�ʵ�����������");
				    break;
				}
			    }

			    if (lifesaved)
/*JP				pline("Unfortunately your brain is still gone.");*/
			        pline("��ǰ�ʤ��餢�ʤ��ˤ�Ǿ���ʤ���");
			    else
/*JP				Your("last thought fades away.");*/
				Your("�Ǹ�λפ����������Τ褦�˲����ä���");
/*JP			    killer = "brainlessness";*/
			    killer = "Ǿ�򼺤��ʤä�";
			    killer_format = KILLED_BY;
			    done(DIED);
			    lifesaved++;
			}
		    }
		}
		/* adjattrib gives dunce cap message when appropriate */
		(void) adjattrib(A_INT, -rnd(2), FALSE);
		forget_levels(25);	/* lose memory of 25% of levels */
		forget_objects(25);	/* lose memory of 25% of objects */
		exercise(A_WIS, FALSE);
		break;
	    case AD_PLYS:
		hitmsg(mtmp, mattk);
		if (uncancelled && multi >= 0 && !rn2(3)) {
/*JP		    if (Blind) You("are frozen!");*/
		    if (Blind) You("ư���ʤ���");
/*JP		    else You("are frozen by %s!", mon_nam(mtmp));*/
		    else pline("%s�ˤ�ä�ư���ʤ��ʤä���", mon_nam(mtmp));
		    nomovemsg = 0;	/* default: "you can move again" */
		    nomul(-rnd(10));
		    exercise(A_DEX, FALSE);
		}
		break;
	    case AD_DRLI:
		hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(3) && !resists_drli(&youmonst))
		    losexp();
		break;
	    case AD_LEGS:
		{ register long side = rn2(2) ? RIGHT_SIDE : LEFT_SIDE;
/*JP		  const char *sidestr = (side == RIGHT_SIDE) ? "right" : "left";*/
		  const char *sidestr = (side == RIGHT_SIDE) ? "��" : "��";
		  if (mtmp->mcan) {
/*JP		    pline("%s nuzzles against your %s %s!", Monnam(mtmp),
			  sidestr, body_part(LEG));*/
		    pline("%s�Ϥ��ʤ���%s%s��ɡ�򤹤�褻����", Monnam(mtmp),
			  sidestr, body_part(LEG));
		  } else {
		    if (uarmf) {
			if (rn2(2) && (uarmf->otyp == LOW_BOOTS ||
					     uarmf->otyp == IRON_SHOES))
/*JP			    pline("%s pricks the exposed part of your %s %s!",*/
			    pline("%s�Ϥ��ʤ���%s%s�������Ȼɤ�����",
				Monnam(mtmp), sidestr, body_part(LEG));
			else if (!rn2(5))
/*JP			    pline("%s pricks through your %s boot!",*/
			    pline("%s�Ϥ��ʤ���%s�η��������Ȼɤ�����", 
				Monnam(mtmp), sidestr);
			else {
/*JP			    pline("%s scratches your %s boot!", Monnam(mtmp),*/
			    pline("%s�Ϥ��ʤ���%s�η���Ҥä�������", Monnam(mtmp),
				sidestr);
			    break;
			}
/*JP		    } else pline("%s pricks your %s %s!", Monnam(mtmp),*/
		    } else pline("%s�Ϥ��ʤ���%s%s�������Ȼɤ�����", Monnam(mtmp),
			  sidestr, body_part(LEG));
		    set_wounded_legs(side, rnd(60-ACURR(A_DEX)));
		    exercise(A_STR, FALSE);
		    exercise(A_DEX, FALSE);
		  }
		  break;
		}
	    case AD_STON:	/* at present only a cockatrice */
		hitmsg(mtmp, mattk);
		if(!rn2(3) && !Stoned) {
		    if (mtmp->mcan) {
			if (flags.soundok)
/*JP			    You_hear("a cough from %s!", mon_nam(mtmp));*/
			    You("%s�����ۥå��ۥäȤ�������ʹ������", mon_nam(mtmp));
		    } else {
			if (flags.soundok)
/*JP			    You_hear("%s hissing!", s_suffix(mon_nam(mtmp)));*/
			    You("%s�������äȤ�������ʹ������", s_suffix(mon_nam(mtmp)));
			if(!rn2(10) ||
			    (flags.moonphase == NEW_MOON && !have_lizard())) {
do_stone:
			    if (!resists_ston(&youmonst)
				    && !(poly_when_stoned(uasmon) &&
					polymon(PM_STONE_GOLEM))) {
				Stoned = 5;
				return(1);
				/* You("turn to stone..."); */
				/* done_in_by(mtmp); */
			    }
			}
		    }
		}
		break;
	    case AD_STCK:
		hitmsg(mtmp, mattk);
		if (uncancelled && !u.ustuck && !sticks(uasmon))
			u.ustuck = mtmp;
		break;
	    case AD_WRAP:
		if ((!mtmp->mcan || u.ustuck == mtmp) && !sticks(uasmon)) {
		    if (!u.ustuck && !rn2(10)) {
			if (u_slip_free(mtmp, mattk)) {
			    dmg = 0;
			} else {
/*JP			    pline("%s swings itself around you!",*/
			    pline("%s�ϼ�ʬ���Ȥ���ߤĤ����Ƥ�����",
				  Monnam(mtmp));
			    u.ustuck = mtmp;
			}
		    } else if(u.ustuck == mtmp) {
			if (is_pool(mtmp->mx,mtmp->my) && !is_swimmer(uasmon)
			    && !Amphibious) {
			    boolean moat = (levl[u.ux][u.uy].typ != POOL) &&
				(levl[u.ux][u.uy].typ != WATER) &&
				!Is_medusa_level(&u.uz) &&
				!Is_waterlevel(&u.uz);

/*JP			    pline("%s drowns you...", Monnam(mtmp));*/
			    pline("���ʤ���%s����ߤĤ����Ů�줿������", Monnam(mtmp));
			    killer_format = KILLED_BY_AN;
/*JP			    Sprintf(buf, "%s by %s",
				    moat ? "moat" : "pool of water",
				    a_monnam(mtmp)) */
			    Sprintf(buf, "%s��%s����ߤĤ����",
				    moat ? "��" : "��",
				    a_monnam(mtmp));
			    killer = buf;
			    done(DROWNING);
			} else if(mattk->aatyp == AT_HUGS)
/*JP			    You("are being crushed.");*/
			    You("�����Ĥ֤��줿��");
		    } else {
			dmg = 0;
			if(flags.verbose)
/*JP			    pline("%s brushes against your %s.", Monnam(mtmp),*/
			    pline("%s�Ϥ��ʤ���%s�˿��줿��", Monnam(mtmp),
				   body_part(LEG));
		    }
		} else dmg = 0;
		break;
	    case AD_WERE:
		hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(4) && u.ulycn == PM_PLAYERMON &&
			!Protection_from_shape_changers &&
			!defends(AD_WERE,uwep)) {
		    You("Ǯ������褦�ʵ���������");
		    exercise(A_CON, FALSE);
		    u.ulycn = monsndx(mdat);
		}
		break;
	    case AD_SGLD:
		hitmsg(mtmp, mattk);
		if (u.usym == mdat->mlet) break;
		if(!mtmp->mcan) stealgold(mtmp);
		break;

	    case AD_SITM:	/* for now these are the same */
	    case AD_SEDU:
		if (dmgtype(uasmon, AD_SEDU)
#ifdef SEDUCE
			|| dmgtype(uasmon, AD_SSEX)
#endif
						) {
			if (mtmp->minvent)
/*JP	pline("%s brags about the goods some dungeon explorer provided.",*/
	pline("%s�Ϥ����µ�õ���Ȥ��֤��Ƥä���ʪ��������",
	Monnam(mtmp));
			else
/*JP	pline("%s makes some remarks about how difficult theft is lately.",*/
	pline("%s�ϺǶᡤ���𤬤����˺���ø���ȽҤ٤���",
	Monnam(mtmp));
			if (!tele_restrict(mtmp)) rloc(mtmp);
			return 3;
		} else if (mtmp->mcan) {
		    if (!Blind) {
/*JP			pline("%s tries to %s you, but you seem %s.",
			    Adjmonnam(mtmp, "plain"),
			    flags.female ? "charm" : "seduce",
			    flags.female ? "unaffected" : "uninterested");*/
			pline("%s�Ϥ��ʤ���%s���褦�Ȥ��������������ʤ���%s",
			    Adjmonnam(mtmp, "���̤�"),
			    flags.female ? "̥λ" : "Ͷ��",
			    flags.female ? "�ƶ��򤦤��ʤ�" : "��̣���ʤ�");
		    }
		    if(rn2(3)) {
			if (!tele_restrict(mtmp)) rloc(mtmp);
			return 3;
		    }
		} else {
		    switch (steal(mtmp)) {
		      case -1:
			return 2;
		      case 0:
			break;
		      default:
			if (!tele_restrict(mtmp)) rloc(mtmp);
			mtmp->mflee = 1;
			return 3;
		    }
		}
		break;
#ifdef SEDUCE
	    case AD_SSEX:
		if(could_seduce(mtmp, &youmonst, mattk) == 1
			&& !mtmp->mcan)
		    if (doseduce(mtmp))
			return 3;
		break;
#endif
	    case AD_SAMU:
		hitmsg(mtmp, mattk);
		/* when the Wiz hits, 1/20 steals the amulet */
		if (u.uhave.amulet ||
		     u.uhave.bell || u.uhave.book || u.uhave.menorah
		     || u.uhave.questart) /* carrying the Quest Artifact */
		    if (!rn2(20)) stealamulet(mtmp);
		break;

	    case AD_TLPT:
		hitmsg(mtmp, mattk);
		if (uncancelled) {
		    if(flags.verbose)
/*JP			Your("position suddenly seems very uncertain!");*/
			pline("��ʬ�Τ�����֤����������Τˤʤä���");
		    tele();
		}
		break;
	    case AD_RUST:
		hitmsg(mtmp, mattk);
		if (mtmp->mcan) break;
		if (u.umonnum == PM_IRON_GOLEM) {
/*JP			You("rust!");*/
			You("���ӤĤ�����");
			rehumanize();
			break;
		}
		hurtarmor(mdat, AD_RUST);
		break;
	    case AD_DCAY:
		hitmsg(mtmp, mattk);
		if (mtmp->mcan) break;
		if (u.umonnum == PM_WOOD_GOLEM ||
		    u.umonnum == PM_LEATHER_GOLEM) {
/*JP			You("rot!");*/
			You("��ä���");
			rehumanize();
			break;
		}
		hurtarmor(mdat, AD_DCAY);
		break;
	    case AD_HEAL:
		if(!uwep
#ifdef TOURIST
		   && !uarmu
#endif
		   && !uarm && !uarmh && !uarms && !uarmg && !uarmc && !uarmf) {
		    boolean goaway = FALSE;
/*JP		    pline("%s hits!  (I hope you don't mind.)", Monnam(mtmp));*/
		    pline("%s�ι����̿�椷��(���ˤ��ʤ��褦��)", Monnam(mtmp));
		    if (u.mtimedone) {
			u.mh += rnd(7);
			if (!rn2(7)) {
			    /* no upper limit necessary; effect is temporary */
			    u.mhmax++;
			    if (!rn2(13)) goaway = TRUE;
			}
			if (u.mh > u.mhmax) u.mh = u.mhmax;
		    } else {
			u.uhp += rnd(7);
			if (!rn2(7)) {
			    /* hard upper limit via nurse care: 25 * ulevel */
			    if (u.uhpmax < 5 * u.ulevel + d(2 * u.ulevel, 10))
				u.uhpmax++;
			    if (!rn2(13)) goaway = TRUE;
			}
			if (u.uhp > u.uhpmax) u.uhp = u.uhpmax;
		    }
		    if (!rn2(3)) exercise(A_STR, TRUE);
		    if (!rn2(3)) exercise(A_CON, TRUE);
		    if (Sick) make_sick(0L, (char *) 0, FALSE, SICK_ALL);
		    flags.botl = 1;
		    if (goaway) {
			mongone(mtmp);
			return 2;
		    } else if (!rn2(33)) {
			if (!tele_restrict(mtmp)) rloc(mtmp);
			if (!mtmp->mflee) {
			    mtmp->mflee = 1;
			    mtmp->mfleetim = d(3,6);
			}
			return 3;
		    }
		    dmg = 0;
		} else {
		    if (Role_is('H')) {
			if (flags.soundok && !(moves % 5))
/*JP		      verbalize("Doc, I can't help you unless you cooperate.");*/
		      verbalize("�ɥ����������Ϥ򤪤ͤ������ޤ��");
			dmg = 0;
		    } else hitmsg(mtmp, mattk);
		}
		break;
	    case AD_CURS:
		hitmsg(mtmp, mattk);
		if(!night() && mdat == &mons[PM_GREMLIN]) break;
		if(!mtmp->mcan && !rn2(10)) {
		    if (flags.soundok)
/*JP			if (Blind) You_hear("laughter.");
			else       pline("%s chuckles.", Monnam(mtmp));*/
			if (Blind) You_hear("�Ф�����ʹ������");
			else       pline("%s�ϥ��������Фä���", Monnam(mtmp));
		    if (u.umonnum == PM_CLAY_GOLEM) {
/*JP			pline("Some writing vanishes from your head!");*/
			pline("�����Ĥ���ʸ�������ʤ���Ƭ����ä�����");
			rehumanize();
			break;
		    }
		    attrcurse();
		}
		break;
	    case AD_STUN:
		hitmsg(mtmp, mattk);
		if(!mtmp->mcan && !rn2(4)) {
		    make_stunned(HStun + dmg, TRUE);
		    dmg /= 2;
		}
		break;
	    case AD_ACID:
		hitmsg(mtmp, mattk);
		if(!mtmp->mcan && !rn2(3))
		    if (resists_acid(&youmonst)) {
/*JP			pline("You're covered in acid, but it seems harmless.");*/
			pline("����ʤ��줿�����������Ĥ��ʤ���");
			dmg = 0;
		    } else {
/*JP			pline("You're covered in acid!	It burns!");*/
			You("����ʤ���Ƥ�����");
			exercise(A_STR, FALSE);
		    }
		else		dmg = 0;
		break;
	    case AD_SLOW:
		hitmsg(mtmp, mattk);
		if (uncancelled && (Fast & (INTRINSIC|TIMEOUT)) &&
					!defends(AD_SLOW, uwep) && !rn2(4))
		    u_slow_down();
		break;
	    case AD_DREN:
		hitmsg(mtmp, mattk);
		if (uncancelled && !rn2(4))
		    drain_en(dmg);
		dmg = 0;
		break;
	    case AD_CONF:
		hitmsg(mtmp, mattk);
		if(!mtmp->mcan && !rn2(4) && !mtmp->mspec_used) {
		    mtmp->mspec_used = mtmp->mspec_used + (dmg + rn2(6));
		    if(Confusion)
/*JP			 You("are getting even more confused.");*/
			 You("�ޤ��ޤ����𤷤���");
/*JP		    else You("are getting confused.");*/
		    else You("���𤷤Ƥ�����");
		    make_confused(HConfusion + dmg, FALSE);
		}
		/* fall through to next case */
	    case AD_DETH:
/*JP		pline("%s reaches out with its deadly touch.", Monnam(mtmp));*/
		pline("%s�ϻ���Ӥ�ΤФ�����", Monnam(mtmp));
		if (is_undead(uasmon)) {
		    /* Still does normal damage */
/*JP		    pline("Was that the touch of death?");*/
		    pline("����ϻ����𤫤ʡ�");
		    break;
		}
		if(!Antimagic && rn2(20) > 16)  {
		    killer_format = KILLED_BY_AN;
/*JP		    killer = "touch of death";*/
		    killer = "����Ӥ�";
		    done(DIED);
		} else {
		    if(!rn2(5)) {
			if(Antimagic) shieldeff(u.ux, u.uy);
/*JP			pline("Lucky for you, it didn't work!");*/
			pline("���Τ褤���Ȥˤʤ�Ȥ�ʤ��ä���");
			dmg = 0;
/*JP		    } else You_feel("your life force draining away...");*/
		    } else You("���Ϥ�å���Ƥ����褦�ʵ�������������");
		}
		break;
	    case AD_PEST:
/*JP		pline("%s reaches out, and you feel fever and chills.",*/
		pline("%s���Ӥ�ΤФ��������ʤ��ϰ����򴶤�����",
			Monnam(mtmp));
		(void) diseasemu(mdat); /* plus the normal damage */
		break;
	    case AD_FAMN:
/*JP		pline("%s reaches out, and your body shrivels.",*/
		pline("%s���Ӥ򿭤Ф��������ʤ����ΤϤ��ܤ����",
			Monnam(mtmp));
		exercise(A_CON, FALSE);
		if (!is_fainted()) morehungry(rn1(40,40));
		/* plus the normal damage */
		break;
	    default:	dmg = 0;
			break;
	}
	if(u.uhp < 1) done_in_by(mtmp);

/*	Negative armor class reduces damage done instead of fully protecting
 *	against hits.
 */
	if (dmg && u.uac < 0) {
		dmg -= rnd(-u.uac);
		if (dmg < 1) dmg = 1;
	}

	if(dmg) {
	    if (Half_physical_damage
					/* Mitre of Holiness */
		|| (Role_is('P') && uarmh && is_quest_artifact(uarmh) &&
		    (is_undead(mtmp->data) || is_demon(mtmp->data))))
		dmg = (dmg+1) / 2;
	    mdamageu(mtmp, dmg);
	}

	if (dmg) {
	    res = passiveum(olduasmon, mtmp, mattk);
	    stop_occupation();
	    return res;
	} else
	    return 1;
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL int
gulpmu(mtmp, mattk)	/* monster swallows you, or damage if u.uswallow */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	struct trap *t = t_at(u.ux, u.uy);
	int	tmp = d((int)mattk->damn, (int)mattk->damd);
	int	tim_tmp;
	register struct obj *otmp2;
	int	i;

	if (!u.uswallow) {	/* swallows you */
		if (uasmon->msize >= MZ_HUGE) return(0);
		if ((t && ((t->ttyp == PIT) || (t->ttyp == SPIKED_PIT))) &&
		    sobj_at(BOULDER, u.ux, u.uy))
			return(0);

		if (Punished) unplacebc();	/* ball&chain go away */
		remove_monster(mtmp->mx, mtmp->my);
		place_monster(mtmp, u.ux, u.uy);
		u.ustuck = mtmp;
		newsym(mtmp->mx,mtmp->my);
/*JP		pline("%s engulfs you!", Monnam(mtmp));*/
		pline("%s�Ϥ��ʤ�����ߤ������", Monnam(mtmp));
		stop_occupation();

		if (u.utrap) {
/*JP			You("are released from the %s!",
				u.utraptype==TT_WEB ? "web" : "trap");*/
			You("%s����������줿��",
				u.utraptype==TT_WEB ? "�������" : "�");
			u.utrap = 0;
		}

		i = number_leashed();
		if (i > 0) {
/*JP			pline_The("leash%s snap%s loose.",
					(i > 1) ? "es" : "",
					(i > 1) ? "" : "s");*/
			pline("ɳ�ϥѥ�����ڤ줿��");
			unleash_all();
		}

		if (u.umonnum==PM_COCKATRICE && !resists_ston(mtmp)) {
			minstapetrify(mtmp, TRUE);
			if (mtmp->mhp > 0) return 0;
			else return 2;
		}

		display_nhwindow(WIN_MESSAGE, FALSE);
		vision_recalc(2);	/* hero can't see anything */
		u.uswallow = 1;
		/* assume that u.uswldtim always set >= 0 */
		u.uswldtim = (tim_tmp =
			(-u.uac + 10 + rnd(25 - (int)mtmp->m_lev)) >> 1) > 0 ?
			    tim_tmp : 0;
		swallowed(1);
		for(otmp2 = invent; otmp2; otmp2 = otmp2->nobj) {
			(void) snuff_lit(otmp2);
		}
	}

	if (mtmp != u.ustuck) return(0);

	switch(mattk->adtyp) {

		case AD_DGST:
		    if(u.uswldtim <= 1) {	/* a3 *//*no cf unsigned <=0*/
/*JP			pline("%s totally digests you!", Monnam(mtmp));*/
			pline("%s�Ϥ��ʤ������˾ò�������", Monnam(mtmp));
			tmp = u.uhp;
			if (Half_physical_damage) tmp *= 2; /* sorry */
		    } else {
/*JP			pline("%s digests you!", Monnam(mtmp));*/
			pline("%s�Ϥ��ʤ���ò����Ƥ��롪", Monnam(mtmp));
		        exercise(A_STR, FALSE);
		    }
		    break;
		case AD_PHYS:
/*JP		    You("are pummeled with debris!");*/
		    You("��㪤��ˤ�Ĥ���줿��");
		    exercise(A_STR, FALSE);
		    break;
		case AD_ACID:
		    if (resists_acid(&youmonst)) {
/*JP			You("are covered with a seemingly harmless goo.");*/
			You("�ͤФĤ���ΤǤ���줿��");
			tmp = 0;
		    } else {
/*JP		      if (Hallucination) pline("Ouch!  You've been slimed!");
		      else You("are covered in slime!  It burns!");*/
		      if (Hallucination) pline("�����󡪤��ʤ��Ϥ̤�̤����");
		      else You("���饤���ʤ��줿�������ƻ��˾Ƥ��줿��");
		      exercise(A_STR, FALSE);
		    }
		    break;
		case AD_BLND:
		    if (!resists_blnd(&youmonst)) {
			if(!Blind) {
/*JP			    You_cant("see in here!");*/
			    You("���⸫���ʤ���");
			    make_blinded((long)tmp,FALSE);
			} else
			    /* keep him blind until disgorged */
			    make_blinded(Blinded+1,FALSE);
		    }
		    tmp = 0;
		    break;
		case AD_ELEC:
		    if(!mtmp->mcan && rn2(2)) {
/*JP			pline_The("air around you crackles with electricity.");*/
			pline("���ʤ��β��ζ��������ŵ��ǥԥ�ԥꤷ�Ƥ��롥");

			if (Shock_resistance) {
				shieldeff(u.ux, u.uy);
/*JP				You("seem unhurt.");*/
				You("���Ĥ��ʤ��褦����");
				ugolemeffects(AD_ELEC,tmp);
				tmp = 0;
			}
		    } else tmp = 0;
		    break;
		case AD_COLD:
		    if(!mtmp->mcan && rn2(2)) {
			if (Cold_resistance) {
				shieldeff(u.ux, u.uy);
/*JP				You_feel("mildly chilly.");*/
				pline("�Ҥ��ꤷ����");
				ugolemeffects(AD_COLD,tmp);
				tmp = 0;
/*JP			} else You("are freezing to death!");*/
			} else You("��ष��������");
		    } else tmp = 0;
		    break;
		case AD_FIRE:
		    if(!mtmp->mcan && rn2(2)) {
			if (Fire_resistance) {
				shieldeff(u.ux, u.uy);
/*JP				You_feel("mildly hot.");*/
				pline("�ݥ��ݥ�������");
				ugolemeffects(AD_FIRE,tmp);
				tmp = 0;
/*JP			} else You("are burning to a crisp!");*/
			} else You("ǳ���ƥ��饫��ˤʤä���");
		    } else tmp = 0;
		    break;
		case AD_DISE:
		    if (!diseasemu(mtmp->data)) tmp = 0;
		    break;
		default:
		    tmp = 0;
		    break;
	}

	if (Half_physical_damage) tmp = (tmp+1) / 2;

	mdamageu(mtmp, tmp);
	if(tmp) stop_occupation();
	if(u.uswldtim) --u.uswldtim;

	if (u.umonnum == PM_COCKATRICE && !resists_ston(mtmp)) {
/*JP		pline("%s very hurriedly %s you!", Monnam(mtmp),
		       is_animal(mtmp->data)? "regurgitates" : "expels");*/
		pline("%s�ϵޤ��Ǥ��ʤ���%s������", Monnam(mtmp),
		       is_animal(mtmp->data)? "�Ǥ���" : "�ӽ�");
		expels(mtmp, mtmp->data, FALSE);
	} else if (!u.uswldtim || uasmon->msize >= MZ_HUGE) {
/*JP	    You("get %s!", is_animal(mtmp->data)? "regurgitated" : "expelled");*/
	    You("%s���줿��", is_animal(mtmp->data)? "�Ǥ���" : "�ӽ�");
	    if (flags.verbose && is_animal(mtmp->data))
/*JP		    pline("Obviously %s doesn't like your taste.",*/
		    You("�ɤ���%s���ߤ�̣����ʤ��褦����",
			   mon_nam(mtmp));
	    expels(mtmp, mtmp->data, FALSE);
	}
	return(1);
}

STATIC_OVL int
explmu(mtmp, mattk, ufound)	/* monster explodes in your face */
register struct monst *mtmp;
register struct attack  *mattk;
boolean ufound;
{
    if (mtmp->mcan) return(0);

    if (!ufound)
/*JP	pline("%s explodes at a spot in thin air!",
	      canseemon(mtmp) ? Monnam(mtmp) : "It");*/
	pline("%s�ϲ���ʤ��Ȥ������ȯ������",
	      canseemon(mtmp) ? Monnam(mtmp) : "���Ԥ�");
    else {
	register int tmp = d((int)mattk->damn, (int)mattk->damd);
	register boolean not_affected = defends((int)mattk->adtyp, uwep);

	hitmsg(mtmp, mattk);

	switch (mattk->adtyp) {
	    case AD_COLD:
		not_affected |= Cold_resistance;

		if (!not_affected) {
		    if (ACURR(A_DEX) > rnd(20)) {
/*JP			You("duck some of the blast.");*/
			You("�����򤵤�����");
			tmp = (tmp+1) / 2;
		    } else {
/*JP		        if (flags.verbose) You("get blasted!");*/
		        if (flags.verbose) You("�����򤯤�ä���");
		    }
		    if (Half_physical_damage) tmp = (tmp+1) / 2;
		    mdamageu(mtmp, tmp);
		}
		break;

	    case AD_BLND:
		not_affected = resists_blnd(&youmonst);
		if (!not_affected) {
		    /* sometimes you're affected even if it's invisible */
		    if (mon_visible(mtmp) || (rnd(tmp /= 2) > u.ulevel)) {
/*JP			You("are blinded by a blast of light!");*/
			You("�ޤФ椤�����ܤ���������");
			make_blinded((long)tmp, FALSE);
		    } else
			if (flags.verbose)
/*JP			You("get the impression it was not terribly bright.");*/
			You("����϶�����������ʤ��Ȼפä���");
		}
		break;

	    case AD_HALU:
		not_affected |= Blind ||
			(u.umonnum == PM_BLACK_LIGHT ||
			 u.umonnum == PM_VIOLET_FUNGUS ||
			 dmgtype(uasmon, AD_STUN));
		if (!not_affected) {
		    if (!Hallucination)
/*JP			You("are freaked by a blast of kaleidoscopic light!");*/
			You("���ڶ��θ��˿줤���줿��");
		    make_hallucinated(HHallucination + (long)tmp,FALSE,0L);
		}
		break;

	    default:
		break;
	}
	if (not_affected) {
/*JP	    You("seem unaffected by it.");*/
	    You("�ƶ�������ʤ��褦����");
	    ugolemeffects((int)mattk->adtyp, tmp);
	}
    }
    mondead(mtmp);
    if (mtmp->mhp > 0) return(0);
    return(2);	/* it dies */
}

int
gazemu(mtmp, mattk)	/* monster gazes at you */
	register struct monst *mtmp;
	register struct attack  *mattk;
{
	switch(mattk->adtyp) {
	    case AD_STON:
		if (mtmp->mcan) {
		    if (mtmp->data == &mons[PM_MEDUSA] && canseemon(mtmp))
/*JP			pline("%s doesn't look all that ugly.", Monnam(mtmp));*/
		        You("%s�Ϥ���ۤɽ����ʤ����Ȥ˵����Ĥ�����", mon_nam(mtmp));
		    break;
		}
		if(Reflecting && m_canseeu(mtmp) &&
		   !mtmp->mcan && mtmp->data == &mons[PM_MEDUSA]) {
		    if(!Blind) {
			if(Reflecting & W_AMUL)
			    makeknown(AMULET_OF_REFLECTION);
			else
			    makeknown(SHIELD_OF_REFLECTION);
/*JP			pline("%s gaze is reflected by your %s.",
			      s_suffix(Monnam(mtmp)),
			      (Reflecting & W_AMUL) ?
			      "medallion" : "shield");*/
			pline("%s�Τˤ�ߤ�%s��ȿ�ͤ��줿��",
			      s_suffix(Monnam(mtmp)),
			      (Reflecting & W_AMUL) ?
			      "�����" : "��");
			pline("%s���Фˤʤä���", Monnam(mtmp));
/*JP	if (mon_reflects(mtmp, "The gaze is reflected away by %s %s!"))*/
	if (mon_reflects(mtmp, "�ˤ�ߤ�%s��%s�ˤ�ä�ȿ�ͤ��줿��"))
			    break;
/*JP			pline("%s is turned to stone!", Monnam(mtmp));*/
			pline("%s���Фˤʤä���", Monnam(mtmp));
		    }
		    stoned = TRUE;
		    killed(mtmp);

		    if (mtmp->mhp > 0) break;
		    return 2;
		}
		if (canseemon(mtmp) && !resists_ston(&youmonst)) {
			char jbuf[BUFSZ];
/*JP			You("look upon %s.", mon_nam(mtmp));*/
			You("%s�򸫤���", mon_nam(mtmp));
			if(poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM))
			    break;
/*JP			You("turn to stone...");*/
			You("�в�����������");
			killer_format = KILLED_BY;
/*JP			killer = mons[PM_MEDUSA].mname;*/
			Strcpy(jbuf, jtrns_mon(mons[PM_MEDUSA].mname, -1));
			Strcat(jbuf, "�Τˤ�ߤ�");
			killer = jbuf;
			done(STONING);
		}
		break;
	    case AD_CONF:
		if(!mtmp->mcan && canseemon(mtmp) && mtmp->mcansee &&
					!mtmp->mspec_used && rn2(5)) {
		    int conf = d(3,4);

		    mtmp->mspec_used = mtmp->mspec_used + (conf + rn2(6));
		    if(!Confusion)
/*JP			pline("%s gaze confuses you!",*/
			pline("%s�Τˤ�ߤǤ��ʤ��Ϻ��𤷤���",
			                  s_suffix(Monnam(mtmp)));
		    else
/*JP			You("are getting more and more confused.");*/
			You("�ޤ��ޤ����𤷤���");
		    make_confused(HConfusion + conf, FALSE);
		}
		break;
	    case AD_STUN:
		if(!mtmp->mcan && canseemon(mtmp) && mtmp->mcansee &&
					!mtmp->mspec_used && rn2(5)) {
		    int stun = d(2,6);

		    mtmp->mspec_used = mtmp->mspec_used + (stun + rn2(6));
		    make_stunned(HStun + stun, TRUE);
/*JP		    pline("%s stares piercingly at you!", Monnam(mtmp));*/
		    pline("%s���䤤�ޤʤ����򤢤ʤ��˸�������", Monnam(mtmp));
		}
		break;
	    case AD_BLND:
		if (!mtmp->mcan && canseemon(mtmp) && !resists_blnd(&youmonst)
			&& distu(mtmp->mx,mtmp->my) <= BOLT_LIM*BOLT_LIM) {
		    int blnd = d((int)mattk->damn, (int)mattk->damd);
		    make_blinded((long)blnd,FALSE);
		    make_stunned((long)d(1,3),TRUE);
/*JP		    You("are blinded by %s radiance!",*/
		    You("%s�θ����ܤ������ʤ��ʤä���",
			              s_suffix(mon_nam(mtmp)));
		}
		break;
#ifdef PM_BEHOLDER /* work in progress */
	    case AD_SLEE:
		if(multi >= 0 && !rn2(5) && !Sleep_resistance) {
		    fall_asleep(-rnd(10), TRUE);
/*JP		    pline("%s gaze makes you very sleepy...",*/
		    pline("%s�Τˤ�ߤǤ��ʤ���̲���ʤä�������",
			  s_suffix(Monnam(mtmp)));
		}
		break;
	    case AD_SLOW:
		if((Fast & (INTRINSIC|TIMEOUT)) &&
					!defends(AD_SLOW, uwep) && !rn2(4))
		    u_slow_down();
		break;
#endif
	    default: impossible("Gaze attack %d?", mattk->adtyp);
		break;
	}
	return(0);
}

#endif /* OVLB */
#ifdef OVL1

void
mdamageu(mtmp, n)	/* mtmp hits you for n points damage */
register struct monst *mtmp;
register int n;
{
	flags.botl = 1;
	if (u.mtimedone) {
		u.mh -= n;
		if (u.mh < 1) rehumanize();
	} else {
		u.uhp -= n;
		if(u.uhp < 1) done_in_by(mtmp);
	}
}

#endif /* OVL1 */
#ifdef OVLB

STATIC_OVL void
urustm(mon, obj)
register struct monst *mon;
register struct obj *obj;
{
	boolean vis;

	if (!mon || !obj) return; /* just in case */
	vis = cansee(mon->mx, mon->my);
	if (u.umonnum == PM_RUST_MONSTER &&
	    is_rustprone(obj) && obj->oeroded < MAX_ERODE) {
		if (obj->greased || obj->oerodeproof || (obj->blessed && rn2(3))) {
		    if (vis)
/*JP			pline("Somehow, %s weapon is not affected.",*/
			pline("�ɤ������櫓����%s�����ϱƶ��򤦤��ʤ���",
						s_suffix(mon_nam(mon)));
		    if (obj->greased && !rn2(2)) obj->greased = 0;
		} else {
		    if (vis)
/*JP			pline("%s %s%s!",
			        s_suffix(Monnam(mon)), aobjnam(obj, "rust"),
			        obj->oeroded ? " further" : "");*/
			pline("%s��%s��%s���Ӥ���",
			        s_suffix(Monnam(mon)), xname(obj),
			        obj->oeroded ? "�����" : "");
		    obj->oeroded++;
		}
	}
}

#endif /* OVLB */
#ifdef OVL1

int
could_seduce(magr,mdef,mattk)
struct monst *magr, *mdef;
struct attack *mattk;
/* returns 0 if seduction impossible,
 *	   1 if fine,
 *	   2 if wrong gender for nymph */
{
	register struct permonst *pagr;
	boolean agrinvis, defperc;
	xchar genagr, gendef;

	if(magr == &youmonst) {
		pagr = uasmon;
		agrinvis = (Invis != 0);
		genagr = poly_gender();
	} else {
		pagr = magr->data;
		agrinvis = magr->minvis;
		genagr = gender(magr);
	}
	if(mdef == &youmonst) {
		defperc = (See_invisible != 0);
		gendef = poly_gender();
	} else {
		defperc = perceives(mdef->data);
		gendef = gender(mdef);
	}

	if(agrinvis && !defperc
#ifdef SEDUCE
		&& mattk && mattk->adtyp != AD_SSEX
#endif
		)
		return 0;

	if(pagr->mlet != S_NYMPH
		&& ((pagr != &mons[PM_INCUBUS] && pagr != &mons[PM_SUCCUBUS])
#ifdef SEDUCE
		    || (mattk && mattk->adtyp != AD_SSEX)
#endif
		   ))
		return 0;
	
	if(genagr == 1 - gendef)
		return 1;
	else
		return (pagr->mlet == S_NYMPH) ? 2 : 0;
}

#endif /* OVL1 */
#ifdef OVLB

#ifdef SEDUCE
/* Returns 1 if monster teleported */
int
doseduce(mon)
register struct monst *mon;
{
	register struct obj *ring, *nring;
	boolean fem = (mon->data == &mons[PM_SUCCUBUS]); /* otherwise incubus */
	char qbuf[QBUFSZ];

	if (mon->mcan || mon->mspec_used) {
/*JP		pline("%s acts as though %s has got a %sheadache.",
		      Monnam(mon), he[pronoun_gender(mon)],
		      mon->mcan ? "severe " : "");*/
	  pline("%s��%sƬ���ˤ��դ�򤷤���",
		      Monnam(mon),
		      mon->mcan ? "�Ҥɤ�" : "");
		return 0;
	}

	if (unconscious()) {
/*JP		pline("%s seems dismayed at your lack of response.",*/
		pline("%s���ֻ����ʤ��Τǵ����न���褦����",
		      Monnam(mon));
		return 0;
	}

/*JP	if (Blind) pline("It caresses you...");
	else You_feel("very attracted to %s.", mon_nam(mon));*/
	if (Blind) pline("���Ԥ��Ϥ��ʤ�����������Ƥ롥����");
	else You("%s�˰����Ĥ����Ƥ�褦�ʵ���������", mon_nam(mon));

	for(ring = invent; ring; ring = nring) {
	    nring = ring->nobj;
	    if (ring->otyp != RIN_ADORNMENT) continue;
	    if (fem) {
		if (rn2(20) < ACURR(A_CHA)) {
/*JP		    Sprintf(qbuf, "\"That %s looks pretty.  May I have it?\"",*/
		    Sprintf(qbuf, "�֤ʤ����Ũ��%s�Ǥ��礦���錄���ˤ���ޤ��󡩡�",
			xname(ring));
		    makeknown(RIN_ADORNMENT);
		    if (yn(qbuf) == 'n') continue;
/*JP		} else pline("%s decides she'd like your %s, and takes it.",
			Blind ? "She" : Monnam(mon), xname(ring));*/
		} else pline("%s��%s���ȤƤⵤ�ˤ��äơ�������ꤢ������",
			Blind ? "���" : Monnam(mon), xname(ring));
		makeknown(RIN_ADORNMENT);
		if (ring==uleft || ring==uright) Ring_gone(ring);
		if (ring==uwep) setuwep((struct obj *)0);
		freeinv(ring);
		mpickobj(mon,ring);
	    } else {
		char buf[BUFSZ];

		if (uleft && uright && uleft->otyp == RIN_ADORNMENT
				&& uright->otyp==RIN_ADORNMENT)
			break;
		if (ring==uleft || ring==uright) continue;
		if (rn2(20) < ACURR(A_CHA)) {
/*JP		    Sprintf(qbuf,"\"That %s looks pretty.  Would you wear it for me?\"",*/
		    Sprintf(qbuf,"�֤���ʤ�Ƥ��Ф餷��%s������Τ���˻ؤˤϤ�Ƥ���ʤ���������",
			xname(ring));
		    makeknown(RIN_ADORNMENT);
		    if (yn(qbuf) == 'n') continue;
		} else {
/*JP		    pline("%s decides you'd look prettier wearing your %s,",
			Blind ? "He" : Monnam(mon), xname(ring));
		    pline("and puts it on your finger.");*/
		    pline("%s��%s��Ĥ������ʤ������̥��Ū���ȹͤ���",
			Blind ? "��" : Monnam(mon), xname(ring));
		    pline("���ʤ��λؤˤ����Ϥ᤿��");
		}
		makeknown(RIN_ADORNMENT);
		if (!uright) {
/*JP		    pline("%s puts %s on your right hand.",
			Blind ? "He" : Monnam(mon), the(xname(ring)));*/
		    pline("%s��%s�򤢤ʤ��α���ˤϤ᤿��",
			Blind ? "��" : Monnam(mon), the(xname(ring)));
		    setworn(ring, RIGHT_RING);
		} else if (!uleft) {
/*JP		    pline("%s puts %s on your left hand.",
			Blind ? "He" : Monnam(mon), the(xname(ring)));*/
		    pline("%s��%s�򤢤ʤ��κ���ˤϤ᤿��",
			Blind ? "��" : Monnam(mon), the(xname(ring)));
		    setworn(ring, LEFT_RING);
		} else if (uright && uright->otyp != RIN_ADORNMENT) {
		    Strcpy(buf, xname(uright));
/*JP		    pline("%s replaces your %s with your %s.",
			Blind ? "He" : Monnam(mon), buf, xname(ring));*/
		    pline("%s��%s��%s�ˤȤ꤫������",
			Blind ? "��" : Monnam(mon), buf, xname(ring));
		    Ring_gone(uright);
		    setworn(ring, RIGHT_RING);
		} else if (uleft && uleft->otyp != RIN_ADORNMENT) {
		    Strcpy(buf, xname(uleft));
/*JP		    pline("%s replaces your %s with your %s.",
			Blind ? "He" : Monnam(mon), buf, xname(ring));*/
		    pline("%s��%s��%s�ˤȤ꤫������",
			Blind ? "��" : Monnam(mon), buf, xname(ring));
		    Ring_gone(uleft);
		    setworn(ring, LEFT_RING);
		} else impossible("ring replacement");
		Ring_on(ring);
		prinv((char *)0, ring, 0L);
	    }
	}

	if (!uarmc && !uarmf && !uarmg && !uarms && !uarmh
#ifdef TOURIST
								&& !uarmu
#endif
									)
/*JP		pline("%s murmurs sweet nothings into your ear.",
			Blind ? (fem ? "She" : "He") : Monnam(mon));*/
		pline("%s�Ϥ��ʤ��μ���ȤǴŤ������䤭��Ĥ֤䤤����",
			Blind ? (fem ? "���" : "��") : Monnam(mon));
	else
/*JP		pline("%s murmurs in your ear, while helping you undress.",
			Blind ? (fem ? "She" : "He") : Monnam(mon));*/
		pline("%s�ϼ���ȤǤ��ʤ�������æ�����ʤ��餵���䤤��",
			Blind ? (fem ? "���" : "��") : Monnam(mon));
/*JP	mayberem(uarmc, "cloak");*/
	mayberem(uarmc, "������");
	if(!uarmc)
/*JP		mayberem(uarm, "suit");*/
		mayberem(uarm, "������");
/*JP	mayberem(uarmf, "boots");*/
	mayberem(uarmf, "�֡���");
	if(!uwep || !welded(uwep))
/*JP		mayberem(uarmg, "gloves");*/
		mayberem(uarmg, "����");
/*JP	mayberem(uarms, "shield");
	mayberem(uarmh, "helmet");*/
	mayberem(uarms, "��");
	mayberem(uarmh, "��");
#ifdef TOURIST
	if(!uarmc && !uarm)
/*JP		mayberem(uarmu, "shirt");*/
		mayberem(uarmu, "�����");
#endif

	if (uarm || uarmc) {
/*JP		verbalize("You're such a %s; I wish...",
				flags.female ? "sweet lady" : "nice guy");*/
		verbalize( flags.female ? "���㡼�ߥ󥰤��������Ȥ����Τ�" : "���Ƥ����������Ȥ����Τ�");
		if (!tele_restrict(mon)) rloc(mon);
		return 1;
	}
	if (u.ualign.type == A_CHAOTIC)
		adjalign(1);

	/* by this point you have discovered mon's identity, blind or not... */


/*JP	pline("Time stands still while you and %s lie in each other's arms...",*/
	pline("���ʤ���%s�������礦�֡����ϻߤޤä��褦���ä�������",
		mon_nam(mon));
	if (rn2(35) > ACURR(A_CHA) + ACURR(A_INT)) {
		/* Don't bother with mspec_used here... it didn't get tired! */
/*JP		pline("%s seems to have enjoyed it more than you...",*/
		pline("%s�Ϥ��ʤ���ꤺ�äȳڤ�����褦��������",
		      Monnam(mon));
		switch (rn2(5)) {
/*JP			case 0: You_feel("drained of energy.");*/
			case 0: You("���Ϥ����פ����褦�ʵ���������");
				u.uen = 0;
				u.uenmax -= rnd(Half_physical_damage ? 5 : 10);
			        exercise(A_CON, FALSE);
				if (u.uenmax < 0) u.uenmax = 0;
				break;
/*JP			case 1: You("are down in the dumps.");*/
			case 1: You("�յ�����������");
				(void) adjattrib(A_CON, -1, TRUE);
			        exercise(A_CON, FALSE);
				flags.botl = 1;
				break;
/*JP			case 2: Your("senses are dulled.");*/
			case 2: Your("�޴����ߤä���");
				(void) adjattrib(A_WIS, -1, TRUE);
			        exercise(A_WIS, FALSE);
				flags.botl = 1;
				break;
			case 3:
				if (!resists_drli(&youmonst)) {
/*JP				    You_feel("out of shape.");*/
				    You("�����Ӥ줿��");
				    losexp();
				    if(u.uhp <= 0) {
					killer_format = KILLED_BY;
/*JP					killer = "overexertion";*/
					killer = "��ϫ��";
					done(DIED);
				    }
				} else {
/*JP				    You("have a curious feeling...");*/
				    You("�Ѥʴ���������������");
				}
				break;
			case 4: {
				int tmp;
/*JP				You_feel("exhausted.");*/
				You("���Ϥ��Ԥ����褦�ʵ���������");
			        exercise(A_STR, FALSE);
				tmp = rn1(10, 6);
				if(Half_physical_damage) tmp = (tmp+1) / 2;
/*JP				losehp(tmp, "exhaustion", KILLED_BY);*/
				losehp(tmp, "���ϤλȤ�������", KILLED_BY);
				break;
			}
		}
	} else {
		mon->mspec_used = rnd(100); /* monster is worn out */
/*JP		You("seem to have enjoyed it more than %s...", mon_nam(mon));*/
		You("%s����ڤ���Ǥ���褦��������", mon_nam(mon));
		switch (rn2(5)) {
/*JP			case 0: You_feel("raised to your full potential.");*/
			case 0: You("��ĺ��ã������");
			        exercise(A_CON, TRUE);
				u.uen = (u.uenmax += rnd(5));
				break;
/*JP			case 1: You_feel("good enough to do it again.");*/
			case 1: You("�ޤ��Ǥ���Ȼפä���");
				(void) adjattrib(A_CON, 1, TRUE);
			        exercise(A_CON, TRUE);
				flags.botl = 1;
				break;
/*JP			case 2: You("will always remember %s...", mon_nam(mon));*/
			case 2: You("���ĤޤǤ�%s��Ф��Ƥ����������", mon_nam(mon));
				(void) adjattrib(A_WIS, 1, TRUE);
			        exercise(A_WIS, TRUE);
				flags.botl = 1;
				break;
/*JP			case 3: pline("That was a very educational experience.");*/
			case 3: pline("�����и��Τ�����");
				pluslvl();
			        exercise(A_WIS, TRUE);
				break;
/*JP			case 4: You_feel("restored to health!");*/
			case 4: You("�ȤƤ�򹯤ˤʤä���");
				u.uhp = u.uhpmax;
				if (u.mtimedone) u.mh = u.mhmax;
			        exercise(A_STR, TRUE);
				flags.botl = 1;
				break;
		}
	}

	if (mon->mtame) /* don't charge */ ;
	else if (rn2(20) < ACURR(A_CHA)) {
/*JP		pline("%s demands that you pay %s, but you refuse...",
			Monnam(mon), him[fem]);*/
		pline("%s�Ϥ��ʤ��˶��ʧ���褦�׵ᤷ�������������ʤ��ϵ���������",
			Monnam(mon));

	} else if (u.umonnum == PM_LEPRECHAUN)
/*JP		pline("%s tries to take your money, but fails...",*/
		pline("%s�϶�����Ȥ����������Ԥ���������",
				Monnam(mon));
	else {
		long cost;

		if (u.ugold > (long)LARGEST_INT - 10L)
			cost = (long) rnd(LARGEST_INT) + 500L;
		else
			cost = (long) rnd((int)u.ugold + 10) + 500L;
		if (mon->mpeaceful) {
			cost /= 5L;
			if (!cost) cost = 1L;
		}
		if (cost > u.ugold) cost = u.ugold;
/*JP		if (!cost) verbalize("It's on the house!");*/
		if (!cost) verbalize("����Ϥ�����%s��",fem?"��":"��");
		else {
/*JP		    pline("%s takes %ld zorkmid%s for services rendered!",
			    Monnam(mon), cost, plur(cost));*/
		    pline("%s�ϥ����ӥ����Ȥ���%ld������ɼ����Ȥä���",
			    Monnam(mon), cost);
		    u.ugold -= cost;
		    mon->mgold += cost;
		    flags.botl = 1;
		}
	}
	if (!rn2(25)) mon->mcan = 1; /* monster is worn out */
	if (!tele_restrict(mon)) rloc(mon);
	return 1;
}

static void
mayberem(obj, str)
register struct obj *obj;
const char *str;
{
	char qbuf[QBUFSZ];

	if (!obj || !obj->owornmask) return;

	if (rn2(20) < ACURR(A_CHA)) {
/*JP
		Sprintf(qbuf,"\"Shall I remove your %s, %s?\"",
			str,
			(!rn2(2) ? "lover" : !rn2(2) ? "dear" : "sweetheart"));*/
		Sprintf(qbuf,"��%s���äƤ�����%s����",
			str,
			(!rn2(2) ? "�ͤ�" : flags.female ? "�ϥˡ�" : "�������" ));

		if (yn(qbuf) == 'n') return;
	} else {
		char hairbuf[BUFSZ];

/*JP		Sprintf(hairbuf,"let me run run my fingers through your %s",*/
		Sprintf(hairbuf,
			flags.female ? "�ʤ������%s�ʤ��" : "�����ä���ʤ��ʤ�����������ʤ�",
			body_part(HAIR));
#if 0 /*JP*/
		verbalize("Take off your %s; %s.", str,
			(obj == uarm)  ? "let's get a little closer" :
			(obj == uarmc || obj == uarms) ? "it's in the way" :
			(obj == uarmf) ? "let me rub your feet" :
			(obj == uarmg) ? "they're too clumsy" :
#ifdef TOURIST
			(obj == uarmu) ? "let me massage you" :
#endif
			/* obj == uarmh */
			hairbuf);
#endif /*JP*/

		verbalize("%s��æ���ǡ�����%s��", str,
			(obj == uarm)  ? "����äȴ�ꤽ�ä�" :
			(obj == uarmc || obj == uarms) ? "��������" :
			(obj == uarmf) ?(flags.female ? "����­����" : "���դá������ޤ���­��") :
			(obj == uarmg) ?(flags.female ? "�ʤ����Ũ�ʼ��" : "�����ޤ����Ӥ�") :
#ifdef TOURIST
			(obj == uarmu) ? "���ʤ��ؤ��λפ�����������" :
#endif
			/* obj == uarmh */
			hairbuf);
	}
	if (donning(obj)) cancel_don();
	if (obj == uarm)  (void) Armor_off();
	else if (obj == uarmc) (void) Cloak_off();
	else if (obj == uarmf) (void) Boots_off();
	else if (obj == uarmg) (void) Gloves_off();
	else if (obj == uarmh) (void) Helmet_off();
	else setworn((struct obj *)0, obj->owornmask & W_ARMOR);
}
#endif  /* SEDUCE */

#endif /* OVLB */

#ifdef OVL1

static int
passiveum(olduasmon,mtmp,mattk)
struct permonst *olduasmon;
register struct monst *mtmp;
register struct attack *mattk;
{
	int i, tmp;

	for(i = 0; ; i++) {
	    if(i >= NATTK) return 1;
	    if(olduasmon->mattk[i].aatyp == AT_NONE) break;
	}
	if (olduasmon->mattk[i].damn)
	    tmp = d((int)olduasmon->mattk[i].damn,
				    (int)olduasmon->mattk[i].damd);
	else if(olduasmon->mattk[i].damd)
	    tmp = d((int)olduasmon->mlevel+1, (int)olduasmon->mattk[i].damd);
	else
	    tmp = 0;

	/* These affect the enemy even if you were "killed" (rehumanized) */
	switch(olduasmon->mattk[i].adtyp) {
	    case AD_ACID:
		if (!rn2(2)) {
/*JP		    pline("%s is splashed by your acid!", Monnam(mtmp));*/
		    pline("%s�Ϥ��ʤ��λ��򤯤�ä���", Monnam(mtmp));
		    if (resists_acid(mtmp)) {
/*JP			pline("%s is not affected.", Monnam(mtmp));*/
			pline("%s�ϱƶ��򤦤��ʤ���", Monnam(mtmp));
			tmp = 0;
		    }
		} else tmp = 0;
		goto assess_dmg;
	    case AD_STON: /* cockatrice */
		if (!resists_ston(mtmp) &&
		    (mattk->aatyp != AT_WEAP || !MON_WEP(mtmp)) &&
		    mattk->aatyp != AT_GAZE && mattk->aatyp != AT_EXPL &&
		    mattk->aatyp != AT_MAGC &&
		    !(mtmp->misc_worn_check & W_ARMG)) {
		    if(poly_when_stoned(mtmp->data)) {
			mon_to_stone(mtmp);
			return (1);
		    }
/*JP		    pline("%s turns to stone!", Monnam(mtmp));*/
		    pline("%s���Фˤʤä���", Monnam(mtmp));
		    stoned = 1;
		    xkilled(mtmp, 0);
		    if (mtmp->mhp > 0) return 1;
		    return 2;
		}
		return 1;
	    default:
		break;
	}
	if (!u.mtimedone) return 1;

	/* These affect the enemy only if you are still a monster */
	if (rn2(3)) switch(uasmon->mattk[i].adtyp) {
	    case AD_PLYS: /* Floating eye */
		if (tmp > 127) tmp = 127;
		if (u.umonnum == PM_FLOATING_EYE) {
		    if (!rn2(4)) tmp = 127;
		    if (mtmp->mcansee && haseyes(mtmp->data) && rn2(3) &&
				(perceives(mtmp->data) || !Invis)) {
			if (Blind)
/*JP			    pline("As a blind %s, you cannot defend yourself.",
							uasmon->mname);*/
			    pline("%s���ܤ������ʤ��Τǡ��ɸ�Ǥ��ʤ���",
							jtrns_mon(uasmon->mname, flags.female));
		        else {
			    if (mon_reflects(mtmp,
/*JP					    "Your gaze is reflected by %s %s."))*/
					    "���ʤ��Τˤ�ߤ�%s��%s�ˤ�ä�ȿ�ͤ��줿��"))
				return 1;
/*JP			    pline("%s is frozen by your gaze!", Monnam(mtmp));*/
			    pline("%s�Ϥ��ʤ��Τˤ�ߤ�ư���ʤ���", Monnam(mtmp));
			    mtmp->mcanmove = 0;
			    mtmp->mfrozen = tmp;
			    return 3;
			}
		    }
		} else { /* gelatinous cube */
/*JP		    pline("%s is frozen by you.", Monnam(mtmp));*/
		    pline("%s��ư���ʤ���.", Monnam(mtmp));
		    mtmp->mcanmove = 0;
		    mtmp->mfrozen = tmp;
		    return 3;
		}
		return 1;
	    case AD_COLD: /* Brown mold or blue jelly */
		if (resists_cold(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
/*JP		    pline("%s is mildly chilly.", Monnam(mtmp));*/
		    pline("%s���䤨����", Monnam(mtmp));
		    golemeffects(mtmp, AD_COLD, tmp);
		    tmp = 0;
		    break;
		}
/*JP		pline("%s is suddenly very cold!", Monnam(mtmp));*/
		pline("%s���������Ť��ˤʤä���", Monnam(mtmp));
		u.mh += tmp / 2;
		if (u.mhmax < u.mh) u.mhmax = u.mh;
		if (u.mhmax > ((uasmon->mlevel+1) * 8)) {
			register struct monst *mon;

			if ((mon = cloneu()) != 0) {
			    mon->mhpmax = u.mhmax /= 2;
/*JP			    You("multiply from %s heat!",*/
			    You("%s��Ǯ��ʬ��������",
				           s_suffix(mon_nam(mtmp)));
			}
		}
		break;
	    case AD_STUN: /* Yellow mold */
		if (!mtmp->mstun) {
		    mtmp->mstun = 1;
/*JP		    pline("%s staggers.", Monnam(mtmp));*/
		    pline("%s�Ϥ��餯�餷����", Monnam(mtmp));
		}
		tmp = 0;
		break;
	    case AD_FIRE: /* Red mold */
		if (resists_fire(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
/*JP		    pline("%s is mildly warm.", Monnam(mtmp));*/
		    pline("%s���Ȥ����ʤä���", Monnam(mtmp));
		    golemeffects(mtmp, AD_FIRE, tmp);
		    tmp = 0;
		    break;
		}
/*JP		pline("%s is suddenly very hot!", Monnam(mtmp));*/
		pline("%s������Ǯ���ʤä���", Monnam(mtmp));
		break;
	    case AD_ELEC:
		if (resists_elec(mtmp)) {
		    shieldeff(mtmp->mx, mtmp->my);
/*JP		    pline("%s is slightly tingled.", Monnam(mtmp));*/
		    pline("%s�Ϥ���äȥԥ�ԥꤷ����", Monnam(mtmp));
		    golemeffects(mtmp, AD_ELEC, tmp);
		    tmp = 0;
		    break;
		}
/*JP		pline("%s is jolted with your electricity!", Monnam(mtmp));*/
		pline("%s���ŵ�����å��򤦤�����", Monnam(mtmp));
		break;
	    default: tmp = 0;
		break;
	}
	else tmp = 0;

    assess_dmg:
	if((mtmp->mhp -= tmp) <= 0) {
/*JP		pline("%s dies!", Monnam(mtmp));*/
		pline("%s�ϻ�����", Monnam(mtmp));
		xkilled(mtmp,0);
		if (mtmp->mhp > 0) return 1;
		return 2;
	}
	return 1;
}

#endif /* OVL1 */
#ifdef OVLB

#include "edog.h"
struct monst *
cloneu()
{
	register struct monst *mon;
	int mndx = monsndx(uasmon);

	if (u.mh <= 1) return(struct monst *)0;
	if (mvitals[mndx].mvflags & G_EXTINCT) return(struct monst *)0;
	uasmon->pxlth += sizeof(struct edog);
	mon = makemon(uasmon, u.ux, u.uy, NO_MM_FLAGS);
	uasmon->pxlth -= sizeof(struct edog);
	mon = christen_monst(mon, plname);
	initedog(mon);
	mon->m_lev = uasmon->mlevel;
	mon->mhp = u.mh /= 2;
	mon->mhpmax = u.mhmax;
	return(mon);
}

#endif /* OVLB */

/*mhitu.c*/
