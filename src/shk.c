/*	SCCS Id: @(#)shk.c	3.2	96/07/20	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "eshk.h"

/*#define DEBUG*/

#define PAY_SOME    2
#define PAY_BUY     1
#define PAY_CANT    0	/* too poor */
#define PAY_SKIP  (-1)
#define PAY_BROKE (-2)

#ifdef KOPS

STATIC_DCL void FDECL(makekops, (coord *));
STATIC_DCL void FDECL(call_kops, (struct monst *,BOOLEAN_P));
# ifdef OVLB
STATIC_DCL void FDECL(kops_gone, (BOOLEAN_P));
# endif /* OVLB */
#endif /* KOPS */

#define IS_SHOP(x)	(rooms[x].rtype >= SHOPBASE)

extern const struct shclass shtypes[];	/* defined in shknam.c */

STATIC_VAR NEARDATA long int followmsg;	/* last time of follow message */

STATIC_DCL void FDECL(setpaid, (struct monst *));
STATIC_DCL long FDECL(addupbill, (struct monst *));
STATIC_DCL void FDECL(pacify_shk, (struct monst *));
STATIC_DCL struct bill_x *FDECL(onbill, (struct obj *, struct monst *, BOOLEAN_P));
STATIC_DCL struct monst *FDECL(next_shkp, (struct monst *, BOOLEAN_P));
STATIC_DCL long FDECL(shop_debt, (struct eshk *));
STATIC_DCL char *FDECL(shk_owns, (char *,struct obj *));
STATIC_DCL char *FDECL(mon_owns, (char *,struct obj *));
STATIC_DCL void FDECL(clear_unpaid,(struct obj *));
STATIC_DCL long FDECL(check_credit, (long, struct monst *));
STATIC_DCL void FDECL(pay, (long, struct monst *));
STATIC_DCL long FDECL(get_cost, (struct obj *, struct monst *));
STATIC_DCL long FDECL(set_cost, (struct obj *, struct monst *));
STATIC_DCL const char *FDECL(shk_embellish, (struct obj *, long));
STATIC_DCL long FDECL(cost_per_charge, (struct monst *,struct obj *));
STATIC_DCL long FDECL(cheapest_item, (struct monst *));
STATIC_DCL int FDECL(dopayobj, (struct monst *, struct bill_x *,
			    struct obj **, int, BOOLEAN_P));
STATIC_DCL long FDECL(stolen_container, (struct obj *, struct monst *, long,
				     BOOLEAN_P));
STATIC_DCL long FDECL(getprice, (struct obj *,BOOLEAN_P));
STATIC_DCL void FDECL(shk_names_obj,
		 (struct monst *,struct obj *,const char *,long,const char *));
STATIC_DCL struct obj *FDECL(bp_to_obj, (struct bill_x *));
STATIC_DCL boolean FDECL(inherits, (struct monst *, int, BOOLEAN_P));
STATIC_DCL void FDECL(set_repo_loc, (struct eshk *));
STATIC_DCL boolean NDECL(angry_shk_exists);
STATIC_DCL void FDECL(rile_shk, (struct monst *));
STATIC_DCL void FDECL(remove_damage, (struct monst *, BOOLEAN_P));
STATIC_DCL void FDECL(sub_one_frombill, (struct obj *, struct monst *));
STATIC_DCL void FDECL(add_one_tobill, (struct obj *, BOOLEAN_P));
STATIC_DCL void FDECL(dropped_container, (struct obj *, struct monst *,
				      BOOLEAN_P));
STATIC_DCL void FDECL(add_to_billobjs, (struct obj *));
STATIC_DCL void FDECL(bill_box_content, (struct obj *, BOOLEAN_P, BOOLEAN_P,
				     struct monst *));

#ifdef OVLB
/*
	invariants: obj->unpaid iff onbill(obj) [unless bp->useup]
		obj->quan <= bp->bquan
 */

STATIC_OVL struct monst *
next_shkp(shkp, withbill)
register struct monst *shkp;
register boolean withbill;
{
	for (; shkp; shkp = shkp->nmon)
	    if (shkp->isshk)
		if (ESHK(shkp)->billct || !withbill) break;

	if (shkp) {
	    if (NOTANGRY(shkp)) {
		if (ESHK(shkp)->surcharge) pacify_shk(shkp);
	    } else {
		if (!ESHK(shkp)->surcharge) rile_shk(shkp);
	    }
	}
	return(shkp);
}

char *
shkname(mtmp)				/* called in do_name.c */
register struct monst *mtmp;
{
	return(ESHK(mtmp)->shknam);
}

void
shkgone(mtmp)				/* called in mon.c */
register struct monst *mtmp;
{
	register struct eshk *eshk = ESHK(mtmp);

	if(on_level(&(eshk->shoplevel), &u.uz)) {
		remove_damage(mtmp, TRUE);
		rooms[eshk->shoproom - ROOMOFFSET].resident
						  = (struct monst *)0;
		if(!search_special(ANY_SHOP))
		    level.flags.has_shop = 0;
	}
	/* make sure bill is set only when the
	 * dead shk is the resident shk.	*/
	if(*u.ushops == eshk->shoproom) {
	    setpaid(mtmp);
	    /* dump core when referenced */
	    ESHK(mtmp)->bill_p = (struct bill_x *) -1000;
	    u.ushops[0] = '\0';
	}
}

void
set_residency(shkp, zero_out)
register struct monst *shkp;
register boolean zero_out;
{
	if (on_level(&(ESHK(shkp)->shoplevel), &u.uz))
	    rooms[ESHK(shkp)->shoproom - ROOMOFFSET].resident =
		(zero_out)? (struct monst *)0 : shkp;
}

void
replshk(mtmp,mtmp2)
register struct monst *mtmp, *mtmp2;
{
	rooms[ESHK(mtmp2)->shoproom - ROOMOFFSET].resident = mtmp2;
	if (inhishop(mtmp) && *u.ushops == ESHK(mtmp)->shoproom) {
		ESHK(mtmp2)->bill_p = &(ESHK(mtmp2)->bill[0]);
	}
}

/* do shopkeeper specific structure munging -dlc */
void
restshk(shkp, ghostly)
struct monst *shkp;
boolean ghostly;
{
    if (u.uz.dlevel) {
	struct eshk *eshkp = ESHK(shkp);

	if (eshkp->bill_p != (struct bill_x *) -1000)
	    eshkp->bill_p = &eshkp->bill[0];
	/* shoplevel can change as dungeons move around */
	/* savebones guarantees that non-homed shk's will be gone */
	if (ghostly) {
	    assign_level(&eshkp->shoplevel, &u.uz);
	    if (ANGRY(shkp) && strncmpi(eshkp->customer, plname, PL_NSIZ))
		pacify_shk(shkp);
	}
    }
}

#endif /* OVLB */
#ifdef OVL3

/* Clear the unpaid bit on all of the objects in the list. */
STATIC_OVL void
clear_unpaid(list)
register struct obj *list;
{
    while (list) {
	if (Has_contents(list)) clear_unpaid(list->cobj);
	list->unpaid = 0;
	list = list->nobj;
    }
}
#endif /*OVL3*/
#ifdef OVLB

STATIC_OVL void
setpaid(shkp)	/* either you paid or left the shop or the shopkeeper died */
register struct monst *shkp;
{
	register struct obj *obj;
	register struct monst *mtmp;

	clear_unpaid(invent);
	clear_unpaid(fobj);
	clear_unpaid(level.buriedobjlist);
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		clear_unpaid(mtmp->minvent);
	for(mtmp = migrating_mons; mtmp; mtmp = mtmp->nmon)
		clear_unpaid(mtmp->minvent);

	while ((obj = billobjs) != 0) {
		obj_extract_self(obj);
		dealloc_obj(obj);
	}
	if(shkp) {
		ESHK(shkp)->billct = 0;
		ESHK(shkp)->credit = 0L;
		ESHK(shkp)->debit = 0L;
		ESHK(shkp)->loan = 0L;
	}
}

STATIC_OVL long
addupbill(shkp)
register struct monst *shkp;
{
	register int ct = ESHK(shkp)->billct;
	register struct bill_x *bp = ESHK(shkp)->bill_p;
	register long total = 0L;

	while(ct--){
		total += bp->price * bp->bquan;
		bp++;
	}
	return(total);
}

#endif /* OVLB */
#ifdef OVL1

#ifdef KOPS
STATIC_OVL void
call_kops(shkp, nearshop)
register struct monst *shkp;
register boolean nearshop;
{
	/* Keystone Kops srt@ucla */
	register boolean nokops;

	if(!shkp) return;

	if (flags.soundok)
/*JP	    pline("An alarm sounds!");*/
	    pline("警報が鳴りひびいた！");

	nokops = ((mvitals[PM_KEYSTONE_KOP].mvflags & G_GONE) &&
		  (mvitals[PM_KOP_SERGEANT].mvflags & G_GONE) &&
		  (mvitals[PM_KOP_LIEUTENANT].mvflags & G_GONE) &&
		  (mvitals[PM_KOP_KAPTAIN].mvflags & G_GONE));

	if(!angry_guards(!flags.soundok) && nokops) {
	    if(flags.verbose && flags.soundok)
/*JP		pline("But no one seems to respond to it.");*/
		pline("しかし誰も応答しなかった．");
	    return;
	}

	if(nokops) return;

	{
	    coord mm;

	    if (nearshop) {
		/* Create swarm around you, if you merely "stepped out" */
		if (flags.verbose)
/*JP		    pline_The("Keystone Kops appear!");*/
		    pline("警備員が現われた！");
		mm.x = u.ux;
		mm.y = u.uy;
		makekops(&mm);
		return;
	    }
	    if (flags.verbose)
/*JP		 pline_The("Keystone Kops are after you!");*/
		 pline("警備員がいる！");
	    /* Create swarm near down staircase (hinders return to level) */
	    mm.x = xdnstair;
	    mm.y = ydnstair;
	    makekops(&mm);
	    /* Create swarm near shopkeeper (hinders return to shop) */
	    mm.x = shkp->mx;
	    mm.y = shkp->my;
	    makekops(&mm);
	}
}
#endif	/* KOPS */

/* x,y is strictly inside shop */
char
inside_shop(x, y)
register xchar x, y;
{
	register char rno;

	rno = levl[x][y].roomno;
	if ((rno < ROOMOFFSET) || levl[x][y].edge || !IS_SHOP(rno-ROOMOFFSET))
	    return(NO_ROOM);
	else
	    return(rno);
}

void
u_left_shop(leavestring, newlev)
register char *leavestring;
register boolean newlev;
{
	register struct monst *shkp;
	register struct eshk *eshkp;
	register long total;

	/*
	 * IF player
	 * ((didn't leave outright) AND
	 *  ((he is now strictly-inside the shop) OR
	 *   (he wasn't strictly-inside last turn anyway)))
	 * THEN (there's nothing to do, so just return)
	 */
	if(!*leavestring &&
	   (!levl[u.ux][u.uy].edge || levl[u.ux0][u.uy0].edge))
	    return;

	shkp = shop_keeper(*u.ushops0);

	if(!shkp || !inhishop(shkp))
				/* shk died, teleported, changed levels... */
	    return;

	eshkp = ESHK(shkp);

	if(!eshkp->billct && !eshkp->debit)	/* bill is settled */
	    return;

	if (!*leavestring && shkp->mcanmove && !shkp->msleep) {
	    /*
	     * Player just stepped onto shop-boundary (known from above logic).
	     * Try to intimidate him into paying his bill
	     */
	    verbalize(NOTANGRY(shkp) ?
/*JP		      "%s!  Please pay before leaving." :
		      "%s!  Don't you leave without paying!",*/
		      "%sさん！帰る前に，お金を払っていただけませんか．" :
		      "%s！帰る前に，金を払え！",
		      plname);
	    return;
	}
	total = (addupbill(shkp) + eshkp->debit);
	if (eshkp->credit >= total) {
/*JP	    Your("credit of %ld zorkmid%s is used to cover your shopping bill.",*/
	    Your("クレジットから%ldゴールドが勘定の支払いに使われた．",
		 eshkp->credit);
	    total = 0L;		/* credit gets cleared by setpaid() */
	} else {
/*JP	    You("escaped the shop without paying!");*/
	    You("金を払わずに店から逃げた！");
	    total -= eshkp->credit;
	}
	setpaid(shkp);
	if (!total) return;

	/* by this point, we know an actual robbery has taken place */
	eshkp->robbed += total;
/*JP	You("stole %ld zorkmid%s worth of merchandise.",*/
	You("雑貨を%ldゴールド分盗んだ．",
	    total);
	if (!Role_is('R'))	/* stealing is unlawful */
	    adjalign(-sgn(u.ualign.type));

	hot_pursuit(shkp);
#ifdef KOPS
	call_kops(shkp, (!newlev && levl[u.ux0][u.uy0].edge));
#else
	(void) angry_guards(FALSE);
#endif
}

void
u_entered_shop(enterstring)
register char *enterstring;
{

	register int rt;
	register struct monst *shkp;
	register struct eshk *eshkp;
	struct obj *pick;
/*JP	static const char no_shk[] = "This shop appears to be deserted.";*/
	static const char no_shk[] = "店は廃虚と化している．";
	static char empty_shops[5];

	if(!*enterstring)
		return;

	if(!(shkp = shop_keeper(*enterstring))) {
	    if (!index(empty_shops, *enterstring) &&
		in_rooms(u.ux, u.uy, SHOPBASE) !=
				  in_rooms(u.ux0, u.uy0, SHOPBASE))
		pline(no_shk);
	    Strcpy(empty_shops, u.ushops);
	    u.ushops[0] = '\0';
	    return;
	}

	eshkp = ESHK(shkp);

	if (!inhishop(shkp)) {
	    /* dump core when referenced */
	    eshkp->bill_p = (struct bill_x *) -1000;
	    if (!index(empty_shops, *enterstring))
		pline(no_shk);
	    Strcpy(empty_shops, u.ushops);
	    u.ushops[0] = '\0';
	    return;
	}

	eshkp->bill_p = &(eshkp->bill[0]);

	if (!eshkp->visitct || strncmpi(eshkp->customer, plname, PL_NSIZ)) {
	    /* You seem to be new here */
	    eshkp->visitct = 0;
	    eshkp->following = 0;
	    (void) strncpy(eshkp->customer,plname,PL_NSIZ);
	    pacify_shk(shkp);
	}

	if (shkp->msleep || !shkp->mcanmove || eshkp->following) /* no dialog */
	    return;

	if (Invis) {
/*JP	    pline("%s senses your presence.", shkname(shkp));*/
	    pline("%sはあなたの存在に気がついた．", shkname(shkp));
/*JP	    verbalize("Invisible customers are not welcome!");*/
	    verbalize("透明なお客さんとは感心しないな！");
	    return;
	}

	rt = rooms[*enterstring - ROOMOFFSET].rtype;

	if (ANGRY(shkp)) {
/*JP	    verbalize("So, %s, you dare return to %s %s?!",*/
	    verbalize("%s！わざわざ%sの%sに戻ってきたのか？！",
		      plname,
		      s_suffix(shkname(shkp)),
		      shtypes[rt - SHOPBASE].name);
	} else if (eshkp->robbed) {
/*JP	    pline("%s mutters imprecations against shoplifters.", shkname(shkp));*/
	    pline("%sは泥棒をののしった．", shkname(shkp));
	} else {
/*JP	    verbalize("Hello, %s!  Welcome%s to %s %s!",*/
	    verbalize("こんにちは%s！%sの%sに%s！",
		      plname,
		      s_suffix(shkname(shkp)),
		      shtypes[rt - SHOPBASE].name,
		      eshkp->visitct++ ? "また来ましたね" : "ようこそ");
	}
	/* can't do anything about blocking if teleported in */
	if (!inside_shop(u.ux, u.uy)) {
	    boolean should_block;

	    if ((pick = carrying(PICK_AXE)) != 0) {
		int cnt = 1;
		/* hack: `pick' already points somewhere into inventory */
		while ((pick = pick->nobj) != 0)
		    if (pick->otyp == PICK_AXE) ++cnt;
		verbalize(NOTANGRY(shkp) ?
/*JP			  "Will you please leave your pick-axe%s outside?" :
			  "Leave the pick-axe%s outside.",*/
		      "つるはしを外に置いてきていただけません？" :
		      "つるはしを外へ置いてこい！",
			  plur(cnt));
		should_block = TRUE;
	    } else {
		should_block = (Fast && sobj_at(PICK_AXE, u.ux, u.uy));
	    }
	    if (should_block) (void) dochug(shkp);  /* shk gets extra move */
	}
	return;
}

/*
   Decide whether two unpaid items are mergable; caller is responsible for
   making sure they're unpaid and the same type of object; we check the price
   quoted by the shopkeeper and also that they both belong to the same shk.
 */
boolean same_price(obj1, obj2)
struct obj *obj1, *obj2;
{
	register struct monst *shkp1, *shkp2;
	register struct bill_x *bp1 = 0, *bp2 = 0;
	register boolean are_mergable = FALSE;

	/* look up the first object by finding shk whose bill it's on */
	for (shkp1 = next_shkp(fmon, TRUE); shkp1;
		shkp1 = next_shkp(shkp1, TRUE))
	    if ((bp1 = onbill(obj1, shkp1, TRUE)) != 0) break;
	/* second object is probably owned by same shk; if not, look harder */
	if (shkp1 && (bp2 = onbill(obj2, shkp1, TRUE)) != 0) {
	    shkp2 = shkp1;
	} else {
	    for (shkp2 = next_shkp(fmon, TRUE); shkp2;
		    shkp2 = next_shkp(shkp2, TRUE))
		if ((bp2 = onbill(obj2, shkp2, TRUE)) != 0) break;
	}

