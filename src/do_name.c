/*	SCCS Id: @(#)do_name.c	3.1	93/05/15	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/


#include "hack.h"

#ifdef OVLB

static void FDECL(do_oname, (struct obj *));

void
getpos(cc,force,goal)
coord	*cc;
boolean force;
const char *goal;
{
    register int cx, cy, i, c;
    int sidx, tx, ty;
    int lastc, lastx, lasty;
    const char *sdp = flags.num_pad ? ndir : sdir;

/*JP    if(flags.verbose) pline("(For instructions type a ?)");*/
    if(flags.verbose) pline("(?でヘルプ)");
    cx = cc->x;
    cy = cc->y;
    lastc = -1;
    lastx = lasty = 0;
#ifdef CLIPPING
    cliparound(cx, cy);
#endif
    curs(WIN_MAP, cx,cy);
    flush_screen(0);
#ifdef MAC
	lock_mouse_cursor(TRUE);
#endif
    while((c = nh_poskey(&tx, &ty, &sidx)) != '.') {
        if(c == '\033') {
            cc->x = -10;
	    clear_nhwindow(WIN_MESSAGE);
            return;
        }
	if(c == 0) {
	    /* a mouse click event, just assign and return */
	    cx = tx;
	    cy = ty;
	    break;
	}
	for(i=0; i<8; i++)
	    if (sdp[i] == c) {
		if (1 <= cx + xdir[i] && cx + xdir[i] < COLNO)
		    cx += xdir[i];
		if (0 <= cy + ydir[i] && cy + ydir[i] < ROWNO)
		    cy += ydir[i];
		goto nxtc;
	    } else if (sdp[i] == lowc((char)c)) {
		cx += xdir[i]*8;
		cy += ydir[i]*8;
		if(cx < 1) cx = 1;
		if(cx > COLNO-1) cx = COLNO-1;
		if(cy < 0) cy = 0;
		if(cy > ROWNO-1) cy = ROWNO-1;
		goto nxtc;
	    }

	if(c == '?'){
	    char sbuf[80];
	    winid tmpwin = create_nhwindow(NHW_MENU);
/*JP	    Sprintf(sbuf, "Use [%s] to move the cursor to %s.",
		  flags.num_pad ? "2468" : "hjkl", goal);*/
	    Sprintf(sbuf, "[%s]で%sへ移動できる．",
		  flags.num_pad ? "2468" : "hjkl", goal);
	    putstr(tmpwin, 0, sbuf);
	    putstr(tmpwin, 0,
/*JP		   "Use [HJKL] to move the cursor 8 units at a time.");*/
		   "[HJKL]で一度に8歩移動できる．");
/*JP	    putstr(tmpwin, 0, "Or enter a background symbol (ex. <).");
	    putstr(tmpwin, 0, "Type a . when you are at the right place.");*/
	    putstr(tmpwin, 0, "[<]で元の位置に戻る．");
	    putstr(tmpwin, 0, "[.]で決定．");
	    if(!force)
/*JP		putstr(tmpwin, 0, "Type Space or Escape when you're done.");*/
		putstr(tmpwin, 0, "スペースまたはエスケープで終了．");
	    putstr(tmpwin, 0, "");
	    display_nhwindow(tmpwin, TRUE);
	    destroy_nhwindow(tmpwin);
	} else {
	    if (!index(quitchars, c)) {
		for(sidx = 1; sidx < sizeof(showsyms); sidx++)
		    if(defsyms[sidx].sym == c) {
			/* sidx = cmap_to_glyph(sidx); */
			if(sidx != lastc) {
			    lasty = 0;
			    lastx = 1;
			}
			lastc = sidx;
		    loopback:
			for (ty = lasty; ty < ROWNO; ty++) {
			    for (tx = lastx; tx < COLNO; tx++) {
				if (glyph_is_cmap(levl[tx][ty].glyph) &&
	 defsyms[sidx].sym == defsyms[glyph_to_cmap(levl[tx][ty].glyph)].sym) {
				    cx = tx;
				    lastx = tx+1;
				    cy = ty;
				    lasty = ty;
				    goto nxtc;
				}
			    }
			    lastx = 1;
			}
			if(lasty != 0) {
			    lasty = 0;
			    lastx = 1;
			    goto loopback;
			}
/*JP			pline("Can't find dungeon feature '%c'", c);*/
			pline("'%c'？", c);
			goto nxtc;
		    }

/*JP		pline("Unknown direction: '%s' (%s).",*/
		pline("その方向はない: '%s' (%s).",
		      visctrl((char)c),
		      force ?
/*JP		      flags.num_pad ? "use 2468 or ." :
		      "use hjkl or ." :
		      "aborted");*/
		      flags.num_pad ? "[2468]で移動，[.]で終了" :
		      "[hjkl]で移動，[.]で終了" :
		      "中断した");
	    }
	    if(force) goto nxtc;
/*JP	    pline("Done.");*/
	    pline("以上．");
	    cc->x = -1;
	    cc->y = 0;
	    return;
	}
    nxtc:	;
#ifdef CLIPPING
	cliparound(cx, cy);
#endif
	curs(WIN_MAP,cx,cy);
	flush_screen(0);
    }
