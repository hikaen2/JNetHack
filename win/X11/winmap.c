/*	SCCS Id: @(#)winmap.c	3.3	96/04/05	*/
/* Copyright (c) Dean Luick, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains:
 *	+ global functions print_glyph() and cliparound()
 *	+ the map window routines
 *	+ the char and pointer input routines
 *
 * Notes:
 *	+ We don't really have a good way to get the compiled ROWNO and
 *	  COLNO as defaults.  They are hardwired to the current "correct"
 *	  values in the Window widget.  I am _not_ in favor of including
 *	  some nethack include file for Window.c.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV	/* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Label.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef PRESERVE_NO_SYSV
# ifdef SYSV
#  undef SYSV
# endif
# undef PRESERVE_NO_SYSV
#endif

#include "xwindow.h"	/* map widget declarations */

#include "hack.h"
#include "dlb.h"
#include "winX.h"

#ifdef USE_XPM
#include <X11/xpm.h>
#endif


/* from tile.c */
extern short glyph2tile[];
extern int total_tiles_used;

/* Define these if you really want a lot of junk on your screen. */
/* #define VERBOSE */		/* print various info & events as they happen */
/* #define VERBOSE_UPDATE */	/* print screen update bounds */
/* #define VERBOSE_INPUT */	/* print input events */


#define USE_WHITE	/* almost always use white as a tile cursor border */


static boolean FDECL(init_tiles, (struct xwindow *));
static void FDECL(set_button_values, (Widget,int,int,unsigned));
static void FDECL(map_check_size_change, (struct xwindow *));
static void FDECL(map_update, (struct xwindow *,int,int,int,int,BOOLEAN_P));
static void FDECL(init_text, (struct xwindow *));
static void FDECL(map_exposed, (Widget,XtPointer,XtPointer));
static void FDECL(set_gc, (Widget,Font,char *,Pixel,GC *,GC *));
static void FDECL(get_text_gc, (struct xwindow *,Font));
static void FDECL(get_char_info, (struct xwindow *));
static void FDECL(display_cursor, (struct xwindow *));

#ifdef RADAR

#define RADAR_WIDTH	(80 * 4)
#define RADAR_HEIGHT	(24 * 4)

#define USE_RADAR	(map_info->is_tile)
static int	radar_is_popuped;
static int	radar_is_initialized;

static Widget	radar;
static int	radar_x, radar_y;
static int	radar_w, radar_h;

static Widget	radar_popup;
static Window	radar_window;
static Pixmap	radar_pixmap;
static Pixmap	radar_pixmap2;
static GC	radar_gc_black;
static GC	radar_gc_white;
static GC	radar_gc[16];

static XColor	radar_color[16] = {
  {0, 0x0000, 0x0000, 0x0000,},	/* black */
  {0, 0xffff, 0xffff, 0xffff,},	/* white */
  {0, 0xffff, 0x0000, 0x0000,},	/* red */
  {0, 0x0000, 0xffff, 0x0000,},	/* green */
  {0, 0x0000, 0x0000, 0xffff,},	/* blue */
  {0, 0xffff, 0xffff, 0x0000,},	/* yellow */
  {0, 0xffff, 0x0000, 0xffff,},	/* magenta */
  {0, 0x8000, 0x8000, 0x8000,},	/* gray */
  {0, 0xffff, 0x8000, 0x0000,},	/* orange */
  {0, 0x0000, 0x8000, 0x0000,},	/* dark green */
  {0, 0x0000, 0xffff, 0xffff,},	/* cyan */
};
enum {
  RADAR_BLACK,
  RADAR_WHITE,
  RADAR_RED,
  RADAR_GREEN,
  RADAR_BLUE,
  RADAR_YELLOW,
  RADAR_MAGENTA,
  RADAR_GRAY,
  RADAR_ORANGE,
  RADAR_DARKGREEN,
  RADAR_CYAN,
};

#endif

/*ITA*/
struct pxm_slot_t {
    int fg;
    int bg;
    int age;
    Pixmap pixmap;
};
#define MAX_PXM_SLOTS 100
    struct pxm_slot_t pxm_slot[MAX_PXM_SLOTS]; 
/*ITA*/


/* Global functions ======================================================== */

void
/*JP
X11_print_glyph(window, x, y, glyph)
*/
X11_print_glyph(window, x, y, glyph)
    winid window;
    xchar x, y;
    int glyph;
{
    struct map_info_t *map_info;
    boolean update_bbox;

    check_winid(window);
    if (window_list[window].type != NHW_MAP) {
	impossible("print_glyph: can (currently) only print to map windows");
	return;
    }
    map_info = window_list[window].map_information;

    if (map_info->is_tile) {
	unsigned short *t_ptr;
	t_ptr = &map_info->mtype.tile_map->glyphs[y][x];

	if (*t_ptr != glyph) {
	    *t_ptr = glyph;
	    update_bbox = TRUE;
	}
	else
	  update_bbox = FALSE;
    } else {
	uchar			ch;
	register int		offset;
	register unsigned char *ch_ptr;
#ifdef TEXTCOLOR
	int			color;
	register unsigned char *co_ptr;

#define zap_color(n)  color = zapcolors[n]
#define cmap_color(n) color = defsyms[n].color
#define obj_color(n)  color = objects[n].oc_color
#define mon_color(n)  color = mons[n].mcolor
#define invis_color(n) color = NO_COLOR
#define pet_color(n)  color = mons[n].mcolor

# else /* no text color */

#define zap_color(n)
#define cmap_color(n)
#define obj_color(n)
#define mon_color(n)
#define invis_color(n)
#define pet_color(n)
#endif

	/*
	 *  Map the glyph back to a character.
	 *
	 *  Warning:  For speed, this makes an assumption on the order of
	 *            offsets.  The order is set in display.h.
	 */
	if ((offset = (glyph - GLYPH_SWALLOW_OFF)) >= 0) {	/* swallow */
	    /* see swallow_to_glyph() in display.c */
	    ch = (uchar) showsyms[S_sw_tl + (offset & 0x7)];
	    mon_color(offset >> 3);
	} else if ((offset = (glyph - GLYPH_ZAP_OFF)) >= 0) {	/* zap beam */
	    /* see zapdir_to_glyph() in display.c */
	    ch = showsyms[S_vbeam + (offset & 0x3)];
	    zap_color((offset >> 2));
	} else if ((offset = (glyph - GLYPH_CMAP_OFF)) >= 0) {	/* cmap */
	    ch = showsyms[offset];
	    cmap_color(offset);
	} else if ((offset = (glyph - GLYPH_OBJ_OFF)) >= 0) {	/* object */
	    ch = oc_syms[(int) objects[offset].oc_class];
	    obj_color(offset);
	} else if ((offset = (glyph - GLYPH_RIDDEN_OFF)) >= 0) {/* ridden mon */
	    ch = monsyms[(int) mons[offset].mlet];
	    mon_color(offset);
	} else if ((offset = (glyph - GLYPH_BODY_OFF)) >= 0) {	/* a corpse */
	    ch = oc_syms[(int) objects[CORPSE].oc_class];
	    mon_color(offset);
	} else if ((offset = (glyph - GLYPH_DETECT_OFF)) >= 0) {
	    /* monster detection; should really be inverse */
	    ch = monsyms[(int) mons[offset].mlet];
	    mon_color(offset);
	} else if ((offset = (glyph - GLYPH_INVIS_OFF)) >= 0) {	/* invisible */
	    ch = DEF_INVISIBLE;
	    invis_color(offset);
	} else if ((offset = (glyph - GLYPH_PET_OFF)) >= 0) {	/* a pet */
	    ch = monsyms[(int) mons[offset].mlet];
	    pet_color(offset);
	} else {						/* a monster */
	    ch = monsyms[(int) mons[glyph].mlet];
	    mon_color(glyph);
	}

	/* Only update if we need to. */
	ch_ptr = &map_info->mtype.text_map->text[y][x];

#ifdef TEXTCOLOR
	co_ptr = &map_info->mtype.text_map->colors[y][x];
	if (*ch_ptr != ch || *co_ptr != color)
#else
	if (*ch_ptr != ch)
#endif
	{
	    *ch_ptr = ch;
#ifdef TEXTCOLOR
	    *co_ptr = color;
#endif
	    update_bbox = TRUE;
	} else
	    update_bbox = FALSE;

#undef zap_color
#undef cmap_color
#undef obj_color
#undef mon_color
#undef pet_color
    }

    if (update_bbox) {		/* update row bbox */
	if ((uchar) x < map_info->t_start[y]) map_info->t_start[y] = x;
	if ((uchar) x > map_info->t_stop[y])  map_info->t_stop[y]  = x;
    }
}

