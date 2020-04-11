/*	SCCS Id: @(#)polyself.c 3.1	93/06/24	*/
/*	Copyright (C) 1987, 1988, 1989 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

/* Polymorph self routine. */

#include "hack.h"

#ifdef OVLB
#ifdef POLYSELF
static void NDECL(polyman);
static void NDECL(break_armor);
static void FDECL(drop_weapon,(int));
static void NDECL(skinback);
static void NDECL(uunstick);
static int FDECL(armor_to_dragon,(int));

/* make a (new) human out of the player */
static void
polyman()
{
	boolean sticky = sticks(uasmon) && u.ustuck && !u.uswallow;

	if (u.umonnum != -1) {
		u.acurr = u.macurr;	/* restore old attribs */
		u.amax = u.mamax;
		u.umonnum = -1;
		flags.female = u.mfemale;
	}
	u.usym = S_HUMAN;
	set_uasmon();

	u.mh = u.mhmax = 0;
	u.mtimedone = 0;
	skinback();
	u.uundetected = 0;
	newsym(u.ux,u.uy);

	if (sticky) uunstick();
	find_ac();
	if(!Levitation && !u.ustuck &&
	   (is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy)))
		spoteffects();
}
#endif /* POLYSELF */

void
change_sex()
{
	flags.female = !flags.female;
	max_rank_sz();
	if (pl_character[0] == 'P')
		Strcpy(pl_character+6, flags.female ? "ess" : "");
	if (pl_character[0] == 'C')
		Strcpy(pl_character+4, flags.female ? "woman" : "man");
}

void
newman()
{
	int tmp, tmp2;

	if (!rn2(10)) change_sex();

	tmp = u.uhpmax;
	tmp2 = u.ulevel;
	u.ulevel = u.ulevel-2+rn2(5);
	if (u.ulevel > 127 || u.ulevel == 0) u.ulevel = 1;
	if (u.ulevel > MAXULEV) u.ulevel = MAXULEV;

	adjabil(tmp2, (int)u.ulevel);

	/* random experience points for the new experience level */
	u.uexp = rndexp();
#ifndef LINT
	u.uhpmax = (u.uhpmax-10)*(long)u.ulevel/tmp2 + 19 - rn2(19);
#endif
/* If it was u.uhpmax*u.ulevel/tmp+9-rn2(19), then a 1st level character
   with 16 hp who polymorphed into a 3rd level one would have an average
   of 48 hp.  */
#ifdef LINT
	u.uhp = u.uhp + tmp;
#else
	u.uhp = u.uhp * (long)u.uhpmax/tmp;
#endif

	tmp = u.uenmax;
#ifndef LINT
	u.uenmax = u.uenmax * (long)u.ulevel/tmp2 + 9 - rn2(19);
#endif
	if (u.uenmax < 0) u.uenmax = 0;
#ifndef LINT
	u.uen = (tmp ? u.uen * (long)u.uenmax / tmp : u.uenmax);
#endif

	redist_attr();
	u.uhunger = rn1(500,500);
	newuhs(FALSE);
	if (Sick) make_sick(0L, FALSE);
	Stoned = 0;
	if (u.uhp <= 0 || u.uhpmax <= 0) {
#ifdef POLYSELF
		if(Polymorph_control) {
		    if (u.uhp <= 0) u.uhp = 1;
		    if (u.uhpmax <= 0) u.uhpmax = 1;
		} else {
#endif
/*JP		    Your("new form doesn't seem healthy enough to survive.");*/
		    Your("新しい姿は生きてくだけの力がないようだ．");
		    killer_format = KILLED_BY_AN;
/*JP		    killer="unsuccessful polymorph";*/
		    killer="変化の失敗で";
		    done(DIED2);
/*JP		    pline("Revived, you are in just as bad a shape as before.");*/
		    pline("生きかえったが，あなたは生前とかわらぬ酷い格好だ．");
		    done(DIED2);
#ifdef POLYSELF
		}
#endif
	}
#ifdef POLYSELF
	polyman();
#endif
/*JP	You("feel like a new %s!",
	    pl_character[0] == 'E' ? "elf" : flags.female ? "woman" : "man");*/
	You("別の%sになったような気がした！", 
	    pl_character[0] == 'E' ? "エルフ" : flags.female ? "女" : "男");
	flags.botl = 1;
	(void) encumber_msg();
}