#ifdef MAC
	lock_mouse_cursor(FALSE);
#endif
    cc->x = cx;
    cc->y = cy;
    return;
}

struct monst *
christen_monst(mtmp, name)
struct monst *mtmp;
const char *name;
{
	register int lth,i;
	register struct monst *mtmp2;

	/* dogname and catname are 63-character arrays; the generic naming
	 * function do_mname() below also cut names off at 63 characters */
	lth = strlen(name)+1;
	if(lth > 63){
		lth = 63;
	}
/*JP*/
	if(is_kanji2(name,lth-1))
	  --lth;

	mtmp2 = newmonst(mtmp->mxlth + lth);
	*mtmp2 = *mtmp;
	for(i=0; i<mtmp->mxlth; i++)
		((char *) mtmp2->mextra)[i] = ((char *) mtmp->mextra)[i];
	mtmp2->mnamelth = lth;
	(void)strncpy(NAME(mtmp2), name, lth);
	NAME(mtmp2)[lth-1] = 0;
	replmon(mtmp,mtmp2);
	return(mtmp2);
}

int
do_mname()
{
	char buf[BUFSZ];
	coord cc;
	register int cx,cy;
	register struct monst *mtmp;
	register char *curr;
	boolean blank;
	char qbuf[QBUFSZ];

	if (Hallucination) {
/*JP		You("would never recognize it anyway.");*/
		You("それを認識できない．");
		return 0;
	}
	cc.x = u.ux;
	cc.y = u.uy;
/*JP	getpos(&cc, FALSE, "the monster you want to name");*/
	getpos(&cc, FALSE, "あなたが名づけたい怪物");
	cx = cc.x;
	cy = cc.y;
	if(cx < 0) return(0);
	if (cx == u.ux && cy == u.uy) {
/*JP		pline("This %s creature is called %s and cannot be renamed.",*/
		pline("この%s生き物は%sと呼ばれていて，名前は変更できない．",
		ACURR(A_CHA) > 14 ?
/*JP		(flags.female ? "beautiful" : "handsome") :
		"ugly",*/
		(flags.female ? "美人の" : "かっこいい") :
		"醜い",
		plname);
		return(0);
	}
	mtmp = m_at(cx, cy);
	if (!mtmp || (!sensemon(mtmp) &&
			(!cansee(cx,cy) || mtmp->mundetected
			|| mtmp->m_ap_type == M_AP_FURNITURE
			|| mtmp->m_ap_type == M_AP_OBJECT
			|| (mtmp->minvis && !See_invisible)))) {
/*JP		pline("I see no monster there.");*/
		pline("そこに怪物はいない．");
		return(0);
	}
/*JP	Sprintf(qbuf, "What do you want to call %s?", x_monnam(mtmp, 0,*/
	Sprintf(qbuf, "%sを何と呼びますか？", x_monnam(mtmp, 0,
		(char *)0, 1));
	getlin(qbuf,buf);
	if(!*buf || *buf == '\033') return(0);

	/* unnames monster if all spaces */
	for (curr = buf, blank = 1; *curr; blank = (*curr++ == ' '));
	if(blank) *buf = '\0';

 	if(type_is_pname(mtmp->data))
/*JP 	    pline("%s doesn't like being called names!", Monnam(mtmp));*/
 	    pline("%sは名前で呼ばれるのが嫌いなようだ！", Monnam(mtmp));
	else (void) christen_monst(mtmp, buf);
	return(0);
}