#ifdef CLIPPING
/*
 * The is the tty clip call.  Since X can resize at any time, we can't depend
 * on this being defined.
 */
/*ARGSUSED*/
void X11_cliparound(x, y) int x, y; { }
#endif /* CLIPPING */

/* End global functions ==================================================== */

#include "tile2x11.h"

/*
 * We're expecting to never read more than one tile file per session.
 * If this is false, then we can make an array of this information,
 * or just keep it on a per-window basis.
 */
Pixmap tile_pixmap = None;
Pixmap tile_clipmask = None;
GC     tile_gc;
/*JP #ifdef USE_XPM*/
XpmImage tile_image;
/* #endif*/

#define	TILE_WIDTH	appResources.tile_width
#define	TILE_HEIGHT	appResources.tile_height
int	TILE_PER_COL;

/*JP #define TILE_PER_COL	32
static int tile_width;
static int tile_height;
static int tile_count;
*/

/*
 * This structure is used for small bitmaps that are used for annotating
 * tiles.  For example, a "heart" annotates pets.
 */
struct tile_annotation {
    Pixmap bitmap;
    Pixel foreground;
    unsigned int width, height;
    int hotx, hoty; /* not currently used */
};

static struct tile_annotation pet_annotation;

static void
init_annotation(annotation, filename, colorpixel)
struct tile_annotation *annotation;
char *filename;
Pixel colorpixel;
{
    Display *dpy = XtDisplay(toplevel);

    if (0!=XReadBitmapFile(dpy, XtWindow(toplevel), filename,
	    &annotation->width, &annotation->height, &annotation->bitmap,
	    &annotation->hotx, &annotation->hoty)) {
	char buf[BUFSIZ];
	Sprintf(buf, "Failed to load %s", filename);
	X11_raw_print(buf);
    }

    annotation->foreground = colorpixel;
}

/*
 * Put the tile image on the server.
 *
 * We can't send the image to the server until the top level
 * is realized.  When the tile file is first processed, the top
 * level is not realized.  This routine is called after we
 * realize the top level, but before we start resizing the
 * map viewport.
 */
void
post_process_tiles()
{
    Display *dpy = XtDisplay(toplevel);
/*    unsigned int width, height;*/
    Colormap cmap;
#ifdef USE_XPM
    XpmAttributes attributes;
#endif
/*    int errorcode;*/
    Arg args[16];
    XGCValues val;

#ifdef USE_XPM
    if(tile_image.data){
      XtSetArg(args[0], XtNcolormap, &cmap);
      XtGetValues(toplevel, args, ONE);
      
      attributes.valuemask = XpmCloseness | XpmColormap;
      attributes.colormap = cmap;
      attributes.closeness = 25000;

      XpmCreatePixmapFromXpmImage(
		dpy,
		XtWindow(toplevel),
		&tile_image,
		&tile_pixmap,
		&tile_clipmask,
		&attributes
		);

      val.function = GXcopy;
      val.clip_mask = tile_clipmask;

      tile_gc = XCreateGC(
		dpy,
		XtWindow(toplevel),
		GCFunction | GCClipMask,
		&val
		);
		
      XpmFreeXpmImage(&tile_image);
    }
#endif
    init_annotation(&pet_annotation,
	appResources.pet_mark_bitmap, appResources.pet_mark_color);
}


/*
 * Open and read the tile file.  Return TRUE if there were no problems.
 * Return FALSE otherwise.
 */
static boolean
init_tiles(wp)
    struct xwindow *wp;
{
#ifndef USE_XPM
    FILE *fp = (FILE *)0;
    x11_header header;
    unsigned char *cp, *colormap = (unsigned char *)0;
    unsigned char *tb, *tile_bytes = (unsigned char *)0;
    int size;
    XColor *colors = (XColor *)0;
    int i, x, y;
    int bitmap_pad;
    unsigned int image_height, image_width;
    int ddepth;
#endif
    Display *dpy = XtDisplay(toplevel);
    Screen *screen = DefaultScreenOfDisplay(dpy);
    struct map_info_t *map_info = (struct map_info_t *)0;
    struct tile_map_info_t *tile_info = (struct tile_map_info_t *)0;
    boolean result = TRUE;
    XGCValues values;
    XtGCMask mask;

    /* already have tile information */
    if (tile_pixmap != None) goto tiledone;

    map_info = wp->map_information;
    tile_info = map_info->mtype.tile_map =
	    (struct tile_map_info_t *) alloc(sizeof(struct tile_map_info_t));
    (void) memset((genericptr_t) tile_info, 0,
				sizeof(struct tile_map_info_t));

#ifdef USE_XPM
    {
	int errorcode;
	char buf[BUFSIZ];

        errorcode = XpmReadFileToXpmImage(
		    appResources.tile_file,
		    &tile_image,
		    NULL
		    );

	if (errorcode!=XpmSuccess) {
#ifdef RADAR
	    USE_RADAR = 0;
#endif
	    if (errorcode == XpmColorFailed) {
		Sprintf(buf, "Insufficient colors available to load %s.",appResources.tile_file);
		X11_raw_print(buf);
	    } else {
		Sprintf(buf, "Failed to load %s: %s",appResources.tile_file,
			XpmGetErrorString(errorcode));
		X11_raw_print(buf);
	    }
	    result = FALSE;
	    X11_raw_print("Switching to text-based mode.");
	    goto tiledone;
	}
	TILE_PER_COL = tile_image.width / TILE_WIDTH;
#if 0
	if (tile_image->height%total_tiles_used != 0) {
	    char buf[BUFSIZ];
	    Sprintf(buf,
		"%s is not a multiple of %d (the number of tiles) pixels high",
		appResources.tile_file, total_tiles_used);
	    X11_raw_print(buf);
	    XDestroyImage(tile_image);
	    tile_image = 0;
	    result = FALSE;
	    goto tiledone;
	}

	/* infer tile dimensions from image size */
	tile_count=total_tiles_used;
	tile_width=tile_image->width;
	tile_height=tile_image->height/tile_count;
#endif
    }
#else
    /* any less than 16 colours makes tiles useless */
    ddepth = DefaultDepthOfScreen(screen);
    if (ddepth < 4) {
	X11_raw_print("need a screen depth of at least 4");
	result = FALSE;
	goto tiledone;
    }

    fp = fopen_datafile(appResources.tile_file, RDBMODE);
    if (!fp) {
	X11_raw_print("can't open tile file");
	result = FALSE;
	goto tiledone;
    }

    if (fread((char *) &header, sizeof(header), 1, fp) != 1) {
	X11_raw_print("read of header failed");
	result = FALSE;
	goto tiledone;
    }

# ifdef VERBOSE
    fprintf(stderr, "X11 tile file:\n    version %ld\n    ncolors %ld\n    tile width %ld\n    tile height %ld\n    ntiles %ld\n",
	header.version,
	header.ncolors,
	header.tile_width,
	header.tile_height,
	header.ntiles);
# endif

    size = 3*header.ncolors;
    colormap = (unsigned char *) alloc((unsigned)size);
    if (fread((char *) colormap, 1, size, fp) != size) {
	X11_raw_print("read of colormap failed");
	result = FALSE;
	goto tiledone;
    }

/* defined in decl.h - these are _not_ good defines to have */
#undef red
#undef green
#undef blue

    colors = (XColor *) alloc(sizeof(XColor) * (unsigned)header.ncolors);
    for (i = 0; i < header.ncolors; i++) {
	cp = colormap + (3 * i);
	colors[i].red   = cp[0] * 256;
	colors[i].green = cp[1] * 256;
	colors[i].blue  = cp[2] * 256;
	colors[i].flags = 0;
	colors[i].pixel = 0;

	if (!XAllocColor(dpy, DefaultColormapOfScreen(screen), &colors[i]) &&
	    !nhApproxColor(screen, DefaultColormapOfScreen(screen),
			   (char *)0, &colors[i])) {
	    char buf[BUFSIZ];
	    Sprintf(buf, "%dth out of %ld color allocation failed",
		i, header.ncolors);
	    X11_raw_print(buf);
	    result = FALSE;
	    goto tiledone;
	}
    }

    size = header.tile_height * header.tile_width;
    /*
     * This alloc() and the one below require 32-bit ints, since tile_bytes
     * is currently ~200k and alloc() takes an int
     */
    tile_bytes = (unsigned char *) alloc((unsigned)header.ntiles*size);
    tile_count = header.ntiles;
    if (fread((char *) tile_bytes, size, tile_count, fp) != tile_count) {
	X11_raw_print("read of tile bytes failed");
	result = FALSE;
	goto tiledone;
    }

    if (header.ntiles < total_tiles_used) {
	char buf[BUFSZ];
	Sprintf(buf, "tile file incomplete, expecting %d tiles, found %lu",
		total_tiles_used, header.ntiles);
	X11_raw_print(buf);
	result = FALSE;
	goto tiledone;
    }


