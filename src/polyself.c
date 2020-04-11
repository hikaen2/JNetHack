/*	SCCS Id: @(#)polyself.c	3.3	1999/08/16	*/
/*	Copyright (C) 1987, 1988, 1989 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

/*
 * Polymorph self routine.
 *
 * Note:  the light source handling code assumes that both youmonst.m_id
 * and youmonst.mx will always remain 0 when it handles the case of the
 * player polymorphed into a light-emitting monster.
 */

#include "hack.h"

#ifdef OVLB
STATIC_DCL void FDECL(polyman, (const char *,const char *));
STATIC_DCL void NDECL(break_armor);
STATIC_DCL void FDECL(drop_weapon,(int));
STATIC_DCL void NDECL(skinback);
STATIC_DCL void NDECL(uunstick);
STATIC_DCL int FDECL(armor_to_dragon,(int));
STATIC_DCL void NDECL(newman);

/* update the youmonst.data structure pointer */
void
set_uasmon()
{
	set_mon_data(&youmonst, &mons[u.umonnum], 0);
}

/* make a (new) human out of the player */
STATIC_OVL void
polyman(fmt, arg)
const char *fmt, *arg;
{
	boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
		was_mimicking_gold = (youmonst.m_ap_type == M_AP_OBJECT
				      && youmonst.mappearance == GOLD_PIECE);
	boolean was_blind = !!Blind;

	if (Upolyd) {
		u.acurr = u.macurr;	/* restore old attribs */
		u.amax = u.mamax;
		u.umonnum = u.umonster;
		flags.female = u.mfemale;
	}
	set_uasmon();

	u.mh = u.mhmax = 0;
	u.mtimedone = 0;
	skinback();
	u.uundetected = 0;

	if (sticky) uunstick();
	find_ac();
	if (was_mimicking_gold) {
	    if (multi < 0) unmul("");
	} else {
	    /*
	     * Clear any in-progress imitations -- the case where not a
	     * mimic is handled above.
	     *
	     * Except, this is not complete if the hero ever gets the
	     * chance to imitate anything, then s/he may be mimicing
	     * gold, but not the way its done for eating a mimic.
	     */
	    youmonst.m_ap_type = M_AP_NOTHING;
	}
	newsym(u.ux,u.uy);

	You(fmt, arg);
	/* check whether player foolishly genocided self while poly'd */
	if ((mvitals[urole.malenum].mvflags & G_GENOD) ||
			(urole.femalenum != NON_PM &&
			(mvitals[urole.femalenum].mvflags & G_GENOD)) ||
			(mvitals[urace.malenum].mvflags & G_GENOD) ||
			(urace.femalenum != NON_PM &&
			(mvitals[urace.femalenum].mvflags & G_GENOD))) {
	    /* intervening activity might have clobbered genocide info */
/*JP	    if (!killer || !strstri(killer, "genocid")) {*/
	    if (!killer || !strstr(killer, "�Ի�") ||
		!strstri(killer, "genocid")) {
		killer_format = KILLED_BY;
/*JP		killer = "self-genocide";*/
		killer = "����Ū�Ի���";
	    }
	    done(GENOCIDED);
	}
	if (was_blind && !Blind) {	/* reverting from eyeless */
	    Blinded = 1L;
	    make_blinded(0L, TRUE);	/* remove blindness */
	}

	if(!Levitation && !u.ustuck &&
	   (is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy)))
		spoteffects();

	see_monsters();
}

void
change_sex()
{
	/* setting u.umonster for caveman/cavewoman or priest/priestess
	   swap unintentionally makes `Upolyd' appear to be true */
	boolean already_polyd = (boolean) Upolyd;

	flags.female = !flags.female;
	if (already_polyd)	/* poly'd: also change saved sex */
		u.mfemale = !u.mfemale;
	max_rank_sz();		/* [this appears to be superfluous] */
	if (flags.female && urole.name.f)
	    Strcpy(pl_character, urole.name.f);
	else
	    Strcpy(pl_character, urole.name.m);
	u.umonster = (flags.female && urole.femalenum != NON_PM) ?
			urole.femalenum : urole.malenum;
	if (!already_polyd) {
	    u.umonnum = u.umonster;
	} else if (u.umonnum == PM_SUCCUBUS || u.umonnum == PM_INCUBUS) {
	    /* change monster type to match new sex */
	    u.umonnum = (u.umonnum == PM_SUCCUBUS) ? PM_INCUBUS : PM_SUCCUBUS;
	    set_uasmon();
	}
}

STATIC_OVL void
newman()
{
	int tmp, tmp2;

	if (!rn2(10)) change_sex();

	tmp = u.uhpmax;
	tmp2 = u.ulevel;
	u.ulevel = u.ulevel + rn1(5, -2);
	if (u.ulevel > 127 || u.ulevel < 1) u.ulevel = 1;
	if (u.ulevel > MAXULEV) u.ulevel = MAXULEV;
	if (u.ulevelmax < u.ulevel) u.ulevelmax = u.ulevel;

	adjabil(tmp2, (int)u.ulevel);
	reset_rndmonst(NON_PM);	/* new monster generation criteria */

	/* random experience points for the new experience level */
	u.uexp = rndexp();

	/* u.uhpmax * u.ulevel / tmp2: proportionate hit points to new level
	 * -10 and +10: don't apply proportionate HP to 10 of a starting
	 *   character's hit points (since a starting character's hit points
	 *   are not on the same scale with hit points obtained through level
	 *   gain)
	 * 9 - rn2(19): random change of -9 to +9 hit points
	 */
#ifndef LINT
	u.uhpmax = ((u.uhpmax - 10) * (long)u.ulevel / tmp2 + 10) +
		(9 - rn2(19));
#endif

#ifdef LINT
	u.uhp = u.uhp + tmp;
#else
	u.uhp = u.uhp * (long)u.uhpmax/tmp;
#endif

	tmp = u.uenmax;
#ifndef LINT
	u.uenmax = u.uenmax * (long)u.ulevel / tmp2 + 9 - rn2(19);
#endif
	if (u.uenmax < 0) u.uenmax = 0;
#ifndef LINT
	u.uen = (tmp ? u.uen * (long)u.uenmax / tmp : u.uenmax);
#endif

	redist_attr();
	u.uhunger = rn1(500,500);
	newuhs(FALSE);
	if (Sick) make_sick(0L, (char *) 0, FALSE, SICK_ALL);
	Stoned = 0;
	if (u.uhp <= 0 || u.uhpmax <= 0) {
		if (Polymorph_control) {
		    if (u.uhp <= 0) u.uhp = 1;
		    if (u.uhpmax <= 0) u.uhpmax = 1;
		} else {
/*JP		    Your("new form doesn't seem healthy enough to survive.");*/
		    Your("�������Ѥ������Ƥ��������Ϥ��ʤ��褦����");
		    killer_format = KILLED_BY_AN;
/*JP		    killer="unsuccessful polymorph";*/
		    killer="�Ѳ��μ��Ԥ�";
		    done(DIED);
		}
	}
/*JP	polyman("feel like a new %s!",
		(flags.female && urace.individual.f) ? urace.individual.f :
		(urace.individual.m) ? urace.individual.m : urace.noun);*/
	polyman("%s�Ȥ������줫��ä��褦�ʵ���������",
		jtrns_mon(
		    (flags.female && urace.individual.f) ? urace.individual.f :
		    (urace.individual.m) ? urace.individual.m : urace.noun, flags.female));
	flags.botl = 1;
	see_monsters();
	(void) encumber_msg();
}

