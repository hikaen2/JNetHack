/*	SCCS Id: @(#)read.c	3.1	93/05/26	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

/* elven armor vibrates warningly when enchanted beyond a limit */
#define is_elven_armor(optr)	((optr)->otyp == ELVEN_LEATHER_HELM\
				|| (optr)->otyp == ELVEN_MITHRIL_COAT\
				|| (optr)->otyp == ELVEN_CLOAK\
				|| (optr)->otyp == ELVEN_SHIELD\
				|| (optr)->otyp == ELVEN_BOOTS)

#ifdef OVLB

boolean	known;

static NEARDATA const char readable[] =
		   { ALL_CLASSES, SCROLL_CLASS, SPBOOK_CLASS, 0 };
static const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };

static void FDECL(wand_explode, (struct obj *));
static void NDECL(do_class_genocide);
static void FDECL(stripspe,(struct obj *));
static void FDECL(p_glow1,(struct obj *));
static void FDECL(p_glow2,(struct obj *,const char *));
static void FDECL(forget,(BOOLEAN_P));

#endif /* OVLB */

#ifndef OVERLAY
STATIC_DCL void FDECL(set_lit, (int,int,genericptr_t));
#endif

#ifdef OVLB

int
doread()
{
	register struct obj *scroll;
	register boolean confused;

	known = FALSE;
	if(check_capacity(NULL)) return (0);
/*JP	scroll = getobj(readable, "read");*/
	scroll = getobj(readable, "�ɤ�");
	if(!scroll) return(0);

	/* outrumor has its own blindness check */
	if(scroll->otyp == FORTUNE_COOKIE) {
	    if(flags.verbose)
/*JP		You("break up the cookie and throw away the pieces.");*/
		You("���å������ꡤ��������ꤲ���Ƥ���");
	    outrumor(bcsign(scroll), TRUE);
	    useup(scroll);
	    return(1);
	} else if (scroll->oclass != SCROLL_CLASS
		&& scroll->oclass != SPBOOK_CLASS) {
/*JP	    pline(silly_thing_to, "read");*/
	    pline(silly_thing_to, "�ɤ�");
	    return(0);
	} else if (Blind) {
	    const char *what = 0;
	    if (scroll->oclass == SPBOOK_CLASS)
/*JP		what = "mystic runes";*/
		what = "����Ū�ʥ롼��ʸ��";
	    else if (!scroll->dknown)
/*JP		what = "formula on the scroll";*/
		what = "��ʪ�μ�ʸ";
	    if (what) {
/*JP		pline("Being blind, you cannot read the %s.", what);*/
		pline("�ܤ������ʤ��Τǡ����ʤ���%s���ɤळ�Ȥ��Ǥ��ʤ���", what);
		return(0);
	    }
	}

	confused = (Confusion != 0);
	if(scroll->oclass == SPBOOK_CLASS) {
	    if(confused) {
/*JP		You("cannot grasp the meaning of this tome.");*/
		You("�����ܤΰ�̣������Ǥ��ʤ���");
		return(0);
	    } else
		return(study_book(scroll));
	}
#ifndef NO_SIGNAL
	scroll->in_use = TRUE;	/* scroll, not spellbook, now being read */
#endif
	if(scroll->otyp != SCR_BLANK_PAPER) {
	  if(Blind)
/*JP	    pline("As you pronounce the formula on it, the scroll disappears.");*/
	    pline("��ʸ�򾧤���ȡ���ʪ�Ͼä�����");
	  else
/*JP	    pline("As you read the scroll, it disappears.");*/
	    pline("��ʪ���ɤ�ȡ�����Ͼä�����");
	  if(confused) {
	    if (Hallucination)
/*JP		pline("Being so trippy, you screw up...");*/
		pline("�ȤƤ�ؤ�ؤ�ʤΤǡ������㤯����ˤ��Ƥ��ޤä�������");
	    else
/*JP		pline("Being confused, you mispronounce the magic words...");*/
		pline("���𤷤Ƥ���Τǡ���ʸ��ְ�äƾ����Ƥ��ޤä�������");
	  }
	}
	if(!seffects(scroll))  {
		if(!objects[scroll->otyp].oc_name_known) {
		    if(known) {
			makeknown(scroll->otyp);
			more_experienced(0,10);
		    } else if(!objects[scroll->otyp].oc_uname)
			docall(scroll);
		}
		if(scroll->otyp != SCR_BLANK_PAPER)
			useup(scroll);
#ifndef NO_SIGNAL
		else scroll->in_use = FALSE;
#endif
	}
	return(1);
}

static void
stripspe(obj)
register struct obj *obj;
{
	if (obj->blessed) pline(nothing_happens);
	else {
		if (obj->spe > 0) {
		    obj->spe = 0;
		    if (obj->otyp == OIL_LAMP || obj->otyp == BRASS_LANTERN)
			obj->age = 0;
/*JP		    Your("%s vibrates briefly.",xname(obj));*/
		    Your("%s�Ͼ���ߤ˿�ư������",xname(obj));
		} else pline(nothing_happens);
	}
}

static void
p_glow1(otmp)
register struct obj	*otmp;
{
/*JP	Your("%s %s briefly.", xname(otmp),
		Blind ? "vibrates" : "glows");*/
	Your("%s�Ͼ���ߤ�%s��", xname(otmp),
		Blind ? "��ư����" : "������");
}

static void
p_glow2(otmp,color)
register struct obj	*otmp;
register const char *color;
{
/*JP	Your("%s %s%s for a moment.",
		xname(otmp),
		Blind ? "vibrates" : "glows ",
		Blind ? (const char *)"" : Hallucination ? hcolor() : color);*/
	Your("%s�ϰ��%s%s��",
		xname(otmp),
		Blind ? (const char *)"" : jconj_adj(Hallucination ? hcolor() : color),
		Blind ? "��ư����" : "������");
}

/*
 * recharge an object; curse_bless is -1 if the recharging implement
 * was cursed, +1 if blessed, 0 otherwise.
 */
