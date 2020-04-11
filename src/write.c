/*	SCCS Id: @(#)write.c	3.2	96/05/05	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-1996
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

static int FDECL(cost,(struct obj *));

/*
 * returns basecost of a scroll or a spellbook
 */
static int
cost(otmp)
register struct obj *otmp;
{

	if (otmp->oclass == SPBOOK_CLASS)
		return(10 * objects[otmp->otyp].oc_level);

	switch (otmp->otyp) {
# ifdef MAIL
	case SCR_MAIL:
		return(2);
/*		break; */
# endif
	case SCR_LIGHT:
	case SCR_GOLD_DETECTION:
	case SCR_FOOD_DETECTION:
	case SCR_MAGIC_MAPPING:
	case SCR_AMNESIA:
	case SCR_FIRE:
		return(8);
/*		break; */
	case SCR_DESTROY_ARMOR:
	case SCR_CREATE_MONSTER:
	case SCR_PUNISHMENT:
		return(10);
/*		break; */
	case SCR_CONFUSE_MONSTER:
		return(12);
/*		break; */
	case SCR_IDENTIFY:
		return(14);
/*		break; */
	case SCR_ENCHANT_ARMOR:
	case SCR_REMOVE_CURSE:
	case SCR_ENCHANT_WEAPON:
	case SCR_CHARGING:
		return(16);
/*		break; */
	case SCR_SCARE_MONSTER:
	case SCR_TAMING:
	case SCR_TELEPORTATION:
		return(20);
/*		break; */
	case SCR_GENOCIDE:
		return(30);
/*		break; */
	case SCR_BLANK_PAPER:
	default:
		impossible("You can't write such a weird scroll!");
	}
	return(1000);
}

static NEARDATA const char write_on[] = { SCROLL_CLASS, SPBOOK_CLASS, 0 };

