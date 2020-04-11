/*	SCCS Id: @(#)write.c	3.1	91/01/04
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
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

	switch(otmp->otyp)  {
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
	char namebuf[BUFSZ], scrbuf[BUFSZ];
	register struct obj *new_obj;
	int basecost, actualcost;
	int curseval;
	char qbuf[QBUFSZ];
	
	if(!pen)
		return(0);
	/* already tested before only call of dowrite() (from doapply())
	if(pen->otyp != MAGIC_MARKER)  {
*JP		You("can't write with that!");*
		You("����ǤϽ񤱤ʤ���");
		return(0);
	}
	*/

	/* get paper to write on */
/*JP	paper = getobj(write_on,"write on");*/
	paper = getobj(write_on,"n��");
	if(!paper)
		return(0);
	if(Blind && !paper->dknown) {
/*JP		You("don't know if that %s is blank or not!",
		      paper->oclass == SPBOOK_CLASS ? "spellbook" :
		      "scroll");*/
		You("%s����椫�ɤ����狼��ʤ���",
		      paper->oclass == SPBOOK_CLASS ? "��ˡ��" :
		      "��ʪ");
		return(1);
	}
	paper->dknown = 1;
	if(paper->otyp != SCR_BLANK_PAPER && paper->otyp != SPE_BLANK_PAPER) {
/*JP		pline("That %s is not blank!",
		    paper->oclass == SPBOOK_CLASS ? "spellbook" :
		    "scroll");*/
		pline("����%s����椸��ʤ���",
		    paper->oclass == SPBOOK_CLASS ? "��ˡ��" :
		    "��ʪ");
		exercise(A_WIS, FALSE);
		return(1);
	}

	/* what to write */
/*JP	Sprintf(qbuf, "What type of %s do you want to write? ",
	      paper->oclass == SPBOOK_CLASS ? "spellbook" :
	      "scroll");*/
	Sprintf(qbuf, "�ɤμ��%s�μ�ʸ��񤭤ޤ�����",
	      paper->oclass == SPBOOK_CLASS ? "��ˡ��" :
	      "��ʪ");
	getlin(qbuf, namebuf);
	if(namebuf[0] == '\033' || !namebuf[0])
		return(1);
	scrbuf[0] = '\0';
	if (paper->oclass == SPBOOK_CLASS) {
		if(strncmp(namebuf,"spellbook of ",13) != 0)
			Strcpy(scrbuf,"spellbook of ");
	}
	else if(strncmp(namebuf,"scroll of ",10) != 0)
		Strcpy(scrbuf,"scroll of ");
	Strcat(scrbuf,namebuf);
	new_obj = readobjnam(scrbuf);

	new_obj->bknown = (paper->bknown && pen->bknown);

	if((new_obj->oclass != SCROLL_CLASS ||
	              new_obj->otyp == SCR_BLANK_PAPER)
	    && (new_obj->oclass != SPBOOK_CLASS || 
                      new_obj->otyp == SPE_BLANK_PAPER)) {
/*JP		You("can't write that!");*/
		pline("��������񤯡���");
/*JP		pline("It's obscene!");*/
		pline("����������꤫���Ϥ���ä����������ʡ�");
		obfree(new_obj, (struct obj *) 0); /* pb@ethz.uucp */
		return(1);
	}

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
/*JP			pline("The spellbook is left unfinished.");*/
			pline("��ˡ��ˤϽ񤭤���ʤ��ä���");
		else {
/*JP			pline("The scroll is now useless and disappears!");*/
			pline("��ʪ�ϻȤ���Τˤʤ�ʤ��äƾ��Ǥ�����");
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
	   (rnl(pl_character[0] == 'W' ? 3 : 15))) {
/*JP		You("don't know how to write that!");*/
		You("�����ɤ���äƽ񤯤Τ��Τ�ʤ���");
		/* scrolls disappear, spellbooks don't */
		if (paper->oclass == SPBOOK_CLASS)
/*JP			You("write in your best handwriting:  \"My Diary\".");*/
			You("�Ǥ��������ǫ�˼��ǽ񤤤��ֻ�������ס�");
		else {
/*JP			You("write \"%s was here!\" and the scroll disappears.",plname);*/
			You("��%s�Ϥ����ˤ������פȽ񤤤�������ȴ�ʪ�Ͼä��Ƥ��ޤä���",plname);
			useup(paper);
		}
		obfree(new_obj, (struct obj *) 0);
		return(1);
	}

	/* useup old scroll / spellbook */
	useup(paper);

	/* now you know it! */
	makeknown(new_obj->otyp);

	/* success */
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
	if (new_obj) new_obj->known = 1;
	return(1);
}

/*write.c*/