	if (!bp1 || !bp2) impossible("same_price: object wasn't on any bill!");
	else are_mergable = (shkp1 == shkp2 && bp1->price == bp2->price);
	return are_mergable;
}

/*
 * Figure out how much is owed to a given shopkeeper.
 * At present, we ignore any amount robbed from the shop, to avoid
 * turning the `$' command into a way to discover that the current
 * level is bones data which has a shk on the warpath.
 */
STATIC_OVL long
shop_debt(eshkp)
struct eshk *eshkp;
{
	struct bill_x *bp;
	int ct;
	long debt = eshkp->debit;

	for (bp = eshkp->bill_p, ct = eshkp->billct; ct > 0; bp++, ct--)
	    debt += bp->price * bp->bquan;
	return debt;
}

/* called in response to the `$' command */
void
shopper_financial_report()
{
	struct monst *shkp, *this_shkp = shop_keeper(inside_shop(u.ux, u.uy));
	struct eshk *eshkp;
	long amt;
	int pass;

	if (this_shkp &&
	    !(ESHK(this_shkp)->credit || shop_debt(ESHK(this_shkp)))) {
/*JP	    You("have no credit or debt in here.");*/
	    You("クレジットも借金もない．");
	    this_shkp = 0;	/* skip first pass */
	}

	/* pass 0: report for the shop we're currently in, if any;
	   pass 1: report for all other shops on this level. */
	for (pass = this_shkp ? 0 : 1; pass <= 1; pass++)
	    for (shkp = next_shkp(fmon, FALSE);
		    shkp; shkp = next_shkp(shkp->nmon, FALSE)) {
		if ((shkp != this_shkp) ^ pass) continue;
		eshkp = ESHK(shkp);
		if ((amt = eshkp->credit) != 0)
/*JP		    You("have %ld zorkmid%s credit at %s %s.",
			amt, plur(amt), s_suffix(shkname(shkp)),
			shtypes[eshkp->shoptype - SHOPBASE].name);*/
		    You("%ldゴールドのクレジットが%sの%sにある．",
			amt, s_suffix(shkname(shkp)),
			shtypes[eshkp->shoptype - SHOPBASE].name);
		else if (shkp == this_shkp)
/*JP		    You("have no credit in here.");*/
		    You("クレジットはない．");
		if ((amt = shop_debt(eshkp)) != 0)
/*JP		    You("owe %s %ld zorkmid%s.",
			shkname(shkp), amt, plur(amt));*/
		    You("%sに%ldゴールドの借りがある．",
			shkname(shkp), amt);
		else if (shkp == this_shkp)
/*JP		    You("don't owe any money here.");*/
		    pline("この店に借りはない．");
	    }
}

#endif /* OVL1 */
#ifdef OVLB

int
inhishop(mtmp)
register struct monst *mtmp;
{
	return(index(in_rooms(mtmp->mx, mtmp->my, SHOPBASE),
		     ESHK(mtmp)->shoproom) &&
		on_level(&(ESHK(mtmp)->shoplevel), &u.uz));
}

struct monst *
shop_keeper(rmno)
register char rmno;
{
	struct monst *shkp = rmno >= ROOMOFFSET ?
				rooms[rmno - ROOMOFFSET].resident : 0;

	if (shkp) {
	    if (NOTANGRY(shkp)) {
		if (ESHK(shkp)->surcharge) pacify_shk(shkp);
	    } else {
		if (!ESHK(shkp)->surcharge) rile_shk(shkp);
	    }
	}
	return shkp;
}

boolean
tended_shop(sroom)
register struct mkroom *sroom;
{
	register struct monst *mtmp = sroom->resident;

	if (!mtmp)
		return(FALSE);
	else
		return((boolean)(inhishop(mtmp)));
}

STATIC_OVL struct bill_x *
onbill(obj, shkp, silent)
register struct obj *obj;
register struct monst *shkp;
register boolean silent;
{
	if (shkp) {
		register struct bill_x *bp = ESHK(shkp)->bill_p;
		register int ct = ESHK(shkp)->billct;

		while (--ct >= 0)
		    if (bp->bo_id == obj->o_id) {
/*JP			if (!obj->unpaid) pline("onbill: paid obj on bill?");*/
			if (!obj->unpaid) pline("勘定：払う？");
			return bp;
		    } else bp++;
	}
/*JP	if(obj->unpaid & !silent) pline("onbill: unpaid obj not on bill?");*/
	if(obj->unpaid & !silent) pline("勘定：払わない？");
	return (struct bill_x *)0;
}

/* Delete the contents of the given object. */
void
delete_contents(obj)
register struct obj *obj;
{
	register struct obj *curr;

	while ((curr = obj->cobj) != 0) {
	    obj_extract_self(curr);
	    obfree(curr, (struct obj *)0);
	}
}

/* called with two args on merge */
void
obfree(obj, merge)
register struct obj *obj, *merge;
{
	register struct bill_x *bp;
	register struct bill_x *bpm;
	register struct monst *shkp;

	if (obj->otyp == LEASH && obj->leashmon) o_unleash(obj);
	if (obj->oclass == FOOD_CLASS) food_disappears(obj);
	if (Has_contents(obj)) delete_contents(obj);

	shkp = shop_keeper(*u.ushops);

	if ((bp = onbill(obj, shkp, FALSE)) != 0) {
		if(!merge){
			bp->useup = 1;
			obj->unpaid = 0;	/* only for doinvbill */
			add_to_billobjs(obj);
			return;
		}
		bpm = onbill(merge, shkp, FALSE);
		if(!bpm){
			/* this used to be a rename */
			impossible("obfree: not on bill??");
			return;
		} else {
			/* this was a merger */
			bpm->bquan += bp->bquan;
			ESHK(shkp)->billct--;
#ifdef DUMB
			{
			/* DRS/NS 2.2.6 messes up -- Peter Kendell */
				int indx = ESHK(shkp)->billct;
				*bp = ESHK(shkp)->bill_p[indx];
			}
#else
			*bp = ESHK(shkp)->bill_p[ESHK(shkp)->billct];
#endif
		}
	}
	dealloc_obj(obj);
}
#endif /* OVLB */
#ifdef OVL3

STATIC_OVL long
check_credit(tmp, shkp)
long tmp;
register struct monst *shkp;
{
	long credit = ESHK(shkp)->credit;

	if(credit == 0L) return(tmp);
	if(credit >= tmp) {
/*JP		pline_The("price is deducted from your credit.");*/
		pline("代金はクレジットから差し引かれた．");
		ESHK(shkp)->credit -=tmp;
		tmp = 0L;
	} else {
/*JP		pline_The("price is partially covered by your credit.");*/
		pline("代金の一部はあなたのクレジットで補われた．");
		ESHK(shkp)->credit = 0L;
		tmp -= credit;
	}
	return(tmp);
}

STATIC_OVL void
pay(tmp,shkp)
long tmp;
register struct monst *shkp;
{
	long robbed = ESHK(shkp)->robbed;
	long balance = ((tmp <= 0L) ? tmp : check_credit(tmp, shkp));

	u.ugold -= balance;
	shkp->mgold += balance;
	flags.botl = 1;
	if(robbed) {
		robbed -= tmp;
		if(robbed < 0) robbed = 0L;
		ESHK(shkp)->robbed = robbed;
	}
}
#endif /*OVL3*/
#ifdef OVLB

/* return shkp to home position */
void
home_shk(shkp, killkops)
register struct monst *shkp;
register boolean killkops;
{
	register xchar x = ESHK(shkp)->shk.x, y = ESHK(shkp)->shk.y;

	(void) mnearto(shkp, x, y, TRUE);
	level.flags.has_shop = 1;
	if (killkops) {
#ifdef KOPS
		kops_gone(TRUE);
#else
/*JP		You_feel("vaguely apprehensive.");*/
		You("なんとなく不安に思った．");
#endif
		pacify_guards();
	}
}

STATIC_OVL boolean
angry_shk_exists()
{
	register struct monst *shkp;

	for (shkp = next_shkp(fmon, FALSE);
		shkp; shkp = next_shkp(shkp->nmon, FALSE))
	    if (ANGRY(shkp)) return(TRUE);
	return(FALSE);
}

/* remove previously applied surcharge from all billed items */
STATIC_OVL void
pacify_shk(shkp)
register struct monst *shkp;
{
	NOTANGRY(shkp) = TRUE;	/* make peaceful */
	if (ESHK(shkp)->surcharge) {
		register struct bill_x *bp = ESHK(shkp)->bill_p;
		register int ct = ESHK(shkp)->billct;

		ESHK(shkp)->surcharge = FALSE;
		while (ct-- > 0) {
			register long reduction = (bp->price + 3L) / 4L;
			bp->price -= reduction;		/* undo 33% increase */
			bp++;
		}
	}
}

/* add aggravation surcharge to all billed items */
STATIC_OVL void
rile_shk(shkp)
register struct monst *shkp;
{
	NOTANGRY(shkp) = FALSE;	/* make angry */
	if (!ESHK(shkp)->surcharge) {
		register struct bill_x *bp = ESHK(shkp)->bill_p;
		register int ct = ESHK(shkp)->billct;

		ESHK(shkp)->surcharge = TRUE;
		while (ct-- > 0) {
			register long surcharge = (bp->price + 2L) / 3L;
			bp->price += surcharge;
			bp++;
		}
	}
}

void
make_happy_shk(shkp, silentkops)
register struct monst *shkp;
register boolean silentkops;
{
	boolean wasmad = ANGRY(shkp);
	struct eshk *eshkp = ESHK(shkp);

	pacify_shk(shkp);
	eshkp->following = 0;
	eshkp->robbed = 0L;
	if (!Role_is('R'))
		adjalign(sgn(u.ualign.type));
	if(!inhishop(shkp)) {
		char shk_nam[BUFSZ];
		boolean vanished = canseemon(shkp);

		Strcpy(shk_nam, mon_nam(shkp));
		if (on_level(&eshkp->shoplevel, &u.uz)) {
			home_shk(shkp, FALSE);
			/* didn't disappear if shk can still be seen */
			if (canseemon(shkp)) vanished = FALSE;
		} else {
			/* if sensed, does disappear regardless whether seen */
			if (sensemon(shkp)) vanished = TRUE;
			/* arrive near shop's door */
			migrate_to_level(shkp, ledger_no(&eshkp->shoplevel),
					 MIGR_APPROX_XY, &eshkp->shd);
		}
		if (vanished)
/*JP		    pline("Satisfied, %s suddenly disappears!", shk_nam);*/
		    pline("満足すると，%sは突然消えた！", shk_nam);
	} else if(wasmad)
/*JP		pline("%s calms down.", Monnam(shkp));*/
		pline("%sは落着いた．", Monnam(shkp));

	if(!angry_shk_exists()) {
#ifdef KOPS
		kops_gone(silentkops);
#endif
		pacify_guards();
	}
}

void
hot_pursuit(shkp)
register struct monst *shkp;
{
	if(!shkp->isshk) return;

	rile_shk(shkp);
	ESHK(shkp)->following = 1;
}

/* used when the shkp is teleported out of his shop,
 * or when the player is not on a costly_spot and he
 * damages something inside the shop.  these conditions
 * must be checked by the calling function.
 */
void
make_angry_shk(shkp, ox, oy)
register struct monst *shkp;
register xchar ox,oy;
{
	xchar sx, sy;
	struct eshk *eshkp = ESHK(shkp);

	/* all pending shop transactions are now "past due" */
	if (eshkp->billct || eshkp->debit || eshkp->loan || eshkp->credit) {
	    eshkp->robbed += (addupbill(shkp) + eshkp->debit + eshkp->loan);
	    eshkp->robbed -= eshkp->credit;
	    if (eshkp->robbed < 0L) eshkp->robbed = 0L;
	    /* billct, debit, loan, and credit will be cleared by setpaid */
	    setpaid(shkp);
	}

	/* If you just used a wand of teleportation to send the shk away, you
	   might not be able to see her any more.  Monnam would yield "it",
	   which makes this message look pretty silly, so temporarily restore
	   her original location during the call to Monnam. */
	sx = shkp->mx,  sy = shkp->my;
	if (cansee(ox, oy) && !cansee(sx, sy))
		shkp->mx = ox,  shkp->my = oy;
/*JP	pline("%s %s!", Monnam(shkp),
	      !ANGRY(shkp) ? "gets angry" : "is furious");*/
	pline("%sは%s！", Monnam(shkp),
	      !ANGRY(shkp) ? "怒った" : "怒り狂った");
	shkp->mx = sx,  shkp->my = sy;
	hot_pursuit(shkp);
}

/*JP
STATIC_VAR const char no_money[] = "Moreover, you%s have no money.";
STATIC_VAR const char not_enough_money[] =
			    "Besides, you don't have enough to interest %s.";
*/
STATIC_VAR const char no_money[] = "しかしあなたはお金がない%s．";
STATIC_VAR const char not_enough_money[] =
		"しかも，あなたは%sが興味を持つほどお金を持っていない！";


#else
STATIC_VAR const char no_money[];
STATIC_VAR const char not_enough_money[];
#endif /*OVLB*/

#ifdef OVL3

STATIC_OVL long
cheapest_item(shkp)   /* delivers the cheapest item on the list */
register struct monst *shkp;
{
	register int ct = ESHK(shkp)->billct;
	register struct bill_x *bp = ESHK(shkp)->bill_p;
	register long gmin = (bp->price * bp->bquan);

	while(ct--){
		if(bp->price * bp->bquan < gmin)
			gmin = bp->price * bp->bquan;
		bp++;
	}
	return(gmin);
}
#endif /*OVL3*/
#ifdef OVL0