#ifdef POLYSELF
void
polyself()
{
	char buf[BUFSZ];
	int mntmp = -1;
	int tries=0;
	boolean draconian = (uarm &&
				uarm->otyp >= GRAY_DRAGON_SCALE_MAIL &&
				uarm->otyp <= YELLOW_DRAGON_SCALES);

	boolean iswere = (u.ulycn > -1 || is_were(uasmon));
	boolean isvamp = (u.usym == S_VAMPIRE || u.umonnum == PM_VAMPIRE_BAT);

	if(!Polymorph_control && !draconian && !iswere && !isvamp) {
	    if (rn2(20) > ACURR(A_CON)) {
		You(shudder_for_moment);
/*JP		losehp(rn2(30),"system shock", KILLED_BY_AN);*/
		losehp(rn2(30),"システムショックで", KILLED_BY_AN);
		exercise(A_CON, FALSE);
		return;
	    }
	}

	if (Polymorph_control) {
		do {
/*JP			getlin("Become what kind of monster? [type the name]",
				buf);*/
			getlin("どの種の怪物になる？[英語名を入れてね]",
				buf);
			mntmp = name_to_mon(buf);
			if (mntmp < 0)
/*JP				pline("I've never heard of such monsters.");*/
				pline("そんな怪物は聞いたことがない．");
			/* Note:  humans are illegal as monsters, but an
			 * illegal monster forces newman(), which is what we
			 * want if they specified a human.... */
			else if (!polyok(&mons[mntmp]) &&
			    ((pl_character[0] == 'E') ? !is_elf(&mons[mntmp])
						: !is_human(&mons[mntmp])) )
/*JP				You("cannot polymorph into that.");*/
				You("それになることはできない．");
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
			if (!(mons[mntmp].geno & G_GENOD)) {
				/* allow G_EXTINCT */
/*JP				You("merge with your scaly armor.");*/
				You("鱗の鎧と一体化した．");
				uskin = uarm;
				uarm = (struct obj *)0;
			}
		} else if (iswere) {
			if (is_were(uasmon))
				mntmp = PM_HUMAN; /* Illegal; force newman() */
			else
				mntmp = u.ulycn;
		} else {
			if (u.usym == S_VAMPIRE)
				mntmp = PM_VAMPIRE_BAT;
			else
				mntmp = PM_VAMPIRE;
		}
		if (polymon(mntmp))
			return;
	}

	if (mntmp < 0) {
		tries = 0;
		do {
			mntmp = rn2(PM_ARCHEOLOGIST);
			/* All valid monsters are from 0 to PM_ARCHEOLOGIST-1 */
		} while(!polyok(&mons[mntmp]) && tries++ < 200);
	}

	/* The below polyok() fails either if everything is genocided, or if
	 * we deliberately chose something illegal to force newman().
	 */
	if (!polyok(&mons[mntmp]) || !rn2(5))
		newman();
	else if(!polymon(mntmp)) return;

/*JP	if (!uarmg) selftouch("No longer petrify-resistant, you");*/
	if (!uarmg) selftouch("もう，石化抵抗力はあなたにない");
}