void
polyself()
{
	char buf[BUFSZ];
	int old_light, new_light;
	int mntmp = NON_PM;
	int tries=0;
	boolean draconian = (uarm &&
				uarm->otyp >= GRAY_DRAGON_SCALE_MAIL &&
				uarm->otyp <= YELLOW_DRAGON_SCALES);
	boolean iswere = (u.ulycn >= LOW_PM || is_were(youmonst.data));
	boolean isvamp = (youmonst.data->mlet == S_VAMPIRE || u.umonnum == PM_VAMPIRE_BAT);

	if(!Polymorph_control && !draconian && !iswere && !isvamp) {
	    if (rn2(20) > ACURR(A_CON)) {
		You(shudder_for_moment);
/*JP		losehp(rnd(30), "system shock", KILLED_BY_AN);*/
		losehp(rnd(30), "�����ƥॷ��å���", KILLED_BY_AN);
		exercise(A_CON, FALSE);
		return;
	    }
	}
	old_light = Upolyd ? emits_light(youmonst.data) : 0;

	if (Polymorph_control) {
		do {
/*JP			getlin("Become what kind of monster? [type the name]",
				buf);
*/
			getlin("�ɤμ�β�ʪ�ˤʤ롩[�Ѹ�̾������Ƥ�]",
  				buf);
			mntmp = name_to_mon(buf);
			if (mntmp < LOW_PM)
/*JP				pline("I've never heard of such monsters.");*/
				pline("����ʲ�ʪ��ʹ�������Ȥ��ʤ���");
			/* Note:  humans are illegal as monsters, but an
			 * illegal monster forces newman(), which is what we
			 * want if they specified a human.... */
			else if (!polyok(&mons[mntmp]) && !your_race(&mons[mntmp]))
/*JP				You("cannot polymorph into that.");*/
				You("����ˤʤ뤳�ȤϤǤ��ʤ���");
			else break;
		} while(++tries < 5);
		if (tries==5) pline(thats_enough_tries);
		/* allow skin merging, even when polymorph is controlled */
		if (draconian &&
		    (mntmp == armor_to_dragon(uarm->otyp) || tries == 5))
		    goto do_merge;
	} else if (draconian || iswere || isvamp) {
		/* special changes that don't require polyok() */
		if (draconian) {
		    do_merge:
			mntmp = armor_to_dragon(uarm->otyp);
			if (!(mvitals[mntmp].mvflags & G_GENOD)) {
				/* allow G_EXTINCT */
/*JP				You("merge with your scaly armor.");*/
				You("�ڤγ��Ȱ��β�������");
				uskin = uarm;
				uarm = (struct obj *)0;
				/* save/restore hack */
				uskin->owornmask |= I_SPECIAL;
			}
		} else if (iswere) {
			if (is_were(youmonst.data))
				mntmp = PM_HUMAN; /* Illegal; force newman() */
			else
				mntmp = u.ulycn;
		} else {
			if (youmonst.data->mlet == S_VAMPIRE)
				mntmp = PM_VAMPIRE_BAT;
			else
				mntmp = PM_VAMPIRE;
		}
		if (polymon(mntmp))
			goto made_change;
	}

	if (mntmp < LOW_PM) {
		tries = 0;
		do {
			/* randomly pick an "ordinary" monster */
			mntmp = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
		} while((!polyok(&mons[mntmp]) || is_placeholder(&mons[mntmp]))
				&& tries++ < 200);
	}

	/* The below polyok() fails either if everything is genocided, or if
	 * we deliberately chose something illegal to force newman().
	 */
	if (!polyok(&mons[mntmp]) || !rn2(5))
		newman();
	else if(!polymon(mntmp)) return;

/*JP	if (!uarmg) selftouch("No longer petrify-resistant, you");*/
	if (!uarmg) selftouch("�⤦���в����ϤϤ��ʤ��ˤʤ�");

 made_change:
	new_light = Upolyd ? emits_light(youmonst.data) : 0;
	if (old_light != new_light) {
	    if (old_light)
		del_light_source(LS_MONSTER, (genericptr_t)&youmonst);
	    if (new_light == 1) ++new_light;  /* otherwise it's undetectable */
	    if (new_light)
		new_light_source(u.ux, u.uy, new_light,
				 LS_MONSTER, (genericptr_t)&youmonst);
	}
}

