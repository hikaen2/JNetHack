/*
  $Id: gtk.c,v 1.3 1999/12/01 03:51:06 issei Exp issei $
 */
/*
  GTK+ NetHack Copyright (c) Issei Numata 1999-2000
  GTK+ NetHack may be freely redistributed.  See license for details. 
*/

#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include "winGTK.h"
#include "wintype.h"
#include "func_tab.h"

static int	initialized;
static int	initialized2;

static void	select_roles(GtkWidget *w, gpointer data);
static void	key_command(GtkWidget *w, gpointer data);
static void	game_option(GtkWidget *w, gpointer data);
static void	game_topten(GtkWidget *w, gpointer data);

static void	help_help(GtkWidget *w, gpointer data);
static void	help_shelp(GtkWidget *w, gpointer data);
static void	help_option(GtkWidget *w, gpointer data);
static void	help_je(GtkWidget *w, gpointer data);
static void	help_history(GtkWidget *w, gpointer data);
static void	help_license(GtkWidget *w, gpointer data);

NHWindow gtkWindows[MAXWIN];

static GtkWidget	*pl_window;

static GtkWidget	*credit_window;
static GtkWidget 	*credit_vbox;
static GtkWidget 	*credit_credit;

static GtkStyle		*credit_style;
static GdkPixmap	*credit_pixmap;
static GdkBitmap	*credit_mask;

GtkWidget		*main_window;
static GtkWidget	*main_vbox;
static GtkWidget	*main_hbox;

static GtkWidget	*main_bar;
static GtkWidget	*main_message;
#ifdef RADAR
static GtkWidget	*main_radar;
#endif

static GtkWidget	*main_status;
static GtkWidget	*main_map;
static GtkItemFactory	*main_item_factory;

int			root_width;
int			root_height;

GdkColor	  nh_color[N_NH_COLORS] = {
    {0, 0*257, 0*257, 0*257,},		/* black */
    {0, 255*257, 0*257, 0*257,},	/* red */
    {0, 152*257, 251*257, 152*257,},	/* pale green */
    {0, 165*257, 42*257, 42*257,},	/* brown */
    {0, 0*257, 0*257, 255*257,},	/* blue */
    {0, 255*257, 0*257, 255*257,}, 	/* magenta */
    {0, 224*257, 255*257, 255*257,}, 	/* light cyan */
    {0, 190*257, 190*257, 190*257,},	/* gray */
    {1, 0*257, 0*257, 0*257,},		/* default  */
    {0, 255*257, 165*257, 0*257,},	/* orange */
    {0, 0*257, 255*257, 0*257,},	/* green */
    {0, 255*257, 255*257, 0*257,}, 	/* yellow */
    {0, 65*257, 105*257, 225*257,},	/* royal blue */
    {0, 238*257, 130*257, 238*257,},	/* violet */
    {0, 0*257, 255*257, 255*257,},	/* cyan */
    {0, 255*257, 255*257, 255*257,},	/* white */
    {0, 0*257, 100*257, 0*257,},	/* dark green */
    {0, 20*257, 60*257, 20*257,},	/* map background */
};

void
hook()
{
    ;
}

struct window_procs GTK_procs = {
    "gtk",
    GTK_init_nhwindows,
    GTK_player_selection,
    hook, /* tty_askname,*/
    GTK_get_nh_event,
    GTK_exit_nhwindows,
    hook, /*tty_suspend_nhwindows,*/
    hook, /*tty_resume_nhwindows,*/
    GTK_create_nhwindow,
    GTK_clear_nhwindow,
    GTK_display_nhwindow,
    GTK_destroy_nhwindow,
    GTK_curs,
    GTK_putstr,
    GTK_display_file,
    GTK_start_menu,
    GTK_add_menu,
    GTK_end_menu,
    GTK_select_menu,
    genl_message_menu,
    hook, /*tty_update_inventory,*/
    GTK_mark_synch,
    GTK_wait_synch,
#ifdef CLIPPING
    GTK_cliparound,
#endif
#ifdef POSITIONBAR
    hook,
#endif
    GTK_print_glyph,
    GTK_raw_print,
    GTK_raw_print_bold,
    GTK_nhgetch,
    GTK_nh_poskey,
    hook, /* tty_nhbell,*/
    GTK_doprev_message,
    GTK_yn_function,
    GTK_getlin,
    GTK_get_ext_cmd,
    hook, /*tty_number_pad,*/
    hook, /*tty_delay_output,*/
#ifdef CHANGE_COLOR     /* only a Mac option currently */
    hook,
    hook,
#endif
    /* other defs that really should go away (they're tty specific) */
    hook, /* tty_start_screen,*/
    hook, /* tty_end_screen,*/
#ifdef GRAPHIC_TOMBSTONE
    GTK_outrip,
#else
    genl_outrip,
#endif
};

#ifdef JNETHACK
static GtkItemFactoryEntry menu_items[] = {
    {"/������",			NULL,		NULL,		0,	"<Branch>"},
    {"/������/ͷ��",		"F2",		select_roles,	'P',	NULL},
    {"/������/Gsep1",		NULL,		NULL,		0,	"<Separator>"},
    {"/������/��¸",		"<shift>S",	key_command,	'S',	NULL},
    {"/������/���ץ����",	"<shift>O",	game_option,	'O',    NULL},
    {"/������/������",		NULL,		game_topten,	0,	NULL},
    {"/������/Gsep2",		NULL,		NULL,		0,	"<Separator>"},

    {"/������/��λ",		NULL,		select_roles,	'Q',	NULL},
};

static GtkItemFactoryEntry helpmenu_items[] = {
    {"/�إ��",			NULL,		NULL,		0,	"<LastBranch>"},
    {"/�إ��/���ޥ�ɥإ��",	NULL,		help_help,	0,	NULL},
    {"/�إ��/�����إ��",	NULL,		help_shelp,	0,	NULL},
    {"/�إ��/���ץ����إ��", NULL,		help_option,	0,	NULL},
    {"/�إ��/sep3",		NULL,		NULL,		0,	"<Separator>"},
    {"/�إ��/���ܸ�<->�Ѹ�",	NULL,		help_je,	0,	NULL},
    {"/�إ��/sep3",		NULL,		NULL,		0,	"<Separator>"},
    {"/�إ��/���",		NULL,		help_history,	0,	NULL},
    {"/�إ��/�饤����",	NULL,		help_license,	0,	NULL},
};