    if (appResources.double_tile_size) {
	tile_width  = 2*header.tile_width;
	tile_height = 2*header.tile_height;
    } else {
	tile_width  = header.tile_width;
	tile_height = header.tile_height;
    }

    image_height = tile_height * tile_count;
    image_width  = tile_width;

    /* calculate bitmap_pad */
    if (ddepth > 16)
	bitmap_pad = 32;
    else if (ddepth > 8)
	bitmap_pad = 16;
    else
	bitmap_pad = 8;

    tile_image = XCreateImage(dpy, DefaultVisualOfScreen(screen),
		ddepth,			/* depth */
		ZPixmap,		/* format */
		0,			/* offset */
		0,			/* data */
		image_width,		/* width */
		image_height,		/* height */
		bitmap_pad,		/* bit pad */
		0);			/* bytes_per_line */

    if (!tile_image)
	impossible("init_tiles: insufficient memory to create image");

    /* now we know the physical memory requirements, we can allocate space */
    tile_image->data =
	(char *) alloc((unsigned)tile_image->bytes_per_line * image_height);

    if (appResources.double_tile_size) {
	unsigned long *expanded_row =
	    (unsigned long *)alloc(sizeof(unsigned long)*(unsigned)tile_width);

	tb = tile_bytes;
	for (y = 0; y < image_height; y++) {
	    for (x = 0; x < header.tile_width; x++)
		expanded_row[2*x] =
			    expanded_row[(2*x)+1] = colors[*tb++].pixel;

	    for (x = 0; x < tile_width; x++)
		XPutPixel(tile_image, x, y, expanded_row[x]);

	    y++;	/* duplicate row */
	    for (x = 0; x < tile_width; x++)
		XPutPixel(tile_image, x, y, expanded_row[x]);
	}
	free((genericptr_t)expanded_row);

    } else {

	for (tb = tile_bytes, y = 0; y < image_height; y++)
	    for (x = 0; x < image_width; x++, tb++)
		XPutPixel(tile_image, x, y, colors[*tb].pixel);
    }
#endif /* USE_XPM */

    /* fake an inverted tile by drawing a border around the edges */
#ifdef USE_WHITE
    /* use white or black as the border */
    mask = GCFunction | GCForeground | GCGraphicsExposures;
    values.graphics_exposures = False;
    values.foreground = WhitePixelOfScreen(screen);
    values.function   = GXcopy;
    tile_info->white_gc = XtGetGC(wp->w, mask, &values);
    values.graphics_exposures = False;
    values.foreground = BlackPixelOfScreen(screen);
    values.function   = GXcopy;
    tile_info->black_gc = XtGetGC(wp->w, mask, &values);
#else
    /*
     * Use xor so we don't have to check for special colors.  Xor white
     * against the upper left pixel of the corridor so that we have a
     * white rectangle when in a corridor.
     */
    mask = GCFunction | GCForeground | GCGraphicsExposures;
    values.graphics_exposures = False;
#if 1
    values.foreground = WhitePixelOfScreen(screen) ^
	XGetPixel(tile_image, 
                tile_width*(glyph2tile[cmap_to_glyph(S_corr)]%TILE_PER_COL),
                tile_height*(glyph2tile[cmap_to_glyph(S_corr)]/TILE_PER_COL));
#else
    values.foreground = ~((unsigned long) 0);
#endif
    values.function = GXxor;
    tile_info->white_gc = XtGetGC(wp->w, mask, &values);

    mask = GCFunction | GCGraphicsExposures;
    values.function = GXCopy;
    values.graphics_exposures = False;
    tile_info->black_gc = XtGetGC(wp->w, mask, &values);
#endif

tiledone:
#ifndef USE_XPM
    if (fp) (void) fclose(fp);
    if (colormap) free((genericptr_t)colormap);
    if (tile_bytes) free((genericptr_t)tile_bytes);
    if (colors) free((genericptr_t)colors);
/*ITA*/
    {int i; for(i=0;i<MAX_PXM_SLOTS;i++){ pxm_slot[i].age=0;
     pxm_slot[i].bg=pxm_slot[i].fg=-99;pxm_slot[i].pixmap=0;} }
/*ITA*/
#endif

    if (result) {				/* succeeded */
#if 0
	map_info->square_height = tile_height;
	map_info->square_width = tile_width;
#endif
	map_info->square_height = TILE_HEIGHT;
	map_info->square_width = TILE_WIDTH;
	map_info->square_ascent = 0;
	map_info->square_lbearing = 0;
    } else {
	if (tile_info) free((genericptr_t)tile_info);
	tile_info = 0;
    }

    return result;
}

#ifdef RADAR
void
check_sb(struct xwindow *wp)
{
    Arg arg[2];
    Widget viewport, horiz_sb, vert_sb;
    float top, shown;

    viewport = XtParent(wp->w);
    horiz_sb = XtNameToWidget(viewport, "horizontal");
    vert_sb  = XtNameToWidget(viewport, "vertical");

    if (horiz_sb) {
	XtSetArg(arg[0], XtNshown,	&shown);
	XtSetArg(arg[1], XtNtopOfThumb, &top);
	XtGetValues(horiz_sb, arg, TWO);
	radar_x = top * RADAR_WIDTH;
	radar_w = shown * RADAR_WIDTH;
    }
    if (vert_sb) {
	XtSetArg(arg[0], XtNshown,      &shown);
	XtSetArg(arg[1], XtNtopOfThumb, &top);
	XtGetValues(vert_sb, arg, TWO);
	radar_y = top * RADAR_HEIGHT;
	radar_h = shown * RADAR_HEIGHT;
    }
}
#endif

/*
 * Make sure the map's cursor is always visible.
 */
void
check_cursor_visibility(wp)
    struct xwindow *wp;
{
    Arg arg[2];
    Widget viewport, horiz_sb, vert_sb;
    float top, shown, cursor_middle;
    Boolean do_call, adjusted = False;
#ifdef VERBOSE
    char *s;
#endif

    viewport = XtParent(wp->w);
    horiz_sb = XtNameToWidget(viewport, "horizontal");
    vert_sb  = XtNameToWidget(viewport, "vertical");

/* All values are relative to currently visible area */

#define V_BORDER 0.3	/* if this far from vert edge, shift */
#define H_BORDER 0.3	/* if this from from horiz edge, shift */

#define H_DELTA 0.4	/* distance of horiz shift */
#define V_DELTA 0.4	/* distance of vert shift */

    if (horiz_sb) {
	XtSetArg(arg[0], XtNshown,	&shown);
	XtSetArg(arg[1], XtNtopOfThumb, &top);
	XtGetValues(horiz_sb, arg, TWO);
#ifdef RADAR
	radar_x = top * RADAR_WIDTH;
	radar_w = shown * RADAR_WIDTH;
#endif

	cursor_middle = (((float) wp->cursx) + 0.5) / (float) COLNO;
	do_call = True;

#ifdef VERBOSE
	if (cursor_middle < top) {
	    s = " outside left";
	} else if (cursor_middle < top + shown*H_BORDER) {
	    s = " close to left";
	} else if (cursor_middle > (top + shown)) {
	    s = " outside right";
	} else if (cursor_middle > (top + shown - shown*H_BORDER)) {
	    s = " close to right";
	} else {
	    s = "";
	}
	printf("Horiz: shown = %3.2f, top = %3.2f%s", shown, top, s);
#endif

	if (cursor_middle < top) {
	    top = cursor_middle - shown*H_DELTA;
	    if (top < 0.0) top = 0.0;
	} else if (cursor_middle < top + shown*H_BORDER) {
	    top -= shown*H_DELTA;
	    if (top < 0.0) top = 0.0;
	} else if (cursor_middle > (top + shown)) {
	    top = cursor_middle - shown*H_DELTA;
	    if (top < 0.0) top = 0.0;
	    if (top + shown > 1.0) top = 1.0 - shown;
	} else if (cursor_middle > (top + shown - shown*H_BORDER)) {
	    top += shown*H_DELTA;
	    if (top + shown > 1.0) top = 1.0 - shown;
	} else {
	    do_call = False;
	}

	if (do_call) {
	    XtCallCallbacks(horiz_sb, XtNjumpProc, &top);
	    adjusted = True;
	}
    }

