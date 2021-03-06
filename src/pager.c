/*	SCCS Id: @(#)pager.c	3.2	96/02/14	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* This file contains the command routines dowhatis() and dohelp() and */
/* a few other help related facilities */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "dlb.h"

static boolean FDECL(is_swallow_sym, (int));
static int FDECL(append_str, (char *, const char *));
/*JP static void FDECL(lookat, (int, int, char *));*/
static void FDECL(lookat, (int, int, char *, char *));
static void FDECL(checkfile, (char *, BOOLEAN_P));
static int FDECL(do_look, (BOOLEAN_P));
static boolean FDECL(help_menu, (int *));
#ifdef PORT_HELP
extern void NDECL(port_help);
#endif

/* Returns "true" for characters that could represent a monster's stomach. */
static boolean
is_swallow_sym(c)
int c;
{
    int i;
    for (i = S_sw_tl; i <= S_sw_br; i++)
	if ((int)showsyms[i] == c) return TRUE;
    return FALSE;
}

/*
 * Append new_str to the end of buf if new_str doesn't already exist as
 * a substring of buf.  Return 1 if the string was appended, 0 otherwise.
 * It is expected that buf is of size BUFSZ.
 */
static int
append_str(buf, new_str)
    char *buf;
    const char *new_str;
{
    int space_left;	/* space remaining in buf */

    if (strstri(buf, new_str)) return 0;

    space_left = BUFSZ - strlen(buf) - 1;
/*JP    (void) strncat(buf, " or ", space_left);*/
    (void) strncat(buf, "または", space_left);
    (void) strncat(buf, new_str, space_left - 4);
    return 1;
}

/*
 * Return the name of the glyph found at (x,y).
 */
static void
/*JP lookat(x, y, buf)*/
lookat(x, y, buf, buf2)
    int x, y;
    char *buf;