static GtkItemFactoryEntry playmenu_items[] = {
    {"/��ư",			NULL,		NULL,		0,	"<Branch>"},
    {"/��ư/��",		"k",		key_command,	'k',	NULL},
    {"/��ư/��",		"h",		key_command,	'h',	NULL},
    {"/��ư/��",		"j",		key_command,	'j',	NULL},
    {"/��ư/��",		"l",		key_command,	'l',	NULL},
    {"/��ư/����",		"u",		key_command,	'u',	NULL},
    {"/��ư/����",		"y",		key_command,	'y',	NULL},
    {"/��ư/����",		"n",		key_command,	'n',	NULL},
    {"/��ư/����",		"b",		key_command,	'b',	NULL},
    {"/��ư/��",		"greater",	key_command,	'>',	NULL},
    {"/��ư/��",		"less",		key_command,	'<',	NULL},
    {"/Ĵ��",			NULL,		NULL,		0,	"<Branch>"},
    {"/Ĵ��/­����Ĵ��",	"colon",	key_command,	':',	NULL},
    {"/Ĵ��/�󤯤θ���Ĵ��",	"semicolon",	key_command,	';',	NULL},
    {"/Ĵ��/櫤�Ĵ��",		"asciicircum",	key_command,	'^',	NULL},
    {"/����/��ˤ���",		"w",		key_command,	'w',	NULL},	
    {"/����/����ȤˤĤ���",	"<shift>w",	key_command,	'W',	NULL},	
    {"/����/����Ϥ���",	"<shift>T",	key_command,	'T',	NULL},	
    {"/����/���ؤ��������Ĥ���", "<shift>p",	key_command,	'P',	NULL},	
    {"/����/���ؤ��������Ϥ���", "<shift>r",	key_command,	'R',	NULL},	
    {"/���ʤ�����",		NULL,		NULL,		0,	"<Branch>"},
    {"/���ʤ�����/����ʪ����",	"i",		key_command,	'i',	NULL},
    {"/���ʤ�����/�������",	"parenright",	key_command,	')',	NULL},	
    {"/���ʤ�����/������",	"bracketleft",	key_command,	'[',	NULL},	
    {"/���ʤ�����/�ȤˤĤ��������", "equal",	key_command,	'=',	NULL},	
    {"/���ʤ�����/�ȤˤĤ��Ƥ��������",	"quotedbl",	key_command,	'"',	NULL},	
    {"/���ʤ�����/�ȤäƤ���ƻ��", "parenleft",	key_command,	'(',	NULL},	
    {"/���ʤ�����/��ˡ����",	"x",		key_command,	'x',	NULL},	
    {"/���ʤ�����/���̺ѥ����ƥ�","backslash",	key_command,	'\\',	NULL},	
    {"/����",			NULL,		NULL,		0,	"<Branch>"},
    {"/����/�����ƥ��̾����Ĥ���","<alt>n",	key_command,	'n' | 0x80,	NULL},
    {"/����/����ʪ��̾����Ĥ���","<shift>C",	key_command,	'C',	NULL},
    {"/����/��Ͽʸ���ѹ�",	"<alt>a",	key_command,	'a' | 0x80,	NULL},
    {"/��ư",			NULL,		NULL,		0,	"<Branch>"},
    {"/��ư/�٤�",		"period",	key_command,	'.',	NULL},
    {"/��ư/õ��",		"s",		key_command,	's',	NULL},
    {"/��ư/���٤�",		"e",		key_command,	'e',	NULL},
    {"/��ư/Asep1",		NULL,		NULL,		0,	"<Separator>"},
    {"/��ư/������",		"o",		key_command,	'o',	NULL},
    {"/��ư/�Ĥ��",		"c",		key_command,	'c',	NULL},
    {"/��ư/Asep2",		NULL,		NULL,		0,	"<Separator>"},
    {"/��ư/����",		"comma",	key_command,	',',	NULL},
    {"/��ư/�֤�",		"<shift>d",	key_command,	'D',	NULL},
    {"/��ư/Ȣ������",	"<alt>l",	key_command,	'l' | 0x80, NULL},
    {"/��ư/ƻ���Ȥ�",	"a",		key_command,	'a',	NULL},
    {"/��ư/����",		"<control>D",	key_command,	'\04',	NULL},
    {"/��ư/�ꤲ��",		"t",		key_command,	't',	NULL},
    {"/��ư/Asep3",		NULL,		NULL,		0,	"<Separator>"},
    {"/��ư/����",		"q",		key_command,	'q',	NULL},
    {"/��ư/�ɤ�",		"r",		key_command,	'r',	NULL},
    {"/��ư/��ˡ�򾧤���",	"<shift>Z",	key_command,	'Z',	NULL},
    {"/��ư/��򿶤�",		"z",		key_command,	'z',	NULL},
    {"/��ư/��˿���",		"<alt>d",	key_command,	'd' | 0x80,	NULL},
    {"/��ư/�¤�",		"<alt>s",	key_command,	's' | 0x80,	NULL},
    {"/����",			NULL,		NULL, 0, "<Branch>"},
    {"/����/����",		"<alt>p",	key_command,	'p' | 0x80,	NULL},
    {"/����/���Τ�������",	"<alt>o",	key_command,	'o' | 0x80,	NULL},
    {"/�ü�",			NULL,		NULL, 0, "<Branch>"},
    {"/�ü�/���˹��",		"<shift>E",	key_command,	'E',		NULL},
    {"/�ü�/�����ʧ��",	"p",		key_command,	'p',		NULL},
    {"/�ü�/���ä򤹤�",	"<alt>c",	key_command,	'c' | 0x80,	NULL},
    {"/�ü�/ǽ�Ϥ����",	"<alt>e",	key_command,	'e' | 0x80,	NULL},
    {"/�ü�/����������",	"<alt>f",	key_command,	'f' | 0x80,	NULL},
    {"/�ü�/ƻ����ü�ǽ�Ϥ���Ѥ���","<alt>i",	key_command,	'i' | 0x80,	NULL},
    {"/�ü�/��ʪ���ü�ǽ�Ϥ���Ѥ���","<alt>m",	key_command,	'm' | 0x80,	NULL},
    {"/�ü�/����",		"<alt>r",	key_command,	'r' | 0x80,	NULL},
    {"/�ü�/�����Ȥ�",		"<alt>w",	key_command,	'w' | 0x80,	NULL},
    {"/�ü�/�ƥ�ݡ���",	"<control>t",	key_command,	'',	NULL},
    {"/�ü�/�ڤ��֤�",		"<alt>t",	key_command,	't' | 0x80,	NULL},
    {"/�ü�/櫲��",		"<alt>u",	key_command,	'u' | 0x80,	NULL},
    {"/�ü�/������",		"<alt>j",	key_command,	'j' | 0x80,	NULL},
};
#else
static GtkItemFactoryEntry menu_items[] = {
    {"/Game",			NULL,		NULL,		0,	"<Branch>"},
    {"/Game/Play",		"F2",		select_roles,    'P',	NULL},
    {"/Game/Gsep1",		NULL,		NULL,		0,	"<Separator>"},
    {"/Game/Save",		"<shift>S",	key_command,	'S',	NULL},
    {"/Game/Option",		"<shift>O",	game_option,	'O',    NULL},
    {"/Game/Score",		NULL,		game_topten,	0,	NULL},
    {"/Game/Gsep2",		NULL,		NULL,		0,	"<Separator>"},
    {"/Game/Quit",		NULL,		select_roles,	'Q',	NULL},
};

static GtkItemFactoryEntry helpmenu_items[] = {
    {"/Help",			NULL,		NULL,		0,	"<LastBranch>"},
    {"/Help/Command Help",	NULL,		help_help,	0,	NULL},
    {"/Help/Key Help",		NULL,		help_shelp,	0,	NULL},
    {"/Help/Option Help",	NULL,		help_option,	0,	NULL},
    {"/Help/sep3",		NULL,		NULL,		0,	"<Separator>"},
    {"/Help/sep3",		NULL,		NULL,		0,	"<Separator>"},
    {"/Help/History",		NULL,		help_history,	0,	NULL},
    {"/Help/License",		NULL,		help_license,	0,	NULL},
};

