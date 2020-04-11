/*	SCCS Id: @(#)spell.c	3.1	93/05/15	*/
/*	Copyright (c) M. Stephenson 1988			  */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

static NEARDATA schar delay;		/* moves left for this spell */
static NEARDATA struct obj *book;	/* last/current book being xscribed */

#define spelluses(spell)	spl_book[spell-1].sp_uses
#define decrnuses(spell)	spl_book[spell-1].sp_uses--
#define spellev(spell)		spl_book[spell-1].sp_lev
#define spellname(spell)	OBJ_NAME(objects[spl_book[spell-1].sp_id])
#define spellid(spell)		spl_book[spell-1].sp_id

static void FDECL(cursed_book, (int));
static void FDECL(deadbook, (struct obj *));
STATIC_PTR int NDECL(learn);
static int NDECL(getspell);
static char FDECL(spellet, (int));
static char NDECL(dospellmenu);

static void
cursed_book(lev)
	register int	lev;
{
	switch(rn2(lev)) {
	case 0:
/*JP		You("feel a wrenching sensation.");*/
		You("�ͤ���줿�褦�ʴ��Ф򴶤�����");
		tele();		/* teleport him */
		break;
	case 1:
/*JP		You("feel threatened.");*/
		You("���ɤ���Ƥ���褦�ʵ���������");
		aggravate();
		break;
	case 2:
		make_blinded(Blinded + rn1(100,250),TRUE);
		break;
	case 3:
		take_gold();
		break;
	case 4:
/*JP		pline("These runes were just too much to comprehend.");*/
		pline("���Υ롼��ʸ�������򤹤�ΤϺ������");
		make_confused(HConfusion + rn1(7,16),FALSE);
		break;
	case 5:
/*JP		pline("The book was coated with contact poison!");*/
		pline("�����ܤ��ܿ������Ǥ�ʤ���Ƥ��롪");
		if (uarmg) {
		    /* Note: at this writing, there are no corrodeable
		     * gloves in the game.  If no one plans on adding
		     * copper gauntlets, most of this could be removed. -3.
		     */
		    if (uarmg->oerodeproof || !is_corrodeable(uarmg)) {
/*JP			Your("gloves seem unaffected.");*/
			Your("����ϱƶ�������ʤ���");
		    } else if (uarmg->oeroded < MAX_ERODE) {
/*JP			Your("gloves corrode%s!",
			     uarmg->oeroded+1 == MAX_ERODE ? " completely" :
			     uarmg->oeroded ? " further" : "");*/
			Your("�����%s�忩������",
			     uarmg->oeroded+1 == MAX_ERODE ? "������" :
			     uarmg->oeroded ? "�����" : "");
			uarmg->oeroded++;
		    } else
/*JP			Your("gloves %s completely corroded.",
			     Blind ? "feel" : "look");*/
			Your("����ϴ������忩����%s��",
			     Blind ? "�褦��" : "�褦�˸�����");
		    break;
		}
		if(Poison_resistance) {
		    losestr(rn1(1,2));
/*JP		    losehp(rnd(6), "contact-poisoned spellbook", KILLED_BY_AN);*/
		    losehp(rnd(6), "�ܿ��Ǥ���ˡ���", KILLED_BY_AN);
		} else {
		    losestr(rn1(4,3));
/*JP		    losehp(rnd(10), "contact-poisoned spellbook", KILLED_BY_AN);*/
		    losehp(rnd(10), "�ܿ��Ǥ���ˡ���", KILLED_BY_AN);
		}
		break;
	case 6:
		if(Antimagic) {
		    shieldeff(u.ux, u.uy);
/*JP		    pline("The book explodes, but you are unharmed!");*/
		    pline("�ܤ���ȯ���������������ʤ��Ͻ��Ĥ��ʤ���");
		} else {
/*JP		    pline("As you read the book, it explodes in your %s!",*/
		    pline("���ʤ����ܤ��ɤ�ȡ�%s��������ȯ������",
			body_part(FACE));
/*JP		    losehp (2*rnd(10)+5, "exploding rune", KILLED_BY_AN);*/
		    losehp (2*rnd(10)+5, "�롼��ʸ������ȯ��", KILLED_BY_AN);
		}
		break;
	default:
		rndcurse();
		break;
	}
	return;
}