/*JP    `buf2' is English name for search data file easily. */
    char *buf2;
{
    register struct monst *mtmp;
    int glyph;

/*JP*/
    if(!query_lang_mode())
      (void)set_trns_mode(0);

    buf[0] = 0;
    buf2[0] = 0;
    glyph = glyph_at(x,y);
    if (u.ux == x && u.uy == y && canseeself()) {
/*JP	Sprintf(buf, "%s%s called %s",
		Invis ? "invisible " : "",
		u.mtimedone ? mons[u.umonnum].mname : player_mon()->mname,
		plname);*/
	Sprintf(buf, "%s%sという名の%s",
		Invis ? "姿の見えない" : "",
		plname,
		u.mtimedone ? jtrns_mon(mons[u.umonnum].mname, flags.female) : 
		jtrns_mon(player_mon()->mname, flags.female));
	Strcpy(buf2, 
		u.mtimedone ? mons[u.umonnum].mname : player_mon()->mname);
    }
    else if (u.uswallow) {
	/* all locations when swallowed other than the hero are the monster */
/*JP	Sprintf(buf, "interior of %s",
				    Blind ? "a monster" : a_monnam(u.ustuck));*/
	Sprintf(buf, "%sの内部",
				    Blind ? "怪物" : a_monnam(u.ustuck));
	Strcpy(buf2, Blind ? "a monster" : a_monnam(u.ustuck));
    }
    else if (glyph_is_monster(glyph)) {
	bhitpos.x = x;
	bhitpos.y = y;
	mtmp = m_at(x,y);
	if(mtmp != (struct monst *) 0) {
	    register boolean hp = (mtmp->data == &mons[PM_HIGH_PRIEST]);

/*JP	    Sprintf(buf, "%s%s%s",
		    (mtmp->mx != x || mtmp->my != y) ?
			((mtmp->isshk && !Hallucination)
				? "tail of " : "tail of a ") : "",
		    (!hp && mtmp->mtame && !Hallucination) ? "tame " :
		    (!hp && mtmp->mpeaceful && !Hallucination) ?
		                                          "peaceful " : "",
		    (hp ? "high priest" : l_monnam(mtmp)));*/
	    Sprintf(buf, "%s%s%s",
		    (!hp && mtmp->mtame && !Hallucination) ? "手なずけられた" :
		    (!hp && mtmp->mpeaceful && !Hallucination) ?
		                                          "友好的な" : "",
		    (hp ? "高僧" : l_monnam(mtmp)),
		    (mtmp->mx != x || mtmp->my != y) ?
			((mtmp->isshk && !Hallucination)
				? "の尻尾" : "の尻尾") : "");
	    if (u.ustuck == mtmp)
/*JP		Strcat(buf, (Upolyd && sticks(uasmon)) ?
			", being held" : ", holding you");*/
		Strcat(buf, (Upolyd && sticks(uasmon)) ?
			"，あなたが掴まえている" : "，あなたを掴まえている");
	    if (mtmp->mleashed)
/*JP		Strcat(buf, ", leashed to you");*/
		Strcat(buf, "，紐で結ばれている");
	    Strcpy(buf2, mtmp->data->mname);
	}
    }
    else if (glyph_is_object(glyph)) {
	struct obj *otmp = vobj_at(x,y);

	if(otmp == (struct obj *) 0 || otmp->otyp != glyph_to_obj(glyph)) {
	    if(glyph_to_obj(glyph) != STRANGE_OBJECT) {
		otmp = mksobj(glyph_to_obj(glyph), FALSE, FALSE);
		if(otmp->oclass == GOLD_CLASS)
		    otmp->quan = 2L; /* to force pluralization */
		else if (otmp->otyp == SLIME_MOLD)
		    otmp->spe = current_fruit;	/* give the fruit a type */
/*JP*/
		Strcpy(buf, distant_name(otmp, xname));
		set_trns_mode(0);	/* English mode */
		Strcpy(buf2, distant_name(otmp, xname));    /* English name */
		set_trns_mode(query_lang_mode()); /* reset trns mode */
		dealloc_obj(otmp);
	    }
	} else {
	    Strcpy(buf, distant_name(otmp, xname));
	    set_trns_mode(0);	/* English mode */
	    Strcpy(buf2, distant_name(otmp, xname));    /* English name */
	    set_trns_mode(query_lang_mode()); /* reset trns mode */
	}

	if (levl[x][y].typ == STONE || levl[x][y].typ == SCORR)
/*JP	    Strcat(buf, " embedded in stone");*/
	    Strcat(buf, "，岩に埋めこまれている");
	else if (IS_WALL(levl[x][y].typ) || levl[x][y].typ == SDOOR)
/*JP	    Strcat(buf, " embedded in a wall");*/
	    Strcat(buf, "，壁に埋めこまれている");
	else if (closed_door(x,y))
/*JP	    Strcat(buf, " embedded in a door");*/
	    Strcat(buf, "，扉に埋めこまれている");
	else if (is_pool(x,y))
/*JP	    Strcat(buf, " in water");*/
	    Strcat(buf, "，水中にいる");
	else if (is_lava(x,y))
/*JP	    Strcat(buf, " in molten lava");*/	/* [can this ever happen?] */
	    Strcat(buf, "，溶岩の中にいる");	/* [can this ever happen?] */
    }
    else if (glyph_is_trap(glyph)) {
	int tnum = glyph_to_trap(glyph);
	Strcpy(buf, jtrns_obj('^', defsyms[
	    trap_to_defsym(Hallucination ? rn2(TRAPNUM-3)+3 : tnum)].explanation));
    }
    else if(!glyph_is_cmap(glyph))
/*JP	Strcpy(buf,"dark part of a room");*/
	Strcpy(buf,"部屋の暗い部分");
    else switch(glyph_to_cmap(glyph)) {
    case S_altar:
	if(!In_endgame(&u.uz))
/*JP	    Sprintf(buf, "%s altar",
		align_str(Amask2align(levl[x][y].altarmask & ~AM_SHRINE)));*/
	    Sprintf(buf, "%sの祭壇",
		align_str(Amask2align(levl[x][y].altarmask & ~AM_SHRINE)));
/*JP	else Sprintf(buf, "aligned altar");*/
	else Sprintf(buf, "属性の祭壇");
	break;
    case S_ndoor:
	if (is_drawbridge_wall(x, y) >= 0)
/*JP	    Strcpy(buf,"open drawbridge portcullis");*/
	    Strcpy(buf,"降りている跳ね橋");
	else if ((levl[x][y].doormask & ~D_TRAPPED) == D_BROKEN)
/*JP	    Strcpy(buf,"broken door");*/
	    Strcpy(buf,"壊れた扉");
	else
/*JP	    Strcpy(buf,"doorway");*/
	    Strcpy(buf,"通路");
	break;
    case S_cloud:
/*JP	Strcpy(buf, Is_airlevel(&u.uz) ? "cloudy area" : "fog/vapor cloud");*/
	Strcpy(buf, Is_airlevel(&u.uz) ? "曇っている場所" : "霧/蒸気の雲");
	break;
    default:
/*JP	Strcpy(buf,defsyms[glyph_to_cmap(glyph)].explanation);*/
	Strcpy(buf, jtrns_obj('S', defsyms[glyph_to_cmap(glyph)].explanation));
	break;
    }
/*JP*/
    set_trns_mode(1);
}

