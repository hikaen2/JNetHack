/*	SCCS Id: @(#)wield.c	3.3	1999/08/16	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"

/* KMH -- Differences between the three weapon slots.
 *
 * The main weapon (uwep):
 * 1.  Is filled by the (w)ield command.
 * 2.  Can be filled with any type of item.
 * 3.  May be carried in one or both hands.
 * 4.  Is used as the melee weapon and as the launcher for
 *     ammunition.
 * 5.  Only conveys intrinsics when it is a weapon, weapon-tool,
 *     or artifact.
 * 6.  Certain cursed items will weld to the hand and cannot be
 *     unwielded or dropped.  See erodeable_wep() and will_weld()
 *     below for the list of which items apply.
 *
 * The secondary weapon (uswapwep):
 * 1.  Is filled by the e(x)change command, which swaps this slot
 *     with the main weapon.  If the "pushweapon" option is set,
 *     the (w)ield command will also store the old weapon in the
 *     secondary slot.
 * 2.  Can be field with anything that will fit in the main weapon
 *     slot; that is, any type of item.
 * 3.  Is usually NOT considered to be carried in the hands.
 *     That would force too many checks among the main weapon,
 *     second weapon, shield, gloves, and rings; and it would
 *     further be complicated by bimanual weapons.  A special
 *     exception is made for two-weapon combat.
 * 4.  Is used as the second weapon for two-weapon combat, and as
 *     a convenience to swap with the main weapon.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld (see #3 for reasons), but they also
 *     prevent two-weapon combat.
 *
 * The quiver (uquiver):
 * 1.  Is filled by the (Q)uiver command.
 * 2.  Can be filled with any type of item.
 * 3.  Is considered to be carried in a special part of the pack.
 * 4.  Is used as the item to throw with the (f)ire command.
 *     This is a convenience over the normal (t)hrow command.
 * 5.  Never conveys intrinsics.
 * 6.  Cursed items never weld; their effect is handled by the normal
 *     throwing code.
 *
 * No item may be in more than one of these slots.
 */


static int FDECL(ready_weapon, (struct obj *));

/* elven weapons vibrate warningly when enchanted beyond a limit */
#define is_elven_weapon(optr)	((optr)->otyp == ELVEN_ARROW\
				|| (optr)->otyp == ELVEN_SPEAR\
				|| (optr)->otyp == ELVEN_DAGGER\
				|| (optr)->otyp == ELVEN_SHORT_SWORD\
				|| (optr)->otyp == ELVEN_BROADSWORD\
				|| (optr)->otyp == ELVEN_BOW)

/* used by will_weld() */
/* probably should be renamed */
#define erodeable_wep(optr)	((optr)->oclass == WEAPON_CLASS \
				|| is_weptool(optr) \
				|| (optr)->otyp == HEAVY_IRON_BALL \
				|| (optr)->otyp == IRON_CHAIN)

/* used by welded(), and also while wielding */
#define will_weld(optr)		((optr)->cursed \
				&& (erodeable_wep(optr) \
				   || (optr)->otyp == TIN_OPENER))


/*** Functions that place a given item in a slot ***/
/* Proper usage includes:
 * 1.  Initializing the slot during character generation or a
 *     restore.
 * 2.  Setting the slot due to a player's actions.
 * 3.  If one of the objects in the slot are split off, these
 *     functions can be used to put the remainder back in the slot.
 * 4.  Putting an item that was thrown and returned back into the slot.
 * 5.  Emptying the slot, by passing a null object.  NEVER pass
 *     zeroobj!
 *
 * Note: setuwep() with a null obj, and uwepgone(), are NOT the same!
 * Sometimes unwielding a weapon can kill you, and lifesaving will then
 * put it back into your hand.  If lifesaving is permitted to do this,
 * use setwuep((struct obj *)0); otherwise use uwepgone().
 *
 * If the item is being moved from another slot, it is the caller's
 * responsibility to handle that.  It's also the caller's responsibility
 * to print the appropriate messages.
 */
void
setuwep(obj)
register struct obj *obj;
{
	if (obj == uwep) return; /* necessary to not set unweapon */
	setworn(obj, W_WEP);
	/* Note: Explicitly wielding a pick-axe will not give a "bashing"
	 * message.  Wielding one via 'a'pplying it will.
	 * 3.2.2:  Wielding arbitrary objects will give bashing message too.
	 */
	if (obj) {
		unweapon = (obj->oclass == WEAPON_CLASS) ?
				is_launcher(obj) || is_ammo(obj) ||
				is_missile(obj) || is_pole(obj) :
			   !is_weptool(obj);
	} else
		unweapon = TRUE;	/* for "bare hands" message */
	update_inventory();
}

