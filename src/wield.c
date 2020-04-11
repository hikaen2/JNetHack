/*	SCCS Id: @(#)wield.c	3.1	92/12/10	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include	"hack.h"

/* elven weapons vibrate warningly when enchanted beyond a limit */
#define is_elven_weapon(optr)	((optr)->otyp == ELVEN_ARROW\
				|| (optr)->otyp == ELVEN_SPEAR\
				|| (optr)->otyp == ELVEN_DAGGER\
				|| (optr)->otyp == ELVEN_SHORT_SWORD\
				|| (optr)->otyp == ELVEN_BROADSWORD\
				|| (optr)->otyp == ELVEN_BOW)

/* Note: setuwep() with a null obj, and uwepgone(), are NOT the same!  Sometimes
 * unwielding a weapon can kill you, and lifesaving will then put it back into
 * your hand.  If lifesaving is permitted to do this, use
 * setwuep((struct obj *)0); otherwise use uwepgone().
 */
void
setuwep(obj)
register struct obj *obj;
{
	setworn(obj, W_WEP);
	/* Note: Explicitly wielding a pick-axe will not give a "bashing"
	 * message.  Wielding one via 'a'pplying it will.
	 */
	if (obj)
		unweapon = ((obj->otyp >= BOW || obj->otyp <= BOOMERANG) &&
			obj->otyp != PICK_AXE && obj->otyp != UNICORN_HORN);
	else
		unweapon = TRUE;	/* for "bare hands" message */
}

void
uwepgone()
{
	if (uwep) {
		setnotworn(uwep);
		unweapon = TRUE;
	}
}

static NEARDATA const char wield_objs[] =
	{ ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, TOOL_CLASS, 0 };

int
dowield()
{
	register struct obj *wep;
	register int res = 0;

	multi = 0;
#ifdef POLYSELF
	if (cantwield(uasmon)) {
/*JP		pline("Don't be ridiculous!");*/
		pline("�Ф��Ф�������");
		return(0);
	}
#endif
/*JP	if (!(wep = getobj(wield_objs, "wield"))) /* nothing */;
	if (!(wep = getobj(wield_objs, "��������"))) /* nothing */;
	else if (uwep == wep)
/*JP		You("are already wielding that!");*/
		You("�⤦������������Ƥ��롪");
	else if (welded(uwep))
		weldmsg(uwep, TRUE);
	else if (wep == &zeroobj) {
	    if (uwep == 0)
/*JP		You("are already empty %s.", body_part(HANDED));*/
		You("�⤦�����������Ƥ��ʤ���");
	    else  {
/*JP	  	You("are empty %s.", body_part(HANDED));*/
	  	You("%s���������", body_part(HAND));
	  	setuwep((struct obj *) 0);
	  	res++;
	    }
	} else if (!uarmg &&
#ifdef POLYSELF
		   !resists_ston(uasmon) &&
#endif
		   (wep->otyp == CORPSE && wep->corpsenm == PM_COCKATRICE)) {
	    /* Prevent wielding cockatrice when not wearing gloves --KAA */
/*JP	    You("wield the cockatrice corpse in your bare %s.",*/
	    You("�����ȥꥹ�λ��Τ�%s�ˤ�����",
			makeplural(body_part(HAND)));
# ifdef POLYSELF
	    if (!(poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM)))
# endif
	    {
/*JP		You("turn to stone...");*/
		You("�Фˤʤä�������");
		killer_format = KILLED_BY;
/*JP		killer="touching a cockatrice corpse";*/
		killer="�����ȥꥹ�λ��Τ˿����";
		done(STONING);
	    }
	} else if (uarms && bimanual(wep))
/*JP	    You("cannot wield a two-handed %s while wearing a shield.",
		is_sword(wep) ? "sword" :
		    wep->otyp == BATTLE_AXE ? "axe" : "weapon");*/
	    pline("����������Ƥ���Ȥ���ξ�������%s�������Ǥ��ʤ���",
		is_sword(wep) ? "��" :
		    wep->otyp == BATTLE_AXE ? "��" : "���");
	else if (wep->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL))