/* (try to) make a mntmp monster out of the player */
int
polymon(mntmp)	/* returns 1 if polymorph successful */
int	mntmp;
{
	boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
		was_blind = !!Blind, dochange = FALSE;
	int mlvl;

	if (mvitals[mntmp].mvflags & G_GENOD) {	/* allow G_EXTINCT */
/*JP		You_feel("rather %s-ish.",mons[mntmp].mname);*/
		You("%s�äݤ��ʤä��褦�ʵ�������",
		    jtrns_mon(mons[mntmp].mname, mntmp));
		exercise(A_WIS, TRUE);
		return(0);
	}

	/* KMH, conduct */
	u.uconduct.polyselfs++;

	if (!Upolyd) {
		/* Human to monster; save human stats */
		u.macurr = u.acurr;
		u.mamax = u.amax;
		u.mfemale = flags.female;
	} else {
		/* Monster to monster; restore human stats, to be
		 * immediately changed to provide stats for the new monster
		 */
		u.acurr = u.macurr;
		u.amax = u.mamax;
		flags.female = u.mfemale;
	}

	if (youmonst.m_ap_type == M_AP_OBJECT &&
		youmonst.mappearance == GOLD_PIECE) {
	    /* stop mimicking gold immediately */
	    if (multi < 0) unmul("");
	}
	if (is_male(&mons[mntmp])) {
		if(flags.female) dochange = TRUE;
	} else if (is_female(&mons[mntmp])) {
		if(!flags.female) dochange = TRUE;
	} else if (!is_neuter(&mons[mntmp]) && mntmp != u.ulycn) {
		if(!rn2(10)) dochange = TRUE;
	}
	if (dochange) {
		flags.female = !flags.female;
/*JP		You("%s %s %s!",
		    (u.umonnum != mntmp) ? "turn into a" : "feel like a new",
		    flags.female ? "female" : "male",
		    mons[mntmp].mname);*/
		You("%s��%s%s��",
		    flags.female ? "��" : "��",
		    jtrns_mon(mons[mntmp].mname, flags.female),
		    (u.umonnum != mntmp) ? "�ˤʤä���" : "�ˤʤä��褦�ʵ�������");
	} else {
		if (u.umonnum != mntmp)
/*JP			You("turn into %s!", an(mons[mntmp].mname));*/
			You("%s�ˤʤä���", 
			    jtrns_mon(mons[mntmp].mname, flags.female));
		else
/*JP			You_feel("like a new %s!", mons[mntmp].mname);*/
			You("�̤�%s�ˤʤä��褦�ʵ���������", 
			    jtrns_mon(mons[mntmp].mname, flags.female));
	}
	if (Stoned && poly_when_stoned(&mons[mntmp])) {
		/* poly_when_stoned already checked stone golem genocide */
/*JP		You("turn to stone!");*/
		You("�в�������");
		mntmp = PM_STONE_GOLEM;
		Stoned = 0;
	}

	u.mtimedone = rn1(500, 500);
	u.umonnum = mntmp;
	set_uasmon();

	/* New stats for monster, to last only as long as polymorphed.
	 * Currently only strength gets changed.
	 */
	if(strongmonst(&mons[mntmp])) ABASE(A_STR) = AMAX(A_STR) = STR18(100);

	if (Stone_resistance && Stoned) { /* parnes@eniac.seas.upenn.edu */
		Stoned = 0;
/*JP		You("no longer seem to be petrifying.");*/
		You("�в�����������줿�褦����");
	}
	if (Sick_resistance && Sick) {
		make_sick(0L, (char *) 0, FALSE, SICK_ALL);
/*JP		You("no longer feel sick.");*/
		You("�µ�����������줿�褦����");
	}
	if (nohands(youmonst.data)) Glib = 0;

	/*
	mlvl = adj_lev(&mons[mntmp]);
	 * We can't do the above, since there's no such thing as an
	 * "experience level of you as a monster" for a polymorphed character.
	 */
	mlvl = (int)mons[mntmp].mlevel;
	if (youmonst.data->mlet == S_DRAGON && mntmp >= PM_GRAY_DRAGON) {
		u.mhmax = In_endgame(&u.uz) ? (8*mlvl) : (4*mlvl + d(mlvl,4));
	} else if (is_golem(youmonst.data)) {
		u.mhmax = golemhp(mntmp);
	} else {
		if (!mlvl) u.mhmax = rnd(4);
		else u.mhmax = d(mlvl, 8);
		if (is_home_elemental(&mons[mntmp])) u.mhmax *= 3;
	}
	u.mh = u.mhmax;

	if (u.ulevel < mlvl) {
	/* Low level characters can't become high level monsters for long */
#ifdef DUMB
		/* DRS/NS 2.2.6 messes up -- Peter Kendell */
		int mtd = u.mtimedone, ulv = u.ulevel;

		u.mtimedone = mtd * ulv / mlvl;
#else
		u.mtimedone = u.mtimedone * u.ulevel / mlvl;
#endif
	}

	if (uskin && mntmp != armor_to_dragon(uskin->otyp))
		skinback();
	break_armor();
	drop_weapon(1);
	if (hides_under(youmonst.data))
		u.uundetected = OBJ_AT(u.ux, u.uy);
	else if (youmonst.data->mlet == S_EEL)
		u.uundetected = is_pool(u.ux, u.uy);
	else
		u.uundetected = 0;

	if (was_blind && !Blind) {	/* previous form was eyeless */
	    Blinded = 1L;
	    make_blinded(0L, TRUE);	/* remove blindness */
	}
	newsym(u.ux,u.uy);		/* Change symbol */

	if (!sticky && !u.uswallow && u.ustuck && sticks(youmonst.data)) u.ustuck = 0;
	else if (sticky && !sticks(youmonst.data)) uunstick();
#ifdef STEED
	if (u.usteed) {
	    if (touch_petrifies(u.usteed->data) &&
	    		!Stone_resistance && rnl(3)) {
	    	char buf[BUFSZ];

/*JP	    	pline("No longer petrifying-resistant, you touch %s.",
	    			mon_nam(u.usteed));
*/
	    	pline("�в��ؤ��������ʤ��Τ�%s�˿���Ƥ��ޤä���",
	    			mon_nam(u.usteed));
	    	Sprintf(buf, "%s�˾�ä�", a_monnam(u.usteed));
	    	instapetrify(buf);
 	    }
	    if (!can_ride(u.usteed)) dismount_steed(DISMOUNT_POLY);
	}
#endif

	if (flags.verbose) {
/*JP	    static const char use_thec[] = "Use the command #%s to %s.";
	    static const char monsterc[] = "monster";
*/
	    static const char use_thec[] = "#%s���ޥ�ɤ�%s���Ȥ��Ǥ��롥";
	    static const char monsterc[] = "monster";
	    if (can_breathe(youmonst.data))
/*JP		pline(use_thec,monsterc,"use your breath weapon");*/
		pline(use_thec,monsterc,"©���Ǥ�������");
	    if (attacktype(youmonst.data, AT_SPIT))
/*JP		pline(use_thec,monsterc,"spit venom");*/
		pline(use_thec,monsterc,"�Ǥ��Ǥ�");
	    if (youmonst.data->mlet == S_NYMPH)
/*JP		pline(use_thec,monsterc,"remove an iron ball");*/
		pline(use_thec,monsterc,"Ŵ���Ϥ���");
	    if (youmonst.data->mlet == S_UMBER)
/*JP		pline(use_thec,monsterc,"confuse monsters");*/
		pline(use_thec,monsterc,"��ʪ���𤵤���");
	    if (is_hider(youmonst.data))
/*JP		pline(use_thec,monsterc,"hide");*/
		pline(use_thec,monsterc,"�����");
	    if (is_were(youmonst.data))
/*JP		pline(use_thec,monsterc,"summon help");*/
		pline(use_thec,monsterc,"��֤򾤴�����");
	    if (webmaker(youmonst.data))
/*JP		pline(use_thec,monsterc,"spin a web");*/
		pline(use_thec,monsterc,"��������ĥ��");
	    if (u.umonnum == PM_GREMLIN)
/*JP		pline(use_thec,monsterc,"multiply in a fountain");*/
		pline(use_thec,monsterc,"�������ʬ������");
	    if (is_unicorn(youmonst.data))
/*JP		pline(use_thec,monsterc,"use your horn");*/
		pline(use_thec,monsterc,"�Ѥ�Ȥ�");
	    if (is_mind_flayer(youmonst.data))
/*JP		pline(use_thec,monsterc,"emit a mental blast");*/
		pline(use_thec,monsterc,"�����Ȥ�ȯ��������");
	    if (youmonst.data->msound == MS_SHRIEK) /* worthless, actually */
/*JP		pline(use_thec,monsterc,"shriek");*/
		pline(use_thec,monsterc,"���ڤ����򤢤���");
	    if (lays_eggs(youmonst.data) && flags.female)
/*JP		pline(use_thec,"sit","lay an egg");*/
		pline(use_thec,"�¤�","��򻺤�");
	}
	/* you now know what an egg of your type looks like */
	if (lays_eggs(youmonst.data)) {
	    learn_egg_type(u.umonnum);
	    /* make queen bees recognize killer bee eggs */
	    learn_egg_type(egg_type_from_parent(u.umonnum, TRUE));
	}
	find_ac();
	if((!Levitation && !u.ustuck && !Flying &&
	    (is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy))) ||
	   (Underwater && !Swimming))
	    spoteffects();
	if (Passes_walls && u.utrap && u.utraptype == TT_INFLOOR) {
	    u.utrap = 0;
/*JP	    pline_The("rock seems to no longer trap you.");*/
	    pline("����Ĥ�������뤳�ȤϤʤ�������");
	} else if (likes_lava(youmonst.data) && u.utrap && u.utraptype == TT_LAVA) {
	    u.utrap = 0;
/*JP	    pline_The("lava now feels soothing.");*/
	    pline("�ϴ䤬����������Ĥ����Ƥ���롥");
	}
	if (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data)) {
	    if (Punished) {
/*JP		You("slip out of the iron chain.");*/
		You("Ŵ�κ����餹����ȴ������");
		unpunish();
	    }
	}
	if (u.utrap && (u.utraptype == TT_WEB || u.utraptype == TT_BEARTRAP) &&
		(amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data) ||
		  (youmonst.data->msize <= MZ_SMALL && u.utraptype == TT_BEARTRAP))) {
/*JP	    You("are no longer stuck in the %s.",
		    u.utraptype == TT_WEB ? "web" : "bear trap");
*/
	    You("%s����æ�Ф�����",
		    u.utraptype == TT_WEB ? "�������" : "�����");
	    /* probably should burn webs too if PM_FIRE_ELEMENTAL */
	    u.utrap = 0;
	}
	if (youmonst.data->mlet == S_SPIDER && u.utrap && u.utraptype == TT_WEB) {
/*JP	    You("orient yourself on the web.");*/
	    You("�������ξ��Ŭ��������");
	    u.utrap = 0;
	}
	flags.botl = 1;
	vision_full_recalc = 1;
	see_monsters();
	exercise(A_CON, FALSE);
	exercise(A_WIS, TRUE);
	(void) encumber_msg();
	return(1);
}