int
dopay()
{
	register struct eshk *eshkp;
	register struct monst *shkp;
	struct monst *nxtm, *resident;
	long ltmp;
	int pass, tmp, shk_pronoun, sk = 0, seensk = 0;
	boolean paid = FALSE, stashed_gold = (hidden_gold() > 0L);

	multi = 0;

	/* find how many shk's there are, how many are in */
	/* sight, and are you in a shop room with one.    */
	nxtm = resident = 0;
	for (shkp = next_shkp(fmon, FALSE);
		shkp; shkp = next_shkp(shkp->nmon, FALSE)) {
	    sk++;
	    if (ANGRY(shkp) && distu(shkp->mx, shkp->my) <= 2) nxtm = shkp;
	    if (canspotmon(shkp)) seensk++;
	    if (inhishop(shkp) && (*u.ushops == ESHK(shkp)->shoproom))
		resident = shkp;
	}

	if (nxtm) {			/* Player should always appease an */
	     shkp = nxtm;		/* irate shk standing next to them. */
	     goto proceed;
	}

	if ((!sk && (!Blind || Telepat)) || (!Blind && !seensk)) {
/*JP      pline("There appears to be no shopkeeper here to receive your payment.");*/
      pline("支払いを受けとる店主はいないようだ．");
		return(0);
	}

	if(!seensk) {
/*JP		You_cant("see...");*/
		You("見ることができない．．．");
		return(0);
	}

	/* the usual case.  allow paying at a distance when */
	/* inside a tended shop.  should we change that?    */
	if(sk == 1 && resident) {
		shkp = resident;
		goto proceed;
	}

	if (seensk == 1) {
		for (shkp = next_shkp(fmon, FALSE);
			shkp; shkp = next_shkp(shkp->nmon, FALSE))
		    if (canspotmon(shkp)) break;
		if (shkp != resident && distu(shkp->mx, shkp->my) > 2) {
/*JP		    pline("%s is not near enough to receive your payment.",*/
		    pline("%sは遠くにいるので支払えない．",
					     Monnam(shkp));
		    return(0);
		}
	} else {
		struct monst *mtmp;
		coord cc;
		int cx, cy;

/*JP		pline("Pay whom?");*/
		pline("誰に払う？");
		cc.x = u.ux;
		cc.y = u.uy;
/*JP		getpos(&cc, TRUE, "the creature you want to pay");*/
		getpos(&cc, TRUE, "支払いたいと思う相手");
		cx = cc.x;
		cy = cc.y;
		if(cx == -10) return(0); /* player pressed esc */
		if(cx < 0) {
/*JP		     pline("Try again...");*/
		     pline("もう一度．．．");
		     return(0);
		}
		if(u.ux == cx && u.uy == cy) {
/*JP		     You("are generous to yourself.");*/
		     pline("自分自身になんて気前のいいこと！");
		     return(0);
		}
		mtmp = m_at(cx, cy);
		if(!mtmp) {
/*JP		     pline("There is no one there to receive your payment.");*/
		     pline("支払いを受けとれる相手はいない．");
		     return(0);
		}
		if(!mtmp->isshk) {
/*JP		     pline("%s is not interested in your payment.",*/
		     pline("%sは支払いに興味を示さない．",
				    Monnam(mtmp));
		     return(0);
		}
		if (mtmp != resident && distu(mtmp->mx, mtmp->my) > 2) {
/*JP		     pline("%s is too far to receive your payment.",*/
		     pline("%sは支払いを受けとるには遠すぎる．",
				    Monnam(mtmp));
		     return(0);
		}
		shkp = mtmp;
	}

	if(!shkp) {
#ifdef DEBUG
		pline("dopay: null shkp.");
#endif
		return(0);
	}
proceed:

	if (shkp->msleep || !shkp->mcanmove) {
/*JP		pline("%s %s.", Monnam(shkp),
		      rn2(2) ? "seems to be napping" : "doesn't respond");*/
		pline("%sは%s．", Monnam(shkp),
		      rn2(2) ? "昼寝をしているようだ．" : "反応がない");
		return 0;
	}
	eshkp = ESHK(shkp);
	shk_pronoun = pronoun_gender(shkp);

	ltmp = eshkp->robbed;
	if(shkp != resident && NOTANGRY(shkp)) {
		if(!ltmp)
/*JP		    You("do not owe %s anything.", mon_nam(shkp));*/
		    You("%sに借りはない．", mon_nam(shkp));
		else if(!u.ugold) {
/*JP		    You("%shave no money.", stashed_gold ? "seem to " : "");*/
		    You("お金がない%s．", stashed_gold ? "ようだ" : "");
		    if(stashed_gold)
/*JP			pline("But you have some gold stashed away.");*/
			pline("しかし，あなたにはちょっっとしたヘソクリがある．");
		} else {
		    long ugold = u.ugold;

		    if(ugold > ltmp) {
/*JP			You("give %s the %ld gold piece%s %s asked for.",
			    mon_nam(shkp), ltmp, plur(ltmp), he[shk_pronoun]);*/
			You("%sに望み通り%ldの金塊を与えた．",
			    mon_nam(shkp), ltmp);
			pay(ltmp, shkp);
		    } else {
/*JP			You("give %s all your%s gold.", mon_nam(shkp),
					stashed_gold ? " openly kept" : "");*/
			You("%sに%sお金全部を与えた．", mon_nam(shkp),
					stashed_gold ? "手持ちの" : "");
			pay(u.ugold, shkp);
/*JP			if (stashed_gold) pline("But you have hidden gold!");*/
			if (stashed_gold) pline("しかし，あなたはヘソクリがある！");
		    }
		    if((ugold < ltmp/2L) || (ugold < ltmp && stashed_gold))
/*JP			pline("Unfortunately, %s doesn't look satisfied.",*/
			pline("残念ながら，%sは満足してないようだ．",
			      he[shk_pronoun]);
		    else
			make_happy_shk(shkp, FALSE);
		}
		return(1);
	}

	/* ltmp is still eshkp->robbed here */
	if (!eshkp->billct && !eshkp->debit) {
		if(!ltmp && NOTANGRY(shkp)) {
/*JP		    You("do not owe %s anything.", mon_nam(shkp));*/
		    You("%sに借りはない．", mon_nam(shkp));
		    if (!u.ugold)
/*JP			pline(no_money, stashed_gold ? " seem to" : "");*/
  		        pline(no_money, stashed_gold ? "ようだ" : "");
		} else if(ltmp) {
/*JP		    pline("%s is after blood, not money!", Monnam(shkp));*/
		    pline("%sは血まみれだ．お金どころじゃない！", Monnam(shkp));
		    if(u.ugold < ltmp/2L ||
				(u.ugold < ltmp && stashed_gold)) {
			if (!u.ugold)
/*JP			    pline(no_money, stashed_gold ? " seem to" : "");*/
			    pline(no_money, stashed_gold ? "ようだ" : "");
			else pline(not_enough_money, him[shk_pronoun]);
			return(1);
		    }
/*JP		    pline("But since %s shop has been robbed recently,",*/
		    pline("しかし，%sの店は最近盗みにあったので，",
			  his[shk_pronoun]);
/*JP		    pline("you %scompensate %s for %s losses.",
			  (u.ugold < ltmp) ? "partially " : "",
			  mon_nam(shkp), his[shk_pronoun]);*/
		    You("%sの損失%sを補填した．", mon_nam(shkp),
			(u.ugold < ltmp) ? "の一部" : "");
		    pay(u.ugold < ltmp ? u.ugold : ltmp, shkp);
		    make_happy_shk(shkp, FALSE);
		} else {
		    /* shopkeeper is angry, but has not been robbed --
		     * door broken, attacked, etc. */
/*JP		    pline("%s is after your hide, not your money!",*/
		    pline("%sはあなたの命を狙っている，お金どころじゃない！",
			  Monnam(shkp));
		    if(u.ugold < 1000L) {
			if (!u.ugold)
/*JP			    pline(no_money, stashed_gold ? " seem to" : "");*/
			    pline(no_money, stashed_gold ? "ようだ" : "");
			else pline(not_enough_money, him[shk_pronoun]);
			return(1);
		    }
/*JP		    You("try to appease %s by giving %s 1000 gold pieces.",
			x_monnam(shkp, 1, "angry", 0), him[shk_pronoun]);*/
		    You("1000ゴールド%sに払ってなだめようとした．",
			x_monnam(shkp, 1, "怒った", 0));

		    pay(1000L,shkp);
		    if (strncmp(eshkp->customer, plname, PL_NSIZ) || rn2(3))
			make_happy_shk(shkp, FALSE);
		    else
/*JP			pline("But %s is as angry as ever.", mon_nam(shkp));*/
			pline("しかし%sはまだ怒っている．", mon_nam(shkp));
		}
		return(1);
	}
	if(shkp != resident) {
		impossible("dopay: not to shopkeeper?");
		if(resident) setpaid(resident);
		return(0);
	}
	/* pay debt, if any, first */
	if(eshkp->debit) {
		long dtmp = eshkp->debit;
		long loan = eshkp->loan;
		char sbuf[BUFSZ];

/*JP		Sprintf(sbuf, "You owe %s %ld zorkmid%s ",
					   shkname(shkp), dtmp, plur(dtmp));*/
		Sprintf(sbuf, "あなたは%sに%ldゴールドの借りがある．",
					   shkname(shkp), dtmp);
		if(loan) {
		    if(loan == dtmp)
/*JP			Strcat(sbuf, "you picked up in the store.");*/
			Strcpy(sbuf, "店の中で拾ったものに対して，");
/*JP		    else Strcat(sbuf,
			   "for gold picked up and the use of merchandise.");*/
		    else Strcpy(sbuf,"拾ったお金や使った雑貨に対して，");
/*JP		} else Strcat(sbuf, "for the use of merchandise.");*/
		} else Strcpy(sbuf, "使った雑貨に対して，");
		Sprintf(eos(sbuf), "%sに%ldゴールドの借りがある．",
/*JP
					   shkname(shkp), dtmp, plur(dtmp));
*/
					   shkname(shkp), dtmp);
		pline(sbuf);
		if (u.ugold + eshkp->credit < dtmp) {
/*JP		    pline("But you don't%s have enough gold%s.",
			stashed_gold ? " seem to" : "",
			eshkp->credit ? " or credit" : "");*/
		    pline("しかし，お金%s足りない%s．",
			eshkp->credit ? "もクレジットも" : "が",
			stashed_gold ? "ようだ" : "");
		    return(1);
		} else {
		    if (eshkp->credit >= dtmp) {
			eshkp->credit -= dtmp;
			eshkp->debit = 0L;
			eshkp->loan = 0L;
/*JP			Your("debt is covered by your credit.");*/
			Your("借金はクレジットで補われた．");
		    } else if (!eshkp->credit) {
			u.ugold -= dtmp;
			shkp->mgold += dtmp;
			eshkp->debit = 0L;
			eshkp->loan = 0L;
/*JP			You("pay that debt.");*/
			You("借金を払った．");
			flags.botl = 1;
		    } else {
			dtmp -= eshkp->credit;
			eshkp->credit = 0L;
			u.ugold -= dtmp;
			shkp->mgold += dtmp;
			eshkp->debit = 0L;
			eshkp->loan = 0L;
/*JP			pline("That debt is partially offset by your credit.");
			You("pay the remainder.");*/
			pline("その借金は一部クレジットで相殺された．");
			You("残りを払った．");
			flags.botl = 1;
		    }
		    paid = TRUE;
		}
	}
	/* now check items on bill */
	if (eshkp->billct) {
	    register boolean itemize;

	    if (!u.ugold && !eshkp->credit) {
/*JP		You("%shave no money or credit%s.",
				    stashed_gold ? "seem to " : "",
				    paid ? " left" : "");*/
		You("%sお金もクレジットも持ってない%s．",
				    paid ? "もう" : "",
				    stashed_gold ? "ようだ" : "");
		return(0);
	    }
	    if ((u.ugold + eshkp->credit) < cheapest_item(shkp)) {
/*JP		You("don't have enough money to buy%s the item%s you picked.",
		    eshkp->billct > 1 ? " any of" : "", plur(eshkp->billct));*/
		pline("拾った品物を買うにはお金が足りない．");
		if(stashed_gold)
/*JP		    pline("Maybe you have some gold stashed away?");*/
		    You("どこかにお金を隠しているのかも？");
		return(0);
	    }

	    /* this isn't quite right; it itemizes without asking if the
	     * single item on the bill is partly used up and partly unpaid */
/*JP	    itemize = (eshkp->billct > 1 ? yn("Itemized billing?") == 'y' : 1);*/
	    itemize = (eshkp->billct > 1 ? yn("個別に勘定しますか？") == 'y' : 1);

	    for (pass = 0; pass <= 1; pass++) {
		tmp = 0;
		while (tmp < eshkp->billct) {
		    struct obj *otmp;
		    register struct bill_x *bp = &(eshkp->bill_p[tmp]);

		    /* find the object on one of the lists */
		    if ((otmp = bp_to_obj(bp)) != 0) {
			/* if completely used up, object quantity is stale;
			   restoring it to its original value here avoids
			   making the partly-used-up code more complicated */
			if (bp->useup) otmp->quan = bp->bquan;
		    } else {
			impossible("Shopkeeper administration out of order.");
			setpaid(shkp);	/* be nice to the player */
			return 1;
		    }
		    if (pass == bp->useup && otmp->quan == bp->bquan) {
			/* pay for used-up items on first pass and others
			 * on second, so player will be stuck in the store
			 * less often; things which are partly used up
			 * are processed on both passes */
			tmp++;
		    } else {
			switch (dopayobj(shkp, bp, &otmp, pass, itemize)) {
			  case PAY_CANT:
				return 1;	/*break*/
			  case PAY_BROKE:
				paid = TRUE;
				goto thanks;	/*break*/
			  case PAY_SKIP:
				tmp++;
				continue;	/*break*/
			  case PAY_SOME:
				paid = TRUE;
				if (itemize) bot();
				continue;	/*break*/
			  case PAY_BUY:
				paid = TRUE;
				break;
			}
			if (itemize) bot();
			*bp = eshkp->bill_p[--eshkp->billct];
		    }
		}
	    }
	}
thanks:
	if(!ANGRY(shkp) && paid)
/*JP	    verbalize("Thank you for shopping in %s %s!",*/
	    verbalize("%sの%sへまたどうぞ！",
		s_suffix(shkname(shkp)),
		shtypes[eshkp->shoptype - SHOPBASE].name);
	return(1);
}
#endif /*OVL0*/
#ifdef OVL3

/* return 2 if used-up portion paid */
/*	  1 if paid successfully    */
/*	  0 if not enough money     */
/*	 -1 if skip this object     */
/*	 -2 if no money/credit left */
STATIC_OVL int
dopayobj(shkp, bp, obj_p, which, itemize)
register struct monst *shkp;
register struct bill_x *bp;
struct obj **obj_p;
int	which;		/* 0 => used-up item, 1 => other (unpaid or lost) */
boolean itemize;
{
	register struct obj *obj = *obj_p;
	long ltmp, quan, save_quan;
	int buy;
	boolean stashed_gold = (hidden_gold() > 0L),
		consumed = (which == 0);

	if(!obj->unpaid && !bp->useup){
		impossible("Paid object on bill??");
		return PAY_BUY;
	}
	if(itemize && u.ugold + ESHK(shkp)->credit == 0L){
/*JP		You("%shave no money or credit left.",
			     stashed_gold ? "seem to " : "");*/
		You("もうお金もクレジットもない%s．",
			     stashed_gold ? "ようだ．" : "");
		return PAY_BROKE;
	}
	/* we may need to temporarily adjust the object, if part of the
	   original quantity has been used up but part remains unpaid  */
	save_quan = obj->quan;
	if (consumed) {
	    /* either completely used up (simple), or split needed */
	    quan = bp->bquan;
	    if (quan > obj->quan)	/* difference is amount used up */
		quan -= obj->quan;
	} else {
	    /* dealing with ordinary unpaid item */
	    quan = obj->quan;
	}
	obj->quan = quan;	/* to be used by doname() */
	obj->unpaid = 0;	/* ditto */
	ltmp = bp->price * quan;
	buy = PAY_BUY;		/* flag; if changed then return early */

	if (itemize) {
	    char qbuf[BUFSZ];
/*JP	    Sprintf(qbuf,"%s for %ld zorkmid%s.  Pay?", quan == 1L ?*/
	    Sprintf(qbuf,"%sは%ldゴールドです．買いますか？", quan == 1L ?
		    Doname2(obj) : doname(obj), ltmp);
	    if (yn(qbuf) == 'n') {
		buy = PAY_SKIP;		/* don't want to buy */
	    } else if (quan < bp->bquan && !consumed) { /* partly used goods */
		obj->quan = bp->bquan - save_quan;	/* used up amount */
/*JP		verbalize("%s for the other %s before buying %s.",
			  ANGRY(shkp) ? "Pay" : "Please pay", xname(obj),
			  save_quan > 1L ? "these" : "this one");*/
		verbalize("それを買うまえに他の%sを%s",
			  xname(obj),
			  ANGRY(shkp) ? "払え！" : "払ってください．");
			  
		buy = PAY_SKIP;		/* shk won't sell */
	    }
	}
	if (buy == PAY_BUY && u.ugold + ESHK(shkp)->credit < ltmp) {
/*JP	    You("don't%s have gold%s enough to pay for %s.",
		stashed_gold ? " seem to" : "",
		(ESHK(shkp)->credit > 0L) ? " or credit" : "",
		doname(obj));*/
	    You("%sの代金を支払うだけのゴールド%s持ってない%s．",
		doname(obj),
		(ESHK(shkp)->credit > 0L) ? "もクレジットも" : "を",
		stashed_gold ? "ようだ" : "");
	    buy = itemize ? PAY_SKIP : PAY_CANT;
	}

	if (buy != PAY_BUY) {
	    /* restore unpaid object to original state */
	    obj->quan = save_quan;
	    obj->unpaid = 1;
	    return buy;
	}

	pay(ltmp, shkp);
/*JP	shk_names_obj(shkp, obj, "bought %s for %ld gold piece%s.%s", ltmp, "");*/
	shk_names_obj(shkp, obj, "%sを%ldゴールドで買った%s．%s", ltmp, "");
	obj->quan = save_quan;		/* restore original count */
	/* quan => amount just bought, save_quan => remaining unpaid count */
	if (consumed) {
	    if (quan != bp->bquan) {
		/* eliminate used-up portion; remainder is still unpaid */
		bp->bquan = obj->quan;
		obj->unpaid = 1;
		bp->useup = 0;
		buy = PAY_SOME;
	    } else {	/* completely used-up, so get rid of it */
		obj_extract_self(obj);
	     /* assert( obj == *obj_p ); */
		dealloc_obj(obj);
		*obj_p = 0;	/* destroy pointer to freed object */
	    }
	}
	return buy;
}
#endif /*OVL3*/
#ifdef OVLB

static coord repo_location;	/* repossession context */

/* routine called after dying (or quitting) */
boolean
paybill(croaked)
register boolean croaked;
{
	register struct monst *mtmp, *mtmp2, *resident= (struct monst *)0;
	register boolean taken = FALSE;
	register int numsk = 0;

	/* this is where inventory will end up if any shk takes it */
	repo_location.x = repo_location.y = 0;

	/* give shopkeeper first crack */
	if ((mtmp = shop_keeper(*u.ushops)) && inhishop(mtmp)) {
	    numsk++;
	    resident = mtmp;
	    taken = inherits(resident, numsk, croaked);
	}
	for (mtmp = next_shkp(fmon, FALSE);
		mtmp; mtmp = next_shkp(mtmp2, FALSE)) {
	    mtmp2 = mtmp->nmon;
	    if (mtmp != resident) {
		/* for bones: we don't want a shopless shk around */
		if(!on_level(&(ESHK(mtmp)->shoplevel), &u.uz))
			mongone(mtmp);
		else {
		    numsk++;
		    taken |= inherits(mtmp, numsk, croaked);
		}
	    }
	}
	if(numsk == 0) return(FALSE);
	return(taken);
}

