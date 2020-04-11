/*	SCCS Id: @(#)engrave.c	3.1	92/06/16	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "lev.h"
#include <ctype.h>

STATIC_VAR NEARDATA struct engr *head_engr;

STATIC_DCL void FDECL(del_engr, (struct engr *));

#ifdef OVLB
/* random engravings */
const char *random_mesg[] = {
	"Elbereth", "ad ae?ar um",
	"?la? ?as he??",
	/* take-offs and other famous engravings */
	"Owlbreath", "?ala??iel",
	"?ilroy wa? h?re",
	"A.S. ->", "<- A.S.", /* Journey to the Center of the Earth */
	"Y?u won t get i? up ?he ste?s", /* Adventure */
	"Lasc?ate o?ni sp?ranz? o vo? c?'en?rate", /* Inferno */
	"Well Come", /* Prisoner */
	"W? ap?l???ze for t?e inc?nve??e?ce", /* So Long... */
	"S?e you n?xt Wed?esd?y", /* Thriller */
	"Fo? a ?ood time c?ll 8?7-53?9",
};

const char *
random_engraving()
{
	char *rumor, *s;
/*JP*/
	static const char *jq = "？";

/* a random engraving may come from the "rumors" file, or from the
   list above */
	rumor = getrumor(0);
	if (rn2(4) && *rumor) {
		for (s = rumor; *s; s++){
/*JP*/
		        if(!rn2(7)){
			  if(is_kanji1(rumor,s-rumor)){
			    *s = jq[0];
			    ++s;
			    *s = jq[1];
			  }
			  else if(is_kanji2(rumor,s-rumor)){
			    --s;
			    *s = jq[0];
			    ++s;
			    *s = jq[1];
			  }
			  else
			    if (*s != ' ') *s = '?';
			}
		}
		if (s[-1] == '.') s[-1] = 0;
		return (const char *)rumor;
	}
	else
		return random_mesg[rn2(SIZE(random_mesg))];
}
#endif /* OVLB */
#ifdef OVL0

const char *
surface(x, y)
register int x, y;
{
	register struct rm *lev = &levl[x][y];

	if (IS_AIR(lev->typ))
/*JP	    return "air";*/
	    return "空中";
	else if (is_pool(x,y))
/*JP	    return "water";*/
	    return "水中";
	else if (is_ice(x,y))
/*JP	    return "ice";*/
	    return "氷";
	else if (is_lava(x,y))
/*JP	    return "lava";*/
	    return "溶岩";
	else if (lev->typ == DRAWBRIDGE_DOWN)
/*JP	    return "bridge";*/
	    return "橋";
	else if ((IS_ROOM(lev->typ) && !Is_earthlevel(&u.uz)) ||
		 IS_WALL(lev->typ) || IS_DOOR(lev->typ) || lev->typ == SDOOR)
/*JP	    return "floor";*/
	    return "床";
	else
/*JP	    return "ground";*/
	    return "地面";
}

struct engr *
engr_at(x,y) register xchar x,y; {
register struct engr *ep = head_engr;
	while(ep) {
		if(x == ep->engr_x && y == ep->engr_y)
			return(ep);
		ep = ep->nxt_engr;
	}
	return((struct engr *) 0);
}

#ifdef ELBERETH
int
sengr_at(s,x,y)
	register const char *s;
	register xchar x,y;
{
	register struct engr *ep = engr_at(x,y);
	register char *t;
	register int n;

	if(ep && ep->engr_time <= moves) {
		t = ep->engr_txt;
/*
		if(!strcmp(s,t)) return(1);
*/
		n = strlen(s);
		while(*t) {
			if(!strncmp(s,t,n)) return(1);
			t++;
		}
	}
	return(0);
}
#endif

#endif /* OVL0 */
#ifdef OVL2

void
u_wipe_engr(cnt)
register int cnt;
{
	if(!u.uswallow && !Levitation)
		wipe_engr_at(u.ux, u.uy, cnt);
}

#endif /* OVL2 */
#ifdef OVL1

void
wipe_engr_at(x,y,cnt) register xchar x,y,cnt; {
register struct engr *ep = engr_at(x,y);
register int lth,pos;
char ch;
/*JP*/
	static const char *jq = "？";

	if(ep){
	    if(ep->engr_type != BURN) {
		if(ep->engr_type != DUST && ep->engr_type != BLOOD) {
			cnt = rn2(1 + 50/(cnt+1)) ? 0 : 1;
		}
		lth = strlen(ep->engr_txt);
		if(lth && cnt > 0 ) {
			while(cnt--) {
				pos = rn2(lth);
				if((ch = ep->engr_txt[pos]) == ' ')
					continue;
/*JP*/
				if(is_kanji2(ep->engr_txt,pos))
				  --pos;
				if(is_kanji1(ep->engr_txt,pos)){
				  if(ep->engr_txt[pos] == jq[0] &&
				     ep->engr_txt[pos+1] == jq[1]){
				    ep->engr_txt[pos] = ' ';
				    ep->engr_txt[pos+1] = ' ';
				  }
				  else{
				    ep->engr_txt[pos] = jq[0];
				    ep->engr_txt[pos+1] = jq[1];
				  }
				}
				else
/*JP*/
				ep->engr_txt[pos] = (ch != '?') ? '?' : ' ';
			}
		}
		while(lth && ep->engr_txt[lth-1] == ' ')
			ep->engr_txt[--lth] = 0;
		while(ep->engr_txt[0] == ' ')
			ep->engr_txt++;
		if(!ep->engr_txt[0]) del_engr(ep);
	    }
	}
}