/*
 * This routine changes the address of obj. Be careful not to call it
 * when there might be pointers around in unknown places. For now: only
 * when obj is in the inventory.
 */
static
void
do_oname(obj)
register struct obj *obj;
{
	char buf[BUFSZ], qbuf[QBUFSZ];
	register char *curr;

/*JP	Sprintf(qbuf, "What do you want to name %s?", doname(obj));*/
	Sprintf(qbuf, "%sを何と名づけますか？", doname(obj));
	getlin(qbuf, buf);
	if(!*buf || *buf == '\033')	return;

	/* strip trailing spaces; unnames item if all spaces */
	for (curr = eos(buf); curr > buf; )
		if (*--curr == ' ') *curr = '\0'; else break;

	if(obj->oartifact)
/*JP		pline("The artifact seems to resist the attempt.");*/
		pline("聖器は名づけを拒否しているようだ．");
	else if (restrict_name(obj, buf) || exist_artifact(obj->otyp, buf)) {
		int n = rn2((int)strlen(buf));
		register char c1, c2;

		c1 = lowc(buf[n]);
		do c2 = 'a' + rn2('z'-'a'); while (c1 == c2);
		buf[n] = (buf[n] == c1) ? c2 : highc(c2);  /* keep same case */
/*JP		pline("While engraving your %s slips.", body_part(HAND));*/
		pline("刻んでいる間に%sが滑ってしまった．", body_part(HAND));
		display_nhwindow(WIN_MESSAGE, FALSE);
/*JP		You("engrave: \"%s\".",buf);*/
		You("刻んだ: 「%s」．",buf);
		(void)oname(obj, buf, 1);
	}
	else
		(void)oname(obj, buf, 1);
}

struct obj *
oname(obj, buf, ininv)
register struct obj *obj;
const char	*buf;
register int ininv;
{
	register struct obj *otmp, *otmp2;
	register int	lth;

	lth = *buf ? strlen(buf)+1 : 0;
	if(lth > 63){
		lth = 63;
	}
/*JP*/
	if(is_kanji2(buf,lth-1))
	  --lth;

	/* if already properly named */
	if(lth == obj->onamelth && (!lth || !strcmp(ONAME(obj),buf)))
		return obj;

	/* If named artifact exists in the game, do not create another.
	 * Also trying to create an artifact shouldn't de-artifact
	 * it (e.g. Excalibur from prayer). In this case the object
	 * will retain its current name. */
	if (obj->oartifact || exist_artifact(obj->otyp, buf))
		return obj;

	otmp2 = newobj(lth);
	*otmp2 = *obj;	/* the cobj pointer is copied to otmp2 */
	otmp2->onamelth = lth;
	artifact_exists(otmp2, buf, TRUE);

#ifdef __GNUC__
	/* Avoid an old compiler bug (always gave empty name otherwise). */
	if (buf) (void)donull();
#endif
	if(lth) {
		(void)strncpy(ONAME(otmp2), buf, lth);
		ONAME(otmp2)[lth-1] = 0;
	}
	if (obj->owornmask) {
		/* Note: dying by burning in Hell causes problems if you
		 * try doing this when owornmask isn't set.
		 */
		setworn((struct obj *)0, obj->owornmask);
		setworn(otmp2, otmp2->owornmask);
	}

	if (ininv) {
		/* do freeinv(obj); etc. by hand in order to preserve
		   the position of this object in the inventory */
		if(obj == invent) invent = otmp2;
		else for(otmp = invent; ; otmp = otmp->nobj){
			if(!otmp)
				panic("oname: cannot find obj.");
			if(otmp->nobj == obj){
				otmp->nobj = otmp2;
				break;
			}
		}
	}
	/* obfree(obj, otmp2);	/* now unnecessary: no pointers on bill */
	dealloc_obj(obj);	/* let us hope nobody else saved a pointer */
	return otmp2;
}