STATIC_OVL void
break_armor()
{
    register struct obj *otmp;

    if (breakarm(youmonst.data)) {
	if ((otmp = uarm) != 0) {
		if (donning(otmp)) cancel_don();
/*JP		You("break out of your armor!");*/
		You("���������֤ä���");
		exercise(A_STR, FALSE);
		(void) Armor_gone();
		useup(otmp);
	}
	if ((otmp = uarmc) != 0) {
	    if(otmp->oartifact) {
/*JP		Your("cloak falls off!");*/
		Your("��������æ���������");
		(void) Cloak_off();
		dropx(otmp);
	    } else {
/*JP		Your("cloak tears apart!");*/
		Your("�������Ϥ��������˰��������줿��");
		(void) Cloak_off();
		useup(otmp);
	    }
	}
#ifdef TOURIST
	if (uarmu) {
/*JP		Your("shirt rips to shreds!");*/
		Your("����Ĥϰ��������줿��");
		useup(uarmu);
	}
#endif
    } else if (sliparm(youmonst.data)) {
	if ((otmp = uarm) != 0) {
		if (donning(otmp)) cancel_don();
/*JP		Your("armor falls around you!");*/
		Your("���Ϥ��ʤ��Τޤ����������");
		(void) Armor_gone();
		dropx(otmp);
	}
	if ((otmp = uarmc) != 0) {
		if (is_whirly(youmonst.data))
/*JP			Your("cloak falls, unsupported!");*/
			Your("�������Ϥ����ä��������");
/*JP		else You("shrink out of your cloak!");*/
		else You("����������̤߽Ф���");
		(void) Cloak_off();
		dropx(otmp);
	}
#ifdef TOURIST
	if ((otmp = uarmu) != 0) {
		if (is_whirly(youmonst.data))
/*JP			You("seep right through your shirt!");*/
			You("����Ĥ��餷�߽Ф���");
/*JP		else You("become much too small for your shirt!");*/
		else You("����Ĥ�ꤺ�äȾ������ʤä���");
		setworn((struct obj *)0, otmp->owornmask & W_ARMU);
		dropx(otmp);
	}
#endif
    }
    if (nohands(youmonst.data) || verysmall(youmonst.data)) {
	if ((otmp = uarmg) != 0) {
	    if (donning(otmp)) cancel_don();
	    /* Drop weapon along with gloves */
/*JP	    You("drop your gloves%s!", uwep ? " and weapon" : "");*/
	    You("����%s�������", uwep ? "�����" : "");
	    drop_weapon(0);
	    (void) Gloves_off();
	    dropx(otmp);
	}
	if ((otmp = uarms) != 0) {
/*JP	    You("can no longer hold your shield!");*/
	    You("�⤦�����äƤ��ʤ���");
	    (void) Shield_off();
	    dropx(otmp);
	}
	if ((otmp = uarmh) != 0) {
	    if (donning(otmp)) cancel_don();
/*JP	    Your("helmet falls to the %s!", surface(u.ux, u.uy));*/
	    Your("����%s���������", surface(u.ux, u.uy));
	    (void) Helmet_off();
	    dropx(otmp);
	}
    }
    if (nohands(youmonst.data) || verysmall(youmonst.data) ||
		slithy(youmonst.data) || youmonst.data->mlet == S_CENTAUR) {
	if ((otmp = uarmf) != 0) {
	    if (donning(otmp)) cancel_don();
	    if (is_whirly(youmonst.data))
/*JP		Your("boots fall away!");*/
		Your("����æ���������");
/*JP	    else Your("boots %s off your feet!",
			verysmall(youmonst.data) ? "slide" : "are pushed");
*/
	    else Your("���Ϥ��ʤ���­����%s",
			verysmall(youmonst.data) ? "��������" : "æ�������");
	    (void) Boots_off();
	    dropx(otmp);
	}
    }
}