static GtkItemFactoryEntry playmenu_items[] = {
    {"/Move",			NULL,		NULL,		0,	"<Branch>"},
    {"/Move/North",		"k",		key_command,	'k',	NULL},
    {"/Move/East",		"h",		key_command,	'h',	NULL},
    {"/Move/South",		"j",		key_command,	'j',	NULL},
    {"/Move/West",		"l",		key_command,	'l',	NULL},
    {"/Move/Northeast",		"u",		key_command,	'u',	NULL},
    {"/Move/Northwest",		"y",		key_command,	'y',	NULL},
    {"/Move/Southeast",		"n",		key_command,	'n',	NULL},
    {"/Move/Southwest",		"b",		key_command,	'b',	NULL},
    {"/Move/Down",		"greater",	key_command,	'>',	NULL},
    {"/Move/Up",		"less",		key_command,	'<',	NULL},
    {"/Check",			NULL,		NULL,		0,	"<Branch>"},
    {"/Check/Here",		"colon",	key_command,	':',	NULL},
    {"/Check/There",		"semicolon",	key_command,	';',	NULL},
    {"/Check/Trap",		"asciicircum",	key_command,	'^',	NULL},
    {"/Equip/Wield",		"w",		key_command,	'w',	NULL},	
    {"/Equip/Wear",		"<shift>w",	key_command,	'W',	NULL},	
    {"/Equip/Take off",		"<shift>T",	key_command,	'T',	NULL},	
    {"/Equip/Puton",		"<shift>p",	key_command,	'P',	NULL},	
    {"/Equip/Remove",		"<shift>r",	key_command,	'R',	NULL},	
    {"/You",			NULL,		NULL,		0,	"<Branch>"},
    {"/You/Inventory",		"i",		key_command,	'i',	NULL},
    {"/You/Weapon",		"parenright",	key_command,	')',	NULL},	
    {"/You/Armor",		"bracketleft",	key_command,	'[',	NULL},	
    {"/You/Ring",		"equal",	key_command,	'=',	NULL},	
    {"/You/Amulet",		"quotedbl",	key_command,	'"',	NULL},	
    {"/You/Item",		"parenleft",	key_command,	'(',	NULL},	
    {"/You/Spells",		"x",		key_command,	'x',	NULL},	
    {"/You/Known Item",		"backslash",	key_command,	'\\',	NULL},	
    {"/Adventure",		NULL,		NULL,		0,	"<Branch>"},
    {"/Adventure/Name",		"<alt>n",	key_command,	'n' | 0x80,	NULL},
    {"/Adventure/Call",		"<shift>C",	key_command,	'C',	NULL},
    {"/Adventure/Adjust",	"<alt>a",	key_command,	'a' | 0x80,	NULL},
    {"/Action",			NULL,		NULL,		0,	"<Branch>"},
    {"/Action/Rest",		"period",	key_command,	'.',	NULL},
    {"/Action/Search",		"s",		key_command,	's',	NULL},
    {"/Action/Eat",		"e",		key_command,	'e',	NULL},
    {"/Action/Asep1",		NULL,		NULL,		0,	"<Separator>"},
    {"/Action/Open",		"o",		key_command,	'o',	NULL},
    {"/Action/Close",		"c",		key_command,	'c',	NULL},
    {"/Action/Asep2",		NULL,		NULL,		0,	"<Separator>"},
    {"/Action/Pickup",		"comma",	key_command,	',',	NULL},
    {"/Action/Drop",		"d",		key_command,	'd',	NULL},
    {"/Action/Loot",		"<alt>l",	key_command,	'l' | 0x80, NULL},
    {"/Action/Apply",		"a",		key_command,	'a',	NULL},
    {"/Action/Kick",		"<control>D",	key_command,	'\04',	NULL},
    {"/Action/Throw",		"t",		key_command,	't',	NULL},
    {"/Action/Asep3",		NULL,		NULL,		0,	"<Separator>"},
    {"/Action/Drink",		"q",		key_command,	'q',	NULL},
    {"/Action/Read",		"r",		key_command,	'r',	NULL},
    {"/Action/Cast Spell",	"<shift>Z",	key_command,	'Z',	NULL},
    {"/Action/Zap",		"z",		key_command,	'z',	NULL},
    {"/Action/Dip",		"<alt>d",	key_command,	'd' | 0x80,	NULL},
    {"/Action/Sit",		"<alt>s",	key_command,	's' | 0x80,	NULL},
    {"/Religion",		NULL,		NULL,		0, 	"<Branch>"},
    {"/Religion/Pray",		"<alt>p",	key_command,	'p' | 0x80,	NULL},
    {"/Religion/Offer",		"<alt>o",	key_command,	'o' | 0x80,	NULL},
    {"/Special",		NULL,		NULL,		0, 	"<Branch>"},
    {"/Special/Engrave",	"<shift>E",	key_command,	'E', 		NULL},
    {"/Special/Pay",		"p",		key_command,	'p', 		NULL},
    {"/Special/Chat",		"<alt>c",	key_command,	'c' | 0x80,	NULL},
    {"/Special/Enhance",	"<alt>e",	key_command,	'e' | 0x80,	NULL},
    {"/Special/Force",		"<alt>f",	key_command,	'f' | 0x80,	NULL},
    {"/Special/Invoke",		"<alt>i",	key_command,	'i' | 0x80,	NULL},
    {"/Special/Monster",	"<alt>m",	key_command,	'm' | 0x80,	NULL},
    {"/Special/Rub",		"<alt>r",	key_command,	'r' | 0x80,	NULL},
    {"/Special/Wipe",		"<alt>w",	key_command,	'w' | 0x80,	NULL},
    {"/Special/Teleport",	"<control>t",	key_command,	'',	NULL},
    {"/Special/Turn",		"<alt>t",	key_command,	't' | 0x80,	NULL},
    {"/Special/Untrap",		"<alt>u",	key_command,	'u' | 0x80,	NULL},
    {"/Special/Jump",		"<alt>j",	key_command,	'j' | 0x80,	NULL},
};
#endif

static int keysym;

static int pl_selection = -1;

/*static int pl_selection;*/

void
win_GTK_init()
{
    ;
}

GtkWidget *
nh_gtk_new(GtkWidget *w, GtkWidget *parent, gchar *lbl)
{
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
    gtk_widget_show(w);

    return w;
}

GtkWidget *
nh_gtk_new_and_add(GtkWidget *w, GtkWidget *parent, gchar *lbl)
{
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
    gtk_widget_show(w);

    gtk_container_add(GTK_CONTAINER(parent), w);

    return w;
}

GtkWidget *
nh_gtk_new_and_pack(GtkWidget *w, GtkWidget *parent, gchar *lbl, 
		    gboolean a1, gboolean a2, guint a3)
{
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
    gtk_widget_show(w);

    gtk_box_pack_start(GTK_BOX(parent), w, a1, a2, a3);

    return w;
}

GtkWidget *
nh_gtk_new_and_attach(GtkWidget *w, GtkWidget *parent, gchar *lbl, 
		      guint a1, guint a2, guint a3, guint a4)
{
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
    gtk_widget_show(w);

    gtk_table_attach_defaults(GTK_TABLE(parent), w, a1, a2, a3, a4);

    return w;
}

GtkWidget *
nh_gtk_new_and_attach2(GtkWidget *w, GtkWidget *parent, gchar *lbl, 
		      guint a1, guint a2, guint a3, guint a4,
		      GtkAttachOptions a5,
		      GtkAttachOptions a6,
		      guint a7, guint  a8)
{
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
    gtk_widget_show(w);

    gtk_table_attach(GTK_TABLE(parent), w, a1, a2, a3, a4, a5, a6, a7, a8);

    return w;
}

int
nh_keysym(GdkEventKey *ev)
{
    int ret;
    int key;

    key = ev->keyval;

    ret = 0;

    if(key == GDK_KP_End)
	ret = 'b';
    else if(key == GDK_KP_Down)
	ret = 'j';
    else if(key == GDK_KP_Page_Down)
	ret = 'n';
    else if(key == GDK_KP_Left)
	ret = 'h';
    else if(key == GDK_KP_Begin)
	ret = '.';
    else if(key == GDK_KP_Right)
	ret = 'l';
    else if(key == GDK_KP_Home)
	ret = 'y';
    else if(key == GDK_KP_Up)
	ret = 'k';
    else if(key == GDK_KP_Page_Up)
	ret = 'u';
    else if(key == GDK_KP_Enter || key == GDK_Return)
	ret = '\n';
    else if(key == GDK_KP_Insert)
	ret = 'i';

    if(!ret)
	ret = *ev->string;

    return ret;
}

