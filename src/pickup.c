/*	SCCS Id: @(#)pickup.c	3.2	96/07/19	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994-2000
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

/*
 *	Contains code for picking objects up, and container use.
 */

#include "hack.h"

static void FDECL(simple_look, (struct obj *,BOOLEAN_P));
static boolean FDECL(query_classes, (char *,boolean *,boolean *,
		const char *,struct obj *,BOOLEAN_P,BOOLEAN_P,int *));
static void FDECL(check_here, (BOOLEAN_P));
static boolean FDECL(n_or_more, (struct obj *));
static boolean FDECL(all_but_uchain, (struct obj *));
#if 0 /* not used */
static boolean FDECL(allow_cat_no_uchain, (struct obj *));
#endif
static int FDECL(autopick, (struct obj*, int, menu_item **));
static int FDECL(count_categories, (struct obj *,int));
static long FDECL(carry_count,
		      (struct obj *,struct obj *,long,BOOLEAN_P,int *,int *));
static int FDECL(lift_object, (struct obj *,struct obj *,long *,BOOLEAN_P));
static boolean FDECL(mbag_explodes, (struct obj *,int));
STATIC_PTR int FDECL(in_container,(struct obj *));
STATIC_PTR int FDECL(ck_bag,(struct obj *));
STATIC_PTR int FDECL(out_container,(struct obj *));
static int FDECL(menu_loot, (int, struct obj *, BOOLEAN_P));
static int FDECL(in_or_out_menu, (const char *,struct obj *));

/* define for query_objlist() and autopickup() */
#define FOLLOW(curr, flags) \
    (((flags) & BY_NEXTHERE) ? (curr)->nexthere : (curr)->nobj)

/*
 *  How much the weight of the given container will change when the given
 *  object is removed from it.  This calculation must match the one used
 *  by weight() in mkobj.c.
 */
#define DELTA_CWT(cont,obj)		\
    ((cont)->cursed ? (obj)->owt * 2 :	\
		      1 + ((obj)->owt / ((cont)->blessed ? 4 : 2)))
#define GOLD_WT(n)		(((n) + 50L) / 100L)
/* if you can figure this out, give yourself a hearty pat on the back... */
#define GOLD_CAPACITY(w,n)	(((w) * -100L) - ((n) + 50L) - 1L)

#if 0 /*JP*/
static const char moderateloadmsg[] = "You have a little trouble lifting";
static const char nearloadmsg[] = "You have much trouble lifting";
static const char overloadmsg[] = "You have extreme difficulty lifting";
#endif

static const char moderateloadmsg[] = "を持ったら少しふらついた．";
static const char nearloadmsg[] = "はずっしりと肩にのしかかった．";
static const char overloadmsg[] = "を持ちあげるのはとてもつらい．";

/* BUG: this lets you look at cockatrice corpses while blind without
   touching them */
/* much simpler version of the look-here code; used by query_classes() */
static void
simple_look(otmp, here)
struct obj *otmp;	/* list of objects */
boolean here;		/* flag for type of obj list linkage */
{
	/* Neither of the first two cases is expected to happen, since
	 * we're only called after multiple classes of objects have been
	 * detected, hence multiple objects must be present.
	 */
	if (!otmp) {
	    impossible("simple_look(null)");
	} else if (!(here ? otmp->nexthere : otmp->nobj)) {
	    pline("%s", doname(otmp));
	} else {
	    winid tmpwin = create_nhwindow(NHW_MENU);
	    putstr(tmpwin, 0, "");
	    do {
		putstr(tmpwin, 0, doname(otmp));
		otmp = here ? otmp->nexthere : otmp->nobj;
	    } while (otmp);
	    display_nhwindow(tmpwin, TRUE);
	    destroy_nhwindow(tmpwin);
	}
}

int
collect_obj_classes(ilets, otmp, here, incl_gold, filter)
char ilets[];
register struct obj *otmp;
boolean here, incl_gold;
boolean FDECL((*filter),(OBJ_P));
{
	register int iletct = 0;
	register char c;

	if (incl_gold)
	    ilets[iletct++] = def_oc_syms[GOLD_CLASS];
	ilets[iletct] = '\0'; /* terminate ilets so that index() will work */
	while (otmp) {
	    c = def_oc_syms[(int)otmp->oclass];
	    if (!index(ilets, c) && (!filter || (*filter)(otmp)))
		ilets[iletct++] = c,  ilets[iletct] = '\0';
	    otmp = here ? otmp->nexthere : otmp->nobj;
	}

	return iletct;
}

/*
 * Suppose some '?' and '!' objects are present, but '/' objects aren't:
 *	"a" picks all items without further prompting;
 *	"A" steps through all items, asking one by one;
 *	"?" steps through '?' items, asking, and ignores '!' ones;
 *	"/" becomes 'A', since no '/' present;
 *	"?a" or "a?" picks all '?' without further prompting;
 *	"/a" or "a/" becomes 'A' since there aren't any '/'
 *	    (bug fix:  3.1.0 thru 3.1.3 treated it as "a");
 *	"?/a" or "a?/" or "/a?",&c picks all '?' even though no '/'
 *	    (ie, treated as if it had just been "?a").
 */
static boolean
query_classes(oclasses, one_at_a_time, everything, action, objs,
	      here, incl_gold, menu_on_demand)
char oclasses[];
boolean *one_at_a_time, *everything;
const char *action;
struct obj *objs;
boolean here, incl_gold;
int *menu_on_demand;
{
	char ilets[20], inbuf[BUFSZ];
	int iletct, oclassct;
	boolean not_everything;
	char qbuf[QBUFSZ];
	boolean m_seen;

	oclasses[oclassct = 0] = '\0';
	*one_at_a_time = *everything = m_seen = FALSE;
	iletct = collect_obj_classes(ilets, objs, here, incl_gold,
				     (boolean FDECL((*),(OBJ_P))) 0);
	if (iletct == 0) {
		return FALSE;
	} else if (iletct == 1) {
		oclasses[0] = def_char_to_objclass(ilets[0]);
		oclasses[1] = '\0';
	} else  {	/* more than one choice available */
		const char *where = 0;
		register char sym, oc_of_sym, *p;
		/* additional choices */
		ilets[iletct++] = ' ';
		ilets[iletct++] = 'a';
		ilets[iletct++] = 'A';
		ilets[iletct++] = (objs == invent ? 'i' : ':');
		if (menu_on_demand) {
			ilets[iletct++] = 'm';
			*menu_on_demand = 0;
		}
		ilets[iletct] = '\0';
ask_again:
		oclasses[oclassct = 0] = '\0';
		*one_at_a_time = *everything = FALSE;
		not_everything = FALSE;
/*JP
		Sprintf(qbuf,"What kinds of thing do you want to %s? [%s]",
			action, ilets);
*/
		Sprintf(qbuf,"どの種類のものを%sか？[%s]",
			jconj(action,"ます"), ilets);
		getlin(qbuf,inbuf);
		if (*inbuf == '\033') return FALSE;

		for (p = inbuf; (sym = *p++); ) {
		    /* new A function (selective all) added by GAN 01/09/87 */
		    if (sym == ' ') continue;
		    else if (sym == 'A') *one_at_a_time = TRUE;
		    else if (sym == 'a') *everything = TRUE;
		    else if (sym == ':') {
			simple_look(objs, here);  /* dumb if objs==invent */
			goto ask_again;
		    } else if (sym == 'i') {
			(void) display_inventory((char *)0, TRUE);
			goto ask_again;
		    } else if (sym == 'm') {
			m_seen = TRUE;
		    } else {
			oc_of_sym = def_char_to_objclass(sym);
			if (index(ilets,sym)) {
			    add_valid_menu_class(oc_of_sym);
			    oclasses[oclassct++] = oc_of_sym;
			    oclasses[oclassct] = '\0';
			} else {
#if 0 /*JP*/
			    if (!where)
				where = !strcmp(action,"pick up")  ? "here" :
					!strcmp(action,"take out") ?
							    "inside" : "";
			    if (*where)
				pline("There are no %c's %s.", sym, where);
			    else
				You("have no %c's.", sym);
#endif /*JP*/
			    if (!where)
			        where = (!strcmp(action,"pick up")||!strcmp(action,"拾う")) ? "ここ" :
					(!strcmp(action,"take out")||!strcmp(action,"取り出す")) ?
							    "の中" : "";
			    if (*where)
				pline("%cは%sにない．", sym, where);
			    else
				You("%cは持っていない．", sym);
			    not_everything = TRUE;
			}
		    }
		}
		if (m_seen && menu_on_demand) {
			*menu_on_demand = (*everything || !oclassct) ? -2 : -3;
			return FALSE;
		}
		if (!oclassct && (!*everything || not_everything)) {
		    /* didn't pick anything,
		       or tried to pick something that's not present */
		    *one_at_a_time = TRUE;	/* force 'A' */
		    *everything = FALSE;	/* inhibit 'a' */
		}
	}
	return TRUE;
}

