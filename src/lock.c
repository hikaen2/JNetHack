/*	SCCS Id: @(#)lock.c	3.1	93/06/12	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

#define CONTAINER_BITS 0	/* box options not [yet?] implemented */

STATIC_PTR int NDECL(picklock);
STATIC_PTR int NDECL(forcelock);

STATIC_VAR NEARDATA struct xlock_s {
	int	door_or_box, picktyp;
	struct rm  *door;
	struct obj *box;
	int chance, usedtime;
} xlock;

#ifdef OVLB

static const char *NDECL(lock_action);
static boolean FDECL(obstructed,(int,int));
static void FDECL(chest_shatter_msg, (struct obj *));

boolean
picking_lock(x, y)
	int *x, *y;
{
	if(occupation != picklock) return 0;
	else {
		*x = u.ux + u.dx;
		*y = u.uy + u.dy;
		return 1;
	}
}

boolean
picking_at(x, y)
int x, y;
{
	return((boolean)((occupation == picklock) && 
	       xlock.door_or_box && (xlock.door == &levl[x][y]))); 
}

/* produce an occupation string appropriate for the current activity */
static const char *
lock_action()
{
	/* "unlocking"+2 == "locking" */
	static const char *actions[] = {
/*JP		* [0] *	"unlocking the door", */
/*JP		* [1] *	"unlocking the chest",*/
/*JP		* [2] *	"unlocking the box",  */
/*JP		* [3] *	"picking the lock"    */
		/* [0] */	"��θ���Ϥ���", 
		/* [1] */	"��Ȣ�θ���Ϥ���",
		/* [2] */	"Ȣ�θ���Ϥ���",  
		/* [3] */	"����Ϥ���"    
	};

	/* if the target is currently unlocked, we're trying to lock it now */
	if (xlock.door_or_box && !(xlock.door->doormask & D_LOCKED))
/*JP		return actions[0]+2;	/* "locking the door" */
		return "��˸��򤫤���";
	else if (!xlock.door_or_box && !xlock.box->olocked)
/*JP		return xlock.box->otyp == CHEST ? actions[1]+2 : actions[2]+2;*/
		return xlock.box->otyp == CHEST ? "��Ȣ�˸��򤫤���" : "Ȣ�˸��򤫤���";
	/* otherwise we're trying to unlock it */
	else if (xlock.picktyp == LOCK_PICK)
		return actions[3];	/* "picking the lock" */
#ifdef TOURIST
	else if (xlock.picktyp == CREDIT_CARD)
		return actions[3];	/* same as lock_pick */
#endif
	else if (xlock.door_or_box)
		return actions[0];	/* "unlocking the door" */
	else
		return xlock.box->otyp == CHEST ? actions[1] : actions[2];
}

STATIC_PTR
int
picklock()	/* try to open/close a lock */
{

	if(!xlock.door_or_box) {	/* box */

	    if((xlock.box->ox != u.ux) || (xlock.box->oy != u.uy)) {
		return((xlock.usedtime = 0));		/* you or it moved */
	    }
	} else {		/* door */
	    if(xlock.door != &(levl[u.ux+u.dx][u.uy+u.dy])) {
		return((xlock.usedtime = 0));		/* you moved */
	    }
	    switch (xlock.door->doormask) {
		case D_NODOOR:
/*JP		    pline("This doorway has no door.");*/
	            pline("�������ˤ��⤬�ʤ���");
		    return((xlock.usedtime = 0));
		case D_ISOPEN:
/*JP		    pline("Picking the lock of an open door is pointless.");*/
		    pline("�����Ƥ���θ���Ϥ����ʤ�Ƥɤ������Ƥ�补");
		    return((xlock.usedtime = 0));
		case D_BROKEN:
/*JP		    pline("This door is broken.");*/
		    pline("��ϲ���Ƥ���");
		    return((xlock.usedtime = 0));
	    }
	}

	if(xlock.usedtime++ >= 50
#ifdef POLYSELF
	   || nohands(uasmon)
#endif
	   ) {
/*JP	    You("give up your attempt at %s.", lock_action());*/
	    pline("%s�Τ򤢤���᤿��", lock_action());
	    exercise(A_DEX, TRUE);	/* even if you don't succeed */
	    return((xlock.usedtime = 0));
	}

	if(rn2(100) > xlock.chance) return(1);		/* still busy */

/*JP	You("succeed in %s.", lock_action());*/
	pline("%s�Τ�����������",lock_action());
	if(xlock.door_or_box) {
	    if(xlock.door->doormask & D_TRAPPED) {
/*JP		    b_trapped("door", FINGER);*/
		    b_trapped("��", FINGER);
		    xlock.door->doormask = D_NODOOR;
		    unblock_point(u.ux+u.dx, u.uy+u.dy);
		    newsym(u.ux+u.dx, u.uy+u.dy);
	    } else if(xlock.door->doormask == D_LOCKED)
		xlock.door->doormask = D_CLOSED;
	    else xlock.door->doormask = D_LOCKED;
	} else {
	    xlock.box->olocked = !xlock.box->olocked;
	    if(xlock.box->otrapped)	
		(void) chest_trap(xlock.box, FINGER, FALSE);
	}
	exercise(A_DEX, TRUE);
	return((xlock.usedtime = 0));
}