/*
 * Look in the "data" file for more info.  Called if the user typed in the
 * whole name (user_typed_name == TRUE), or we've found a possible match
 * with a character/glyph and flags.help is TRUE.
 *
 * NOTE: when (user_typed_name == FALSE), inp is considered read-only and
 *	 must not be changed directly, e.g. via lcase(). We want to force
 *	 lcase() for data.base lookup so that we can have a clean key.
 *	 Therefore, we create a copy of inp _just_ for data.base lookup.
 */
static void
checkfile(inp, user_typed_name)
    char *inp;
    boolean user_typed_name;
{
    dlb *fp;
    char buf[BUFSZ], newstr[BUFSZ];
    char *ep, *dbase_str;
    long txt_offset;
    int chk_skip;
    boolean found_in_file = FALSE, skipping_entry = FALSE;

    fp = dlb_fopen(DATAFILE, "r");
    if (!fp) {
	pline("Cannot open data file!");
	return;
    }

    /* To prevent the need for entries in data.base like *ngel to account
     * for Angel and angel, make the lookup string the same for both
     * user_typed_name and picked name.
     */
    dbase_str = strcpy(newstr, inp);
    (void) lcase(dbase_str);

    if (!strncmp(dbase_str, "interior of ", 12))
	dbase_str += 12;
    if (!strncmp(dbase_str, "a ", 2))
	dbase_str += 2;
    else if (!strncmp(dbase_str, "an ", 3))
	dbase_str += 3;
    else if (!strncmp(dbase_str, "the ", 4))
	dbase_str += 4;
    if (!strncmp(dbase_str, "tame ", 5))
	dbase_str += 5;
    else if (!strncmp(dbase_str, "peaceful ", 9))
	dbase_str += 9;
    if (!strncmp(dbase_str, "invisible ", 10))
	dbase_str += 10;

    /* Make sure the name is non-empty. */
    if (*dbase_str) {
	/* adjust the input to remove "named " and convert to lower case */
	char *alt = 0;	/* alternate description */
	if ((ep = strstri(dbase_str, " named ")) != 0)
	    alt = ep + 7;
	else
	    ep = strstri(dbase_str, " called ");
	if (ep) *ep = '\0';

	/*
	 * If the object is named, then the name is the alternate description;
	 * otherwise, the result of makesingular() applied to the name is. This
	 * isn't strictly optimal, but named objects of interest to the user
	 * will usually be found under their name, rather than under their
	 * object type, so looking for a singular form is pointless.
	 */

	if (!alt)
	    alt = makesingular(dbase_str);
	else
	    if (user_typed_name)
		(void) lcase(alt);

	/* skip first record; read second */
	txt_offset = 0L;
	if (!dlb_fgets(buf, BUFSZ, fp) || !dlb_fgets(buf, BUFSZ, fp)) {
	    impossible("can't read 'data' file");
	    (void) dlb_fclose(fp);
	    return;
	} else if (sscanf(buf, "%8lx\n", &txt_offset) < 1 || txt_offset <= 0)
	    goto bad_data_file;

	/* look for the appropriate entry */
	while (dlb_fgets(buf,BUFSZ,fp)) {
	    if (*buf == '.') break;  /* we passed last entry without success */

	    if (digit(*buf)) {
		/* a number indicates the end of current entry */
		skipping_entry = FALSE;
	    } else if (!skipping_entry) {
		if (!(ep = index(buf, '\n'))) goto bad_data_file;
		*ep = 0;
		/* if we match a key that begins with "~", skip this entry */
		chk_skip = (*buf == '~') ? 1 : 0;
		if (pmatch(&buf[chk_skip], dbase_str) ||
			(alt && pmatch(&buf[chk_skip], alt))) {
		    if (chk_skip) {
			skipping_entry = TRUE;
			continue;
		    } else {
			found_in_file = TRUE;
			break;
		    }
		}
	    }
	}
    }

    if(found_in_file) {
	long entry_offset;
	int  entry_count;
	int  i;

	/* skip over other possible matches for the info */
	do {
	    if (!dlb_fgets(buf, BUFSZ, fp)) goto bad_data_file;
	} while (!digit(*buf));
	if (sscanf(buf, "%ld,%d\n", &entry_offset, &entry_count) < 2) {
bad_data_file:	impossible("'data' file in wrong format");
		(void) dlb_fclose(fp);
		return;
	}

/*JP	if (user_typed_name || yn("More info?") == 'y') {*/
	if (user_typed_name || yn("詳細を見る？") == 'y') {
	    winid datawin;

	    if (dlb_fseek(fp, txt_offset + entry_offset, SEEK_SET) < 0) {
		pline("? Seek error on 'data' file!");
		(void) dlb_fclose(fp);
		return;
	    }
	    datawin = create_nhwindow(NHW_MENU);
	    for (i = 0; i < entry_count; i++) {
		if (!dlb_fgets(buf, BUFSZ, fp)) goto bad_data_file;
		if ((ep = index(buf, '\n')) != 0) *ep = 0;
		if (index(buf+1, '\t') != 0) (void) tabexpand(buf+1);
		putstr(datawin, 0, buf+1);
	    }
	    display_nhwindow(datawin, FALSE);
	    destroy_nhwindow(datawin);
	}
    } else if (user_typed_name)
/*JP	pline("I don't have any information on those things.");*/
	pline("そんな名前は聞いたことがない．");

    (void) dlb_fclose(fp);
}