/* look at the objects at our location, unless there are too many of them */
static void
check_here(picked_some)
boolean picked_some;
{
	register struct obj *obj;
	register int ct = 0;

	/* count the objects here */
	for (obj = level.objects[u.ux][u.uy]; obj; obj = obj->nexthere) {
	    if (obj != uchain)
		ct++;
	}

	/* If there are objects here, take a look. */
	if (ct) {
	    if (flags.run) nomul(0);
	    flush_screen(1);
	    if (ct < 5) {
		(void) dolook();
	    } else {
		read_engr_at(u.ux,u.uy);
/*JP
		pline("There are several %sobjects here.",
		      picked_some ? "more " : "");
*/
		pline("ここには%sいくつかものがある．",
		      picked_some ? "さらに" : "");
	    }
	} else {
	    read_engr_at(u.ux,u.uy);
	}
}

/* Value set by query_objlist() for n_or_more(). */
static long val_for_n_or_more;

/* query_objlist callback: return TRUE if obj's count is >= reference value */
static boolean
n_or_more(obj)
struct obj *obj;
{
    if (obj == uchain) return FALSE;
    return (obj->quan >= val_for_n_or_more);
}

/* List of valid menu classes for query_objlist() and allow_category callback */
static char valid_menu_classes[MAXOCLASSES + 2];

void
add_valid_menu_class(c)
int c;
{
	static int vmc_count = 0;

	if (c == 0)  /* reset */
	  vmc_count = 0;
	else
	  valid_menu_classes[vmc_count++] = (char)c;
	valid_menu_classes[vmc_count] = '\0';
}

/* query_objlist callback: return TRUE if not uchain */
static boolean
all_but_uchain(obj)
struct obj *obj;
{
    return (obj != uchain);
}

/* query_objlist callback: return TRUE */
/*ARGSUSED*/
boolean
allow_all(obj)
struct obj *obj;
{
    return TRUE;
}

boolean
allow_category(obj)
struct obj *obj;
{
    if (((index(valid_menu_classes,'u') != (char *)0) && obj->unpaid) ||
	(index(valid_menu_classes, obj->oclass) != (char *)0))
	return TRUE;
    else
	return FALSE;
}

#if 0 /* not used */
/* query_objlist callback: return TRUE if valid category (class), no uchain */
static boolean
allow_cat_no_uchain(obj)
struct obj *obj;
{
    if ((obj != uchain) &&
	(((index(valid_menu_classes,'u') != (char *)0) && obj->unpaid) ||
	(index(valid_menu_classes, obj->oclass) != (char *)0)))
	return TRUE;
    else
	return FALSE;
}
#endif

/* query_objlist callback: return TRUE if valid class and worn */
boolean
is_worn_by_type(otmp)
register struct obj *otmp;
{
	return((boolean)(!!(otmp->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_WEP)))
	        && (index(valid_menu_classes, otmp->oclass) != (char *)0));
}

/*
 * Have the hero pick things from the ground.
 *
 * Arg what:
 *	>0  autopickup
 *	=0  interactive
 *	<0  pickup count of something
 */
void
pickup(what)
int what;		/* should be a long */
{
	int i, n, res, count, n_picked = 0;
	menu_item *pick_list = (menu_item *) 0;
	boolean autopickup = what > 0;

	if (what < 0)		/* pick N of something */
	    count = -what;
	else			/* pick anything */
	    count = 0;

	/* no auto-pick if no-pick move, nothing there, or in a pool */
	if (autopickup && (flags.nopick || !OBJ_AT(u.ux, u.uy) ||
			(is_pool(u.ux, u.uy) && !Underwater))) {
	    read_engr_at(u.ux, u.uy);
	    return;
	}

	/* no pickup if levitating & not on air or water level */
	if (!can_reach_floor()) {
	    if ((multi && !flags.run) || (autopickup && !flags.pickup))
		read_engr_at(u.ux, u.uy);
	    return;
	}

	/* multi && !flags.run means they are in the middle of some other
	 * action, or possibly paralyzed, sleeping, etc.... and they just
	 * teleported onto the object.  They shouldn't pick it up.
	 */
	if ((multi && !flags.run) || (autopickup && !flags.pickup)) {
	    check_here(FALSE);
	    return;
	}

	/* if there's anything here, stop running */
	if (OBJ_AT(u.ux,u.uy) && flags.run && !flags.nopick) nomul(0);

	add_valid_menu_class(0);	/* reset */
	/*
	 * Start the actual pickup process.  This is split into two main
	 * sections, the newer menu and the older "traditional" methods.
	 * Automatic pickup has been split into its own menu-style routine
	 * to make things less confusing.
	 */
	if (autopickup) {
	    n = autopick(level.objects[u.ux][u.uy], BY_NEXTHERE, &pick_list);
	    goto menu_pickup;
	}
#ifdef NEWBIE
	newbie.pickup = 1;
#endif

	if (flags.menu_style != MENU_TRADITIONAL) {
	    /* use menus exclusively */

	    if (count) {	/* looking for N of something */
		char buf[QBUFSZ];
/*JP		Sprintf(buf, "Pick %d of what?", count);*/
		Sprintf(buf, "何を%d個拾いますか？", count);
		val_for_n_or_more = count;	/* set up callback selector */
		n = query_objlist(buf, level.objects[u.ux][u.uy],
			    BY_NEXTHERE|AUTOSELECT_SINGLE|INVORDER_SORT,
			    &pick_list, PICK_ONE, n_or_more);
		/* correct counts, if any given */
		for (i = 0; i < n; i++)
		    pick_list[i].count = count;
	    } else {
/*JP		n = query_objlist("Pick up what?", level.objects[u.ux][u.uy],*/
		n = query_objlist("何を拾いますか？", level.objects[u.ux][u.uy],
			    BY_NEXTHERE|AUTOSELECT_SINGLE|INVORDER_SORT,
			    &pick_list, PICK_ANY, all_but_uchain);
	    }
menu_pickup:
	    for (n_picked = i = 0 ; i < n; i++) {
		res = pickup_object(pick_list[i].item.a_obj,pick_list[i].count,
					FALSE);
		if (res < 0) break;	/* can't continue */
		n_picked += res;
	    }
	    if (pick_list) free((genericptr_t)pick_list);

	} else {
	    /* old style interface */
	    int ct = 0;
	    long lcount;
	    boolean all_of_a_type, selective;
	    char oclasses[MAXOCLASSES];
	    struct obj *obj, *obj2;

	    oclasses[0] = '\0';		/* types to consider (empty for all) */
	    all_of_a_type = TRUE;	/* take all of considered types */
	    selective = FALSE;		/* ask for each item */

	    /* check for more than one object */
	    for (obj = level.objects[u.ux][u.uy]; obj; obj = obj->nexthere)
		ct++;

	    if (ct == 1 && count) {
		/* if only one thing, then pick it */
		obj = level.objects[u.ux][u.uy];
		lcount = min(obj->quan, (long)count);
		if (pickup_object(obj, lcount, FALSE) > 0)
		    n_picked++;	/* picked something */
		goto end_query;

	    } else if (ct >= 2) {
		int via_menu = 0;

/*JP		pline("There are several objects here.");*/
		pline("ここにはいくつかものがある．");
		if (!query_classes(oclasses, &selective, &all_of_a_type,
/*JP				   "pick up", level.objects[u.ux][u.uy],*/
				   "拾う", level.objects[u.ux][u.uy],
				   TRUE, FALSE, &via_menu)) {
		    if (!via_menu) return;
/*JP		    n = query_objlist("Pick up what?",*/
		    n = query_objlist("何を拾いますか？",
				  level.objects[u.ux][u.uy],
				  BY_NEXTHERE|(selective ? 0 : INVORDER_SORT),
				  &pick_list, PICK_ANY,
				  via_menu == -2 ? allow_all : allow_category);
		    goto menu_pickup;
		}
	    }

	    for (obj = level.objects[u.ux][u.uy]; obj; obj = obj2) {
		obj2 = obj->nexthere;	/* perhaps obj will be picked up */
		lcount = -1L;

		if (!selective && oclasses[0] && !index(oclasses,obj->oclass))
		    continue;

		if (!all_of_a_type) {
		    char qbuf[QBUFSZ];
/*JP		    Sprintf(qbuf, "Pick up %s?", doname(obj));*/
		    Sprintf(qbuf, "%sを拾いますか？", doname(obj));
		    switch ((obj->quan < 2L) ? ynaq(qbuf) : ynNaq(qbuf)) {
		    case 'q': goto end_query;	/* out 2 levels */
		    case 'n': continue;
		    case 'a':
			all_of_a_type = TRUE;
			if (selective) {
			    selective = FALSE;
			    oclasses[0] = obj->oclass;
			    oclasses[1] = '\0';
			}
			break;
		    case '#':	/* count was entered */
			if (!yn_number) continue; /* 0 count => No */
			lcount = (long) yn_number;
			if (lcount > obj->quan) lcount = obj->quan;
			/* fall thru */
		    default:	/* 'y' */
			break;
		    }
		}
		if (lcount == -1L) lcount = obj->quan;

		if ((res = pickup_object(obj, lcount, FALSE)) < 0) break;
		n_picked += res;
	    }
end_query:
	    ;	/* semicolon needed by brain-damaged compilers */
	}

	/* position may need updating (invisible hero) */
	if (n_picked) newsym(u.ux,u.uy);

	/* see whether there's anything else here, after auto-pickup is done */
	if (autopickup) check_here(n_picked > 0);
}