/* (try to) make a mntmp monster out of the player */
int
polymon(mntmp)	/* returns 1 if polymorph successful */
int	mntmp;
{
	boolean sticky = sticks(uasmon) && u.ustuck && !u.uswallow;
	boolean dochange = FALSE;
	int	tmp;

	if (mons[mntmp].geno & G_GENOD) {	/* allow G_EXTINCT */
/*JP		You("feel rather %s-ish.",mons[mntmp].mname);*/
		You("%sっぽくなったような気がした",jtrns_mon(mons[mntmp].mname));
		exercise(A_WIS, TRUE);
		return(0);
	}

	if (u.umonnum == -1) {
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
		You("%sの%s%s！",
		    flags.female ? "女" : "男",
		    jtrns_mon(mons[mntmp].mname),
		    (u.umonnum != mntmp) ? "になった．" : "になったような気がした");
	} else {
		if (u.umonnum != mntmp)
/*JP			You("turn into %s!", an(mons[mntmp].mname));*/
			You("%sになった！", jtrns_mon(mons[mntmp].mname));
		else
/*JP			You("feel like a new %s!", mons[mntmp].mname);*/
			You("別の%sになったような気がした！", jtrns_mon(mons[mntmp].mname));
	}

	u.umonnum = mntmp;
	u.usym = mons[mntmp].mlet;
	set_uasmon();

	/* New stats for monster, to last only as long as polymorphed.
	 * Currently only strength gets changed.
	 */
	if(strongmonst(&mons[mntmp])) ABASE(A_STR) = AMAX(A_STR) = 118;

	if (resists_ston(uasmon) && Stoned) { /* parnes@eniac.seas.upenn.edu */
		Stoned = 0;
/*JP		You("no longer seem to be petrifying.");*/
		You("もう石化から解放されたようだ．");
	}
	if (u.usym == S_FUNGUS && Sick) {
		make_sick(0L, FALSE);
/*JP		You("no longer feel sick.");*/
		You("病気から解放されたようだ．");
	}

	if (u.usym == S_DRAGON && mntmp >= PM_GRAY_DRAGON)
		u.mhmax = 8 * mons[mntmp].mlevel;
	else if (is_golem(uasmon)) u.mhmax = golemhp(mntmp);
	else {
		/*
		tmp = adj_lev(&mons[mntmp]);
		 * We can't do this, since there's no such thing as an
		 * "experience level of you as a monster" for a polymorphed
		 * character.
		 */
		tmp = mons[mntmp].mlevel;
		if (!tmp) u.mhmax = rnd(4);
		else u.mhmax = d(tmp, 8);
	}
	u.mh = u.mhmax;

	u.mtimedone = rn1(500, 500);
	if (u.ulevel < mons[mntmp].mlevel)
	/* Low level characters can't become high level monsters for long */
#ifdef DUMB
		{
		/* DRS/NS 2.2.6 messes up -- Peter Kendell */
			int	mtd = u.mtimedone,
				ulv = u.ulevel,
				mlv = mons[mntmp].mlevel;

			u.mtimedone = mtd * ulv / mlv;
		}
#else
		u.mtimedone = u.mtimedone * u.ulevel / mons[mntmp].mlevel;
#endif

	if (uskin && mntmp != armor_to_dragon(uskin->otyp))
		skinback();
	break_armor();
	drop_weapon(1);
	if (hides_under(uasmon))
		u.uundetected = OBJ_AT(u.ux, u.uy);
	else
		u.uundetected = 0;
	newsym(u.ux,u.uy);		/* Change symbol */

	if (!sticky && !u.uswallow && u.ustuck && sticks(uasmon)) u.ustuck = 0;
	else if (sticky && !sticks(uasmon)) uunstick();

	if (flags.verbose) {
/*JP	    static const char use_thec[] = "Use the command #%s to %s.";
	    static const char monsterc[] = "monster";*/
	    static const char use_thec[] = "#%sコマンドで%sことができる．";
	    static const char monsterc[] = "monster";
	    if (can_breathe(uasmon))
/*JP		pline(use_thec,monsterc,"use your breath weapon");*/
		pline(use_thec,monsterc,"息を吐きかける");
	    if (attacktype(uasmon, AT_SPIT))
/*JP		pline(use_thec,monsterc,"spit venom");*/
		pline(use_thec,monsterc,"毒を吐く");
	    if (u.usym == S_NYMPH)
/*JP		pline(use_thec,monsterc,"remove an iron ball");*/
		pline(use_thec,monsterc,"鉄球をはずす");
	    if (u.usym == S_UMBER)
/*JP		pline(use_thec,monsterc,"confuse monsters");*/
		pline(use_thec,monsterc,"怪物を混乱させる");
	    if (is_hider(uasmon))
/*JP		pline(use_thec,monsterc,"hide");*/
		pline(use_thec,monsterc,"隠れる");
	    if (is_were(uasmon))
/*JP		pline(use_thec,monsterc,"summon help");*/
		pline(use_thec,monsterc,"仲間を召喚する");
	    if (webmaker(uasmon))
/*JP		pline(use_thec,monsterc,"spin a web");*/
		pline(use_thec,monsterc,"蜘蛛の巣を張る");
	    if (u.umonnum == PM_GREMLIN)
/*JP		pline(use_thec,monsterc,"multiply in a fountain");*/
		pline(use_thec,monsterc,"泉の中で分裂する");
	    if (u.usym == S_UNICORN)
/*JP		pline(use_thec,monsterc,"use your horn");*/
		pline(use_thec,monsterc,"角を使う");
	    if (u.umonnum == PM_MIND_FLAYER)
/*JP		pline(use_thec,monsterc,"emit a mental blast");*/
		pline(use_thec,monsterc,"精神波を発生させる");
	    if (uasmon->msound == MS_SHRIEK) /* worthless, actually */
/*JP		pline(use_thec,monsterc,"shriek");*/
		pline(use_thec,monsterc,"金切り声をあげる");
	    if ((lays_eggs(uasmon) || u.umonnum==PM_QUEEN_BEE) && flags.female)
/*JP		pline(use_thec,"sit","lay an egg");*/
		pline(use_thec,"座る","卵を産む");
	}
	find_ac();
	if((!Levitation && !u.ustuck && !is_flyer(uasmon) &&
	    (is_pool(u.ux,u.uy) || is_lava(u.ux,u.uy))) ||
	   (Underwater && !is_swimmer(uasmon)))
	    spoteffects();
	if (passes_walls(uasmon) && u.utrap && u.utraptype == TT_INFLOOR) {
	    u.utrap = 0;
/*JP	    pline("The rock seems to no longer trap you.");*/
	    pline("もう岩はあなたを閉じ込めないようだ．");
	}
	if ((amorphous(uasmon) || is_whirly(uasmon)) && Punished) {
/*JP	    You("slip out of the iron chain.");*/
	    You("鉄球から解放された．");
	    unpunish();
	}
	flags.botl = 1;
	vision_full_recalc = 1;
	exercise(A_CON, FALSE);
	exercise(A_WIS, TRUE);
	(void) encumber_msg();
	return(1);
}