static int
do_look(quick)
    boolean quick;	/* use cursor && don't search for "more info" */
{
    char    out_str[BUFSZ], look_buf[BUFSZ];
/*JP*/
    char    look_buf2[BUFSZ];	
    const char *x_str, *firstmatch = 0;
/*JP*/
    const char *firstmatch2 = 0;
    int     i;
    int     sym;		/* typed symbol or converted glyph */
    int	    found;		/* count of matching syms found */
    coord   cc;			/* screen pos of unknown glyph */
    boolean save_verbose;	/* saved value of flags.verbose */
    boolean from_screen;	/* question from the screen */
    boolean need_to_look;	/* need to get explan. from glyph */
    boolean hit_trap;		/* true if found trap explanation */
    int skipped_venom = 0;	/* non-zero if we ignored "splash of venom" */
/*JP    static const char *mon_interior = "the interior of a monster";*/
    static const char *mon_interior = "怪物の内部";

    if (quick) {
	from_screen = TRUE;	/* yes, we want to use the cursor */
    } else {
/*JP	i = ynq("Specify unknown object by cursor?");*/
	i = ynq("カーソルで物体を指定する？");
	if (i == 'q') return 0;
	from_screen = (i == 'y');
    }

    if (from_screen) {
	cc.x = u.ux;
	cc.y = u.uy;
	sym = 0;		/* gcc -Wall lint */
    } else {
/*JP	getlin("Specify what? (type the word)", out_str);*/
	getlin("何を調べる？(文字を入れてね)", out_str);
	if (out_str[0] == '\0' || out_str[0] == '\033')
	    return 0;

	if (out_str[1]) {	/* user typed in a complete string */
	    checkfile(out_str, TRUE);
	    return 0;
	}
	sym = out_str[0];
    }

    /* Save the verbose flag, we change it later. */
    save_verbose = flags.verbose;
    flags.verbose = flags.verbose && !quick;
    /*
     * The user typed one letter, or we're identifying from the screen.
     */
    do {
	/* Reset some variables. */
	need_to_look = FALSE;
	found = 0;
	out_str[0] = '\0';

	if (from_screen) {
	    int glyph;	/* glyph at selected position */

	    if (flags.verbose)
/*JP		pline("Please move the cursor to an unknown object.");*/
		pline("カーソルを物体に移動してください．");
	    else
/*JP		pline("Pick an object.");*/
		pline("物体を指定してください．");

/*JP	    getpos(&cc, FALSE, "an unknown object");*/
	    getpos(&cc, FALSE, "物体");
	    if (cc.x < 0) {
		flags.verbose = save_verbose;
		return 0;	/* done */
	    }
	    flags.verbose = FALSE;	/* only print long question once */

	    /* Convert the glyph at the selected position to a symbol. */
	    glyph = glyph_at(cc.x,cc.y);
	    if (glyph_is_cmap(glyph)) {
		sym = showsyms[glyph_to_cmap(glyph)];
	    } else if (glyph_is_trap(glyph)) {
		sym = showsyms[trap_to_defsym(glyph_to_trap(glyph))];
	    } else if (glyph_is_object(glyph)) {
		sym = oc_syms[(int)objects[glyph_to_obj(glyph)].oc_class];
	    } else if (glyph_is_monster(glyph)) {
		sym = monsyms[(int)mons[glyph_to_mon(glyph)].mlet];
	    } else if (glyph_is_swallow(glyph)) {
		sym = showsyms[glyph_to_swallow(glyph)+S_sw_tl];
	    } else {
		impossible("do_look:  bad glyph %d at (%d,%d)",
						glyph, (int)cc.x, (int)cc.y);
		sym = ' ';
	    }
	}

	/*
	 * Check all the possibilities, saving all explanations in a buffer.
	 * When all have been checked then the string is printed.
	 */

	/* Check for monsters */
	for (i = 0; i < MAXMCLASSES; i++) {
	    if (sym == (from_screen ? monsyms[i] : def_monsyms[i])) {
		need_to_look = TRUE;
		if (!found) {
/*JP		    Sprintf(out_str, "%c       %s", sym, an(monexplain[i]));*/
		    Sprintf(out_str, "%c       %s", sym, jtrns_obj('S', monexplain[i]));
/*JP		    firstmatch = monexplain[i];*/
		    firstmatch = jtrns_obj('S',monexplain[i]);
		    firstmatch2 = monexplain[i];
		    found++;
		} else {
/*JP		    found += append_str(out_str, an(monexplain[i]));*/
		    found += append_str(out_str, jtrns_obj('S',monexplain[i]));
		}
	    }
	}

	/*
	 * Special case: if identifying from the screen, and we're swallowed,
	 * and looking at something other than our own symbol, then just say
	 * "the interior of a monster".
	 */
	if (u.uswallow && from_screen && is_swallow_sym(sym)) {
	    if (!found) {
		Sprintf(out_str, "%c       %s", sym, mon_interior);
		firstmatch = mon_interior;
		firstmatch2 = "the interior of a monster";
	    } else {
		found += append_str(out_str, mon_interior);
	    }
	    need_to_look = TRUE;
	}

	/* Now check for objects */
	for (i = 1; i < MAXOCLASSES; i++) {
	    if (sym == (from_screen ? oc_syms[i] : def_oc_syms[i])) {
		need_to_look = TRUE;
		if (from_screen && i == VENOM_CLASS) {
		    skipped_venom++;
		    continue;
		}
		if (!found) {
/*JP		    Sprintf(out_str, "%c       %s", sym, an(objexplain[i]));*/
		    Sprintf(out_str, "%c       %s", sym, jtrns_obj('S',objexplain[i]));
/*JP		    firstmatch = objexplain[i];*/
		    firstmatch = jtrns_obj('S',objexplain[i]);
		    firstmatch2 = objexplain[i];
		    found++;
		} else {
/*JP		    found += append_str(out_str, an(objexplain[i]));*/
		    found += append_str(out_str, jtrns_obj('S',objexplain[i]));
		}
	    }
	}

#define is_cmap_trap(i) ((i) >= S_arrow_trap && (i) <= S_polymorph_trap)
#define is_cmap_drawbridge(i) ((i) >= S_vodbridge && (i) <= S_hcdbridge)

	/* Now check for graphics symbols */
	for (hit_trap = FALSE, i = 0; i < MAXPCHARS; i++) {
	    x_str = defsyms[i].explanation;
	    if (sym == (from_screen ? showsyms[i] : defsyms[i].sym) && *x_str) {
		/* avoid "an air", "a water", or "a floor of a room" */
#if 0	/*JP*/
		int article = (i == S_room) ? 2 :		/* 2=>"the" */
			      !(strcmp(x_str, "air") == 0 ||	/* 1=>"an"  */
				strcmp(x_str, "water") == 0);	/* 0=>(none)*/
#endif

		if (!found) {
		    if (is_cmap_trap(i)) {
/*JP			Sprintf(out_str, "%c       a trap", sym);*/
			Sprintf(out_str, "%c       罠", sym);
			hit_trap = TRUE;
		    } else {
/*JP			Sprintf(out_str, "%c       %s", sym,
				article == 2 ? the(x_str) :
				article == 1 ? an(x_str) : x_str);*/
			Sprintf(out_str, "%c       %s", sym,
				jtrns_obj('S', x_str));
		    }
		    firstmatch = jtrns_obj('S', x_str);
		    firstmatch2 = x_str;
		    found++;
		} else if (!u.uswallow && !(hit_trap && is_cmap_trap(i)) &&
			   !(found >= 3 && is_cmap_drawbridge(i))) {
/*JP		    found += append_str(out_str,
					article == 2 ? the(x_str) :
					article == 1 ? an(x_str) : x_str);*/
		    found += append_str(out_str, jtrns_obj('S', x_str));
		    if (is_cmap_trap(i)) hit_trap = TRUE;
		}

		if (i == S_altar || is_cmap_trap(i))
		    need_to_look = TRUE;
	    }
	}

	/* if we ignored venom and list turned out to be short, put it back */
	if (skipped_venom && found < 2) {
	    x_str = objexplain[VENOM_CLASS];
	    if (!found) {
/*JP		Sprintf(out_str, "%c       %s", sym, an(x_str));
		firstmatch = x_str;*/
		Sprintf(out_str, "%c       %s", sym, jtrns_obj('S', (x_str)));
		firstmatch = jtrns_obj('S', x_str);
		firstmatch2 = x_str;
		found++;
	    } else {
/*JP		found += append_str(out_str, an(x_str));*/
		found += append_str(out_str, jtrns_obj('S', x_str));
	    }
	}

	/*
	 * If we are looking at the screen, follow multiple posibilities or
	 * an ambigious explanation by something more detailed.
	 */
	if (from_screen) {
	    if (found > 1 || need_to_look) {
/*JP		lookat(cc.x, cc.y, look_buf);*/
		lookat(cc.x, cc.y, look_buf, look_buf2);
		firstmatch = look_buf;
		firstmatch2 = look_buf2;
		if (*firstmatch) {
		    char temp_buf[BUFSZ];
		    Sprintf(temp_buf, " (%s)", firstmatch);
		    (void)strncat(out_str, temp_buf, BUFSZ-strlen(out_str)-1);
		    found = 1;	/* we have something to look up */
		}
	    }
	}

	/* Finally, print out our explanation. */
	if (found) {
	    pline("%s", out_str);
	    /* check the data file for information about this thing */
	    if (found == 1 && !quick && flags.help) {
		char temp_buf[BUFSZ];
/*JP		Strcpy(temp_buf, firstmatch);*/
		Strcpy(temp_buf, firstmatch2);
		checkfile(temp_buf, FALSE);
	    }
	} else {
/*JP	    pline("I've never heard of such things.");*/
	    pline("そんな名前は聞いたことがない．");
	}

    } while (from_screen && !quick);

    flags.verbose = save_verbose;
    return 0;
}