/*
 * Pick from the given list using flags.pickup_types.  Return the number
 * of items picked (not counts).  Create an array that returns pointers
 * and counts of the items to be picked up.  If the number of items
 * picked is zero, the pickup list is left alone.  The caller of this
 * function must free the pickup list.
 */
static int
autopick(olist, follow, pick_list)
struct obj *olist;	/* the object list */
int follow;		/* how to follow the object list */
menu_item **pick_list;	/* list of objects and counts to pick up */
{
	menu_item *pi;	/* pick item */
	struct obj *curr;
	int n;
	const char *otypes = flags.pickup_types;

	/* first count the number of eligible items */
	for (n = 0, curr = olist; curr; curr = FOLLOW(curr, follow))
	    if (!*otypes || index(otypes, curr->oclass))
		n++;

	if (n) {
	    *pick_list = pi = (menu_item *) alloc(sizeof(menu_item) * n);
	    for (n = 0, curr = olist; curr; curr = FOLLOW(curr, follow))
		if (!*otypes || index(otypes, curr->oclass)) {
		    pi[n].item.a_obj = curr;
		    pi[n].count = curr->quan;
		    n++;
		}
	}
	return n;
}


/*
 * Put up a menu using the given object list.  Only those objects on the
 * list that meet the approval of the allow function are displayed.  Return
 * a count of the number of items selected, as well as an allocated array of
 * menu_items, containing pointers to the objects selected and counts.  The
 * returned counts are guaranteed to be in bounds and non-zero.
 *
 * Query flags:
 *	BY_NEXTHERE	  - Follow object list via nexthere instead of nobj.
 *	AUTOSELECT_SINGLE - Don't ask if only 1 object qualifies - just
 *			    use it.
 *	USE_INVLET	  - Use object's invlet.
 *	INVORDER_SORT	  - Use hero's pack order.
 *	SIGNAL_NOMENU	  - Return -1 rather than 0 if nothing passes "allow".
 */
int
query_objlist(qstr, olist, qflags, pick_list, how, allow)
const char *qstr;		/* query string */
struct obj *olist;		/* the list to pick from */
int qflags;			/* options to control the query */
menu_item **pick_list;		/* return list of items picked */
int how;			/* type of query */
boolean FDECL((*allow), (OBJ_P));/* allow function */
{
	int n;
	winid win;
	struct obj *curr, *last;
	char *pack;
	anything any;
	boolean printed_type_name;

	*pick_list = (menu_item *) 0;
	if (!olist) return 0;

	/* count the number of items allowed */
	for (n = 0, last = 0, curr = olist; curr; curr = FOLLOW(curr, qflags))
	    if ((*allow)(curr)) {
		last = curr;
		n++;
	    }

	if (n == 0)	/* nothing to pick here */
	    return (qflags & SIGNAL_NOMENU) ? -1 : 0;

	if (n == 1 && (qflags & AUTOSELECT_SINGLE)) {
	    *pick_list = (menu_item *) alloc(sizeof(menu_item));
	    (*pick_list)->item.a_obj = last;
	    (*pick_list)->count = last->quan;
	    return 1;
	}

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	any.a_obj = (struct obj *) 0;

	/*
	 * Run through the list and add the objects to the menu.  If
	 * INVORDER_SORT is set, we'll run through the list once for
	 * each type so we can group them.  The allow function will only
	 * be called once per object in the list.
	 */
	pack = flags.inv_order;
	do {
	    printed_type_name = FALSE;
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags))
		if ((!(qflags & INVORDER_SORT) || curr->oclass == *pack)
							&& (*allow)(curr)) {

		    /* if sorting, print type name (once only) */
		    if (qflags & INVORDER_SORT && !printed_type_name) {
			any.a_obj = (struct obj *) 0;
			add_menu(win, NO_GLYPH, &any, 0, 0, ATR_INVERSE,
					let_to_name(*pack, FALSE), MENU_UNSELECTED);
			printed_type_name = TRUE;
		    }

		    any.a_obj = curr;
		    add_menu(win, obj_to_glyph(curr), &any,
			    qflags & USE_INVLET ? curr->invlet : 0,
			    def_oc_syms[(int)objects[curr->otyp].oc_class],
			    ATR_NONE, doname(curr), MENU_UNSELECTED);
		}
	    pack++;
	} while (qflags & INVORDER_SORT && *pack);

	end_menu(win, qstr);
	n = select_menu(win, how, pick_list);
	destroy_nhwindow(win);

	if (n > 0) {
	    menu_item *mi;
	    int i;

	    /* fix up counts:  -1 means no count used => pick all */
	    for (i = 0, mi = *pick_list; i < n; i++, mi++)
		if (mi->count == -1L || mi->count > mi->item.a_obj->quan)
		    mi->count = mi->item.a_obj->quan;
	} else if (n < 0) {
	    n = 0;	/* caller's don't expect -1 */
	}
	return n;
}

/*
 * allow menu-based category (class) selection (for Drop,take off etc.)
 *
 */
int
query_category(qstr, olist, qflags, pick_list, how)
const char *qstr;		/* query string */
struct obj *olist;		/* the list to pick from */
int qflags;			/* behaviour modification flags */
menu_item **pick_list;		/* return list of items picked */
int how;			/* type of query */
{
	int n;
	winid win;
	struct obj *curr;
	char *pack;
	anything any;
	boolean collected_type_name;
	char invlet;
	int ccount;
	boolean do_unpaid = FALSE;

	*pick_list = (menu_item *) 0;
	if (!olist) return 0;
	if ((qflags & UNPAID_TYPES) && count_unpaid(olist)) do_unpaid = TRUE;

	ccount = count_categories(olist, qflags);
	/* no point in actually showing a menu for a single category */
	if (ccount == 1 && !do_unpaid && !(qflags & BILLED_TYPES)) {
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if ((qflags & WORN_TYPES) &&
		    !(curr->owornmask & (W_ARMOR|W_RING|W_AMUL|W_TOOL|W_WEP)))
		    continue;
		break;
	    }
	    if (curr) {
		*pick_list = (menu_item *) alloc(sizeof(menu_item));
		(*pick_list)->item.a_int = curr->oclass;
		return 1;
	    } else {
#ifdef DEBUG
		impossible("query_category: no single object match");
#endif
	    }
	    return 0;
	}

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	pack = flags.inv_order;
	if ((qflags & ALL_TYPES) && (ccount > 1)) {
		invlet = 'a';
		any.a_void = 0;
		any.a_int = ALL_TYPES_SELECTED;
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
/*JP		       (qflags & WORN_TYPES) ? "All worn types" : "All types",*/
		       (qflags & WORN_TYPES) ? "身につけるもの全て" : "全て",
			MENU_UNSELECTED);
		invlet = 'b';
	} else
		invlet = 'a';
	do {
	    collected_type_name = FALSE;
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if (curr->oclass == *pack) {
		   if ((qflags & WORN_TYPES) &&
	!(curr->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_WEP)))
			 continue;
		   if (!collected_type_name) {
			any.a_void = 0;
			any.a_int = curr->oclass;
			add_menu(win, NO_GLYPH, &any, invlet++,
				def_oc_syms[(int)objects[curr->otyp].oc_class],
				ATR_NONE, let_to_name(*pack, FALSE),
				MENU_UNSELECTED);
			collected_type_name = TRUE;
		   }
		}
	    }
	    pack++;
	    if (invlet >= 'u') {
		impossible("query_category: too many categories");
		return 0;
	    }
	} while (*pack);
	/* unpaid items if there are any */
	if (do_unpaid) {
		invlet = 'u';
		any.a_void = 0;
		any.a_int = 'u';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
/*JP			"Unpaid items", MENU_UNSELECTED);*/
			"未払の道具", MENU_UNSELECTED);
	}
	/* billed items: checked by caller, so always include if BILLED_TYPES */
	if (qflags & BILLED_TYPES) {
		invlet = 'x';
		any.a_void = 0;
		any.a_int = 'x';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
/*JP			 "Unpaid items already used up", MENU_UNSELECTED);*/
			 "未払で使ってしまった道具", MENU_UNSELECTED);
	}
	if (qflags & CHOOSE_ALL) {
		invlet = 'A';
		any.a_void = 0;
		any.a_int = 'A';
		add_menu(win, NO_GLYPH, &any, invlet, 0, ATR_NONE,
			(qflags & WORN_TYPES) ?
/*JP
			"Auto-select every item being worn" :
			"Auto-select every item", MENU_UNSELECTED);
*/
			"身につけられる物全て" :
			"全て", MENU_UNSELECTED);
	}
	end_menu(win, qstr);
	n = select_menu(win, how, pick_list);
	destroy_nhwindow(win);
	if (n < 0)
	    n = 0;	/* caller's don't expect -1 */
	return n;
}

