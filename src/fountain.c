/*	SCCS Id: @(#)fountain.c	3.2	95/11/04	*/
/*	Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-1996
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

static void NDECL(dowatersnakes);
static void NDECL(dowaterdemon);
static void NDECL(dowaternymph);
STATIC_PTR void FDECL(gush, (int,int,genericptr_t));
static void NDECL(dofindgem);

void
floating_above(what)
const char *what;
{
/*JP    You("are floating high above the %s.", what);*/
    You("%s���ڤ��������⤤�Ƥ��롥", what);
}

static void
dowatersnakes() /* Fountain of snakes! */
{
    register int num = rn1(5,2);
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
	if (!Blind)
/*JP	    pline("An endless stream of %s pours forth!",
		  Hallucination ? makeplural(rndmonnam()) : "snakes");*/
	    pline("%s���ɤɤä�ή�줿��",
		  Hallucination ? makeplural(rndmonnam()) : "��");
	else
/*JP	    You_hear("something hissing!");*/
	    You_hear("�����äȤ�������ʹ������");
	while(num-- > 0)
	    if((mtmp = makemon(&mons[PM_WATER_MOCCASIN],
			u.ux, u.uy, NO_MM_FLAGS)) && t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
    } else
/*JP	pline_The("fountain bubbles furiously for a moment, then calms.");*/
	pline("���������㤷��ˢ������������ä���");
}

static
void
dowaterdemon() /* Water demon */
{
	register struct monst *mtmp;

	if(mvitals[PM_WATER_DEMON].mvflags & G_GONE) return;
	if((mtmp = makemon(&mons[PM_WATER_DEMON],u.ux,u.uy, NO_MM_FLAGS))) {
	    if (!Blind)
/*JP		You("unleash %s!", a_monnam(mtmp));*/
		You("%s�����������", a_monnam(mtmp));
	    else
/*JP		You_feel("the presence of evil.");*/
		You("�ٰ���¸�ߤ򴶤�����");

	/* Give those on low levels a (slightly) better chance of survival */
	    if (rnd(100) > (80 + level_difficulty())) {
/*JP		pline("Grateful for %s release, %s grants you a wish!",
		      his[pronoun_gender(mtmp)], he[pronoun_gender(mtmp)]);*/
		pline("%s�ϲ�����ȤƤⴶ�դ����Τ��ߤ򤫤ʤ��Ƥ����褦����",
		      his[pronoun_gender(mtmp)]);
		makewish();
		mongone(mtmp);
	    } else if (t_at(mtmp->mx, mtmp->my))
		(void) mintrap(mtmp);
	}
}

static void
dowaternymph() /* Water Nymph */
{
	register struct monst *mtmp;

	if(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) return;
	if((mtmp = makemon(&mons[PM_WATER_NYMPH],u.ux,u.uy, NO_MM_FLAGS))) {
		if (!Blind)
/*JP		   You("attract %s!", a_monnam(mtmp));*/
		   pline("%s������줿��", a_monnam(mtmp));
		else
/*JP		   You_hear("a seductive voice.");*/
		   You("̥��Ū������ʹ������");
		mtmp->msleep = 0;
		if (t_at(mtmp->mx, mtmp->my))
		    (void) mintrap(mtmp);
	} else
		if (!Blind)
/*JP		   pline("A large bubble rises to the surface and pops.");*/
		   pline("�礭��ˢ��ʨ���ФƤϤ�������");
		else
/*JP		   You_hear("a loud pop.");*/
		   You("�礭�ʤ�Τ��Ϥ����벻��ʹ������");
}

void
dogushforth(drinking) /* Gushing forth along LOS from (u.ux, u.uy) */
int drinking;
{
	int madepool = 0;

	do_clear_area(u.ux, u.uy, 7, gush, (genericptr_t)&madepool);
	if (!madepool)
	    if (drinking)
/*JP		Your("thirst is quenched.");*/
		Your("��������줿��");
	    else
/*JP		pline("Water sprays all over you.");*/
		pline("�夷�֤������ʤ��ˤ����ä���");
}