STATIC_OVL void
drop_weapon(alone)
int alone;
{
    struct obj *otmp;
    if ((otmp = uwep) != 0) {
	/* !alone check below is currently superfluous but in the
	 * future it might not be so if there are monsters which cannot
	 * wear gloves but can wield weapons
	 */
	if (!alone || cantwield(youmonst.data)) {
	    struct obj *wep = uwep;

/*JP	    if (alone) You("find you must drop your weapon%s!",
			   	u.twoweap ? "s" : "");
*/
	    if (alone) You("����������Ȥ˵����Ĥ�����");
	    uwepgone();
	    if (!wep->cursed || wep->otyp != LOADSTONE)
		dropx(otmp);
		untwoweapon();
	}
    }
}

void
rehumanize()
{
	/* You can't revert back while unchanging */
	if (Unchanging && (u.mh < 1)) {
		killer_format = NO_KILLER_PREFIX;
/*JP		killer = "killed while stuck in creature form";*/
		killer = "���λѤ���줺��";
		done(DIED);
	}

	if (emits_light(youmonst.data))
	    del_light_source(LS_MONSTER, (genericptr_t)&youmonst);
/*JP	polyman("return to %s form!", urace.adj);*/
	polyman("%s����ä���", urace.j);

	if (u.uhp < 1) {
	    char kbuf[256];

/*JP	    Sprintf(kbuf, "reverting to unhealthy %s form", urace.adj);*/
	    Sprintf(kbuf, "�Է򹯤�%s�λѤ���ä�", urace.j);
	    killer_format = KILLED_BY;
	    killer = kbuf;
	    done(DIED);
	}
/*JP	if (!uarmg) selftouch("No longer petrify-resistant, you");*/
	if (!uarmg) selftouch("�в��ؤ����Ϥ��ʤ��ʤäơ����ʤ���");
	nomul(0);

	flags.botl = 1;
	vision_full_recalc = 1;
	(void) encumber_msg();
}

int
dobreathe()
{
	if (Strangled) {
/*JP	    You_cant("breathe.  Sorry.");*/
	    You_cant("©���Ǥ����Ȥ��Ǥ��ʤ�����ǰ��");
	    return(0);
	}
	if (u.uen < 15) {
/*JP	    You("don't have enough energy to breathe!");*/
	    You("ͭ�Ǥ��礭�ʤ��äפ򤷤���");
	    return(0);
	}
	u.uen -= 15;
	flags.botl = 1;

	if (!getdir((char *)0)) return(0);
	else {
	    register struct attack *mattk;
	    register int i;

	    for(i = 0; i < NATTK; i++) {
		mattk = &(youmonst.data->mattk[i]);
		if(mattk->aatyp == AT_BREA) break;
	    }
	    buzz((int) (20 + mattk->adtyp-1), (int)mattk->damn,
		u.ux, u.uy, u.dx, u.dy);
	}
	return(1);
}

int
dospit()
{
	struct obj *otmp;

	if (!getdir((char *)0)) return(0);
	otmp = mksobj(u.umonnum==PM_COBRA ? BLINDING_VENOM : ACID_VENOM,
			TRUE, FALSE);
	otmp->spe = 1; /* to indicate it's yours */
	throwit(otmp);
	return(1);
}

int
doremove()
{
	if (!Punished) {
/*JP		You("are not chained to anything!");*/
		You("����Ĥʤ���Ƥ��ʤ���");
		return(0);
	}
	unpunish();
	return(1);
}

int
dospinweb()
{
	register struct trap *ttmp = t_at(u.ux,u.uy);

	if (Levitation || Is_airlevel(&u.uz)
	    || Underwater || Is_waterlevel(&u.uz)) {
/*JP		You("must be on the ground to spin a web.");*/
		You("��������ĥ��ˤ����̤ξ�ˤ��ʤ��ƤϤʤ�ʤ���");
		return(0);
	}
	if (u.uswallow) {
/*JP		You("release web fluid inside %s.", mon_nam(u.ustuck));*/
		You("%s������������Ǥ��Ф�����", mon_nam(u.ustuck));
		if (is_animal(u.ustuck->data)) {
			expels(u.ustuck, u.ustuck->data, TRUE);
			return(0);
		}
		if (is_whirly(u.ustuck->data)) {
			int i;

			for (i = 0; i < NATTK; i++)
				if (u.ustuck->data->mattk[i].aatyp == AT_ENGL)
					break;
			if (i == NATTK)
			       impossible("Swallower has no engulfing attack?");
			else {
				char sweep[30];

				sweep[0] = '\0';
				switch(u.ustuck->data->mattk[i].adtyp) {
					case AD_FIRE:
/*JP						Strcpy(sweep, "ignites and ");*/
						Strcpy(sweep, "ȯ�Ф�");
						break;
					case AD_ELEC:
/*JP						Strcpy(sweep, "fries and ");*/
						Strcpy(sweep, "�Ǥ�");
						break;
					case AD_COLD:
						Strcpy(sweep,
/*JP						      "freezes, shatters and ");*/
						      "���Ĥ������ʤ��ʤˤʤ�");
						break;
				}
/*JP				pline_The("web %sis swept away!", sweep);*/
				pline("��������%s�ʤ��ʤä���", sweep);
			}
			return(0);
		}		     /* default: a nasty jelly-like creature */
/*JP		pline_The("web dissolves into %s.", mon_nam(u.ustuck));*/
		pline("��������ʬ�򤷤�%s�ˤʤä���", mon_nam(u.ustuck));
		return(0);
	}
	if (u.utrap) {
/*JP		You("cannot spin webs while stuck in a trap.");*/
		You("櫤ˤϤޤäƤ���֤���������ĥ��ʤ���");
		return(0);
	}
	exercise(A_DEX, TRUE);
	if (ttmp) switch (ttmp->ttyp) {
		case PIT:
/*JP		case SPIKED_PIT: You("spin a web, covering up the pit.");*/
		case SPIKED_PIT: You("��������ĥ�ꡤ����ʤ�ä���");
			deltrap(ttmp);
			bury_objs(u.ux, u.uy);
			if (Invisible) newsym(u.ux, u.uy);
			return(1);
/*JP		case SQKY_BOARD: pline_The("squeaky board is muffled.");*/
		case SQKY_BOARD: pline("�������Ĥ�ʤ��줿��");
			deltrap(ttmp);
			if (Invisible) newsym(u.ux, u.uy);
			return(1);
		case TELEP_TRAP:
		case LEVEL_TELEP:
		case MAGIC_PORTAL:
/*JP			Your("webbing vanishes!");*/
			Your("�������Ͼä�����");
			return(0);
/*JP		case WEB: You("make the web thicker.");*/
		case WEB: You("���������������ä���");
			return(1);
		case HOLE:
		case TRAPDOOR:
/*JP			You("web over the %s.",
			    (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole");*/
			You("%s����������ʤ�ä���",
			    (ttmp->ttyp == TRAPDOOR) ? "���" : "��");
			deltrap(ttmp);
			if (Invisible) newsym(u.ux, u.uy);
			return 1;
		case ARROW_TRAP:
		case DART_TRAP:
		case BEAR_TRAP:
		case LANDMINE:
		case SLP_GAS_TRAP:
		case RUST_TRAP:
		case MAGIC_TRAP:
		case ANTI_MAGIC:
		case POLY_TRAP:
/*JP			You("have triggered a trap!");*/
			You("櫤��ư�����Ƥ��ޤä���");
			dotrap(ttmp);
			return(1);
		default:
			impossible("Webbing over trap type %d?", ttmp->ttyp);
			return(0);
	}
	ttmp = maketrap(u.ux, u.uy, WEB);
	if (ttmp) {
		ttmp->tseen = 1;
		ttmp->madeby_u = 1;
	}
	if (Invisible) newsym(u.ux, u.uy);
	return(1);
}