static int
count_categories(olist, qflags)
struct obj *olist;
int qflags;
{
	char *pack;
	boolean counted_category;
	int ccount = 0;
	struct obj *curr;

	pack = flags.inv_order;
	do {
	    counted_category = FALSE;
	    for (curr = olist; curr; curr = FOLLOW(curr, qflags)) {
		if (curr->oclass == *pack) {
		   if ((qflags & WORN_TYPES) &&
	!(curr->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_WEP)))
			 continue;
		   if (!counted_category) {
			ccount++;
			counted_category = TRUE;
		   }
		}
	    }
	    pack++;
	} while (*pack);
	return ccount;
}

/* could we carry `obj'? if not, could we carry some of it/them? */
static
long carry_count(obj, container, count, telekinesis, wt_before, wt_after)
struct obj *obj, *container;	/* object to pick up, bag it's coming out of */
long count;
boolean telekinesis;
int *wt_before, *wt_after;
{
    boolean adjust_wt = container && carried(container),
	    is_gold = obj->oclass == GOLD_CLASS;
    int wt, iw, ow, oow;
    long qq, savequan;
    unsigned saveowt;
    const char *verb, *prefx1;/*JP, *prefx2, *suffx;*/
    char obj_nambuf[BUFSZ], where[BUFSZ];

    savequan = obj->quan;
    saveowt = obj->owt;
    iw = max_capacity();
    if (count != savequan) {
	obj->quan = count;
	obj->owt = (unsigned)weight(obj);
    }
    wt = iw + (int)obj->owt;
    if (adjust_wt)
	wt -= (container->otyp == BAG_OF_HOLDING) ?
		(int)DELTA_CWT(container, obj) : (int)obj->owt;
    if (is_gold)	/* merged gold might affect cumulative weight */
	wt -= (GOLD_WT(u.ugold) + GOLD_WT(count) - GOLD_WT(u.ugold + count));
    if (count != savequan) {
	obj->quan = savequan;
	obj->owt = saveowt;
    }
    *wt_before = iw;
    *wt_after  = wt;
    if (wt < 0)
	return count;

    /* see how many we can lift */
    if (is_gold) {
	iw -= (int)GOLD_WT(u.ugold);
	if (!adjust_wt) {
	    qq = GOLD_CAPACITY((long)iw, u.ugold);
	} else {
	    oow = 0;
	    qq = 50L - (u.ugold % 100L) - 1L;
	    if (qq < 0L) qq += 100L;
	    for ( ; qq <= count; qq += 100L) {
		obj->quan = qq;
		obj->owt = (unsigned)GOLD_WT(qq);
		ow = (int)GOLD_WT(u.ugold + qq);
		ow -= (container->otyp == BAG_OF_HOLDING) ?
			(int)DELTA_CWT(container, obj) : (int)obj->owt;
		if (iw + ow >= 0) break;
		oow = ow;
	    }
	    iw -= oow;
	    qq -= 100L;
	}
	if (qq < 0L) qq = 0L;
	else if (qq > count) qq = count;
	wt = iw + (int)GOLD_WT(u.ugold + qq);
    } else if (count > 1 || count < obj->quan) {
	/*
	 * Ugh. Calc num to lift by changing the quan of of the
	 * object and calling weight.
	 *
	 * This works for containers only because containers
	 * don't merge.		-dean
	 */
	for (qq = 1L; qq <= count; qq++) {
	    obj->quan = qq;
	    obj->owt = (unsigned)(ow = weight(obj));
	    if (adjust_wt)
		ow -= (container->otyp == BAG_OF_HOLDING) ?
			(int)DELTA_CWT(container, obj) : (int)obj->owt;
	    if (iw + ow >= 0)
		break;
	    wt = iw + ow;
	}
	--qq;
    } else {
	/* there's only one, and we can't lift it */
	qq = 0L;
    }
    obj->quan = savequan;
    obj->owt = saveowt;

    if (qq < count) {
	/* some message will be given */
	Strcpy(obj_nambuf, doname(obj));
	if (container) {
/*JP	    Sprintf(where, "in %s", the(xname(container)));*/
	    Sprintf(where, "%sの中に入っている", the(xname(container)));
/*JP	    verb = "carry";*/
	    verb = "運べない";
	} else {
/*JP	    Strcpy(where, "lying here");*/
	    Strcpy(where, "ここに置いてある");
/*JP	    verb = telekinesis ? "acquire" : "lift";*/
	    verb = telekinesis ? "獲得できない" : "持ちあげられない";
	}
    } else {
	/* lint supppression */
	*obj_nambuf = *where = '\0';
	verb = "";
    }
    /* we can carry qq of them */
    if (qq > 0) {
	if (qq < count)
/*JP
	    You("can only %s %s of the %s %s.",
		verb, (qq == 1L) ? "one" : "some", obj_nambuf, where);
*/
	    You("%s%sのうちの%sしか%sできない．",
		where, obj_nambuf, (qq == 1L) ? "１つ" : "いくつか", verb);
	*wt_after = wt;
	return qq;
    }

/*JP    if (!container) Strcpy(where, "here");*//* slightly shorter form */
    if (!container) Strcpy(where, "ここには");  /* slightly shorter form */
    if (invent || u.ugold) {
/*JP
	prefx1 = "you cannot ";
	prefx2 = "";
	suffx  = " any more";
*/
	prefx1 = "これ以上";
    } else {
/*JP
	prefx1 = (obj->quan == 1L) ? "it " : "even one ";
	prefx2 = "is too heavy for you to ";
	suffx  = "";
*/
	prefx1 = "重すぎて";
    }
/*JP
    pline("There %s %s %s, but %s%s%s%s.",
	  (obj->quan == 1L) ? "is" : "are", obj_nambuf, where,
	  prefx1, prefx2, verb, suffx);
*/
    pline("%s%sがある，しかし%s．",
	  where, obj_nambuf, verb);

 /* *wt_after = iw; */
    return 0L;
}

/* determine whether character is able and player is willing to carry `obj' */
static
int lift_object(obj, container, cnt_p, telekinesis)
struct obj *obj, *container;	/* object to pick up, bag it's coming out of */
long *cnt_p;
boolean telekinesis;
{
    int result, old_wt, new_wt, prev_encumbr, next_encumbr;

    if (obj->otyp == LOADSTONE ||
	    (obj->otyp == BOULDER && throws_rocks(uasmon)))
	return 1;		/* lift regardless of current situation */

    *cnt_p = carry_count(obj, container, *cnt_p, telekinesis, &old_wt, &new_wt);
    if (*cnt_p < 1L) {
	result = -1;	/* nothing lifted */
    } else if (obj->oclass != GOLD_CLASS && inv_cnt() >= 52 &&
		!merge_choice(invent, obj)) {
/*JP	Your("knapsack cannot accommodate any more items.");*/
	Your("ナップザックはこれ以上アイテムを詰め込めない．");
	result = -1;	/* nothing lifted */
    } else {
	result = 1;
	prev_encumbr = near_capacity();
	if (prev_encumbr < MOD_ENCUMBER) prev_encumbr = MOD_ENCUMBER;
	next_encumbr = calc_capacity(new_wt - old_wt);
	if (next_encumbr > prev_encumbr) {
	    if (telekinesis) {
		result = 0;	/* don't lift */
	    } else {
		char qbuf[QBUFSZ];
		long savequan = obj->quan;

		obj->quan = *cnt_p;
/*JP
		Sprintf(qbuf, "%s %s.  Continue?",
			next_encumbr==HVY_ENCUMBER ? nearloadmsg : overloadmsg,
			doname(obj));
*/
		Sprintf(qbuf, "%s%s．続けますか？", 
			doname(obj), 
			next_encumbr==HVY_ENCUMBER ? nearloadmsg : overloadmsg);

		obj->quan = savequan;
		switch (ynq(qbuf)) {
		case 'q':  result = -1; break;
		case 'n':  result =  0; break;
		default:   break;	/* 'y' => result == 1 */
		}
	    }
	}
    }

    if (obj->otyp == SCR_SCARE_MONSTER && result <= 0 && !container)
	obj->spe = 0;
    return result;
}