int
dowhatis()
{
	return do_look(FALSE);
}

int
doquickwhatis()
{
	return do_look(TRUE);
}

int
doidtrap()
{
	register struct trap *trap;
	int x, y, tt;

	if (!getdir((char *)0)) return 0;
	x = u.ux + u.dx;
	y = u.uy + u.dy;
	for (trap = ftrap; trap; trap = trap->ntrap)
	    if (trap->tx == x && trap->ty == y) {
		if (!trap->tseen) break;
		tt = trap->ttyp;
		if (u.dz) {
		    if (u.dz < 0 ? (tt == TRAPDOOR || tt == HOLE) :
			    tt == ROCKTRAP) break;
		}
		if (Hallucination) tt = rn1(TRAPNUM-3, 3);
#if 0 /*JP*/
		pline("That is %s%s%s.",
		      an(defsyms[trap_to_defsym(tt)].explanation),
		      !trap->madeby_u ? "" : (tt == WEB) ? " woven" :
			  /* trap doors & spiked pits can't be made by
			     player, and should be considered at least
			     as much "set" as "dug" anyway */
			  (tt == HOLE || tt == PIT) ? " dug" : " set",
		      !trap->madeby_u ? "" : " by you");
#endif
		pline("それは%s%sだ．",
		      !trap->madeby_u ? "" : (tt == WEB) ? "あなたが張った" :
			  (tt == HOLE || tt == PIT) ? "あなたが掘った" : "あなたが仕掛けた",
		      jtrns_obj('^', defsyms[trap_to_defsym(tt)].explanation));
		return 0;
	    }
/*JP	pline("I can't see a trap there.");*/
	pline("そこには罠はない．");
	return 0;
}