int
dosummon()
{
	if (u.uen < 10) {
/*JP	    You("lack the energy to send forth a call for help!");*/
	    You("������Ƥ֤��������Ϥ��ʤ���");
	    return(0);
	}
	u.uen -= 10;
	flags.botl = 1;

/*JP	You("call upon your brethren for help!");*/
	You("��֤�Ƥ����");
	exercise(A_WIS, TRUE);
	if (!were_summon(youmonst.data,TRUE))
/*JP		pline("But none arrive.");*/
		pline("��������������ʤ���");
	return(1);
}

int
doconfuse()
{
	register struct monst *mtmp;
	int looked = 0;
	char qbuf[QBUFSZ];

	if (Blind) {
/*JP		You_cant("see anything to gaze at.");*/
		You("�ܤ������ʤ��Τǡ��ˤ��ʤ���");
		return 0;
	}
	if (u.uen < 15) {
/*JP	    You("lack the energy to use your special gaze!");*/
	    You("�ˤ����������Ϥ��ʤ���");
	    return(0);
	}
	u.uen -= 15;
	flags.botl = 1;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (canseemon(mtmp)) {
		looked++;
		if (Invis && !perceives(mtmp->data))
/*JP		    pline("%s seems not to notice your gaze.", Monnam(mtmp));*/
		    pline("%s�Ϥ��ʤ��Τˤ�ߤ˵����Ĥ��Ƥʤ���", Monnam(mtmp));
		else if (mtmp->minvis && !See_invisible)
/*JP		    You_cant("see where to gaze at %s.", Monnam(mtmp));*/
		    You("%s�ϸ����ʤ��Τǡ��ˤ�ʤ�", Monnam(mtmp));
		else if (mtmp->m_ap_type == M_AP_FURNITURE
			|| mtmp->m_ap_type == M_AP_OBJECT) {
		    looked--;
		    continue;
		} else if (flags.safe_dog && !Confusion && !Hallucination
		  && mtmp->mtame) {
/*JP		    You("avoid gazing at %s.", y_monnam(mtmp));*/
		    You("%s�����ܤ򤽤餷�Ƥ��ޤä���", y_monnam(mtmp));
		} else {
		    if (flags.confirm && mtmp->mpeaceful && !Confusion
							&& !Hallucination) {
/*JP			Sprintf(qbuf, "Really confuse %s?", mon_nam(mtmp));*/
			Sprintf(qbuf, "������%s���𤵤��ޤ�����", mon_nam(mtmp));
			if (yn(qbuf) != 'y') continue;
			setmangry(mtmp);
		    }
		    if (!mtmp->mcanmove || mtmp->mstun || mtmp->msleeping ||
				    !mtmp->mcansee || !haseyes(mtmp->data)) {
			looked--;
			continue;
		    }
/*JP		    if (!mon_reflects(mtmp,"Your gaze is reflected by %s %s.")){*/
		    if (!mon_reflects(mtmp,"���ʤ��Τˤ�ߤ�%s��%s��ȿ�ͤ��줿��")){
			if (!mtmp->mconf)
/*JP			    Your("gaze confuses %s!", mon_nam(mtmp));*/
			    Your("�ˤ�ߤ�%s���𤵤�����", mon_nam(mtmp));
			else
/*JP			    pline("%s is getting more and more confused.",*/
			    pline("%s�Ϥޤ��ޤ����𤷤���",
							    Monnam(mtmp));
			mtmp->mconf = 1;
		    }
		    if ((mtmp->data==&mons[PM_FLOATING_EYE]) && !mtmp->mcan) {
			if (!Free_action) {
/*JP			    You("are frozen by %s gaze!",*/
			    You("%s�Τˤ�ߤ�ư���ʤ��ʤä���", 
					     s_suffix(mon_nam(mtmp)));
			    nomul((u.ulevel > 6 || rn2(4)) ?
				    -d((int)mtmp->m_lev+1,
					    (int)mtmp->data->mattk[0].damd)
				    : -200);
			    return 1;
			} else
/*JP			    You("stiffen momentarily under %s gaze.",*/
			    You("%s�Τˤ�ߤǰ�ֹ�ľ������",
				    s_suffix(mon_nam(mtmp)));
		    }
		    if ((mtmp->data == &mons[PM_MEDUSA]) && !mtmp->mcan) {
			pline(
/*JP			 "Gazing at the awake %s is not a very good idea.",*/
			 "�ܤ�Фޤ��Ƥ���%s��ˤ��Τϸ������Ȥ���ʤ���",
			    l_monnam(mtmp));
			/* as if gazing at a sleeping anything is fruitful... */
/*JP			You("turn to stone...");*/
			You("�в�����������");
			killer_format = KILLED_BY;
			killer =
/*JP			 "deliberately gazing at Medusa's hideous countenance";*/
			 "�ΰդ˽����ʥ�ǥ塼���δ��ˤ���";
			done(STONING);
		    }
		}
	    }
	}
/*JP	if (!looked) You("gaze at no place in particular.");*/
	if (!looked) You("�ºݤˤϲ���ˤ��ʤ��ä���");
	return 1;
}