static void
break_armor()
{
    register struct obj *otmp;

    if (breakarm(uasmon)) {
	if ((otmp = uarm) != 0) {
		if (donning(otmp)) cancel_don();
/*JP		You("break out of your armor!");*/
		You("鎧を壊しやぶった！");
		exercise(A_STR, FALSE);
		(void) Armor_gone();
		useup(otmp);
	}
	if ((otmp = uarmc) != 0) {
	    if(otmp->oartifact) {
/*JP		Your("cloak falls off!");*/
		Your("クロークは脱げ落ちた！");
		(void) Cloak_off();
		dropx(otmp);
	    } else {
/*JP		Your("cloak tears apart!");*/
		Your("クロークはずたずたに引き裂かれた！");
		(void) Cloak_off();
		useup(otmp);
	    }
	}
#ifdef TOURIST
	if (uarmu) {
/*JP		Your("shirt rips to shreds!");*/
		Your("シャツは引き裂かれた！");
		useup(uarmu);
	}
#endif
    } else if (sliparm(uasmon)) {
	if ((otmp = uarm) != 0) {
		if (donning(otmp)) cancel_don();
/*JP		Your("armor falls around you!");*/
		Your("鎧はあなたのまわりに落ちた！");
		(void) Armor_gone();
		dropx(otmp);
	}
	if ((otmp = uarmc) != 0) {
		if (is_whirly(uasmon))
/*JP			Your("cloak falls, unsupported!");*/
			Your("クロークはすーっと落ちた！");
/*JP		else You("shrink out of your cloak!");*/
		else You("クロークから縮み出た！");
		(void) Cloak_off();
		dropx(otmp);
	}
#ifdef TOURIST
	if ((otmp = uarmu) != 0) {
		if (is_whirly(uasmon))
/*JP			You("seep right through your shirt!");*/
			You("シャツからしみ出た！");
/*JP		else You("become much too small for your shirt!");*/
		else You("シャツよりずっと小さくなった！");
		setworn((struct obj *)0, otmp->owornmask & W_ARMU);
		dropx(otmp);
	}
#endif
    }
    if (nohands(uasmon) || verysmall(uasmon)) {
	if ((otmp = uarmg) != 0) {
	    if (donning(otmp)) cancel_don();
	    /* Drop weapon along with gloves */
/*JP	    You("drop your gloves%s!", uwep ? " and weapon" : "");*/
	    You("小手%sを落した！", uwep ? "や武器" : "");
	    drop_weapon(0);
	    (void) Gloves_off();
	    dropx(otmp);
	}
	if ((otmp = uarms) != 0) {
/*JP	    You("can no longer hold your shield!");*/
	    You("もう盾を持ってられない！");
	    (void) Shield_off();
	    dropx(otmp);
	}
	if ((otmp = uarmh) != 0) {
	    if (donning(otmp)) cancel_don();
/*JP	    Your("helmet falls to the %s!", surface(u.ux, u.uy));*/
	    Your("兜は%sに落ちた！", surface(u.ux, u.uy));
	    (void) Helmet_off();
	    dropx(otmp);
	}
    }
    if (nohands(uasmon) || verysmall(uasmon) || slithy(uasmon) || 
		u.usym == S_CENTAUR) {
	if ((otmp = uarmf) != 0) {
	    if (donning(otmp)) cancel_don();
	    if (is_whirly(uasmon))
/*JP		Your("boots fall away!");*/
		Your("靴は脱げ落ちた！");
/*JP	    else Your("boots %s off your feet!",
			verysmall(uasmon) ? "slide" : "are pushed");*/
	    else Your("靴はあなたの足から%s",
			verysmall(uasmon) ? "滑り落ちた" : "脱げ落ちた");
	    (void) Boots_off();
	    dropx(otmp);
	}
    }
}