    if (vert_sb) {
	XtSetArg(arg[0], XtNshown,      &shown);
	XtSetArg(arg[1], XtNtopOfThumb, &top);
	XtGetValues(vert_sb, arg, TWO);
#ifdef RADAR
	radar_y = top * RADAR_HEIGHT;
	radar_h = shown * RADAR_HEIGHT;
#endif

	cursor_middle = (((float) wp->cursy) + 0.5) / (float) ROWNO;
	do_call = True;

#ifdef VERBOSE
	if (cursor_middle < top) {
	    s = " above top";
	} else if (cursor_middle < top + shown*V_BORDER) {
	    s = " close to top";
	} else if (cursor_middle > (top + shown)) {
	    s = " below bottom";
	} else if (cursor_middle > (top + shown - shown*V_BORDER)) {
	    s = " close to bottom";
	} else {
	    s = "";
	}
	printf("%sVert: shown = %3.2f, top = %3.2f%s",
				    horiz_sb ? ";  " : "", shown, top, s);
#endif

	if (cursor_middle < top) {
	    top = cursor_middle - shown*V_DELTA;
	    if (top < 0.0) top = 0.0;
	} else if (cursor_middle < top + shown*V_BORDER) {
	    top -= shown*V_DELTA;
	    if (top < 0.0) top = 0.0;
	} else if (cursor_middle > (top + shown)) {
	    top = cursor_middle - shown*V_DELTA;
	    if (top < 0.0) top = 0.0;
	    if (top + shown > 1.0) top = 1.0 - shown;
	} else if (cursor_middle > (top + shown - shown*V_BORDER)) {
	    top += shown*V_DELTA;
	    if (top + shown > 1.0) top = 1.0 - shown;
	} else {
	    do_call = False;
	}

	if (do_call) {
	    XtCallCallbacks(vert_sb, XtNjumpProc, &top);
	    adjusted = True;
	}
    }

    /* make sure cursor is displayed during dowhatis.. */
    if (adjusted) display_cursor(wp);

#ifdef VERBOSE
    if (horiz_sb || vert_sb) printf("\n");
#endif
}


/*
 * Check to see if the viewport has grown smaller.  If so, then we want to make
 * sure that the cursor is still on the screen.  We do this to keep the cursor
 * on the screen when the user resizes the nethack window.
 */
static void
map_check_size_change(wp)
    struct xwindow *wp;
{
    struct map_info_t *map_info = wp->map_information;
    Arg arg[2];
    Dimension new_width, new_height;
    Widget viewport;

    viewport = XtParent(wp->w);

    XtSetArg(arg[0], XtNwidth,  &new_width);
    XtSetArg(arg[1], XtNheight, &new_height);
    XtGetValues(viewport, arg, TWO);

    /* Only do cursor check if new size is smaller. */
    if (new_width < map_info->viewport_width
		    || new_height < map_info->viewport_height) {
	check_cursor_visibility(wp);
    }

    map_info->viewport_width = new_width;
    map_info->viewport_height = new_height;
}

/*
 * Fill in parameters "regular" and "inverse" with newly created GCs.
 * Using the given background pixel and the foreground pixel optained
 * by querying the widget with the resource name.
 */
static void
set_gc(w, font, resource_name, bgpixel, regular, inverse)
    Widget w;
    Font font;
    char *resource_name;
    Pixel bgpixel;
    GC   *regular, *inverse;
{
    XGCValues values;
    XtGCMask mask = GCFunction | GCForeground | GCBackground | GCFont;
    Pixel curpixel;
    Arg arg[1];

    XtSetArg(arg[0], resource_name, &curpixel);
    XtGetValues(w, arg, ONE);

    values.foreground = curpixel;
    values.background = bgpixel;
    values.function   = GXcopy;
    values.font	      = font;
    *regular = XtGetGC(w, mask, &values);
    values.foreground = bgpixel;
    values.background = curpixel;
    values.function   = GXcopy;
    values.font	      = font;
    *inverse = XtGetGC(w, mask, &values);
}

/*
 * Create the GC's for each color.
 *
 * I'm not sure if it is a good idea to have a GC for each color (and
 * inverse). It might be faster to just modify the foreground and
 * background colors on the current GC as needed.
 */
static void
get_text_gc(wp, font)
    struct xwindow *wp;
    Font font;
{
    struct map_info_t *map_info = wp->map_information;
    Pixel bgpixel;
    Arg arg[1];

    /* Get background pixel. */
    XtSetArg(arg[0], XtNbackground, &bgpixel);
    XtGetValues(wp->w, arg, ONE);

#ifdef TEXTCOLOR
#define set_color_gc(nh_color, resource_name)			\
	    set_gc(wp->w, font, resource_name, bgpixel,		\
		&map_info->mtype.text_map->color_gcs[nh_color],	\
		    &map_info->mtype.text_map->inv_color_gcs[nh_color]);

    set_color_gc(CLR_BLACK,	XtNblack);
    set_color_gc(CLR_RED,	XtNred);
    set_color_gc(CLR_GREEN,	XtNgreen);
    set_color_gc(CLR_BROWN,	XtNbrown);
    set_color_gc(CLR_BLUE,	XtNblue);
    set_color_gc(CLR_MAGENTA,	XtNmagenta);
    set_color_gc(CLR_CYAN,	XtNcyan);
    set_color_gc(CLR_GRAY,	XtNgray);
    set_color_gc(NO_COLOR,	XtNforeground);
    set_color_gc(CLR_ORANGE,	XtNorange);
    set_color_gc(CLR_BRIGHT_GREEN, XtNbright_green);
    set_color_gc(CLR_YELLOW,	XtNyellow);
    set_color_gc(CLR_BRIGHT_BLUE, XtNbright_blue);
    set_color_gc(CLR_BRIGHT_MAGENTA, XtNbright_magenta);
    set_color_gc(CLR_BRIGHT_CYAN, XtNbright_cyan);
    set_color_gc(CLR_WHITE,	XtNwhite);
#else
    set_gc(wp->w, font, XtNforeground, bgpixel,
		&map_info->mtype.text_map->copy_gc,
		&map_info->mtype.text_map->inv_copy_gc);
#endif
}


/*
 * Display the cursor on the map window.
 */
static void
display_cursor(wp)
    struct xwindow *wp;
{
    /* Redisplay the cursor location inverted. */
    map_update(wp, wp->cursy, wp->cursy, wp->cursx, wp->cursx, TRUE);
}


/*
 * Check if there are any changed characters.  If so, then plaster them on
 * the screen.
 */
void
display_map_window(wp)
    struct xwindow *wp;
{
    register int row;
    struct map_info_t *map_info = wp->map_information;

    /*
     * If the previous cursor position is not the same as the current
     * cursor position, then update the old cursor position.
     */
    if (wp->prevx != wp->cursx || wp->prevy != wp->cursy) {
	register unsigned int x = wp->prevx, y = wp->prevy;
	if (x < map_info->t_start[y]) map_info->t_start[y] = x;
	if (x > map_info->t_stop[y])  map_info->t_stop[y]  = x;
    }

    for (row = 0; row < ROWNO; row++) {
	if (map_info->t_start[row] <= map_info->t_stop[row]) {
	    map_update(wp, row, row,
			(int) map_info->t_start[row],
			(int) map_info->t_stop[row], FALSE);
	    map_info->t_start[row] = COLNO-1;
	    map_info->t_stop[row] = 0;
	}
    }
    display_cursor(wp);
    wp->prevx = wp->cursx;	/* adjust old cursor position */
    wp->prevy = wp->cursy;
}

/*
 * Set all map tiles to S_stone
 */
static void
map_all_stone(map_info)
struct map_info_t *map_info;
{
    int i;
    unsigned short *sp, stone;
    stone = cmap_to_glyph(S_stone);

    for (sp = (unsigned short *) map_info->mtype.tile_map->glyphs, i = 0;
	i < ROWNO*COLNO; sp++, i++)
      *sp = stone;
}

/*
 * Fill the saved screen characters with the "clear" tile or character.
 *
 * Flush out everything by resetting the "new" bounds and calling
 * display_map_window().
 */
void
clear_map_window(wp)
    struct xwindow *wp;
{
    struct map_info_t *map_info = wp->map_information;

    if (map_info->is_tile) {
	map_all_stone(map_info);
    } else {
	/* Fill text with spaces, and update */
	(void) memset((genericptr_t) map_info->mtype.text_map->text, ' ',
			sizeof(map_info->mtype.text_map->text));
#ifdef TEXTCOLOR
	(void) memset((genericptr_t) map_info->mtype.text_map->colors, NO_COLOR,
			sizeof(map_info->mtype.text_map->colors));
#endif
    }

    /* force a full update */
    (void) memset((genericptr_t) map_info->t_start, (char) 0,
			sizeof(map_info->t_start));
    (void) memset((genericptr_t) map_info->t_stop, (char) COLNO-1,
			sizeof(map_info->t_stop));
    display_map_window(wp);
}

/*
 * Retreive the font associated with the map window and save attributes
 * that are used when updating it.
 */