int
dohide()
{
	boolean ismimic = youmonst.data->mlet == S_MIMIC;

	if (u.uundetected || (ismimic && youmonst.m_ap_type != M_AP_NOTHING)) {
/*JP		You("are already hiding.");*/
		You("�⤦����Ƥ��롥");
		return(0);
	}
	if (ismimic) {
		/* should bring up a dialog "what would you like to imitate?" */
		youmonst.m_ap_type = M_AP_OBJECT;
		youmonst.mappearance = STRANGE_OBJECT;
	} else
		u.uundetected = 1;
	newsym(u.ux,u.uy);
	return(1);
}

int
domindblast()
{
	struct monst *mtmp, *nmon;

	if (u.uen < 10) {
/*JP	    You("concentrate but lack the energy to maintain doing so.");*/
	    You("���椷�������������ͥ륮����­��ʤ���");
	    return(0);
	}
	u.uen -= 10;
	flags.botl = 1;

/*JP	You("concentrate.");*/
	You("���椷����");
/*JP	pline("A wave of psychic energy pours out.");*/
	pline("�������ͥ륮���Ȥ�����������");
	for(mtmp=fmon; mtmp; mtmp = nmon) {
		int u_sen;

		nmon = mtmp->nmon;
		if (distu(mtmp->mx, mtmp->my) > BOLT_LIM * BOLT_LIM)
			continue;
		if(mtmp->mpeaceful)
			continue;
		u_sen = telepathic(mtmp->data) && !mtmp->mcansee;
		if (u_sen || (telepathic(mtmp->data) && rn2(2)) || !rn2(10)) {
/*JP			You("lock in on %s %s.", s_suffix(mon_nam(mtmp)),
				u_sen ? "telepathy" :
				telepathic(mtmp->data) ? "latent telepathy" :
				"mind");*/
			pline("%s��%s��", mon_nam(mtmp),
				u_sen ? "��������������" :
				telepathic(mtmp->data) ? "����Ū��������������" :
				"���ذռ�����������");
			mtmp->mhp -= rnd(15);
			if (mtmp->mhp <= 0)
				killed(mtmp);
		}
	}
	return 1;
}

STATIC_OVL void
uunstick()
{
/*JP	pline("%s is no longer in your clutches.", Monnam(u.ustuck));*/
	pline("%s�Ϥ��ʤ��μ꤫��ƨ�줿��", Monnam(u.ustuck));
	u.ustuck = 0;
}

STATIC_OVL void
skinback()
{
	if (uskin) {
/*JP		Your("skin returns to its original form.");*/
		Your("����Ϥ�������λѤ���ä���");
		uarm = uskin;
		uskin = (struct obj *)0;
		/* undo save/restore hack */
		uarm->owornmask &= ~I_SPECIAL;
	}
}

#endif /* OVLB */
#ifdef OVL1

const char *
mbodypart(mon, part)
struct monst *mon;
int part;
{
#if 0 /*JP*/
	static NEARDATA const char
	*humanoid_parts[] = { "arm", "eye", "face", "finger",
		"fingertip", "foot", "hand", "handed", "head", "leg",
		"light headed", "neck", "spine", "toe", "hair" },
	*jelly_parts[] = { "pseudopod", "dark spot", "front",
		"pseudopod extension", "pseudopod extremity",
		"pseudopod root", "grasp", "grasped", "cerebral area",
		"lower pseudopod", "viscous", "middle", "surface",
		"pseudopod extremity", "ripples" },
	*animal_parts[] = { "forelimb", "eye", "face", "foreclaw", "claw tip",
		"rear claw", "foreclaw", "clawed", "head", "rear limb",
		"light headed", "neck", "spine", "rear claw tip", "fur" },
	*horse_parts[] = { "foreleg", "eye", "face", "forehoof", "hoof tip",
		"rear hoof", "foreclaw", "hooved", "head", "rear leg",
		"light headed", "neck", "backbone", "rear hoof tip", "mane" },
	*sphere_parts[] = { "appendage", "optic nerve", "body", "tentacle",
		"tentacle tip", "lower appendage", "tentacle", "tentacled",
		"body", "lower tentacle", "rotational", "equator", "body",
		"lower tentacle tip", "cilia" },
	*fungus_parts[] = { "mycelium", "visual area", "front", "hypha",
		"hypha", "root", "strand", "stranded", "cap area",
		"rhizome", "sporulated", "stalk", "root", "rhizome tip",
		"spores" },
	*vortex_parts[] = { "region", "eye", "front", "minor current",
		"minor current", "lower current", "swirl", "swirled",
		"central core", "lower current", "addled", "center",
		"currents", "edge", "currents" },
	*snake_parts[] = { "vestigial limb", "eye", "face", "large scale",
		"large scale tip", "rear region", "scale gap", "scale gapped",
		"head", "rear region", "light headed", "neck", "length",
		"rear scale", "scales" };
#endif
	static NEARDATA const char 
	*humanoid_parts[] = { "��", "��", "��", "��",
		"����", "­", "��", "��ˤ���", "Ƭ", "­",
		"��ޤ�������", "��", "�ع�", "����", "ȱ" },
 	*jelly_parts[] = { "��������", "��������", "����",
 		"�����������", "��������",
 		"��������δ�", "����", "����", "Ǿ���ΰ�",
 		"�����ε�������", "�ͤФͤФ��Ƥ���", "����ΰ�", "ɽ��",
 		"��������", "����" },
 	*animal_parts[] = { "��­", "��", "��", "����", "����",
 		"����", "����", "�Ҥä�����", "Ƭ", "��­",
 		"��ޤ�������", "��", "�ع�", "������", "����" },
 	*horse_parts[] = { "��­", "��", "��", "����", "��",
 		"����", "����", "���ˤϤ���", "Ƭ", "��­",
 		"��ޤ�������", "��", "�ع�", "������", "���Ƥ���" },
 	*sphere_parts[] = { "�͵�", "��п���", "��", "����",
 		"�������", "�����͵�", "����", "����˻���",
 		"��", "���ο���", "��ž����", "�濴��", "��",
 		"���ο������", "����" },
 	*fungus_parts[] = { "�ݻ���", "����ΰ�", "��", "�ݻ�",
 		"�ݻ�", "��", "����", "����ˤ���ߤĤ���", "��",
 		"����", "���𤹤�", "��", "��", "���Ԥ���",  "��˦" },
 	*vortex_parts[] = { "�ΰ�", "��", "��", "������ή��",
 		"������ή��", "������ή��", "����", "���˴���",
 		"�����濴", "������ή��", "���𤷤�", "�濴��",
 		"ή��", "����", "��ή" },
 	*snake_parts[] = { "�ಽ����­", "��", "��", "�礭����",
 		"�礭���ڤ���", "����ʬ", "�ڤη��", "�ڤη�֤ˤĤ���",
 		"Ƭ", "����ʬ", "��ޤ�������", "��", "��",
 		"����ʬ�γ�", "��" };

	/* claw attacks are overloaded in mons[]; most humanoids with
	   such attacks should still reference hands rather than claws */
#if 0
	static const char not_claws[] = {
		S_HUMAN, S_MUMMY, S_ZOMBIE, S_ANGEL,
		S_NYMPH, S_LEPRECHAUN, S_QUANTMECH, S_VAMPIRE,
		S_ORC, S_GIANT,		/* quest nemeses */
		'\0'		/* string terminator; assert( S_xxx != 0 ); */
	};
#endif
	struct permonst *mptr = mon->data;
#if 0 /*JP*/
/* paw�ϸ��Ȥ�ǭ�μꡤclaw�ϥ�����­�Τ褦�ʤ����Ĥᡤ
   �ɤä�������ܸ줸��ּ�פǤ����Ǥ��礦��
*/
	if (part == HAND || part == HANDED) {	/* some special cases */
	    if (mptr->mlet == S_DOG || mptr->mlet == S_FELINE ||
		    mptr->mlet == S_YETI)
		return part == HAND ? "paw" : "pawed";
	    if (humanoid(mptr) && attacktype(mptr, AT_CLAW) &&
		    !index(not_claws, mptr->mlet) &&
		    mptr != &mons[PM_STONE_GOLEM] &&
		    mptr != &mons[PM_INCUBUS] && mptr != &mons[PM_SUCCUBUS])
		return part == HAND ? "claw" : "clawed";
	}
#endif
	if (humanoid(mptr) &&
		(part == ARM || part == FINGER || part == FINGERTIP ||
		    part == HAND || part == HANDED))
	    return humanoid_parts[part];
	if (mptr->mlet == S_CENTAUR || mptr->mlet == S_UNICORN ||
		(mptr == &mons[PM_ROTHE] && part != HAIR))
	    return horse_parts[part];
	if (slithy(mptr))
	    return snake_parts[part];
	if (mptr->mlet == S_EYE)
	    return sphere_parts[part];
	if (mptr->mlet == S_JELLY || mptr->mlet == S_PUDDING ||
		mptr->mlet == S_BLOB)
	    return jelly_parts[part];
	if (mptr->mlet == S_VORTEX || mptr->mlet == S_ELEMENTAL)
	    return vortex_parts[part];
	if (mptr->mlet == S_FUNGUS)
	    return fungus_parts[part];
	if (humanoid(mptr))
	    return humanoid_parts[part];
	return animal_parts[part];
}