#endif /* OVL1 */
#ifdef OVL2

void
read_engr_at(x,y)
register int x,y;
{
	register struct engr *ep = engr_at(x,y);
	register int	sensed = 0;

	if(ep && ep->engr_txt[0]) {
	    switch(ep->engr_type) {
	    case DUST:
		if(!Blind) {
			sensed = 1;
/*JP			pline("Something is written here in the %s.",
				is_ice(x,y) ? "frost" : "dust");*/
			pline("%sに何か書いてある．",
				is_ice(x,y) ? "氷" : "ほこり");
		}
		break;
	    case ENGRAVE:
		if(!Blind || !Levitation) {
			sensed = 1;
/*JP			pline("Something is engraved here on the %s.",*/
			pline("%sに何か文字が刻まれている．",
				surface(x,y));
		}
		break;
	    case BURN:
		if(!Blind || !Levitation) {
			sensed = 1;
/*JP			pline("Some text has been %s into the %s here.",*/
			pline("何かの文字が%sに%sいる．",
/*JP				is_ice(x,y) ? "melted" : "burned",*/
				surface(x,y),
				is_ice(x,y) ? "刻まれて" : "燃えて");
		}
		break;
	    case MARK:
		if(!Blind) {
			sensed = 1;
/*JP			pline("There's some graffiti on the %s here.",*/
			pline("何かの落書が%sにある．",
				surface(x,y));
		}
		break;
	    case BLOOD:
		/* "It's a message!  Scrawled in blood!"
		 * "What's it say?"
		 * "It says... `See you next Wednesday.'" -- Thriller
		 */
		if(!Blind) {
			sensed = 1;
/*JP			You("see a message scrawled in blood here.");*/
			You("血文字がなぐり書きされているのを見つけた．");
		}
		break;
	    default:
		impossible("Something is written in a very strange way.");
		sensed = 1;
	    }
	    if (sensed) {
/*JP		You("%s: \"%s\".",
		      (Blind) ? "feel the words" : "read",  ep->engr_txt);*/
		You("%s:「%s」",
		      (Blind) ? "次のように感じた" : "読んだ",  ep->engr_txt);
		if(flags.run > 1) nomul(0);
	    }
	}
}

#endif /* OVL2 */
#ifdef OVLB

void
make_engr_at(x,y,s,e_time,e_type)
register int x,y;
register const char *s;
register long e_time;
register xchar e_type;
{
	register struct engr *ep;

	if ((ep = engr_at(x,y)) != 0)
	    del_engr(ep);
	ep = newengr(strlen(s) + 1);
	ep->nxt_engr = head_engr;
	head_engr = ep;
	ep->engr_x = x;
	ep->engr_y = y;
	ep->engr_txt = (char *)(ep + 1);
	Strcpy(ep->engr_txt, s);
	if(strcmp(s, "Elbereth")) exercise(A_WIS, TRUE);
	ep->engr_time = e_time;
	ep->engr_type = e_type > 0 ? e_type : rnd(N_ENGRAVE);
	ep->engr_lth = strlen(s) + 1;
}

/* delete any engraving at location <x,y> */
void
del_engr_at(x, y)
int x, y;
{
	register struct engr *ep = engr_at(x, y);

	if (ep) del_engr(ep);
}

/*
 *	freehand - returns true if player has a free hand
 */
int
freehand()
{
	return(!uwep || !welded(uwep) ||
	   (!bimanual(uwep) && (!uarms || !uarms->cursed)));
/*	if ((uwep && bimanual(uwep)) ||
	    (uwep && uarms))
		return(0);
	else
		return(1);*/
}

static NEARDATA const char styluses[] =
	{ ALL_CLASSES, ALLOW_NONE, TOOL_CLASS, WEAPON_CLASS, WAND_CLASS,
	  GEM_CLASS, RING_CLASS, 0 };

/* Mohs' Hardness Scale:
 *  1 - Talc		 6 - Orthoclase
 *  2 - Gypsum		 7 - Quartz
 *  3 - Calcite		 8 - Topaz
 *  4 - Fluorite	 9 - Corundum
 *  5 - Apatite		10 - Diamond
 *
 * Since granite is a igneous rock hardness ~ 7, anything >= 8 should
 * probably be able to scratch the rock.
 * Devaluation of less hard gems is not easily possible because obj struct
 * does not contain individual oc_cost currently. 7/91
 *
 * dilithium  - ??			* jade	    -  5-6	(nephrite)
 * diamond    - 10			* turquoise -  5-6
 * ruby	      -  9	(corundum)	* opal	    -  5-6
 * sapphire   -  9	(corundum)	* iron	    -  4-5
 * topaz      -  8			* fluorite  -  4
 * emerald    -  7.5-8	(beryl)		* brass     -  3-4
 * aquamarine -  7.5-8	(beryl)		* gold	    -  2.5-3
 * garnet     -  7.25	(var. 6.5-8)	* silver    -  2.5-3
 * agate      -  7	(quartz)	* copper    -  2.5-3
 * amethyst   -  7	(quartz)	* amber     -  2-2.5
 * jasper     -  7	(quartz)	*	
 * onyx	      -  7 	(quartz)	* steel     -  5-8.5	(usu. weapon)
 * moonstone  -  6	(orthoclase)	*
 */