/*
 * Pick up <count> of obj from the ground and add it to the hero's inventory.
 * Returns -1 if caller should break out of its loop, 0 if nothing picked
 * up, 1 if otherwise.
 */
int
pickup_object(obj, count, telekinesis)
struct obj *obj;
long count;
boolean telekinesis;	/* not picking it up directly by hand */
{
	int res, nearload;

	if (obj->quan < count) {
	    impossible("pickup_object: count %ld > quan %ld?",
		count, obj->quan);
	    return 0;
	}

	/* In case of auto-pickup, where we haven't had a chance
	   to look at it yet; affects docall(SCR_SCARE_MONSTER). */
	if (!Blind) obj->dknown = 1;

	if (obj == uchain) {    /* do not pick up attached chain */
	    return 0;
	} else if (obj->oartifact && !touch_artifact(obj,&youmonst)) {
	    return 0;
	} else if (obj->oclass == GOLD_CLASS) {
	    /* Special consideration for gold pieces... */
	    long iw = (long)max_capacity() - GOLD_WT(u.ugold);
	    long gold_capacity = GOLD_CAPACITY(iw, u.ugold);

	    if (gold_capacity <= 0L) {
/*JP
       pline("There %s %ld gold piece%s here, but you cannot carry any more.",
			(obj->quan == 1L) ? "is" : "are",
			obj->quan, plur(obj->quan));
*/
       pline("ここには%ld個の金塊がある, しかしこれ以上持てない．",
			obj->quan);
		return 0;
	    } else if (gold_capacity < count) {
/*JP
		You("can only %s %s of the %ld gold pieces lying here.",
		    telekinesis ? "acquire" : "carry",
		    gold_capacity == 1L ? "one" : "some", obj->quan);
*/
		You("%ld個の金塊の%sしか%s．",
		    obj->quan, gold_capacity == 1L ? "ひとつ" : "一部",
		    telekinesis ? "獲得できない" : "運べない");
/*JP
		pline("%s %ld gold piece%s.",
		    nearloadmsg, gold_capacity, plur(gold_capacity));
*/
		pline("%ld個の金塊%s．",
		    gold_capacity, nearloadmsg);
		u.ugold += gold_capacity;
		obj->quan -= gold_capacity;
		costly_gold(obj->ox, obj->oy, gold_capacity);
	    } else {
		u.ugold += count;
		if ((nearload = near_capacity()) != 0)
/*JP
		    pline("%s %ld gold piece%s.",
			  nearload < MOD_ENCUMBER ?
			  moderateloadmsg : nearloadmsg,
			  count, plur(count));
*/
		    pline("%ld個の金塊%s",
			  count,
			  nearload < MOD_ENCUMBER ?
			  moderateloadmsg : nearloadmsg);
		else
		    prinv((char *) 0, obj, count);
		costly_gold(obj->ox, obj->oy, count);
		if (count == obj->quan)
		    delobj(obj);
		else
		    obj->quan -= count;
	    }
	    flags.botl = 1;
	    if (flags.run) nomul(0);
	    return 1;
	} else if (obj->otyp == CORPSE) {
	    if (obj->corpsenm == PM_COCKATRICE && !uarmg
			&& !resists_ston(&youmonst) && !telekinesis) {
		if (poly_when_stoned(uasmon) && polymon(PM_STONE_GOLEM))
		    display_nhwindow(WIN_MESSAGE, FALSE);
		else {
/*JP		  pline("Touching the cockatrice corpse is a fatal mistake.");*/
		  pline("コカトリスの死体に触れるのは致命的な間違いだ．");
/*JP		    instapetrify("cockatrice corpse");*/
		    instapetrify("コカトリスの死体に触れて");
		    return -1;
		}
	    } else if (is_rider(&mons[obj->corpsenm])) {
/*JP		pline("At your %s, the corpse suddenly moves...",
			telekinesis ? "attempted acquisition" : "touch");*/
		pline("あなたが%s突然死体が動き出した．．．",
			telekinesis ? "獲得しようとすると" : "触ると");
		(void) revive_corpse(obj);
		exercise(A_WIS, FALSE);
		return -1;
	    }
	} else  if (obj->otyp == SCR_SCARE_MONSTER) {
	    if (obj->blessed) obj->blessed = 0;
	    else if (!obj->spe && !obj->cursed) obj->spe = 1;
	    else {
/*JP
		pline_The("scroll%s turn%s to dust as you %s %s up.",
			plur(obj->quan), (obj->quan == 1L) ? "s" : "",
			telekinesis ? "raise" : "pick",
			(obj->quan == 1L) ? "it" : "them");
*/
		pline("%s巻物はあなたが拾い上げると塵となってしまった．",
			(obj->quan == 1L) ? "その" : "それらの");
		if (!(objects[SCR_SCARE_MONSTER].oc_name_known) &&
				    !(objects[SCR_SCARE_MONSTER].oc_uname))
		    docall(obj);
		useupf(obj);
		return 1;	/* tried to pick something up and failed, but
				   don't want to terminate pickup loop yet   */
	    }
	}

	if ((res = lift_object(obj, (struct obj *)0, &count, telekinesis)) <= 0)
	    return res;

	if (obj->quan != count && obj->otyp != LOADSTONE)
	    (void) splitobj(obj, count);

	obj = pick_obj(obj);

	if (uwep && uwep == obj) mrg_to_wielded = TRUE;
	nearload = near_capacity();
	prinv(nearload == SLT_ENCUMBER ? moderateloadmsg : (char *) 0, obj, count);
	mrg_to_wielded = FALSE;
	return 1;
}

/*
 * Do the actual work of picking otmp from the floor and putting
 * it in the hero's inventory.  Take care of billing.  Return a
 * pointer to the object where otmp ends up.  This may be different
 * from otmp because of merging.
 *
 * Gold never reaches this routine.
 */
struct obj *
pick_obj(otmp)
register struct obj *otmp;
{
	obj_extract_self(otmp);
	if (*u.ushops && costly_spot(u.ux, u.uy) &&
	    otmp != uball)     /* don't charge for this - kd, 1/17/90 */
	   /* sets obj->unpaid if necessary */
	    addtobill(otmp, TRUE, FALSE, FALSE);
	if(Invisible) newsym(u.ux,u.uy);
	return(addinv(otmp));    /* might merge it with other objects */
}

/*
 * prints a message if encumbrance changed since the last check and
 * returns the new encumbrance value (from near_capacity()).
 */
int
encumber_msg()
{
    static int oldcap = UNENCUMBERED;
    int newcap = near_capacity();

    if(oldcap < newcap) {
	switch(newcap) {
#if 0 /*JP*/
	case 1: Your("movements are slowed slightly because of your load.");
		break;
	case 2: You("rebalance your load.  Movement is difficult.");
		break;
	case 3: You("stagger under your heavy load.  Movement is very hard.");
		break;
	default: You("%s move a handspan with this load!",
		     newcap == 4 ? "can barely" : "can't even");
#endif
	case 1: Your("動きは荷物のために少し遅くなった．");
		break;
	case 2: You("荷物の釣合をとり直したが，動きにくい．");
		break;
	case 3: You("荷物の重みでのたうちまわった．動くのが非常にきつい．");
		break;
	case 4: You("かろうじて，動くことができる．");
		break;
	default: You("この重さでは少しも動けない！");
		break;
	}
	flags.botl = 1;
    } else if(oldcap > newcap) {
	switch(newcap) {
#if 0 /*JP*/
	case 0: Your("movements are now unencumbered.");
		break;
	case 1: Your("movements are only slowed slightly by your load.");
		break;
	case 2: You("rebalance your load.  Movement is still difficult.");
		break;
	case 3: You("stagger under your load.  Movement is still very hard.");
		break;
#endif
	case 0: Your("動きは楽になった．");
		break;
	case 1: You("ちょっと動きやすくなった．");
		break;
	case 2: You("荷物の釣合をとり直した．だがまだ動くのはきつい．");
		break;
	case 3: You("荷物の重みがずっしりと肩にくる．まだ動くのが非常にきつい．");
		break;
	}
	flags.botl = 1;
    }

    oldcap = newcap;
    return (newcap);
}