static void
get_char_info(wp)
    struct xwindow *wp;
{
    XFontStruct *fs;
    struct map_info_t *map_info = wp->map_information;

    fs = WindowFontStruct(wp->w);
    map_info->square_width    = fs->max_bounds.width;
    map_info->square_height   = fs->max_bounds.ascent + fs->max_bounds.descent;
    map_info->square_ascent   = fs->max_bounds.ascent;
    map_info->square_lbearing = -fs->min_bounds.lbearing;

#ifdef VERBOSE
    printf("Font information:\n");
    printf("fid = %d, direction = %d\n", fs->fid, fs->direction);
    printf("first = %d, last = %d\n",
			fs->min_char_or_byte2, fs->max_char_or_byte2);
    printf("all chars exist? %s\n", fs->all_chars_exist?"yes":"no");
    printf("min_bounds:lb=%d rb=%d width=%d asc=%d des=%d attr=%d\n",
		fs->min_bounds.lbearing, fs->min_bounds.rbearing,
		fs->min_bounds.width, fs->min_bounds.ascent,
		fs->min_bounds.descent, fs->min_bounds.attributes);
    printf("max_bounds:lb=%d rb=%d width=%d asc=%d des=%d attr=%d\n",
		fs->max_bounds.lbearing, fs->max_bounds.rbearing,
		fs->max_bounds.width, fs->max_bounds.ascent,
		fs->max_bounds.descent, fs->max_bounds.attributes);
    printf("per_char = 0x%x\n", fs->per_char);
    printf("Text: (max) width = %d, height = %d\n",
	    map_info->square_width, map_info->square_height);
#endif

    if (fs->min_bounds.width != fs->max_bounds.width)
	X11_raw_print("Warning:  map font is not monospaced!");
}

/*
 * keyhit buffer
 */
#define INBUF_SIZE 64
int inbuf[INBUF_SIZE];
int incount = 0;
int inptr = 0;	/* points to valid data */


/*
 * Keyboard and button event handler for map window.
 */
void
map_input(w, event, params, num_params)
    Widget   w;
    XEvent   *event;
    String   *params;
    Cardinal *num_params;
{
    XKeyEvent *key;
    XButtonEvent *button;
    boolean meta = FALSE;
    int i, nbytes;
    Cardinal in_nparams = (num_params ? *num_params : 0);
    char c;
    char keystring[MAX_KEY_STRING];
    /* JP */
    KeySym keysym = 0;

    switch (event->type) {
	case ButtonPress:
	    button = (XButtonEvent *) event;
#ifdef VERBOSE_INPUT
	    printf("button press\n");
#endif
	    if (in_nparams > 0 &&
		(nbytes = strlen(params[0])) < MAX_KEY_STRING) {
		Strcpy(keystring, params[0]);
		key = (XKeyEvent *) event; /* just in case */
		goto key_events;
	    }
	    if (w != window_list[WIN_MAP].w) {
#ifdef VERBOSE_INPUT
		printf("map_input called from wrong window\n");
#endif
		X11_nhbell();
		return;
	    }
	    set_button_values(w, button->x, button->y, button->button);
	    break;
	case KeyPress:
#ifdef VERBOSE_INPUT
	    printf("key: ");
#endif
	    if(appResources.slow && input_func) {
		(*input_func)(w, event, params, num_params);
		break;
	    }

	    /*
	     * Don't use key_event_to_char() because we want to be able
	     * to allow keys mapped to multiple characters.
	     */

	    key = (XKeyEvent *) event;
	    if (in_nparams > 0 &&
		(nbytes = strlen(params[0])) < MAX_KEY_STRING) {
		Strcpy(keystring, params[0]);
	    } else {
		/*
		 * Assume that mod1 is really the meta key.
		 */
		meta = !!(key->state & Mod1Mask);
		nbytes =
		  /*JP
		    XLookupString(key, keystring, MAX_KEY_STRING,
				  (KeySym *)0, (XComposeStatus *)0);
		  */
		    XLookupString(key, keystring, MAX_KEY_STRING,
				  &keysym, (XComposeStatus *)0);
	    }
	    /*
	      ������
	     */
	    if(!iflags.num_pad){
	      if(keysym == XK_KP_1 || keysym == XK_KP_End){
		keystring[0] = 'b';
		nbytes = 1;
	      }
	      else if(keysym == XK_KP_2 || keysym == XK_KP_Down){
		keystring[0] = 'j';
		nbytes = 1;
	      }
	      else if(keysym == XK_KP_3 || keysym == XK_KP_Page_Down){
		keystring[0] = 'n';
		nbytes = 1;
	      }
	      else if(keysym == XK_KP_4 || keysym == XK_KP_Left){
		keystring[0] = 'h';
		nbytes = 1;
	      }
	      else if(keysym == XK_KP_5 || keysym == XK_KP_Begin){
		keystring[0] = '.';
		nbytes = 1;
	      }
	      else if(keysym == XK_KP_6 || keysym == XK_KP_Right){
		keystring[0] = 'l';
		nbytes = 1;
	      }
	      else if(keysym == XK_KP_7 || keysym == XK_KP_Home){
		keystring[0] = 'y';
		nbytes = 1;
	      }
	      else if(keysym == XK_KP_8 || keysym == XK_KP_Up){
		keystring[0] = 'k';
		nbytes = 1;
	      }
	      else if(keysym == XK_KP_9 || keysym == XK_KP_Page_Up){
		keystring[0] = 'u';
		nbytes = 1;
	      }
	    }

	key_events:
	    /* Modifier keys return a zero length string when pressed. */
	    if (nbytes) {
#ifdef VERBOSE_INPUT
		printf("\"");
#endif
		for (i = 0; i < nbytes; i++) {
		    c = keystring[i];

		    if (incount < INBUF_SIZE) {
			inbuf[(inptr+incount)%INBUF_SIZE] =
			    ((int) c) + (meta ? 0x80 : 0);
			incount++;
		    } else {
			X11_nhbell();
		    }
#ifdef VERBOSE_INPUT
		    if (meta)			/* meta will print as M<c> */
			(void) putchar('M');
		    if (c < ' ') {		/* ctrl will print as ^<c> */
			(void) putchar('^');
			c += '@';
		    }
		    (void) putchar(c);
#endif
		}
#ifdef VERBOSE_INPUT
		printf("\" [%d bytes]\n", nbytes);
#endif
	    }
	    break;

	default:
	    impossible("unexpected X event, type = %d\n", (int) event->type);
	    break;
    }
}

static void
set_button_values(w, x, y, button)
    Widget w;
    int x;
    int y;
    unsigned int button;
{
    struct xwindow *wp;
    struct map_info_t *map_info;

    wp = find_widget(w);
    map_info = wp->map_information;

    click_x = x / map_info->square_width;
    click_y = y / map_info->square_height;

    /* The values can be out of range if the map window has been resized */
    /* to be larger than the max size.					 */
    if (click_x >= COLNO) click_x = COLNO-1;
    if (click_y >= ROWNO) click_x = ROWNO-1;

    /* Map all buttons but the first to the second click */
    click_button = (button == Button1) ? CLICK_1 : CLICK_2;
}

/*
 * Map window expose callback.
 */
/*ARGSUSED*/
static void
map_exposed(w, client_data, widget_data)
    Widget w;
    XtPointer client_data;	/* unused */
    XtPointer widget_data;	/* expose event from Window widget */
{
    int x, y;
    struct xwindow *wp;
    struct map_info_t *map_info;
    unsigned width, height;
    int start_row, stop_row, start_col, stop_col;
    XExposeEvent *event = (XExposeEvent *) widget_data;
    int t_height, t_width;	/* tile/text height & width */

    if (!XtIsRealized(w) || event->count > 0) return;

    wp = find_widget(w);
    map_info = wp->map_information;
    if (wp->keep_window && !map_info) return;
    /*
     * The map is sent an expose event when the viewport resizes.  Make sure
     * that the cursor is still in the viewport after the resize.
     */
    map_check_size_change(wp);

    if (event) {		/* called from button-event */
	x      = event->x;
	y      = event->y;
	width  = event->width;
	height = event->height;
    } else {
	x     = 0;
	y     = 0;
	width = wp->pixel_width;
	height= wp->pixel_height;
    }
    /*
     * Convert pixels into INCLUSIVE text rows and columns.
     */
    t_height = map_info->square_height;
    t_width = map_info->square_width;
    start_row = y / t_height;
    stop_row = ((y + height) / t_height) +
		((((y + height) % t_height) == 0) ? 0 : 1) - 1;

    start_col = x / t_width;
    stop_col = ((x + width) / t_width) +
		((((x + width) % t_width) == 0) ? 0 : 1) - 1;

#ifdef VERBOSE
    printf("map_exposed: x = %d, y = %d, width = %d, height = %d\n",
						    x, y, width, height);
    printf("chars %d x %d, rows %d to %d, columns %d to %d\n",
			map_info->square_height, map_info->square_width,
			start_row, stop_row, start_col, stop_col);
#endif

    /* Out of range values are possible if the map window is resized to be */
    /* bigger than the largest expected value.				   */
    if (stop_row >= ROWNO) stop_row = ROWNO-1;
    if (stop_col >= COLNO) stop_col = COLNO-1;

    map_update(wp, start_row, stop_row, start_col, stop_col, FALSE);
    display_cursor(wp);		/* make sure cursor shows up */
}