static int
ready_weapon(wep)
struct obj *wep;
{
	/* Separated function so swapping works easily */
	int res = 0;

	if (!wep) {
	    /* No weapon */
	    if (uwep) {
/*JP		You("are empty %s.", body_part(HANDED));*/
	        You("%sを空けた．", body_part(HAND));
		setuwep((struct obj *) 0);
		res++;
	    } else
/*JP		You("are already empty %s.", body_part(HANDED));*/
		You("何も%sにしていない！", body_part(HAND));
	} else if (!uarmg && !Stone_resistance && wep->otyp == CORPSE
				&& touch_petrifies(&mons[wep->corpsenm])) {
	    /* Prevent wielding cockatrice when not wearing gloves --KAA */
            char kbuf[BUFSZ];

/*JP        You("wield the %s corpse in your bare %s.",
                mons[wep->corpsenm].mname, makeplural(body_part(HAND)));*/
	    You("%sの死体を%sにした．",
		jtrns_mon(mons[wep->corpsenm].mname,-1), makeplural(body_part(HAND)));
/*JP        Sprintf(kbuf, "%s corpse", an(mons[wep->corpsenm].mname));*/
            Sprintf(kbuf, "%sの死体に触れて", jtrns_mon(mons[wep->corpsenm].mname,-1));
            instapetrify(kbuf);
	} else if (uarms && bimanual(wep))
/*JP	    You("cannot wield a two-handed %s while wearing a shield.",
		is_sword(wep) ? "sword" :
		    wep->otyp == BATTLE_AXE ? "axe" : "weapon");
*/
	    pline("盾を装備しているときに両手持ちの%sを装備できない．",
		is_sword(wep) ? "剣" :
		    wep->otyp == BATTLE_AXE ? "斧" : "武器");
	else if (wep->oartifact && !touch_artifact(wep, &youmonst)) {
	    res++;	/* takes a turn even though it doesn't get wielded */
	} else {
	    /* Weapon WILL be wielded after this point */
	    res++;
	    if (will_weld(wep)) {
/*JP		const char *tmp = xname(wep), *thestr = "The ";*/
		const char *tmp = xname(wep), *thestr = "";
		if (strncmp(tmp, thestr, 4) && !strncmp(The(tmp),thestr,4))
		    tmp = thestr;
		else tmp = "";
/*JP		pline("%s%s %s to your %s!", tmp, aobjnam(wep, "weld"),
			(wep->quan == 1L) ? "itself" : "themselves", / * a3 * /
			bimanual(wep) ?
				(const char *)makeplural(body_part(HAND))
				: body_part(HAND)));
*/
	        pline("%sは勝手にあなたの%sに装備された．",
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
		prinv((char *)0, wep, 0L);
		wep->owornmask = dummy;
	    }
	    setuwep(wep);

	    /* KMH -- Talking artifacts are finally implemented */
	    arti_speak(wep);

#if 0
	    /* we'll get back to this someday, but it's not balanced yet */
	    if (Race_if(PM_ELF) && !wep->oartifact &&
			    objects[wep->otyp].oc_material == IRON) {
		/* Elves are averse to wielding cold iron */
		You("have an uneasy feeling about wielding cold iron.");
		change_luck(-1);
	    }
#endif

	    if (wep->unpaid) {
		struct monst *this_shkp;

		if ((this_shkp = shop_keeper(inside_shop(u.ux, u.uy))) !=
		    (struct monst *)0) {
/*JP		    pline("%s says \"You be careful with my %s!\"",*/
		    pline("%sは述べた「%sの扱いは気をつけてくれよ！」",
			  shkname(this_shkp),
			  xname(wep));
		}
	    }
	}
	return(res);
	/* Take no time if we are dextrous enough */
/*	return ((rnd(20) > ACURR(A_DEX)) ? res : 0);*/
}

void
setuqwep(obj)
register struct obj *obj;
{
	setworn(obj, W_QUIVER);
	update_inventory();
}

void
setuswapwep(obj)
register struct obj *obj;
{
	setworn(obj, W_SWAPWEP);
	update_inventory();
}


/*** Commands to change particular slot(s) ***/

static NEARDATA const char wield_objs[] =
	{ ALL_CLASSES, ALLOW_NONE, WEAPON_CLASS, TOOL_CLASS, 0 };

int
dowield()
{
	register struct obj *wep;
	int result;

	/* May we attempt this? */
	multi = 0;
	if (cantwield(youmonst.data)) {
/*JP		pline("Don't be ridiculous!");*/
		pline("ばかばかしい！");
		return(0);
	}

	/* Prompt for a new weapon */
/*JP	if (!(wep = getobj(wield_objs, "wield")))*/
	if (!(wep = getobj(wield_objs, body_part(HANDED))))
		/* Cancelled */
		return (0);
	else if (wep == uwep) {
/*JP	    You("are already wielding that!");*/
	    You("もうそれを%sにしている！", body_part(HAND));
	    if (is_weptool(wep)) unweapon = FALSE;	/* [see setuwep()] */
		return (0);
	} else if (welded(uwep)) {
		weldmsg(uwep);
		return (0);
	}

	/* Handle no object, or object in other slot */
	if (wep == &zeroobj)
		wep = (struct obj *) 0;
	else if (wep == uswapwep)
		return (doswapweapon());
	else if (wep == uquiver)
		setuqwep((struct obj *) 0);
	else if (wep->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL
#ifdef STEED
			| W_SADDLE
#endif
			)) {
/*JP		You("cannot wield that!");*/
		You("それを装備できない！");
		return (0);
	}