void
recharge(obj, curse_bless)
struct obj *obj;
int curse_bless;
{
	register int n;
	boolean is_cursed, is_blessed;

	is_cursed = curse_bless < 0;
	is_blessed = curse_bless > 0;

	if (obj->oclass == WAND_CLASS) {
	    if (obj->otyp == WAN_WISHING) {
		if (obj->recharged) {	/* recharged once already? */
		    wand_explode(obj);
		    return;
		}
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    if (obj->spe != 3) {
			obj->spe = 3;
			p_glow2(obj,blue);
		    } else {
			wand_explode(obj);
			return;
		    }
		} else {
		    if (obj->spe < 3) {
			obj->spe++;
			p_glow2(obj,blue);
		    } else pline(nothing_happens);
		}
		obj->recharged = 1; /* another recharging disallowed */
	    } else {
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    if (objects[obj->otyp].oc_dir == NODIR) {
			n = rn1(5,11);
			if (obj->spe < n) obj->spe = n;
			else obj->spe++;
		    } else {
			n = rn1(5,4);
			if (obj->spe < n) obj->spe = n;
			else obj->spe++;
		    }
		    p_glow2(obj,blue);
		} else {
		    obj->spe++;
		    p_glow1(obj);
		}
	    }
	} else if (obj->oclass == RING_CLASS &&
					objects[obj->otyp].oc_charged) {
	    /* charging does not affect ring's curse/bless status */
	    int s = is_blessed ? rnd(3) : is_cursed ? -rnd(2) : 1;
	    boolean is_on = (obj == uleft || obj == uright);

	    /* destruction depends on current state, not adjustment */
	    if (obj->spe > rn2(7) || obj->spe <= -5) {
/*JP		Your("%s pulsates momentarily, then explodes!",*/
		Your("%s�ϰ��̮ư������ȯ������",
		     xname(obj));
		if (is_on) Ring_gone(obj);
		s = rnd(3 * abs(obj->spe));	/* amount of damage */
		useup(obj);
/*JP		losehp(s, "exploding ring", KILLED_BY_AN);*/
		losehp(s, "���ؤ���ȯ��", KILLED_BY_AN);
	    } else {
		long mask = is_on ? (obj == uleft ? LEFT_RING :
				     RIGHT_RING) : 0L;
/*JP		Your("%s spins %sclockwise for a moment.",
		     xname(obj), s < 0 ? "counter" : "");*/
		Your("%s�ϰ��%s���ײ��˲�ž������",
		     xname(obj), s < 0 ? "ȿ" : "");
		/* cause attributes and/or properties to be updated */
		if (is_on) Ring_off(obj);
		obj->spe += s;	/* update the ring while it's off */
		if (is_on) setworn(obj, mask), Ring_on(obj);
		/* oartifact: if a touch-sensitive artifact ring is
		   ever created the above will need to be revised  */
	    }
	} else {
	    switch(obj->otyp) {
	    case MAGIC_MARKER:
		if (is_cursed) stripspe(obj);
		else if (obj->recharged) {
		    if (obj->spe < 3)
/*JP			Your("marker seems permanently dried out.");*/
			Your("�ޡ����ϴ����˳夭���äƤ��ޤä���");
		    else
			pline(nothing_happens);
		} else if (is_blessed) {
		    n = obj->spe;
		    if (n < 50) obj->spe = 50;
		    if (n >= 50 && n < 75) obj->spe = 75;
		    if (n >= 75) obj->spe += 10;
		    p_glow2(obj,blue);
		    obj->recharged = 1;
		} else {
		    if (obj->spe < 50) obj->spe = 50;
		    else obj->spe++;
		    p_glow2(obj,White);
		    obj->recharged = 1;
		}
		break;
	    case OIL_LAMP:
	    case BRASS_LANTERN:
		if (is_cursed) {
		    stripspe(obj);
		    if (obj->lamplit) {
			if (!Blind)
/*JP			    pline("%s goes out!", The(xname(obj)));*/
			    pline("%s�Ͼä�����", The(xname(obj)));
			obj->lamplit = 0;
			check_lamps();
		    }
		} else if (is_blessed) {
		    obj->spe = 1;
		    obj->age = 1500;
		    p_glow2(obj,blue);
		} else {
		    obj->spe = 1;
		    obj->age += 750;
		    if (obj->age > 1500) obj->age = 1500;
		    p_glow1(obj);
		}
		break;
	    case CRYSTAL_BALL:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    obj->spe = 6;
		    p_glow2(obj,blue);
		} else {
		    if (obj->spe < 5) {
			obj->spe++;
			p_glow1(obj);
		    } else pline(nothing_happens);
		}
		break;
	    case HORN_OF_PLENTY:
	    case BAG_OF_TRICKS:
	    case CAN_OF_GREASE:
		if (is_cursed) stripspe(obj);
		else if (is_blessed) {
		    if (obj->spe <= 10)
			obj->spe += rn1(10, 6);
		    else obj->spe += rn1(5, 6);
		    p_glow2(obj,blue);
		} else {
		    obj->spe += rnd(5);
		    p_glow1(obj);
		}
		break;
	    case MAGIC_FLUTE:
	    case MAGIC_HARP:
	    case FROST_HORN:
	    case FIRE_HORN:
	    case DRUM_OF_EARTHQUAKE:
		if (is_cursed) {
		    stripspe(obj);
		} else if (is_blessed) {
		    obj->spe += d(2,4);
		    p_glow2(obj,blue);
		} else {
		    obj->spe += rnd(4);
		    p_glow1(obj);
		}
		break;
	    default:
/*JP		You("have a feeling of loss.");*/
		You("æ�ϴ��򴶤�����");
		break;
	    } /* switch */
	}
}

/*
 * forget some things (e.g. after reading a scroll of amnesia). abs(howmuch)
 * controls the level of forgetfulness; 0 == part of the map, 1 == all of
 * of map,  2 == part of map + spells, 3 == all of map + spells.
 */

static void
forget(howmuch)
boolean howmuch;
{
	register int zx, zy;
	register struct trap *trap;

	if (Punished) u.bc_felt = 0;	/* forget felt ball&chain */

	known = TRUE;
	for(zx = 0; zx < COLNO; zx++) for(zy = 0; zy < ROWNO; zy++)
	    if (howmuch & 1 || rn2(7)) {
		/* Zonk all memory of this location. */
		levl[zx][zy].seen = levl[zx][zy].waslit = 0;
		levl[zx][zy].glyph = cmap_to_glyph(S_stone);
	    }

	/* forget all traps (except the one the hero is in :-) */
	for (trap = ftrap; trap; trap = trap->ntrap)
	    if (trap->tx != u.ux || trap->ty != u.uy) trap->tseen = 0;

	/*
	 * Make sure that what was seen is restored correctly.  To do this,
	 * we need to go blind for an instant --- turn off the display,
	 * then restart it.  All this work is needed to correctly handle
	 * walls which are stone on one side and wall on the other.  Turning
	 * off the seen bit above will make the wall revert to stone,  but
	 * there are cases where we don't want this to happen.  The easiest
	 * thing to do is to run it through the vision system again, which
	 * is always correct.
	 */
	docrt();		/* this correctly will reset vision */

	if(howmuch & 2) losespells();
}