/* special effects for The Book of the Dead */
static void
deadbook(book2)
struct obj *book2;
{
/*JP    You("turn the pages of the Book of the Dead...");*/
    You("������ܤΥڡ�����᤯�ä�������");
    makeknown(SPE_BOOK_OF_THE_DEAD);
    if(invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy)) {
	register struct obj *otmp;
	register boolean arti1_primed = FALSE, arti2_primed = FALSE,
			 arti_cursed = FALSE;

	if(book2->cursed) {
/*JP	    pline("The runes appear scrambled.  You can't read them!");*/
	    pline("�롼��ʸ���Ϥ�����ޤ��ˤʤäƤ��ꡤ�ɤळ�Ȥ��Ǥ��ʤ��ä���");
	    return;
	}

	if(!u.uhave.bell || !u.uhave.menorah) {
/*JP	    pline("A chill runs down your %s.", body_part(SPINE));*/
	    pline("���������ʤ���%s�����ä���", body_part(SPINE));
/*JP	    if(!u.uhave.bell) You("hear a faint chime...");*/
	    if(!u.uhave.bell) You("�������ʥ٥�β���ʹ����������");
/*JP	    if(!u.uhave.menorah) pline("Vlad's doppelganger is amused.");*/
	    if(!u.uhave.menorah) pline("Vlad������ϳڤ���Ǥ��롥");
	    return;
 	}

	for(otmp = invent; otmp; otmp = otmp->nobj) {
	    if(otmp->otyp == CANDELABRUM_OF_INVOCATION &&
	       otmp->spe == 7 && otmp->lamplit) {
		if(!otmp->cursed) arti1_primed = TRUE;
		else arti_cursed = TRUE;
	    }
	    if(otmp->otyp == BELL_OF_OPENING &&
	       (moves - otmp->age) < 5L) { /* you rang it recently */
		if(!otmp->cursed) arti2_primed = TRUE;
		else arti_cursed = TRUE;
	    }
	}

	if(arti_cursed) {
/*JP	    pline("The invocation fails!");
	    pline("At least one of your artifacts is cursed...");*/
	    pline("�ü�ǽ�Ϥ�ȯ������ʤ��ä���");
	    pline("�����Ȥ⤢�ʤ�������ΤҤȤĤ������Ƥ��롥����");
	} else if(arti1_primed && arti2_primed) {
	    mkinvokearea();
	    u.uevent.invoked = 1;
	} else {	/* at least one artifact not prepared properly */
/*JP	    You("have a feeling that something is amiss...");*/
	    You("�������ְ�äƤ���褦�ʵ�������������");
	    goto raise_dead;
	}
	return;
    }

    /* when not an invocation situation */
    if(book2->cursed)
raise_dead:
    {
	register struct monst *mtmp;
	coord mm;

/*JP	You("raised the dead!");*/
	You("��Ԥ��ɤ��餻����");
	mm.x = u.ux;
	mm.y = u.uy;
	mkundead(&mm);
	if(!rn2(4))
	    if ((mtmp = makemon(&mons[PM_MASTER_LICH],u.ux,u.uy)) != 0) {
		mtmp->mpeaceful = 0;
		set_malign(mtmp);
	    }
    } else if(book2->blessed) {
	register struct monst *mtmp, *mtmp2;

	for(mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;		/* tamedog() changes chain */
	    if(is_undead(mtmp->data) && cansee(mtmp->mx, mtmp->my)) {
		mtmp->mpeaceful = TRUE;
		if(sgn(mtmp->data->maligntyp) == sgn(u.ualign.type)
		   && distu(mtmp->mx, mtmp->my) < 4)
		    if (mtmp->mtame)
			mtmp->mtame++;
		    else
			(void) tamedog(mtmp, (struct obj *)0);
		else mtmp->mflee = TRUE;
	    }
	}
    } else {
	switch(rn2(3)) {
	case 0: 
/*JP	    Your("ancestors are annoyed with you!"); */
	    Your("���ĤϤ��ʤ��������ʤ褦����");
	    break;
	case 1: 
/*JP	    pline("The headstones in the cemetery begin to move!");*/
	    pline("���Ϥ����Ф�ư���Ϥ��᤿��");
	    break;
	default:
/*JP	    pline("Oh my!  Your name appears in the book!");*/
	    pline("�ʤ�Ƥ��ä��������ʤ���̾�����ܤ˽񤤤Ƥ��롪");
	}
    }
    return;
}