	/* Set your new primary weapon */
	if (flags.pushweapon && uwep)
		setuswapwep(uwep);
	result = ready_weapon(wep);
	untwoweapon();

	return (result);
}

int
doswapweapon()
{
	register struct obj *oldwep, *oldswap;
	int result = 0;


	/* May we attempt this? */
	multi = 0;
	if (cantwield(youmonst.data)) {
/*JP		pline("Don't be ridiculous!");*/
		pline("ばかばかしい！");
		return(0);
	}
	if (welded(uwep)) {
		weldmsg(uwep);
		return (0);
	}

	/* Unwield your current secondary weapon */
	oldwep = uwep;
	oldswap = uswapwep;
	setuswapwep((struct obj *) 0);

	/* Set your new primary weapon */
	result = ready_weapon(oldswap);

	/* Set your new secondary weapon */
	if (uwep == oldwep)
		/* Wield failed for some reason */
		setuswapwep(oldswap);
	else {
		setuswapwep(oldwep);
		if (uswapwep)
			prinv((char *)0, uswapwep, 0L);
		else
/*JP			You("have no secondary weapon readied.");*/
			You("左手を空けた．");
	}

	if (u.twoweap && !can_twoweapon())
		untwoweapon();

	return (result);
}

int
dowieldquiver()
{
	register struct obj *newquiver;


	/* Since the quiver isn't in your hands, don't check cantwield(), */
	/* will_weld(), touch_petrifies(), etc. */
	multi = 0;

	/* Because 'Q' used to be quit... */
	if (!flags.suppress_alert || flags.suppress_alert < FEATURE_NOTICE_VER(3,3,0))
/*JP		pline("Note: Please use #quit if you wish to exit the game.");*/
		pline("注意:ゲームを終了するときは #quitコマンドを使うこと．");

	/* Prompt for a new quiver */
/*JP	if (!(newquiver = getobj(wield_objs, "ready")))*/
	if (!(newquiver = getobj(wield_objs, "装填する")))
		/* Cancelled */
		return (0);

	/* Handle no object, or object in other slot */
	/* Any type is okay, since we give no intrinsics anyways */
	if (newquiver == &zeroobj) {
		/* Explicitly nothing */
		if (uquiver) {
/*JP			pline("You now have no ammunition readied.");*/
			pline("装填するための矢(など)がなくなった．");
			setuqwep(newquiver = (struct obj *) 0);
		} else {
/*JP			pline("You already have no ammunition readied!");*/
			pline("装填するための矢(など)がない．");
			return(0);
		}
	} else if (newquiver == uquiver) {
/*JP		pline("That ammunition is already readied!");*/
		pline("もう装填されている！");
		return(0);
	} else if (newquiver == uwep) {
		/* Prevent accidentally readying the main weapon */
/*JP		pline("That is already being used as a weapon!");*/
		pline("もう武器として使われている！");
		return(0);
	} else if (newquiver->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL
#ifdef STEED
			| W_SADDLE
#endif
			)) {
/*JP		You("cannot ready that!");*/
		You("それは使えない！");
		return (0);
	} else {
		long dummy;


		/* Check if it's the secondary weapon */
		if (newquiver == uswapwep) {
			setuswapwep((struct obj *) 0);
			untwoweapon();
		}

		/* Okay to put in quiver; print it */
		dummy = newquiver->owornmask;
		newquiver->owornmask |= W_QUIVER;
		prinv((char *)0, newquiver, 0L);
		newquiver->owornmask = dummy;
	}

	/* Finally, place it in the quiver */
	setuqwep(newquiver);
	/* Take no time if we are dextrous enough */
	return (rnd(20) > ACURR(A_DEX));
}