STATIC_OVL boolean
inherits(shkp, numsk, croaked)
struct monst *shkp;
int numsk;
boolean croaked;
{
	long loss = 0L;
	struct eshk *eshkp = ESHK(shkp);
	boolean take = FALSE, taken = FALSE;
	int roomno = *u.ushops;

	/* the simplifying principle is that first-come */
	/* already took everything you had.		*/
	if(numsk > 1) {
	    if(cansee(shkp->mx, shkp->my) && croaked)
/*JP		pline("%s %slooks at your corpse%s%s", Monnam(shkp),
		     (shkp->msleep || !shkp->mcanmove) ?
				   "wakes up, " : "",
		     !rn2(2) ? (shkp->female ? ", shakes her head," :
				 ", shakes his head,") : "",
		     !inhishop(shkp) ? " and disappears. " : " and sighs.");*/
		pline("%sは%sあなたの死体を見て%s%s．", Monnam(shkp),
		     (shkp->msleep || !shkp->mcanmove) ?
				   "目をさますと" : "",
		     !rn2(2) ? "首を振り，" : "",
		     !inhishop(shkp) ? "姿を消した" : "溜息をついた");
	    taken = (roomno == eshkp->shoproom);
	    goto skip;
	}

	/* get one case out of the way: you die in the shop, the */
	/* shopkeeper is peaceful, nothing stolen, nothing owed. */
	if(roomno == eshkp->shoproom && inhishop(shkp) &&
	    !eshkp->billct && !eshkp->robbed && !eshkp->debit &&
	     NOTANGRY(shkp) && !eshkp->following) {
		if (invent)
/*JP			pline("%s gratefully inherits all your possessions.",*/
			pline("%sはあなたの持ち物を有難く受けとった．",
				shkname(shkp));
		set_repo_loc(eshkp);
		goto clear;
	}

	if (eshkp->billct || eshkp->debit || eshkp->robbed) {
		register long total = 0L;

		if(roomno == eshkp->shoproom && inhishop(shkp))
		    total = (addupbill(shkp) + eshkp->debit);
		loss = ((total >= eshkp->robbed) ? total : eshkp->robbed);
		take = TRUE;
	}

	if (eshkp->following || ANGRY(shkp) || take) {
		if(!invent && !u.ugold) goto skip;

		if ((loss > u.ugold) || !loss || roomno == eshkp->shoproom) {
/*JP			pline("%s %s%stakes all your possessions.",*/
			pline("%sは%s%sあなたの持ち物すべてをもらった．",
				shkname(shkp),
				(shkp->msleep || !shkp->mcanmove) ?
/*JP				   "wakes up and " : "",*/
				   "目がさめると，" : "",
				(distu(shkp->mx, shkp->my) > 2) ?
/*JP				    "comes and " : "");*/
				    "近づき，" : "");
			taken = TRUE;
			shkp->mgold += u.ugold;
			u.ugold = 0L;
			set_repo_loc(eshkp);
		} else {
			u.ugold -= loss;
			shkp->mgold += loss;
/*JP			pline("%s %sand takes %ld zorkmid%s %sowed %s.",*/
			pline("%sは%s%s借りている%ldゴールドを受けとった．",
			      Monnam(shkp),
			      (shkp->msleep || !shkp->mcanmove) ?
/*JP					"wakes up " : "comes ",*/
					"目をさますと，" : "近づき，",
			      strncmp(eshkp->customer,
/*JP					   plname, PL_NSIZ) ? "" : "you ",
			      shkp->female ? "her" : "him");*/
					   plname, PL_NSIZ) ? "" : "あなたの",
			      loss);
		}
skip:
		/* in case we create bones */
		shkp->msleep = 0;
		if(!inhishop(shkp))
			home_shk(shkp, FALSE);
	}
clear:
	setpaid(shkp);
	return(taken);
}

STATIC_OVL void
set_repo_loc(eshkp)
struct eshk *eshkp;
{
	register xchar ox, oy;

	/* if you're not in this shk's shop room, or if you're in its doorway
	    or entry spot, then your gear gets dumped all the way inside */
	if (*u.ushops != eshkp->shoproom ||
		IS_DOOR(levl[u.ux][u.uy].typ) ||
		(u.ux == eshkp->shk.x && u.uy == eshkp->shk.y)) {
	    /* shk.x,shk.y is the position immediately in
	     * front of the door -- move in one more space
	     */
	    ox = eshkp->shk.x;
	    oy = eshkp->shk.y;
	    ox += sgn(ox - eshkp->shd.x);
	    oy += sgn(oy - eshkp->shd.y);
	} else {		/* already inside this shk's shop */
	    ox = u.ux;
	    oy = u.uy;
	}
	/* finish_paybill will deposit invent here */
	repo_location.x = ox;
	repo_location.y = oy;
}

/* called at game exit, after inventory disclosure but before making bones */
void finish_paybill()
{
	register struct obj *otmp;
	int ox = repo_location.x,
	    oy = repo_location.y;

#if 0		/* don't bother */
	if (ox == 0 && oy == 0) impossible("finish_paybill: no location");
#endif
	/* transfer all of the character's inventory to the shop floor */
	while ((otmp = invent) != 0) {
	    otmp->owornmask = 0L;	/* perhaps we should call setnotworn? */
	    otmp->lamplit = 0;		/* avoid "goes out" msg from freeinv */
	    if (rn2(5)) curse(otmp);	/* normal bones treatment for invent */
	    obj_extract_self(otmp);
	    place_object(otmp, ox, oy);
	}
}

/* find obj on one of the lists */
STATIC_OVL struct obj *
bp_to_obj(bp)
register struct bill_x *bp;
{
	register struct obj *obj;
	register unsigned int id = bp->bo_id;

	if(bp->useup)
		obj = o_on(id, billobjs);
	else
		obj = find_oid(id);
	return obj;
}

/*
 * Look for o_id on all lists but billobj.  Return obj or NULL if not found.
 * Its OK for restore_timers() to call this function, there should not
 * be any timeouts on the billobjs chain.
 */
struct obj *
find_oid(id)
unsigned id;
{
	struct obj *obj;
	struct monst *mon, *mmtmp[3];
	int i;

	/* first check various obj lists directly */
	if ((obj = o_on(id, invent)) != 0) return obj;
	if ((obj = o_on(id, fobj)) != 0) return obj;
	if ((obj = o_on(id, level.buriedobjlist)) != 0) return obj;
	if ((obj = o_on(id, migrating_objs)) != 0) return obj;

	/* not found yet; check inventory for members of various monst lists */
	mmtmp[0] = fmon;
	mmtmp[1] = migrating_mons;
	mmtmp[2] = mydogs;		/* for use during level changes */
	for (i = 0; i < 3; i++)
	    for (mon = mmtmp[i]; mon; mon = mon->nmon)
		if ((obj = o_on(id, mon->minvent)) != 0) return obj;

	/* not found at all */
	return (struct obj *)0;
}
#endif /*OVLB*/
#ifdef OVL3

/* calculate the value that the shk will charge for [one of] an object */
STATIC_OVL long
get_cost(obj, shkp)
register struct obj *obj;
register struct monst *shkp;	/* if angry, impose a surcharge */
{
	register long tmp = getprice(obj, FALSE);

	if (!tmp) tmp = 5L;
	/* shopkeeper may notice if the player isn't very knowledgeable -
	   especially when gem prices are concerned */
	if (!obj->dknown || !objects[obj->otyp].oc_name_known) {
		if (obj->oclass == GEM_CLASS) {
			/* all gems are priced high - real or not */
			if (objects[obj->otyp].oc_material == GLASS) {
			    int i = obj->otyp - LUCKSTONE + JADE + 1;
			    /* real gem's cost (worthless gems come
			       after jade but before luckstone) */
			    tmp = (long) objects[i].oc_cost;
			}
		} else if (!(obj->o_id % 4)) /* arbitrarily impose surcharge */
			tmp += tmp / 3L;
	}
#ifdef FIGHTER
	if (obj->otyp == SAILOR_BLOUSE){
	  if (!flags.female)
	    tmp *= 15;
	  else
	    tmp *= 10;
	}
#endif
#ifdef TOURIST
	if ((Role_is('T') && u.ulevel < (MAXULEV/2))
	    || (uarmu && !uarm && !uarmc))	/* touristy shirt visible */
		tmp += tmp / 3L;
	else
#endif
	if (uarmh && uarmh->otyp == DUNCE_CAP)
		tmp += tmp / 3L;

	if (ACURR(A_CHA) > 18)		tmp /= 2L;
	else if (ACURR(A_CHA) > 17)	tmp -= tmp / 3L;
	else if (ACURR(A_CHA) > 15)	tmp -= tmp / 4L;
	else if (ACURR(A_CHA) < 6)	tmp *= 2L;
	else if (ACURR(A_CHA) < 8)	tmp += tmp / 2L;
	else if (ACURR(A_CHA) < 11)	tmp += tmp / 3L;
	if (tmp <= 0L) tmp = 1L;
	else if (obj->oartifact) tmp *= 4L;
	/* anger surcharge should match rile_shk's */
	if (shkp && ESHK(shkp)->surcharge) tmp += (tmp + 2L) / 3L;
	return tmp;
}
#endif /*OVL3*/
#ifdef OVLB

/* returns the price of a container's content.  the price
 * of the "top" container is added in the calling functions.
 * a different price quoted for selling as vs. buying.
 */
long
contained_cost(obj, shkp, price, usell)
register struct obj *obj;
register struct monst *shkp;
long price;
register boolean usell;
{
	register struct obj *otmp;

	/* the price of contained objects */
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
	    if (otmp->oclass == GOLD_CLASS) continue;
	    /* the "top" container is evaluated by caller */
	    if (usell) {
		if (saleable(shkp, otmp) &&
			!otmp->unpaid && otmp->oclass != BALL_CLASS &&
			!(otmp->oclass == FOOD_CLASS && otmp->oeaten) &&
			!(Is_candle(otmp) && otmp->age <
				20L * (long)objects[otmp->otyp].oc_cost))
		    price += set_cost(otmp, shkp);
	    } else if (!otmp->no_charge) {
		    price += get_cost(otmp, shkp) * otmp->quan;
	    }

	    if (Has_contents(otmp))
		    price += contained_cost(otmp, shkp, price, usell);
	}

	return(price);
}

long
contained_gold(obj)
register struct obj *obj;
{
	register struct obj *otmp;
	register long value = 0L;

	/* accumulate contained gold */
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
	    if (otmp->oclass == GOLD_CLASS)
		value += otmp->quan;
	    else if (Has_contents(otmp))
		value += contained_gold(otmp);

	return(value);
}

STATIC_OVL void
dropped_container(obj, shkp, sale)
register struct obj *obj;
register struct monst *shkp;
register boolean sale;
{
	register struct obj *otmp;

	/* the "top" container is treated in the calling fn */
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
	    if (otmp->oclass == GOLD_CLASS) continue;

	    if (!otmp->unpaid && !(sale && saleable(shkp, otmp)))
		otmp->no_charge = 1;

	    if (Has_contents(otmp))
		dropped_container(otmp, shkp, sale);
	}
}

void
picked_container(obj)
register struct obj *obj;
{
	register struct obj *otmp;

	/* the "top" container is treated in the calling fn */
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
	    if (otmp->oclass == GOLD_CLASS) continue;

	    if (otmp->no_charge)
		otmp->no_charge = 0;

	    if (Has_contents(otmp))
		picked_container(otmp);
	}
}
#endif /*OVLB*/
#ifdef OVL3

/* calculate how much the shk will pay when buying [all of] an object */
STATIC_OVL long
set_cost(obj, shkp)
register struct obj *obj;
register struct monst *shkp;
{
	long tmp = getprice(obj, TRUE) * obj->quan;

#ifdef FIGHTER
	if (obj->otyp == SAILOR_BLOUSE){
	  if (flags.female && Role_is('F'))
	    tmp *= 10;
	  else if (flags.female)
	    tmp *= 5;
	  else if (!Role_is('F'))
	    tmp = 0;
	}
#endif
#ifdef TOURIST
	if ((Role_is('T') && u.ulevel < (MAXULEV/2))
	    || (uarmu && !uarm && !uarmc))	/* touristy shirt visible */
		tmp /= 3L;
	else
#endif
	if (uarmh && uarmh->otyp == DUNCE_CAP)
		tmp /= 3L;
	else
		tmp /= 2L;

	/* shopkeeper may notice if the player isn't very knowledgeable -
	   especially when gem prices are concerned */
	if (!obj->dknown || !objects[obj->otyp].oc_name_known) {
		if (obj->oclass == GEM_CLASS) {
			/* different shop keepers give different prices */
			if (objects[obj->otyp].oc_material == GEMSTONE ||
			    objects[obj->otyp].oc_material == GLASS) {
				tmp = (obj->otyp % (6 - shkp->m_id % 3));
				tmp = (tmp + 3) * obj->quan;
			}
		} else if (tmp > 1L && !rn2(4))
			tmp -= tmp / 4L;
	}
	return tmp;
}

#endif /*OVL3*/
#ifdef OVLB

/* called from doinv(invent.c) for inventory of unpaid objects */
long
unpaid_cost(unp_obj)
register struct obj *unp_obj;	/* known to be unpaid */
{
	register struct bill_x *bp = (struct bill_x *)0;
	register struct monst *shkp;

	for(shkp = next_shkp(fmon, TRUE); shkp;
					shkp = next_shkp(shkp->nmon, TRUE))
	    if ((bp = onbill(unp_obj, shkp, TRUE)) != 0) break;

	/* onbill() gave no message if unexpected problem occurred */
	if(!bp) impossible("unpaid_cost: object wasn't on any bill!");

	return bp ? unp_obj->quan * bp->price : 0L;
}

STATIC_OVL void
add_one_tobill(obj, dummy)
register struct obj *obj;
register boolean dummy;
{
	register struct monst *shkp;
	register struct bill_x *bp;
	register int bct;
	register char roomno = *u.ushops;

	if (!roomno) return;
	if (!(shkp = shop_keeper(roomno))) return;
	if (!inhishop(shkp)) return;

	if (onbill(obj, shkp, FALSE) || /* perhaps thrown away earlier */
		    (obj->oclass == FOOD_CLASS && obj->oeaten))
		return;

	if (ESHK(shkp)->billct == BILLSZ) {
/*JP		You("got that for free!");*/
		You("それをただで手に入れた！");
		return;
	}

	/* To recognize objects the shopkeeper is not interested in. -dgk
	 */
	if (obj->no_charge) {
		obj->no_charge = 0;
		return;
	}

	bct = ESHK(shkp)->billct;
	bp = &(ESHK(shkp)->bill_p[bct]);
	bp->bo_id = obj->o_id;
	bp->bquan = obj->quan;
	if(dummy) {		  /* a dummy object must be inserted into  */
	    bp->useup = 1;	  /* the billobjs chain here.  crucial for */
	    add_to_billobjs(obj); /* eating floorfood in shop.  see eat.c  */
	} else	bp->useup = 0;
	bp->price = get_cost(obj, shkp);
	ESHK(shkp)->billct++;
	obj->unpaid = 1;
}

STATIC_OVL void
add_to_billobjs(obj)
    struct obj *obj;
{
    if (obj->where != OBJ_FREE)
	panic("add_to_billobjs: obj not free");
    if (obj->timed)
	panic("add_to_billobjs: obj is timed");

    obj->nobj = billobjs;
    billobjs = obj;
    obj->where = OBJ_ONBILL;
}

/* recursive billing of objects within containers. */
STATIC_OVL void
bill_box_content(obj, ininv, dummy, shkp)
register struct obj *obj;
register boolean ininv, dummy;
register struct monst *shkp;
{
	register struct obj *otmp;

	for (otmp = obj->cobj; otmp; otmp = otmp->nobj) {
		if (otmp->oclass == GOLD_CLASS) continue;

		/* the "top" box is added in addtobill() */
		if (!otmp->no_charge)
		    add_one_tobill(otmp, dummy);
		if (Has_contents(otmp))
		    bill_box_content(otmp, ininv, dummy, shkp);
	}

}

/* shopkeeper tells you what you bought or sold, sometimes partly IDing it */
STATIC_OVL void
shk_names_obj(shkp, obj, fmt, amt, arg)
struct monst *shkp;
struct obj *obj;
const char *fmt;	/* "%s %ld %s %s", doname(obj), amt, plur(amt), arg */
long amt;
const char *arg;
{
	char *obj_name, fmtbuf[BUFSZ];
	boolean was_unknown = !obj->dknown;

	obj->dknown = TRUE;
	/* Use real name for ordinary weapons/armor, and spell-less
	 * scrolls/books (that is, blank and mail), but only if the
	 * object is within the shk's area of interest/expertise.
	 */
	if (!objects[obj->otyp].oc_magic && saleable(shkp, obj) &&
	    (obj->oclass == WEAPON_CLASS || obj->oclass == ARMOR_CLASS ||
	     obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS ||
	     obj->otyp == MIRROR)) {
	    was_unknown |= !objects[obj->otyp].oc_name_known;
	    makeknown(obj->otyp);
	}
	obj_name = doname(obj);
	/* Use an alternate message when extra information is being provided */
	if (was_unknown) {
/*JP	    Sprintf(fmtbuf, "%%s; you %s", fmt);*/
	    Sprintf(fmtbuf, "%s", fmt);
	    obj_name[0] = highc(obj_name[0]);
/*JP	    pline(fmtbuf, obj_name, (obj->quan > 1) ? "them" : "it",
		  amt, plur(amt), arg);*/
	    pline(fmtbuf, obj_name,
		  amt, "", arg);
	} else {
/*JP	    You(fmt, obj_name, amt, plur(amt), arg);*/
	    You(fmt, obj_name, amt, "", arg);
	}
}