/*
 * Do the actual work of the putting characters onto our X window.  This
 * is called from the expose event routine, the display window (flush)
 * routine, and the display cursor routine.  The later is a kludge that
 * involves the inverted parameter of this function.  A better solution
 * would be to double the color count, with any color above CLR_MAX
 * being inverted.
 *
 * This works for rectangular regions (this includes one line rectangles).
 * The start and stop columns are *inclusive*.
 */
static void
map_update(wp, start_row, stop_row, start_col, stop_col, inverted)
    struct xwindow *wp;
    int start_row, stop_row, start_col, stop_col;
    boolean inverted;
{
    int win_start_row, win_start_col;
    struct map_info_t *map_info = wp->map_information;
    int row;
    register int count;

#ifdef RADAR
    Arg args[16];
    Cardinal num_args;
    Dimension width, height;
    int depth;
    Display *dpy;
    XGCValues gcval;
    Colormap cmap;

    dpy = XtDisplay(radar);

    if(USE_RADAR){
      int i;
    
      if(flags.radar && !radar_is_popuped){
	  XtPopup(radar_popup, XtGrabNone);
	  radar_is_popuped = TRUE;
      }
      if(!flags.radar && radar_is_popuped){
	  XtPopdown(radar_popup);
	  radar_is_popuped = FALSE;
      }

      if(!radar_is_initialized){
	  radar_is_initialized = TRUE;

	  XtRealizeWidget(radar_popup);

	  num_args = 0;
	  XtSetArg(args[num_args], XtNwidth, &width); ++num_args;
	  XtSetArg(args[num_args], XtNheight, &height); ++num_args;

	  XtGetValues(radar, args, num_args);
	  depth = DefaultDepth(dpy, DefaultScreen(dpy));
	  radar_window = XtWindow(radar);
	  radar_pixmap = XCreatePixmap(dpy, radar_window,
				       width, height, depth);
	  radar_pixmap2 = XCreatePixmap(dpy, radar_window,
					width, height, depth);
	  
	  num_args = 0;
	  XtSetArg(args[num_args], XtNbackgroundPixmap, radar_pixmap); num_args++;
	  XtSetValues(radar, args, num_args);
	  
	  num_args = 0;
	  XtSetArg(args[num_args], XtNcolormap, &cmap); num_args++;
	  XtGetValues(radar, args, num_args);
	  
	  gcval.function = GXcopy;
	  for(i=0 ; i<16 ; ++i){
	      XAllocColor(dpy, cmap, &radar_color[i]);
	      
	      gcval.foreground = radar_color[i].pixel;
	      radar_gc[i] = XCreateGC(dpy, radar_window, 
				      GCFunction | GCForeground,
				      &gcval);
	  }
	  radar_gc_black = radar_gc[RADAR_BLACK];
	  radar_gc_white = radar_gc[RADAR_WHITE];

	  XFillRectangle(dpy, radar_pixmap,
			 radar_gc_black, 0, 0, width, height);
	  XFillRectangle(dpy, radar_pixmap2,
			 radar_gc_black, 0, 0, width, height);
      }
    }
#endif

    if (start_row < 0 || stop_row >= ROWNO) {
	impossible("map_update:  bad row range %d-%d\n", start_row, stop_row);
	return;
    }
    if (start_col < 0 || stop_col >=COLNO) {
	impossible("map_update:  bad col range %d-%d\n", start_col, stop_col);
	return;
    }

#ifdef VERBOSE_UPDATE
    printf("update: [0x%x] %d %d %d %d\n",
		(int) wp->w, start_row, stop_row, start_col, stop_col);
#endif
    win_start_row = start_row;
    win_start_col = start_col;

    if (map_info->is_tile) {
	struct tile_map_info_t *tile_map = map_info->mtype.tile_map;
	int cur_col;
	Display* dpy=XtDisplay(wp->w);
	Screen* screen=DefaultScreenOfDisplay(dpy);
/*ITA*/
         /* each slots ages */
        {
	     int i; 
	     
	     for(i=0 ; i<MAX_PXM_SLOTS ; i++)
		  pxm_slot[i].age++;
	}
/*ITA*/
	for (row = start_row; row <= stop_row; row++) {
	    for (cur_col = start_col; cur_col <= stop_col; cur_col++) {
		struct rm *lev = &levl[cur_col][row];
		int glyph = tile_map->glyphs[row][cur_col];
		int bg = back_to_glyph(cur_col, row);
		int tile = 0;
		int bgtile = 0;
		int dest_x = 0;
		int dest_y = 0;
		int src_x;
		int src_y;
		int bgsrc_x;
		int bgsrc_y;
		
		if(tile_pixmap){
		    if(youmonst.data && (Blind || (viz_array && !cansee(cur_col, row))))
		      bg = lev->glyph;

		  bgtile = glyph2tile[bg];
		  tile = glyph2tile[glyph];
		  dest_x = cur_col * map_info->square_width;
		  dest_y = row * map_info->square_height;
		  bgsrc_x = (bgtile % TILE_PER_COL) * TILE_WIDTH;
		  bgsrc_y = (bgtile / TILE_PER_COL) * TILE_HEIGHT;
		  src_x = (tile % TILE_PER_COL) * TILE_WIDTH;
		  src_y = (tile / TILE_PER_COL) * TILE_HEIGHT;
/*ITA*/
   {
     int i, match;
     int maxage = 0;

     if(bgtile != -1){
       match = -1;
       for(i=0 ; i<MAX_PXM_SLOTS ; i++){
	 if(tile== pxm_slot[i].fg && bgtile== pxm_slot[i].bg){
	   match = i;
	   break;
	 }
       }
       if(match == -1){
           /* no match found:dispose the oldest slot and compose pixmap */
	 for(i=0 ; i<MAX_PXM_SLOTS ; i++)
           if(maxage < pxm_slot[i].age){
	     match = i;
	     maxage = pxm_slot[i].age;
	   }
           if(!pxm_slot[match].pixmap) 
	     pxm_slot[match].pixmap = XCreatePixmap(
		 dpy, XtWindow(toplevel),
		 TILE_WIDTH, TILE_HEIGHT, DefaultDepth(dpy, DefaultScreen(dpy)));
           XCopyArea(dpy, tile_pixmap, pxm_slot[match].pixmap,
		     tile_map->black_gc,
		     bgsrc_x, bgsrc_y,
		     TILE_WIDTH, TILE_HEIGHT,
		     0,0);

           XSetClipOrigin(dpy,tile_gc, 0 - src_x, 0 - src_y);

           XCopyArea(dpy, tile_pixmap, pxm_slot[match].pixmap,
                              tile_gc,
                              src_x, src_y,
                              TILE_WIDTH, TILE_HEIGHT,
                              0,0);
           pxm_slot[match].fg=tile;
           pxm_slot[match].bg=bgtile;
       }
    /* slot ready */
       pxm_slot[match].age=0;
       XCopyArea(dpy, pxm_slot[match].pixmap, XtWindow(wp->w),
		 tile_map->black_gc,
		 0,0,
		 TILE_WIDTH, TILE_HEIGHT,
		 dest_x,dest_y);
     }
     else{
    /* no clip mask */
	 XCopyArea(dpy, tile_pixmap, XtWindow(wp->w),
		   tile_map->black_gc,
		   src_x, src_y,
		   TILE_WIDTH, TILE_HEIGHT,
		   dest_x,dest_y);
     }
}
/*ITA*/
}
#ifdef RADAR
		if(tile_pixmap && USE_RADAR){
#define RADAR_MONSTER	RADAR_YELLOW
#define RADAR_HUMAN	RADAR_WHITE
#define RADAR_OBJECT	RADAR_BLUE

#define RADAR_WALL	RADAR_GRAY
#define RADAR_FLOOR	RADAR_DARKGREEN
#define RADAR_DOOR	RADAR_ORANGE
#define RADAR_LADDER	RADAR_MAGENTA
#define RADAR_WATER	RADAR_BLUE
#define RADAR_TRAP	RADAR_RED

		  int c;
		  c = RADAR_BLACK;
		  if(tile < 290)
		    c = RADAR_MONSTER;
		  else if(tile < 345)
		    c = RADAR_HUMAN;
		  else if(tile < 345 + 394)
		    c = RADAR_OBJECT;
		  else if(tile < 345 + 394 + 1)
		    ;
		  else if(tile < 345 + 394 + 12)
		    c = RADAR_WALL;
		  else if(tile < 345 + 394 + 15)
		    c = RADAR_FLOOR;
		  else if(tile < 345 + 394 + 17)
		    c = RADAR_DOOR;
		  else if(tile < 345 + 394 + 20)
		    c = RADAR_FLOOR;
		  else if(tile < 345 + 394 + 24)
		    c = RADAR_LADDER;
		  else if(tile < 345 + 394 + 29)
		    c = RADAR_WATER;
		  else if(tile < 345 + 394 + 30)
		    c = RADAR_GRAY;
		  else if(tile < 345 + 394 + 31)
		    c = RADAR_ORANGE;
		  else if(tile < 345 + 394 + 35)
		    c = RADAR_ORANGE;
		  else if(tile < 345 + 394 + 38)
		    c = RADAR_CYAN;
		  else if(tile < 345 + 394 + 60)
		    c = RADAR_TRAP;
		  else if(tile < 345 + 394 + 121)
		    c = RADAR_YELLOW;
		  else
		    c = RADAR_WALL;

		  XFillRectangle(dpy, radar_pixmap2, radar_gc[c],
				 cur_col * 4, row * 4, 4, 4);
		}
#endif
		if (glyph_is_pet(glyph)
#ifdef TEXTCOLOR
			&& iflags.hilite_pet
#endif
			) {
		    /* draw pet annotation (a heart) */
		    XSetForeground(dpy, tile_map->black_gc, pet_annotation.foreground);
		    XSetClipOrigin(dpy, tile_map->black_gc, dest_x, dest_y);
		    XSetClipMask(dpy, tile_map->black_gc, pet_annotation.bitmap);
		    XCopyPlane(
			dpy,
			pet_annotation.bitmap,
			XtWindow(wp->w),
			tile_map->black_gc,
			0,0,
			pet_annotation.width,pet_annotation.height,
			dest_x,dest_y,
			1
		    );
		    XSetClipOrigin(dpy, tile_map->black_gc, 0, 0);
		    XSetClipMask(dpy, tile_map->black_gc, None);
		    XSetForeground(dpy, tile_map->black_gc, BlackPixelOfScreen(screen));
		}
	    }
	}

	if (inverted) {
	    XDrawRectangle(XtDisplay(wp->w), XtWindow(wp->w),
#ifdef USE_WHITE
		/* kludge for white square... */
		tile_map->glyphs[start_row][start_col] ==
		    cmap_to_glyph(S_ice) ?
			tile_map->black_gc : tile_map->white_gc,
#else
		tile_map->white_gc,
#endif
		start_col * map_info->square_width,
		start_row * map_info->square_height,
		map_info->square_width-1,
		map_info->square_height-1);
	}
#ifdef RADAR
	if(USE_RADAR){
	  XCopyArea(dpy, radar_pixmap2, radar_pixmap, radar_gc_black,
		    0, 0, RADAR_WIDTH, RADAR_HEIGHT, 0, 0);
	  check_sb(wp);
	  XDrawRectangle(dpy, radar_pixmap, radar_gc_white,
			 radar_x, radar_y,
			 radar_w, radar_h);
	  XCopyArea(dpy, radar_pixmap, radar_window, radar_gc_black,
		    0, 0, RADAR_WIDTH, RADAR_HEIGHT, 0, 0);
	  XRaiseWindow(dpy, XtWindow(radar_popup));
	}
#endif
    } else {
	struct text_map_info_t *text_map = map_info->mtype.text_map;

#ifdef TEXTCOLOR
	if (iflags.use_color) {
	    register char *c_ptr;
	    char *t_ptr;
	    int cur_col, color, win_ystart;

	    for (row = start_row; row <= stop_row; row++) {
		win_ystart = map_info->square_ascent +
					(row * map_info->square_height);

		t_ptr = (char *) &(text_map->text[row][start_col]);
		c_ptr = (char *) &(text_map->colors[row][start_col]);
		cur_col = start_col;
		while (cur_col <= stop_col) {
		    color = *c_ptr++;
		    count = 1;
		    while ((cur_col + count) <= stop_col && *c_ptr == color) {
			count++;
			c_ptr++;
		    }

		    XDrawImageString(XtDisplay(wp->w), XtWindow(wp->w),
			inverted ? text_map->inv_color_gcs[color] :
				   text_map->color_gcs[color],
			map_info->square_lbearing + (map_info->square_width * cur_col),
			win_ystart,
			t_ptr, count);

		    /* move text pointer and column count */
		    t_ptr += count;
		    cur_col += count;
		} /* col loop */
	    } /* row loop */
	} else
#endif /* TEXTCOLOR */
	{
	    int win_row, win_xstart;

	    /* We always start at the same x window position and have	*/
	    /* the same character count.				*/
	    win_xstart = map_info->square_lbearing +
				    (win_start_col * map_info->square_width);
	    count = stop_col - start_col + 1;

	    for (row = start_row, win_row = win_start_row;
					row <= stop_row; row++, win_row++) {

		XDrawImageString(XtDisplay(wp->w), XtWindow(wp->w),
		    inverted ? text_map->inv_copy_gc : text_map->copy_gc,
		    win_xstart,
		    map_info->square_ascent + (win_row * map_info->square_height),
		    (char *) &(text_map->text[row][start_col]), count);
	    }
	}
    }
}