static void
drop_weapon(alone)
int alone;
{
     struct obj *otmp;
     if ((otmp = uwep) != 0) {
	  /* !alone check below is currently superfluous but in the
	   * future it might not be so if there are monsters which cannot
	   * wear gloves but can wield weapons
	   */
	  if (!alone || cantwield(uasmon)) {
/*JP	       if (alone) You("find you must drop your weapon!");*/
	       if (alone) You("武器を落したことに気がついた！");
	       uwepgone();
	       dropx(otmp);
	  }
     }
}

void
rehumanize()
{
	polyman();
/*JP	You("return to %sn form!", (pl_character[0] == 'E')? "elve" : "huma");*/
	You("%sに戻った！", (pl_character[0] == 'E')? "エルフ" : "人間");

	if (u.uhp < 1)	done(DIED);
/*JP	if (!uarmg) selftouch("No longer petrify-resistant, you");*/
	if (!uarmg) selftouch("もう石化抵抗力はあなたにはない");
	nomul(0);

	flags.botl = 1;
	vision_full_recalc = 1;
	(void) encumber_msg();
}

int
dobreathe() {
	if (Strangled) {
/*JP	    You("can't breathe.  Sorry.");*/
	    You("息を吐くことができない．残念．");
	    return(0);
	}
	if (!getdir(NULL)) return(0);
	if (rn2(4))
/*JP	    You("produce a loud and noxious belch.");*/
	    You("有毒な大きなげっぷをした．");
	else {
	    register struct attack *mattk;
	    register int i;

	    for(i = 0; i < NATTK; i++) {
		mattk = &(uasmon->mattk[i]);
		if(mattk->aatyp == AT_BREA) break;
	    }
	    buzz((int) (20 + mattk->adtyp-1), (int)mattk->damn,
		u.ux, u.uy, u.dx, u.dy);
	}
	return(1);
}

int
dospit() {
	struct obj *otmp;

	if (!getdir(NULL)) return(0);
	otmp = mksobj(u.umonnum==PM_COBRA ? BLINDING_VENOM : ACID_VENOM, TRUE, FALSE);
	otmp->spe = 1; /* to indicate it's yours */
	(void) throwit(otmp);
	return(1);
}

int
doremove() {
	if (!Punished) {
/*JP		You("are not chained to anything!");*/
		You("何もつながれていない！");
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
		You("蜘蛛の巣を張るには地面の上にいなくてはならない．");
		return(0);
	}
	if (u.uswallow) {
/*JP		You("release web fluid inside %s.", mon_nam(u.ustuck));*/
		You("%s内の蜘蛛の巣を吐き出した．", mon_nam(u.ustuck));
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
						Strcpy(sweep, "発火し");
						break;
					case AD_ELEC:
/*JP						Strcpy(sweep, "fries and ");*/
						Strcpy(sweep, "焦げ");
						break;
					case AD_COLD:
						Strcpy(sweep,
/*JP						      "freezes, shatters and ");*/
						      "凍りつき，こなごなになり");
						break;
				}
/*JP				pline("The web %sis swept away!", sweep);*/
				pline("蜘蛛の巣は%sなくなった！", sweep);
			}
			return(0);
		}		     /* default: a nasty jelly-like creature */