STATIC_PTR void
gush(x, y, poolcnt)
int x, y;
genericptr_t poolcnt;
{
	register struct monst *mtmp;
	register struct trap *ttmp;

	if (((x+y)%2) || (x == u.ux && y == u.uy) ||
	    (rn2(1 + distmin(u.ux, u.uy, x, y)))  ||
	    (levl[x][y].typ != ROOM) ||
	    (sobj_at(BOULDER, x, y)) || nexttodoor(x, y))
		return;

	if ((ttmp = t_at(x, y)) != 0 && !delfloortrap(ttmp))
		return;

	if (!((*(int *)poolcnt)++))
/*JP	    pline("Water gushes forth from the overflowing fountain!");*/
	    pline("������夬�ɤɤäȰ��Ф���");

	/* Put a pool at x, y */
	levl[x][y].typ = POOL;
	del_engr_at(x, y);
	water_damage(level.objects[x][y], FALSE, TRUE);

	if ((mtmp = m_at(x, y)) != 0)
		(void) minwater(mtmp);
	else
		newsym(x,y);
}

static void
dofindgem() /* Find a gem in the sparkling waters. */
{
/*JP	if (!Blind) You("spot a gem in the sparkling waters!");*/
	if (!Blind) pline("����᤯���������Ф򸫤Ĥ�����");
	(void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE-1),
						u.ux, u.uy, FALSE);
	levl[u.ux][u.uy].looted |= F_LOOTED;
	newsym(u.ux, u.uy);
	exercise(A_WIS, TRUE);			/* a discovery! */
}

void
dryup(x,y)
xchar x, y;
{
	boolean isyou = (x == u.ux && y == u.uy);

	if (IS_FOUNTAIN(levl[x][y].typ) &&
	    (!rn2(3) || (levl[x][y].looted & F_WARNED))) {
		s_level *slev = Is_special(&u.uz);
		if(isyou && slev && slev->flags.town &&
		   !(levl[x][y].looted & F_WARNED)) {
			struct monst *mtmp;
			levl[x][y].looted |= F_WARNED;
			/* Warn about future fountain use. */
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
			    if((mtmp->data == &mons[PM_WATCHMAN] ||
				mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
			       couldsee(mtmp->mx, mtmp->my) &&
			       mtmp->mpeaceful) {
/*JP				pline("%s yells:", Amonnam(mtmp));*/
				pline("%s�϶������", Amonnam(mtmp));
/*JP				verbalize("Hey, stop using that fountain!");*/
				verbalize("��������������ʡ�");
				break;
			    }
			}
			/* You can see or hear this effect */
/*JP			if(!mtmp) pline_The("flow reduces to a trickle.");*/
			if(!mtmp) pline("ή��Ϥ��������ˤʤä���");
			return;
		}
#ifdef WIZARD
		if (isyou && wizard) {
/*JP			if (yn("Dry up fountain?") == 'n')*/
			if (yn("������ߤۤ��ޤ�����") == 'n')
				return;
		}
#endif
/*JP		if (cansee(x,y)) pline_The("fountain dries up!");*/
		if (cansee(x,y)) pline("���ϴ��夬�ä���");
		levl[x][y].typ = ROOM;
		levl[x][y].looted = 0;
		levl[x][y].blessedftn = 0;
		/* The location is seen if the hero/monster is invisible */
		/* or felt if the hero is blind.			 */
		newsym(x, y);
		level.flags.nfountains--;
		if(isyou && slev && slev->flags.town)
		    (void) angry_guards(FALSE);
	}
}