static void
nh_menu_sensitive(char *menu, boolean f)
{
    GtkWidget *p;
    
    p = gtk_item_factory_get_widget(
	main_item_factory, menu);
    gtk_widget_set_sensitive(p, f);
}

void
quit_hook()
{
    gtk_main_quit();
}

void
main_hook()
{
    nh_map_check_visibility();
#ifdef RADAR
    nh_radar_update();
#endif

    gtk_main();
}

static void
game_option(GtkWidget *widget, gpointer data)
{
    nh_option_new();
    keysym = '\0';
}

static void
game_topten(GtkWidget *widget, gpointer data)
{
    winid id;
#ifdef JNETHACK
    char *argv[] = {
	"jnethack",
	"-sall",
    };
#else
    char *argv[] = {
	"nethack",
	"-sall",
    };
#endif
    
    id = create_toptenwin();
    prscore(2, argv);
    GTK_display_nhwindow(id, TRUE);
    GTK_destroy_nhwindow(id);
    keysym = '\0';
}

static void
help_license(GtkWidget *widget, gpointer data)
{
    GTK_display_file(LICENSE, TRUE);
    keysym = '\0';
}

static void
help_history(GtkWidget *widget, gpointer data)
{
    dohistory();
    keysym = '\0';
}

#ifdef JNETHACK
static void
help_je(GtkWidget *widget, gpointer data)
{
    GTK_display_file(JJJ, TRUE);
    keysym = '\0';
}
#endif

static void
help_option(GtkWidget *widget, gpointer data)
{
    GTK_display_file(OPTIONFILE, TRUE);
    keysym = '\0';
}

static void
help_shelp(GtkWidget *widget, gpointer data)
{
    GTK_display_file(SHELP, TRUE);
    keysym = '\0';
}

static void
help_help(GtkWidget *widget, gpointer data)
{
    GTK_display_file(HELP, TRUE);
    keysym = '\0';
}

static void
key_command(GtkWidget *widget, gpointer data)
{
    keysym = (int)data;

    if(iflags.num_pad){
	switch(keysym){
	case 'y':
	    keysym = '7';
	    break;
	case 'u':
	    keysym = '9';
	    break;
	case 'b':
	    keysym = '1';
	    break;
	case 'n':
	    keysym = '3';
	    break;
	case 'h':
	    keysym = '4';
	    break;
	case 'j':
	    keysym = '2';
	    break;
	case 'k':
	    keysym = '8';
	    break;
	case 'l':
	    keysym = '6';
	    break;
	}
    }

    quit_hook();
}

static void
quit()
{
    if(initialized2)
	done2();
    else{
	clearlocks();
	GTK_exit_nhwindows(NULL);
	terminate(0);
    }
}

static gint
main_window_destroy(GtkWidget *widget, gpointer data)
{
    quit();

    return TRUE;
}

static gint
default_destroy(GtkWidget *widget, gpointer data)
{
    guint *hid = (guint *)data;
    *hid = 0;
    keysym = '\033';
    
    quit_hook();
    return FALSE;
}

static gint
default_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    keysym = nh_keysym(event);

    if(keysym)
	quit_hook();
    
    return FALSE;
}

static gint
default_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    if(data)
	keysym = (int)data;
    else
	keysym = '\n';

    quit_hook();

    return FALSE;
}

static gint
role_selected(GtkWidget *clist, gint row, gint column,
	      GdkEventButton *event, gpointer data)
{
    fprintf(stderr, "%d\n", row);

    pl_selection = row;

    quit_hook();

    return FALSE;
}

static gint
role_button_clicked(GtkWidget *widget, gpointer data)
{
     keysym = (int)data;

     pl_selection = -1;

     quit_hook();

     return FALSE;
}

static gint
role_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    int i;
    gchar *let;
    GtkCList	*clist = (GtkCList *)data;

    keysym = nh_keysym(event);

    pl_selection = -1;
    for(i=0 ; ; ++i){
	if(!gtk_clist_get_text(clist, i, 0, &let))
	    break;
	if(*let == keysym){
	    pl_selection = i;
	    break;
	}
    }

    if(pl_selection >= 0 || keysym == '*')
	quit_hook();

    return FALSE;
}

static void
select_roles(GtkWidget *widget, gpointer data)
{
    int 	i, n;
    int		plet = 0;
    guint 	hid;
    GtkWidget	*frame;
    GtkWidget	*vbox;
    GtkWidget	*hbox, *hbox2;
    GtkWidget	*button_random, *b;
    GtkWidget	*clist;
    gchar	*text[2];
    char	pbuf[BUFSZ];

    if((int)data == 'Q'){
	quit();
    }

    pl_selection = -1;
    if(flags.initrole < 0){
	pl_window  = gtk_window_new(GTK_WINDOW_DIALOG);
	hid = gtk_signal_connect(
	    GTK_OBJECT(pl_window), "destroy",
	    GTK_SIGNAL_FUNC(default_destroy), &hid);

#ifdef JNETHACK
	frame = nh_gtk_new_and_add(gtk_frame_new("���Ȥ����򤷤Ƥ�������"), pl_window, "");
#else
	frame = nh_gtk_new_and_add(gtk_frame_new("Pick a role"), pl_window, "");
#endif
	gtk_container_border_width(GTK_CONTAINER(frame), NH_PAD);

	vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), frame, "");

	hbox = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);

	b = button_random = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("random"), hbox, "",  FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(b), "clicked",
			   GTK_SIGNAL_FUNC(role_button_clicked), (gpointer)'R');
/*
	b = button_quit = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("back"), hbox, "",  FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(b), "clicked",
			   GTK_SIGNAL_FUNC(role_button_clicked), (gpointer)'B');
*/

	hbox2 = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);

	clist = nh_gtk_new_and_pack(
	    gtk_clist_new(2), hbox2, "", FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(clist), "select_row",
			   GTK_SIGNAL_FUNC(role_selected), NULL);

	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);

	for(i = 0; roles[i].name.m; i++) {
	    char tmp[2] = {0, 0};
	    
	    tmp[0] = lowc(roles[i].name.m[0]);
	    if(plet == tmp[0])
		tmp[0] = highc(tmp[0]);
	    
	    plet = tmp[0];
	    
	    text[0] = (gchar *)tmp;
#ifdef JNETHACK
	    text[1] = (gchar *)roles[i].jname.m;
#else
	    text[1] = (gchar *)roles[i].name.m;
#endif
	    gtk_clist_append(GTK_CLIST(clist), text);
	}

	gtk_signal_connect(
	    GTK_OBJECT(pl_window), "key_press_event",
	    GTK_SIGNAL_FUNC(role_key_press), clist);

	gtk_grab_add(pl_window);
	gtk_widget_show_all(pl_window);
	gtk_main();

	if(hid > 0){	/* if does not received 'destroyed signal' */
	    gtk_widget_hide(pl_window);
	    gtk_widget_destroy(pl_window);
	}
    }

    if(pl_selection < 0)
	pl_selection = randrole();

    flags.initrole = pl_selection;
    pl_selection = -1;

    if(flags.initrace < 0){
	/* Count the number of valid races */
	n = 0;
	for(i = 0; races[i].noun; i++) {
	    if(validrace(flags.initrole, i)){
		n++;
		pl_selection = i;
	    }
	}
	if(n <= 1)
	    goto race_done;

	pl_window  = gtk_window_new(GTK_WINDOW_DIALOG);
	hid = gtk_signal_connect(
	    GTK_OBJECT(pl_window), "destroy",
	    GTK_SIGNAL_FUNC(default_destroy), &hid);

#ifdef JNETHACK
	Sprintf(pbuf, "%s�μ�²�����򤷤Ƥ���������",
		roles[flags.initrole].jname.m);
#else
	Sprintf(pbuf, "Pick the race of your %s",
		roles[flags.initrole].name.m);
#endif
	frame = nh_gtk_new_and_add(gtk_frame_new(pbuf), pl_window, "");
	gtk_container_border_width(GTK_CONTAINER(frame), NH_PAD);

	vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), frame, "");

	hbox = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);

	b = button_random = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("random"), hbox, "",  FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(b), "clicked",
			   GTK_SIGNAL_FUNC(role_button_clicked), (gpointer)'R');