STATIC_PTR
int
learn()
{
	register int	i;
	register unsigned booktype;

	if (delay) {	/* not if (delay++), so at end delay == 0 */
		delay++;
		return(1); /* still busy */
	}
	exercise(A_WIS, TRUE);		/* you're studying. */
	booktype = book->otyp;
	if(booktype == SPE_BOOK_OF_THE_DEAD) {
	    deadbook(book);
	    return(0);
	}

	for (i = 0; i < MAXSPELL; i++)  {
		if (spl_book[i].sp_id == booktype)  {
			if (book->spestudied >= rn1(1,8-spl_book[i].sp_lev)) {
/*JP			    pline("This spellbook is too faint to be read anymore.");*/
			    pline("������ˡ���ʸ�����������Ƥ���ʾ��ɤ�ʤ���");
			    book->otyp = booktype = SPE_BLANK_PAPER;
			    makeknown((int)booktype);
			}
			else if (spl_book[i].sp_uses < 11-spl_book[i].sp_lev) {
/*JP			    Your("knowledge of that spell is keener.");*/
			    Your("������ˡ���Ф����μ��ϸ������ޤ��줿��");
			    spl_book[i].sp_uses += rn1(1,9-spl_book[i].sp_lev);
			    book->spestudied++;
			    exercise(A_WIS, TRUE);	/* extra study */
			} else
/*JP			    You("know that spell quite well already.");*/
			    You("������ˡ����Τ��Ƥ��롥");
			break;
		} else if (spl_book[i].sp_id == NO_SPELL)  {
			spl_book[i].sp_id = booktype;
			spl_book[i].sp_lev = objects[booktype].oc_level;
			/* spells have 1 .. 9-level uses. */
			/* ie 1 or 2 uses w/ most potent */
			spl_book[i].sp_uses = rn1(1,9-spl_book[i].sp_lev);
			book->spestudied++;
/*JP			You("add the spell to your repertoire.");*/
			You("��ˡ���μ��˲ä�����");
			makeknown((int)booktype);
			break;
		}
	}
	if (i == MAXSPELL) impossible("Too many spells memorized!");

	if (book->cursed) {	/* maybe a demon cursed it */
		cursed_book(objects[booktype].oc_level);
	}
        check_unpaid(book);
	book = 0;
	return(0);
}

