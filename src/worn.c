/*	SCCS Id: @(#)worn.c	3.1	93/06/24	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright (c) Issei Numata 1994 
**	changing point is marked `JP' (94/1/6)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

static void FDECL(rel_1_obj, (struct monst *,struct obj *));

const struct worn {
	long w_mask;
	struct obj **w_obj;
} worn[] = {
	{ W_ARM, &uarm },
	{ W_ARMC, &uarmc },
	{ W_ARMH, &uarmh },
	{ W_ARMS, &uarms },
	{ W_ARMG, &uarmg },
	{ W_ARMF, &uarmf },
#ifdef TOURIST
	{ W_ARMU, &uarmu },
#endif
	{ W_RINGL, &uleft },
	{ W_RINGR, &uright },
	{ W_WEP, &uwep },
	{ W_AMUL, &uamul },
	{ W_TOOL, &ublindf },
	{ W_BALL, &uball },
	{ W_CHAIN, &uchain },
	{ 0, 0 }
};

void
setworn(obj, mask)
register struct obj *obj;
long mask;
{
	register const struct worn *wp;
	register struct obj *oobj;

	for(wp = worn; wp->w_mask; wp++) if(wp->w_mask & mask) {
		oobj = *(wp->w_obj);
		if(oobj && !(oobj->owornmask & wp->w_mask))
			impossible("Setworn: mask = %ld.", wp->w_mask);
		if(oobj) {
		    oobj->owornmask &= ~wp->w_mask;
		    /* leave as "x = x <op> y", here and below, for broken
		     * compilers */
		    u.uprops[objects[oobj->otyp].oc_oprop].p_flgs = 
			    u.uprops[objects[oobj->otyp].oc_oprop].p_flgs & 
				~wp->w_mask;
		    if (oobj->oartifact) set_artifact_intrinsic(oobj, 0, mask);
		}
		*(wp->w_obj) = obj;
		if(obj) {
		    obj->owornmask |= wp->w_mask;
		/* prevent getting intrinsics from wielding potions, etc... */
		/* wp_mask should be same as mask at this point */
		    if(obj->oclass == WEAPON_CLASS || mask != W_WEP) {
			u.uprops[objects[obj->otyp].oc_oprop].p_flgs = 
			    u.uprops[objects[obj->otyp].oc_oprop].p_flgs | 
				wp->w_mask;
			if (obj->oartifact) set_artifact_intrinsic(obj,1,mask);
		    } else if(obj->oartifact)
			set_artifact_intrinsic(obj,1,mask);
		}
	}
}

/* called e.g. when obj is destroyed */
void
setnotworn(obj)
register struct obj *obj;
{
	register const struct worn *wp;

	if (!obj) return;
	for(wp = worn; wp->w_mask; wp++)
		if(obj == *(wp->w_obj)) {
			*(wp->w_obj) = 0;
			u.uprops[objects[obj->otyp].oc_oprop].p_flgs = 
				u.uprops[objects[obj->otyp].oc_oprop].p_flgs & 
					~wp->w_mask;
			obj->owornmask &= ~wp->w_mask;
			if (obj->oartifact)
			    set_artifact_intrinsic(obj, 0, wp->w_mask);
		}
}

#ifdef MUSE
int
find_mac(mon)
register struct monst *mon;
{
	register struct obj *obj;
	int base = mon->data->ac;
	long mwflags = mon->misc_worn_check;

	for(obj = mon->minvent; obj; obj = obj->nobj) {
		if (obj->owornmask & mwflags)
			base -= ARM_BONUS(obj);
	}
	return base;
}

/* Wear first object of that type it finds, and never switch unless it
 * has none at all.  This means that monsters with leather armor never
 * switch to plate mail, but it also avoids the overhead of either having 8
 * struct obj *s for every monster in the game, or of doing multiple inventory
 * searches each round using which_armor().
 */