STATIC_PTR
int
forcelock()	/* try to force a locked chest */
{

	register struct obj *otmp, *otmp2;

	if((xlock.box->ox != u.ux) || (xlock.box->oy != u.uy))
		return((xlock.usedtime = 0));		/* you or it moved */

	if(xlock.usedtime++ >= 50 || !uwep
#ifdef POLYSELF
	   || nohands(uasmon)
#endif
	   ) {
/*JP	    You("give up your attempt to force the lock.");*/
	    pline("�������������Τ򤢤���᤿��");
	    if(xlock.usedtime >= 50)		/* you made the effort */
	      exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
	    return((xlock.usedtime = 0));
	}

	if(xlock.picktyp) {	/* blade */

	    if(rn2(1000-(int)uwep->spe) > (992-(int)uwep->oeroded*10) && 
	       !uwep->cursed && !obj_resists(uwep, 0, 99)) {
		/* for a +0 weapon, probability that it survives an unsuccessful
		 * attempt to force the lock is (.992)^50 = .67
		 */
/*JP
		pline("%sour %s broke!",
		      (uwep->quan > 1L) ? "One of y" : "Y", xname(uwep));
*/
	        pline("���ʤ���%s�ϲ���Ƥ��ޤä���",xname(uwep));
		useup(uwep);
/*JP		You("give up your attempt to force the lock.");*/
		pline("�������������Τ򤢤���᤿��");
		exercise(A_DEX, TRUE);
		return((xlock.usedtime = 0));
	    }
	} else			/* blunt */
	    wake_nearby();	/* due to hammering on the container */

	if(rn2(100) > xlock.chance) return(1);		/* still busy */

/*JP	You("succeed in forcing the lock.");*/
	pline("���������������");
	xlock.box->olocked = 0;
	xlock.box->obroken = 1;
	if(!xlock.picktyp && !rn2(3)) {
	    struct monst *shkp;
	    boolean costly;
	    long loss = 0L;

	    costly = (*u.ushops && costly_spot(u.ux, u.uy));
	    shkp = costly ? shop_keeper(*u.ushops) : 0;

/*JP	    pline("In fact, you've totally destroyed %s.",
		  the(xname(xlock.box)));*/
	    pline("%s�����˲����Ƥ��ޤä���",
		  the(xname(xlock.box)));

	    /* Put the contents on ground at the hero's feet. */
	    for (otmp = xlock.box->cobj; otmp; otmp = otmp2) {
		otmp2 = otmp->nobj;
		if(!rn2(3) || otmp->oclass == POTION_CLASS) {
		    chest_shatter_msg(otmp);
		    if (costly)
		        loss += stolen_value(otmp, u.ux, u.uy,
					     (boolean)shkp->mpeaceful, TRUE);
		    if (otmp->quan == 1L) {
			obfree(otmp, (struct obj *) 0);
			continue;
		    }
		    useup(otmp);
		}
		place_object(otmp,u.ux,u.uy);
		otmp->nobj = fobj;
		fobj = otmp;
		stackobj(otmp);
	    }
	    xlock.box->cobj = (struct obj *) 0;	/* no contents */
	    if (costly)
		loss += stolen_value(xlock.box, u.ux, u.uy,
					     (boolean)shkp->mpeaceful, TRUE);
/*JP	    if(loss) You("owe %ld zorkmids for objects destroyed.", loss);*/
	    if(loss) You("��ʪ��»��%ld������ɤμڤ��Ĥ��ä���", loss);
	    delobj(xlock.box);
	}
	exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
	return((xlock.usedtime = 0));
}