int
study_book(spellbook)
register struct obj *spellbook;
{
	register int	 booktype = spellbook->otyp;
	register boolean confused = (Confusion != 0);

	if (delay && spellbook == book)
/*JP		You("continue your efforts to memorize the spell.");*/
		You("��ˡ�γؽ���Ƴ�������");
	else {
		switch(booktype)  {

/* blank spellbook */
	case SPE_BLANK_PAPER:
/*JP		pline("This spellbook is all blank.");*/
		pline("������ˡ��Ͽ��������");
		makeknown(SPE_BLANK_PAPER);
		return(1);
/* level 1 spells */
	case SPE_HEALING:
	case SPE_DETECT_MONSTERS:
	case SPE_FORCE_BOLT:
	case SPE_LIGHT:
	case SPE_SLEEP:
	case SPE_KNOCK:
/* level 2 spells */
	case SPE_MAGIC_MISSILE:
	case SPE_CONFUSE_MONSTER:
	case SPE_SLOW_MONSTER:
	case SPE_CURE_BLINDNESS:
	case SPE_CREATE_MONSTER:
	case SPE_DETECT_FOOD:
	case SPE_WIZARD_LOCK:
		delay = -objects[booktype].oc_delay;
		break;
/* level 3 spells */
	case SPE_HASTE_SELF:
	case SPE_CAUSE_FEAR:
	case SPE_CURE_SICKNESS:
	case SPE_DETECT_UNSEEN:
	case SPE_EXTRA_HEALING:
	case SPE_CHARM_MONSTER:
	case SPE_CLAIRVOYANCE:
/* level 4 spells */
	case SPE_LEVITATION:
	case SPE_RESTORE_ABILITY:
	case SPE_INVISIBILITY:
	case SPE_FIREBALL:
	case SPE_DETECT_TREASURE:
		delay = -(objects[booktype].oc_level - 1) * objects[booktype].oc_delay;
		break;
/* level 5 spells */
	case SPE_REMOVE_CURSE:
	case SPE_MAGIC_MAPPING:
	case SPE_CONE_OF_COLD:
	case SPE_IDENTIFY:
	case SPE_DIG:
/* level 6 spells */
	case SPE_TURN_UNDEAD:
	case SPE_POLYMORPH:
	case SPE_CREATE_FAMILIAR:
	case SPE_TELEPORT_AWAY:
		delay = -objects[booktype].oc_level * objects[booktype].oc_delay;
		break;
/* level 7 spells */
	case SPE_CANCELLATION:
	case SPE_FINGER_OF_DEATH:
	case SPE_BOOK_OF_THE_DEAD:
		delay = -8 * objects[booktype].oc_delay;
		break;
/* impossible */
	default:
		impossible("Unknown spellbook, %d;", booktype);
		return(0);
	}

		/* Books are often wiser than their readers (Rus.) */
		if(!spellbook->blessed &&
		        spellbook->otyp != SPE_BOOK_OF_THE_DEAD &&
			(spellbook->cursed ||
			    rn2(20) > (ACURR(A_INT) + 4 + (int)(u.ulevel/2)
					- 2*objects[booktype].oc_level))) {
			cursed_book(objects[booktype].oc_level);
			nomul(delay);			/* study time */
			delay = 0;
			if(!rn2(3)) {
				useup(spellbook);
/*JP				pline("The spellbook crumbles to dust!");*/
				pline("��ˡ��ϿФȤʤä���");
			}
			return(1);
		}
		else if(confused) {
			if(!rn2(3) && 
			    spellbook->otyp != SPE_BOOK_OF_THE_DEAD) {
				useup(spellbook);
/*JP				pline("Being confused you have difficulties in controlling your actions.");*/
				pline("���𤷤Ƥ���Τǡ������������Ȥ򤹤�Τ��񤷤���");
				display_nhwindow(WIN_MESSAGE, FALSE);
/*JP				You("accidentally tear the spellbook to pieces.");*/
				You("���ä��ꡤ��ˡ�����������Ƥ��ޤä���");
			}
			else
/*JP				You("find yourself reading the first line over and over again.");*/
				You("�ǽ�ιԤ��٤ⷫ���֤����ɤ�Ǥ�����");
			nomul(delay);
			delay = 0;
			return(1);
		}

/*JP		You("begin to %s the runes.",
		    spellbook->otyp == SPE_BOOK_OF_THE_DEAD ? "recite" :
		    "memorize");*/
		You("�롼��ʸ����%s���Ϥ��᤿��",
		    spellbook->otyp == SPE_BOOK_OF_THE_DEAD ? "�ž�" :
		    "����");
	}

	book = spellbook;
/*JP	set_occupation(learn, "studying", 0);*/
	set_occupation(learn, "�ؤ�", 0);
	return(1);
}

static int
getspell()
{
	register int	maxs, ilet, i;
	char	 lets[BUFSZ], buf[BUFSZ], qbuf[QBUFSZ];

	if (spl_book[0].sp_id == NO_SPELL)  {

/*JP		You("don't know any spells right now.");*/
		You("��ˡ���Τ�ʤ���");
		return(0);
	} else  {

	    for(maxs = 1; (maxs < MAXSPELL) && (spl_book[maxs].sp_id != NO_SPELL); maxs++);
	    if (maxs >= MAXSPELL)  {

		impossible("Too many spells memorized.");
		return(0);
	    }

	    for(i = 0; (i < maxs) && (i < 26); buf[++i] = 0)  buf[i] = 'a' + i;
	    for(i = 26; (i < maxs) && (i < 52); buf[++i] = 0) buf[i] = 'A' + i - 26;

	    if (maxs == 1)  Strcpy(lets, "a");
	    else if (maxs < 27)  Sprintf(lets, "a-%c", 'a' + maxs - 1);
	    else if (maxs == 27)  Sprintf(lets, "a-z A");
	    else Sprintf(lets, "a-z A-%c", 'A' + maxs - 27);
	    for(;;)  {
/*JP		Sprintf(qbuf, "Cast which spell? [%s ?]", lets);*/
		Sprintf(qbuf, "�ɤ���ˡ�򾧤��롩[%s ?]", lets);
		if ((ilet = yn_function(qbuf, NULL, '\0')) == '?') {
			ilet = dospellmenu();
			if(!ilet)
			    continue;
		}
		if (index(quitchars, ilet))
			return(0);
		else for(i = 0; buf[i] != 0; i++)
		    if(ilet == buf[i])  return(++i);
/*JP		You("don't know that spell.");*/
		You("�������ˡ���Τ�ʤ���");
	    }
	}
}