int
can_twoweapon()
{
#define NOT_WEAPON(obj) (!is_weptool(obj) && obj->oclass != WEAPON_CLASS)
	if (Upolyd)
/*JP		You("can only use two weapons in your normal form.");*/
		You("両手持ちは通常の姿でのみ使用できる．");
	else if (!uwep || !uswapwep)
/*JP		Your("%s%s%s empty.", uwep ? "left " : uswapwep ? "right " : "",
			body_part(HAND), (!uwep && !uswapwep) ? "s are" : " is");
*/
		Your("%s%sは空っぽだ．", uwep ? "左の" : uswapwep ? "右の" : "",
		     body_part(HAND));
	else if (NOT_WEAPON(uwep) || bimanual(uwep))
/*JP		pline("%s isn't %s%s%s.", Yname2(uwep),
			NOT_WEAPON(uwep) ? "a ": "",
			bimanual(uwep) ? "one-handed": "",
			NOT_WEAPON(uwep) ? "weapon": "");
*/
		pline("%sは%s%sじゃない．", Yname2(uwep),
			bimanual(uwep) ? "片手持ちの": "",
			NOT_WEAPON(uwep) ? "武器": "");
	else if (NOT_WEAPON(uswapwep) || bimanual(uswapwep))
/*JP		pline("%s isn't %s%s%s.", Yname2(uswapwep),
			NOT_WEAPON(uswapwep) ? "a ": "",
			bimanual(uswapwep) ? "one-handed": "",
			NOT_WEAPON(uswapwep) ? "weapon": "");
*/
		pline("%sは%s%sじゃない．", Yname2(uswapwep),
			bimanual(uswapwep) ? "片手持ちの": "",
			NOT_WEAPON(uswapwep) ? "武器": "");
	else if (uarms)
/*JP		You("can't use two weapons while wearing a shield.");*/
		You("盾を持っている間は両手持ちできない．");
	else if (uswapwep->oartifact)
/*JP		pline("%s resists being held second to another weapon!",
			Yname2(uswapwep));
*/
		pline("%sは左手で持つことを拒んだ！",
			Yname2(uswapwep));
	else if (!uarmg && !Stone_resistance && (uswapwep->otyp == CORPSE &&                   
                  (touch_petrifies(&mons[uswapwep->corpsenm])))) {
		char kbuf[BUFSZ];

/*JP		You("wield the %s corpse with your bare %s.",
		    mons[uswapwep->corpsenm].mname, body_part(HAND));*/
		You("%sを素%sで掴んだ．",
		    jtrns_mon(mons[uswapwep->corpsenm].mname,-1), body_part(HAND));
/*JP		Sprintf(kbuf, "%s corpse", an(mons[uswapwep->corpsenm].mname));*/
		Sprintf(kbuf, "%sの死体に触れて", jtrns_mon(mons[uswapwep->corpsenm].mname,-1));            
		instapetrify(kbuf);
	} else if (Glib || uswapwep->cursed) {
		struct obj *obj = uswapwep;

/*JP		Your("%s from your %s!",  aobjnam(obj, "slip"),
				makeplural(body_part(HAND)));
*/
		You("%sを落してしまった！", xname(obj));
		if (!Glib)
			obj->bknown = TRUE;
		setuswapwep((struct obj *) 0);
		dropx(obj);
	} else
		return (TRUE);
	return (FALSE);
}