static NEARDATA const char callable[] = {
	SCROLL_CLASS, POTION_CLASS, WAND_CLASS, RING_CLASS, AMULET_CLASS,
	GEM_CLASS, SPBOOK_CLASS, ARMOR_CLASS, TOOL_CLASS, 0 };

int
ddocall()
{
	register struct obj *obj;
#ifdef REDO
	char	ch;
#endif
	char allow_all[2];

	switch(
#ifdef REDO
		ch = 
#endif
/*JP		ynq("Name an individual object?")) {*/
		ynq("持ち物に個別の名前をつけますか？")) {
	case 'q':
		break;
	case 'y':
#ifdef REDO
		savech(ch);
#endif
		allow_all[0] = ALL_CLASSES; allow_all[1] = '\0';
/*JP		obj = getobj(allow_all, "name");*/
		obj = getobj(allow_all, "名づける");
		if(obj) do_oname(obj);
		break;
	default :
#ifdef REDO
		savech(ch);
#endif
/*JP		obj = getobj(callable, "call");*/
		obj = getobj(callable, "呼ぶ");
		if (obj) {
			if (!obj->dknown) {
/*JP				You("would never recognize another one.");*/
				You("他に認識できない．");
				return 0;
			}
			docall(obj);
		}
		break;
	}
	return 0;
}

void
docall(obj)
register struct obj *obj;
{
	char buf[BUFSZ], qbuf[QBUFSZ];
	struct obj otemp;
	register char **str1;
	register char *str;
	boolean blank;

	if (!obj->dknown) return; /* probably blind */
	otemp = *obj;
	otemp.quan = 1L;
	otemp.onamelth = 0;
	if (objects[otemp.otyp].oc_class == POTION_CLASS && otemp.corpsenm) {
		/* kludge, meaning it's sink water */
/*JP		Sprintf(qbuf,"Call a stream of %s fluid:",*/
		Sprintf(qbuf,"%sの液体:",
				OBJ_DESCR(objects[otemp.otyp]));
	} else
/*JP		Sprintf(qbuf, "Call %s:", an(xname(&otemp)));*/
		Sprintf(qbuf, "%s:", an(xname(&otemp)));
	getlin(qbuf, buf);
	if(!*buf || *buf == '\033')
		return;

	/* clear old name */
	str1 = &(objects[obj->otyp].oc_uname);
	if(*str1) free((genericptr_t)*str1);

	/* uncalls item if all spaces */
	for (str = buf, blank = 1; *str; blank = (*str++ == ' '));
	if(blank) *buf = '\0';
	if (!*buf) {
		if (*str1)	/* had name, so possibly remove from disco[] */
			undiscover_object(obj->otyp),  *str1 = NULL;
	} else {
		*str1 = strcpy((char *) alloc((unsigned)strlen(buf)+1), buf);
		discover_object(obj->otyp, FALSE); /* possibly add to disco[] */
	}
}

#endif /*OVLB*/
#ifdef OVL0

static const char *ghostnames[] = {
	/* these names should have length < PL_NSIZ */
	/* Capitalize the names for aesthetics -dgk */
	"Adri", "Andries", "Andreas", "Bert", "David", "Dirk", "Emile",
	"Frans", "Fred", "Greg", "Hether", "Jay", "John", "Jon", "Karnov",
	"Kay", "Kenny", "Kevin", "Maud", "Michiel", "Mike", "Peter", "Robert",
	"Ron", "Tom", "Wilmar", "Nick Danger", "Phoenix", "Jiro", "Mizue",
	"Stephan", "Lance Braccus", "Shadowhawk"
};