#endif /* OVLB */
#ifdef OVL0

void
reset_pick() { xlock.usedtime = 0; }

#endif /* OVL0 */
#ifdef OVLB

int
pick_lock(pick) /* pick a lock with a given object */
	register struct	obj	*pick;
{
	register int x, y, picktyp, c, ch;
	register struct rm	*door;
	register struct obj	*otmp;
	char qbuf[QBUFSZ];

#ifdef GCC_WARN
	ch = 0;		/* GCC myopia */
#endif
	picktyp = pick->otyp;
	if (xlock.usedtime && picktyp == xlock.picktyp) {
		const char *action = lock_action();

/*JP		You("resume your attempt at %s.", action);*/
		pline("%s��Ƴ�������", action);
		set_occupation(picklock, action, 0);
		return(1);
	}

#ifdef POLYSELF
	if(nohands(uasmon)) {
/*JP		You("can't hold a %s - you have no hands!", xname(pick));*/
		You("%s��Ĥ��ळ�Ȥ��Ǥ��ʤ���--���ʤ��ˤϼ꤬�ʤ���", xname(pick));
		return(0);
	}
#endif
	if((picktyp != LOCK_PICK &&
#ifdef TOURIST
	    picktyp != CREDIT_CARD &&
#endif
	    picktyp != SKELETON_KEY)) {
		impossible("picking lock with object %d?", picktyp);
		return(0);
	}
	if(!getdir(NULL)) return(0);

	x = u.ux + u.dx;
	y = u.uy + u.dy;
	if((x == u.ux) && (y == u.uy)) { /* pick the lock on a container */
	    c = 'n';			/* in case there are no boxes here */
	    for(otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
		if (Is_box(otmp)) {
		    const char *verb;
		    boolean it = 0;
#if CONTAINER_BITS
		    if (!otmp->oclosed) {	/* it's open */
			pline("There is %s here.", doname(otmp));
			continue;
		    }
		    if (!otmp->lknown) verb = "work";
		    else
#endif
/*JP			 if (otmp->obroken) verb = "fix";*/
			 if (otmp->obroken) verb = "��������";
/*JP		    else if (!otmp->olocked) verb = "lock", it = 1;*/
		    else if (!otmp->olocked) verb = "���򤫤���", it = 1;
/*JP		    else if (picktyp != LOCK_PICK) verb = "unlock", it = 1;*/
		    else if (picktyp != LOCK_PICK) verb = "����Ϥ���", it = 1;
/*JP		    else verb = "pick";*/
		    else verb = "����������";
/*JP		    Sprintf(qbuf, "There is %s here, %s %s?",
			    doname(otmp), verb, it ? "it" : "its lock");*/
		    Sprintf(qbuf, "�����ˤ�%s�����롥%s��",
			    doname(otmp), jconj(verb,"�ޤ���"));

		    c = ynq(qbuf);
		    if(c == 'q') return(0);
		    if(c == 'n') continue;

#if CONTAINER_BITS
		    otmp->lknown = 1;
#endif
		    if (otmp->obroken) {
/*JP			You("can't fix its broken lock with %s.", doname(pick));*/
			You("���줿����%s�ǽ����Ǥ��ʤ���", doname(pick));
			return 0;
		    }
#ifdef TOURIST
		    else if (picktyp == CREDIT_CARD && !otmp->olocked) {
			/* credit cards are only good for unlocking */
/*JP			You("can't do that with %s.", doname(pick));*/
			pline("%s���㤽��ʤ��ȤϤǤ��ʤ���", doname(pick));
			return 0;
		    }
#endif
		    switch(picktyp) {
#ifdef TOURIST
			case CREDIT_CARD:
			    ch = ACURR(A_DEX)+(20*(pl_character[0] == 'R'));
			    break;
#endif
			case LOCK_PICK:
			    ch = 4*ACURR(A_DEX)+(25*(pl_character[0] == 'R'));
			    break;
			case SKELETON_KEY:
			    ch = 75 + ACURR(A_DEX);
			    break;
			default:	ch = 0;
		    }
		    if(otmp->cursed) ch /= 2;

		    xlock.door_or_box = 0;
		    xlock.picktyp = picktyp;
		    xlock.box = otmp;
		    break;
		}
	    if(c != 'y')
		return(0);		/* decided against all boxes */
	} else {			/* pick the lock in a door */
	    struct monst *mtmp;

	    door = &levl[x][y];
	    if ((mtmp = m_at(x, y)) && canseemon(mtmp)
			&& mtmp->m_ap_type != M_AP_FURNITURE
			&& mtmp->m_ap_type != M_AP_OBJECT) {
#ifdef TOURIST
		if (picktyp == CREDIT_CARD &&
		    (mtmp->isshk || mtmp->data == &mons[PM_ORACLE]))
/*JP		    verbalize("No checks, no credit, no problem.");*/
		    verbalize("���Ĥ�˥��˥�����ʧ����");
		else
#endif
/*JP		    pline("I don't think %s would appreciate that.", mon_nam(mtmp));*/
		    pline("%s�ˤ��ڤ˿�����Ȼפ��补", mon_nam(mtmp));
		return(0);
	    }
	    if(!IS_DOOR(door->typ)) {
		if (is_drawbridge_wall(x,y) >= 0)
/*JP		    You("%s no lock on the drawbridge.",
				Blind ? "feel" : "see");*/
		    pline("ķ�Ͷ��ˤϸ����ʤ�%s��",
				Blind ? "�褦��" : "�褦�˸�����");
		else
/*JP		    You("%s no door there.",
				Blind ? "feel" : "see");*/
		    pline("�����ˤ��⤬�ʤ�%s��",
				Blind ? "�褦��" : "�褦�˸�����");
		return(0);
	    }
	    switch (door->doormask) {
		case D_NODOOR:
/*JP		    pline("This doorway has no door.");*/
	            pline("�������ˤ��⤬�ʤ���");
		    return(0);
		case D_ISOPEN:
/*JP		    pline("Picking the lock of an open door is pointless.");*/
		    pline("�����Ƥ���θ���Ϥ����ʤ�Ƥɤ������Ƥ�补");
		    return(0);
		case D_BROKEN:
/*JP		    pline("This door is broken.");*/
		    pline("��ϲ���Ƥ���");
		    return(0);
		default:
#ifdef TOURIST
		    /* credit cards are only good for unlocking */
		    if(picktyp == CREDIT_CARD && !(door->doormask & D_LOCKED)) {
/*JP			You("can't lock a door with a credit card.");*/
		        You("���쥸�åȥ����ɤǸ��򤫤���Τ˼��Ԥ�����");
			return(0);
		    }
#endif

/*JP		    Sprintf(qbuf,"%sock it?",
			(door->doormask & D_LOCKED) ? "Unl" : "L" );*/
		    Sprintf(qbuf,"%s��",
			(door->doormask & D_LOCKED) ? "�Ϥ����ޤ���" : "�����ޤ���" );

		    c = yn(qbuf);
		    if(c == 'n') return(0);

		    switch(picktyp) {
#ifdef TOURIST
			case CREDIT_CARD:
			    ch = 2*ACURR(A_DEX)+(20*(pl_character[0] == 'R'));
			    break;
#endif
			case LOCK_PICK:
			    ch = 3*ACURR(A_DEX)+(30*(pl_character[0] == 'R'));
			    break;
			case SKELETON_KEY:
			    ch = 70 + ACURR(A_DEX);
			    break;
			default:    ch = 0;
		    }
		    xlock.door_or_box = 1;
		    xlock.door = door;
	    }
	}
	flags.move = 0;
	xlock.chance = ch;
	xlock.picktyp = picktyp;
	xlock.usedtime = 0;
	set_occupation(picklock, lock_action(), 0);
	return(1);
}

int
doforce()		/* try to force a chest with your weapon */
{
	register struct obj *otmp;
	register int c, picktyp;
	char qbuf[QBUFSZ];

	if(!uwep ||	/* proper type test */
	   (uwep->oclass != WEAPON_CLASS && uwep->oclass != ROCK_CLASS &&
						uwep->otyp != PICK_AXE) ||
	   (uwep->otyp < BOOMERANG) ||
	   (uwep->otyp > AKLYS && uwep->oclass != ROCK_CLASS &&
						uwep->otyp != PICK_AXE)
#ifdef KOPS
	   || uwep->otyp == RUBBER_HOSE
#endif
	  ) {
/*JP	    You("can't force anything without a %sweapon.",
		  (uwep) ? "proper " : "");*/
	    pline("%s���ʤ��Ǹ��򤳤������뤳�ȤϤǤ��ʤ���",
		  (uwep) ? "Ŭ�ڤ�" : "");
	    return(0);
	}

	picktyp = is_blade(uwep);
	if(xlock.usedtime && xlock.box && picktyp == xlock.picktyp) {
/*JP	    You("resume your attempt to force the lock.");*/
	    pline("���򤳤�������Τ�Ƴ�������");
/*JP	    set_occupation(forcelock, "forcing the lock", 0);*/
	    set_occupation(forcelock, "���򤳤�������", 0);
	    return(1);
	}

	/* A lock is made only for the honest man, the thief will break it. */
	xlock.box = (struct obj *)0;
	for(otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere)
	    if(Is_box(otmp)) {
#if CONTAINER_BITS
		if (!otmp->oclosed) {	/* it's open */
		    pline("There is %s here.", doname(otmp));
		    continue;
		}
		if (otmp->obroken || (otmp->lknown && !otmp->olocked)) {
#else
		if (otmp->obroken || !otmp->olocked) {
#endif
/*JP		    pline("There is %s here, but its lock is already %s.",*/
		    pline("�����ˤ�%s�����롤���������θ��Ϥ⤦%s��",
			  doname(otmp), otmp->obroken ? "����Ƥ���" : "�Ϥ�����Ƥ���");
		    continue;
		}
/*JP		Sprintf(qbuf,"There is %s here, force its lock?", doname(otmp));*/
		Sprintf(qbuf,"�����ˤ�%s�����롤���򤳤������ޤ�����", doname(otmp));

		c = ynq(qbuf);
		if(c == 'q') return(0);
		if(c == 'n') continue;

#if CONTAINER_BITS
		if (!otmp->olocked) {	/* lock-status wasn't known */
		    pline("Well what'd'ya know?  It's already unlocked.");
		    otmp->lknown = 1;
		    return 0;
		}
#endif
		if(picktyp)
/*JP		    You("force your %s into a crack and pry.", xname(uwep));*/
		    You("%s�򸰷������ƥ����㥫���㤷����",xname(uwep));
		else
/*JP		    You("start bashing it with your %s.", xname(uwep));*/
		    pline("%s�ǲ���Ĥ�����", xname(uwep));
		xlock.box = otmp;
		xlock.chance = objects[otmp->otyp].oc_wldam * 2;
		xlock.picktyp = picktyp;
		xlock.usedtime = 0;
		break;
	    }

/*JP	if(xlock.box)	set_occupation(forcelock, "forcing the lock", 0);
	else		You("decide not to force the issue.");*/
	if(xlock.box)	set_occupation(forcelock, "���򤳤�������", 0);
	else		pline("�����̵��̣�ʹ԰٤���");
	return(1);
}

int
doopen()		/* try to open a door */
{
	register int x, y;
	register struct rm *door;
	struct monst *mtmp;

	if (u.utrap && u.utraptype == TT_PIT) {
/*JP	    You("can't reach over the edge of the pit.");*/
	    You("����ü�ޤ��Ϥ��ʤ���");
	    return 0;
	}

	if(!getdir(NULL)) return(0);

	x = u.ux + u.dx;
	y = u.uy + u.dy;
	if((x == u.ux) && (y == u.uy)) return(0);

	if ((mtmp = m_at(x,y))				&&
		mtmp->m_ap_type == M_AP_FURNITURE	&&
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp);
	    return(1);
	}

	door = &levl[x][y];

	if(!IS_DOOR(door->typ)) {
		if (is_db_wall(x,y)) {
/*JP		    pline("There is no obvious way to open the drawbridge.");*/
		    pline("ķ�Ͷ��򳫤������餫����ˡ�ʤ�Ƥʤ���");
		    return(0);
		}
/*JP		You("%s no door there.",
				Blind ? "feel" : "see");*/
		pline("�����ˤ���Ϥʤ�%s��",
				Blind ? "�褦��" : "�褦�˸�����");
		return(0);
	}

	if(!(door->doormask & D_CLOSED)) {
	  switch(door->doormask) {
/*JP	     case D_BROKEN: pline("This door is broken."); break;*/
/*JP	     case D_NODOOR: pline("This doorway has no door."); break;*/
/*JP	     case D_ISOPEN: pline("This door is already open."); break;*/
/*JP	     default:	    pline("This door is locked."); break;*/
	     case D_BROKEN: pline("��ϲ���Ƥ��롥"); break;
	     case D_NODOOR: pline("�������ˤ��⤬�ʤ���"); break;
	     case D_ISOPEN: pline("��Ϥ⤦�����Ƥ��롥"); break;
	     default:	    pline("��ˤϸ����ݤ��äƤ���"); break;
	  }
	  return(0);
	}

#ifdef POLYSELF
	if(verysmall(uasmon)) {
/*JP	    pline("You're too small to pull the door open.");*/
	    You("������������򳫤����ʤ���");
	    return(0);
	}
#endif
	/* door is known to be CLOSED */
	if (rnl(20) < (ACURRSTR+ACURR(A_DEX)+ACURR(A_CON))/3) {
/*JP	    pline("The door opens.");*/
	    pline("��ϳ�������");
	    if(door->doormask & D_TRAPPED) {
/*JP		b_trapped("door", FINGER);*/
		b_trapped("��", FINGER);
		door->doormask = D_NODOOR;
	    } else
		door->doormask = D_ISOPEN;
	    if (Blind)
		feel_location(x,y);	/* the hero knows she opened it  */
	    else
		newsym(x,y);
	    unblock_point(x,y);		/* vision: new see through there */
	} else {
	    exercise(A_STR, TRUE);
/*JP	    pline("The door resists!");*/
	    pline("�ʤ��ʤ������ʤ���");
	}

	return(1);
}

static
boolean
obstructed(x,y)
register int x, y;
{
	register struct monst *mtmp = m_at(x, y);

	if(mtmp && mtmp->m_ap_type != M_AP_FURNITURE) {
		if (mtmp->m_ap_type == M_AP_OBJECT) goto objhere;
/*JP		pline("%s stands in the way!", Blind ?
			"Some creature" : Monnam(mtmp));*/
		pline("%s��Ω���դ����äƤ��롥", Blind ?
			"���Ԥ�" : Monnam(mtmp));
		return(TRUE);
	}
	if (OBJ_AT(x, y)) {
/*JP
objhere:	pline("Something's in the way.");*/
objhere:	pline("��������ϩ�ˤ��롥");
		return(TRUE);
	}
	return(FALSE);
}

int
doclose()		/* try to close a door */
{
	register int x, y;
	register struct rm *door;
	struct monst *mtmp;

	if (u.utrap && u.utraptype == TT_PIT) {
/*JP	    You("can't reach over the edge of the pit.");*/
	    You("����ü�ޤ��Ϥ��ʤ���");
	    return 0;
	}

	if(!getdir(NULL)) return(0);

	x = u.ux + u.dx;
	y = u.uy + u.dy;
	if((x == u.ux) && (y == u.uy)) {
/*JP		You("are in the way!");*/
		You("��ϩ��ˤ��롪");
		return(1);
	}

	if ((mtmp = m_at(x,y))				&&
		mtmp->m_ap_type == M_AP_FURNITURE	&& 
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp);
	    return(1);
	}

	door = &levl[x][y];

	if(!IS_DOOR(door->typ)) {
		if (door->typ == DRAWBRIDGE_DOWN)
/*JP		    pline("There is no obvious way to close the drawbridge.");*/
		    pline("ķ�Ͷ����Ĥ��뤢���餫����ˡ�ʤ�Ƥʤ���");
		else
/*JP		    You("%s no door there.",
				Blind ? "feel" : "see");*/
		    pline("��������Ϥʤ�%s��",
				Blind ? "�褦��" : "�褦�˸�����");
		return(0);
	}

	if(door->doormask == D_NODOOR) {
/*JP	    pline("This doorway has no door.");*/
	    pline("�������ˤ��⤬�ʤ���");
	    return(0);
	}

	if(obstructed(x, y)) return(0);

	if(door->doormask == D_BROKEN) {
/*JP	    pline("This door is broken.");*/
	    pline("��ϲ��줿��");
	    return(0);
	}

	if(door->doormask & (D_CLOSED | D_LOCKED)) {
/*JP	    pline("This door is already closed.");*/
	    pline("��Ϥ⤦�Ĥ��Ƥ��롥");
	    return(0);
	}

	if(door->doormask == D_ISOPEN) {
#ifdef POLYSELF
	    if(verysmall(uasmon)) {
/*JP		 pline("You're too small to push the door closed.");*/
		 You("��������������Ĥ���ʤ���");
		 return(0);
 	    }
#endif
	    if (rn2(25) < (ACURRSTR+ACURR(A_DEX)+ACURR(A_CON))/3) {
/*JP		pline("The door closes.");*/
		pline("����Ĥ�����");
		door->doormask = D_CLOSED;
		if (Blind)
		    feel_location(x,y);	/* the hero knows she closed it */
		else
		    newsym(x,y);
		block_point(x,y);	/* vision:  no longer see there */
	    }
	    else {
	        exercise(A_STR, TRUE);
/*JP	        pline("The door resists!");*/
	        pline("�ʤ��ʤ��Ĥޤ�ʤ���");
	    }
	}

	return(1);
}