int
doloot()	/* loot a container on the floor. */
{
	register struct obj *cobj, *nobj;
	register int c = -1;
	int timepassed = 0;

#ifdef NEWBIE
	newbie.loot = 1;
#endif
	if (check_capacity((char *)0)) {
		/* "Can't do that while carrying so much stuff." */
		return 0;
	} else if (!can_reach_floor()) {
/*JP		You("cannot reach the %s.", surface(u.ux, u.uy));*/
		pline("%sに届かない", surface(u.ux, u.uy));
		return(0);
	} else if (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)) {
		/* at present, can't loot in water even when Underwater */
/*JP
		You("cannot loot things that are deep in the %s.",
		    is_lava(u.ux, u.uy) ? "lava" : "water");
*/
		You("%sに深くに沈んだものを開けることできない．",
		    is_lava(u.ux, u.uy) ? "溶岩" : "水");

		return(0);
	} else if (nolimbs(uasmon)) {
/*JP		pline("Without limbs, you cannot loot anything.");*/
		pline("手足がないので，開けることはできない．");
		return(0);
	}

	for(cobj = level.objects[u.ux][u.uy]; cobj; cobj = nobj) {
		nobj = cobj->nexthere;
		if(Is_container(cobj)) {
		    char qbuf[QBUFSZ];

/*JP		    Sprintf(qbuf, "There is %s here, loot it?", doname(cobj));*/
		    Sprintf(qbuf, "ここには%sがある．開けますか？",
			    doname(cobj));
		    c = ynq(qbuf);
		    if(c == 'q') return (timepassed);
		    if(c == 'n') continue;

		    if(cobj->olocked) {
/*JP			pline("Hmmm, it seems to be locked.");*/
			pline("むーん，鍵がかかっているようだ．");
			continue;
		    }
		    if(cobj->otyp == BAG_OF_TRICKS) {
/*JP			You("carefully open the bag...");
			pline("It develops a huge set of teeth and bites you!");*/
			You("慎重に鞄を開けた．．．");
			pline("鞄から大きな歯が生えてきて，あなたを噛んだ！");
			c = rnd(10);
			if(Half_physical_damage) c = (c+1) / 2;
/*JP			losehp(c, "carnivorous bag", KILLED_BY_AN);*/
			losehp(c, "食肉鞄によって", KILLED_BY_AN);
			makeknown(BAG_OF_TRICKS);
			timepassed = 1;
			continue;
		    }

/*JP		    You("carefully open %s...", the(xname(cobj)));*/
		    You("慎重に%sを開けた．．．", the(xname(cobj)));
		    timepassed |= use_container(cobj, 0);
		    if (multi < 0) return 1;		/* chest trap */
		}
	}
	if(c == -1){
	    if(Confusion){
		if(u.ugold){
		    long contribution = rnd((int)min(LARGEST_INT,u.ugold));
		    struct obj *goldob = mkgoldobj(contribution);
		    if(IS_THRONE(levl[u.ux][u.uy].typ)){
			struct obj *coffers;
			int pass;
			    /* find the original coffers chest, or any chest */
			for(pass = 2; pass > -1; pass -= 2)
			    for(coffers=fobj; coffers; coffers=coffers->nobj)
				if(coffers->otyp==CHEST && coffers->spe ==pass)
				    goto gotit;	/* two level break */
gotit:
			if(coffers){
			    struct obj *tmp;
/*JP
verbalize("Thank you for your contribution to reduce the debt.");
*/
verbalize("赤字建設国債返済のための寄付に感謝します．");
			    for (tmp = coffers->cobj; tmp; tmp = tmp->nobj)
				if (tmp->otyp == goldob->otyp) break;

			    if (tmp) {
				tmp->quan += goldob->quan;
				delobj(goldob);
			    } else {
				add_to_container(coffers, goldob);
			    }
			} else {
			    struct monst *mon = makemon(courtmon(),
						    u.ux, u.uy, NO_MM_FLAGS);
			    if (mon) {
				mon->mgold += goldob->quan;
				delobj(goldob);
				pline(
/*JP				   "The exchequer accepts your contribution.");*/
				   "大蔵省はあなたの寄付を受けとった．");
			    } else {
				dropx(goldob);
			    }
			}
		    } else {
			dropx(goldob);
/*JP			pline("Ok, now there is loot here.");*/
			pline("オーケー，ここに賄賂を置いておこう．");
		    }
		}
	    } else {
/*JP		You("don't find anything here to loot.");*/
		pline("ここには開けるものがない．");
	    }
	}
    return (timepassed);
}

/*
 * Decide whether an object being placed into a magic bag will cause
 * it to explode.  If the object is a bag itself, check recursively.
 */
static boolean
mbag_explodes(obj, depthin)
    struct obj *obj;
    int depthin;
{
    /* these won't cause an explosion when they're empty */
    if ((obj->otyp == WAN_CANCELLATION || obj->otyp == BAG_OF_TRICKS) &&
	    obj->spe <= 0)
	return FALSE;

    /* odds: 1/1, 2/2, 3/4, 4/8, 5/16, 6/32, 7/64, 8/128, 9/128, 10/128,... */
    if ((Is_mbag(obj) || obj->otyp == WAN_CANCELLATION) &&
	(rn2(1 << (depthin > 7 ? 7 : depthin)) <= depthin))
	return TRUE;
    else if (Has_contents(obj)) {
	struct obj *otmp;

	for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
	    if (mbag_explodes(otmp, depthin+1)) return TRUE;
    }
    return FALSE;
}

/* A variable set in use_container(), to be used by the callback routines   */
/* in_container(), and out_container() from askchain() and use_container(). */
static NEARDATA struct obj *current_container;
#define Icebox (current_container->otyp == ICE_BOX)

/* Returns: -1 to stop, 1 item was inserted, 0 item was not inserted. */
STATIC_PTR int
in_container(obj)
register struct obj *obj;
{
	register struct obj *gold;
	boolean is_gold = (obj->oclass == GOLD_CLASS);
	boolean floor_container = !carried(current_container);
	char buf[BUFSZ];

	if (!current_container) {
		impossible("<in> no current_container?");
		return 0;
	} else if (obj == uball || obj == uchain) {
/*JP		You("must be kidding.");*/
		You("ふざけている．");
		return 0;
	} else if (obj == current_container) {
/*JP		pline("That would be an interesting topological exercise.");*/
		pline("それは興味をそそられるトポロジーの問題だ．");
		return 0;
	} else if (obj->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL)) {
/*JP
		Norep("You cannot %s %s you are wearing.",
			Icebox ? "refrigerate" : "stash", something);
*/
		Norep("身につけているものを%sことはできない．", 
			Icebox ? "冷凍する" : "しまう");
		return 0;
	} else if ((obj->otyp == LOADSTONE) && obj->cursed) {
		obj->bknown = 1;
/*JP
	      pline_The("stone%s won't leave your person.", plur(obj->quan));	
*/
		pline("どういうわけかその石をしまうことはできない．");
	return 0;
	} else if (obj->otyp == AMULET_OF_YENDOR ||
		   obj->otyp == CANDELABRUM_OF_INVOCATION ||
		   obj->otyp == BELL_OF_OPENING ||
		   obj->otyp == SPE_BOOK_OF_THE_DEAD) {
	/* Prohibit Amulets in containers; if you allow it, monsters can't
	 * steal them.  It also becomes a pain to check to see if someone
	 * has the Amulet.  Ditto for the Candelabrum, the Bell and the Book.
	 */
/*JP
	    pline("%s cannot be confined in such trappings.", The(xname(obj)));
*/
	    pline("%sは詰めることはできない．", The(xname(obj)));
	    return 0;
	} else if (obj->otyp == LEASH && obj->leashmon != 0) {
/*JP
		pline("%s is attached to your pet.", The(xname(obj)));
*/
		pline("%sはペットに付けられている．", The(xname(obj)));
		return 0;
	} else if (obj == uwep) {
		if (welded(obj)) {
			weldmsg(obj);
			return 0;
		}
		setuwep((struct obj *) 0);
		if (uwep) return 0;	/* unwielded, died, rewielded */
	}

	/* boxes can't fit into any container */
	if (obj->otyp == ICE_BOX || Is_box(obj)) {
		/*
		 *  xname() uses a static result array.  Save obj's name
		 *  before current_container's name is computed.  Don't
		 *  use the result of strcpy() within You() --- the order
		 *  of evaluation of the parameters is undefined.
		 */
		Strcpy(buf, the(xname(obj)));
/*JP		You("cannot fit %s into %s.", buf,*/
		pline("%sを%sに詰め込むことはできない．", buf,
		    the(xname(current_container)));
		return 0;
	}

	freeinv(obj);

	if (is_gold) {	/* look for other money to merge within the container */
		for (gold = current_container->cobj; gold; gold = gold->nobj)
			if (gold->otyp == obj->otyp) break;
	} else
		gold = 0;

	if (gold) {
		gold->quan += obj->quan;
	} else {
		add_to_container(current_container, obj);
	}

	current_container->owt = weight(current_container);

	Strcpy(buf, the(xname(current_container)));