void
m_dowear(mon, creation)
register struct monst *mon;
boolean creation;
{
	register struct obj *obj;

	/* Note the restrictions here are the same as in dowear in do_wear.c
	 * except for the additional restriction on intelligence.  (Players
	 * are always intelligent, even if polymorphed).
	 */
	if (verysmall(mon->data) || nohands(mon->data)) return;
	if (is_animal(mon->data) || mindless(mon->data)) return;

	for(obj = mon->minvent; obj; obj = obj->nobj) {
		long flag;

		if (obj->otyp == AMULET_OF_LIFE_SAVING ||
				obj->otyp == AMULET_OF_REFLECTION)
			flag = W_AMUL;
# ifdef TOURIST
		else if (obj->otyp == HAWAIIAN_SHIRT) flag = W_ARMU;
# endif
		else if (is_cloak(obj)) {
			if (cantweararm(mon->data))
				continue;
			flag = W_ARMC;
		} else if (is_helmet(obj)) flag = W_ARMH;
		else if (is_shield(obj)) {
			if (MON_WEP(mon) && bimanual(MON_WEP(mon)))
				continue;
			flag = W_ARMS;
		} else if (is_gloves(obj)) {
			if (MON_WEP(mon) && MON_WEP(mon)->cursed)
				continue;
			flag = W_ARMG;
		} else if (is_boots(obj)) {
			if (slithy(mon->data) || mon->data->mlet == S_CENTAUR)
				continue;
			flag = W_ARMF;
		} else if (obj->oclass == ARMOR_CLASS) {
			if (cantweararm(mon->data))
				continue;
			flag = W_ARM;
		} else continue;
		if (mon->misc_worn_check & flag) continue;
			/* already wearing one */
		if (!creation && canseemon(mon)) {
/*JP			pline("%s puts on %s.", Monnam(mon),*/
			pline("%s��%s��ȤˤĤ�����", Monnam(mon),
						distant_name(obj, doname));
			mon->mfrozen = objects[obj->otyp].oc_delay;
			if (mon->mfrozen) mon->mcanmove = 0;
		}
		mon->misc_worn_check |= flag;
		obj->owornmask |= flag;
	}
}

struct obj *
which_armor(mon, flag)
struct monst *mon;
long flag;
{
	register struct obj *obj;

	for(obj = mon->minvent; obj; obj = obj->nobj)
		if (obj->owornmask & flag) return obj;
	return((struct obj *)0);
}

static void
rel_1_obj(mon, obj)
struct monst *mon;
struct obj *obj;
{
	struct obj *otmp;
	struct obj *backobj = (struct obj *)0;

	for(otmp = mon->minvent; otmp; otmp = otmp->nobj) {
		if (obj == otmp) {
			if (!backobj) mon->minvent = otmp->nobj;
			else backobj->nobj = otmp->nobj;
			place_object(otmp, mon->mx, mon->my);
			otmp->nobj = fobj;
			fobj = otmp;
			if (cansee(mon->mx, mon->my)) newsym(mon->mx, mon->my);
			return;
		}
		backobj = otmp;
	}
	impossible("%s has %s missing?", Monnam(mon), xname(obj));
}

void
mon_break_armor(mon)
struct monst *mon;
{
	register struct obj *otmp;
	struct permonst *mdat = mon->data;
	boolean vis = cansee(mon->mx, mon->my);
/*JP
	const char *pronoun = him[pronoun_gender(mon)],
			*ppronoun = his[pronoun_gender(mon)];*/