/*
	b = button_quit = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("cancel"), hbox, "",  FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(b), "clicked",
			   GTK_SIGNAL_FUNC(role_button_clicked), (gpointer)'C');
*/
	hbox2 = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);

	clist = nh_gtk_new_and_pack(
	    gtk_clist_new(2), hbox2, "", FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(clist), "select_row",
			   GTK_SIGNAL_FUNC(role_selected), NULL);

	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);

	for(i = 0; races[i].noun; i++) {
	    char tmp[2] = {0, 0};
	    if(!validrace(flags.initrole, i))
		continue;
	    
	    tmp[0] = races[i].noun[0];
	    text[0] = (gchar *)tmp;
#ifdef JNETHACK
	    text[1] = (gchar *)races[i].j;
#else
	    text[1] = (gchar *)races[i].noun;
#endif
	    gtk_clist_append(GTK_CLIST(clist), text);
	}

	gtk_signal_connect(
	    GTK_OBJECT(pl_window), "key_press_event",
	    GTK_SIGNAL_FUNC(role_key_press), clist);

	gtk_grab_add(pl_window);
	gtk_widget_show_all(pl_window);
	gtk_main();

	if(hid > 0){	/* if does not received 'destroyed signal' */
	    gtk_widget_hide(pl_window);
	    gtk_widget_destroy(pl_window);
	}
    }
 race_done:
    if(pl_selection < 0)
	pl_selection = randrace(flags.initrole);

    flags.initrace = pl_selection;

    pl_selection = -1;
    if(flags.initgend < 0){
	/* Count the number of valid genders */
	n = 0;
	for(i = 0; i<ROLE_GENDERS; i++) {
	    if(validgend(flags.initrole, flags.initrace, i)){
		n++;
		pl_selection = i;
	    }
	}
	if(n <= 1)
	    goto gender_done;

	pl_window  = gtk_window_new(GTK_WINDOW_DIALOG);
	hid = gtk_signal_connect(
	    GTK_OBJECT(pl_window), "destroy",
	    GTK_SIGNAL_FUNC(default_destroy), &hid);

#ifdef JNETHACK
	Sprintf(pbuf, "%s��%s�����̤�����Ǥ�������",
		races[flags.initrace].j,
		roles[flags.initrole].jname.m);
#else
	Sprintf(pbuf, "Pick the gender of your %s %s",
		races[flags.initrace].adj,
		roles[flags.initrole].name.m);
#endif
	frame = nh_gtk_new_and_add(gtk_frame_new(pbuf), pl_window, "");
	gtk_container_border_width(GTK_CONTAINER(frame), NH_PAD);

	vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), frame, "");

	hbox = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);

	b = button_random = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("random"), hbox, "",  FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(b), "clicked",
			   GTK_SIGNAL_FUNC(role_button_clicked), (gpointer)'R');
/*
	b = button_quit = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("cancel"), hbox, "",  FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(b), "clicked",
			   GTK_SIGNAL_FUNC(role_button_clicked), (gpointer)'C');
*/
	hbox2 = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);

	clist = nh_gtk_new_and_pack(
	    gtk_clist_new(2), hbox2, "", FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(clist), "select_row",
			   GTK_SIGNAL_FUNC(role_selected), NULL);

	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);

	for(i = 0; i < ROLE_GENDERS; i++) {
	    char tmp[2] = {0, 0};
	    if(!validgend(flags.initrole, flags.initrace, i))
		continue;
	    
	    tmp[0] = genders[i].adj[0];
	    text[0] = (gchar *)tmp;
#ifdef JNETHACK
	    text[1] = (gchar *)genders[i].j;
#else
	    text[1] = (gchar *)genders[i].adj;
#endif
	    gtk_clist_append(GTK_CLIST(clist), text);
	}

	gtk_signal_connect(
	    GTK_OBJECT(pl_window), "key_press_event",
	    GTK_SIGNAL_FUNC(role_key_press), clist);

	gtk_grab_add(pl_window);
	gtk_widget_show_all(pl_window);
	gtk_main();

	if(hid > 0){	/* if does not received 'destroyed signal' */
	    gtk_widget_hide(pl_window);
	    gtk_widget_destroy(pl_window);
	}
    }
 gender_done:
    if(pl_selection < 0)
	pl_selection = randrace(flags.initrole);

    flags.initgend = pl_selection;

    pl_selection = -1;
    if(flags.initalign < 0){
	/* Count the number of valid aligns */
	n = 0;
	for(i = 0; i<ROLE_ALIGNS; i++) {
	    if(validalign(flags.initrole, flags.initrace, i)){
		n++;
		pl_selection = i;
	    }
	}
	if(n <= 1)
	    goto align_done;

	pl_window  = gtk_window_new(GTK_WINDOW_DIALOG);
	hid = gtk_signal_connect(
	    GTK_OBJECT(pl_window), "destroy",
	    GTK_SIGNAL_FUNC(default_destroy), &hid);

#ifdef JNETHACK
	Sprintf(pbuf, "%s��%s��%s��°�������򤷤Ƥ�������",
		genders[flags.initgend].j,
		races[flags.initrace].j,
		(flags.initgend && roles[flags.initrole].jname.f) ?
		roles[flags.initrole].jname.f :
		roles[flags.initrole].jname.m);
#else
	Sprintf(pbuf, "Pick the alignment of your %s %s %s",
		genders[flags.initgend].adj,
		races[flags.initrace].adj,
		(flags.initgend && roles[flags.initrole].name.f) ?
		roles[flags.initrole].name.f :
		roles[flags.initrole].name.m);
#endif
	frame = nh_gtk_new_and_add(gtk_frame_new(pbuf), pl_window, "");
	gtk_container_border_width(GTK_CONTAINER(frame), NH_PAD);

	vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), frame, "");

	hbox = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);

	b = button_random = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("random"), hbox, "",  FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(b), "clicked",
			   GTK_SIGNAL_FUNC(role_button_clicked), (gpointer)'R');
/*
	b = button_quit = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("cancel"), hbox, "",  FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(b), "clicked",
			   GTK_SIGNAL_FUNC(role_button_clicked), (gpointer)'C');
*/
	hbox2 = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);

	clist = nh_gtk_new_and_pack(
	    gtk_clist_new(2), hbox2, "", FALSE, FALSE, NH_PAD);
	gtk_signal_connect(GTK_OBJECT(clist), "select_row",
			   GTK_SIGNAL_FUNC(role_selected), NULL);

	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
	gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);

	for(i = 0; i<ROLE_ALIGNS ; i++) {
	    char tmp[2] = {0, 0};
	    if(!validalign(flags.initrole, flags.initrace, i))
		continue;

	    
	    tmp[0] = aligns[i].adj[0];
	    text[0] = (gchar *)tmp;
#ifdef JNETHACK
	    text[1] = (gchar *)aligns[i].j;
#else
	    text[1] = (gchar *)aligns[i].adj;
#endif
	    gtk_clist_append(GTK_CLIST(clist), text);
	}

	gtk_signal_connect(
	    GTK_OBJECT(pl_window), "key_press_event",
	    GTK_SIGNAL_FUNC(role_key_press), clist);

	gtk_grab_add(pl_window);
	gtk_widget_show_all(pl_window);
	gtk_main();

	if(hid > 0){	/* if does not received 'destroyed signal' */
	    gtk_widget_hide(pl_window);
	    gtk_widget_destroy(pl_window);
	}
    }
 align_done:
    if(pl_selection < 0)
	pl_selection = randalign(flags.initrole, flags.initrace);
    flags.initalign = pl_selection;

    quit_hook();
}