/*JP		You("cannot wield that!");*/
		You("����������Ǥ��ʤ���");
	else if (!wep->oartifact || touch_artifact(wep,&youmonst)) {
		res++;
		if (wep->cursed &&
		    (wep->oclass == WEAPON_CLASS ||
		     wep->otyp == HEAVY_IRON_BALL || wep->otyp == PICK_AXE ||
		     wep->otyp == UNICORN_HORN || wep->otyp == TIN_OPENER)) {
		    const char *tmp = xname(wep), *thestr = "The ";
		    if (strncmp(tmp, thestr, 4) && !strncmp(The(tmp),thestr,4))
			tmp = thestr;
		    else tmp = "";
/*JP		    pline("%s%s %s to your %s!",
			tmp, aobjnam(wep, "weld"),
			(wep->quan == 1L) ? "itself" : "themselves", /* a3 *
			body_part(HAND));*/
		    pline("%s�Ͼ���ˤ��ʤ���%s���������줿��",
			xname(wep), 
			body_part(HAND));
		    wep->bknown = TRUE;
		} else {
			/* The message must be printed before setuwep (since
			 * you might die and be revived from changing weapons),
			 * and the message must be before the death message and
			 * Lifesaved rewielding.  Yet we want the message to
			 * say "weapon in hand", thus this kludge.
			 */
			long dummy = wep->owornmask;
			wep->owornmask |= W_WEP;
			prinv(NULL, wep, 0L);
			wep->owornmask = dummy;
		}
		setuwep(wep);
	}
	return(res);
}

void
erode_weapon(acid_dmg)
boolean acid_dmg;
/* Rust weapon, or corrode it if acid damage is called for */
{
	if(!uwep || uwep->oclass != WEAPON_CLASS) return;	/* %% */
	if (uwep->greased) {
		grease_protect(uwep,NULL,FALSE);
	} else if(uwep->oerodeproof ||
	   (acid_dmg ? !is_corrodeable(uwep) : !is_rustprone(uwep))) {
		if (flags.verbose || !(uwep->oerodeproof && uwep->rknown))
/*JP		    Your("%s not affected.", aobjnam(uwep, "are"));*/
		    Your("%s�ϱƶ�������ʤ���", xname(uwep));
		if (uwep->oerodeproof) uwep->rknown = TRUE;
	} else if (uwep->oeroded < MAX_ERODE) {
/*JP		Your("%s%s!", aobjnam(uwep, acid_dmg ? "corrode" : "rust"),
		     uwep->oeroded+1 == MAX_ERODE ? " completely" :
		     uwep->oeroded ? " further" : "");*/
		Your("%s��%s%s��", xname(uwep),
		     uwep->oeroded+1 == MAX_ERODE ? "������" :
		     uwep->oeroded ? "�����" : "",
		     acid_dmg ? "�忩����" : "���Ӥ�");
		uwep->oeroded++;
	} else
		if (flags.verbose)
/*JP		    Your("%s completely %s.",
			 aobjnam(uwep, Blind ? "feel" : "look"),
			 acid_dmg ? "corroded" : "rusty");*/
		    Your("%s�ϴ�����%s%s��",
			 xname(uwep),
			 acid_dmg ? "�忩����" : "���Ӥ�",
			 Blind ? "�褦��" : "�褦�˸�����");
}