int
dowhatdoes()
{
	dlb *fp;
	char bufr[BUFSZ+6];
	register char *buf = &bufr[6], *ep, q, ctrl, meta;

	fp = dlb_fopen(CMDHELPFILE, "r");
	if (!fp) {
		pline("Cannot open data file!");
		return 0;
	}

#if defined(UNIX) || defined(VMS)
	introff();
#endif
/*JP	q = yn_function("What command?", (char *)0, '\0');*/
	q = yn_function("どういうコマンド？", (char *)0, '\0');
#if defined(UNIX) || defined(VMS)
	intron();
#endif
	ctrl = ((q <= '\033') ? (q - 1 + 'A') : 0);
	meta = ((0x80 & q) ? (0x7f & q) : 0);
	while(dlb_fgets(buf,BUFSZ,fp))
	    if ((ctrl && *buf=='^' && *(buf+1)==ctrl) ||
		(meta && *buf=='M' && *(buf+1)=='-' && *(buf+2)==meta) ||
		*buf==q) {
		ep = index(buf, '\n');
		if(ep) *ep = 0;
		if (ctrl && buf[2] == '\t'){
			buf = bufr + 1;
			(void) strncpy(buf, "^?      ", 8);
			buf[1] = ctrl;
		} else if (meta && buf[3] == '\t'){
			buf = bufr + 2;
			(void) strncpy(buf, "M-?     ", 8);
			buf[2] = meta;
		} else if(buf[1] == '\t'){
			buf = bufr;
			buf[0] = q;
			(void) strncpy(buf+1, "       ", 7);
		}
		pline("%s", buf);
		(void) dlb_fclose(fp);
		return 0;
	    }
/*JP	pline("I've never heard of such commands.");*/
	pline("そんなコマンドは知らない．");
	(void) dlb_fclose(fp);
	return 0;
}