/*JP	You("put %s into %s.", doname(obj), buf);*/
	You("%sを%sの中にしまった．", doname(obj), buf);

	if (obj_is_burning(obj))	/* this used to be part of freeinv() */
		(void) snuff_lit(obj);

	if (floor_container && costly_spot(u.ux, u.uy)) {
		sellobj_state(TRUE);
		sellobj(obj, u.ux, u.uy);
		sellobj_state(FALSE);
	}
	if (Icebox && obj->otyp != OIL_LAMP && obj->otyp != BRASS_LANTERN
			&& !Is_candle(obj)) {
		obj->age = monstermoves - obj->age; /* actual age */
		/* stop any corpse timeouts when frozen */
		if (obj->otyp == CORPSE && obj->timed) {
			(void) stop_timer(ROT_CORPSE, (genericptr_t)obj);
			(void) stop_timer(REVIVE_MON, (genericptr_t)obj);
		}
	}

	else if (Is_mbag(current_container) && mbag_explodes(obj, 0)) {
/*JP		You("are blasted by a magical explosion!");*/
		You("魔法の爆発を浴びた！");

		/* the !floor_container case is taken care of */
		if(*u.ushops && costly_spot(u.ux, u.uy) && floor_container) {
		    register struct monst *shkp;

		    if ((shkp = shop_keeper(*u.ushops)) != 0)
			(void)stolen_value(current_container, u.ux, u.uy,
					   (boolean)shkp->mpeaceful, FALSE);
		}
		delete_contents(current_container);
		if (!floor_container)
			useup(current_container);
		else if (obj_here(current_container, u.ux, u.uy))
			useupf(current_container);
		else
			panic("in_container:  bag not found.");

/*JP		losehp(d(6,6),"magical explosion", KILLED_BY_AN);*/
		losehp(d(6,6),"魔法の爆発で", KILLED_BY_AN);
		current_container = 0;	/* baggone = TRUE; */
	}

	if (is_gold) {
		if (gold) dealloc_obj(obj);
		bot();	/* update character's gold piece count immediately */
	}

	return(current_container ? 1 : -1);
}

STATIC_PTR int
ck_bag(obj)
struct obj *obj;
{
	return current_container && obj != current_container;
}

/* Returns: -1 to stop, 1 item was removed, 0 item was not removed. */
STATIC_PTR int
out_container(obj)
register struct obj *obj;
{
	register struct obj *otmp;
	boolean is_gold = (obj->oclass == GOLD_CLASS);
	int res, loadlev;
	long count;

	if (!current_container) {
		impossible("<out> no current_container?");
		return -1;
	} else if (is_gold) {
		obj->owt = weight(obj);
	}

	if(obj->oartifact && !touch_artifact(obj,&youmonst)) return 0;

	count = obj->quan;
	if ((res = lift_object(obj, current_container, &count, FALSE)) <= 0)
	    return res;

	if (obj->quan != count && obj->otyp != LOADSTONE)
	    (void) splitobj(obj, count);

	/* Remove the object from the list. */
	obj_extract_self(obj);
	current_container->owt = weight(current_container);

	if (Icebox && obj->otyp != OIL_LAMP && obj->otyp != BRASS_LANTERN
			&& !Is_candle(obj)) {
		obj->age = monstermoves - obj->age; /* actual age */
		if (obj->otyp == CORPSE)
			start_corpse_timeout(obj);
	}
	/* simulated point of time */

	if(!obj->unpaid && !carried(current_container) &&
	     costly_spot(current_container->ox, current_container->oy)) {

		obj->ox = current_container->ox;
		obj->oy = current_container->oy;
		addtobill(obj, FALSE, FALSE, FALSE);
	}

	otmp = addinv(obj);
	loadlev = near_capacity();
	prinv(loadlev ?
	      (loadlev < MOD_ENCUMBER ?
/*JP
	       "You have a little trouble removing" :
	       "You have much trouble removing") : (char *)0,
*/
	       "を運ぶのは少々困難だ．" :
	       "を運ぶのはかなり困難だ．") : (char *)0,
	      otmp, count);

	if (is_gold) {
		dealloc_obj(obj);
		bot();	/* update character's gold piece count immediately */
	}
	return 1;
}

#undef Icebox