/* Adjust the number of rows and columns on the given map window */
void
set_map_size(wp, cols, rows)
    struct xwindow *wp;
    Dimension cols, rows;
{
    Arg args[4];
    Cardinal num_args;

    wp->pixel_width  = wp->map_information->square_width  * cols;
    wp->pixel_height = wp->map_information->square_height * rows;

    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, wp->pixel_width);   num_args++;
    XtSetArg(args[num_args], XtNheight, wp->pixel_height); num_args++;
    XtSetValues(wp->w, args, num_args);
}


static void
init_text(wp)
    struct xwindow *wp;
{

    struct map_info_t *map_info = wp->map_information;
    struct text_map_info_t *text_map;

    map_info->is_tile = FALSE;
    text_map = map_info->mtype.text_map =
	(struct text_map_info_t *) alloc(sizeof(struct text_map_info_t));

    (void) memset((genericptr_t) text_map->text, ' ', sizeof(text_map->text));
#ifdef TEXTCOLOR
    (void) memset((genericptr_t) text_map->colors, NO_COLOR,
			sizeof(text_map->colors));
#endif

    get_char_info(wp);
    get_text_gc(wp, WindowFont(wp->w));
}

static char map_translations[] =
"#override\n\
 <Key>Left: scroll(4)\n\
 <Key>Right: scroll(6)\n\
 <Key>Up: scroll(8)\n\
 <Key>Down: scroll(2)\n\
 <Key>:		input()	\
";

#ifdef RADAR
static char radar_translations[] =
"#override\n\
 <Key>Left: scroll(4)\n\
 <Key>Right: scroll(6)\n\
 <Key>Up: scroll(8)\n\
 <Key>Down: scroll(2)\n\
 <Key>Escape: dismiss_radar()\n\
 <Key>: input()\
";
#endif

#ifdef RADAR
void
dismiss_radar(Widget w, XEvent*ev , String *s, Cardinal*n)
{
    XtPopdown(radar_popup);
    flags.radar = FALSE;
    radar_is_popuped = FALSE;
}
#endif

/*
 * The map window creation routine.
 */
void
create_map_window(wp, create_popup, parent)
    struct xwindow *wp;
    boolean create_popup;	/* parent is a popup shell that we create */
    Widget parent;
{
    struct map_info_t *map_info;	/* map info pointer */
    Widget map, viewport;
    Arg args[16];
    Cardinal num_args;
    Dimension rows, columns;
#if 0
    int screen_width, screen_height;
#endif

#ifdef RADAR
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, RADAR_WIDTH); num_args++;
    XtSetArg(args[num_args], XtNheight, RADAR_HEIGHT); num_args++;

    radar_popup = XtCreatePopupShell("radar",
				     /*
				     trainsientShellWidgetClass,
				     */
				     topLevelShellWidgetClass,
				     toplevel, args, num_args);

    num_args = 0;
    /*
    XtSetArg(args[num_args], XtNsensitive, False); num_args++;
    */
    XtSetArg(args[num_args], XtNlabel, ""); num_args++;
    XtSetArg(args[num_args], XtNtranslations,
	     XtParseTranslationTable(radar_translations));	num_args++;

    radar = XtCreateManagedWidget("radar",
				  labelWidgetClass,
				  radar_popup,		
				  args,			
				  num_args);