/* data for help_menu() */
static const char *help_menu_items[] = {
#if 0 /*JP*/
/* 0*/	"Long description of the game and commands.",
/* 1*/	"List of game commands.",
/* 2*/	"Concise history of NetHack.",
/* 3*/	"Info on a character in the game display.",
/* 4*/	"Info on what a given key does.",
/* 5*/	"List of game options.",
/* 6*/	"Longer explanation of game options.",
/* 7*/	"List of extended commands.",
/* 8*/	"The NetHack license.",
#ifdef PORT_HELP
	"%s-specific help and commands.",
#define PORT_HELP_ID 100
#define WIZHLP_SLOT 10
#else
#define WIZHLP_SLOT 9
#endif
#ifdef WIZARD
	"List of wizard-mode commands.",
#endif
#endif
/* 0*/	"ゲームおよびコマンドの解説．(長文)",
/* 1*/	"コマンド一覧．",
/* 2*/	"NetHackの簡単な歴史．",
/*	"JNetHackの簡単な歴史．",*/
/* 3*/	"画面に表示される文字の説明．",
/* 4*/	"このキーが何を意味するかの説明．",
/* 5*/	"ゲームのオプション一覧．",
/* 6*/	"ゲームのオプション一覧．(長文)",
/* 7*/	"拡張コマンド一覧．",
/* 8*/	"NetHackのライセンス．",
/* 9*/	"英名-和名の表．",
#ifdef PORT_HELP
	"%sに分類されるヘルプおよびコマンド．",
#define PORT_HELP_ID 100
#define WIZHLP_SLOT 11
#else
#define WIZHLP_SLOT 10
#endif
#ifdef WIZARD
	"ウィザードモードのコマンド一覧．",
#endif
	"",
	(char *)0
};