void
addtobill(obj, ininv, dummy, silent)
register struct obj *obj;
register boolean ininv, dummy, silent;
{
	register struct monst *shkp;
	register char roomno = *u.ushops;
	long ltmp = 0L, cltmp = 0L, gltmp = 0L;
	register boolean container = Has_contents(obj);

	if(!*u.ushops) return;

	if(!(shkp = shop_keeper(roomno))) return;

	if(!inhishop(shkp)) return;

	if(/* perhaps we threw it away earlier */
		 onbill(obj, shkp, FALSE) ||
		 (obj->oclass == FOOD_CLASS && obj->oeaten)
	      ) return;

	if(ESHK(shkp)->billct == BILLSZ) {
/*JP		You("got that for free!");*/
		You("それをただで手に入れた！");
		return;
	}

	if(obj->oclass == GOLD_CLASS) {
		costly_gold(obj->ox, obj->oy, obj->quan);
		return;
	}

	if(!obj->no_charge)
	    ltmp = get_cost(obj, shkp);

	if (obj->no_charge && !container) {
		obj->no_charge = 0;
		return;
	}

	if(container) {
	    if(obj->cobj == (struct obj *)0) {
		if(obj->no_charge) {
		    obj->no_charge = 0;
		    return;
		} else {
		    add_one_tobill(obj, dummy);
		    goto speak;
		}
	    } else {
		cltmp += contained_cost(obj, shkp, cltmp, FALSE);
		gltmp += contained_gold(obj);
	    }

	    if(ltmp) add_one_tobill(obj, dummy);
	    if(cltmp) bill_box_content(obj, ininv, dummy, shkp);
	    picked_container(obj); /* reset contained obj->no_charge */

	    ltmp += cltmp;

	    if(gltmp) {
		costly_gold(obj->ox, obj->oy, gltmp);
		if(!ltmp) return;
	    }

	    if(obj->no_charge)
		obj->no_charge = 0;

	} else /* i.e., !container */
	    add_one_tobill(obj, dummy);
speak:
	if (shkp->mcanmove && !shkp->msleep && !silent) {
	    char buf[BUFSZ];

	    if(!ltmp) {
/*JP		pline("%s has no interest in %s.", Monnam(shkp),*/
		pline("%sは%sに興味を示さない．", Monnam(shkp),
					     the(xname(obj)));
		return;
	    }
/*JP	    Strcpy(buf, "\"For you, ");
	    if (ANGRY(shkp)) Strcat(buf, "scum ");*/
	    Strcpy(buf, "「");
	    if (ANGRY(shkp)) Strcat(buf, "このクソったれ");
	    else {
		static const char *honored[5] = {
/*JP		  "good", "honored", "most gracious", "esteemed",
		  "most renowned and sacred"*/
		  "やぁ", "やぁ，名誉ある", "やぁ，上品な", "やぁ，尊敬する",
		  "やぁ，高名で神聖な"
		};
		Strcat(buf, honored[rn2(4) + u.uevent.udemigod]);
/*JP		if (!is_human(uasmon)) Strcat(buf, " creature");
		else
		    Strcat(buf, (flags.female) ? " lady" : " sir");*/
		if (!is_human(uasmon)) Strcat(buf, "生物さん");
		else
		    Strcat(buf, (flags.female) ? "お嬢さん" : "お客さん");
	    }
	    if(ininv) {
		long quan = obj->quan;
		obj->quan = 1L; /* fool xname() into giving singular */
/*JP		pline("%s; only %ld %s %s.\"", buf, ltmp,
			(quan > 1L) ? "per" : "for this", xname(obj));*/
		pline("%s．%sは%sたったの%ldゴールドだ．」", buf, xname(obj),
			(quan > 1L) ? "1つ" : "", ltmp);
		obj->quan = quan;
	    } else
/*JP		pline("%s will cost you %ld zorkmid%s%s.",
			The(xname(obj)), ltmp, plur(ltmp),
			(obj->quan > 1L) ? " each" : "");*/
		pline("%sは%s%ldゴールドだ",
			The(xname(obj)),
			(obj->quan > 1L) ? "それぞれ" : "", ltmp);

	} else if(!silent) {
/*JP	    if(ltmp) pline_The("list price of %s is %ld zorkmid%s%s.",
				   the(xname(obj)), ltmp, plur(ltmp),
				   (obj->quan > 1L) ? " each" : "");*/
	    if(ltmp) pline("%sの値段は%s%ldゴールドだ．",
				   the(xname(obj)), 
				   (obj->quan > 1L) ? "それぞれ" : "", ltmp);
/*JP	    else pline("%s does not notice.", Monnam(shkp));*/
	    else pline("%sは気がついていない．", Monnam(shkp));
	}
}

void
splitbill(obj, otmp)
register struct obj *obj, *otmp;
{
	/* otmp has been split off from obj */
	register struct bill_x *bp;
	register long tmp;
	register struct monst *shkp = shop_keeper(*u.ushops);

	if(!shkp || !inhishop(shkp)) {
		impossible("splitbill: no resident shopkeeper??");
		return;
	}
	bp = onbill(obj, shkp, FALSE);
	if(!bp) {
		impossible("splitbill: not on bill?");
		return;
	}
	if(bp->bquan < otmp->quan) {
		impossible("Negative quantity on bill??");
	}
	if(bp->bquan == otmp->quan) {
		impossible("Zero quantity on bill??");
	}
	bp->bquan -= otmp->quan;

	if(ESHK(shkp)->billct == BILLSZ) otmp->unpaid = 0;
	else {
		tmp = bp->price;
		bp = &(ESHK(shkp)->bill_p[ESHK(shkp)->billct]);
		bp->bo_id = otmp->o_id;
		bp->bquan = otmp->quan;
		bp->useup = 0;
		bp->price = tmp;
		ESHK(shkp)->billct++;
	}
}

STATIC_OVL void
sub_one_frombill(obj, shkp)
register struct obj *obj;
register struct monst *shkp;
{
	register struct bill_x *bp;

	if((bp = onbill(obj, shkp, FALSE)) != 0) {
		register struct obj *otmp;

		obj->unpaid = 0;
		if(bp->bquan > obj->quan){
			otmp = newobj(0);
			*otmp = *obj;
			bp->bo_id = otmp->o_id = flags.ident++;
			otmp->where = OBJ_FREE;
			otmp->quan = (bp->bquan -= obj->quan);
			otmp->owt = 0;	/* superfluous */
			otmp->onamelth = 0;
			otmp->oxlth = 0;
			otmp->mtraits = 0;
			bp->useup = 1;
			add_to_billobjs(otmp);
			return;
		}
		ESHK(shkp)->billct--;
#ifdef DUMB
		{
		/* DRS/NS 2.2.6 messes up -- Peter Kendell */
			int indx = ESHK(shkp)->billct;
			*bp = ESHK(shkp)->bill_p[indx];
		}
#else
		*bp = ESHK(shkp)->bill_p[ESHK(shkp)->billct];
#endif
		return;
	} else if (obj->unpaid) {
		impossible("sub_one_frombill: unpaid object not on bill");
		obj->unpaid = 0;
	}
}

/* recursive check of unpaid objects within nested containers. */
void
subfrombill(obj, shkp)
register struct obj *obj;
register struct monst *shkp;
{
	register struct obj *otmp;

	sub_one_frombill(obj, shkp);

	if (Has_contents(obj))
	    for(otmp = obj->cobj; otmp; otmp = otmp->nobj) {
		if(otmp->oclass == GOLD_CLASS) continue;

		if (Has_contents(otmp))
		    subfrombill(otmp, shkp);
		else
		    sub_one_frombill(otmp, shkp);
	    }
}

#endif /*OVLB*/
#ifdef OVL3

STATIC_OVL long
stolen_container(obj, shkp, price, ininv)
register struct obj *obj;
register struct monst *shkp;
long price;
register boolean ininv;
{
	register struct obj *otmp;

	if(ininv && obj->unpaid)
	    price += get_cost(obj, shkp);
	else {
	    if(!obj->no_charge)
		price += get_cost(obj, shkp);
	    obj->no_charge = 0;
	}

	/* the price of contained objects, if any */
	for(otmp = obj->cobj; otmp; otmp = otmp->nobj) {

	    if(otmp->oclass == GOLD_CLASS) continue;

	    if (!Has_contents(otmp)) {
		if(ininv) {
		    if(otmp->unpaid)
			price += get_cost(otmp, shkp);
		} else {
		    if(!otmp->no_charge) {
			if(!(otmp->oclass == BALL_CLASS ||
			    (otmp->oclass == FOOD_CLASS && otmp->oeaten) ||
			    (Is_candle(otmp) && otmp->age <
				  20L * (long)objects[otmp->otyp].oc_cost))
			  ) price += get_cost(otmp, shkp);
		    }
		    otmp->no_charge = 0;
		}
	    } else
		price += stolen_container(otmp, shkp, price, ininv);
	}

	return(price);
}
#endif /*OVL3*/
#ifdef OVLB

long
stolen_value(obj, x, y, peaceful, silent)
register struct obj *obj;
register xchar x, y;
register boolean peaceful, silent;
{
	register long value = 0L, gvalue = 0L;
	register struct monst *shkp = shop_keeper(*in_rooms(x, y, SHOPBASE));

	if (!shkp || !inhishop(shkp))
	    return (0L);

	if(obj->oclass == GOLD_CLASS) {
	    gvalue += obj->quan;
	} else if (Has_contents(obj)) {
	    register boolean ininv = !!count_unpaid(obj->cobj);

	    value += stolen_container(obj, shkp, value, ininv);
	    if(!ininv) gvalue += contained_gold(obj);
	} else if (!obj->no_charge && saleable(shkp, obj)) {
	    value += get_cost(obj, shkp);
	}

	if(gvalue + value == 0L) return(0L);

	value += gvalue;

	if(peaceful) {
	    value = check_credit(value, shkp);
	    ESHK(shkp)->debit += value;

	    if(!silent) {
		if(obj->oclass == GOLD_CLASS)
/*JP		    You("owe %s %ld zorkmids!", mon_nam(shkp), value);*/
		    You("%sに%ldゴールドの借りをつくった！", mon_nam(shkp), value);
/*JP		else You("owe %s %ld zorkmids for %s!",*/
		else You("%sに%ldゴールドの借りをつくった！",
/*JP			obj->quan > 1L ? "それら" : "それ",*/
			mon_nam(shkp),
			value);
	    }
	} else {
	    ESHK(shkp)->robbed += value;

	    if(!silent) {
		if(cansee(shkp->mx, shkp->my)) {
		    if(ESHK(shkp)->customer[0] == 0)
			(void) strncpy(ESHK(shkp)->customer,plname,PL_NSIZ);
/*JP		    Norep("%s booms: \"%s, you are a thief!\"",
				Monnam(shkp), plname);*/
		    Norep("%sは叫んだ：「%s，待て！このどろぼうめ！」",
				Monnam(shkp), plname);
/*JP		} else  Norep("You hear a scream, \"Thief!\"");*/
		} else  Norep("金切り声を聞いた，「待て！このどろぼうめ！」");
	    }
	    hot_pursuit(shkp);
	    (void) angry_guards(FALSE);
	}
	return(value);
}

/* auto-response flag for/from "sell foo?" 'a' => 'y', 'q' => 'n' */
static char sell_response = 'a';
static boolean sell_voluntarily = FALSE;

void
sellobj_state(deliberate)	/* called from dodrop(do.c) and doddrop() */
boolean deliberate;
{
	/* If we're deliberately dropping something, there's no automatic
	response to the shopkeeper's "want to sell" query; however, if we
	accidentally drop anything, the shk will buy it/them without asking.
	This retains the old pre-query risk that slippery fingers while in
	shops entailed:  you drop it, you've lost it.
	 */
	sell_response = deliberate ? '\0' : 'a';
	sell_voluntarily = deliberate;
}

void
sellobj(obj, x, y)
register struct obj *obj;
xchar x, y;
{
	register struct monst *shkp;
	register struct eshk *eshkp;
	long ltmp = 0L, cltmp = 0L, gltmp = 0L, offer;
	boolean saleitem, cgold = FALSE, container = Has_contents(obj);
	boolean isgold = (obj->oclass == GOLD_CLASS);

	if(!(shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) ||
	   !inhishop(shkp)) return;
	if(!costly_spot(x, y))	return;
	if(!*u.ushops) return;

	if(obj->unpaid && !container && !isgold) {
	    sub_one_frombill(obj, shkp);
	    return;
	}
	if(container) {
		/* find the price of content before subfrombill */
		cltmp += contained_cost(obj, shkp, cltmp, TRUE);
		/* find the value of contained gold */
		gltmp += contained_gold(obj);
		cgold = (gltmp > 0L);
	}

	saleitem = saleable(shkp, obj);
	if(!isgold && !obj->unpaid && saleitem)
	    ltmp = set_cost(obj, shkp);

	offer = ltmp + cltmp;

	/* get one case out of the way: nothing to sell, and no gold */
	if(!isgold && (offer + gltmp) == 0L) {
		register boolean unpaid = (obj->unpaid ||
				  (container && count_unpaid(obj->cobj)));

		if(container) {
			dropped_container(obj, shkp, FALSE);
			if(!obj->unpaid && !saleitem)
			    obj->no_charge = 1;
			if(obj->unpaid || count_unpaid(obj->cobj))
			    subfrombill(obj, shkp);
		} else obj->no_charge = 1;

		if(!unpaid)
/*JP		    pline("%s seems uninterested.", Monnam(shkp));*/
		    pline("%sは興味がないようだ．", Monnam(shkp));
		return;
	}

	/* you dropped something of your own - probably want to sell it */
	if (shkp->msleep || !shkp->mcanmove) {
		if (container)
		    dropped_container(obj, shkp, TRUE);
		if (!obj->unpaid)
		    obj->no_charge = 1;
		if (!shkp->mcanmove) {
		    if(ANGRY(shkp) && !rn2(4))
/*JP			pline("%s utters a curse.", Monnam(shkp));*/
			pline("%sは呪いの言葉をつぶやいた．", Monnam(shkp));
/*JP		    else pline("%s is indisposed.", Monnam(shkp));*/
		    else pline("%sは気がなえた．", Monnam(shkp));
		} else if(!rn2(3)) {
/*JP		    pline("%s snores indifferently.", Monnam(shkp));*/
		    pline("%sは無頓着にいびきをかいた．", Monnam(shkp));
		}
		subfrombill(obj, shkp);
		return;
	}

	eshkp = ESHK(shkp);

	if (ANGRY(shkp)) { /* they become shop-objects, no pay */
/*JP		pline("Thank you, scum!");*/
		pline("ありがよ，このクソったれ！");
		subfrombill(obj, shkp);
		return;
	}

	if(eshkp->robbed) {  /* shkp is not angry? */
		if(isgold) offer = obj->quan;
		else if(cgold) offer += cgold;
		if((eshkp->robbed -= offer < 0L))
			eshkp->robbed = 0L;
		if(offer) verbalize(
/*JP  "Thank you for your contribution to restock this recently plundered shop.");*/
  "寄贈をどうもありがろう．最近盗みにあって参ってたんだ．");
		subfrombill(obj, shkp);
		return;
	}

	if(isgold || cgold) {
		if(!cgold) gltmp = obj->quan;

		if(eshkp->debit >= gltmp) {
		    if(eshkp->loan) { /* you carry shop's gold */
			 if(eshkp->loan >= gltmp)
			     eshkp->loan -= gltmp;
			 else eshkp->loan = 0L;
		    }
		    eshkp->debit -= gltmp;
/*JP		    Your("debt is %spaid off.",*/
		    Your("借金は%s支払われた．",
/*JP				eshkp->debit ? "partially " : "");*/
				eshkp->debit ? "一部" : "");
		} else {
		    long delta = gltmp - eshkp->debit;

		    eshkp->credit += delta;
		    if(eshkp->debit) {
			eshkp->debit = 0L;
			eshkp->loan = 0L;
/*JP			Your("debt is paid off.");*/
			Your("借金は支払われた．");
		    }
/*JP		    pline("%ld zorkmid%s added to your credit.",
				delta, delta > 1L ? "s are" : " is");*/
		    pline("%ldゴールドがあなたのクレジットに加えられた．",
				delta);
		}
		if(offer) goto move_on;
		else {
		    if(!isgold) {
			if (container)
			    dropped_container(obj, shkp, FALSE);
			if (!obj->unpaid && !saleitem) obj->no_charge = 1;
			subfrombill(obj, shkp);
		    }
		    return;
		}
	}
move_on:
	if((!saleitem && !(container && cltmp > 0L))
	   || eshkp->billct == BILLSZ
	   || obj->oclass == BALL_CLASS
	   || obj->oclass == CHAIN_CLASS || offer == 0L
	   || (obj->oclass == FOOD_CLASS && obj->oeaten)
	   || (Is_candle(obj) &&
		   obj->age < 20L * (long)objects[obj->otyp].oc_cost)) {
/*JP		pline("%s seems not interested%s.", Monnam(shkp),
			cgold ? " in the rest" : "");*/
		pline("%sは%s興味がないようだ．", Monnam(shkp),
			cgold ? "残り物" : "");
		if (container)
		    dropped_container(obj, shkp, FALSE);
		obj->no_charge = 1;
		return;
	}

	if(!shkp->mgold) {
		char c, qbuf[BUFSZ];
		long tmpcr = ((offer * 9L) / 10L) + (offer <= 1L);

		if (!sell_voluntarily) {
		    c = sell_response = 'y';
		} else if (sell_response != 'n') {
/*JP		    pline("%s cannot pay you at present.", Monnam(shkp));*/
		    pline("%sは今のところは支払えない．", Monnam(shkp));
		    Sprintf(qbuf,
/*JP			    "Will you accept %ld zorkmid%s in credit for %s?",
			    tmpcr, plur(tmpcr), doname(obj));*/
		    "%sについて%ldゴールドのクレジットを受けいれますか？",

			    doname(obj), tmpcr);
		    /* won't accept 'a' response here */
		    c = ynq(qbuf);
		} else		/* previously specified "quit" */
		    c = 'n';

		if (c == 'y') {
		    shk_names_obj(shkp, obj, sell_voluntarily ?
/*JP			    "traded %s for %ld zorkmid%s in %scredit." :
			"relinquish %s and acquire %ld zorkmid%s in %scredit.",
			    tmpcr,
			    (eshkp->credit > 0L) ? "additional " : "");*/
			  "%sを%ldゴールド分のクレジットで受けとった%s．" :
			  "%sを渡し，%ldゴールド分のクレジットを得た%s．",
				  tmpcr, "");
		    eshkp->credit += tmpcr;
		    subfrombill(obj, shkp);
		} else {
		    if (c == 'q') sell_response = 'n';
		    if (container)
			dropped_container(obj, shkp, FALSE);
		    if (!obj->unpaid) obj->no_charge = 1;
		    subfrombill(obj, shkp);
		}
	} else {
		char qbuf[BUFSZ];
		boolean short_funds = (offer > shkp->mgold);

		if (short_funds) offer = shkp->mgold;

		if (!sell_response) {
		    Sprintf(qbuf,
/*JP			 "%s offers%s %ld gold piece%s for%s %s %s.  Sell %s?",
			    Monnam(shkp), short_funds ? " only" : "",
			    offer, plur(offer),
			    (!ltmp && cltmp) ? " the contents of" : "",
			    obj->unpaid ? "the" : "your", xname(obj),
			    (obj->quan == 1L) ? "it" : "them");*/
			    "%sはあなたの%s%sに%ldの値%s．売りますか？",
			    Monnam(shkp), xname(obj),
    			    (!ltmp && cltmp) ? "の中身" : "",
			    offer, 
			    short_funds ? "しかつけなかった" : "をつけた");

		} else  qbuf[0] = '\0';		/* just to pacify lint */

		switch (sell_response ? sell_response : ynaq(qbuf)) {
		 case 'q':  sell_response = 'n';
		 case 'n':  if (container)
				dropped_container(obj, shkp, FALSE);
			    if (!obj->unpaid) obj->no_charge = 1;
			    subfrombill(obj, shkp);
			    break;
		 case 'a':  sell_response = 'y';
		 case 'y':  if (container)
				dropped_container(obj, shkp, TRUE);
			    if (!obj->unpaid && !saleitem) obj->no_charge = 1;
			    subfrombill(obj, shkp);
			    pay(-offer, shkp);
			    shk_names_obj(shkp, obj, sell_voluntarily ?
/*JP				    "sold %s for %ld gold piece%s.%s" :
	       "relinquish %s and receive %ld gold piece%s in compensation.%s",
*/
				    "%sを%ldゴールドで売った%s．%s" :
	       "%sを渡し，%ldゴールドの代償を受けとった%s．%s",
				    offer, "");
			    break;
		 default:   impossible("invalid sell response");
		}
	}
}