static gint
credit_expose_event(GtkWidget *widget, GdkEventExpose *event)
{
    gtk_main_quit();

    return FALSE;
}

static void
nh_rc(void)
{
    gtk_rc_parse("gtkrc");
}

void
GTK_init_nhwindows(int *argc, char **argv)
{
    if(initialized2)
	goto selection;

    gtk_set_locale();
    nh_rc();

    gtk_init(argc, &argv);
/*
  creat credit widget and show
*/
    credit_window = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_position(GTK_WINDOW(credit_window), GTK_WIN_POS_CENTER);

    gtk_container_border_width(GTK_CONTAINER(credit_window), 2);
/*
  gtk_signal_connect(GTK_OBJECT(credit_window), "check_resize",
  GTK_SIGNAL_FUNC(credit_map_event), NULL);
*/

    gtk_signal_connect(GTK_OBJECT(credit_window), "expose_event",
		       GTK_SIGNAL_FUNC(credit_expose_event), NULL);

    gtk_widget_realize(credit_window);
    root_width = gdk_screen_width();
    root_height = gdk_screen_height();

    credit_vbox = nh_gtk_new_and_add(
	gtk_vbox_new(FALSE, 0), credit_window, "");
     
    credit_style = gtk_widget_get_style(credit_window);
    credit_pixmap = gdk_pixmap_create_from_xpm(
	credit_window->window,
	&credit_mask,
	&credit_style->bg[GTK_STATE_NORMAL],
	"credit.xpm");
    credit_credit = nh_gtk_new_and_pack(
	gtk_pixmap_new(credit_pixmap, credit_mask), credit_vbox, "",
	FALSE, FALSE, NH_PAD);

    gtk_widget_show_all(credit_window);
    gtk_main();

/*
  create main widget
*/
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_policy(GTK_WINDOW(main_window), TRUE, TRUE, TRUE);

    gtk_signal_connect(
	GTK_OBJECT(main_window), "delete_event",
	GTK_SIGNAL_FUNC(main_window_destroy), 0);

#ifdef JNETHACK
    gtk_window_set_title(GTK_WINDOW(main_window), "JNetHack");
#else
    gtk_window_set_title(GTK_WINDOW(main_window), "NetHack");
#endif
    
    gtk_widget_set_events(main_window, GDK_KEY_PRESS_MASK);
    gtk_widget_realize(main_window);

/*
  allocate color
 */
    {
	int 		i;
	GdkColormap	*cmap;

	cmap = gdk_window_get_colormap(main_window->window);
  
	for(i=0 ; i < N_NH_COLORS ; ++i){
	    if(0 && nh_color[i].pixel){
		nh_color[i] = *(main_window->style->fg);
	    }
	    else if(gdk_colormap_alloc_color(cmap, &nh_color[i], TRUE, TRUE) == TRUE){
		;
	    }
	    else{
		fprintf(stderr, "cannot allocate color\n");
	    }
	}
    }

    main_vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), main_window, "");

    {
	int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
	int nplaymenu_items = sizeof(playmenu_items) / sizeof(playmenu_items[0]);
	int nhelpmenu_items = sizeof(helpmenu_items) / sizeof(helpmenu_items[0]);

	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new();

	main_item_factory = gtk_item_factory_new(
	    GTK_TYPE_MENU_BAR, "<main>",
	    accel_group);

	gtk_item_factory_create_items(
	    main_item_factory,
	    nmenu_items, menu_items,
	    NULL);

	gtk_item_factory_create_items(
	    main_item_factory,
	    nplaymenu_items, playmenu_items,
	    NULL);

	gtk_item_factory_create_items(
	    main_item_factory,
	    nhelpmenu_items, helpmenu_items,
	    NULL);


	gtk_accel_group_attach(accel_group, GTK_OBJECT(main_window));
    }

    main_bar = nh_gtk_new_and_pack(
	gtk_item_factory_get_widget(main_item_factory, "<main>"), main_vbox, "",
	FALSE, FALSE, 0);
#ifdef JNETHACK
    nh_menu_sensitive("/������/��¸", FALSE);
/*
  nh_menu_sensitive("/������/���ץ����", FALSE);
*/
    nh_menu_sensitive("/��ư", FALSE);
    nh_menu_sensitive("/Ĵ��", FALSE);
    nh_menu_sensitive("/����", FALSE);
    nh_menu_sensitive("/���ʤ�����", FALSE);
    nh_menu_sensitive("/����", FALSE);
    nh_menu_sensitive("/��ư", FALSE);
    nh_menu_sensitive("/����", FALSE);
    nh_menu_sensitive("/�ü�", FALSE);
#else
    nh_menu_sensitive("/Game/Save", FALSE);
/*
  nh_menu_sensitive("/Game/Option", FALSE);
*/
    nh_menu_sensitive("/Move", FALSE);
    nh_menu_sensitive("/Check", FALSE);
    nh_menu_sensitive("/Equip", FALSE);
    nh_menu_sensitive("/You", FALSE);
    nh_menu_sensitive("/Adventure", FALSE);
    nh_menu_sensitive("/Action", FALSE);
    nh_menu_sensitive("/Religion", FALSE);
    nh_menu_sensitive("/Special", FALSE);
#endif
    
    main_hbox = nh_gtk_new_and_pack(
	gtk_hbox_new(FALSE, 0), main_vbox, "",
	FALSE, FALSE, 0);
	
    main_message = nh_gtk_new_and_pack(
	nh_message_new(), main_hbox, "",
	FALSE, FALSE, 0);

    main_status = nh_gtk_new_and_pack(
	nh_status_new(), main_hbox, "",
	FALSE, FALSE, 0);
#ifdef RADAR
    main_radar = nh_radar_new();
#endif
    main_map = nh_gtk_new_and_pack(
	nh_map_new(main_window), main_vbox, "",
	FALSE, FALSE, 0);

 selection:
    initialized = 1;

    gtk_widget_hide(credit_window);
    gtk_widget_show_all(main_window);
    
    iflags.window_inited = 1;
}

void
GTK_exit_nhwindows(const char *str)
{
    int id;

    if(str && *str){
	id = GTK_create_nhwindow(NHW_MENU);
	GTK_putstr(id, 0, str);
	GTK_display_nhwindow(id, TRUE);
	GTK_destroy_nhwindow(id);
    }
}

void
GTK_init_nhwindows2()
{
    if(initialized2)
	return;
#ifdef JNETHACK
    nh_menu_sensitive("/������/ͷ��", FALSE);
    nh_menu_sensitive("/������/��¸", TRUE);
    nh_menu_sensitive("/������/���ץ����", TRUE);

    nh_menu_sensitive("/��ư", TRUE);
    nh_menu_sensitive("/Ĵ��", TRUE);
    nh_menu_sensitive("/����", TRUE);
    nh_menu_sensitive("/���ʤ�����", TRUE);
    nh_menu_sensitive("/����", TRUE);
    nh_menu_sensitive("/��ư", TRUE);
    nh_menu_sensitive("/����", TRUE);
    nh_menu_sensitive("/�ü�", TRUE);
#else
    nh_menu_sensitive("/Game/Play", FALSE);
    nh_menu_sensitive("/Game/Save", TRUE);
    nh_menu_sensitive("/Game/Option", TRUE);

    nh_menu_sensitive("/Game/Save", TRUE);
    nh_menu_sensitive("/Game/Option", TRUE);
    nh_menu_sensitive("/Move", TRUE);
    nh_menu_sensitive("/Check", TRUE);
    nh_menu_sensitive("/Equip", TRUE);
    nh_menu_sensitive("/You", TRUE);
    nh_menu_sensitive("/Adventure", TRUE);
    nh_menu_sensitive("/Action", TRUE);
    nh_menu_sensitive("/Religion", TRUE);
    nh_menu_sensitive("/Special", TRUE);
#endif

    nh_option_lock();

    gtk_signal_connect(
	GTK_OBJECT(main_window), "key_press_event",
	GTK_SIGNAL_FUNC(default_key_press), NULL);

#ifdef RADAR
    gtk_signal_connect(
	GTK_OBJECT(main_radar), "key_press_event",
	GTK_SIGNAL_FUNC(default_key_press), NULL);
#endif

    initialized2 = 1;
}