	if (breakarm(mdat)) {
	    if ((otmp = which_armor(mon, W_ARM)) != 0) {
		if (vis)
/*JP		    pline("%s breaks out of %s armor!", Monnam(mon), ppronoun);*/
		    pline("%s�ϳ����֤�Ф���", Monnam(mon));
		else
/*JP		    You("hear a cracking sound.");*/
		    You("�Х�Х�ȸ�������ʹ������");
		mon->misc_worn_check &= ~W_ARM;
		m_useup(mon, otmp);
	    }
	    if ((otmp = which_armor(mon, W_ARMC)) != 0) {
		if (otmp->oartifact) {
		    if (vis)
/*JP			pline("%s cloak falls off!", s_suffix(Monnam(mon)));*/
			pline("%s�Υ��������������", s_suffix(Monnam(mon)));
		    mon->misc_worn_check &= ~W_ARMC;
		    otmp->owornmask &= ~W_ARMC;
		    rel_1_obj(mon, otmp);
		} else {
		    if (vis)
/*JP			pline("%s cloak tears apart!", s_suffix(Monnam(mon)));*/
			pline("%s�Υ������Ϥ��������ˤʤä���", s_suffix(Monnam(mon)));
		    else
/*JP			You("hear a ripping sound.");*/
			You("�ӥ�äȸ�������ʹ������");
		    mon->misc_worn_check &= ~W_ARMC;
		    m_useup(mon, otmp);
		}
	    }
# ifdef TOURIST
	    if ((otmp = which_armor(mon, W_ARMU)) != 0) {
		if (vis)
/*JP		    pline("%s shirt rips to shreds!", s_suffix(Monnam(mon)));*/
		    pline("%s�Υ���ĤϤ��������ˤʤä���", s_suffix(Monnam(mon)));
		else
/*JP		    You("hear a ripping sound.");*/
		    You("�ӥ�äȸ�������ʹ������");
		mon->misc_worn_check &= ~W_ARMU;
		m_useup(mon, otmp);
	    }
# endif
        } else if (sliparm(mdat)) {
	    if ((otmp = which_armor(mon, W_ARM)) != 0) {
		if (vis)
/*JP		    pline("%s armor falls around %s!",
			         s_suffix(Monnam(mon)), pronoun);*/
		    pline("%s�γ��������������", 
			         s_suffix(Monnam(mon)));
		else
/*JP		    You("hear a thud.");*/
		    You("�ɥ���ȸ�������ʹ������");
		mon->misc_worn_check &= ~W_ARM;
		otmp->owornmask &= ~W_ARM;
		rel_1_obj(mon, otmp);
	    }
	    if ((otmp = which_armor(mon, W_ARMC)) != 0) {
		if (vis)
		    if (is_whirly(mon->data))
/*JP			pline("%s cloak falls, unsupported!", */
			pline("%s�Υ������ϻ٤����줺���������", 
			             s_suffix(Monnam(mon)));
		    else
/*JP			pline("%s shrinks out of %s cloak!", Monnam(mon),
								ppronoun);*/
			pline("%s�ϥ��������̤����", Monnam(mon));
		mon->misc_worn_check &= ~W_ARMC;
		otmp->owornmask &= ~W_ARMC;
		rel_1_obj(mon, otmp);
	    }
# ifdef TOURIST
	    if ((otmp = which_armor(mon, W_ARMU)) != 0) {
		if (vis)
		    if (sliparm(mon->data))
/*JP			pline("%s seeps right through %s shirt!",
					Monnam(mon), ppronoun);*/
			pline("%s�ϼ�ʬ�Υ���Ĥˤ��߹������",
					Monnam(mon));
		    else
/*JP			pline("%s becomes much too small for %s shirt!",
					Monnam(mon), ppronoun);*/
			pline("%s�ϼ�ʬ�Υ���Ĥ�ꤺ�äȾ������ʤä���",
					Monnam(mon));
		mon->misc_worn_check &= ~W_ARMU;
		otmp->owornmask &= ~W_ARMU;
		rel_1_obj(mon, otmp);
	    }
# endif
	}
	if (nohands(mdat) || verysmall(mdat)) {
	    if ((otmp = which_armor(mon, W_ARMG)) != 0) {
		if (vis)
/*JP		    pline("%s drops %s gloves%s!", Monnam(mon), ppronoun,
					MON_WEP(mon) ? " and weapon" : "");*/
		    pline("%s�Ͼ���%s�������", Monnam(mon), 
					MON_WEP(mon) ? "�����" : "");
		possibly_unwield(mon);
		mon->misc_worn_check &= ~W_ARMG;
		otmp->owornmask &= ~W_ARMG;
		rel_1_obj(mon, otmp);
	    }
	    if ((otmp = which_armor(mon, W_ARMS)) != 0) {
		if (vis)
/*JP		    pline("%s can no longer hold %s shield!", Monnam(mon),
								ppronoun);*/
		    pline("%s�Ϥ�Ϥ�����Ĥ��Ȥ��Ǥ��ʤ���", Monnam(mon));
		else
/*JP		    You("hear a clank.");*/
		    You("�������ȸ�������ʹ������");
		mon->misc_worn_check &= ~W_ARMS;
		otmp->owornmask &= ~W_ARMS;
		rel_1_obj(mon, otmp);
	    }
	    if ((otmp = which_armor(mon, W_ARMH)) != 0) {
		if (vis)
/*JP		    pline("%s helmet falls to the %s!", */
		    pline("%s�γ���%s���������", 
			  s_suffix(Monnam(mon)), surface(mon->mx, mon->my));
		else
/*JP		    You("hear a clank.");*/
		    You("�������ȸ�������ʹ������");
		mon->misc_worn_check &= ~W_ARMH;
		otmp->owornmask &= ~W_ARMH;
		rel_1_obj(mon, otmp);
	    }
	}
	if (nohands(mdat) || verysmall(mdat) || slithy(mdat) ||
	    mdat->mlet == S_CENTAUR) {
	    if ((otmp = which_armor(mon, W_ARMF)) != 0) {
		if (vis) {
		    if (is_whirly(mon->data))
/*JP			pline("%s boots fall away!", */
			pline("%s�η���æ���������", 
			               s_suffix(Monnam(mon)));
/*JP		    else pline("%s boots %s off %s feet!", */
		    else pline("%s�η���­����%s��", 
			s_suffix(Monnam(mon)),
/*JP			verysmall(mdat) ? "slide" : "are pushed");*/
			verysmall(mdat) ? "��������" : "���äѤ�줿");
		}
		mon->misc_worn_check &= ~W_ARMF;
		otmp->owornmask &= ~W_ARMF;
		rel_1_obj(mon, otmp);
	    }
	}
}
#endif

/*worn.c*/