/* Monster naming functions:
 * x_monnam is the generic monster-naming function.
 * mon_nam: the rust monster  it  the invisible orc  Fido
 * l_monnam:  rust monster    it  invisible orc      dog called fido
 * Monnam:    The rust monster    It  The invisible orc  Fido
 * Adjmonnam: The poor rust monster  It   The poor invisible orc  The poor Fido
 * Amonnam:   A rust monster      It  An invisible orc   Fido
 * a_monnam:  a rust monster      it  an invisible orc   Fido
 */

char *
x_monnam(mtmp, article, adjective, called)
register struct monst *mtmp;
/* Articles:
 * 0: "the" in front of everything except names and "it"
 * 1: "the" in front of everything except "it"; looks bad for names unless you
 *    are also using an adjective.
 * 2: "a" in front of everything except "it".
 * 3: no article at all.
 */
int article, called;
const char *adjective;
{
#ifdef LINT	/* static char buf[BUFSZ]; */
	char buf[BUFSZ];
#else
	static char buf[BUFSZ];
	static char mname[BUFSZ];
#endif
	char *name = (mtmp->mnamelth && !Hallucination && !mtmp->isshk) ?
	                          NAME(mtmp) : 0;
	int force_the = (!Hallucination && mtmp->data ==
		&mons[PM_WIZARD_OF_YENDOR]);

	buf[0] = '\0';
	if(mtmp->ispriest || mtmp->isminion) {
		name = priestname(mtmp);
		if (article == 3 && !strncmp(name, "the ", 4)) name += 4;
		return name;
	}

	Strcpy(mname,jtrns_mon(mtmp->data->mname));
/*
	if(is_were(mtmp->data))
	  Strcpy(mname+strlen(mname)-4,is_female(mtmp->data)?jtrns_mon("woman"):jtrns_mon("man"));
*/
	if(!canseemon(mtmp) && !sensemon(mtmp) &&
				!(u.uswallow && mtmp == u.ustuck) && !killer) {
	    if(!mtmp->wormno || (mtmp != m_at(bhitpos.x, bhitpos.y)) ||
	       !(cansee(bhitpos.x, bhitpos.y) && mon_visible(mtmp))) {
/*JP		Strcpy(buf, "it");*/
		Strcpy(buf, "何者か");
		return (buf);
	    }
	}
	if (mtmp->isshk) {
		Strcpy(buf, shkname(mtmp));
		if (mtmp->data == &mons[PM_SHOPKEEPER] && !mtmp->minvis)
		    return(buf);
		/* For normal shopkeepers, just 'Asidonhopo'.
		 * For unusual ones, 'Asidonhopo the invisible shopkeeper'
		 * or 'Asidonhopo the blue dragon'.
		 */
/*JP		Strcat(buf, " ");*/
		Strcat(buf, "と言う名の");
	}
/*JP
	if (force_the ||
	       ((article == 1 || ((!name || called) && article == 0)) &&
		   (Hallucination || !type_is_pname(mtmp->data))))
		Strcat(buf, "the ");*/
	if (adjective) {
		Strcat(buf, adjective);
/*JP		Strcat(buf, " ");*/
	}
	if (mtmp->minvis && !Blind)
/*JP		Strcat(buf, "invisible ");*/
		Strcat(buf, "姿の見えない");
	if (name && !called) {
/*JP		Strcat(buf, name);*/
		Strcat(buf, name);
		goto bot_nam;
	}
	if(name) {
/*JP		Strcat(buf, " called ");*/
		Strcat(buf, NAME(mtmp));
		Strcat(buf, "と呼ばれる");
	}
	if (mtmp->data == &mons[PM_GHOST] && !Hallucination) {
		register const char *gn = (const char *) mtmp->mextra;
		if(!*gn) {		/* might also look in scorefile */
		    gn = ghostnames[rn2(SIZE(ghostnames))];
		    Strcpy((char *) mtmp->mextra, !rn2(5) ? 
			                   (const char *)plname : gn);
		}
/*JP		Sprintf(buf, "%s ghost", s_suffix((char *) mtmp->mextra));*/
		Sprintf(buf, "%sの幽霊", s_suffix((char *) mtmp->mextra));
	} else {
	        if(Hallucination)
/*JP		    Strcat(buf, rndmonnam());*/
		    Strcat(buf, jtrns_mon(rndmonnam()));
		else {
		    if(is_mplayer(mtmp->data) && !In_endgame(&u.uz)) { 
		        char pbuf[BUFSZ];
	                Strcpy(pbuf, rank_of((unsigned)mtmp->m_lev,
		                              highc(mtmp->data->mname[0]), 
			                      (boolean)mtmp->female));
			Strcat(buf, lcase(pbuf));
		    } else
/*JP		        Strcat(buf, mtmp->data->mname);*/
		        Strcat(buf, mname);
		}
	}
bot_nam:
	if (article == 2 && !force_the && (!name || called) &&
	    (Hallucination || !type_is_pname(mtmp->data)))
/*JP		return an(buf);*/
		return buf;
	else
/*JP		return(buf);*/
		return buf;
}