int
dowrite(pen)
register struct obj *pen;
{
	register struct obj *paper;
	char namebuf[BUFSZ], *nm, *bp;
	register struct obj *new_obj;
	int basecost, actualcost;
	int curseval;
	char qbuf[QBUFSZ];
	int first, last, i;
	boolean by_descr = FALSE;
	const char *typeword;
/*JP*/
	char tmp[BUFSZ];

	if (nohands(uasmon)) {
/*JP	    You("need hands to be able to write!");*/
	    You("�񤯤���ˤϼ꤬ɬ�פ���");
	    return 0;
	} else if (Glib) {
	    dropx(pen);
/*JP	    pline("%s slips from your %s.", The(xname(pen)),*/
	    pline("%s��%s�����ꤪ������", The(xname(pen)),
			makeplural(body_part(FINGER)));
	    return 1;
	}

	/* get paper to write on */
/*JP	paper = getobj(write_on,"write on");*/
	paper = getobj(write_on,"�˽�");
	if(!paper)
		return(0);
/*JP	typeword = (paper->oclass == SPBOOK_CLASS) ? "spellbook" : "scroll";*/
	typeword = (paper->oclass == SPBOOK_CLASS) ? "��ˡ��" : "��ʪ";
	if(Blind && !paper->dknown) {
/*JP		You("don't know if that %s is blank or not!", typeword);*/
		You("%s����椫�ɤ����狼��ʤ���", typeword);
		return(1);
	}
	paper->dknown = 1;
	if(paper->otyp != SCR_BLANK_PAPER && paper->otyp != SPE_BLANK_PAPER) {
/*JP		pline("That %s is not blank!", typeword);*/
		pline("%s����椸��ʤ���", typeword);
		exercise(A_WIS, FALSE);
		return(1);
	}

	/* what to write */
/*JP	Sprintf(qbuf, "What type of %s do you want to write?", typeword);*/
	Sprintf(qbuf, "�ɤμ��%s�μ�ʸ��񤭤ޤ�����", typeword);
	getlin(qbuf, tmp);

	if(paper->oclass == SPBOOK_CLASS){
/*JP	  if(strlen(tmp)>8 && strcmp(tmp + strlen(tmp) - 8, "����ˡ��"))
	    Strcat(tmp, "����ˡ��");*/
	  Strcpy(namebuf, etrns_obj('+', tmp));
	}
	else{
/*JP	  if(strlen(tmp)>8 && strcmp(tmp + strlen(tmp) - 8, "�δ�ʪ"))
	    Strcat(tmp, "�δ�ʪ");*/
	  Strcpy(namebuf, etrns_obj('?', tmp));
	}

	(void)mungspaces(namebuf);	/* remove any excess whitespace */
	if(namebuf[0] == '\033' || !namebuf[0])
		return(1);
	nm = namebuf;
	if (!strncmpi(nm, "scroll ", 7)) nm += 7;
	else if (!strncmpi(nm, "spellbook ", 10)) nm += 10;
	if (!strncmpi(nm, "of ", 3)) nm += 3;

	if ((bp = strstri(nm, " armour")) != 0) {
		(void)strncpy(bp, " armor ", 7);	/* won't add '\0' */
		(void)mungspaces(bp + 1);	/* remove the extra space */
	}

	first = bases[(int)paper->oclass];
	last = bases[(int)paper->oclass + 1] - 1;
	for (i = first; i <= last; i++) {
		/* extra shufflable descr not representing a real object */
		if (!OBJ_NAME(objects[i])) continue;

		if (!strcmpi(OBJ_NAME(objects[i]), nm))
			goto found;
		if (!strcmpi(OBJ_DESCR(objects[i]), nm)) {
			by_descr = TRUE;
			goto found;
		}
	}

/*JP	pline("There is no such %s!", typeword);*/
	pline("���Τ褦��%s�Ϥʤ���", typeword);
	return 1;
found:

	if (i == SCR_BLANK_PAPER || i == SPE_BLANK_PAPER) {
/*JP		You_cant("write that!");*/
		pline("��������񤯡���");
/*JP		pline("It's obscene!");*/
		pline("����������꤫���Ϥ���ä����������ʡ�");
		return 1;
	} else if (i == SPE_BOOK_OF_THE_DEAD) {
/*JP		pline("No mere dungeon adventurer could write that.");*/
		pline("ñ�ʤ��µ������ȤǤϤ���˽񤱤ʤ���");
		return 1;
	} else if (by_descr && paper->oclass == SPBOOK_CLASS &&
		    !objects[i].oc_name_known) {
		/* can't write unknown spellbooks by description */
		pline(
/*JP		  "Unfortunately you don't have enough information to go on.");*/
		  "�ʤ�Ƥ��ä����������񤯤����ν�ʬ���μ����ʤ���");
		return 1;
	}

	new_obj = mksobj(i, FALSE, FALSE);
	new_obj->bknown = (paper->bknown && pen->bknown);

	/* shk imposes a flat rate per use, not based on actual charges used */
	check_unpaid(pen);

	/* see if there's enough ink */
	basecost = cost(new_obj);
	if(pen->spe < basecost/2)  {
/*JP		Your("marker is too dry to write that!");*/
		Your("�ޡ����ϳ夭�����Ƥ��ꤦ�ޤ��񤱤ʤ��ä���");
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}

	/* we're really going to write now, so calculate cost
	 */
	actualcost = rn1(basecost/2,basecost/2);
	curseval = bcsign(pen) + bcsign(paper);
	exercise(A_WIS, TRUE);
	/* dry out marker */
	if(pen->spe < actualcost)  {
/*JP		Your("marker dries out!");*/
		pline("�񤤤Ƥ�������ǥޡ����ϳ夭���ä���");
		/* scrolls disappear, spellbooks don't */
		if (paper->oclass == SPBOOK_CLASS)
			pline_The(
/*JP		       "spellbook is left unfinished and your writing fades.");*/
			"��ˡ��ˤϽ񤭤���ʤ��ä��������ƽ񤤤�ʸ���Ͼä��Ƥ��ޤä���");

		else {
/*JP			pline_The("scroll is now useless and disappears!");*/
			pline_The("��ʪ�ϻȤ���Τˤʤ�ʤ��ʤäƾ��Ǥ�����");
			useup(paper);
		}
		pen->spe = 0;
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}
	pen->spe -= actualcost;

	/* can't write if we don't know it - unless we're lucky */
	if(!(objects[new_obj->otyp].oc_name_known) &&
	   !(objects[new_obj->otyp].oc_uname) &&
	   (rnl(Role_is('W') ? 3 : 15))) {
/*JP		You("%s to write that!", by_descr ? "fail" : "don't know how");*/
		You("%s��", by_descr ? "�񤯤Τ˼��Ԥ���" : 
		    "�ɤ���äƽ񤯤Τ��Τ�ʤ���");
		/* scrolls disappear, spellbooks don't */
		if (paper->oclass == SPBOOK_CLASS)
			You(
/*JP       "write in your best handwriting:  \"My Diary\", but it quickly fades.");*/
       "��ǫ�˽񤤤����ֲ椬�����ס����������äȸ����֤˾ä��Ƥ��ޤä���");
		else {
			if (by_descr) {
			    Strcpy(namebuf, OBJ_DESCR(objects[new_obj->otyp]));
			    wipeout_text(namebuf, (6+MAXULEV - u.ulevel)/6, 0);
			} else
/*JP			    Sprintf(namebuf, "%s was here!", plname);*/
			    Sprintf(namebuf, "%s�Ϥ����ˤ��ꡪ", plname);
/*JP			You("write \"%s\" and the scroll disappears.", namebuf);*/
		You("��%s�פȽ񤤤�������ȴ�ʪ�Ͼä��Ƥ��ޤä���", namebuf);

			useup(paper);
		}
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}

	/* useup old scroll / spellbook */
	useup(paper);

	/* success */
	if (new_obj->oclass == SPBOOK_CLASS) {
		/* acknowledge the change in the object's description... */
/*JP		pline("The spellbook warps strangely, then turns %s.",
		      OBJ_DESCR(objects[new_obj->otyp]));*/
		pline("��ˡ���̯��ȿ�꤫���ꡢ������%s�ˤʤä�",
		      jtrns_obj('+', OBJ_DESCR(objects[new_obj->otyp])));
	}
	new_obj->blessed = (curseval > 0);
	new_obj->cursed = (curseval < 0);
#ifdef MAIL
	if (new_obj->otyp == SCR_MAIL) new_obj->spe = 1;
#endif
/*JP	new_obj = hold_another_object(new_obj, "Oops!  %s out of your grasp!",
					       The(aobjnam(new_obj, "slip")),*/
	new_obj = hold_another_object(new_obj, "���äȡ�%s�Ϥ��ʤ��μ꤫���������",
					       xname(new_obj),
					       (const char *)0);
	return(1);
}

/*write.c*/