winid
GTK_create_nhwindow(int type)
{
    winid	id;
    NHWindow	*w;

    switch(type){
/* 
   these windows had already created
*/
    case NHW_MESSAGE:
    case NHW_STATUS:
    case NHW_MAP:
	w = &gtkWindows[type];
	w->w = main_window;	
	return type;
	break;
/*
  create new window
*/
    case NHW_MENU:
    case NHW_TEXT:
	id = type;
	while(id < MAXWIN){
	    w = &gtkWindows[id];
	    memset(w, 0, sizeof(NHWindow));

	    return id;
	    ++id;
	}
    }
    return 0;
}

void
GTK_destroy_nhwindow(winid id)
{
/*    int i;*/
    NHWindow *w;

    if(id == NHW_MAP)
	nh_map_destroy();

    if(id == NHW_STATUS || id == NHW_MESSAGE || id == NHW_MAP)
	return;

    w = &gtkWindows[id]; 

    if(w->w){
	gtk_widget_hide_all(w->w);
	if(w->hid > 0)
	    gtk_signal_disconnect(GTK_OBJECT(w->w), w->hid);

	gtk_widget_destroy(w->w);

	memset(w, 0, sizeof(NHWindow));
    }
}

void
GTK_display_nhwindow(winid id, BOOLEAN_P blocking)
{
    NHWindow *w;

    if(id == NHW_STATUS || id == NHW_MESSAGE){
    }
    else if(id == NHW_MAP){	/* flush out */
	nh_map_flush();
    }
    else{
	w = &gtkWindows[id];

	gtk_grab_add(w->w);
	gtk_widget_show_all(w->w);
    }

    if((id != NHW_MESSAGE && blocking) || id == NHW_MENU)
	main_hook();
}

void
GTK_clear_nhwindow(winid id)
{
    if(id == NHW_MAP){
	nh_map_clear();
    }
    return;
}

/*
  ATR_ULINE
  ATR_BOLD
  ATR_BLINK
  ATR_INVERSE
 */
void
GTK_putstr(winid id, int attr, const char *str)
{
    const gchar	*text[1];
    NHWindow	*w;

    w = &gtkWindows[id]; 

    if(id == NHW_MESSAGE){
	nh_message_putstr(str);
	return;
    }
    else if(id == NHW_STATUS){
	nh_status_update();
	return;
    }
    else if(id <= 3){
	panic("bad window");
	return;
    }

    if(!w->w){
	w->w = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_widget_set_name(GTK_WIDGET(w->w), "fixed font");
	gtk_window_set_position(GTK_WINDOW(w->w), GTK_WIN_POS_MOUSE);

	gtk_signal_connect(
	    GTK_OBJECT(w->w), "key_press_event",
	    GTK_SIGNAL_FUNC(default_key_press), NULL);
	w->hid = gtk_signal_connect(
	    GTK_OBJECT(w->w), "destroy",
	    GTK_SIGNAL_FUNC(default_destroy), &w->hid);

	w->frame = nh_gtk_new_and_add(
	    gtk_frame_new(NULL), w->w, "");

	w->vbox = nh_gtk_new_and_add(
	    gtk_vbox_new(FALSE, 0), w->frame, "");

	w->clist = nh_gtk_new_and_pack(
	    gtk_clist_new(1), w->vbox, "",
	    FALSE, FALSE, NH_PAD);
	gtk_clist_set_column_auto_resize(GTK_CLIST(w->clist), 0, TRUE);

	w->hbox2 = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), w->vbox, "",
	    FALSE, FALSE, NH_PAD);

#ifdef JNETHACK
	w->button[0] = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("�Ĥ���"), w->hbox2, "",
	    TRUE, FALSE, 0);
#else
	w->button[0] = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("Close"), w->hbox2, "",
	    TRUE, FALSE, 0);
#endif
	gtk_signal_connect(
	    GTK_OBJECT(w->button[0]), "clicked",
	    GTK_SIGNAL_FUNC(default_button_press), (gpointer)'\033');
    }

    text[0] = str;
    gtk_clist_append(GTK_CLIST(w->clist), (gchar **)text);

    if(attr != 0){
	gtk_clist_set_foreground(
	    GTK_CLIST(w->clist), 
	    GTK_CLIST(w->clist)->rows - 1,
	    GTK_WIDGET(w->clist)->style->bg); 
	gtk_clist_set_background(
	    GTK_CLIST(w->clist), 
	    GTK_CLIST(w->clist)->rows - 1,
	    GTK_WIDGET(w->clist)->style->fg); 
    }
}

void
GTK_get_nh_event()
{
    return;
}

int
GTK_nhgetch(void)
{
    int key;
#ifdef RADAR
    nh_radar_update();
#endif

    keysym = 0;
    if(!keysym)
	main_hook();

    key = keysym;
    keysym = 0;

    return key;
}

int
GTK_nh_poskey(int *x, int *y, int *mod)
{
    int key;

#ifdef RADAR
    nh_radar_update();
#endif

    nh_map_click(TRUE);

    keysym = 0;
    main_hook();

    if(!keysym)
	nh_map_pos(x, y);

    nh_map_click(FALSE);

    key = keysym;
    keysym = 0;

    return key;
}

void
GTK_display_file(const char *fname, BOOLEAN_P complain)
{
    guint hid;	
    GtkWidget *w;
    GtkWidget *scrollbar;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox, *hbox2;
    GtkWidget *text;
    GtkWidget *button;

    FILE	*fp;
    char	buf[NH_BUFSIZ];

    fp = fopen(fname, "r");
    if(!fp)
	return;

    w = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_widget_set_name(GTK_WIDGET(w), "fixed font");

    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
    gtk_signal_connect(
	GTK_OBJECT(w), "key_press_event",
	GTK_SIGNAL_FUNC(default_key_press), NULL);
    hid = gtk_signal_connect(
	GTK_OBJECT(w), "destroy",
	GTK_SIGNAL_FUNC(default_destroy), &hid);

    vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), w, "");

    label = nh_gtk_new_and_pack(
	gtk_label_new("HELP"), vbox, "",
	FALSE, FALSE, NH_PAD);

    hbox = nh_gtk_new_and_pack(
	gtk_hbox_new(FALSE, 0), vbox, "",
	FALSE, FALSE, NH_PAD);

    text = nh_gtk_new_and_pack(
	gtk_text_new(NULL, NULL), hbox, "",
	FALSE, FALSE, NH_PAD);

    gtk_widget_set_usize(
	GTK_WIDGET(text), 600, (root_height * 2)/3);

    scrollbar = nh_gtk_new_and_pack(
	gtk_vscrollbar_new(GTK_TEXT(text)->vadj), hbox, "",
	FALSE, FALSE, NH_PAD);

    hbox2 = nh_gtk_new_and_pack(
	gtk_hbox_new(FALSE, 0), vbox, "",
	FALSE, FALSE, NH_PAD);

#ifdef JNETHACK
    button = nh_gtk_new_and_pack(
	gtk_button_new_with_label("�Ĥ���"), hbox2, "",
	TRUE, FALSE, NH_PAD);
#else
    button = nh_gtk_new_and_pack(
	gtk_button_new_with_label("Close"), hbox2, "",
	TRUE, FALSE, NH_PAD);