#endif /* OVL0 */
#ifdef OVLB

char *
l_monnam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, 3, (char *)0, 1));
}

#endif /* OVLB */
#ifdef OVL0

char *
mon_nam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, 0, (char *)0, 0));
}

char *
Monnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = mon_nam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

#endif /* OVL0 */
#ifdef OVLB

char *
Adjmonnam(mtmp, adj)
register struct monst *mtmp;
register const char *adj;
{
	register char *bp = x_monnam(mtmp,1,adj,0);

	*bp = highc(*bp);
	return(bp);
}

char *
a_monnam(mtmp)
register struct monst *mtmp;
{
	return x_monnam(mtmp, 2, (char *)0, 0);
}

char *
Amonnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = a_monnam(mtmp);

	*bp = highc(*bp);

	return(bp);
}

static const char *bogusmons[] = {
	"jumbo shrimp", "giant pigmy", "gnu", "killer penguin", 
	"giant cockroach", "giant slug", "maggot", "pterodactyl",
	"tyrannosaurus rex", "basilisk", "beholder", "nightmare",
	"efreeti", "marid", "rot grub", "bookworm", "doppelganger",
	"shadow", "hologram", "jester", "attorney", "sleazoid",
	"killer tomato", "amazon", "robot", "battlemech",
	"rhinovirus", "harpy", "lion-dog", "rat-ant",
						/* misc. */
	"grue", "Christmas-tree monster", "luck sucker", "paskald",
	"brogmoid", "dornbeast",		/* Quendor (Zork, &c.) */
	"Ancient Multi-Hued Dragon", "Evil Iggy",
						/* Moria */
	"emu", "kestrel", "xeroc", "venus flytrap",
						/* Rogue */
	"creeping coins",			/* Wizardry */
	"hydra", "siren",			/* Greek legend */
	"killer bunny",				/* Monty Python */
	"rodent of unusual size",		/* The Princess Bride */
	"Smokey the bear",	/* "Only you can prevent forest fires!" */
	"Luggage",				/* Discworld */
	"Ent", 					/* Lord of the Rings */
	"tangle tree", "nickelpede", "wiggle",	/* Xanth */
	"white rabbit", "snark",		/* Lewis Carroll */
	"pushmi-pullyu",			/* Dr. Doolittle */
	"smurf",				/* The Smurfs */
	"tribble", "Klingon", "Borg",		/* Star Trek */
	"Ewok", 				/* Star Wars */
	"Totoro",				/* Tonari no Totoro */
	"ohmu", 				/* Nausicaa */
	"Godzilla", "King Kong",		/* monster movies */
	"earthquake beast",			/* old L of SH */
	"Invid",				/* Robotech */
	"Terminator",				/* The Terminator */
	"boomer",				/* Bubblegum Crisis */
	"Dalek",				/* Dr. Who ("Exterminate!") */
	"microscopic space fleet", "Ravenous Bugblatter Beast of Traal",
						/* HGttG */
	"teenage mutant ninja turtle",		/* TMNT */
	"samurai rabbit",			/* Usagi Yojimbo */
	"aardvark",				/* Cerebus */
	"Audrey II",				/* Little Shop of Horrors */
	"witch doctor", "one-eyed one-horned flying purple people eater",
						/* 50's rock 'n' roll */
	"Barney the dinosaur"			/* saccharine kiddy TV */
};