int
doinvbill(mode)
int mode;		/* 0: deliver count 1: paged */
{
#ifdef	__SASC
	void sasc_bug(struct obj *, unsigned);
#endif
	struct monst *shkp;
	struct eshk *eshkp;
	struct bill_x *bp, *end_bp;
	struct obj *obj;
	long totused;
	char *buf_p;
	winid datawin;

	shkp = shop_keeper(*u.ushops);
	if (!shkp || !inhishop(shkp)) {
	    if (mode != 0) impossible("doinvbill: no shopkeeper?");
	    return 0;
	}
	eshkp = ESHK(shkp);

	if (mode == 0) {
	    /* count expended items, so that the `I' command can decide
	       whether to include 'x' in its prompt string */
	    int cnt = !eshkp->debit ? 0 : 1;

	    for (bp = eshkp->bill_p, end_bp = &eshkp->bill_p[eshkp->billct];
		    bp < end_bp; bp++)
		if (bp->useup ||
			((obj = bp_to_obj(bp)) != 0 && obj->quan < bp->bquan))
		    cnt++;
	    return cnt;
	}

	datawin = create_nhwindow(NHW_MENU);
/*JP	putstr(datawin, 0, "Unpaid articles already used up:");*/
	putstr(datawin, 0, "すでに使ってしまった未払の品目：");
	putstr(datawin, 0, "");

	totused = 0L;
	for (bp = eshkp->bill_p, end_bp = &eshkp->bill_p[eshkp->billct];
		bp < end_bp; bp++) {
	    obj = bp_to_obj(bp);
	    if(!obj) {
		impossible("Bad shopkeeper administration.");
		goto quit;
	    }
	    if(bp->useup || bp->bquan > obj->quan) {
		long oquan, uquan, thisused;
		unsigned save_unpaid;

		save_unpaid = obj->unpaid;
		oquan = obj->quan;
		uquan = (bp->useup ? bp->bquan : bp->bquan - oquan);
		thisused = bp->price * uquan;
		totused += thisused;
		obj->quan = uquan;		/* cheat doname */
		obj->unpaid = 0;		/* ditto */
		/* Why 'x'?  To match `I x', more or less. */
		buf_p = xprname(obj, (char *)0, 'x', FALSE, thisused);
		obj->quan = oquan;		/* restore value */
#ifdef __SASC
				/* SAS/C 6.2 can't cope for some reason */
		sasc_bug(obj,save_unpaid);
#else
		obj->unpaid = save_unpaid;
#endif
		putstr(datawin, 0, buf_p);
	    }
	}
	if (eshkp->debit) {
	    /* additional shop debt which has no itemization available */
	    if (totused) putstr(datawin, 0, "");
	    totused += eshkp->debit;
	    buf_p = xprname((struct obj *)0,
			    "使用料または他の手数料",
			    GOLD_SYM, FALSE, eshkp->debit);
	    putstr(datawin, 0, buf_p);
	}
/*JP	buf_p = xprname((struct obj *)0, "Total:", '*', FALSE, totused);*/
	buf_p = xprname((struct obj *)0, "合計：", '*', FALSE, totused);
	putstr(datawin, 0, "");
	putstr(datawin, 0, buf_p);
	display_nhwindow(datawin, FALSE);
    quit:
	destroy_nhwindow(datawin);
	return(0);
}

#define HUNGRY	2

STATIC_OVL long
getprice(obj, shk_buying)
register struct obj *obj;
boolean shk_buying;
{
	register long tmp = (long) objects[obj->otyp].oc_cost;

	switch(obj->oclass) {
	case FOOD_CLASS:
		/* simpler hunger check, (2-4)*cost */
		if (u.uhs >= HUNGRY && !shk_buying) tmp *= (long) u.uhs;
		if (obj->oeaten) tmp = 0L;
		break;
	case WAND_CLASS:
		if (obj->spe == -1) tmp = 0L;
		break;
	case POTION_CLASS:
		if (obj->otyp == POT_WATER && !obj->blessed && !obj->cursed)
			tmp = 0L;
		break;
	case ARMOR_CLASS:
	case WEAPON_CLASS:
		if (obj->spe > 0) tmp += 10L * (long) obj->spe;
		break;
	case TOOL_CLASS:
		if (Is_candle(obj) &&
			obj->age < 20L * (long)objects[obj->otyp].oc_cost)
		    tmp /= 2L;
		break;
	}
	if (obj->oartifact) tmp *= 25L;
	return tmp;
}

/* shk catches thrown pick-axe */
int
shkcatch(obj, x, y)
register struct obj *obj;
register xchar x, y;
{
	register struct monst *shkp;

	if (!(shkp = shop_keeper(inside_shop(x, y))) ||
	    !inhishop(shkp)) return(0);

	if (shkp->mcanmove && !shkp->msleep &&
	    (*u.ushops != ESHK(shkp)->shoproom || !inside_shop(u.ux, u.uy)) &&
	    dist2(shkp->mx, shkp->my, x, y) < 3 &&
	    /* if it is the shk's pos, you hit and anger him */
	    (shkp->mx != x || shkp->my != y)) {
		if (mnearto(shkp, x, y, TRUE))
/*JP		    verbalize("Out of my way, scum!");*/
		    verbalize("どけ，クソったれ！");
		if (cansee(x, y)) {
/*JP		    pline("%s nimbly catches %s.",*/
		    pline("%sはすばやく%sをつかまえた．", 
			  Monnam(shkp), the(xname(obj)));
		    delay_output();
		    mark_synch();
		}
		mpickobj(shkp, obj);
		subfrombill(obj, shkp);
		return(1);
	}
	return(0);
}

void
add_damage(x, y, cost)
register xchar x, y;
long cost;
{
	struct damage *tmp_dam;
	char *shops;

	if (IS_DOOR(levl[x][y].typ)) {
	    struct monst *mtmp;

	    /* Don't schedule for repair unless it's a real shop entrance */
	    for (shops = in_rooms(x, y, SHOPBASE); *shops; shops++)
		if ((mtmp = shop_keeper(*shops)) != 0 &&
			x == ESHK(mtmp)->shd.x && y == ESHK(mtmp)->shd.y)
		    break;
	    if (!*shops) return;
	}
	for (tmp_dam = level.damagelist; tmp_dam; tmp_dam = tmp_dam->next)
	    if (tmp_dam->place.x == x && tmp_dam->place.y == y) {
		tmp_dam->cost += cost;
		return;
	    }
	tmp_dam = (struct damage *)alloc((unsigned)sizeof(struct damage));
	tmp_dam->when = monstermoves;
	tmp_dam->place.x = x;
	tmp_dam->place.y = y;
	tmp_dam->cost = cost;
	tmp_dam->typ = levl[x][y].typ;
	tmp_dam->next = level.damagelist;
	level.damagelist = tmp_dam;
	/* If player saw damage, display as a wall forever */
	if (cansee(x, y))
	    levl[x][y].seenv = SVALL;
}

#endif /*OVLB*/
#ifdef OVL0

/*
 * Do something about damage. Either (!croaked) try to repair it, or
 * (croaked) just discard damage structs for non-shared locations, since
 * they'll never get repaired. Assume that shared locations will get
 * repaired eventually by the other shopkeeper(s). This might be an erroneous
 * assumption (they might all be dead too), but we have no reasonable way of
 * telling that.
 */
STATIC_OVL
void
remove_damage(shkp, croaked)
register struct monst *shkp;
register boolean croaked;
{
	register struct damage *tmp_dam, *tmp2_dam;
	register boolean did_repair = FALSE, saw_door = FALSE;
	register boolean saw_floor = FALSE, stop_picking = FALSE;
	register boolean saw_untrap = FALSE;
	uchar saw_walls = 0;

	tmp_dam = level.damagelist;
	tmp2_dam = 0;
	while (tmp_dam) {
	    register xchar x = tmp_dam->place.x, y = tmp_dam->place.y;
	    char shops[5];
	    int disposition;

	    disposition = 0;
	    Strcpy(shops, in_rooms(x, y, SHOPBASE));
	    if (index(shops, ESHK(shkp)->shoproom)) {
		if (croaked)
		    disposition = (shops[1])? 0 : 1;
		else if (stop_picking)
		    disposition = repair_damage(shkp, tmp_dam, FALSE);
		else {
		    /* Defer the stop_occupation() until after repair msgs */
		    if (closed_door(x, y))
			stop_picking = picking_at(x, y);
		    disposition = repair_damage(shkp, tmp_dam, FALSE);
		    if (!disposition)
			stop_picking = FALSE;
		}
	    }

	    if (!disposition) {
		tmp2_dam = tmp_dam;
		tmp_dam = tmp_dam->next;
		continue;
	    }

	    if (disposition > 1) {
		did_repair = TRUE;
		if (cansee(x, y)) {
		    if (IS_WALL(levl[x][y].typ))
			saw_walls++;
		    else if (IS_DOOR(levl[x][y].typ))
			saw_door = TRUE;
		    else if (disposition == 3)		/* untrapped */
			saw_untrap = TRUE;
		    else
			saw_floor = TRUE;
		}
	    }

	    tmp_dam = tmp_dam->next;
	    if (!tmp2_dam) {
		free((genericptr_t)level.damagelist);
		level.damagelist = tmp_dam;
	    } else {
		free((genericptr_t)tmp2_dam->next);
		tmp2_dam->next = tmp_dam;
	    }
	}
	if (!did_repair)
	    return;
	if (saw_walls) {
/*JP	    pline("Suddenly, %s section%s of wall close%s up!",
		  (saw_walls == 1) ? "a" : (saw_walls <= 3) ?
						  "some" : "several",
		  (saw_walls == 1) ? "" : "s", (saw_walls == 1) ? "s" : "");*/
	    pline("突然，壁が%s閉まった！",
		  (saw_walls == 1) ? "一箇所" : (saw_walls <= 3) ?
						  "何箇所か" : "あちこちで");
	    if (saw_door)
/*JP		pline_The("shop door reappears!");*/
		pline("店の扉がまた現われた！");

	    if (saw_floor)
/*JP		pline_The("floor is repaired!");*/
		pline("床は修復された！");
	} else {
	    if (saw_door)
/*JP		pline("Suddenly, the shop door reappears!");*/
		pline("突然，店の扉がまた現われた！");
	    else if (saw_floor)
/*JP		pline("Suddenly, the floor damage is gone!");*/
		pline("突然，床の傷がなくなった！");
	    else if (saw_untrap)
/*JP	        pline("Suddenly, the trap is removed from the floor!");*/
	        pline("突然罠が床から消えた！");
	    else if (inside_shop(u.ux, u.uy) == ESHK(shkp)->shoproom)
/*JP		You_feel("more claustrophobic than before.");*/
		You("前より閉所恐怖症気味になった．");
	    else if (flags.soundok && !rn2(10))
/*JP		Norep("The dungeon acoustics noticeably change.");*/
		Norep("迷宮の音響はいちじるしく変った．");
	}
	if (stop_picking)
		stop_occupation();
}

/*
 * 0: repair postponed, 1: silent repair (no messages), 2: normal repair
 * 3: untrap
 */
int
repair_damage(shkp, tmp_dam, catchup)
register struct monst *shkp;
register struct damage *tmp_dam;
boolean catchup;	/* restoring a level */
{
	register xchar x, y, i;
	xchar litter[9];
	register struct monst *mtmp;
	register struct obj *otmp;
	register struct trap *ttmp;

	if ((monstermoves - tmp_dam->when) < REPAIR_DELAY)
	    return(0);
	if (shkp->msleep || !shkp->mcanmove || ESHK(shkp)->following)
	    return(0);
	x = tmp_dam->place.x;
	y = tmp_dam->place.y;
	if (!IS_ROOM(tmp_dam->typ)) {
	    if (x == u.ux && y == u.uy)
		if (!passes_walls(uasmon))
		    return(0);
	    if (x == shkp->mx && y == shkp->my)
		return(0);
	    if ((mtmp = m_at(x, y)) && (!passes_walls(mtmp->data)))
		return(0);
	}
	if ((ttmp = t_at(x, y)) != 0) {
	    if (x == u.ux && y == u.uy)
		if (!passes_walls(uasmon))
		    return(0);
	    if (ttmp->ttyp == LANDMINE || ttmp->ttyp == BEAR_TRAP) {
		/* convert to an object */
		otmp = mksobj((ttmp->ttyp == LANDMINE) ? LAND_MINE :
				BEARTRAP, TRUE, FALSE);
		otmp->quan= 1;
		otmp->owt = weight(otmp);
		mpickobj(shkp, otmp);
	    }
	    deltrap(ttmp);
	    newsym(x, y);
	    return(3);
	}
	if (IS_ROOM(tmp_dam->typ)) {
	    /* No messages if player already filled trapdoor */
	    if (catchup || !ttmp)
		return(1);
	    newsym(x, y);
	    return(2);
	}
	if (!ttmp && (tmp_dam->typ == levl[x][y].typ) &&
	    (!IS_DOOR(tmp_dam->typ) || (levl[x][y].doormask > D_BROKEN)))
	    /* No messages if player already replaced shop door */
	    return(1);
	levl[x][y].typ = tmp_dam->typ;
	(void) memset((genericptr_t)litter, 0, sizeof(litter));
	if ((otmp = level.objects[x][y]) != 0) {
	    /* Scatter objects haphazardly into the shop */
#define NEED_UPDATE 1
#define OPEN	    2
#define INSHOP	    4
#define horiz(i) ((i%3)-1)
#define vert(i)  ((i/3)-1)
	    for (i = 0; i < 9; i++) {
		if ((i == 4) || (!ZAP_POS(levl[x+horiz(i)][y+vert(i)].typ)))
		    continue;
		litter[i] = OPEN;
		if (inside_shop(x+horiz(i),
				y+vert(i)) == ESHK(shkp)->shoproom)
		    litter[i] |= INSHOP;
	    }
	    if (Punished && !u.uswallow &&
				((uchain->ox == x && uchain->oy == y) ||
				 (uball->ox == x && uball->oy == y))) {
		/*
		 * Either the ball or chain is in the repair location.
		 *
		 * Take the easy way out and put ball&chain under hero.
		 */
/*JP		verbalize("Get your junk out of my wall!");*/
		verbalize("そのガラクタを外に持って行きな！");
		unplacebc();	/* pick 'em up */
		placebc();	/* put 'em down */
	    }
	    while ((otmp = level.objects[x][y]) != 0)
		/* Don't mess w/ boulders -- just merge into wall */
		if ((otmp->otyp == BOULDER) || (otmp->otyp == ROCK)) {
		    obj_extract_self(otmp);
		    obfree(otmp, (struct obj *)0);
		} else {
		    while (!(litter[i = rn2(9)] & INSHOP));
			remove_object(otmp);
			place_object(otmp, x+horiz(i), y+vert(i));
			litter[i] |= NEED_UPDATE;
		}
	}
	if (catchup) return 1;	/* repair occurred while off level */

	block_point(x, y);
	if(IS_DOOR(tmp_dam->typ)) {
	    levl[x][y].doormask = D_CLOSED; /* arbitrary */
	    newsym(x, y);
	} else {
	    /* don't set doormask  - it is (hopefully) the same as it was */
	    /* if not, perhaps save it with the damage array...  */

	    if (IS_WALL(tmp_dam->typ) && cansee(x, y)) {
	    /* Player sees actual repair process, so they KNOW it's a wall */
		levl[x][y].seenv = SVALL;
		newsym(x, y);
	    }
	    /* Mark this wall as "repaired".  There currently is no code */
	    /* to do anything about repaired walls, so don't do it.	 */
	}
	for (i = 0; i < 9; i++)
	    if (litter[i] & NEED_UPDATE)
		newsym(x+horiz(i), y+vert(i));
	return(2);
#undef NEED_UPDATE
#undef OPEN
#undef INSHOP
#undef vert
#undef horiz
}
#endif /*OVL0*/
#ifdef OVL3
/*
 * shk_move: return 1: moved  0: didn't  -1: let m_move do it  -2: died
 */
