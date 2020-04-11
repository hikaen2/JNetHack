/*
  $Id:$
 */

#ifndef WINGTK_H
#define WINGTK_H

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

#include "hack.h"
#include "wintty.h"

#define	NH_PAD			5

extern GtkWidget *nh_gtk_new(GtkWidget *w, GtkWidget *parent, gchar *lbl);

extern GtkWidget *nh_gtk_new_and_add(GtkWidget *w, GtkWidget *parent, gchar *lbl);

extern GtkWidget *nh_gtk_new_and_pack(GtkWidget *w, GtkWidget *parent, gchar *lbl, 
				      gboolean a1, gboolean a2, guint a3);

extern GtkWidget *nh_gtk_new_and_attach(GtkWidget *w, GtkWidget *parent, gchar *lbl,
					guint a1, guint a2, guint a3, guint a4);

extern GtkWidget *nh_gtk_new_and_attach2(GtkWidget *w, GtkWidget *parent, gchar *lbl,
					 guint a1, guint a2, guint a3, guint a4,
					 GtkAttachOptions xoptions,
					 GtkAttachOptions yoptions,
					 guint xpadding,
					 guint ypadding);


#define		N_NH_COLORS	20
extern GdkColor  nh_color[N_NH_COLORS];

enum {
    MAP_BLACK,
    MAP_RED,
    MAP_GREEN,
    MAP_BROWN,
    MAP_BLUE,
    MAP_MAGENTA,
    MAP_CYAN,
    MAP_GRAY,
    MAP_FOREGROUND,
    MAP_ORANGE,
    MAP_BRIGHT_GREEN,
    MAP_YELLOW,
    MAP_BRIGHT_BLUE,
    MAP_BRIGHT_MAGENTA,
    MAP_BRIGHT_CYAN,
    MAP_WHITE,
    MAP_DARK_GREEN,
    MAP_BACKGROUND
};


#define	NH_BUFSIZ		4096
#define NH_TEXT_REMEMBER	4096

extern void	GTK_init_nhwindows(int *, char **);
extern void	GTK_player_selection(void);
extern void	GTK_askname(void);
extern void	GTK_get_nh_event(void);
extern void	GTK_exit_nhwindows(const char *);
extern void	GTK_suspend_nhwindows(void);
extern void	GTK_resume_nhwindows(void);
extern winid	GTK_create_nhwindow(int);
extern void	GTK_clear_nhwindow(winid);
extern void	GTK_display_nhwindow(winid, BOOLEAN_P);
extern void	GTK_destroy_nhwindow(winid);
extern void	GTK_curs(winid, int, int);
extern void	GTK_putstr(winid, int, const char *);
extern void	GTK_display_file(const char *, BOOLEAN_P);
extern void	GTK_start_menu(winid);
extern void	GTK_add_menu(winid, int, const ANY_P *, CHAR_P,CHAR_P,int,const char *, BOOLEAN_P);
extern void	GTK_end_menu(winid, const char *);
extern int	GTK_select_menu(winid, int, MENU_ITEM_P **);
extern void	GTK_update_inventory(void);
extern void	GTK_mark_synch(void);
extern void	GTK_wait_synch(void);
#ifdef CLIPPING
extern void	GTK_cliparound(int, int);
#endif
extern void	GTK_prvoid_glyph(void);
extern void	GTK_raw_prvoid(void);
extern void	GTK_raw_prvoid_bold(void);
extern int	GTK_nhgetch(void);
extern int	GTK_nh_poskey(int *, int *, int *);
extern void	GTK_nhbell(void);
extern int	GTK_doprev_message(void);
extern char	GTK_yn_function(const char *, const char *, CHAR_P);
extern void	GTK_getlin(const char *, char *);
extern int	GTK_get_ext_cmd(void);
extern void	GTK_number_pad(void);
extern void	GTK_delay_output(void);
extern void	GTK_start_screen(void);
extern void	GTK_end_screen(void);
#ifdef GRAPHIC_TOMBSTONE
extern void	GTK_outrip(winid, int);
#endif
extern void	GTK_print_glyph(winid, XCHAR_P, XCHAR_P, int);
extern void	GTK_raw_print(const char *);
extern void	GTK_raw_print_bold(const char *);

extern GtkWidget	*nh_map_new(GtkWidget *);
extern void		nh_map_destroy(void);
extern void		nh_map_clear(void);
extern void		nh_map_check_visibility(void);
extern void		nh_map_pos(int *, int *);
extern void		nh_map_click(int);
extern void		nh_map_flush(void);

extern void		nh_set_map_visual(int);
extern int		nh_get_map_visual(void);

extern void		main_hook(void);
extern void		quit_hook(void);

extern GtkWidget	*nh_radar_new(void);
extern void		nh_radar_update(void);

extern GtkWidget	*nh_message_new(void);
extern void		nh_message_putstr(const char *);

extern GtkWidget	*nh_status_new(void);
extern void		nh_status_update(void);
extern void		nh_status_index_update(void);

extern int		nh_keysym(GdkEventKey *ev);

extern void		nh_option_new(void);
extern void		nh_option_lock(void);

/*
  topten.c
 */
extern int		create_toptenwin();


typedef struct _NHWindow{
    int		type;

    guint	hid;
    GtkWidget	*w;
    GtkWidget	*hbox, *hbox2, *hbox3;
    GtkWidget	*vbox, *vbox2;
    GtkWidget	*clist;
    GtkWidget	*scrolled;

    GtkWidget	*frame;
    GtkWidget	*query;

    GtkAdjustment *adj;

    int	n_subclist;
    GtkWidget	*subclist[20];

    int	n_subframe;
    GtkWidget	*subframe[20];

    int	n_button;
    GtkWidget	*button[20];

}NHWindow;

typedef struct _TileTab{
    char *ident;
    char *file;
    int tilemap_width, tilemap_height;
    int unit_width, unit_height;

    int ofsetx_3d;
    int ofsety_3d;

    int transparent:1;
    int spread:1;
} TileTab;

extern void	xshm_init(Display *dpy);
extern int	xshm_map_init(int width, int height);
extern void	xshm_map_destroy();
extern void	xshm_map_clear();
extern void	xshm_map_tile_draw(int dst_x, int dst_y);
extern void	xshm_map_draw(Window, int src_x, int src_y, int dst_x, int dst_y, int width, int height);
extern void	x_tmp_clear();
extern void	x_tile_init(XImage *, TileTab *t);
extern void	x_tile_destroy();
extern void	x_tile_tmp_draw(int src_x, int src_y, int ofsx, int ofsy);
extern void	x_tile_tmp_draw_rectangle(int ofsx, int ofsy, int c);

#endif