int
docast()
{
	register int	 spell;

	spell = getspell();
	if (!spell) return(0);

	return(spelleffects(spell,FALSE));
}

int
spelleffects(spell,atme)
register int spell;
boolean atme;
{
	register int energy, damage;
	boolean confused = (Confusion != 0);
	struct obj *pseudo;

	/* note that trying to cast it decrements the # of uses,    */
	/* even if the mage does not have enough food/energy to use */
	/* the spell */
	switch (spelluses(spell)) {
		case 0:
/*JP		    pline ("Curdled magical energy twists through you...");
		    pline ("...you have overloaded and burned out this spell.");*/
		    pline ("�ŷ뤷����ˡ�Υ��ͥ륮�������ʤ����Τ����˥���褦�ˤ����̤���������");
		    pline ("���������ʤ��ϲ���٤Τ��ᤳ����ˡ����פ����Ƥ��ޤä���");
		    make_confused((long)spellev(spell) * 3, FALSE);
		    return(0);
		case 1:
/*JP		    Your("nerves tingle warningly.");*/
		    Your("���ФϷٲ��ǥԥ�ԥꤷ����");
		    break;
		case 2:
/*JP		    pline ("This spell is starting to be over-used.");*/
		    pline ("������ˡ�ϻȤ������Ƥ��롥");
		    break;
		default:
		    break;
	}
	decrnuses(spell);
	energy = spellev(spell) * 7 / 2 - 2;    /* 1 <= energy <= 22 */
	if (u.uhunger <= 100 && spell != SPE_DETECT_FOOD) {
/*JP		You("are too hungry to cast that spell.");*/
		pline("ʢ�����ꤹ������ˡ�򾧤����ʤ���");
		return(0);
	} else if (ACURR(A_STR) < 6)  {
/*JP		You("lack the strength to cast spells.");*/
		pline("���������ʤ�������ˡ�򾧤����ʤ���");
		return(0);
	} else if(check_capacity(
/*JP		"Your concentration falters while carrying so much stuff.")) {*/
		"���������Τ���������ƽ���Ǥ��ʤ���")){
	    return (1);
	}

	if (u.uhave.amulet) {
/*JP		You("feel the amulet draining your energy away.");*/
		pline("����������ʤ��Υ��ͥ륮����ۤ��ȤäƤ���褦�ʵ���������");
		energy *= rnd(3);
	}
	if(energy > u.uen)  {
/*JP		You("don't have enough energy to cast that spell.");*/
		pline("��ˡ�򾧤�������ν�ʬ�ʥ��ͥ륮������äƤ��ʤ���");
		return(0);
	} else {
		if (spell != SPE_DETECT_FOOD)
			morehungry(energy * 10);
		u.uen -= energy;
	}
	flags.botl = 1;

	if (confused ||
	    ((int)(ACURR(A_INT) + Luck) - 3 * spellev(spell)) < 0) {

		if (Hallucination)
/*JP			pline("Far out... a light show!");
		else	pline("The air around you crackles as you goof up.");*/
			pline("�����������");
		else	pline("���ʤ�����������Ȳ��ζ����ϥԥ�ԥꤷ����");
		return(0);
	}
	exercise(A_WIS, TRUE);
/*	pseudo is a temporary "false" object containing the spell stats. */
	pseudo = mksobj(spellid(spell), FALSE, FALSE);
	pseudo->blessed = pseudo->cursed = 0;
	pseudo->quan = 20L;			/* do not let useup get it */
	switch(pseudo->otyp)  {

/* These spells are all duplicates of wand effects */
	case SPE_FORCE_BOLT:
	case SPE_SLEEP:
	case SPE_MAGIC_MISSILE:
	case SPE_KNOCK:
	case SPE_SLOW_MONSTER:
	case SPE_WIZARD_LOCK:
	case SPE_FIREBALL:
	case SPE_CONE_OF_COLD:
	case SPE_DIG:
	case SPE_TURN_UNDEAD:
	case SPE_POLYMORPH:
	case SPE_TELEPORT_AWAY:
	case SPE_CANCELLATION:
	case SPE_FINGER_OF_DEATH:
	case SPE_LIGHT:
	case SPE_DETECT_UNSEEN:
	case SPE_HEALING:
	case SPE_EXTRA_HEALING:
		if (!(objects[pseudo->otyp].oc_dir == NODIR)) {
			if (atme) u.dx = u.dy = u.dz = 0;
			else (void) getdir(NULL);
			if(!u.dx && !u.dy && !u.dz) {
			    if((damage = zapyourself(pseudo)))
				losehp(damage, 
/*JP		self_pronoun("zapped %sself with a spell", "him"),
					NO_KILLER_PREFIX);*/
				       "��ʬ���Ȥ���ˡ����Ӥ�",
					KILLED_BY);
			} else	weffects(pseudo);
		} else weffects(pseudo);
		break;
/* These are all duplicates of scroll effects */
	case SPE_CONFUSE_MONSTER:
	case SPE_DETECT_FOOD:
	case SPE_CAUSE_FEAR:
	case SPE_CHARM_MONSTER:
	case SPE_REMOVE_CURSE:
	case SPE_MAGIC_MAPPING:
	case SPE_CREATE_MONSTER:
	case SPE_IDENTIFY:
		(void) seffects(pseudo);
		break;
	case SPE_HASTE_SELF:
	case SPE_DETECT_TREASURE:
	case SPE_DETECT_MONSTERS:
	case SPE_LEVITATION:
	case SPE_RESTORE_ABILITY:
	case SPE_INVISIBILITY:
		(void) peffects(pseudo);
		break;
	case SPE_CURE_BLINDNESS:
		healup(0, 0, FALSE, TRUE);
		break;
	case SPE_CURE_SICKNESS:
/*JP		if (Sick) You("are no longer ill.");*/
		if (Sick) Your("�µ���ľ�ä���");
		healup(0, 0, TRUE, FALSE);
		break;
	case SPE_CREATE_FAMILIAR:
		make_familiar((struct obj *)0, u.ux, u.uy);
		break;
	case SPE_CLAIRVOYANCE:
		do_vicinity_map();
		break;
	default:
		impossible("Unknown spell %d attempted.", spell);
		obfree(pseudo, (struct obj *)0);
		return(0);
	}
	obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
	return(1);
}