int
shk_move(shkp)
register struct monst *shkp;
{
	register xchar gx,gy,omx,omy;
	register int udist;
	register schar appr;
	register struct eshk *eshkp = ESHK(shkp);
	int z;
	boolean uondoor = FALSE, satdoor, avoid = FALSE, badinv;

	omx = shkp->mx;
	omy = shkp->my;

	if (inhishop(shkp))
	    remove_damage(shkp, FALSE);

	if((udist = distu(omx,omy)) < 3 &&
	   (shkp->data != &mons[PM_GRID_BUG] || (omx==u.ux || omy==u.uy))) {
		if(ANGRY(shkp) ||
		   (Conflict && !resist(shkp, RING_CLASS, 0, 0))) {
			if(Displaced)
/*JP			  Your("displaced image doesn't fool %s!",*/
			  pline("%sはあなたの幻影にだまされなかった！",
				mon_nam(shkp));
			(void) mattacku(shkp);
			return(0);
		}
		if(eshkp->following) {
			if(strncmp(eshkp->customer, plname, PL_NSIZ)) {
/*JP			    verbalize("Hello, %s!  I was looking for %s.",*/
			    verbalize("こんにちは%s！わたしは%sを探しています．",
				    plname, eshkp->customer);
				    eshkp->following = 0;
			    return(0);
			}
			if(moves > followmsg+4) {
/*JP			    verbalize("Hello, %s!  Didn't you forget to pay?",*/
			    verbalize("こんにちは%s！支払いを忘れていませんか？",
				    plname);
			    followmsg = moves;
			    if (!rn2(9)) {
/*JP			      pline("%s doesn't like customers who don't pay.",*/
			      pline ("%sは金を払わない客が嫌いみたいだ．",
				    Monnam(shkp));
				rile_shk(shkp);
			    }
			}
			if(udist < 2)
			    return(0);
		}
	}

	appr = 1;
	gx = eshkp->shk.x;
	gy = eshkp->shk.y;
	satdoor = (gx == omx && gy == omy);
	if(eshkp->following || ((z = holetime()) >= 0 && z*z <= udist)){
		if(udist > 4)
		    return(-1);	/* leave it to m_move */
		gx = u.ux;
		gy = u.uy;
	} else if(ANGRY(shkp)) {
		/* Move towards the hero if the shopkeeper can see him. */
		if(shkp->mcansee && m_canseeu(shkp)) {
			gx = u.ux;
			gy = u.uy;
		}
		avoid = FALSE;
	} else {
#define	GDIST(x,y)	(dist2(x,y,gx,gy))
		if(Invis) {
		    avoid = FALSE;
		} else {
		    uondoor = (u.ux == eshkp->shd.x && u.uy == eshkp->shd.y);
		    if(uondoor) {
			badinv = (carrying(PICK_AXE) ||
				  (Fast && sobj_at(PICK_AXE, u.ux, u.uy)));
			if(satdoor && badinv)
			    return(0);
			avoid = !badinv;
		    } else {
			avoid = (*u.ushops && distu(gx,gy) > 8);
			badinv = FALSE;
		    }

		    if(((!eshkp->robbed && !eshkp->billct && !eshkp->debit)
			|| avoid) && GDIST(omx,omy) < 3) {
			if (!badinv && !onlineu(omx,omy))
			    return(0);
			if(satdoor)
			    appr = gx = gy = 0;
		    }
		}
	}

	return(move_special(shkp,inhishop(shkp),
			    appr,uondoor,avoid,omx,omy,gx,gy));
}

#endif /*OVL3*/
#ifdef OVLB

/* for use in levl_follower (mondata.c) */
boolean
is_fshk(mtmp)
register struct monst *mtmp;
{
	return((boolean)(mtmp->isshk && ESHK(mtmp)->following));
}

/* You are digging in the shop. */
void
shopdig(fall)
register int fall;
{
    register struct monst *shkp = shop_keeper(*u.ushops);

    if(!shkp) return;

    if(!inhishop(shkp)) {
	if (Role_is('K')) adjalign(-sgn(u.ualign.type));
	return;
    }

    if(!fall) {
	if(u.utraptype == TT_PIT)
/*JP	    verbalize("Be careful, %s, or you might fall through the floor.",*/
	    verbalize("注意してください%s，床から落ちますよ．",
/*JP		flags.female ? "madam" : "sir");*/
		flags.female ? "お嬢さん" : "お客さん");
	else
/*JP	    verbalize("%s, do not damage the floor here!",
			flags.female ? "Madam" : "Sir");*/
	    verbalize("%s，床に傷をつけないでください！",
			flags.female ? "お嬢さん" : "お客さん");
	if (Role_is('K')) adjalign(-sgn(u.ualign.type));
    } else if(!um_dist(shkp->mx, shkp->my, 5) &&
		!shkp->msleep && shkp->mcanmove &&
		(ESHK(shkp)->billct || ESHK(shkp)->debit)) {
	    register struct obj *obj, *obj2;

	    if (distu(shkp->mx, shkp->my) > 2) {
		mnexto(shkp);
		/* for some reason the shopkeeper can't come next to you */
		if (distu(shkp->mx, shkp->my) > 2) {
/*JP		    pline("%s curses you in anger and frustration!",*/
		    pline("怒りで不満のたまっている%sはあなたを呪った！",
					shkname(shkp));
		    rile_shk(shkp);
		    return;
/*JP		} else pline("%s leaps, and grabs your backpack!",*/
		} else pline("%sは飛びついて，あなたの背負い袋をつかんだ！",
					shkname(shkp));
/*JP	    } else pline("%s grabs your backpack!", shkname(shkp));*/
	    } else pline("%sはあなたの背負い袋をつかんだ！", shkname(shkp));

	    for(obj = invent; obj; obj = obj2) {
		obj2 = obj->nobj;
		if(obj->owornmask) continue;
		if(obj->otyp == LEASH && obj->leashmon) continue;
		freeinv(obj);
		add_to_minv(shkp, obj);
		subfrombill(obj, shkp);
	    }
    }
}

#ifdef KOPS
STATIC_OVL void
makekops(mm)
coord *mm;
{
	static const short k_mndx[4] = {
	    PM_KEYSTONE_KOP, PM_KOP_SERGEANT, PM_KOP_LIEUTENANT, PM_KOP_KAPTAIN
	};
	int k_cnt[4], cnt, mndx, k;

	k_cnt[0] = cnt = abs(depth(&u.uz)) + rnd(5);
	k_cnt[1] = (cnt / 3) + 1;	/* at least one sarge */
	k_cnt[2] = (cnt / 6);		/* maybe a lieutenant */
	k_cnt[3] = (cnt / 9);		/* and maybe a kaptain */

	for (k = 0; k < 4; k++) {
	    if ((cnt = k_cnt[k]) == 0) break;
	    mndx = k_mndx[k];
	    if (mvitals[mndx].mvflags & G_GONE) continue;

	    while (cnt--)
		if (enexto(mm, mm->x, mm->y, &mons[mndx]))
		    (void) makemon(&mons[mndx], mm->x, mm->y, NO_MM_FLAGS);
	}
}
#endif	/* KOPS */

void
pay_for_damage(dmgstr)
const char *dmgstr;
{
	register struct monst *shkp = (struct monst *)0;
	char shops_affected[5];
	register boolean uinshp = (*u.ushops != '\0');
	char qbuf[80];
	register xchar x, y;
#if 0 /*JP*/
	boolean dugwall = !strcmp(dmgstr, "dig into") ||	/* wand */
			  !strcmp(dmgstr, "damage");		/* pick-axe */
#endif 
	boolean dugwall = !strcmp(dmgstr, "穴を開ける") ||	/* wand */
			  !strcmp(dmgstr, "傷つける");		/* pick-axe */
	struct damage *tmp_dam, *appear_here = 0;
	/* any number >= (80*80)+(24*24) would do, actually */
	long cost_of_damage = 0L;
	unsigned int nearest_shk = 7000, nearest_damage = 7000;
	int picks = 0;

	for (tmp_dam = level.damagelist;
	     (tmp_dam && (tmp_dam->when == monstermoves));
	     tmp_dam = tmp_dam->next) {
	    char *shp;

	    if (!tmp_dam->cost)
		continue;
	    cost_of_damage += tmp_dam->cost;
	    Strcpy(shops_affected,
		   in_rooms(tmp_dam->place.x, tmp_dam->place.y, SHOPBASE));
	    for (shp = shops_affected; *shp; shp++) {
		struct monst *tmp_shk;
		unsigned int shk_distance;

		if (!(tmp_shk = shop_keeper(*shp)))
		    continue;
		if (tmp_shk == shkp) {
		    unsigned int damage_distance =
				   distu(tmp_dam->place.x, tmp_dam->place.y);

		    if (damage_distance < nearest_damage) {
			nearest_damage = damage_distance;
			appear_here = tmp_dam;
		    }
		    continue;
		}
		if (!inhishop(tmp_shk))
		    continue;
		shk_distance = distu(tmp_shk->mx, tmp_shk->my);
		if (shk_distance > nearest_shk)
		    continue;
		if ((shk_distance == nearest_shk) && picks) {
		    if (rn2(++picks))
			continue;
		} else
		    picks = 1;
		shkp = tmp_shk;
		nearest_shk = shk_distance;
		appear_here = tmp_dam;
		nearest_damage = distu(tmp_dam->place.x, tmp_dam->place.y);
	    }
	}

	if (!cost_of_damage || !shkp)
	    return;

	x = appear_here->place.x;
	y = appear_here->place.y;

	/* not the best introduction to the shk... */
	(void) strncpy(ESHK(shkp)->customer,plname,PL_NSIZ);

	/* if the shk is already on the war path, be sure it's all out */
	if(ANGRY(shkp) || ESHK(shkp)->following) {
		hot_pursuit(shkp);
		return;
	}

	/* if the shk is not in their shop.. */
	if(!*in_rooms(shkp->mx,shkp->my,SHOPBASE)) {
		if(!cansee(shkp->mx, shkp->my))
			return;
		goto getcad;
	}

	if(uinshp) {
		if(um_dist(shkp->mx, shkp->my, 1) &&
			!um_dist(shkp->mx, shkp->my, 3)) {
/*JP		    pline("%s leaps towards you!", shkname(shkp));*/
		    pline("%sはあなたに飛びかかった！", shkname(shkp));
		    mnexto(shkp);
		}
		if(um_dist(shkp->mx, shkp->my, 1)) goto getcad;
	} else {
	    /*
	     * Make shkp show up at the door.  Effect:  If there is a monster
	     * in the doorway, have the hero hear the shopkeeper yell a bit,
	     * pause, then have the shopkeeper appear at the door, having
	     * yanked the hapless critter out of the way.
	     */
	    if (MON_AT(x, y)) {
		if(flags.soundok) {
/*JP		    You_hear("an angry voice:");*/
		    You("怒りの声を聞いた：");
/*JP		    verbalize("Out of my way, scum!");*/
		    verbalize("どけ！クソったれ！");
		    wait_synch();
#if defined(UNIX) || defined(VMS)
# if defined(SYSV) || defined(ULTRIX) || defined(VMS)
		    (void)
# endif
			sleep(1);
#endif
		}
	    }
	    (void) mnearto(shkp, x, y, TRUE);
	}

	if((um_dist(x, y, 1) && !uinshp) ||
			(u.ugold + ESHK(shkp)->credit) < cost_of_damage
				|| !rn2(50)) {
		if(um_dist(x, y, 1) && !uinshp) {
/*JP		    pline("%s shouts:", shkname(shkp));*/
		    pline("%sはさけんだ：", shkname(shkp));
/*JP		    verbalize("Who dared %s my %s?", dmgstr,
					 dugwall ? "shop" : "door");*/
		    verbalize("誰が%sを%sりしたんだろう？",
					 dugwall ? "店" : "扉",jconj(dmgstr,"た"));
		} else {
getcad:
/*JP		    verbalize("How dare you %s my %s?", dmgstr,
					 dugwall ? "shop" : "door");*/
		    verbalize("どうして%sを%sりしたんだ？",
					 dugwall ? "店" : "扉",jconj(dmgstr,"た"));
		}
		hot_pursuit(shkp);
		return;
	}

/*JP	if(Invis) Your("invisibility does not fool %s!", shkname(shkp));*/
	if(Invis) pline("%sは透明なあなたにだまされなかった！", shkname(shkp));
/*JP	Sprintf(qbuf,"\"Cad!  You did %ld zorkmids worth of damage!\"  Pay? ",*/
	Sprintf(qbuf,"「おい！%ldゴールドの損害だ！」払いますか？",
		 cost_of_damage);
	if(yn(qbuf) != 'n') {
		cost_of_damage = check_credit(cost_of_damage, shkp);
		u.ugold -= cost_of_damage;
		shkp->mgold += cost_of_damage;
		flags.botl = 1;
/*JP		pline("Mollified, %s accepts your restitution.",*/
		pline("%sは，感情をやわらげ賠償金を受けとった．",
			shkname(shkp));
		/* move shk back to his home loc */
		home_shk(shkp, FALSE);
		pacify_shk(shkp);
	} else {
/*JP		verbalize("Oh, yes!  You'll pay!");*/
		verbalize("さあ，払うんだ！");
		hot_pursuit(shkp);
		adjalign(-sgn(u.ualign.type));
	}
}
#endif /*OVLB*/
#ifdef OVL0
/* called in dokick.c when we kick an object that might be in a store */
boolean
costly_spot(x, y)
register xchar x, y;
{
	register struct monst *shkp;

	if (!level.flags.has_shop) return FALSE;
	shkp = shop_keeper(*in_rooms(x, y, SHOPBASE));
	if(!shkp || !inhishop(shkp)) return(FALSE);

	return((boolean)(inside_shop(x, y) &&
		!(x == ESHK(shkp)->shk.x &&
			y == ESHK(shkp)->shk.y)));
}
#endif /*OVL0*/
#ifdef OVLB

/* called by dotalk(sounds.c) when #chatting; returns obj if location
   contains shop goods and shopkeeper is willing & able to speak */
struct obj *
shop_object(x, y)
register xchar x, y;
{
    register struct obj *otmp;
    register struct monst *shkp;

    if(!(shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) || !inhishop(shkp))
	return(struct obj *)0;

    for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
	if (otmp->oclass != GOLD_CLASS)
	    break;
    /* note: otmp might have ->no_charge set, but that's ok */
    return (otmp && costly_spot(x, y) && NOTANGRY(shkp)
	    && shkp->mcanmove && !shkp->msleep)
		? otmp : (struct obj *)0;
}

/* give price quotes for all objects linked to this one (ie, on this spot) */
void
price_quote(first_obj)
register struct obj *first_obj;
{
    register struct obj *otmp;
    char buf[BUFSZ], price[40];
    long cost;
    int cnt = 0;
    winid tmpwin;
    struct monst *shkp = shop_keeper(inside_shop(u.ux, u.uy));

    tmpwin = create_nhwindow(NHW_MENU);
/*JP    putstr(tmpwin, 0, "Fine goods for sale:");*/
    putstr(tmpwin, 0, "売りにでているすばらしい商品：");
    putstr(tmpwin, 0, "");
    for (otmp = first_obj; otmp; otmp = otmp->nexthere) {
	if (otmp->oclass == GOLD_CLASS) continue;
	cost = (otmp->no_charge || otmp == uball || otmp == uchain) ? 0L :
		get_cost(otmp, (struct monst *)0);
	if (Has_contents(otmp))
	    cost += contained_cost(otmp, shkp, 0L, FALSE);
	if (!cost) {
/*JP	    Strcpy(price, "no charge");*/
	    Strcpy(price, "無料");
	} else {
/*JP	    Sprintf(price, "%ld zorkmid%s%s", cost, plur(cost),
		    otmp->quan > 1L ? " each" : "");*/
	    Sprintf(price, "%s%ldゴールド", 
		    otmp->quan > 1L ? "それぞれ" : "", cost);
	}
/*JP	Sprintf(buf, "%s, %s", doname(otmp), price);*/
	Sprintf(buf, "%s，%s", doname(otmp), price);
	putstr(tmpwin, 0, buf),  cnt++;
    }
    if (cnt > 1) {
	display_nhwindow(tmpwin, TRUE);
    } else if (cnt == 1) {
	if (first_obj->no_charge || first_obj == uball || first_obj == uchain){
/*JP	    pline("%s!", buf);	*//* buf still contains the string */
	    pline("%s！", buf);	/* buf still contains the string */
	} else {
	    /* print cost in slightly different format, so can't reuse buf */
	    cost = get_cost(first_obj, (struct monst *)0);
	    if (Has_contents(first_obj))
		cost += contained_cost(first_obj, shkp, 0L, FALSE);
/*JP	    pline("%s, price %ld zorkmid%s%s%s", doname(first_obj),
		cost, plur(cost), first_obj->quan > 1L ? " each" : "",
		shk_embellish(first_obj, cost));*/
	    pline("%s%sは%s%ldゴールドだ．", 
		  shk_embellish(first_obj, cost),
		  doname(first_obj),
		  first_obj->quan > 1L ? "それぞれ" : "",
		  cost);
	}
    }
    destroy_nhwindow(tmpwin);
}
#endif /*OVLB*/
#ifdef OVL3