boolean			/* box obj was hit with spell effect otmp */
boxlock(obj, otmp)	/* returns true if something happened */
register struct obj *obj, *otmp;	/* obj *is* a box */
{
	register boolean res = 0;

	switch(otmp->otyp) {
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
	    if (!obj->olocked) {	/* lock it; fix if broken */
/*JP		pline("Klunk!");*/
		pline("������");
#if CONTAINER_BITS
		obj->lknown = 0;
		obj->oclosed = 1;
#endif
		obj->olocked = 1;
		obj->obroken = 0;
		res = 1;
	    } /* else already closed and locked */
	    break;
	case WAN_OPENING:
	case SPE_KNOCK:
	    if (obj->olocked) {		/* unlock; couldn't be broken */
/*JP		pline("Klick!");*/
		pline("���󥳥�");
#if CONTAINER_BITS
		obj->lknown = 0;
#endif
		obj->olocked = 0;
		res = 1;
	    } else			/* silently fix if broken */
		obj->obroken = 0;
#if CONTAINER_BITS
	    obj->oclosed = 0;		/* now open, unconditionally */
#endif
	    break;
	}
	return res;
}

boolean			/* Door/secret door was hit with spell effect otmp */
doorlock(otmp,x,y)	/* returns true if something happened */
register struct obj *otmp;
int x, y;
{
	register struct rm *door = &levl[x][y];
	boolean res = 1;
	const char *msg = NULL;

	if (door->typ == SDOOR) {
	    if (otmp->otyp == WAN_OPENING || otmp->otyp == SPE_KNOCK) {
		door->typ = DOOR;
		door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
/*JP		if (cansee(x,y)) pline("A section of the wall opens up!");*/
		if (cansee(x,y)) pline("�ɤΰ�������������");
		newsym(x,y);
		return(1);
	    } else
		return(0);
	}

	switch(otmp->otyp) {
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
	    if (obstructed(x,y)) return 0;
	    /* Don't allow doors to close over traps.  This is for pits */
	    /* & trap doors, but is it ever OK for anything else? */
	    if (t_at(x,y)) {
		/* maketrap() clears doormask, so it should be NODOOR */
		pline(
/*JP	"A cloud of dust springs up in the doorway, but quickly dissipates.");*/
	"�ۤ��꤬��ϩ��Ω�����᤿�����������äȸ����ޤ����ӻ��ä���");
		return 0;
	    }

	    switch (door->doormask & ~D_TRAPPED) {
	    case D_CLOSED:
/*JP		msg = "The door locks!";*/
	        msg = "��˸��������ä���";
		break;
	    case D_ISOPEN:
/*JP		msg = "The door swings shut, and locks!";*/
		msg = "��������褯�Ĥޤꡤ���������ä���";
		break;
	    case D_BROKEN:
/*JP		msg = "The broken door reassembles and locks!";*/
		msg = "���줿�⤬���ޤäơ����������ä���";
		break;
	    case D_NODOOR:
		msg =
/*JP		"A cloud of dust springs up and assembles itself into a door!";*/
		"�ۤ��꤬�������ᡤ���ä���ˤʤä���";
		break;
	    default:
		res = 0;
		break;
	    }
	    block_point(x, y);
	    door->doormask = D_LOCKED | (door->doormask & D_TRAPPED);
	    newsym(x,y);
	    break;
	case WAN_OPENING:
	case SPE_KNOCK:
	    if (door->doormask & D_LOCKED) {
/*JP		msg = "The door unlocks!";*/
		msg = "��θ��ϤϤ��줿��";
		door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
	    } else res = 0;
	    break;
	case WAN_STRIKING:
	case SPE_FORCE_BOLT:
	    if (door->doormask & (D_LOCKED | D_CLOSED)) {
		if (door->doormask & D_TRAPPED) {
		    if (MON_AT(x, y))
			(void) mb_trapped(m_at(x,y));
		    else if (flags.verbose) {
			if (cansee(x,y))
/*JP			    pline("KABOOM!!  You see a door explode.");*/
			    pline("����ɡ��󡪤��ʤ����⤬��ȯ�����Τ򸫤���");
			else if (flags.soundok)
/*JP			    You("hear a distant explosion.");*/
			    You("��������ȯ����ʹ������");
		    }
		    door->doormask = D_NODOOR;
		    unblock_point(x,y);
		    newsym(x,y);
		    break;
		}
		door->doormask = D_BROKEN;
		if (flags.verbose) {
		    if (cansee(x,y))
/*JP			pline("The door crashes open!");*/
			pline("��ϲ��쳫������");
		    else if (flags.soundok)
/*JP			You("hear a crashing sound.");*/
			You("����������벻��ʹ������");
		}
		unblock_point(x,y);
		newsym(x,y);
	    } else res = 0;
	    break;
	default: impossible("magic (%d) attempted on door.", otmp->otyp);
	    break;
	}
	if (msg && cansee(x,y)) pline(msg);
	return res;
}