void
losespells() {
	register boolean confused = (Confusion != 0);
	register int	 n, nzap, i;

	book = 0;
	for(n = 0;(spl_book[n].sp_id != NO_SPELL) && (n < MAXSPELL); n++);
	if (!n) return;
	if (n < MAXSPELL) {
		nzap = rnd(n);
		if (nzap < n) nzap += confused;
		for (i = 0; i < nzap; i++) {
		    spl_book[n-i-1].sp_id = NO_SPELL;
		    exercise(A_WIS, FALSE);	/* ouch! */
		}
	} else impossible("Too many spells memorized!");
	return;
}

static char
spellet(spl)
int spl;
{
	return((char)((spl < 27) ? ('a' + spl - 1) : ('A' + spl - 27)));
}

int
dovspell()
{
    (void) dospellmenu();
    return 0;
}

static char
dospellmenu()
{
	winid tmpwin;
	register int maxs, i;
	char rval;
	char     buf[BUFSZ];

	if (spl_book[0].sp_id == NO_SPELL)  {

/*JP		You("don't know any spells right now.");*/
		You("��ˡ���Τ�ʤ���");
		return 0;
	}

	for(maxs = 1; (maxs < MAXSPELL) && (spl_book[maxs].sp_id != NO_SPELL); maxs++);
	if (maxs >= MAXSPELL)  {

		impossible("Too many spells memorized.");
		return 0;
	}
	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
/*JP	add_menu(tmpwin, 0, 0, "Currently known spells:");*/
	add_menu(tmpwin, 0, 0, "�����ΤäƤ�����ˡ����:");
	add_menu(tmpwin, 0, 0, "");

	for(i = 1; i <= maxs; i++) {
/*JP		Sprintf(buf, "%c %c %s (%d)",*/
		Sprintf(buf, "%c %c %s (%d)",
			spellet(i), (spelluses(i)) ? '-' : '*',
/*JP			spellname(i), spellev(i));*/
			jtrns_obj('+',spellname(i)), spellev(i));
		add_menu(tmpwin, spellet(i), 0, buf);
  	}
	end_menu(tmpwin, '\0', "\033 ", NULL);
	rval = select_menu(tmpwin);
	destroy_nhwindow(tmpwin);

	return rval;
}

/*spell.c*/