#endif
    gtk_signal_connect(
	GTK_OBJECT(button), "clicked",
	GTK_SIGNAL_FUNC(default_button_press), (gpointer)'\033');

    while(fgets(buf, NH_BUFSIZ, fp)){
	gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, buf, strlen(buf));
    }

    gtk_widget_show_all(w);
    main_hook();

    if(hid > 0){
	gtk_signal_disconnect(GTK_OBJECT(w), hid);

	gtk_widget_destroy(button);
	gtk_widget_destroy(hbox2);
	gtk_widget_destroy(scrollbar);
	gtk_widget_destroy(text);
	gtk_widget_destroy(hbox);
	gtk_widget_destroy(label);
	gtk_widget_destroy(vbox);
	gtk_widget_destroy(w);
    }
}

void
GTK_player_selection(void)
{
    if(flags.initrole < 0){
	gtk_main();
    }
}

void
GTK_wait_synch()
{
    ;
}

void
GTK_mark_synch()
{
    ;
}

int
GTK_get_ext_cmd()
{
#if 0 /* JNetHack original */
    extern struct ext_func_tab extcmdlist[];
    int i;

    GTK_putstr(NHW_MESSAGE, 7, "#");
    main_hook();

    i = 0;
    while(extcmdlist[i].ef_txt){
	if(extcmdlist[i].ef_txt[0] == keysym){
	    GTK_putstr(NHW_MESSAGE, 7, extcmdlist[i].ef_txt);
	    return i;
	}
	++i;
    }
    
    return -1;

#else
#if 0 /* string input */

    extern struct ext_func_tab extcmdlist[];
    char buf[BUFSZ];
    int i, len;

    getlin("extended command", buf);
    for( len = strlen( buf ); 0 < len; len-- ){
	i = 0;
	while( extcmdlist[i].ef_txt ){
	    if( strncasecmp(extcmdlist[i].ef_txt, buf, len) == 0 ){
		return i;
	    }
	    ++i;
	}
    }
    return -1;

#else /* select menu */

    int i, j, num;
    winid win;
    anything any;
    menu_item *pick_list;
    char buf[BUFSZ];

    /* count the number of items allowed */
    for( i = 0; extcmdlist[i].ef_txt; i++ )
	;
    num = i;
    if( num == 0 )
	return -1;

    win = create_nhwindow( NHW_MENU );
    start_menu( win );
    any.a_obj = (struct obj *) 0;
    for( i = 0; i < num; i++ ){
	any.a_void = 0;
	any.a_int = i;
	strcpy( buf, "                " );
	memcpy( buf, extcmdlist[i].ef_txt, strlen(extcmdlist[i].ef_txt) );
	strncat( buf, extcmdlist[i].ef_desc, sizeof(buf) );
	add_menu( win, NO_GLYPH, &any, extcmdlist[i].ef_txt[0], ' ',
	        ATR_NONE, buf, MENU_UNSELECTED);
    }
    end_menu( win, "extended command" );

    i = select_menu( win, PICK_ONE, &pick_list );

    destroy_nhwindow( win );
    if( i > 0 ){
	return pick_list->item.a_int;
    }
    return -1;
#endif
#endif
}

#define NAME_LINE 0		/* line # for player name */
#define GOLD_LINE 1		/* line # for amount of gold */
#define DEATH_LINE 2		/* line # for death description */
#define YEAR_LINE 6		/* line # for year */

static struct{
    GdkWChar	str[NH_BUFSIZ];
    int		len;
    int		width;
} rip_line[YEAR_LINE + 1];

void
GTK_outrip(winid id, int how)
{
    int		x, y;
    int		width;
    int		total_len, len, line;
    GtkWidget	*w;
    GtkWidget	*vbox;
    GtkWidget	*rip;
    GdkPixmap 	*rip_pixmap;
    char	mstr[NH_BUFSIZ];
    GdkWChar	*wc;
    GdkWChar	wstr[NH_BUFSIZ];
    extern const char *killed_by_prefix[];

    w = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);

    gtk_widget_set_events(
	w, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

    gtk_signal_connect(
	GTK_OBJECT(w), "button_press_event",
	GTK_SIGNAL_FUNC(default_button_press), NULL);
    gtk_signal_connect(
	GTK_OBJECT(w), "key_press_event",
	GTK_SIGNAL_FUNC(default_key_press), NULL);

    gtk_widget_realize(w);

    vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), w, "");

    rip_pixmap = gdk_pixmap_create_from_xpm(
	w->window, 
	0, 0, "rip.xpm");

    rip = nh_gtk_new_and_pack(
	gtk_pixmap_new(rip_pixmap, 0), vbox, "",
	FALSE, FALSE, NH_PAD);

    Sprintf(mstr, "%s", plname);
    rip_line[NAME_LINE].len = gdk_mbstowcs(rip_line[NAME_LINE].str, mstr, NH_BUFSIZ);

    Sprintf(mstr, "%ld Au", u.ugold);
    rip_line[GOLD_LINE].len = gdk_mbstowcs(rip_line[GOLD_LINE].str, mstr, NH_BUFSIZ);

    Sprintf(mstr, "%4d", getyear());
    rip_line[YEAR_LINE].len = gdk_mbstowcs(rip_line[YEAR_LINE].str, mstr, NH_BUFSIZ);

    switch (killer_format) {
    default:
	impossible("bad killer format?");
    case KILLED_BY_AN:
#ifndef JNETHACK
	Strcpy(mstr, killed_by_prefix[how]);
	Strcat(mstr, an(killer));
#else
	Strcpy(mstr, an(killer));
	Strcat(mstr, killed_by_prefix[how]);
#endif
	break;
    case KILLED_BY:
#ifndef	JNETHACK
	Strcpy(mstr, killed_by_prefix[how]);
	Strcat(mstr, killer);
#else
	Strcpy(mstr, killer);
	Strcat(mstr, killed_by_prefix[how]);
#endif
	break;
    case NO_KILLER_PREFIX:
	Strcpy(mstr, killer);
	break;
    }

    total_len = gdk_mbstowcs(wstr, mstr, NH_BUFSIZ);
    line = DEATH_LINE;
    wc = wstr;

    while(total_len > 0 && line < YEAR_LINE){
	len = total_len;
	while(1){
	    width = gdk_text_width_wc(rip->style->font, wc, len);
	    if(width < 96)
		break;
	    --len;
	}
	memcpy(rip_line[line].str, wc, len * sizeof(GdkWChar));
	rip_line[line].len = len;
	wc += len;
	total_len -= len;

	++line;
    }

    x = 155;
    y = 78;

    {
	gint dummy;
	gint height = 0, ascent, descent;

	for(line = 0 ; line <= YEAR_LINE ; ++line){
	    gdk_text_extents_wc(
		rip->style->font,
		rip_line[line].str, rip_line[line].len,
		&dummy,
		&dummy,
		&rip_line[line].width,
		&ascent,
		&descent);
	    if(height < (ascent + descent))
		height = ascent + descent;
	}

	for(line = 0 ; line <= YEAR_LINE ; ++line){
	    gdk_draw_text_wc(
		rip_pixmap,
		rip->style->font,
		rip->style->black_gc,
		x - rip_line[line].width / 2, y,
		rip_line[line].str, rip_line[line].len);
	    y += height;
	}
    }

    gtk_widget_show_all(w);
    gtk_main();
}

void
GTK_raw_print(const char *str)
{
  /*
    if(initialized)
	GTK_putstr(NHW_MESSAGE, 0, str);
    else
  */
	tty_raw_print(str);
}

void
GTK_raw_print_bold(const char *str)
{
  /*
    if(initialized)
	GTK_putstr(NHW_MESSAGE, ATR_BOLD, str);
    else
  */
	tty_raw_print_bold(str);
}