int
seffects(sobj)
register struct obj	*sobj;
{
	register int cval;
	register boolean confused = (Confusion != 0);
	register struct obj *otmp;

	if (objects[sobj->otyp].oc_magic)
		exercise(A_WIS, TRUE);		/* just for trying */
	switch(sobj->otyp) {
#ifdef MAIL
	case SCR_MAIL:
		known = TRUE;
		if (sobj->spe)
/*JP		    pline("This seems to be junk mail addressed to the finder of the Eye of Larn.");*/
		    pline("Eye of Larn�Υե���������˰��Ƥ�줿���ߥᥤ��Τ褦����");
		/* note to the puzzled: the game Larn actually sends you junk
		 * mail if you win!
		 */
		else readmail(sobj);
		break;
#endif
	case SCR_ENCHANT_ARMOR:
	    {
		register schar s;
		otmp = some_armor();
		if(!otmp) {
			strange_feeling(sobj,
/*JP					!Blind ? "Your skin glows then fades." :
					"Your skin feels warm for a moment.");*/
					!Blind ? "���ʤ�������ϰ�ֵ�������" :
					"���ʤ�������ϰ���Ȥ����ʤä���");
			exercise(A_CON, !sobj->cursed);
			exercise(A_STR, !sobj->cursed);
			return(1);
		}
		if(confused) {
			otmp->oerodeproof = !(sobj->cursed);
			if(Blind) {
			    otmp->rknown = FALSE;
/*JP			    Your("%s feels warm for a moment.",*/
			    Your("%s�ϰ���Ȥ����ʤä���",
				xname(otmp));
			} else {
			    otmp->rknown = TRUE;
/*JP			    Your("%s is covered by a %s %s %s!",*/
			    Your("%s��%s%s%s��ʤ��줿��",
				xname(otmp),
/*JP				sobj->cursed ? "mottled" : "shimmering",*/
				jconj_adj(Hallucination ? hcolor() :
				  sobj->cursed ? Black : golden),
				sobj->cursed ? "����ޤ����" : "���᤯",
/*JP				sobj->cursed ? "glow" :
				  (is_shield(otmp) ? "layer" : "shield"));*/
				sobj->cursed ? "����" :
				  (is_shield(otmp) ? "�Хꥢ" : "�Хꥢ"));
			}
			if (otmp->oerodeproof && otmp->oeroded) {
			    otmp->oeroded = 0;
/*JP			    Your("%s %ss good as new!",
				 xname(otmp), Blind ? "feel" : "look");*/
			    Your("%s�Ͽ���Ʊ�ͤˤʤä��褦��%s��",
				 xname(otmp), Blind ? "������" : "������");
			}
			break;
		}
		if((otmp->spe > ((otmp->otyp == ELVEN_MITHRIL_COAT) ? 5 : 3))
				&& rn2(otmp->spe) && !sobj->cursed) {
/*JP		Your("%s violently %s%s for a while, then evaporates.",
			    xname(otmp),
			    Blind ? "vibrates" : "glows ",
			    Blind ? nul : Hallucination ? hcolor() : silver);*/
		Your("%s�Ϥ��Ф餯�δַ㤷��%s%s����ȯ������",
			    xname(otmp),
			    Blind ? nul : jconj_adj(Hallucination ? hcolor() : silver),
			    Blind ? "��ư��" : "����");
			if(is_cloak(otmp)) (void) Cloak_off();
			if(is_boots(otmp)) (void) Boots_off();
			if(is_helmet(otmp)) (void) Helmet_off();
			if(is_gloves(otmp)) (void) Gloves_off();
			if(is_shield(otmp)) (void) Shield_off();
			if(otmp == uarm) (void) Armor_gone();
			useup(otmp);
			break;
		}
		s = sobj->cursed ? -1 :
		    otmp->spe >= 9 ? (rn2(otmp->spe) == 0) :
		    sobj->blessed ? rnd(3-otmp->spe/3) : 1;
		if (s >= 0 && otmp->otyp >= GRAY_DRAGON_SCALES &&
					otmp->otyp <= YELLOW_DRAGON_SCALES) {
			/* dragon scales get turned into dragon scale mail */
/*JP			Your("%s merges and hardens!", xname(otmp));*/
			Your("%s��ͻ�礷�Ǥ��ʤä���", xname(otmp));
			setworn((struct obj *)0, W_ARM);
			/* assumes same order */
			otmp->otyp = GRAY_DRAGON_SCALE_MAIL +
						otmp->otyp - GRAY_DRAGON_SCALES;
			otmp->cursed = 0;
			if (sobj->blessed) {
				otmp->spe++;
				otmp->blessed = 1;
			}
			otmp->known = 1;
			setworn(otmp, W_ARM);
			break;
		}
/*JP		Your("%s %s%s%s for a %s.",
			xname(otmp),
		        s == 0 ? "violently " : nul,
			Blind ? "vibrates" : "glows ",
			Blind ? nul : Hallucination ? hcolor() :
			  sobj->cursed ? Black : silver,
			  (s*s>1) ? "while" : "moment");*/
		Your("%s��%s%s%s%s��",
			xname(otmp),
			(s*s>1) ? "���Ф餯�δ�" : "���",
		        s == 0 ? "�㤷��" : nul,
			Blind ? nul : jconj_adj(Hallucination ? hcolor() :
			  sobj->cursed ? Black : silver),
			Blind ? "��ư����" : "������");
		otmp->cursed = sobj->cursed;
		if (!otmp->blessed || sobj->cursed)
			otmp->blessed = sobj->blessed;
		if (s) {
			otmp->spe += s;
			adj_abon(otmp, s);
		}

		/* an elven magic clue, cookie@keebler */
		if((otmp->spe > ((otmp->otyp == ELVEN_MITHRIL_COAT) ? 5 : 3))
				&& (is_elven_armor(otmp) || !rn2(7)))
/*JP			Your("%s suddenly vibrates %s.",
				xname(otmp),
				Blind ? "again" : "unexpectedly");*/
			Your("%s������%s��ư������",
				xname(otmp),
				Blind ? "�ޤ�" : "�פ����餺");
		break;
	    }
	case SCR_DESTROY_ARMOR:
	    {
		otmp = some_armor();
		if(confused) {
			if(!otmp) {
/*JP				strange_feeling(sobj,"Your bones itch.");*/
				strange_feeling(sobj,"�����ॺ�ॺ���롥");
				exercise(A_STR, FALSE);
				exercise(A_CON, FALSE);
				return(1);
			}
			otmp->oerodeproof = sobj->cursed;
			p_glow2(otmp,purple);
			break;
		}
		if(!sobj->cursed || !otmp || !otmp->cursed) {
		    if(!destroy_arm(otmp)) {
/*JP			strange_feeling(sobj,"Your skin itches.");*/
			strange_feeling(sobj,"���椬�ॺ�ॺ���롥");
			exercise(A_STR, FALSE);
			exercise(A_CON, FALSE);
			return(1);
		    }
		} else {	/* armor and scroll both cursed */
/*JP		    Your("%s vibrates.", xname(otmp));*/
		    Your("%s�Ͽ�ư������", xname(otmp));
		    if (otmp->spe >= -6) otmp->spe--;
		    make_stunned(HStun + rn1(10, 10), TRUE);
		}
	    }
	    break;
	case SCR_CONFUSE_MONSTER:
	case SPE_CONFUSE_MONSTER:
		if(u.usym != S_HUMAN || sobj->cursed) {
/*JP			if(!HConfusion) You("feel confused.");*/
			if(!HConfusion) You("���𤷤���");
			make_confused(HConfusion + rnd(100),FALSE);
		} else  if(confused) {
		    if(!sobj->blessed) {
/*JP			Your("%s begin to %s%s.",
			    makeplural(body_part(HAND)),
			    Blind ? "tingle" : "glow ",
			    Blind ? nul : Hallucination ? hcolor() : purple);*/
			Your("%s��%s%s�Ϥ��᤿��",
			    makeplural(body_part(HAND)),
			    Blind ? nul : jconj_adj(Hallucination ? hcolor() : purple),
			    Blind ? "�ҥ�ҥꤷ" : "����");
			make_confused(HConfusion + rnd(100),FALSE);
		    } else {
/*JP			pline("A %s%s surrounds your %s.",
			    Blind ? nul : Hallucination ? hcolor() : red,
			    Blind ? "faint buzz" : " glow",
			    body_part(HEAD));*/
			pline("%s%s�����ʤ���%s���괬������",
			    Blind ? nul : jconj_adj(Hallucination ? hcolor() : red),
			    Blind ? "�������˥֡�����Ĥ���" : "�������",
			    body_part(HEAD));
			make_confused(0L,TRUE);
		    }
		} else {
		    if (!sobj->blessed) {
/*JP			Your("%s%s %s%s.",
			makeplural(body_part(HAND)),
			Blind ? "" : " begin to glow",
			Blind ? (const char *)"tingle" : Hallucination ? hcolor() : red,
			u.umconf ? " even more" : "");*/
			Your("%s��%s%s%s��",
			makeplural(body_part(HAND)),
			Blind ? (const char *)"�����ҥ�ҥꤷ��" : jconj_adj(Hallucination ? hcolor() : red),
			u.umconf ? "" : "",
			Blind ? "" : "�����Ϥ��᤿");
			u.umconf++;
		    } else {
			if (Blind)
/*JP			    Your("%s tingle %s sharply.",
				makeplural(body_part(HAND)),
				u.umconf ? "even more" : "very");*/
			    Your("%s��%s�ԥ�ԥꤹ�롥",
				makeplural(body_part(HAND)),
				u.umconf ? "����" : "�ȤƤ�");
			else
/*JP			    Your("%s glow a%s brilliant %s.",
				makeplural(body_part(HAND)),
				u.umconf ? "n even more" : "",
				Hallucination ? hcolor() : red);*/
			    Your("%s��%s%s���뤯��������",
				makeplural(body_part(HAND)),
				u.umconf ? "����" : "",
				jconj_adj(Hallucination ? hcolor() : red));
			u.umconf += rn1(8, 2);
		    }
		}
		break;
	case SCR_SCARE_MONSTER:
	case SPE_CAUSE_FEAR:
	    {	register int ct = 0;
		register struct monst *mtmp;

		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		    if(cansee(mtmp->mx,mtmp->my)) {
			if(confused || sobj->cursed) {
			    mtmp->mflee = mtmp->mfrozen = mtmp->msleep = 0;
			    mtmp->mcanmove = 1;
			} else
			    if (! resist(mtmp, sobj->oclass, 0, NOTELL))
				mtmp->mflee = 1;
			if(!mtmp->mtame) ct++;	/* pets don't laugh at you */
		    }
		if(!ct)
/*JP		      You("hear %s in the distance.",
			       (confused || sobj->cursed) ? "sad wailing" :
							"maniacal laughter");*/
		      You("�󤯤�%s��ʹ������",
			       (confused || sobj->cursed) ? "�ᤷ���㤭������" :
							"���ä��褦�˾Ф���");
		else if(sobj->otyp == SCR_SCARE_MONSTER)
/*JP			You("hear %s close by.",
				  (confused || sobj->cursed) ? "sad wailing" :
						 "maniacal laughter");*/
			You("�᤯��%s��ʹ������",
				  (confused || sobj->cursed) ? "�ᤷ���㤭������" :
						 "���ä��褦�˾Ф���");
		break;
	    }
	case SCR_BLANK_PAPER:
	    if (Blind)
/*JP		You("don't remember there being any magic words on this scroll.");*/
		You("��ʪ�˼�ʸ���񤤤Ƥʤ��ä����Ȥ�פ���������");
	    else
/*JP		pline("This scroll seems to be blank.");*/
		pline("���δ�ʪ�ˤϲ���񤤤Ƥʤ��褦�˸����롥");
	    known = TRUE;
	    break;
	case SCR_REMOVE_CURSE:
	case SPE_REMOVE_CURSE:
	    {	register struct obj *obj;
		if(confused)
		    if (Hallucination)
/*JP			You("feel the power of the Force against you!");*/
			You("����Ϥ����ʤ��ˤϤफ�äƤ���褦�˴�������");
		    else
/*JP			You("feel like you need some help.");*/
			You("��ʬ��������ɬ�פȤ��Ƥ���褦�ʵ���������");
		else
		    if (Hallucination)
/*JP			You("feel in touch with the Universal Oneness.");*/
			You("���踶����Ĵ�¤˿���Ƥ���褦�ʵ���������");
		    else
/*JP			You("feel like someone is helping you.");*/
			pline("ï�������ʤ�������Ƥ���褦�ʵ���������");

/*JP		if(sobj->cursed) pline("The scroll disintegrates.");*/
		if(sobj->cursed) pline("��ʪ��ʴ���ˤʤä���");
		else {
		    for(obj = invent; obj ; obj = obj->nobj)
			if(sobj->blessed || obj->owornmask ||
			   (obj->otyp == LOADSTONE)) {
			    if(confused) blessorcurse(obj, 2);
			    else uncurse(obj);
			}
		}
		if(Punished && !confused) unpunish();
		break;
	    }
	case SCR_CREATE_MONSTER:
#if defined(WIZARD) || defined(EXPLORE_MODE)
	    if (wizard || discover)
		known = TRUE;
#endif /* WIZARD || EXPLORE_MODE */
	case SPE_CREATE_MONSTER:
	    {	register int cnt = 1;

		if(!rn2(73) && !sobj->blessed) cnt += rnd(4);
		if(confused || sobj->cursed) cnt += 12;
		while(cnt--) {
#ifdef WIZARD
		    if(!wizard || !create_particular())
#endif /* WIZARD  */
		    (void) makemon (confused ? &mons[PM_ACID_BLOB] :
					(struct permonst *) 0, u.ux, u.uy);
		}
		/* flush monsters before asking for identification */
		flush_screen(0);
		break;
	    }
/*	    break;	/*NOTREACHED*/
	case SCR_ENCHANT_WEAPON:
		if(uwep && (uwep->oclass == WEAPON_CLASS ||
			    uwep->otyp == PICK_AXE ||
			    uwep->otyp == UNICORN_HORN) && confused) {
		/* oclass check added 10/25/86 GAN */
			uwep->oerodeproof = !(sobj->cursed);
			if(Blind) {
			    uwep->rknown = FALSE;
/*JP			    Your("weapon feels warm for a moment.");*/
			    Your("��郎����Ȥ����ʤä��褦�ʵ���������");
			} else {
			    uwep->rknown = TRUE;
/*JP			    Your("%s covered by a %s %s %s!",
				aobjnam(uwep, "are"),
				sobj->cursed ? "mottled" : "shimmering",
				Hallucination ? hcolor() :
				  sobj->cursed ? purple : golden,
				sobj->cursed ? "glow" : "shield");*/
			    Your("%s��%s%s%s��ʤ��줿��",
				xname(uwep),
				jconj_adj(Hallucination ? hcolor() :
				  sobj->cursed ? purple : golden),
				sobj->cursed ? "����ޤ����" : "���᤯",
				sobj->cursed ? "����" : "�Хꥢ");
			}
			if (uwep->oerodeproof && uwep->oeroded) {
			    uwep->oeroded = 0;
/*JP			    Your("%s good as new!",
				 aobjnam(uwep, Blind ? "feel" : "look"));*/
			    Your("%s�Ͽ���Ʊ�ͤˤʤä��褦��%s��",
				 xname(uwep), Blind ? "������" : "������");
			}
		} else return !chwepon(sobj,
				       sobj->cursed ? -1 :
				       !uwep ? 1 :
				       uwep->spe >= 9 ? (rn2(uwep->spe) == 0) :
				       sobj->blessed ? rnd(3-uwep->spe/3) : 1);
		break;
	case SCR_TAMING:
	case SPE_CHARM_MONSTER:
	    {	register int i,j;
		register int bd = confused ? 5 : 1;
		register struct monst *mtmp;

		for(i = -bd; i <= bd; i++) for(j = -bd; j <= bd; j++)
		if(isok(u.ux+i, u.uy+j) && (mtmp = m_at(u.ux+i, u.uy+j))) {
		    if(sobj->cursed) {
			if(!mtmp->mtame) mtmp->mpeaceful = 0;
		    } else {
			if (mtmp->isshk) {
			    if (!mtmp->mpeaceful) {
/*JP				pline("%s calms down.", Monnam(mtmp));*/
				pline("%s�Ϥ��Ȥʤ����ʤä���", Monnam(mtmp));
				mtmp->mpeaceful = 1;
			    }
			} else if(!resist(mtmp, sobj->oclass, 0, NOTELL))
			    (void) tamedog(mtmp, (struct obj *) 0);
		    }
		}
		break;
	    }
	case SCR_GENOCIDE:
/*JP		You("have found a scroll of genocide!");*/
		pline("����ϵԻ��δ�ʪ����");
		known = TRUE;
		if (sobj->blessed) do_class_genocide();
		else do_genocide(!sobj->cursed | (2 * !!Confusion));
		break;
	case SCR_LIGHT:
		if(!Blind) known = TRUE;
		litroom(!confused && !sobj->cursed, sobj);
		break;
	case SCR_TELEPORTATION:
		if(confused || sobj->cursed) level_tele();
		else {
			if (sobj->blessed && !Teleport_control) {
				known = TRUE;
/*JP				if (yn("Do you wish to teleport?")=='n')*/
				if (yn("�ְִ�ư���ޤ�����")=='n')
					break;
			}
			tele();
			if(Teleport_control || !couldsee(u.ux0, u.uy0) ||
			   (distu(u.ux0, u.uy0) >= 16))
				known = TRUE;
		}
		break;
	case SCR_GOLD_DETECTION:
		if (confused || sobj->cursed) return(trap_detect(sobj));
		else return(gold_detect(sobj));
	case SCR_FOOD_DETECTION:
	case SPE_DETECT_FOOD:
		if (food_detect(sobj))
			return(1);	/* nothing detected */
		break;
	case SPE_IDENTIFY:
		cval = rn2(5);
		goto id;
	case SCR_IDENTIFY:
		/* known = TRUE; */
		if(confused)
/*JP			You("identify this as an identify scroll.");*/
			You("����ϼ��̤δ�ʪ���ȼ��̤�����");
		else
/*JP			pline("This is an identify scroll.");*/
			pline("����ϼ��̤δ�ʪ����");
		if (sobj->blessed || (!sobj->cursed && !rn2(5))) {
			cval = rn2(5);
			/* Note: if rn2(5)==0, identify all items */
			if (cval == 1 && sobj->blessed && Luck > 0) ++cval;
		} else	cval = 1;
		useup(sobj);
		makeknown(SCR_IDENTIFY);
	id:
		if(invent && !confused) {
		    int ret;
		    /* use up `cval' "charges"; 0 is special case */
		    do {
/*JP			ret = ggetobj("identify", identify, cval);*/
			ret = ggetobj("���̤���", identify, cval);
			if (ret < 0) break;	/* quit or no eligible items */
		    } while (ret == 0 || (cval -= ret) > 0);
		}
		return(1);
	case SCR_CHARGING:
		if (confused) {
/*JP		    You("feel charged up!");*/
		    You("��Ŷ���줿�褦�ʵ������롪");
		    if (u.uen < u.uenmax)
			u.uen = u.uenmax;
		    else
			u.uen = (u.uenmax += d(5,4));
		    flags.botl = 1;
		    break;
		}
		known = TRUE;
/*JP		pline("This is a charging scroll.");
		otmp = getobj(all_count, "charge");*/
		pline("����Ͻ�Ŷ�δ�ʪ����");
		otmp = getobj(all_count, "��Ŷ����");
		if (!otmp) break;
		recharge(otmp, sobj->cursed ? -1 : (sobj->blessed ? 1 : 0));
		break;
	case SCR_MAGIC_MAPPING:
		if (level.flags.nommap) {
/*JP		    Your("mind is filled with crazy lines!");*/
		    Your("���ˤ����ʤ������������������Ӥ����ä���");
		    if (Hallucination)
/*JP			pline("Wow!  Modern art.");*/
			pline("�數������󥢡��Ȥ���");
		    else
/*JP			Your("head spins in bewilderment.");*/
			You("���Ǥ����ܤ��ޤ�ä���");
		    make_confused(HConfusion + rnd(30), FALSE);
		    break;
		}
		known = TRUE;
	case SPE_MAGIC_MAPPING:
		if (level.flags.nommap) {
/*JP		    Your("head spins as something blocks the spell!");*/
		    You("��������ʸ�򤵤����ꡤ�ܤ��ޤ�ä�����");
		    make_confused(HConfusion + rnd(30), FALSE);
		    break;
		}
/*JP		pline("A map coalesces in your mind!");*/
		pline("�Ͽޤ����ʤ��ο���ͻ�礷����");
		cval = (sobj->cursed && !confused);
		if(cval) HConfusion = 1;	/* to screw up map */
		do_mapping();
		if(cval) {
		    HConfusion = 0;		/* restore */
/*JP		    pline("Unfortunately, you can't grasp the details.");*/
		    pline("�Թ��ˤ⡤���ʤ��Ͼܺ٤����뤳�Ȥ��Ǥ��ʤ��ä���");
		}
		break;
	case SCR_AMNESIA:
		known = TRUE;
		forget( ((!sobj->blessed) << 1) | (!confused || sobj->cursed) );
		if (Hallucination) /* Ommmmmm! */
/*JP			Your("mind releases itself from mundane concerns.");*/
			Your("����ʿ�ޤ������ط�����������줿��");
		else if (!strncmpi(plname, "Maud", 4))
/*JP			pline("As your mind turns inward on itself, you forget everything else.");*/
			pline("���ʤ��ο�����¦�˸��������Ƥ�˺��Ƥ��ޤä���");
		else if (rn2(2))
/*JP			pline("Who was that Maud person anyway?");*/
			pline("Maud�äƿͤϤ��ä���ï������");
		else
/*JP			pline("Thinking of Maud you forget everything else.");*/
			pline("Maud��ͤ��뤳�Ȱʳ������ʤ������Ƥ�˺��Ƥ��ޤä���");
		exercise(A_WIS, FALSE);
		break;
	case SCR_FIRE:
		/*
		 * Note: Modifications have been made as of 3.0 to allow for
		 * some damage under all potential cases.
		 */
		cval = bcsign(sobj);
		useup(sobj);
		makeknown(SCR_FIRE);
		if(confused) {
		    if(Fire_resistance) {
  			shieldeff(u.ux, u.uy);
			if(!Blind)
/*JP			    pline("Oh, look, what a pretty fire in your %s.",*/
			    pline("�嵐����󡥾����ʲФ�%s�ˤ��롥",
				makeplural(body_part(HAND)));
/*JP			else You("feel a pleasant warmth in your %s.",*/
			else You("%s����˲�Ŭ���Ȥ����򴶤�����",
				makeplural(body_part(HAND)));
		    } else {
/*JP			pline("The scroll catches fire and you burn your %s.",*/
			pline("��ʪ�˲Ф�ǳ�����Ĥꡤ���ʤ���%s��Ƥ�����",
				makeplural(body_part(HAND)));
/*JP			losehp(1, "scroll of fire", KILLED_BY_AN);*/
			losehp(1, "��δ�ʪ��", KILLED_BY_AN);
		    }
		    return(1);
		}
		if (Underwater)
/*JP			pline("The water around you vaporizes violently!");*/
			pline("���ʤ��β��ο��ʨƭ������");
		else
/*JP			pline("The scroll erupts in a tower of flame!");*/
			pline("��ʪ������줬Ω�����ä���");
		explode(u.ux, u.uy, 11, (2*(rn1(3, 3) + 2 * cval) + 1)/3,
							SCROLL_CLASS);
		return(1);
	case SCR_PUNISHMENT:
		known = TRUE;
		if(confused || sobj->blessed) {
/*JP			You("feel guilty.");*/
			You("��򴶤�����");
			break;
		}
		punish(sobj);
		break;
	default:
		impossible("What weird effect is this? (%u)", sobj->otyp);
	}
	return(0);
}