int
dotwoweapon()
{
	/* You can always toggle it off */
	if (u.twoweap) {
/*JP		You("switch to your primary weapon.");*/
		You("片手で戦闘することにした．");
		u.twoweap = 0;
		return (0);
	}

	/* May we use two weapons? */
	if (can_twoweapon()) {
		/* Success! */
/*JP		You("begin two-weapon combat.");*/
		You("両手で戦闘することにした．");
		u.twoweap = 1;
		return (rnd(20) > ACURR(A_DEX));
	}
	return (0);
}

/*** Functions to empty a given slot ***/
/* These should be used only when the item can't be put back in
 * the slot by life saving.  Proper usage includes:
 * 1.  The item has been eaten, stolen, burned away, or rotted away.
 * 2.  Making an item disappear for a bones pile.
 */
void
uwepgone()
{
	if (uwep) {
		setworn((struct obj *)0, W_WEP);
		unweapon = TRUE;
		update_inventory();
	}
}

void
uswapwepgone()
{
	if (uswapwep) {
		setworn((struct obj *)0, W_SWAPWEP);
		update_inventory();
	}
}

void
uqwepgone()
{
	if (uquiver) {
		setworn((struct obj *)0, W_QUIVER);
		update_inventory();
	}
}

void
untwoweapon()
{
	if (u.twoweap) {
/*JP		You("can no longer use two weapons at once.");*/
		You("もう２つの武器を同時に使用することはできない．");
		u.twoweap = FALSE;
	}
	return;
}

/* Maybe rust weapon, or corrode it if acid damage is called for */
void
erode_weapon(victim, acid_dmg)
struct monst *victim;
boolean acid_dmg;
{
	int erosion;
	struct obj *target;
	boolean vismon = (victim != &youmonst) && canseemon(victim);

	target = (victim == &youmonst) ? uwep : MON_WEP(victim);
	if (!target)
	    return;

	erosion = acid_dmg ? target->oeroded2 : target->oeroded;

	if (target->greased) {
	    grease_protect(target,(char *)0,FALSE,victim);
	} else if (target->oerodeproof ||
		(acid_dmg ? !is_corrodeable(target) : !is_rustprone(target))) {
	    if (flags.verbose || !(target->oerodeproof && target->rknown)) {
		if (victim == &youmonst)
/*JP		    Your("%s not affected.", aobjnam(target, "are"));*/
		    Your("%sは影響を受けない．", xname(uwep));
		else if (vismon)
/*JP		    pline("%s's %s not affected.", Monnam(victim),
			aobjnam(target, "are"));
*/
		    pline("%sの%sは影響を受けない．", Monnam(victim), xname(uwep));
	    }
	    if (target->oerodeproof) target->rknown = TRUE;
	} else if (erosion < MAX_ERODE) {
	    if (victim == &youmonst)
/*JP		Your("%s%s!", aobjnam(target, acid_dmg ? "corrode" : "rust"),
		    erosion+1 == MAX_ERODE ? " completely" :
		    erosion ? " further" : "");
*/
		Your("%sは%s%s！", xname(uwep),
		     uwep->oeroded+1 == MAX_ERODE ? "完全に" :
		     uwep->oeroded ? "さらに" : "",
		     acid_dmg ? "腐食した" : "錆びた");
	    else if (vismon)
/*JP		pline("%s's %s%s!", Monnam(victim),
		    aobjnam(target, acid_dmg ? "corrode" : "rust"),
		    erosion+1 == MAX_ERODE ? " completely" :
		    erosion ? " further" : "");*/
		pline("%sの%sは%s%s！", Monnam(victim),
		      xname(target),
		      erosion+1 == MAX_ERODE ? "完全に" :
		      erosion ? "さらに" : "",
		      acid_dmg ? "腐食した" : "錆びた");
	    if (acid_dmg)
		target->oeroded2++;
	    else
		target->oeroded++;
	} else {
	    if (flags.verbose) {
		if (victim == &youmonst)
/*JP		    Your("%s completely %s.",
			aobjnam(target, Blind ? "feel" : "look"),
			acid_dmg ? "corroded" : "rusty");
*/
		    Your("%sは完全に%s%s．",
			 xname(uwep),
			 acid_dmg ? "腐食した" : "錆びた",
			 Blind ? "ようだ" : "ように見える");
		else if (vismon)
/*JP		    pline("%s's %s completely %s.", Monnam(victim),
			aobjnam(target, "look"),
			acid_dmg ? "corroded" : "rusty");
*/
		    pline("%sの%sは完全に%s%s．", Monnam(victim),
			  xname(uwep),
			  acid_dmg ? "腐食した" : "錆びた",
			  Blind ? "ようだ" : "ように見える");
	    }
	}
}