static NEARDATA const short hard_gems[] =
	{ DIAMOND, RUBY, SAPPHIRE, TOPAZ, EMERALD, AQUAMARINE, GARNET, 0 };

static NEARDATA const char *hard_ring_names[] =
/*JP	{"diamond", "ruby", "sapphire", "emerald", "topaz", ""};*/
	{"ダイヤモンド", "ルビー", "サファイア", "エメラルド", "トパーズ", ""};

/* return 1 if action took 1 (or more) moves, 0 if error or aborted */
int
doengrave()
{
	boolean dengr = FALSE;	/* TRUE if we wipe out the current engraving */
	boolean doblind = FALSE;/* TRUE if engraving blinds the player */
	boolean doknown = FALSE;/* TRUE if we identify the stylus */
	boolean eow = FALSE;	/* TRUE if we are overwriting oep */
	boolean jello = FALSE;	/* TRUE if we are engraving in slime */
	boolean ptext = TRUE;	/* TRUE if we must prompt for engrave text */
	boolean teleengr =FALSE;/* TRUE if we move the old engraving */
	boolean zapwand = FALSE;/* TRUE if we remove a wand charge */
	xchar type = DUST;	/* Type of engraving made */
	char buf[BUFSZ];	/* Buffer for final/poly engraving text */
	char ebuf[BUFSZ];	/* Buffer for initial engraving text */
	char qbuf[QBUFSZ];	/* Buffer for query text */
	char post_engr_text[BUFSZ]; /* Text displayed after engraving prompt */
	const char *everb;	/* Present tense of engraving type */
	const char *eloc;	/* Where the engraving is (ie dust/floor/...) */
	register char *sp;	/* Place holder for space count of engr text */
	register int len;	/* # of nonspace chars of new engraving text */
	register int maxelen;	/* Max allowable length of new engraving text */
	register int spct;	/* # of spaces in new engraving text */
	register struct engr *oep = engr_at(u.ux,u.uy);
				/* The current engraving */
	register struct obj *otmp; /* Object selected with which to engrave */


	multi = 0;		/* moves consumed */
	nomovemsg = (char *)0;	/* occupation end message */

	buf[0] = (char)0;
	ebuf[0] = (char)0;
	post_engr_text[0] = (char)0;
	maxelen = BUFSZ - 1;

	/* Can the adventurer engrave at all? */

	if(u.uswallow) {
		if (is_animal(u.ustuck->data)) {
/*JP			pline("What would you write?  \"Jonah was here\"?");*/
			pline("何て書きたいの？「ヨナはここにいる」？");
			return(0);
		} else if (is_whirly(u.ustuck->data)) {
/*JP			You("can't reach the %s.", surface(u.ux,u.uy));*/
			You("%sに届かない．", surface(u.ux,u.uy));
			return(0);
		} else 
			jello = TRUE;
	} else if (is_lava(u.ux, u.uy)) {
/*JP		You("can't write on the lava!");*/
		You("溶岩には書けない！");
		return(0);
	} else if (is_pool(u.ux,u.uy) || IS_FOUNTAIN(levl[u.ux][u.uy].typ)) {
/*JP		You("can't write on the water!");*/
		You("水には書けない！");
		return(0);
	}
	if(Is_airlevel(&u.uz) || Is_waterlevel(&u.uz)/* in bubble */) {
/*JP		You("can't write in thin air!");*/
		You("空中には書けない！");
		return(0);
	}
#ifdef POLYSELF
	if (cantwield(uasmon)) {
/*JP		You("can't even hold anything!");*/
		You("何も持てない！");
		return(0);
	}
#endif
	if (check_capacity(NULL)) return (0);

	/* One may write with finger, or weapon, or wand, or..., or...
	 * Edited by GAN 10/20/86 so as not to change weapon wielded.
	 */

/*JP	otmp = getobj(styluses, "write with");*/
	otmp = getobj(styluses, "使う");
	if(!otmp) return(0);		/* otmp == zeroobj if fingers */

	/* There's no reason you should be able to write with a wand
	 * while both your hands are tied up.
	 */
	if (!freehand() && otmp != uwep && !otmp->owornmask) {
/*JP		You("have no free %s to write with!", body_part(HAND));*/
		pline("%sがふさがってて書けない！", body_part(HAND));
		return(0);
	}

	if (jello) {
/*JP		You("tickle %s with your %s.", mon_nam(u.ustuck), 
		    (otmp == &zeroobj) ? makeplural(body_part(FINGER)) :
			xname(otmp));*/
		You("%sで%sをくすぐった．", 
		    (otmp == &zeroobj) ? makeplural(body_part(FINGER)) :
			xname(otmp), mon_nam(u.ustuck));
/*JP		Your("message dissolves...");*/
		Your("メッセージは消えた．．．");
		return(0);
	}
	if(Levitation && otmp->oclass != WAND_CLASS){		/* riv05!a3 */
/*JP		You("can't reach the %s!", surface(u.ux,u.uy));*/
		You("%sに届かない！", surface(u.ux,u.uy));
		return(0);
	}

	/* SPFX for items */

	switch (otmp->oclass) {
	    default:
	    case AMULET_CLASS:
	    case CHAIN_CLASS:
	    case POTION_CLASS:
	    case GOLD_CLASS:
		break;

	    case RING_CLASS:
		/* "diamond" rings and others should work */
		{
		    register int i, j;

		    for (i=0, j=strlen(hard_ring_names[i]); j; i++)
			if ( !strncmp(hard_ring_names[i],
			     OBJ_DESCR(objects[otmp->otyp]),
			     j=strlen(hard_ring_names[i])) ) {
			    type = ENGRAVE;
			    break;
			}
		}
		break;

	    case GEM_CLASS:
		/* diamonds & other gems should work */
		{
		    register int i;

		    for (i=0; hard_gems[i]; i++)
			if (otmp->otyp == hard_gems[i]) {
			    type = ENGRAVE;
			    break;
			}
		}
		break;

	    /* Objects too large to engrave with */
	    case BALL_CLASS:
	    case ROCK_CLASS:
	    case ARMOR_CLASS:
/*JP		You("can't engrave with such a large object!");*/
		pline("そんな大きなものを使って文字を刻めない！");
		ptext = FALSE;
		break;

	    /* Objects too silly to engrave with */
	    case FOOD_CLASS:
	    case SCROLL_CLASS:
	    case SPBOOK_CLASS:
/*JP		Your("%s would get %s.", xname(otmp),
			is_ice(u.ux,u.uy) ? "all frosty" : "too dirty");*/
		Your("%sは%sなった．", xname(otmp),
			is_ice(u.ux,u.uy) ? "氷づけに" : "汚なく");
		ptext = FALSE;
		break;

	    case RANDOM_CLASS:	/* This should mean fingers */
		break;

	    /* The charge is removed from the wand before prompting for
	     * the engraving text, because all kinds of setup decisions
	     * and pre-engraving messages are based upon knowing what type
	     * of engraving the wand is going to do.  Also, the player
	     * will have potentially seen "You wrest .." message, and
	     * therefore will know they are using a charge.
	     */
	    case WAND_CLASS:
		if (zappable(otmp)) {
		    check_unpaid(otmp);
		    zapwand = TRUE;
		    if (Levitation) ptext = FALSE;

		    switch (otmp->otyp) {
		    /* DUST wands */
		    default:
			break;

			/* NODIR wands */
		    case WAN_LIGHT:
		    case WAN_SECRET_DOOR_DETECTION:
		    case WAN_CREATE_MONSTER:
		    case WAN_WISHING:
	  		zapnodir(otmp);
			break;

			/* IMMEDIATE wands */
	    		/* If wand is "IMMEDIATE", remember to affect the
			 * previous engraving even if turning to dust.
			 */
		    case WAN_STRIKING:
			Strcpy(post_engr_text,
/*JP			"The wand unsuccessfully fights your attempt to write!"*/
			"あなたが書こうとすると杖は抵抗した！"
			);
			break;
		    case WAN_SLOW_MONSTER:
			if (!Blind) {
			   Sprintf(post_engr_text,
/*JP				   "The bugs on the %s slow down!",*/
				   "%sの上の虫の動きが遅くなった！",
				   surface(u.ux, u.uy));
			}
			break;
		    case WAN_SPEED_MONSTER:
			if (!Blind) {
			   Sprintf(post_engr_text,
/*JP				   "The bugs on the %s speed up!",*/
				   "%sの上の虫の動きが速くなった！",
				   surface(u.ux, u.uy));
			}
			break;
		    case WAN_POLYMORPH:
			if(oep)  {
			    if (!Blind) {
				type = (xchar)0;	/* random */
				Strcpy(buf,random_engraving());
			    }
			    dengr = TRUE;
			}
			break;
		    case WAN_NOTHING:
		    case WAN_UNDEAD_TURNING:
		    case WAN_OPENING:
		    case WAN_LOCKING:
		    case WAN_PROBING:
			break;

			/* RAY wands */
		    case WAN_MAGIC_MISSILE:
			ptext = TRUE;
			if (!Blind) {
			   Sprintf(post_engr_text,
/*JP				   "The %s is riddled by bullet holes!",*/
				   "%sは散弾で細い穴だらけになった！",
				   surface(u.ux, u.uy));
			}
			break;

		    /* can't tell sleep from death - Eric Backus */
		    case WAN_SLEEP:
		    case WAN_DEATH:
			if (!Blind) {
			   Sprintf(post_engr_text,
/*JP				   "The bugs on the %s stop moving!",*/
				   "%sの上の虫の動きが止った！",
				   surface(u.ux, u.uy));
			}
			break;

		    case WAN_COLD:
			if (!Blind)
			    Strcpy(post_engr_text,
/*JP				"A few ice cubes drop from the wand.");*/
				"氷のかけらが杖からこぼれ落ちた．");
			if(!oep || (oep->engr_type != BURN))
			    break;
		    case WAN_CANCELLATION:
		    case WAN_MAKE_INVISIBLE:
			if(oep) {
			    if (!Blind)
/*JP				pline("The engraving on the %s vanishes!",*/
				pline("%sの上の文字は消えた！",
					surface(u.ux,u.uy));
			    dengr = TRUE;
			}
			break;
		    case WAN_TELEPORTATION:
			if (oep) {
			    if (!Blind)
/*JP				pline("The engraving on the %s vanishes!",*/
				pline("%sの上の文字は消えた！",
					surface(u.ux,u.uy));
			    teleengr = TRUE;
			}
			break;

		    /* type = ENGRAVE wands */
		    case WAN_DIGGING:
			ptext = TRUE;
			type  = ENGRAVE;
			if(!objects[otmp->otyp].oc_name_known) {
	    		    if (flags.verbose)
/*JP				pline("This %s is a wand of digging!",*/
				pline("この%sは穴掘りの杖だ！",
				xname(otmp));
			    doknown = TRUE;
			}
			if (!Blind)
			    Strcpy(post_engr_text,
				is_ice(u.ux,u.uy) ? 
/*JP				"Ice chips fly up from the ice surface!" :
			        "Gravel flies up from the floor.");*/
				"氷の表面から氷のかけらが飛び散った．" :
			        "砂利が床から飛び散った．");
			else
/*JP			    Strcpy(post_engr_text, "You hear drilling!");*/
			    Strcpy(post_engr_text, "穴が開く音を聞いた！");
			break;

		    /* type = BURN wands */
		    case WAN_FIRE:
			ptext = TRUE;
			type  = BURN;
			if(!objects[otmp->otyp].oc_name_known) {
	    		if (flags.verbose)
/*JP			    pline("This %s is a wand of fire!", xname(otmp));*/
			    pline("この%sは炎の杖だ！", xname(otmp));
			    doknown = TRUE;
			}
			Strcpy(post_engr_text,
/*JP				Blind ? "You feel the wand heat up." :
					"Flames fly from the wand.");*/
				Blind ? "杖が暖かくなったような気がした．" :
					"炎が杖から飛び散った．");
			break;
		    case WAN_LIGHTNING:
			ptext = TRUE;
			type  = BURN;
			if(!objects[otmp->otyp].oc_name_known) {
	    		    if (flags.verbose)
/*JP				pline("This %s is a wand of lightning!",*/
				pline("この%sは雷の杖だ！",
					xname(otmp));
			    doknown = TRUE;
			}
			if (!Blind) {
			    Strcpy(post_engr_text,
/*JP				    "Lightning arcs from the wand.");*/
				    "火花が杖から飛び散った．");
			    doblind = TRUE;
			} else
/*JP			    Strcpy(post_engr_text, "You hear crackling!");*/
			    Strcpy(post_engr_text, "パチパチと言う音を聞いた！");
			break;

		    /* type = MARK wands */
		    /* type = BLOOD wands */
		    }
		} else /* end if zappable */
		    if (Levitation) {
/*JP			You("can't reach the %s!", surface(u.ux,u.uy));*/
			You("%sに届かない！", surface(u.ux,u.uy));
			return(0);
		    }
		break;

	    case WEAPON_CLASS:
		if(is_blade(otmp))
		    if ((int)otmp->spe > -3)
			type = ENGRAVE;
		    else
/*JP			Your("%s too dull for engraving.", aobjnam(otmp,"are"));*/
			pline("%sは刃がボロボロで，文字を彫れない．",xname(otmp));
		break;

	    case TOOL_CLASS:
		if(otmp == ublindf) {
		    pline(
/*JP		"That is a bit difficult to engrave with, don't you think?");*/
		"ちょっとそれで彫るのは大変だろう，そう思わない？");
		    return(0);
		}
		switch (otmp->otyp)  {
		    case MAGIC_MARKER:
			if (otmp->spe <= 0)
/*JP			    Your("marker has dried out.");*/
			    Your("マーカは喝ききった．");
			else
			    type = MARK;
			break;
		    case TOWEL:
 			/* Can't really engrave with a towel */
			ptext = FALSE;
			if (oep)
			    if ((oep->engr_type == DUST ) ||
				(oep->engr_type == BLOOD) ||
				(oep->engr_type == MARK )) {
				if (!Blind)
/*JP				    You("wipe out the message here.");*/
				    You("メッセージを拭きとった．");
				else
/*JP				    Your("%s gets %s.", xname(otmp),
					  is_ice(u.ux,u.uy) ?
					  "frosty" : "dusty");*/
				    pline("%sは%sなった．", xname(otmp),
					  is_ice(u.ux,u.uy) ?
					  "氷づけに" : "ほこりまみれに");
				dengr = TRUE;
			    } else
/*JP				Your("%s can't wipe out this engraving.",*/
				pline("%sはこの文字を拭きとれない．",
				     xname(otmp));
			else
/*JP			    Your("%s gets %s.", xname(otmp),
				  is_ice(u.ux,u.uy) ? "frosty" : "dusty");*/
			    pline("%sは%sなった．", xname(otmp),
				  is_ice(u.ux,u.uy) ? "氷づけに" : "ほこりまみれに");
			break;
		    default:
			break;
		}
		break;

	    case VENOM_CLASS:
#ifdef WIZARD
		if (wizard) {
/*JP		    pline("Writing a poison pen letter??");*/
		    pline("ふむ．これこそ本当の毒舌だ．");
		    break;
		}
#endif
	    case ILLOBJ_CLASS:
		impossible("You're engraving with an illegal object!");
		break;
	}

	/* End of implement setup */

	/* Identify stylus */
	if (doknown) {
	    makeknown(otmp->otyp);
	    more_experienced(0,10);
	}

	if (teleengr) {
	    register int tx,ty;

	    do  {
 		tx = rn1(COLNO-3,2);
		ty = rn2(ROWNO);
	    } while(!goodpos(tx,ty, (struct monst *)0, (struct permonst *)0));

	    oep->engr_x = tx;
	    oep->engr_y = ty;

	    oep = (struct engr *)0;
	}

	if (dengr) {
	    del_engr (oep);
	    oep = (struct engr *)0;
	}

	/* Something has changed the engraving here */
	if (*buf) {
	    make_engr_at(u.ux, u.uy, buf, moves, type);
/*JP	    pline("The engraving now reads: \"%s\".", buf);*/
	    pline("刻まれた文字を読んだ:「%s」．", buf);
	    ptext = FALSE;
	}

	if (zapwand && (otmp->spe < 0)) {
/*JP	    pline("%s %sturns to dust.",
		  The(xname(otmp)), Blind ? "" : "glows violently, then ");*/
	    pline("%sは%s塵となった．",
		  The(xname(otmp)), Blind ? "" : "激しく輝き");
/*JP You("are not going to get anywhere trying to write in the %s with your dust.",
		is_ice(u.ux,u.uy) ? "frost" : "dust");*/
 You("塵で%sに何か書こうとしたが，できなかった．",
		is_ice(u.ux,u.uy) ? "氷" : "ほこり");
	    useup(otmp);
	    ptext = FALSE;
	}

	if (!ptext) {		/* Early exit for some implements. */
	    if (Levitation && (otmp->oclass == WAND_CLASS))
/*JP		You("can't reach the %s!", surface(u.ux,u.uy));*/
		You("%sに届かない！", surface(u.ux,u.uy));
	    return(1);
	}

	/* Special effects should have deleted the current engraving (if
	 * possible) by now.
	 */

	if (oep) {
	    register char c = 'n';

	    /* Give player the choice to add to engraving. */

	    if ( (type == oep->engr_type) && (!Blind ||
		 (oep->engr_type == BURN) || (oep->engr_type == ENGRAVE)) ) {
/*JP	    	c = yn_function("Do you want to add to the current engraving?",*/
	    	c = yn_function("何か書き加えますか？",
				ynqchars, 'y');
		if (c == 'q') {
/*JP		    pline("Never mind.");*/
		    pline("へ？");
		    return(0);
		}
	    }

	    if (c == 'n' || Blind)

		if( (oep->engr_type == DUST) || (oep->engr_type == BLOOD) ||
		    (oep->engr_type == MARK) ) {
		    if (!Blind) {
/*JP			You("wipe out the message that was %s here.",
			    ((oep->engr_type == DUST)  ? "written in the dust" :
			    ((oep->engr_type == BLOOD) ? "scrawled in blood"   :
							 "written")));*/
			You("%sメッセージを拭きとった．",
			    ((oep->engr_type == DUST)  ? "ほこりに書かれている" :
			    ((oep->engr_type == BLOOD) ? "血文字でなぐり書きされている"   :
							 "書かれている")));
			del_engr(oep);
			oep = (struct engr *)0;
		    } else
		   /* Don't delete engr until after we *know* we're engraving */
			eow = TRUE;
		} else
		    if ( (type == DUST) || (type == MARK) || (type == BLOOD) ) {
			You(
/*JP			 "cannot wipe out the message that is %s the %s here.",
			 oep->engr_type == BURN ? 
			   (is_ice(u.ux,u.uy) ? "melted into" : "burned into") :
			   "engraved in", surface(u.ux,u.uy));*/
			 "%sに%sメッセージを拭きとれなかった．", surface(u.ux,u.uy),
			 oep->engr_type == BURN ? 
			   (is_ice(u.ux,u.uy) ? "刻まれている" : "燃えている") :
			   "刻まれている");
			return(1);
		    } else
			if ( (type != oep->engr_type) || (c == 'n') ) {
			    if (!Blind || !Levitation)
/*JP				You("will overwrite the current message.");*/
				You("メッセージを上書きしようとした．");
			    eow = TRUE;
			}
	}

	eloc = surface(u.ux,u.uy);
	switch(type){
	    default:
/*JP		everb = (oep && !eow ? "add to the weird writing on" :
				       "write strangely on");*/
		everb = (oep && !eow ? "奇妙な文字列に書き加える" :
				       "奇妙な文字列を書く");
		break;
	    case DUST:
/*JP		everb = (oep && !eow ? "add to the writing in" :
				       "write in");*/
		everb = (oep && !eow ? "書き加える" :
				       "書く");
/*JP		eloc = is_ice(u.ux,u.uy) ? "frost" : "dust";*/
		eloc = is_ice(u.ux,u.uy) ? "氷" : "ほこり";
		break;
	    case ENGRAVE:
/*JP		everb = (oep && !eow ? "add to the engraving in" :
				       "engrave in");*/
		everb = (oep && !eow ? "刻み加える" :
				       "刻む");
		break;
	    case BURN:
		everb = (oep && !eow ? 
/*JP			( is_ice(u.ux,u.uy) ? "add to the text melted into" :
			                      "add to the text burned into") :
			( is_ice(u.ux,u.uy) ? "melt into" : "burn into"));*/
			( is_ice(u.ux,u.uy) ? "刻み加える" :
			                      "燃えている文字に書き加え") :
			( is_ice(u.ux,u.uy) ? "刻む" : "焼印をいれる"));
		break;
	    case MARK:
/*JP		everb = (oep && !eow ? "add to the graffiti on" :
				       "scribble on");*/
		everb = (oep && !eow ? "落書に書き加える" :
				       "はしり書きする");
		break;
	    case BLOOD:
/*JP		everb = (oep && !eow ? "add to the scrawl on" :
				       "scrawl on");*/
		everb = (oep && !eow ? "なぐり書きに書き加える" :
				       "なぐり書きする");
		break;
	}

	/* Tell adventurer what is going on */
	if (otmp != &zeroobj)
/*JP	    You("%s the %s with %s.", everb, eloc, doname(otmp));*/
	    You("%sで%sに%s", doname(otmp), eloc, jconj(everb,"た"));
	else
/*JP	    You("%s the %s with your %s.", everb, eloc,
		makeplural(body_part(FINGER)));*/
	    You("%sで%sに%s",		makeplural(body_part(FINGER)),
						   eloc,jconj(everb,"た"));

	/* Prompt for engraving! */
/*JP	Sprintf(qbuf,"What do you want to %s the %s here?", everb, eloc);*/
	Sprintf(qbuf,"%sに何と%s？", eloc, jconj(everb,"ますか"));
	getlin(qbuf, ebuf);

	/* Mix up engraving if surface or state of mind is unsound.  */
	/* Original kludge by stewr 870708.  modified by njm 910722. */
	for (sp = ebuf; *sp; sp++)
	    if ( ((type == DUST || type == BLOOD) && !rn2(25)) ||
		 (Blind   && !rn2(9)) || (Confusion     && !rn2(12)) ||
		 (Stunned && !rn2(4)) || (Hallucination && !rn2(1)) )
		 *sp = '!' + rn2(93); /* ASCII-code only */

	/* Count the actual # of chars engraved not including spaces */
	len = strlen(ebuf);

/*JP	for (sp = ebuf, spct = 0; *sp; sp++) if (isspace(*sp)) spct++;*/
	for (sp = ebuf, spct = 0; *sp; sp++) if (isspace_8(*sp)) spct++;

	if ( (len == spct) || index(ebuf, '\033') ) {
	    if (zapwand) {
		if (!Blind)
/*JP		    pline("%s glows, then fades.", The(xname(otmp)));*/
		    pline("%sは一瞬輝いた", The(xname(otmp)));
	    	return(1);
	    } else {
/*JP		pline("Never mind.");*/
		pline("へ？");
		return(0);
	    }
	}

	len -= spct;

	/* Previous engraving is overwritten */
	if (eow) {
	    del_engr(oep);
	    oep = (struct engr *)0;
	}

	/* Figure out how long it took to engrave, and if player has
	 * engraved too much.
	 */
	switch(type){
	    default:
		multi = -(len/10);
/*JP		if (multi) nomovemsg = "You finish your weird engraving.";*/
		if (multi) nomovemsg = "あなたは奇妙な刻みを終えた．";
		break;
	    case DUST:
		multi = -(len/10);
/*JP		if (multi) nomovemsg = "You finish writing in the dust.";*/
		if (multi) nomovemsg = "あなたはほこりに書くのを終えた．";
		break;
	    case ENGRAVE:
		multi = -(len/10);
		if ((otmp->oclass == WEAPON_CLASS) &&
		    ((otmp->otyp != ATHAME) || otmp->cursed)) {
		    multi = -len;
		    maxelen = ((otmp->spe + 3) * 2) + 1;
			/* -2 = 3, -1 = 5, 0 = 7, +1 = 9, +2 = 11
			 * Note: this does not allow a +0 anything (except
			 *	 an athame) to engrave "Elbereth" all at once.
			 *	 However, you could now engrave "Elb", then
			 *	 "ere", then "th".
			 */
/*JP		    Your("%s dull.", aobjnam(otmp, "get"));*/
		    Your("%sは刃こぼれした．", xname(otmp));
		    if (len > maxelen) {
		    	multi = -maxelen;
			otmp->spe = -3;
		    } else
			if (len > 1) otmp->spe -= len >> 1;
			else otmp->spe -= 1; /* Prevent infinite engraving */
		} else
		    if ( (otmp->oclass == RING_CLASS) ||
			 (otmp->oclass == GEM_CLASS) )
			multi = -len;
/*JP		if (multi) nomovemsg = "You finish engraving.";*/
		if (multi) nomovemsg = "あなたは刻むのを終えた．";
		break;
	    case BURN:
		multi = -(len/10);
		if (multi)
		    nomovemsg = is_ice(u.ux,u.uy) ?
/*JP			"You finish melting your message into the ice.":
			"You finish burning your message into the floor.";*/
			"氷へメッセージを刻むのを終えた．":
			"炎へメッセージを燃印をいれるのを終えた．";
		break;
	    case MARK:
		multi = -(len/10);
		if ((otmp->oclass == TOOL_CLASS) &&
		    (otmp->otyp == MAGIC_MARKER)) {
		    maxelen = (otmp->spe) * 2; /* one charge / 2 letters */
		    if (len > maxelen) {
/*JP			Your("marker dries out.");*/
			Your("マーカは喝ききった．");
			otmp->spe = 0;
			multi = -(maxelen/10);
		    } else
			if (len > 1) otmp->spe -= len >> 1;
			else otmp->spe -= 1; /* Prevent infinite grafitti */
		}
/*JP		if (multi) nomovemsg = "You finish defacing the dungeon.";*/
		if (multi) nomovemsg = "あなたは迷宮への落書を書き終えた．";
		break;
	    case BLOOD:
		multi = -(len/10);
/*JP		if (multi) nomovemsg = "You finish scrawling.";*/
		if (multi) nomovemsg = "はしり書きを書き終えた．";
		break;
	}

	/* Chop engraving down to size if necessary */
	if (len > maxelen) {
	    for (sp = ebuf; (maxelen && *sp); sp++)
/*JP		if (!isspace(*sp)) maxelen--;*/
		if (!isspace_8(*sp)) maxelen--;
	    if (!maxelen && *sp) {
		*sp = (char)0;
/*JP		if (multi) nomovemsg = "You cannot write any more.";*/
		if (multi) nomovemsg = "これ以上何も書けなかった．";
/*JP		You("only are able to write \"%s\"", ebuf);*/
		You("単に「%s」と書けただけだ．", ebuf);
	    }
	}

	/* Add to existing engraving */
	if (oep) Strcpy(buf, oep->engr_txt);	

	(void) strncat(buf, ebuf, (BUFSZ - (int)strlen(buf) - 1));

	make_engr_at(u.ux, u.uy, buf, (moves - multi), type);

	if (post_engr_text[0]) pline(post_engr_text);

	if (doblind) {
/*JP	    You("are blinded by the flash!");*/
	    You("まばゆい光で目がくらんだ！");
	    make_blinded((long)rnd(50),FALSE);
	}

	return(1);
}