static void
wand_explode(obj)
register struct obj *obj;
{
/*JP    Your("%s vibrates violently, and explodes!",xname(obj));*/
    Your("%s�Ϸ㤷����ư������ȯ������",xname(obj));
    nhbell();
/*JP    losehp(rn2(2*(u.uhpmax+1)/3),"exploding wand", KILLED_BY_AN);*/
    losehp(rn2(2*(u.uhpmax+1)/3),"�����ȯ��", KILLED_BY_AN);
    useup(obj);
    exercise(A_STR, FALSE);
}

/*
 * Low-level lit-field update routine.
 */
STATIC_PTR void
set_lit(x,y,val)
int x, y;
genericptr_t val;
{
	levl[x][y].lit = (val == (genericptr_t)-1) ? 0 : 1;
	return;
}

void
litroom(on,obj)
register boolean on;
struct obj *obj;
{
	/* first produce the text (provided you're not blind) */
	if(!on) {
		register struct obj *otmp;

		if (!Blind) {
		    if(u.uswallow) {
/*JP			pline("It seems even darker in here than before.");*/
			pline("�����Ť��ʤä��褦�˸����롥");
			return;
		    }
/*JP		    You("are surrounded by darkness!");*/
		    You("�ŰǤ�ʤ��줿��");
		}

		/* the magic douses lamps, et al, too */
		for(otmp = invent; otmp; otmp = otmp->nobj)
		    if (otmp->lamplit)
			(void) snuff_lit(otmp);
		if (Blind) goto do_it;
	} else {
		if (Blind) goto do_it;
		if(u.uswallow){
			if (is_animal(u.ustuck->data))
/*JP				pline("%s stomach is lit.",*/
				pline("%s�ΰߤ����뤯�ʤä���",
				         s_suffix(Monnam(u.ustuck)));
			else
				if (is_whirly(u.ustuck->data))
/*JP					pline("%s shines briefly.",*/
					pline("%s�Ϥ���äȵ�������",
					      Monnam(u.ustuck));
				else
/*JP					pline("%s glistens.", Monnam(u.ustuck));*/
					pline("%s�Ϥ��餭�鵱������", Monnam(u.ustuck));
			return;
		}
/*JP		pline("A lit field surrounds you!");*/
		pline("���꤬���ʤ�����Ϥ����");
	}

do_it:
	/* No-op in water - can only see the adjacent squares and that's it! */
	if (Underwater || Is_waterlevel(&u.uz)) return;
	/*
	 *  If we are darkening the room and the hero is punished but not
	 *  blind, then we have to pick up and replace the ball and chain so
	 *  that we don't remember them if they are out of sight.
	 */
	if (Punished && !on && !Blind)
	    move_bc(1, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz)) {
	    /* Can't use do_clear_area because MAX_RADIUS is too small */
	    /* rogue lighting must light the entire room */
	    int rnum = levl[u.ux][u.uy].roomno - ROOMOFFSET;
	    int rx, ry;
	    if(rnum >= 0) {
		for(rx = rooms[rnum].lx-1; rx <= rooms[rnum].hx+1; rx++)
		    for(ry = rooms[rnum].ly-1; ry <= rooms[rnum].hy+1; ry++)
			set_lit(rx, ry, (genericptr_t)((on)? 1 : -1));
		rooms[rnum].rlit = on;
	    }
	    /* hallways remain dark on the rogue level */
	} else