const char *
rndmonnam() {  /* Random name of monster type, if hallucinating */
	int name;

	do {
		name = rn2(PM_ARCHEOLOGIST + SIZE(bogusmons));
		/* archeologist: 1 past last valid monster */
	} while(name < PM_ARCHEOLOGIST &&
	    (type_is_pname(&mons[name]) || (mons[name].geno & G_NOGEN)));
	if (name >= PM_ARCHEOLOGIST) return jtrns_mon(bogusmons[name-PM_ARCHEOLOGIST]);
/*JP	return(mons[name].mname);*/
	return(jtrns_mon(mons[name].mname));
}

#ifdef OVL2

static NEARDATA const char *hcolors[] = {
/*JP	"ultraviolet", "infrared", "bluish-orange",
	"reddish-green", "dark white", "light black", "sky blue-pink",
	"salty", "sweet", "sour", "bitter",
	"striped", "spiral", "swirly", "plaid", "checkered", "argyle",
	"paisley", "blotchy", "guernsey-spotted", "polka-dotted",
	"square", "round", "triangular",
	"cabernet", "sangria", "fuchsia", "wisteria",
	"lemon-lime", "strawberry-banana", "peppermint",
	"romantic", "incandescent"*/
	"紫外色の", "赤外色の", "青色がかったオレンジ色の",
	"赤みがかった緑色の", "暗い白色の", "明るい黒の", "水色がかったピンク色の",
	"塩辛い", "甘い", "すっぱい", "苦い",
	"しま模様の", "らせん状の", "波状の", "格子模様状の", "チェック状の", "放射状の",
	"ペーズリー模様の", "しみ状の", "青色の斑点状の", "点状の",
	"四角形状の", "丸状の", "三角状の",
	"日本酒色の", "ぶどう酒色の", "桜色の", "藤色の",
	"レモンライム色の", "苺バナナ色の", "ペパーミント色の",
	"ロマンチックな色の", "まぶしい"
};

const char *
hcolor()
{
	return hcolors[rn2(SIZE(hcolors))];
}
#endif /* OVL2 */

const char *pronoun_pairs[][2] = {
	{"him", "her"}, {"Him", "Her"}, {"his", "her"}, {"His", "Her"},
	{"he", "she"}, {"He", "She"},
	{0, 0}
};

char *
self_pronoun(str, pronoun)
const char *str;
const char *pronoun;
{
	static NEARDATA char buf[BUFSZ];
	register int i;

	for(i=0; pronoun_pairs[i][0]; i++) {
		if(!strncmp(pronoun, pronoun_pairs[i][0], 3)) {
			Sprintf(buf, str, pronoun_pairs[i][flags.female]);
			return buf;
		}
	}
	impossible("never heard of pronoun %s?", pronoun);
	Sprintf(buf, str, pronoun_pairs[i][0]);
	return buf;
}

#ifdef REINCARNATION
const char *
roguename() /* Name of a Rogue player */
{
	char *i, *opts;

	if ((opts = getenv("ROGUEOPTS")) != 0) {
		for (i = opts; *i; i++)
			if (!strncmp("name=",i,5)) {
				char *j;
				if ((j = index(i+5,',')) != 0)
					*j = (char)0;
				return i+5;
			}
	}
	return rn2(3) ? (rn2(2) ? "Michael Toy" : "Kenneth Arnold")
		: "Glenn Wichman";
}
#endif

#endif /* OVLB */

/*do_name.c*/