int
chwepon(otmp, amount)
register struct obj *otmp;
register int amount;
{
	register const char *color = hcolor((amount < 0) ? Black : blue);
	register const char *xtime;

	if(!uwep || (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep))) {
		char buf[BUFSZ];

/*JP		Sprintf(buf, "Your %s %s.", makeplural(body_part(HAND)),
			(amount >= 0) ? "twitch" : "itch");*/
		Sprintf(buf, "あなたの%sは%s．", makeplural(body_part(HAND)),
			(amount >= 0) ? "ひきつった" : "ムズムズした");
		strange_feeling(otmp, buf);
		exercise(A_DEX, (boolean) (amount >= 0));
		return(0);
	}

	if(uwep->otyp == WORM_TOOTH && amount >= 0) {
		uwep->otyp = CRYSKNIFE;
		uwep->oerodeproof = 0;
/*JP		Your("weapon seems sharper now.");*/
		Your("武器はより鋭さを増したようだ．");
		uwep->cursed = 0;
		return(1);
	}

	if(uwep->otyp == CRYSKNIFE && amount < 0) {
		uwep->otyp = WORM_TOOTH;
		uwep->oerodeproof = 0;
/*JP		Your("weapon seems duller now.");*/
		Your("武器はより鈍くなったようだ．");
		return(1);
	}

	if (amount < 0 && uwep->oartifact && restrict_name(uwep, ONAME(uwep))) {
	    if (!Blind)
/*JP		Your("%s %s.", aobjnam(uwep, "faintly glow"), color);*/
		Your("%sはわずかに%s輝いた．", xname(uwep),jconj_adj(color));
	    return(1);
	}
	/* there is a (soft) upper and lower limit to uwep->spe */
	if(((uwep->spe > 5 && amount >= 0) || (uwep->spe < -5 && amount < 0))
								&& rn2(3)) {
	    if (!Blind)
/*JP	    Your("%s %s for a while and then evaporate%s.",
		 aobjnam(uwep, "violently glow"), color,
		 uwep->quan == 1L ? "s" : "");
*/
	    Your("%sはしばらく激しく%s輝き，蒸発した．",
		 xname(uwep), jconj_adj(color));
	    else
/*JP		Your("%s.", aobjnam(uwep, "evaporate"));*/
		Your("%sは蒸発した．", xname(uwep));

	    while(uwep)		/* let all of them disappear */
				/* note: uwep->quan = 1 is nogood if unpaid */
		useup(uwep);
	    return(1);
	}
	if (!Blind) {
/*JP	    xtime = (amount*amount == 1) ? "moment" : "while";*/
	    xtime = (amount*amount == 1) ? "一瞬" : "しばらくの間";
/*JP	    Your("%s %s for a %s.",
		 aobjnam(uwep, amount == 0 ? "violently glow" : "glow"),
		 color, xtime);*/
	    Your("%sは%s%s%s輝いた．",
		 xname(uwep), xtime, jconj_adj(color), 
		 amount == 0 ? "激しく" : "");
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
		Your("右%sは%s！",
			body_part(HAND),
			(((amount > 1) && (uwep->spe > 1)) ? "ひりひりした" : "ムズムズした"));
	}

	/* an elven magic clue, cookie@keebler */
	if ((uwep->spe > 5)
		&& (is_elven_weapon(uwep) || uwep->oartifact || !rn2(7)))
/*JP	    Your("%s unexpectedly.",
		aobjnam(uwep, "suddenly vibrate"));*/
	    Your("%sは突然震えだした．",
		xname(uwep));

	return(1);
}

int
welded(obj)
register struct obj *obj;
{
	if (obj && obj == uwep && will_weld(obj)) {
		obj->bknown = TRUE;
		return 1;
	}
	return 0;
}

void
weldmsg(obj)
register struct obj *obj;
{
	long savewornmask;

	savewornmask = obj->owornmask;
/*JP
	Your("%s %s welded to your %s!",
		xname(obj), (obj->quan == 1L) ? "is" : "are",
		bimanual(obj) ? (const char *)makeplural(body_part(HAND))
				: body_part(HAND));
*/
	You("%sを%sに構えた！", 
		xname(obj), body_part(HAND));
	obj->owornmask = savewornmask;
}

/*wield.c*/