void
drinkfountain()
{
	/* What happens when you drink from a fountain? */
	register boolean mgkftn = (levl[u.ux][u.uy].blessedftn == 1);
	register int fate = rnd(30);

	if (Levitation) {
/*JP		floating_above("fountain");*/
		floating_above("��");
		return;
	}

	if (mgkftn && u.uluck >= 0 && fate >= 10) {
		int i, ii, littleluck = (u.uluck < 4);

/*JP		pline("Wow!  This makes you feel great!");*/
		pline("�數���ȤƤⵤ�����褯�ʤä���");
		/* blessed restore ability */
		for (ii = 0; ii < A_MAX; ii++)
		    if (ABASE(ii) < AMAX(ii)) {
			ABASE(ii) = AMAX(ii);
			flags.botl = 1;
		    }
		/* gain ability, blessed if "natural" luck is high */
		i = rn2(A_MAX);		/* start at a random attribute */
		for (ii = 0; ii < A_MAX; ii++) {
		    if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
			break;
		    if (++i >= A_MAX) i = 0;
		}
		display_nhwindow(WIN_MESSAGE, FALSE);
/*JP		pline("A wisp of vapor escapes the fountain...");*/
		pline("��Τ����ޤ꤬������ƨ����������");
		exercise(A_WIS, TRUE);
		levl[u.ux][u.uy].blessedftn = 0;
		return;
	}

	if (fate < 10) {
/*JP		pline_The("cool draught refreshes you.");*/
		pline("�䤿�����դϤ��ʤ��򤵤äѤꤵ������");
		u.uhunger += rnd(10); /* don't choke on water */
		newuhs(FALSE);
		if(mgkftn) return;
	} else {
	    switch (fate) {

		case 19: /* Self-knowledge */

/*JP			You_feel("self-knowledgeable...");*/
			You("��ʬ���Ȥ�Ƚ��褦�ʵ�������������");
			display_nhwindow(WIN_MESSAGE, FALSE);
			enlightenment(0);
			exercise(A_WIS, TRUE);
/*JP			pline_The("feeling subsides.");*/
			pline("���δ����Ϥʤ��ʤä���");
			break;

		case 20: /* Foul water */

/*JP			pline_The("water is foul!  You gag and vomit.");*/
			pline("��ϤҤɤ��Բ���̣�����������ʤ����Ǥ��ᤷ����");
			morehungry(rn1(20, 11));
			vomit();
			break;

		case 21: /* Poisonous */

/*JP			pline_The("water is contaminated!");*/
			pline("��ϱ�������Ƥ��롪");
			if (Poison_resistance) {
/*JP	   pline("Perhaps it is runoff from the nearby %s farm.", pl_fruit);*/
	   pline("���֤󡤤����%s������ζ᤯����ή��Ƥ��롥", pl_fruit);
/*JP			   losehp(rnd(4),"unrefrigerated sip of juice",*/
			   losehp(rnd(4),"��ä��̽��Τ��������",
				KILLED_BY_AN);
			   break;
			}
			losestr(rn1(4,3));
/*JP			losehp(rnd(10),"contaminated water", KILLED_BY);*/
			losehp(rnd(10),"�������줿���", KILLED_BY);
			exercise(A_CON, FALSE);
			break;

		case 22: /* Fountain of snakes! */

			dowatersnakes();
			break;

		case 23: /* Water demon */
			dowaterdemon();
			break;

		case 24: /* Curse an item */ {
			register struct obj *obj;

/*JP			pline("This water's no good!");*/
			pline("���ο�ϤȤƤ�ޤ�����");
			morehungry(rn1(20, 11));
			exercise(A_CON, FALSE);
			for(obj = invent; obj ; obj = obj->nobj)
				if (!rn2(5))	curse(obj);
			break;
			}

		case 25: /* See invisible */

/*JP			You("see an image of someone stalking you.");
			pline("But it disappears.");*/
			You("���Ĥ��Ƥ��벿���α����򸫤���");
			pline("������������Ͼä��Ƥ��ޤä���");
			HSee_invisible |= FROMOUTSIDE;
			newsym(u.ux,u.uy);
			exercise(A_WIS, TRUE);
			break;

		case 26: /* See Monsters */

			(void) monster_detect((struct obj *)0, 0);
			exercise(A_WIS, TRUE);
			break;

		case 27: /* Find a gem in the sparkling waters. */

			if (!levl[u.ux][u.uy].looted) {
				dofindgem();
				break;
			}

		case 28: /* Water Nymph */

			dowaternymph();
			break;

		case 29: /* Scare */ {
			register struct monst *mtmp;

/*JP			pline("This water gives you bad breath!");*/
			pline("���������©�������ʤä���");
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
				mtmp->mflee = 1;
			}
			break;

		case 30: /* Gushing forth in this room */

			dogushforth(TRUE);
			break;

		default:

/*JP			pline("This tepid water is tasteless.");*/
			pline("���Τʤޤ̤뤤���̣���ʤ�");
			break;
	    }
	}
	dryup(u.ux, u.uy);
}