#endif
	    do_clear_area(u.ux,u.uy,
		(obj && obj->oclass==SCROLL_CLASS && obj->blessed) ? 9 : 5,
		set_lit, (genericptr_t)((on)? 1 : -1));

	/*
	 *  If we are not blind, then force a redraw on all positions in sight
	 *  by temporarily blinding the hero.  The vision recalculation will
	 *  correctly update all previously seen positions *and* correctly
	 *  set the waslit bit [could be messed up from above].
	 */
	if (!Blind) {
	    vision_recalc(2);

	    /* replace ball&chain */
	    if (Punished && !on)
		move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
	}

	vision_full_recalc = 1;	/* delayed vision recalculation */
}

static void
do_class_genocide()
{
	register int i, j, immunecnt, gonecnt, goodcnt, class;
	char buf[BUFSZ];

	for(j=0; ; j++) {
		if (j >= 5) {
			pline(thats_enough_tries);
			return;
		}
		do {
/*JP    getlin("What class of monsters do you wish to genocide? [type a letter]",*/
    getlin("�ɤΥ��饹��°�����ʪ��Ի����ޤ�����[ʸ��������Ƥ�]",
	   buf);
		} while (buf[0]=='\033' || strlen(buf) != 1);
		immunecnt = gonecnt = goodcnt = 0;
		class = def_char_to_monclass(buf[0]);
		for(i = 0; i < NUMMONS; i++) {
			if(mons[i].mlet == class) {
				if (!(mons[i].geno & G_GENO)) immunecnt++;
				else if(mons[i].geno & G_GENOD) gonecnt++;
				else goodcnt++;
			}
		}
		if (!goodcnt && class != S_HUMAN) {
			if (gonecnt)
/*JP	pline("All such monsters are already nonexistent.");*/
	pline("���β�ʪ�Ϥ⤦���ʤ���");
			else if (immunecnt)
/*JP	You("aren't permitted to genocide such monsters.");*/
	You("���β�ʪ��Ի����뤳�ȤϤǤ��ʤ���");
			else
/*JP	pline("That symbol does not represent any monster.");*/
	pline("���ε���β�ʪ�Ϥ��ʤ���");
			continue;
		}
		for(i = 0; i < NUMMONS; i++) {
		    if(mons[i].mlet == class) {
			register struct monst *mtmp, *mtmp2;
/*JP			char *n = makeplural(mons[i].mname);*/
			const char *n = jtrns_mon(mons[i].mname);

			if (&mons[i]==player_mon() || ((mons[i].geno & G_GENO)
				&& !(mons[i].geno & G_GENOD))) {
			/* This check must be first since player monsters might
			 * have G_GENOD or !G_GENO.
			 */
/*JP			    pline("Wiped out all %s.", n);*/
			    pline("%s�������ӽ�������", n);
			    if (&mons[i] == player_mon()) {
				u.uhp = -1;
				killer_format = KILLED_BY_AN;
/*JP				killer = "scroll of genocide";*/
				killer = "�Ի��δ�ʪ��";
#ifdef POLYSELF
				if (u.umonnum >= 0)
/*JP				    You("feel dead inside.");*/
				    Your("���������褦�ʵ���������");
				else
#endif
				    done(GENOCIDED);
			    }
			    /* for simplicity (and fairness) let's avoid
			     * alignment changes here...
			     */
#ifdef POLYSELF
			    if (i==u.umonnum) rehumanize();
#endif
			    mons[i].geno |= G_GENOD;
			    for(mtmp = fmon; mtmp; mtmp = mtmp2) {
				mtmp2 = mtmp->nmon;
				if(mtmp->data == &mons[i])
				    mondead(mtmp);
			    }
			} else if (mons[i].geno & G_GENOD)
/*JP			    pline("All %s are already nonexistent.", n);*/
			    pline("%s�ϴ��ˤ��ʤ���", n);
			else
/*JP			    You("aren't permitted to genocide %s%s.",*/
			    You("%s��Ի��Ǥ��ʤ���",
/*JP				i == PM_WIZARD_OF_YENDOR ? "the " : "",*/
				jtrns_mon(type_is_pname(&mons[i]) ? mons[i].mname : (const char *)n));
			}
		}
		return;
	}
}

