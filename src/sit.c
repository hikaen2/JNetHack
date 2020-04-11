/*	SCCS Id: @(#)sit.c	3.1	93/05/19	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "artifact.h"

void
take_gold()
{
	if (u.ugold <= 0)  {
/*JP		You("feel a strange sensation.");*/
		You("��̯�ʴ��Ф�Ф�����");
	} else {
/*JP		You("notice you have no gold!");*/
		You("�������äƤʤ����Ȥ˵����Ĥ�����");
		u.ugold = 0;
		flags.botl = 1;
	}
}

int
dosit()
{
/*JP	static const char *sit_message = "sit on the %s.";*/
	static const char *sit_message = "%s�˺¤ä���";
	register struct trap *trap;
	register int typ = levl[u.ux][u.uy].typ;

	if(Levitation)  {
/*JP	    pline("You're sitting on air.");*/
	    You("����˺¤ä���");
	    return 0;
	} 

	if(OBJ_AT(u.ux, u.uy)) { 
	    register struct obj *obj;

	    obj = level.objects[u.ux][u.uy];
/*JP	    You("sit on %s.", the(xname(obj)));*/
	    You("%s�˺¤ä���", the(xname(obj)));
/*JP	    if(!Is_box(obj)) pline("It's not very comfortable...");*/
	    if(!Is_box(obj)) pline("���ޤ�¤ꤴ�������褯�ʤ�������");

	} else if ((trap = t_at(u.ux, u.uy)) != 0) {

	    if (u.utrap) {
		exercise(A_WIS, FALSE);	/* you're getting stuck longer */
		if(u.utraptype == TT_BEARTRAP) {
/*JP		    You("can't sit down with your %s in the bear trap.", body_part(FOOT));*/
		    pline("%s������櫤ˤϤ��ޤäƤ���ΤǺ¤�ʤ���", body_part(FOOT));
		    u.utrap++;
	        } else if(u.utraptype == TT_PIT) {
		    if(trap->ttyp == SPIKED_PIT) {
/*JP			You("sit down on a spike.  Ouch!");*/
			You("�ȥ��ξ�˺¤ä������Ƥá�");
/*JP			losehp(1, "sitting on an iron spike", KILLED_BY);*/
			losehp(1, "Ŵ�Υȥ��ξ�˺¤ä�", KILLED_BY);
			exercise(A_STR, FALSE);
		    } else
/*JP			You("sit down in the pit.");*/
			You("������Ǻ¤ä���");
		    u.utrap += rn2(5);
		} else if(u.utraptype == TT_WEB) {
/*JP		    You("sit in the spider web and get entangled further!");*/
		    You("����������Ǻ¤ä��顤�����ޤä���");
		    u.utrap += rn1(10, 5);
		} else if(u.utraptype == TT_LAVA) {
		    /* Must have fire resistance or they'd be dead already */
/*JP		    You("sit in the lava!");*/
		    You("�ϴ����˺¤ä���");
		    u.utrap += rnd(4);
/*JP		    losehp(d(2,10), "sitting in lava", KILLED_BY);*/
		    losehp(d(2,10), "�ϴ����Ǻ¤ä�", KILLED_BY);
		} else if(u.utraptype == TT_INFLOOR) {
/*JP		    You("can't maneuver to sit!");*/
		    You("�¤ä��Ȥ���ǰ�ư�Ǥ��ʤ���");
		    u.utrap++;
		}
	    } else {
/*JP	        You("sit down.");*/
	        You("�¤ä���");
		dotrap(trap);
	    }
	} else if(Underwater || Is_waterlevel(&u.uz)) {
	    if (Is_waterlevel(&u.uz))
/*JP		pline("There are no cushions floating nearby.");*/
		pline("�᤯���⤤�Ƥ��륯�å����Ϥʤ���");
	    else
/*JP		You("sit down in the muddy bottom.");*/
		You("�ɤ�ɤ�����˺¤ä���");
	} else if(is_pool(u.ux, u.uy)) {

/*JP	    You("sit in the water.");*/
	    You("�����Ǻ¤ä���");
	    if (!rn2(10) && uarm)
/*JP		(void) rust_dmg(uarm, "armor", 1, TRUE);*/
		(void) rust_dmg(uarm, "��", 1, TRUE);
#ifdef POLYSELF
	    /* Note: without POLYSELF, this can't _happen_ without */
	    /* water walking boots.... */
	    if (!rn2(10) && uarmf && uarmf->otyp != WATER_WALKING_BOOTS)
/*JP		(void) rust_dmg(uarm, "armor", 1, TRUE);*/
		(void) rust_dmg(uarm, "��", 1, TRUE);
#endif
#ifdef SINKS
	} else if(IS_SINK(typ)) {

/*JP	    You(sit_message, defsyms[S_sink].explanation);*/
	    You(sit_message, jtrns_obj('S',defsyms[S_sink].explanation));
/*JP	    Your("%s gets wet.", humanoid(uasmon) ? "rump" : "underside");*/
	    Your("%s��Ǩ�줿��", humanoid(uasmon) ? "��" : "����");
#endif
	} else if(IS_ALTAR(typ)) {

/*JP	    You(sit_message, defsyms[S_altar].explanation);*/
	    You(sit_message, jtrns_obj('S',defsyms[S_altar].explanation));
	    altar_wrath(u.ux, u.uy);

	} else if(typ == STAIRS) {

/*JP	    You(sit_message, "stairs");*/
	    You(sit_message, "����");

	} else if(typ == LADDER) {

/*JP	    You(sit_message, "ladder");*/
	    You(sit_message, "����");

	} else if (is_lava(u.ux, u.uy)) {

	    /* must be WWalking */
/*JP	    You(sit_message, "lava");*/
	    You(sit_message, "�ϴ�");
/*JP	    pline("The lava burns you!");*/
	    pline("�ϴ�Ϥ��ʤ���ǳ�䤷����");
	    losehp(d((Fire_resistance ? 2 : 10), 10),
/*JP		   "sitting on lava", KILLED_BY);*/
		   "�ϴ�˺¤ä�", KILLED_BY);

	} else if (is_ice(u.ux, u.uy)) {

/*JP	    You(sit_message, defsyms[S_ice].explanation);*/
	    You(sit_message, jtrns_obj('S',defsyms[S_ice].explanation));
/*JP	    if (!Cold_resistance) pline("The ice feels cold.");*/
	    if (!Cold_resistance) pline("ɹ���䤿��������");

	} else if (typ == DRAWBRIDGE_DOWN) {

/*JP	    You(sit_message, "drawbridge");*/
	    You(sit_message, "ķ�Ͷ�");

	} else if(IS_THRONE(typ)) {

/*JP	    You(sit_message, defsyms[S_throne].explanation);*/
	    You(sit_message, jtrns_obj('S',defsyms[S_throne].explanation));
	    if (rnd(6) > 4)  {
		switch (rnd(13))  {
		    case 1:
			(void) adjattrib(rn2(A_MAX), -rn1(4,3), FALSE);
/*JP			losehp(rnd(10), "cursed throne", KILLED_BY_AN);*/
			losehp(rnd(10), "����줿�̺¤�", KILLED_BY_AN);
			break;
		    case 2:
			(void) adjattrib(rn2(A_MAX), 1, FALSE);
			break;
		    case 3:
/*JP			pline("A%s electric shock shoots through your body!",
			      (Shock_resistance) ? "" : " massive");*/
			pline("%s�ŵ�����å������ʤ����Τ�����ȴ������",
			      (Shock_resistance) ? "" : "�㤷��");
			losehp(Shock_resistance ? rnd(6) : rnd(30),
/*JP			       "electric chair", KILLED_BY_AN);*/
			       "�ŵ��ػҤ�", KILLED_BY_AN);
			exercise(A_CON, FALSE);
			break;
		    case 4:
/*JP			You("feel much, much better!");*/
			You("�ȤƤ⡤�ȤƤ⸵���ˤʤä���");
			if(u.uhp >= (u.uhpmax - 5))  u.uhpmax += 4;
			u.uhp = u.uhpmax;
			make_blinded(0L,TRUE);
			make_sick(0L,FALSE);
			heal_legs();
			flags.botl = 1;
			break;
		    case 5:
			take_gold();
			break;
		    case 6:
			if(u.uluck + rn2(5) < 0) {
/*JP			    You("feel your luck is changing.");*/
			    pline("�����褯�ʤä��褦�ʵ������롥");
			    change_luck(1);
			} else	    makewish();
			break;
		    case 7:
			{
			register int cnt = rnd(10);

/*JP			pline("A voice echoes:");*/
			pline("����������:");
/*JP			verbalize("Thy audience hath been summoned, %s!",
				  flags.female ? "Dame" : "Sire");*/
			verbalize("%s�衪���İ���������줷��",
				  flags.female ? "��" : "��");
			while(cnt--)
			    (void) makemon(courtmon(), u.ux, u.uy);
			break;
			}
		    case 8:
/*JP			pline("A voice echoes:");*/
			pline("����������:");
/*JP			verbalize("By thy Imperious order, %s...",
				  flags.female ? "Dame" : "Sire");*/
			verbalize("%s�衪�������ʹ������褦����",
				  flags.female ? "��" : "��");
			do_genocide(1);
			break;
		    case 9:
/*JP			pline("A voice echoes:");*/
			pline("����������:");
/*JP	verbalize("A curse upon thee for sitting upon this most holy throne!");*/
	verbalize("���ʤ�̺¤˺¤ꤷ��˼������졪");
			if (Luck > 0)  {
			    make_blinded(Blinded + rn1(100,250),TRUE);
			} else	    rndcurse();
			break;
		    case 10:
			if (Luck < 0 || (HSee_invisible & INTRINSIC))  {
				if (level.flags.nommap) {
					pline(
/*JP					"A terrible drone fills your head!");*/
					"�������֥�֥�ȸ�������Ƭ�˶�������");
					make_confused(HConfusion + rnd(30),
									FALSE);
				} else {
/*JP					pline("An image forms in your mind.");*/
					pline("���᡼����Ƭ��������");
					do_mapping();
				}
			} else  {
/*JP				Your("vision becomes clear.");*/
				Your("�볦�Ϻ㤨�Ϥä���");
				HSee_invisible |= FROMOUTSIDE;
				newsym(u.ux, u.uy);
			}
			break;
		    case 11:
			if (Luck < 0)  {
/*JP			    You("feel threatened.");*/
			    You("��������Ƥ���褦�ʵ���������");
			    aggravate();
			} else  {

/*JP			    You("feel a wrenching sensation.");*/
			    You("�ͤ���줿�褦�ʴ��Ф򴶤�����");
			    tele();		/* teleport him */
			}
			break;
		    case 12:
/*JP			You("are granted an insight!");*/
			You("ƶ���Ϥ�������");
			if (invent) {
			    int ret, cval = rn2(5); /* agrees w/seffects() */
			    /* use up `cval' "charges"; 0 is special case */
			    do {
/*JP				ret = ggetobj("identify", identify, cval);*/
				ret = ggetobj("���̤���", identify, cval);
				if (ret < 0) break;	/* quit */
			    } while (ret == 0 || (cval -= ret) > 0);
			}
			break;
		    case 13:
/*JP			Your("mind turns into a pretzel!");*/
			Your("���ϥ��ͥ��ͤˤʤä���");
			make_confused(HConfusion + rn1(7,16),FALSE);
			break;
		    default:	impossible("throne effect");
				break;
		}
/*JP	    } else	You("feel somehow out of place...");*/
	    } else	You("���Τ���㤤�ε�������������");

	    if (!rn2(3) && IS_THRONE(levl[u.ux][u.uy].typ)) {
		/* may have teleported */
/*JP		pline("The throne vanishes in a puff of logic.");*/
		pline("�̺¤ϤդäȾä�����");
		levl[u.ux][u.uy].typ = ROOM;
		if(Invisible) newsym(u.ux,u.uy);
	    }

#ifdef POLYSELF
	} else if (lays_eggs(uasmon) || u.umonnum == PM_QUEEN_BEE) {
		struct obj *uegg;

		if (!flags.female) {
/*JP			pline("Males can't lay eggs!");*/
			pline("ͺ����򻺤�ʤ���");
			return 0;
		}

		if (u.uhunger < (int)objects[EGG].oc_nutrition) {
/*JP			You("don't have enough energy to lay an egg.");*/
			You("��򻺤�����Υ��ͥ륮�����ʤ���");
			return 0;
		}

		uegg = mksobj(EGG, FALSE, FALSE);
		uegg->spe = 1;
		uegg->quan = 1;
		uegg->owt = weight(uegg);
		uegg->corpsenm =
		    (u.umonnum==PM_QUEEN_BEE ? PM_KILLER_BEE : monsndx(uasmon));
		uegg->known = uegg->dknown = 1;
/*JP		You("lay an egg.");*/
		You("��򻺤��");
		dropy(uegg);
		stackobj(uegg);
		morehungry((int)objects[EGG].oc_nutrition);
#endif
	} else if (u.uswallow)
/*JP		pline("There are no seats in here!");*/
		pline("�����ˤϰػҤϤʤ���");
	else
/*JP		pline("Having fun sitting on the %s?", surface(u.ux,u.uy));*/
		pline("%s�˺¤äƳڤ���������", surface(u.ux,u.uy));
	return(1);
}