/*JP		pline("The web dissolves into %s.", mon_nam(u.ustuck));*/
		pline("蜘蛛の巣は分解して%sになった．", mon_nam(u.ustuck));
		return(0);
	}
	if (u.utrap) {
/*JP		You("cannot spin webs while stuck in a trap.");*/
		You("罠にはまっている間は蜘蛛の巣を張れない．");
		return(0);
	}
	exercise(A_DEX, TRUE);
	if (ttmp) switch (ttmp->ttyp) {
		case PIT:
/*JP		case SPIKED_PIT: You("spin a web, covering up the pit.");*/
		case SPIKED_PIT: You("蜘蛛の巣を張り，落し穴を覆った．");
			deltrap(ttmp);
			bury_objs(u.ux, u.uy);
			if (Invisible) newsym(u.ux, u.uy);
			return(1);
/*JP		case SQKY_BOARD: pline("The squeaky board is muffled.");*/
		case SQKY_BOARD: pline("きしむ板は覆われた．");
			deltrap(ttmp);
			if (Invisible) newsym(u.ux, u.uy);
			return(1);
		case TELEP_TRAP:
		case LEVEL_TELEP:
		case MAGIC_PORTAL:
/*JP			Your("webbing vanishes!");*/
			Your("蜘蛛の巣は消えた！");
			return(0);
/*JP		case WEB: You("make the web thicker.");*/
		case WEB: You("より厚い蜘蛛の巣を作った．");
			return(1);
		case TRAPDOOR:
/*JP			You("web over the trap door.");*/
			You("落し扉を蜘蛛の巣で覆った．");
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
			You("罠を始動させてしまった！");
			dotrap(ttmp);
			return(1);
		default:
			impossible("Webbing over trap type %d?", ttmp->ttyp);
			return(0);
	}
	ttmp = maketrap(u.ux, u.uy, WEB);
	if (ttmp) ttmp->tseen = 1;
	if (Invisible) newsym(u.ux, u.uy);
	return(1);
}

int
dosummon()
{
/*JP	You("call upon your brethren for help!");*/
	You("仲間を呼んだ！");
	exercise(A_WIS, TRUE);
	if (!were_summon(uasmon,TRUE))
/*JP		pline("But none arrive.");*/
		pline("しかし，何も来ない．");
	return(1);
}

int
doconfuse()
{
	register struct monst *mtmp;
	int looked = 0;
	char qbuf[QBUFSZ];

	if (Blind) {
/*JP		You("can't see anything to gaze at.");*/
		You("目が見えないので，睨めない．");
		return 0;
	}
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (canseemon(mtmp)) {
		looked = 1;
		if (Invis && !perceives(mtmp->data))
/*JP		    pline("%s seems not to notice your gaze.", Monnam(mtmp));*/
		    pline("%sはあなたの睨みに気がついてない．", Monnam(mtmp));
		else if (mtmp->minvis && !See_invisible)
/*JP		    You("can't see where to gaze at %s.", Monnam(mtmp));*/
		    You("%sは見えないので，睨めない", Monnam(mtmp));
		else if (mtmp->m_ap_type == M_AP_FURNITURE
			|| mtmp->m_ap_type == M_AP_OBJECT)
		    continue;
		else if (flags.safe_dog && !Confusion && !Hallucination
		  && mtmp->mtame) {
		    if (mtmp->mnamelth)
/*JP			You("avoid gazing at %s.", NAME(mtmp));*/
			You("%sから目をそらしてしまった．", NAME(mtmp));
		    else
/*JP			You("avoid gazing at your %s.",*/
			You("%sを睨むのをやめた．",
						jtrns_mon(mtmp->data->mname));
		} else {
		    if (flags.confirm && mtmp->mpeaceful && !Confusion
							&& !Hallucination) {
/*JP			Sprintf(qbuf, "Really confuse %s?", mon_nam(mtmp));*/
			Sprintf(qbuf, "本当に%sを混乱させますか？", mon_nam(mtmp));
			if (yn(qbuf) != 'y') continue;
			setmangry(mtmp);
		    }
		    if (!mtmp->mcanmove || mtmp->mstun || mtmp->msleep ||
					!mtmp->mcansee || !haseyes(mtmp->data))
			continue;
#ifdef MUSE
/*JP		    if (!mon_reflects(mtmp, "Your gaze is reflected by %s %s."))*/
		    if (!mon_reflects(mtmp, "あなたの睨みは%sの%sで反射された．"))
#endif
		    if (!mtmp->mconf)
/*JP			Your("gaze confuses %s!", mon_nam(mtmp));*/
			Your("睨みは%sを混乱させた！", mon_nam(mtmp));
		    else
/*JP			pline("%s is getting more and more confused.",*/
			pline("%sはますます混乱した！",
							Monnam(mtmp));
		    mtmp->mconf = 1;
		    if ((mtmp->data==&mons[PM_FLOATING_EYE]) && !mtmp->mcan) {
/*JP			You("are frozen by %s gaze!", */
			You("%sの睨みで動けなくなった！", 
			                 s_suffix(mon_nam(mtmp)));
			nomul((u.ulevel > 6 || rn2(4)) ?
				-d((int)mtmp->m_lev+1,
					(int)mtmp->data->mattk[0].damd)
				: -200);
			return 1;
		    }
		    if ((mtmp->data==&mons[PM_MEDUSA]) && !mtmp->mcan) {
/*JP			pline("Gazing at the awake Medusa is not a very good idea.");*/
			pline("目を覚ましているメデューサを睨むのは賢いことじゃない．");
			/* as if gazing at a sleeping anything is fruitful... */
/*JP			You("turn to stone...");*/
			killer = "メデューサを睨んで";
			killer_format = KILLED_BY;
			You("石化した．．．");
			done(STONING);
		    }
		}
	    }
	}