void
dipfountain(obj)
register struct obj *obj;
{
	if (Levitation) {
/*JP		floating_above("fountain");*/
		floating_above("��");
		return;
	}

	/* Don't grant Excalibur when there's more than one object.  */
	/* (quantity could be > 1 if merged daggers got polymorphed) */
	if (obj->otyp == LONG_SWORD && obj->quan == 1L
	    && u.ulevel >= 5 && !rn2(6)
	    && !obj->oartifact
	    && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {
		if (u.ualign.type != A_LAWFUL) {
			/* Ha!  Trying to cheat her. */
/*JP			pline("A freezing mist rises from the water and envelopes the sword.");
			pline_The("fountain disappears!");*/
			pline("ɹ��̸���夫��Ω�����ꡤ����ĤĤ����");
			pline("���Ͼä��Ƥ��ޤä���");
			curse(obj);
			if (obj->spe > -6 && !rn2(3)) obj->spe--;
			obj->oerodeproof = FALSE;
			exercise(A_WIS, FALSE);
		} else {
			/* The lady of the lake acts! - Eric Backus */
			/* Be *REAL* nice */
/*JP	  pline("From the murky depths, a hand reaches up to bless the sword.");
			pline("As the hand retreats, the fountain disappears!");*/
	  pline("�����ʿ��ߤ��顤��ʡ���줿���˼꤬���ӤƤ�����");
			pline("�꤬�य�ȡ����Ͼä��Ƥ��ޤä���");
			obj = oname(obj, artiname(ART_EXCALIBUR));
			bless(obj);
			obj->oeroded = 0;
			obj->oerodeproof = TRUE;
			exercise(A_WIS, TRUE);
		}
		levl[u.ux][u.uy].typ = ROOM;
		levl[u.ux][u.uy].looted = 0;
		if(Invisible) newsym(u.ux, u.uy);
		level.flags.nfountains--;
		return;
	} else (void) get_wet(obj);

	switch (rnd(30)) {
		case 16: /* Curse the item */
			curse(obj);
			break;
		case 17:
		case 18:
		case 19:
		case 20: /* Uncurse the item */
			if(obj->cursed) {
			    if (!Blind)
/*JP				pline_The("water glows for a moment.");*/
				pline("��ϵ�����������");
			    uncurse(obj);
			} else {
/*JP			    pline("A feeling of loss comes over you.");*/
			    pline("��̯��æ�ϴ������ʤ��򤪤��ä���");
			}
			break;
		case 21: /* Water Demon */
			dowaterdemon();
			break;
		case 22: /* Water Nymph */
			dowaternymph();
			break;
		case 23: /* an Endless Stream of Snakes */
			dowatersnakes();
			break;
		case 24: /* Find a gem */
			dofindgem();
			break;
		case 25: /* Water gushes forth */
			dogushforth(FALSE);
			break;
		case 26: /* Strange feeling */
/*JP			pline("A strange tingling runs up your %s.",*/
			pline("��̯�ʤ��Ӥ줬���ʤ���%s�����ä���",
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
/*JP			You_feel("a sudden chill.");*/
			You("���������򴶤�����");
			break;
		case 28: /* Strange feeling */
/*JP			pline("An urge to take a bath overwhelms you.");*/
			pline("��Ϥ�����ꤿ���Ȥ�����˾�ˤ���줿��");
			if (u.ugold > 10) {
			    u.ugold -= somegold() / 10;
/*JP			    You("lost some of your gold in the fountain!");*/
			    You("���������������˼��ä���");
			    levl[u.ux][u.uy].looted &= ~F_LOOTED;
			    exercise(A_WIS, FALSE);
			}
			break;
		case 29: /* You see coins */

		/* We make fountains have more coins the closer you are to the
		 * surface.  After all, there will have been more people going
		 * by.	Just like a shopping mall!  Chris Woodbury  */

		    mkgold((long)
			(rnd((dunlevs_in_dungeon(&u.uz)-dunlev(&u.uz)+1)*2)+5),
			u.ux, u.uy);
		    if (!Blind)
/*JP		pline("Far below you, you see coins glistening in the water.");*/
		You("�ڤ����ο���˶�ߤε�����ߤĤ�����");
		    exercise(A_WIS, TRUE);
		    newsym(u.ux,u.uy);
		    break;
	}
	dryup(u.ux, u.uy);
}

#ifdef SINKS
void
breaksink(x,y)
int x, y;
{
    if(cansee(x,y) || (x == u.ux && y == u.uy))
/*JP	pline_The("pipes break!  Water spurts out!");*/
	pline("�۴ɤ�����夬ʮ�Ф�����");
    level.flags.nsinks--;
    levl[x][y].doormask = 0;
    levl[x][y].typ = FOUNTAIN;
    level.flags.nfountains++;
    newsym(x,y);
}

void
drinksink()
{
	struct obj *otmp;
	struct monst *mtmp;

	if (Levitation) {
/*JP		floating_above("sink");*/
		floating_above("ή����");
		return;
	}
	switch(rn2(20)) {
/*JP		case 0: You("take a sip of very cold water.");*/
		case 0: You("�ȤƤ��䤤������������");
			break;
/*JP		case 1: You("take a sip of very warm water.");*/
		case 1: You("�ȤƤ�̤뤤������������");
			break;
/*JP		case 2: You("take a sip of scalding hot water.");*/
		case 2: You("�ȤƤ�Ǯ��������������");
			if (Fire_resistance)
/*JP				pline("It seems quite tasty.");*/
				pline("�ȤƤ⤪�����������");
/*JP			else losehp(rnd(6), "sipping boiling water", KILLED_BY);*/
			else losehp(rnd(6), "ʨƭ�����������", KILLED_BY);
			break;
		case 3: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
/*JP				pline_The("sink seems quite dirty.");*/
				pline("ή����ϤȤƤ���ʤ餷����");
			else {
				mtmp = makemon(&mons[PM_SEWER_RAT],
						u.ux, u.uy, NO_MM_FLAGS);
/*JP				pline("Eek!  There's %s in the sink!",
					Blind ? "something squirmy" :
					a_monnam(mtmp));*/
				pline("����ή�����%s�����롪",
					Blind ? "�Ȥ��������褦�ʤ��" :
					a_monnam(mtmp));
			}
			break;
		case 4: do {
				otmp = mkobj(POTION_CLASS,FALSE);
				if (otmp->otyp == POT_WATER) {
					obfree(otmp, (struct obj *)0);
					otmp = (struct obj *) 0;
				}
			} while(!otmp);
			otmp->cursed = otmp->blessed = 0;
/*JP			pline("Some %s liquid flows from the faucet.",
			      Blind ? "odd" :
			      hcolor(OBJ_DESCR(objects[otmp->otyp])));*/
			pline("�ظ�����%s%s��ή�줿��",
			      Blind ? "��̯��" :
  			      hcolor(jtrns_obj('!',OBJ_DESCR(objects[otmp->otyp]))),
 			      (Blind || Hallucination) ? "����" : "");

			otmp->dknown = !(Blind || Hallucination);
			otmp->quan++; /* Avoid panic upon useup() */
			otmp->corpsenm = 1; /* kludge for docall() */
			(void) dopotion(otmp);
			obfree(otmp, (struct obj *)0);
			break;
		case 5: if (!(levl[u.ux][u.uy].looted & S_LRING)) {
/*JP			    You("find a ring in the sink!");*/
			    You("ή����˻��ؤ�ߤĤ�����");
			    (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
			    levl[u.ux][u.uy].looted |= S_LRING;
			    exercise(A_WIS, TRUE);
			    newsym(u.ux,u.uy);
/*JP			} else pline("Some dirty water backs up in the drain.");*/
			} else pline("���ʤ��夬�ӿ����ή�줿��");
			break;
		case 6: breaksink(u.ux,u.uy);
			break;
/*JP		case 7: pline_The("water moves as though of its own will!");*/
		case 7: pline("�夬�ջפ���äƤ��뤫�Τ褦��ư������");
			if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
			    || !makemon(&mons[PM_WATER_ELEMENTAL],
			 		u.ux, u.uy, NO_MM_FLAGS))
/*JP				pline("But it quiets down.");*/
				pline("���������Ť��ˤʤä���");
			break;
/*JP		case 8: pline("Yuk, this water tastes awful.");*/
		case 8: pline("�������ȤƤ�Ҥɤ�̣�����롥");
			more_experienced(1,0);
			newexplevel();
			break;
/*JP		case 9: pline("Gaggg... this tastes like sewage!  You vomit.");*/
		case 9: pline("������������Τ褦��̣�����롪���ʤ����Ǥ��ᤷ����");
			morehungry(rn1(30-ACURR(A_CON), 11));
			vomit();
			break;
/*JP		case 10: pline("This water contains toxic wastes!");
			You("undergo a freakish metamorphosis!");*/
		case 10: pline("���ο��ͭ�Ǥ��ӿ��ޤ�Ǥ��롪");
			You("������Ѳ��򤷤Ϥ��᤿��");
			polyself();
			break;
		/* more odd messages --JJB */
/*JP		case 11: You_hear("clanking from the pipes...");*/
		case 11: You("�۴ɤΥ�����Ȥ�������ʹ����������");
			break;
/*JP		case 12: You_hear("snatches of song from among the sewers...");*/
		case 12: You("������椫��Ȥ���Ȥ���βΤ�ʹ����������");
			break;
		case 19: if (Hallucination) {
/*JP		   pline("From the murky drain, a hand reaches up... --oops--");*/
		   pline("�Ť��ӿ������꤬���ӤƤ�������--���ä�--");
				break;
			}
/*JP		default: You("take a sip of %s water.",
			rn2(3) ? (rn2(2) ? "cold" : "warm") : "hot");*/
		default: You("%s������������",
			rn2(3) ? (rn2(2) ? "�䤤" : "�̤뤤") : "Ǯ��");
	}
}
#endif /* SINKS */

/*fountain.c*/