STATIC_OVL const char *
shk_embellish(itm, cost)
register struct obj *itm;
long cost;
{
    if (!rn2(3)) {
	register int o, choice = rn2(5);
	if (choice == 0) choice = (cost < 100L ? 1 : cost < 500L ? 2 : 3);
	switch (choice) {
	    case 4:
		if (cost < 10L) break; else o = itm->oclass;
/*JP		if (o == FOOD_CLASS) return ", gourmets' delight!";*/
		if (o == FOOD_CLASS) return "グルメが泣いて喜ぶ";
		if (objects[itm->otyp].oc_name_known
		    ? objects[itm->otyp].oc_magic
		    : (o == AMULET_CLASS || o == RING_CLASS   ||
		       o == WAND_CLASS   || o == POTION_CLASS ||
		       o == SCROLL_CLASS || o == SPBOOK_CLASS))
/*JP		    return ", painstakingly developed!";*/
		    return "一級の魔力を秘めた";
/*JP		return ", superb craftsmanship!";*/
		return "一流職人の作った";
/*JP	    case 3: return ", finest quality.";
	    case 2: return ", an excellent choice.";
	    case 1: return ", a real bargain.";*/
	    case 3: return "最高の品質を誇る";
	    case 2: return "さすがお客さん目が高い！";
	    case 1: return "本日の目玉商品！";
	   default: break;
	}
    } else if (itm->oartifact) {
/*JP	return ", one of a kind!";*/
	return "これは世界にまたとない！";
    }
/*JP    return ".";*/
    return "";
}
#endif /*OVL3*/
#ifdef OVLB

/* First 4 supplied by Ronen and Tamar, remainder by development team */
const char *Izchak_speaks[]={
#if 0
    "%s says: 'These shopping malls give me a headache.'",
    "%s says: 'Slow down.  Think clearly.'",
    "%s says: 'You need to take things one at a time.'",
    "%s says: 'I don't like poofy coffee... give me Columbian Supremo.'",
    "%s says that getting the devteam's agreement on anything is difficult.",
    "%s says that he has noticed those who serve their deity will prosper.",
    "%s says: 'Don't try to steal from me - I have friends in high places!'",
    "%s says: 'You may well need something from this shop in the future.'",
    "%s comments about the Valley of the Dead as being a gateway."
#endif
    "%s曰く『これらショッピング街は頭痛のタネだ．』",
    "%s曰く『ゆっくり考えよ．』",
    "%s曰く『一度に一個取る必要がある．』",
    "%s曰く『ホモっぽいコーヒは好きじゃない．．．アメリカのやつをたのむ．』",
    "開発チームに何らかの協定を求めることは困難だと%sは述べた．",
    "%sは神に仕えるやつは金を儲けていると述べた．",
    "%s曰く『私から盗もうなんて思わないことだ．上の方に知り合いがいるんだから．』",
    "%s曰く『未来においてこの店で何かを必要とするだろう』",
    "%sは死の谷はゲートウェイだろうとコメントを述べた．"
};

void
shk_chat(shkp)
register struct monst *shkp;
{
	register struct eshk *eshk = ESHK(shkp);

	if (ANGRY(shkp))
/*JP		pline("%s mentions how much %s dislikes %s customers.",
			shkname(shkp), he[shkp->female],
			eshk->robbed ? "non-paying" : "rude");*/
		pline("%sは%s客は大嫌いだと言った．",
			shkname(shkp), 
			eshk->robbed ? "金を支払わない" : "無礼な");
	else if (eshk->following) {
		if (strncmp(eshk->customer, plname, PL_NSIZ)) {
/*JP		    verbalize("Hello %s!  I was looking for %s.",*/
		    verbalize("こんにちは%s！私は%sを探しています．",
			    plname, eshk->customer);
		    eshk->following = 0;
		} else {
/*JP		    verbalize("Hello %s!  Didn't you forget to pay?", plname);*/
		    verbalize("こんにちは%s！支払いを忘れていませんか？", plname);
		}
	} else if (eshk->billct) {
		register long total = addupbill(shkp) + eshk->debit;
/*JP		pline("%s says that your bill comes to %ld zorkmid%s.",
		      shkname(shkp), total, plur(total));*/
		pline("%sは勘定が%ldゴールドになると言った．",
		      shkname(shkp), total);
	} else if (eshk->debit)
/*JP		pline("%s reminds you that you owe %s %ld zorkmid%s.",
		      shkname(shkp), him[shkp->female],
		      eshk->debit, plur(eshk->debit));*/
		pline("あなたは%sに%ldゴールドの借りがあることを思いだした",
		      shkname(shkp),
		      eshk->debit);
	else if (eshk->credit)
/*JP		pline("%s encourages you to use your %ld zorkmid%s of credit.",
		      shkname(shkp), eshk->credit, plur(eshk->credit));*/
		pline("%sはクレジットで%ldゴールド使うよう勧めた．",
		      shkname(shkp), eshk->credit);
	else if (eshk->robbed)
/*JP		pline("%s complains about a recent robbery.", shkname(shkp));*/
		pline("%s最近の強盗について愚痴をこぼした．", shkname(shkp));
	else if (shkp->mgold < 50)
/*JP		pline("%s complains that business is bad.", shkname(shkp));*/
		pline("%sは商売が旨くいってないと愚痴をこぼした．", shkname(shkp));
	else if (shkp->mgold > 4000)
/*JP		pline("%s says that business is good.", shkname(shkp));*/
		pline("%sは商売が旨くいっていると言った．", shkname(shkp));
	else if (strcmp(shkname(shkp), "Izchak") == 0)
		pline(Izchak_speaks[rn2(SIZE(Izchak_speaks))],shkname(shkp));
	else
/*JP		pline("%s talks about the problem of shoplifters.",shkname(shkp));*/
		pline("%sは万引の問題について話した．", shkname(shkp));
}

#ifdef KOPS
STATIC_OVL void
kops_gone(silent)
register boolean silent;
{
	register int cnt = 0;
	register struct monst *mtmp, *mtmp2;

	for (mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;
	    if (mtmp->data->mlet == S_KOP) {
		if (canspotmon(mtmp)) cnt++;
		mongone(mtmp);
	    }
	}
	if (cnt && !silent)
/*JP	    pline_The("Kop%s (disappointed) vanish%s into thin air.",
		      plur(cnt), cnt == 1 ? "es" : "");*/
	    pline("がっくりした警官は空気にとけて消えた．");
}
#endif	/* KOPS */

#endif /*OVLB*/
#ifdef OVL3

STATIC_OVL long
cost_per_charge(shkp, otmp)
struct monst *shkp;
struct obj *otmp;
{
	long tmp = 0L;

	if(!shkp || !inhishop(shkp)) return(0L); /* insurance */
	tmp = get_cost(otmp, shkp);

	/* The idea is to make the exhaustive use of */
	/* an unpaid item more expensive than buying */
	/* it outright.				     */
	if(otmp->otyp == MAGIC_LAMP) {			 /* 1 */
		tmp += tmp / 3L;
	} else if(otmp->otyp == MAGIC_MARKER) {		 /* 70 - 100 */
		/* no way to determine in advance   */
		/* how many charges will be wasted. */
		/* so, arbitrarily, one half of the */
		/* price per use.		    */
		tmp /= 2L;
	} else if(otmp->otyp == BAG_OF_TRICKS ||	 /* 1 - 20 */
		  otmp->otyp == HORN_OF_PLENTY) {
		tmp /= 5L;
	} else if(otmp->otyp == CRYSTAL_BALL ||		 /* 1 - 5 */
		  otmp->otyp == OIL_LAMP ||		 /* 1 - 10 */
		  otmp->otyp == BRASS_LANTERN ||
		 (otmp->otyp >= MAGIC_FLUTE &&
		  otmp->otyp <= DRUM_OF_EARTHQUAKE) ||	 /* 5 - 9 */
		  otmp->oclass == WAND_CLASS) {		 /* 3 - 11 */
		    if (otmp->spe > 1) tmp /= 4L;
	} else if (otmp->oclass == SPBOOK_CLASS) {
		    tmp -= tmp / 5L;
	} else if (otmp->otyp == CAN_OF_GREASE) {
		    tmp /= 10L;
	} else if (otmp->otyp == POT_OIL) {
		    tmp /= 5L;
	}
	return(tmp);
}
#endif /*OVL3*/
#ifdef OVLB

/* for using charges of unpaid objects */
void
check_unpaid(otmp)
register struct obj *otmp;
{
	struct monst *shkp;
	const char *fmt, *arg1, *arg2;
	long tmp;

	if (!otmp->unpaid || !*u.ushops ||
		(otmp->spe <= 0 && objects[otmp->otyp].oc_charged))
	    return;
	if (!(shkp = shop_keeper(*u.ushops)) || !inhishop(shkp) ||
		(tmp = cost_per_charge(shkp, otmp)) == 0L)
	    return;

	arg1 = arg2 = "";
	if (otmp->oclass == SPBOOK_CLASS) {
/*JP	    fmt = "%sYou owe%s %ld zorkmids.";*/
	    fmt = "%s%s%ldゴールドの借りだ．";
/*JP	    arg1 = rn2(2) ? "This is no free library, cad!  " : "";*/
	    arg1 = rn2(2) ? "おい！ここは図書館じゃない！" : "";
/*JP	    arg2 = ESHK(shkp)->debit > 0L ? " an additional" : "";*/
	    arg2 = ESHK(shkp)->debit > 0L ? "さらに言えば" : "";
	} else if (otmp->otyp == POT_OIL) {
/*JP	    fmt = "%s%sThat will cost you %ld zorkmids (Yendorian Fuel Tax).";*/
	    fmt = "%s%s値段は%ldゴールド(イェンダー燃料税)だ．";
	} else {
/*JP	    fmt = "%s%sUsage fee, %ld zorkmids.";*/
	    fmt = "%s%s使用料は，%ldゴールドだ．";
/*JP	    if (!rn2(3)) arg1 = "Hey!  ";
	    if (!rn2(3)) arg2 = "Ahem.  ";*/
	    if (!rn2(3)) arg1 = "おい！";
	    if (!rn2(3)) arg2 = "";
	}

	if (shkp->mcanmove || !shkp->msleep)
	    verbalize(fmt, arg1, arg2, tmp);
	ESHK(shkp)->debit += tmp;
	exercise(A_WIS, TRUE);		/* you just got info */
}

void
costly_gold(x, y, amount)
register xchar x, y;
register long amount;
{
	register long delta;
	register struct monst *shkp;
	register struct eshk *eshkp;

	if(!costly_spot(x, y)) return;
	/* shkp now guaranteed to exist by costly_spot() */
	shkp = shop_keeper(*in_rooms(x, y, SHOPBASE));

	eshkp = ESHK(shkp);
	if(eshkp->credit >= amount) {
	    if(eshkp->credit > amount)
/*JP		Your("credit is reduced by %ld zorkmid%s.",*/
		Your("クレジットは%ldゴールド減った．",
					amount);
/*JP	    else Your("credit is erased.");*/
	    else Your("クレジットは帳消しになった．");
	    eshkp->credit -= amount;
	} else {
	    delta = amount - eshkp->credit;
	    if(eshkp->credit)
/*JP		Your("credit is erased.");*/
	        Your("クレジットは帳消しになった．");
	    if(eshkp->debit)
/*JP		Your("debt increases by %ld zorkmid%s.",*/
		Your("借金は%ldゴールドに増えた．",
					delta);
/*JP	    else You("owe %s %ld zorkmid%s.",*/
	    else You("%sに%ldゴールドの借りをつくった．",
				shkname(shkp), delta);
	    eshkp->debit += delta;
	    eshkp->loan += delta;
	    eshkp->credit = 0L;
	}
}

/* used in domove to block diagonal shop-exit */
/* x,y should always be a door */
boolean
block_door(x,y)
register xchar x, y;
{
	register int roomno = *in_rooms(x, y, SHOPBASE);
	register struct monst *shkp;

	if(roomno < 0 || !IS_SHOP(roomno)) return(FALSE);
	if(!IS_DOOR(levl[x][y].typ)) return(FALSE);
	if(roomno != *u.ushops) return(FALSE);

	if(!(shkp = shop_keeper((char)roomno)) || !inhishop(shkp))
		return(FALSE);

	if(shkp->mx == ESHK(shkp)->shk.x && shkp->my == ESHK(shkp)->shk.y
	    /* Actually, the shk should be made to block _any_
	     * door, including a door the player digs, if the
	     * shk is within a 'jumping' distance.
	     */
	    && ESHK(shkp)->shd.x == x && ESHK(shkp)->shd.y == y
	    && shkp->mcanmove && !shkp->msleep
	    && (ESHK(shkp)->debit || ESHK(shkp)->billct ||
		ESHK(shkp)->robbed)) {
/*JP		pline("%s%s blocks your way!", shkname(shkp),
				Invis ? " senses your motion and" : "");*/
		pline("%sは%sあなたの前に立ちふさがった！", shkname(shkp),
				Invis ? "動きを感じとり，" : "");
		return(TRUE);
	}
	return(FALSE);
}

/* used in domove to block diagonal shop-entry */
/* u.ux, u.uy should always be a door */
boolean
block_entry(x,y)
register xchar x, y;
{
	register xchar sx, sy;
	register int roomno;
	register struct monst *shkp;

	if(!(IS_DOOR(levl[u.ux][u.uy].typ) &&
		levl[u.ux][u.uy].doormask == D_BROKEN)) return(FALSE);

	roomno = *in_rooms(x, y, SHOPBASE);
	if(roomno < 0 || !IS_SHOP(roomno)) return(FALSE);
	if(!(shkp = shop_keeper((char)roomno)) || !inhishop(shkp))
		return(FALSE);

	if(ESHK(shkp)->shd.x != u.ux || ESHK(shkp)->shd.y != u.uy)
		return(FALSE);

	sx = ESHK(shkp)->shk.x;
	sy = ESHK(shkp)->shk.y;

	if(shkp->mx == sx && shkp->my == sy
		&& shkp->mcanmove && !shkp->msleep
		&& (x == sx-1 || x == sx+1 || y == sy-1 || y == sy+1)
		&& (Invis || carrying(PICK_AXE))
	  ) {
/*JP		pline("%s%s blocks your way!", shkname(shkp),
				Invis ? " senses your motion and" : "");*/
		pline("%s%sあなたの前に立ちふさがった！", shkname(shkp),
				Invis ? "動きを感じとり，" : "");
		return(TRUE);
	}
	return(FALSE);
}

#endif /* OVLB */
#ifdef OVL2

char *
shk_your(buf, obj)
char *buf;
struct obj *obj;
{
	if (!shk_owns(buf, obj) && !mon_owns(buf, obj))
/*JP	    Strcpy(buf, obj->where == OBJ_INVENT ? "your" : "the");*/
	    Strcpy(buf, obj->where == OBJ_INVENT ? "" : "");
	return buf;
}

char *
Shk_Your(buf, obj)
char *buf;
struct obj *obj;
{
	(void) shk_your(buf, obj);
	*buf = highc(*buf);
	return buf;
}

STATIC_OVL char *
shk_owns(buf, obj)
char *buf;
struct obj *obj;
{
	struct monst *shkp;
	xchar x, y;

	if (get_obj_location(obj, &x, &y, 0) &&
	    (obj->unpaid ||
	     (obj->where==OBJ_FLOOR && !obj->no_charge && costly_spot(x,y)))) {
	    shkp = shop_keeper(inside_shop(x, y));
/*JP	    return strcpy(buf, shkp ? s_suffix(shkname(shkp)) : "the");*/
	    if (shkp) {
		strcpy(buf, shkname(shkp));
		strcat(buf, "の");
	    } else {
		strcpy(buf, "");
	    }
	    return buf;
	}
	return (char *)0;
}

STATIC_OVL char *
mon_owns(buf, obj)
char *buf;
struct obj *obj;
{
	if (obj->where == OBJ_MINVENT)
/*JP	    return strcpy(buf, s_suffix(mon_nam(obj->ocarry)));*/
	{
	    strcpy(buf, mon_nam(obj->ocarry));
	    strcat(buf, "の");
	    return buf;
	}
	return (char *)0;
}

#endif /* OVL2 */
#ifdef OVLB

#ifdef __SASC
void
sasc_bug(struct obj *op, unsigned x){
	op->unpaid=x;
}
#endif

#endif /* OVLB */

/*shk.c*/