/*JP	if (!looked) You("gaze at no place in particular.");*/
	if (!looked) You("実際には何も睨めなかった．");
	return 1;
}

int
dohide()
{
	if (u.uundetected || u.usym == S_MIMIC_DEF) {
/*JP		You("are already hiding.");*/
		You("もう隠れている．");
		return(0);
	}
	if (u.usym == S_MIMIC) {
		u.usym = S_MIMIC_DEF;
	} else {
		u.uundetected = 1;
	}
	newsym(u.ux,u.uy);
	return(1);
}

int
domindblast()
{
	struct monst *mtmp, *nmon;

/*JP	You("concentrate.");*/
	You("集中した．");
	if (rn2(3)) return 0;
/*JP	pline("A wave of psychic energy pours out.");*/
	pline("精神エネルギー波が放散した．");
	for(mtmp=fmon; mtmp; mtmp = nmon) {
		int u_sen;

		nmon = mtmp->nmon;
		if (distu(mtmp->mx, mtmp->my) > BOLT_LIM * BOLT_LIM)
			continue;
		if(mtmp->mpeaceful)
			continue;
		u_sen = telepathic(mtmp->data) && !mtmp->mcansee;
		if (u_sen || (telepathic(mtmp->data) && rn2(2)) || !rn2(10)) {
/*JP			pline("You lock in on %s's %s.", mon_nam(mtmp),
				u_sen ? "telepathy" :
				telepathic(mtmp->data) ? "latent telepathy" :
				"mind");*/
			pline("%sの%s．", mon_nam(mtmp),
				u_sen ? "精神に入り込んだ" :
				telepathic(mtmp->data) ? "潜在的精神に入り込んだ" :
				"深層意識に潜り込んだ");
			mtmp->mhp -= rnd(15);
			if (mtmp->mhp <= 0)
				killed(mtmp);
		}
	}
	return 1;
}

static void
uunstick()
{
/*JP	pline("%s is no longer in your clutches.", Monnam(u.ustuck));*/
	pline("%sはあなたの手から逃れた．", Monnam(u.ustuck));
	u.ustuck = 0;
}

static void
skinback()
{
	if (uskin) {
/*JP		Your("skin returns to its original form.");*/
		Your("皮膚はそれ本来の姿に戻った．");
		uarm = uskin;
		uskin = (struct obj *)0;
	}
}
#endif