const char *
body_part(part)
int part;
{
	return mbodypart(&youmonst, part);
}

#endif /* OVL1 */
#ifdef OVL0

int
poly_gender()
{
/* Returns gender of polymorphed player; 0/1=same meaning as flags.female,
 * 2=none.
 * Used in:
 *	- Seduction by succubus/incubus
 *	- Talking to nymphs (sounds.c)
 * Not used in:
 *	- Messages given by nymphs stealing armor (they can't steal from
 *	  incubi/succubi/nymphs, and nonhumanoids can't wear armor).
 *	- Amulet of change (must refer to real gender no matter what
 *	  polymorphed into).
 *	- Priest/Priestess, Caveman/Cavewoman (ditto)
 *	- Polymorph self (only happens when human)
 *	- Shopkeeper messages (since referred to as "creature" and not "sir"
 *	  or "lady" when polymorphed)
 */
	if (!humanoid(youmonst.data)) return 2;
	return flags.female;
}

#endif /* OVL0 */
#ifdef OVLB

void
ugolemeffects(damtype, dam)
int damtype, dam;
{
	int heal = 0;
	/* We won't bother with "slow"/"haste" since players do not
	 * have a monster-specific slow/haste so there is no way to
	 * restore the old velocity once they are back to human.
	 */
	if (u.umonnum != PM_FLESH_GOLEM && u.umonnum != PM_IRON_GOLEM)
		return;
	switch (damtype) {
		case AD_ELEC: if (u.umonnum == PM_IRON_GOLEM)
				heal = dam / 6; /* Approx 1 per die */
			break;
		case AD_FIRE: if (u.umonnum == PM_IRON_GOLEM)
				heal = dam;
			break;
	}
	if (heal && (u.mh < u.mhmax)) {
		u.mh += heal;
		if (u.mh > u.mhmax) u.mh = u.mhmax;
		flags.botl = 1;
/*JP		pline("Strangely, you feel better than before.");*/
		pline("��̯�ʤ��Ȥˡ�����굤ʬ���褯�ʤä���");
		exercise(A_STR, TRUE);
	}
}

STATIC_OVL int
armor_to_dragon(atyp)
int atyp;
{
	switch(atyp) {
	    case GRAY_DRAGON_SCALE_MAIL:
	    case GRAY_DRAGON_SCALES:
		return PM_GRAY_DRAGON;
	    case SILVER_DRAGON_SCALE_MAIL:
	    case SILVER_DRAGON_SCALES:
		return PM_SILVER_DRAGON;
#if 0	/* DEFERRED */
	    case SHIMMERING_DRAGON_SCALE_MAIL:
	    case SHIMMERING_DRAGON_SCALES:
		return PM_SHIMMERING_DRAGON;
#endif
	    case RED_DRAGON_SCALE_MAIL:
	    case RED_DRAGON_SCALES:
		return PM_RED_DRAGON;
	    case ORANGE_DRAGON_SCALE_MAIL:
	    case ORANGE_DRAGON_SCALES:
		return PM_ORANGE_DRAGON;
	    case WHITE_DRAGON_SCALE_MAIL:
	    case WHITE_DRAGON_SCALES:
		return PM_WHITE_DRAGON;
	    case BLACK_DRAGON_SCALE_MAIL:
	    case BLACK_DRAGON_SCALES:
		return PM_BLACK_DRAGON;
	    case BLUE_DRAGON_SCALE_MAIL:
	    case BLUE_DRAGON_SCALES:
		return PM_BLUE_DRAGON;
	    case GREEN_DRAGON_SCALE_MAIL:
	    case GREEN_DRAGON_SCALES:
		return PM_GREEN_DRAGON;
	    case YELLOW_DRAGON_SCALE_MAIL:
	    case YELLOW_DRAGON_SCALES:
		return PM_YELLOW_DRAGON;
	    default:
		return -1;
	}
}

#endif /* OVLB */

/*polyself.c*/