#define REALLY 1
#define PLAYER 2
void
do_genocide(how)
int how;
/* 0 = no genocide; create monsters (cursed scroll) */
/* 1 = normal genocide */
/* 3 = forced genocide of player */
{
	char buf[BUFSZ];
	register int	i, j, killplayer = 0;
	register struct permonst *ptr;
	register struct monst *mtmp, *mtmp2;

	if (how & PLAYER) {
		ptr = player_mon();
		Strcpy(buf, ptr->mname);
		killplayer++;
	} else {
	    for(j = 0; ; j++) {
		if(j >= 5) {
		    pline(thats_enough_tries);
		    return;
		}
/*JP		getlin("What monster do you want to genocide? [type the name]",*/
		getlin("�ɤβ�ʪ��Ի����ޤ�����[�Ѹ������Ƥ�]",
			buf);

		i = name_to_mon(buf);
		if(i == -1 || (mons[i].geno & G_GENOD)) {
/*JP			pline("Such creatures do not exist in this world.");*/
			pline("���Τ褦������ʪ�Ϥ���������¸�ߤ��ʤ���");
			continue;
		}
		ptr = &mons[i];
		if (ptr == player_mon()) {
			killplayer++;
			goto deadmeat;
		}
		if (is_human(ptr)) adjalign(-sgn(u.ualign.type));
		if (is_demon(ptr)) adjalign(sgn(u.ualign.type));

		if(!(ptr->geno & G_GENO))  {
			if(flags.soundok) {
			    if(flags.verbose)
/*JP			pline("A thunderous voice booms though the caverns:");
			    pline("\"No, mortal!  That will not be done.\"");*/
			pline("��Τ褦������ƶ���˶�������");
			    pline("�ֿʹ֤衤����˾�ߤϤ��ʤ��ޤ���");
			}
			continue;
		}
		break;
	    }
	}
deadmeat:
	if (Hallucination) {
#ifdef POLYSELF
	    if (u.umonnum != -1)
		Strcpy(buf,uasmon->mname);
	    else
#endif
	    {
		Strcpy(buf, pl_character);
		buf[0] += 'a' - 'A';
	    }
	} else Strcpy(buf,ptr->mname); /* make sure we have standard singular */
	if (how & REALLY) {
/*JP	    pline("Wiped out all %s.", makeplural(buf));*/
	    pline("%s�����ư��ݤ�����", jtrns_mon(buf));
	    if(killplayer) {
		u.uhp = -1;
		killer_format = KILLED_BY_AN;
/*JP		killer = "genocide spell";*/
		killer = "�Ի�����ˡ��";
#ifdef POLYSELF
	/* Polymorphed characters will die as soon as they're rehumanized. */
/*JP		if(u.umonnum >= 0)	You("feel dead inside.");*/
		if(u.umonnum >= 0)	Your("���������褦�ʵ���������");
		else
#endif
			done(GENOCIDED);
		return;
	    }
#ifdef POLYSELF
	    else if (ptr == uasmon) rehumanize();
#endif
	    ptr->geno |= G_GENOD;
	    for(mtmp = fmon; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if(mtmp->data == ptr)
		    mondead(mtmp);
	    }
	} else if (!(ptr->geno & G_EXTINCT)) {
/*JP	    pline("Sent in some %s.", makeplural(buf));*/
	    pline("��ɤ��%s�������Ƥ�����", jtrns_mon(buf));
	    j = rn1(3, 4);
	    for(i=1; i<=j; i++) {
		struct monst *mmon = makemon(ptr, u.ux, u.uy);
		struct obj *otmp;

		while ((otmp = mmon->minvent) != 0) {
			mmon->minvent = otmp->nobj;
			dealloc_obj(otmp);
		}
	    }
	}
}