#endif /* OVLB */
#ifdef OVL1
const char *
body_part(part)
int part;
{
	/* Note: it is assumed these will never be >22 characters long,
	 * plus the trailing null, after pluralizing (since sometimes a
	 * buffer is made a fixed size and must be able to hold it)
	 */

/*JP	static NEARDATA const char *humanoid_parts[] = { "arm", "eye", "face", "finger",
		"fingertip", "foot", "hand", "handed", "head", "leg",
		"light headed", "neck", "spine", "toe" };*/
	static NEARDATA const char *humanoid_parts[] = { "腕", "目", "顔", "指",
		"指先", "足", "手", "手にした", "頭", "足",
		"めまいがした", "首", "背骨", "爪先" };
#ifdef POLYSELF
/*JP	static NEARDATA const char *jelly_parts[] = { "pseudopod", "dark spot", "front",
		"pseudopod extension", "pseudopod extremity",
		"pseudopod root", "grasp", "grasped", "cerebral area",
		"lower pseudopod", "viscous", "middle", "surface",
		"pseudopod extremity" },*/
	static NEARDATA const char *jelly_parts[] = { "擬似触手", "黒い斑点", "前面",
		"擬似触手の先", "擬似触手",
		"擬似触手の幹", "触手", "握った", "脳の領域",
		"下方の擬似触手", "ねばねばしてきた", "中間領域", "表面",
		"擬似触手" },
/*JP	*animal_parts[] = { "forelimb", "eye", "face", "foreclaw", "claw tip",
		"rear claw", "foreclaw", "clawed", "head", "rear limb",
		"light headed", "neck", "spine", "rear claw tip" },*/
	*animal_parts[] = { "前足", "目", "顔", "前爪", "爪先",
		"後爪", "前爪", "ひっかけた", "頭", "後足",
		"めまいがした", "首", "背骨", "後爪先" },
/*JP	*horse_parts[] = { "forelimb", "eye", "face", "forehoof", "hoof tip",
		"rear hoof", "foreclaw", "hooved", "head", "rear limb",
		"light headed", "neck", "backbone", "rear hoof tip" },*/
	*horse_parts[] = { "前足", "目", "顔", "前蹄", "蹄",
		"後蹄", "前爪", "蹄にはさんだ", "頭", "後足",
		"めまいがした", "首", "背骨", "後爪先" },
/*JP	*sphere_parts[] = { "appendage", "optic nerve", "body", "tentacle",
		"tentacle tip", "lower appendage", "tentacle", "tentacled",
		"body", "lower tentacle", "rotational", "equator", "body",
		"lower tentacle tip" },*/
	*sphere_parts[] = { "突起", "視覚神経", "体", "触手",
		"触手の先", "下の突起", "触手", "触手に持った",
		"体", "下の触手", "回転した", "中心線", "体",
		"下の触手の先" },
/*JP	*fungus_parts[] = { "mycelium", "visual area", "front", "hypha",
		"hypha", "root", "strand", "stranded", "cap area",
		"rhizome", "sporulated", "stalk", "root", "rhizome tip" },*/
	*fungus_parts[] = { "菌糸体", "視覚領域", "前", "菌糸",
		"菌糸", "根", "触手", "触手はからみついた", "傘",
		"根茎", "混乱する", "軸", "根", "根茎の先" },
/*JP	*vortex_parts[] = { "region", "eye", "front", "minor current",
		"minor current", "lower current", "swirl", "swirled",
		"central core", "lower current", "addled", "center",
		"currents", "edge" },*/
	*vortex_parts[] = { "領域", "目", "前", "小さい流れ",
		"小さい流れ", "下部の流れ", "渦巻", "渦に巻かれた",
		"渦の中心", "下部の流れ", "混乱した", "中心部",
		"流れ", "外周" },
/*JP	*snake_parts[] = { "vestigial limb", "eye", "face", "large scale",
		"large scale tip", "rear region", "scale gap", "scale gapped",
		"head", "rear region", "light headed", "neck", "length",
		"rear scale" };*/
	*snake_parts[] = { "退化した足", "目", "顔", "大きな鱗",
		"大きな鱗の先", "後部分", "鱗の隙間", "鱗の隙間につけられた",
		"頭", "後部分", "めまいがした", "首", "体",
		"後部分の鎧" };

	if (humanoid(uasmon) && (part==ARM || part==FINGER || part==FINGERTIP
		|| part==HAND || part==HANDED)) return humanoid_parts[part];
	if (u.usym==S_CENTAUR || u.usym==S_UNICORN) return horse_parts[part];
	if (slithy(uasmon)) return snake_parts[part];
	if (u.usym==S_EYE) return sphere_parts[part];
	if (u.usym==S_JELLY || u.usym==S_PUDDING || u.usym==S_BLOB)
		return jelly_parts[part];
	if (u.usym==S_VORTEX || u.usym==S_ELEMENTAL) return vortex_parts[part];
	if (u.usym==S_FUNGUS) return fungus_parts[part];
	if (humanoid(uasmon)) return humanoid_parts[part];
	return animal_parts[part];
#else
	return humanoid_parts[part];
#endif
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
#ifdef POLYSELF
	if (!humanoid(uasmon)) return 2;
#endif
	return flags.female;
}

#endif /* OVL0 */
#ifdef OVLB

#if defined(POLYSELF)
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
		pline("奇妙なことに，前より気分がよくなった．");
		exercise(A_STR, TRUE);
	}
}

static int
armor_to_dragon(atyp)
int atyp;
{
	switch(atyp) {
	    case GRAY_DRAGON_SCALE_MAIL:
	    case GRAY_DRAGON_SCALES:
		return PM_GRAY_DRAGON;
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
#endif /* POLYSELF */

#endif /* OVLB */

/*polyself.c*/