static boolean
help_menu(sel)
	int *sel;
{
	winid tmpwin = create_nhwindow(NHW_MENU);
#ifdef PORT_HELP
	char helpbuf[QBUFSZ];
#endif
	int i, n;
	menu_item *selected;
	anything any;

	any.a_void = 0;		/* zero all bits */
	start_menu(tmpwin);
#ifdef WIZARD
	if (!wizard) help_menu_items[WIZHLP_SLOT] = "",
		     help_menu_items[WIZHLP_SLOT+1] = (char *)0;
#endif
	for (i = 0; help_menu_items[i]; i++)
#ifdef PORT_HELP
	    /* port-specific line has a %s in it for the PORT_ID */
	    if (help_menu_items[i][0] == '%') {
		Sprintf(helpbuf, help_menu_items[i], PORT_ID);
		any.a_int = PORT_HELP_ID + 1;
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
			 helpbuf, MENU_UNSELECTED);
	    } else
#endif
	    {
		any.a_int = (*help_menu_items[i]) ? i+1 : 0;
		add_menu(tmpwin, NO_GLYPH, &any, 0, 0,
			ATR_NONE, help_menu_items[i], MENU_UNSELECTED);
	    }
/*JP	end_menu(tmpwin, "Select one item:");*/
	end_menu(tmpwin, "選んでください：");
	n = select_menu(tmpwin, PICK_ONE, &selected);
	destroy_nhwindow(tmpwin);
	if (n > 0) {
	    *sel = selected[0].item.a_int - 1;
	    free((genericptr_t)selected);
	    return TRUE;
	}
	return FALSE;
}

int
dohelp()
{
	int sel = 0;

	if (help_menu(&sel)) {
		switch (sel) {
			case  0:  display_file(HELP, TRUE);  break;
			case  1:  display_file(SHELP, TRUE);  break;
			case  2:  (void) dohistory();  break;
			case  3:  (void) dowhatis();  break;
			case  4:  (void) dowhatdoes();  break;
			case  5:  option_help();  break;
			case  6:  display_file(OPTIONFILE, TRUE);  break;
			case  7:  (void) doextlist();  break;
			case  8:  display_file(LICENSE, TRUE);  break;
/*JP*/
			case  9:  display_file(JJJ, TRUE);  break;
#ifdef WIZARD
			/* handle slot 9 or 10 */
			default: display_file(DEBUGHELP, TRUE);  break;
#endif
#ifdef PORT_HELP
			case PORT_HELP_ID:  port_help();  break;
#endif
		}
	}
	return 0;
}

int
dohistory()
{
	display_file(HISTORY, TRUE);
	return 0;
}

/*pager.c*/