#endif

    wp->type = NHW_MAP;

    if (create_popup) {
	/*
	 * Create a popup that accepts key and button events.
	 */
	num_args = 0;
	XtSetArg(args[num_args], XtNinput, False);            num_args++;

	wp->popup = parent = XtCreatePopupShell("jnethack",
					topLevelShellWidgetClass,
				       toplevel, args, num_args);
	/*
	 * If we're here, then this is an auxiliary map window.  If we're
	 * cancelled via a delete window message, we should just pop down.
	 */
    }

    num_args = 0;
    XtSetArg(args[num_args], XtNallowHoriz, True);	num_args++;
    XtSetArg(args[num_args], XtNallowVert,  True);	num_args++;
    /* XtSetArg(args[num_args], XtNforceBars,  True);	num_args++; */
    XtSetArg(args[num_args], XtNuseBottom,  True);	num_args++;
    XtSetArg(args[num_args], XtNtranslations,
		XtParseTranslationTable(map_translations));	num_args++;
    viewport = XtCreateManagedWidget(
			"map_viewport",		/* name */
			viewportWidgetClass,	/* widget class from Window.h */
			parent,			/* parent widget */
			args,			/* set some values */
			num_args);		/* number of values to set */

    /*
     * Create a map window.  We need to set the width and height to some
     * value when we create it.  We will change it to the value we want
     * later
     */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth,  100); num_args++;
    XtSetArg(args[num_args], XtNheight, 100); num_args++;
    XtSetArg(args[num_args], XtNtranslations,
		XtParseTranslationTable(map_translations));	num_args++;

    wp->w = map = XtCreateManagedWidget(
		"map",			/* name */
		windowWidgetClass,	/* widget class from Window.h */
		viewport,		/* parent widget */
		args,			/* set some values */
		num_args);		/* number of values to set */

    XtAddCallback(map, XtNexposeCallback, map_exposed, (XtPointer) 0);

    map_info = wp->map_information =
			(struct map_info_t *) alloc(sizeof(struct map_info_t));

    map_info->viewport_width = map_info->viewport_height = 0;

    /* reset the "new entry" indicators */
    (void) memset((genericptr_t) map_info->t_start, (char) COLNO,
			sizeof(map_info->t_start));
    (void) memset((genericptr_t) map_info->t_stop, (char) 0,
			sizeof(map_info->t_stop));

    /* we probably want to restrict this to the 1st map window only */
    if (appResources.tile_file[0] && init_tiles(wp)) {
	map_info->is_tile = TRUE;
    } else {
	init_text(wp);
	map_info->is_tile = FALSE;
    }


    /*
     * Initially, set the map widget to be the size specified by the
     * widget rows and columns resources.  We need to do this to
     * correctly set the viewport window size.  After the viewport is
     * realized, then the map can resize to its normal size.
     */
    num_args = 0;
    XtSetArg(args[num_args], XtNrows,    &rows);	num_args++;
    XtSetArg(args[num_args], XtNcolumns, &columns);	num_args++;
    XtGetValues(wp->w, args, num_args);

    /* Don't bother with windows larger than ROWNOxCOLNO. */
    if (columns > COLNO) columns = COLNO;
    if (rows    > ROWNO) rows = ROWNO;

#if 0 /* This is insufficient.  We now resize final window in winX.c */
    /*
     * Check for overrunning the size of the screen.  This does an ad hoc
     * job.
     *
     * Width:	We expect that there is nothing but borders on either side
     *		of the map window.  Use some arbitrary width to decide
     *		when to shrink.
     *
     * Height:	if the map takes up more than 1/2 of the screen height, start
     *		reducing its size.
     */
    screen_height = HeightOfScreen(XtScreen(wp->w));
    screen_width  = WidthOfScreen(XtScreen(wp->w));

#define WOFF 50
    if ((int)(columns*map_info->square_width) > screen_width-WOFF) {
	columns = (screen_width-WOFF) / map_info->square_width;
	if (columns == 0) columns = 1;
    }

    if ((int)(rows*map_info->square_height) > screen_height/2) {
	rows = screen_height / (2*map_info->square_height);
	if (rows == 0) rows = 1;
    }
#endif

    set_map_size(wp, columns, rows);

    /*
     * If we have created our own popup, then realize it so that the
     * viewport is also realized.  Then resize the map window.
     */
    if (create_popup) {
	XtRealizeWidget(wp->popup);
	XSetWMProtocols(XtDisplay(wp->popup), XtWindow(wp->popup),
			&wm_delete_window, 1);
	set_map_size(wp, COLNO, ROWNO);
    }

    if (map_info->is_tile) {
	map_all_stone(map_info);
    }
}

/*
 * Destroy this map window.
 */
void
destroy_map_window(wp)
    struct xwindow *wp;
{
    struct map_info_t *map_info = wp->map_information;

    if (wp->popup)
	nh_XtPopdown(wp->popup);

    if (map_info) {
	struct text_map_info_t *text_map = map_info->mtype.text_map;

	/* Free allocated GCs. */
	if (!map_info->is_tile) {
#ifdef TEXTCOLOR
	    int i;

	    for (i = 0; i < CLR_MAX; i++) {
		XtReleaseGC(wp->w, text_map->color_gcs[i]);
		XtReleaseGC(wp->w, text_map->inv_color_gcs[i]);
	    }
#else
	    XtReleaseGC(wp->w, text_map->copy_gc);
	    XtReleaseGC(wp->w, text_map->inv_copy_gc);
#endif
	}
	/* free alloc'ed text information */
	free((genericptr_t)text_map),   map_info->mtype.text_map = 0;

	/* Free malloc'ed space. */
	free((genericptr_t)map_info),  wp->map_information = 0;
    }

	/* Destroy map widget. */
    if (wp->popup && !wp->keep_window)
	XtDestroyWidget(wp->popup),  wp->popup = (Widget)0;

    if (wp->keep_window)
	XtRemoveCallback(wp->w, XtNexposeCallback, map_exposed, (XtPointer)0);
    else
	wp->type = NHW_NONE;	/* allow re-use */
}



boolean exit_x_event;	/* exit condition for the event loop */
/*******
pkey(k)
    int k;
{
    printf("key = '%s%c'\n", (k<32) ? "^":"", (k<32) ? '@'+k : k);
}
******/

/*
 * Main X event loop.  Here we accept and dispatch X events.  We only exit
 * under certain circumstances.
 */
int
x_event(exit_condition)
    int exit_condition;
{
    XEvent  event;
    int     retval = 0;
    boolean keep_going = TRUE;

    /* Hold globals so function is re-entrant */
    boolean hold_exit_x_event = exit_x_event;

    click_button = NO_CLICK;	/* reset click exit condition */
    exit_x_event = FALSE;	/* reset callback exit condition */

    /*
     * Loop until we get a sent event, callback exit, or are accepting key
     * press and button press events and we receive one.
     */
    if((exit_condition == EXIT_ON_KEY_PRESS ||
	exit_condition == EXIT_ON_KEY_OR_BUTTON_PRESS) && incount)
	goto try_test;

    do {
	XtAppNextEvent(app_context, &event);
	XtDispatchEvent(&event);

	/* See if we can exit. */
    try_test:
	switch (exit_condition) {
	    case EXIT_ON_SENT_EVENT: {
		XAnyEvent *any = (XAnyEvent *) &event;
		if (any->send_event) {
		    retval = 0;
		    keep_going = FALSE;
		}
		break;
	    }
	    case EXIT_ON_EXIT:
		if (exit_x_event) {
		    incount = 0;
		    retval = 0;
		    keep_going = FALSE;
		}
		break;
	    case EXIT_ON_KEY_PRESS:
		if (incount != 0) {
		    /* get first pressed key */
		    --incount;
		    retval = inbuf[inptr];
		    inptr = (inptr+1) % INBUF_SIZE;
		    /* pkey(retval); */
		    keep_going = FALSE;
		}
		break;
	    case EXIT_ON_KEY_OR_BUTTON_PRESS:
		if (incount != 0 || click_button != NO_CLICK) {
		    if (click_button != NO_CLICK) {	/* button press */
			/* click values are already set */
			retval = 0;
		    } else {				/* key press */
			/* get first pressed key */
			--incount;
			retval = inbuf[inptr];
			inptr = (inptr+1) % INBUF_SIZE;
			/* pkey(retval); */
		    }
		    keep_going = FALSE;
		}
		break;
	    default:
		panic("x_event: unknown exit condition %d\n", exit_condition);
		break;
	}
    } while (keep_going);

    /* Restore globals */
    exit_x_event = hold_exit_x_event;

    return retval;
}

/*winmap.c*/