static void
chest_shatter_msg(otmp)
struct obj *otmp;
{
/*JP	const char *disposition, *article = (otmp->quan > 1L) ? "A" : "The";*/
	const char *disposition;
	char *thing;
	long save_Blinded;

	if (otmp->oclass == POTION_CLASS) {
/*JP		You("%s a flask shatter!", Blind ? "hear" : "see");*/
		You("���Ӥ�����%s��", Blind ? "����ʹ����" : "�Τ򸫤�");
		potionbreathe(otmp);
		return;
	}
	/* We have functions for distant and singular names, but not one */
	/* which does _both_... */
	save_Blinded = Blinded;
	Blinded = 1;
	thing = singular(otmp, xname);
	Blinded = save_Blinded;
	switch (objects[otmp->otyp].oc_material) {
/*JP	case PAPER:	disposition = "is torn to shreds";*/
	case PAPER:	disposition = "�����Ǥ��줿";
		break;
/*JP	case WAX:	disposition = "is crushed";*/
	case WAX:	disposition = "�򾲤ˤ֤��ޤ���";
		break;
/*JP	case VEGGY:	disposition = "is pulped";*/
	case VEGGY:	disposition = "�Ϥɤ�ɤ�ˤʤä�";
		break;
/*JP	case FLESH:	disposition = "is mashed";*/
	case FLESH:	disposition = "�Ϥɤ�ɤ�ˤʤä�";
		break;
/*JP	case GLASS:	disposition = "shatters";*/
	case GLASS:	disposition = "�ϳ�줿";
		break;
/*JP	case WOOD:	disposition = "splinters to fragments";*/
	case WOOD:	disposition = "�Ϥ�����ˤʤä�";
		break;
/*JP	default:	disposition = "is destroyed";*/
	default:	disposition = "�ϲ��줿";
		break;
	}
/*JP	pline("%s %s %s!", article, thing, disposition);*/
	pline("%s%s��", thing, disposition);
}

#endif /* OVLB */

/*lock.c*/