void
rndcurse()			/* curse a few inventory items at random! */
{
	int	nobj = 0;
	int	cnt, onum;
	struct	obj	*otmp;
/*JP	static const char *mal_aura = "feel a malignant aura surround %s.";*/
	static const char *mal_aura = "�ٰ��ʥ������%s�β��˴�������";

	if (uwep && (uwep->oartifact == ART_MAGICBANE) && rn2(20)) {
/*JP	    You(mal_aura, "the magic-absorbing blade");*/
	    You(mal_aura, "���Ϥ�ۤ��Ȥ���");
	    return;
	}

	if(Antimagic) {
	    shieldeff(u.ux, u.uy);
/*JP	    You(mal_aura, "you");*/
	    You(mal_aura, "���ʤ�");
	}

	for (otmp = invent; otmp; otmp = otmp->nobj)  nobj++;

	if (nobj)
	    for (cnt = rnd(6/((!!Antimagic) + (!!Half_spell_damage) + 1));
		 cnt > 0; cnt--)  {
		onum = rn2(nobj);
		for(otmp = invent; onum != 0; onum--)
		    otmp = otmp->nobj;

		if(otmp->oartifact && spec_ability(otmp, SPFX_INTEL) &&
		   rn2(10) < 8) {
/*JP		    pline("%s resists!", The(xname(otmp)));*/
		    pline("%s�ϱƶ�������ʤ���", The(xname(otmp)));
		    continue;
		}

		if(otmp->blessed)
			unbless(otmp);
		else
			curse(otmp);
	    }
}