int
use_container(obj, held)
register struct obj *obj;
register int held;
{
	struct obj *curr, *otmp, *u_gold = (struct obj *)0;
	struct monst *shkp;
	boolean one_by_one, allflag, loot_out = FALSE, loot_in = FALSE;
	char select[MAXOCLASSES+1];
	char qbuf[QBUFSZ];
	long loss = 0L;
	int cnt = 0, used = 0, lcnt = 0,
	    menu_on_request;

	if (obj->olocked) {
/*JP	    pline("%s seems to be locked.", The(xname(obj)));
	    if (held) You("must put it down to unlock.");*/
	    pline("%sは鍵がかかっているようだ．", The(xname(obj)));
	    if (held) pline("下に置かないことには鍵をはずせない．");
	    return 0;
	} else if (obj->otrapped) {
/*JP	    if (held) You("open %s...", the(xname(obj)));*/
	    if (held) You("%sを開けた．．．", the(xname(obj)));
	    (void) chest_trap(obj, HAND, FALSE);
	    /* even if the trap fails, you've used up this turn */
	    if (multi >= 0) {	/* in case we didn't become paralyzed */
		nomul(-1);
		nomovemsg = "";
	    }
	    return 1;
	}
	current_container = obj;	/* for use by in/out_container */

	if (obj->spe == 1) {
/*JP	    static NEARDATA const char sc[] = "Schroedinger's Cat";*/
	    static NEARDATA const char sc[] = "シュレディンガーの猫";
	    struct obj *ocat;
	    struct monst *cat;

	    obj->spe = 0;		/* obj->owt will be updated below */
	    /* this isn't really right, since any form of observation
	       (telepathic or monster/object/food detection) ought to
	       force the determination of alive vs dead state; but basing
	       it just on opening the box is much simpler to cope with */
	    cat = rn2(2) ? makemon(&mons[PM_HOUSECAT],
				   obj->ox, obj->oy, NO_MINVENT) : 0;
	    if (cat) {
		cat->mpeaceful = 1;
		set_malign(cat);
		if (Blind)
/*JP		    You("think %s brushed your %s.", something,*/
		    You("%sがあなたの%sをくすぐった．", something,
			body_part(FOOT));
		else
/*JP		    pline("%s inside the box is still alive!", Monnam(cat));*/
		    pline("箱のなかの%sはまだ生きている！", Monnam(cat));
		(void) christen_monst(cat, sc);
	    } else {
		ocat = mk_named_object(CORPSE, &mons[PM_HOUSECAT],
				       obj->ox, obj->oy, sc);
		if (ocat) {
		    obj_extract_self(ocat);
		    add_to_container(obj, ocat);  /* weight handled below */
		}
/*JP		pline_The("%s inside the box is dead!",
		    Hallucination ? rndmonnam() : "housecat");*/
		pline("箱の中の%sは死んでいる！",
		    Hallucination ? rndmonnam() : jtrns_mon("housecat", -1));
	    }
	    used = 1;
	}
	/* Count the number of contained objects. Sometimes toss objects if */
	/* a cursed magic bag.						    */
	for (curr = obj->cobj; curr; curr = otmp) {
	    otmp = curr->nobj;
	    if (Is_mbag(obj) && obj->cursed && !rn2(13)) {
		if (curr->dknown)
/*JP		    pline("%s to have vanished!", The(aobjnam(curr,"seem")));*/
		    pline("%sは消え去った！", The(xname(curr)));
		else
/*JP		    You("%s %s disappear.", Blind ? "notice" : "see",
							doname(curr));*/
		    You("%sが見えなくなるの%s．",
			doname(curr),
			Blind ? "に気づいた" : "を見た");
		obj_extract_self(curr);
		if (*u.ushops && (shkp = shop_keeper(*u.ushops)) != 0) {
		    if(held) {
			if(curr->unpaid)
			    loss += stolen_value(curr, u.ux, u.uy,
					     (boolean)shkp->mpeaceful, TRUE);
			lcnt++;
		    } else if(costly_spot(u.ux, u.uy)) {
			loss += stolen_value(curr, u.ux, u.uy,
					     (boolean)shkp->mpeaceful, TRUE);
			lcnt++;
		    }
		}
		/* obfree() will free all contained objects */
		obfree(curr, (struct obj *) 0);
		used = 1;
	    } else {
		cnt++;
	    }
	}

	if (cnt && loss)
/*JP
	    You("owe %ld zorkmids for lost item%s.",
		loss, lcnt > 1 ? "s" : "");
*/
	    You("なくなったアイテムのために%ldゴールドの借りをつくった．",
		loss);

	obj->owt = weight(obj);

	if (!cnt) {
/*JP
	    pline("%s is empty.", Yname2(obj));
*/
	    pline("%sは空っぽだ．", Yname2(obj));
	} else {
/*JP
	    Sprintf(qbuf, "Do you want to take %s out of %s?",
		    something, yname(obj));
*/
	    Sprintf(qbuf, "%sから何か取り出しますか？", yname(obj));
	    if (flags.menu_style != MENU_TRADITIONAL) {
		if (flags.menu_style == MENU_FULL) {
/*JP
		    int t = in_or_out_menu("Do what?", current_container);
*/
		    int t = in_or_out_menu("どうする？", current_container);
		    if (t <= 0) return 0;
		    loot_out = (t & 0x01) != 0;
		    loot_in  = (t & 0x02) != 0;
		} else {	/* MENU_COMBINATION or MENU_PARTIAL */
		    loot_out = (yn_function(qbuf, "ynq", 'n') == 'y');
		}
		if (loot_out) {
		    add_valid_menu_class(0);	/* reset */
		    used |= menu_loot(0, current_container, FALSE) > 0;
		}
	    } else {
		/* traditional code */
ask_again2:
		menu_on_request = 0;
		add_valid_menu_class(0);	/* reset */
		switch (yn_function(qbuf, ":ynq", 'n')) {
		case ':':
		    container_contents(current_container, FALSE, FALSE);
		    goto ask_again2;
		case 'y':
/*JP		    if (query_classes(select, &one_by_one, &allflag, "take out",*/
		    if (query_classes(select, &one_by_one, &allflag, "取り出す",
				      current_container->cobj,
				      FALSE, FALSE, &menu_on_request)) {
			if (askchain((struct obj **)&current_container->cobj,
				     (one_by_one ? (char *)0 : select), allflag,
				     out_container, (int (*)())0, 0, "nodot"))
			    used = 1;
		    } else if (menu_on_request < 0) {
			used |= menu_loot(menu_on_request,
					  current_container, FALSE) > 0;
		    }
		    /*FALLTHRU*/
		case 'n':
		    break;
		case 'q':
		default:
		    return used;
		}
	    }
	}

	if (!invent && u.ugold == 0) {
	    /* nothing to put in, but some feedback is necessary */
/*JP	    You("don't have anything to put in.");*/
	    You("入れるものが何もない．");
	    return used;
	}
	if (flags.menu_style != MENU_FULL || !cnt) {
/*JP	    loot_in = (yn_function("Do you wish to put something in?",*/
	    loot_in = (yn_function("中に何か入れますか？",
				   ynqchars, 'n') == 'y');
	}
	/*
	 * Gone: being nice about only selecting food if we know we are
	 * putting things in an ice chest.
	 */
	if (loot_in) {
	    if (u.ugold) {
		/*
		 * Hack: gold is not in the inventory, so make a gold object
		 * and put it at the head of the inventory list.
		 */
		u_gold = mkgoldobj(u.ugold);	/* removes from u.ugold */
		u.ugold = u_gold->quan;		/* put the gold back */
		assigninvlet(u_gold);		/* might end up as NOINVSYM */
		u_gold->nobj = invent;
		invent = u_gold;
	    }
	    add_valid_menu_class(0);	  /* reset */
	    if (flags.menu_style != MENU_TRADITIONAL) {
		used |= menu_loot(0, current_container, TRUE) > 0;
	    } else {
		/* traditional code */
		menu_on_request = 0;
/*JP		if (query_classes(select, &one_by_one, &allflag, "put in",*/
		if (query_classes(select, &one_by_one, &allflag, "入れる",
				   invent, FALSE, (u.ugold != 0L),
				   &menu_on_request)) {
		    (void) askchain((struct obj **)&invent,
				    (one_by_one ? (char *)0 : select), allflag,
				    in_container, ck_bag, 0, "nodot");
		    used = 1;
		} else if (menu_on_request < 0) {
		    used |= menu_loot(menu_on_request,
				      current_container, TRUE) > 0;
		}
	    }
	}

	if (u_gold && invent && invent->oclass == GOLD_CLASS) {
	    /* didn't stash [all of] it */
	    u_gold = invent;
	    invent = u_gold->nobj;
	    dealloc_obj(u_gold);
	}

	return used;
}

/* Loot a container (take things out, put things in), using a menu. */
static int
menu_loot(retry, container, put_in)
int retry;
struct obj *container;
boolean put_in;
{
    int n, i, n_looted = 0;
    boolean all_categories = TRUE, loot_everything = FALSE;
    char buf[BUFSZ];
/*JP    const char *takeout = "Take out", *putin = "Put in";*/
    const char *takeout = "取り出しますか", *putin = "入れますか";
    menu_item *pick_list;
    int mflags, res;

    if (retry) {
	all_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
	all_categories = FALSE;
/*JP	Sprintf(buf,"%s what type of objects?", put_in ? putin : takeout);*/
	Sprintf(buf,"どのタイプの物体を%s？", put_in ? putin : takeout);
	mflags = put_in ? ALL_TYPES : ALL_TYPES|CHOOSE_ALL;
	n = query_category(buf, put_in ? invent : container->cobj,
			   mflags, &pick_list, PICK_ANY);
	if (!n) return 0;
	for (i = 0; i < n; i++) {
	    if (pick_list[i].item.a_int == 'A')
		loot_everything = TRUE;
	    else if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
		all_categories = TRUE;
	    else
		add_valid_menu_class(pick_list[i].item.a_int);
	}
	free((genericptr_t) pick_list);
    }

    if (loot_everything) {
	struct obj *otmp, *otmp2;

	for (otmp = container->cobj; otmp; otmp = otmp2) {
	    otmp2 = otmp->nobj;
	    res = out_container(otmp);
	    if (res < 0) break;
	}
    } else {
	mflags = INVORDER_SORT;
	if (put_in && flags.invlet_constant) mflags |= USE_INVLET;
/*JP	Sprintf(buf,"%s what?", put_in ? putin : takeout);*/
	Sprintf(buf,"何を%s？", put_in ? putin : takeout);
	n = query_objlist(buf, put_in ? invent : container->cobj,
			  mflags, &pick_list, PICK_ANY,
			  all_categories ? allow_all : allow_category);
	if (n) {
		n_looted = n;
		for (i = 0; i < n; i++) {
		    if (pick_list[i].item.a_obj->quan != pick_list[i].count) {
			(void) splitobj(pick_list[i].item.a_obj,
					pick_list[i].count);
		    }
		    res = put_in ? in_container(pick_list[i].item.a_obj) :
				   out_container(pick_list[i].item.a_obj);
		    if (res < 0)
			break;
		}
		free((genericptr_t)pick_list);
	}
    }
    return n_looted;
}

static int
in_or_out_menu(prompt, obj)
const char *prompt;
struct obj *obj;
{
    winid win;
    anything any;
    menu_item *pick_list;
    char buf[BUFSZ];
    int n;

    any.a_void = 0;
    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    any.a_int = 1;
/*JP    Sprintf(buf,"Take something out of %s", the(xname(obj)));*/
    Sprintf(buf,"%sから出す", the(xname(obj)));
    add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE, buf, MENU_UNSELECTED);
    any.a_int = 2;
/*JP    Sprintf(buf,"Put something into %s",the(xname(obj)));*/
    Sprintf(buf,"%sへ入れる",the(xname(obj)));
    add_menu(win, NO_GLYPH, &any, 'b', 0, ATR_NONE, buf, MENU_UNSELECTED);
    any.a_int = 3;
    add_menu(win, NO_GLYPH, &any, 'c', 0, ATR_NONE,
/*JP		"Both of the above", MENU_UNSELECTED);*/
		"両方", MENU_UNSELECTED);
    end_menu(win, prompt);
    n = select_menu(win, PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
	n = pick_list[0].item.a_int;
	free((genericptr_t) pick_list);
    }
    return n;
}

/*pickup.c*/