void
save_engravings(fd, mode)
int fd, mode;
{
	register struct engr *ep = head_engr;
	register struct engr *ep2;
#ifdef GCC_WARN
	static long nulls[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif
	while(ep) {
	    ep2 = ep->nxt_engr;
	    if(ep->engr_lth && ep->engr_txt[0]){
		bwrite(fd, (genericptr_t)&(ep->engr_lth), sizeof(ep->engr_lth));
		bwrite(fd, (genericptr_t)ep, sizeof(struct engr) + ep->engr_lth);
	    }
	    if (mode & FREE_SAVE)
		dealloc_engr(ep);
	    ep = ep2;
	}

#ifdef GCC_WARN
	bwrite(fd, (genericptr_t)nulls, sizeof(unsigned));
#else
	bwrite(fd, (genericptr_t)nul, sizeof(unsigned));
#endif

	if (mode & FREE_SAVE)
	    head_engr = 0;
}

void
rest_engravings(fd)
int fd;
{
register struct engr *ep;
unsigned lth;
	head_engr = 0;
	while(1) {
		mread(fd, (genericptr_t) &lth, sizeof(unsigned));
		if(lth == 0) return;
		ep = newengr(lth);
		mread(fd, (genericptr_t) ep, sizeof(struct engr) + lth);
		ep->nxt_engr = head_engr;
		head_engr = ep;
		ep->engr_txt = (char *) (ep + 1);	/* Andreas Bormann */
		/* mark as finished for bones levels -- no problem for
		 * normal levels as the player must have finished engraving
		 * to be able to move again */
		ep->engr_time = moves;
	}
}

STATIC_OVL void
del_engr(ep)
register struct engr *ep;
{
	if (ep == head_engr) {
		head_engr = ep->nxt_engr;
	} else {
		register struct engr *ept;

		for (ept = head_engr; ept; ept = ept->nxt_engr)
		    if (ept->nxt_engr == ep) {
			ept->nxt_engr = ep->nxt_engr;
			break;
		    }
		if (!ept) {
		    impossible("Error in del_engr?");
		    return;
		}
	}
	dealloc_engr(ep);
}

#endif /* OVLB */

/*engrave.c*/