void
attrcurse()			/* remove a random INTRINSIC ability */
{
	switch(rnd(10)) {
	case 1 : if (HFire_resistance & INTRINSIC) {
			HFire_resistance &= ~INTRINSIC;
/*JP			You("feel warmer.");*/
			You("�Ȥ����򴶤�����");
			break;
		}
	case 2 : if (HTeleportation & INTRINSIC) {
			HTeleportation &= ~INTRINSIC;
/*JP			You("feel less jumpy.");*/
			You("����äȿ��в��Ҥˤʤä���");
			break;
		}
	case 3 : if (HPoison_resistance & INTRINSIC) {
			HPoison_resistance &= ~INTRINSIC;
/*JP			You("feel a little sick!");*/
			You("������ʬ�������ʤä���");
			break;
		}
	case 4 : if (HTelepat & INTRINSIC) {
			HTelepat &= ~INTRINSIC;
			if (Blind && !Telepat)
			    see_monsters();	/* Can't sense mons anymore! */
/*JP			Your("senses fail!");*/
			Your("�޴������㤷����");
			break;
		}
	case 5 : if (HCold_resistance & INTRINSIC) {
			HCold_resistance &= ~INTRINSIC;
/*JP			You("feel cooler.");*/
			You("�ä����򴶤�����");
			break;
		}
	case 6 : if (HInvis & INTRINSIC) {
			HInvis &= ~INTRINSIC;
/*JP			You("feel paranoid.");*/
			You("���ۤ���������");
			break;
		}
	case 7 : if (HSee_invisible & INTRINSIC) {
			HSee_invisible &= ~INTRINSIC;
/*JP			You("thought you saw something!");*/
			You("ï���˸����Ƥ���褦�ʵ���������");
			break;
		}
	case 8 : if (Fast & INTRINSIC) {
			Fast &= ~INTRINSIC;
/*JP			You("feel slower.");*/
			You("�٤��ʤä��褦�ʵ���������");
			break;
		}
	case 9 : if (Stealth & INTRINSIC) {
			Stealth &= ~INTRINSIC;
/*JP			You("feel clumsy.");*/
			You("�Դ��Ѥˤʤä��褦�ʵ���������");
			break;
		}
	case 10: if (Protection & INTRINSIC) {
			Protection &= ~INTRINSIC;
/*JP			You("feel vulnerable.");*/
			You("��Ω�Ĥ褦�ʵ���������");
			break;
		}
	default: break;
	}
}

/*sit.c*/