#endif /* OVLB */
#ifdef OVLB

void
punish(sobj)
register struct obj	*sobj;
{
/*JP	You("are being punished for your misbehavior!");*/
	You("�Կ����Τ���ȳ���������");
	if(Punished){
/*JP		Your("iron ball gets heavier.");*/
		Your("Ŵ��Ϥ���˽Ť��ʤä���");
		uball->owt += 160 * (1 + sobj->cursed);
		return;
	}
	setworn(mkobj(CHAIN_CLASS, TRUE), W_CHAIN);
	setworn(mkobj(BALL_CLASS, TRUE), W_BALL);
	uball->spe = 1;		/* special ball (see save) */

	/*
	 *  Place ball & chain if not swallowed.  If swallowed, the ball &
	 *  chain variables will be set at the next call to placebc().
	 */
	if (!u.uswallow) {
	    placebc();
	    if (Blind) set_bc(1);	/* set up ball and chain variables */
	    newsym(u.ux,u.uy);		/* see ball&chain if can't see self */
	}
}

void
unpunish()
{	    /* remove the ball and chain */
	freeobj(uchain);
	newsym(uchain->ox,uchain->oy);
	dealloc_obj(uchain);
	setworn((struct obj *)0, W_CHAIN);
	uball->spe = 0;
	setworn((struct obj *)0, W_BALL);
}