int
chwepon(otmp, amount)
register struct obj *otmp;
register int amount;
{
	register const char *color = Hallucination ? hcolor() :
				     (amount < 0) ? Black : blue;
	register const char *xtime;

	if(!uwep || (uwep->oclass != WEAPON_CLASS && uwep->otyp != PICK_AXE
			&& uwep->otyp != UNICORN_HORN)) {
		char buf[36];

/*JP		Sprintf(buf, "Your %s %s.", makeplural(body_part(HAND)),
			(amount >= 0) ? "twitch" : "itch");*/
		Sprintf(buf, "���ʤ���%s��%s��", makeplural(body_part(HAND)),
			(amount >= 0) ? "�Ҥ��Ĥä�" : "�ॺ�ॺ����");
		strange_feeling(otmp, buf);
		exercise(A_DEX, amount >= 0);
		return(0);
	}

	if(uwep->otyp == WORM_TOOTH && amount >= 0) {
		uwep->otyp = CRYSKNIFE;
/*JP		Your("weapon seems sharper now.");*/
		Your("���Ϥ��Ԥ����������褦����");
		uwep->cursed = 0;
		return(1);
	}

	if(uwep->otyp == CRYSKNIFE && amount < 0) {
		uwep->otyp = WORM_TOOTH;
/*JP		Your("weapon seems duller now.");*/
		Your("���Ϥ���ߤ��ʤä��褦����");
		return(1);
	}

	if (amount < 0 && uwep->oartifact && restrict_name(uwep, ONAME(uwep))) {
	    if (!Blind)
/*JP		Your("%s %s.", aobjnam(uwep, "faintly glow"), color);*/
		Your("%s�Ϥ鷺����%s��������", xname(uwep),jconj_adj(color));
	    return(1);
	}
	/* there is a (soft) upper and lower limit to uwep->spe */
	if(((uwep->spe > 5 && amount >= 0) || (uwep->spe < -5 && amount < 0))
								&& rn2(3)) {
	    if (!Blind)
/*JP	    Your("%s %s for a while and then evaporate%s.",
		 aobjnam(uwep, "violently glow"), color,
		 uwep->quan == 1L ? "s" : "");*/
	    Your("%s�Ϥ��Ф餯�㤷��%s��������ȯ������",
		 xname(uwep), jconj_adj(color));
	    else
/*JP		Your("%s.", aobjnam(uwep, "evaporate"));*/
		Your("%s�Ͼ�ȯ����", xname(uwep));

	    while(uwep)		/* let all of them disappear */
				/* note: uwep->quan = 1 is nogood if unpaid */
		useup(uwep);
	    return(1);
	}
	if (!Blind) {
/*JP	    xtime = (amount*amount == 1) ? "moment" : "while";*/
	    xtime = (amount*amount == 1) ? "���" : "���Ф餯";
/*JP	    Your("%s %s for a %s.",
		 aobjnam(uwep, amount == 0 ? "violently glow" : "glow"),
		 color, xtime);*/
	    Your("%s��%s%s%s��������",
		 xname(uwep), xtime, jconj_adj(color), 
		 amount == 0 ? "�㤷��" : "");
	}
	uwep->spe += amount;
	if(amount > 0) uwep->cursed = 0;

	/*
	 * Enchantment, which normally improves a weapon, has an
	 * addition adverse reaction on Magicbane whose effects are
	 * spe dependent.  Give an obscure clue here.
	 */
	if (uwep->oartifact == ART_MAGICBANE && uwep->spe >= 0) {
/*JP		Your("right %s %sches!",
			body_part(HAND),
			(((amount > 1) && (uwep->spe > 1)) ? "flin" : "it"));*/
		Your("��%s��%s��",
			body_part(HAND),
			(((amount > 1) && (uwep->spe > 1)) ? "�Ҥ�Ҥꤷ��" : "�ॺ�ॺ����"));
	}

	/* an elven magic clue, cookie@keebler */
	if ((uwep->spe > 5)
		&& (is_elven_weapon(uwep) || uwep->oartifact || !rn2(7)))
/*JP	    Your("%s unexpectedly.",
		aobjnam(uwep, "suddenly vibrate"));*/
	    Your("%s�������̤���������",
		xname(uwep));

	return(1);
}

int
welded(obj)
register struct obj *obj;
{
	if (obj && obj == uwep && obj->cursed &&
		  (obj->oclass == WEAPON_CLASS ||
		   obj->otyp == HEAVY_IRON_BALL ||
		   obj->otyp == TIN_OPENER || obj->otyp == PICK_AXE ||
		   obj->otyp == UNICORN_HORN))
	{
		obj->bknown = TRUE;
		return 1;
	}
	return 0;
}

/* The reason for "specific" is historical; some parts of the code used
 * the object name and others just used "weapon"/"sword".  This function
 * replaced all of those.  Which one we use is really arbitrary.
 */
void
weldmsg(obj, specific)
register struct obj *obj;
boolean specific;
{
	char buf[BUFSZ];

	if (specific) {
		long savewornmask = obj->owornmask;
		obj->owornmask &= ~W_WEP;
		Strcpy(buf, Doname2(obj));
		obj->owornmask = savewornmask;
	} else
/*JP		Sprintf(buf, "Your %s%s",
			is_sword(obj) ? "sword" : "weapon",
			plur(obj->quan));*/
		Sprintf(buf, "���ʤ���%s",
			is_sword(obj) ? "��" : "���");
/*JP	Strcat(buf, (obj->quan == 1L) ? " is" : " are");*/
#ifdef POLYSELF
/*JP	Sprintf(eos(buf), " welded to your %s!",
		bimanual(obj) ? (const char *)makeplural(body_part(HAND)) : body_part(HAND));*/
	Sprintf(eos(buf), "��%s�˹�������",
		bimanual(obj) ? (const char *)makeplural(body_part(HAND)) : body_part(HAND));
#else
/*JP	Sprintf(eos(buf), " welded to your hand%s!",
		bimanual(obj) ? "s" : "");*/
	Sprintf(eos(buf), "���˹�������");
#endif
	pline(buf);
}

/*wield.c*/