/* some creatures have special data structures that only make sense in their
 * normal locations -- if the player tries to create one elsewhere, or to revive
 * one, the disoriented creature becomes a zombie
 */
boolean
cant_create(mtype)
int *mtype;
{

	if (*mtype==PM_GUARD || *mtype==PM_SHOPKEEPER
	     || *mtype==PM_ALIGNED_PRIEST || *mtype==PM_ANGEL) {
		*mtype = PM_HUMAN_ZOMBIE;
		return TRUE;
	} else if (*mtype==PM_LONG_WORM_TAIL) {	/* for create_particular() */
		*mtype = PM_LONG_WORM;
		return TRUE;
	}
	return FALSE;
}

#ifdef WIZARD
boolean
create_particular()
{
	char buf[BUFSZ];
	int which, tries = 0;

	do {
/*JP	    getlin("Create what kind of monster? [type the name]", buf);*/
	    getlin("�ɤμ�β�ʪ����ޤ�����[�Ѹ�Ǥ���Ƥ�]", buf);
	    if (buf[0] == '\033') return FALSE;
	    which = name_to_mon(buf);
/*JP	    if (which < 0) pline("I've never heard of such monsters.");*/
	    if (which < 0) pline("���Τ褦�ʲ�ʪ��ʹ�������Ȥ��ʤ���");
	    else break;
	} while (++tries < 5);
	if (tries == 5) pline(thats_enough_tries);
	else {
	    (void) cant_create(&which);
	    return((boolean)(makemon(&mons[which], u.ux, u.uy) != 0));
	}
	return FALSE;
}
#endif /* WIZARD */

#endif /* OVLB */

/*read.c*/
