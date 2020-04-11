/*
  $Id: gtkmisc.c,v 1.3 1999/12/01 03:51:06 issei Exp issei $
 */
/*
  GTK+ NetHack Copyright (c) Issei Numata 1999-2000
  GTK+ NetHack may be freely redistributed.  See license for details. 
*/

#include <sys/types.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "winGTK.h"

static gboolean	 option_lock;
static int	 keysym;
static GtkWidget *entry_url;
static GtkWidget *entry_plname;
static GtkWidget *entry_dogname;
static GtkWidget *entry_catname;
static GtkWidget *entry_fruit;
static GtkWidget *entry_proxy, *entry_proxy_port;
static GtkWidget *radio_m, *radio_f;
static GtkWidget *radio_k, *radio_d, *radio_r;
static GtkWidget *radio_menu_t, *radio_menu_p, *radio_menu_c, *radio_menu_f;
static GtkWidget *radio_visual_monji, *radio_visual_tile;
static GtkWidget *radio_visual_bigtile, *radio_visual_big3dtile;

static struct GTK_Option{
    char      *opt_name;	
    char      *on;	
    char      *off;	
    boolean   *opt_p;	
    boolean   not;
    GSList    *group;
    GtkWidget *radio1;
    GtkWidget *radio2;
} gtk_option[] = {
#ifdef JNETHACK
    {"�ڥåȤؤι���", "�Ԥ�", "�Ԥʤ�ʤ�", &flags.safe_dog, 1},
    {"ͧ��Ū�ʲ�ʪ�ؤι���", "��ǧ����", "��ǧ���ʤ�", &flags.confirm},
#ifdef TEXTCOLOR
    {"�ڥå�", "���ȤǰϤ�", "�Ϥޤʤ�", &iflags.hilite_pet},
#endif
#ifdef RADAR
    {"�졼����", "ɽ������", "ɽ�����ʤ�", &flags.radar},
#endif
    {NULL,},
    {"�и���", "ɽ������", "ɽ�����ʤ�", &flags.showexp},
#ifdef SCORE_ON_BOTL
    {"������", "ɽ������", "ɽ�����ʤ�", &flags.showscore},
#endif
    {"���", "ɽ������", "ɽ�����ʤ�", &flags.time},
    {NULL,},
    {"��ư�ǥ����ƥ��", "����", "����ʤ�", &flags.pickup},
    {NULL,},
    {"���ϻ��Υ���ȥ���������", "ɽ������", "ɽ�����ʤ�", &flags.legacy},
#ifdef NEWS
    {"���ϻ��Υ˥塼��", "ɽ������", "ɽ�����ʤ�", &iflags.news},
#endif
#ifdef MAIL
    {"�᡼��ǡ����", "�о줹��", "�о줷�ʤ�", &flags.biff},
#endif
    {NULL,},
    {"���ڡ��������ǵ٤�", "�٤�", "�٤ޤʤ�", &flags.rest_on_space},
    {"��������Υ�å�����", "�ܤ����Ԥ�", "�Ԥʤ�ʤ�", &flags.verbose},
#ifdef NEWBIE
    {"�鿴�ԤؤΥҥ��", "�Ԥ�", "�Ԥʤ�ʤ�", &flags.newbie},
#endif
    {NULL,},
/*
    {"��λ���ξ���", "ɽ������", "ɽ�����ʤ�", &flags.end_disclose},
*/
    {"��λ��������", "ɽ������", "ɽ�����ʤ�", &flags.tombstone},
#ifdef NH_EXTENSION_REPORT
    {"�����������", "��𤹤�", "��𤷤ʤ�", &flags.reportscore},
#endif
    {NULL,},
    {"�����ƥ���Ͽ����1", "�Ǥ������Ʊ��ʸ���ˤ���", "���ʤ�", &flags.invlet_constant},
    {"�����ƥ���Ͽ����2", "�����褦�ʼ����ޤȤ��", "�ޤȤ�ʤ�", &flags.sortpack},
#else
    {"prevent you from attacking your pet", "Yes", "No", &flags.safe_dog},
    {"ask before hidding peaceful monsters", "Yes", "No", &flags.confirm},
#ifdef TEXTCOLOR
    {"display pets in a red square", "Yes", "No", &iflags.hilite_pet},
#endif
#ifdef RADAR
    {"display radar", "Yes", "No", &flags.radar},
#endif
    {NULL,},
    {"display experience points", "Yes", "No", &flags.showexp},
#ifdef SCORE_ON_BOTL
    {"display score points", "Yes", "No", &flags.showscore},
#endif
    {"display elapsed game time", "Yes", "No", &flags.time},
    {NULL,},
    {"automatically pick up objects", "Yes", "No", &flags.pickup},
    {NULL,},
    {"print introductory message", "Yes", "No", &flags.legacy},
#ifdef NEWS
    {"print any news", "Yes", "No", &iflags.news},
#endif
#ifdef MAIL
    {"enable the mail dameon", "Yes", "No", &flags.biff},
#endif
    {NULL,},
    {"space bar as a rest character", "Yes", "No", &flags.rest_on_space},
    {"print more commentary", "Yes", "No", &flags.verbose},
    {NULL,},
    {"print tombstone when die", "Yes", "No", &flags.tombstone},
    {NULL,},
    {"try to retain the same letter for the same objects", "Yes", "No", &flags.invlet_constant},
    {"group similar kinds of objects in inventory", "Yes", "No", &flags.sortpack},
#endif
};

static void	nh_option_set(void);

static gint
default_destroy(GtkWidget *widget, gpointer data)
{
    guint *hid = (guint *)data;
    *hid = 0;
    keysym = '\033';
    
    gtk_main_quit();

    return FALSE;
}

static gint
default_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    keysym = nh_keysym(event);

    if(keysym == '\n' || keysym == '\033')
      gtk_main_quit();
    
    return FALSE;
}

static gint
default_clicked(GtkWidget *widget, gpointer data)
{
    if(data)
	keysym = (int)data;
    else
	keysym = '\n';

    if(keysym == 'm'){
	doset();
	nh_option_set();
    }
    else
	gtk_main_quit();

    return FALSE;
}

static void
nh_option_set(void)
{
    int i;
    struct GTK_Option *p;

    if(flags.female)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_f), TRUE);
    if(preferred_pet == 'c' || preferred_pet == 'k')
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_k), TRUE);
    else if(preferred_pet == 'd')
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_d), TRUE);
    else
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_r), TRUE);

    switch(flags.menu_style){
    case MENU_TRADITIONAL:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_menu_t), TRUE);
	break;
    case MENU_PARTIAL:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_menu_p), TRUE);
	break;
    case MENU_COMBINATION:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_menu_c), TRUE);
	break;
    case MENU_FULL:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_menu_f), TRUE);
	break;
    }
	
    for(i=0 ; i<sizeof(gtk_option)/sizeof(struct GTK_Option) ; ++i){
	p = &gtk_option[i];
	p->group = NULL;
	if(p->opt_name){
	    if(p->opt_p && *p->opt_p){
		if(!p->not)
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->radio1), TRUE);
		else
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->radio2), TRUE);
	    }
	    else{
		if(!p->not)
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->radio2), TRUE);
		else
		    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->radio1), TRUE);
	    }
	    
	}
    }
}

static GtkWidget*
nh_option_plname_new()
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox, *hbox2;
  GtkWidget *label;
  GSList    *female_group = NULL;

#ifdef JNETHACK
  frame = gtk_frame_new("�ץ쥤�䡼");
#else
  frame = gtk_frame_new("Player");
#endif
/*  gtk_container_border_width(GTK_CONTAINER(frame), NH_PAD);*/
  
  vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), frame, "");
  hbox = nh_gtk_new_and_pack(gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);
#ifdef JNETHACK
  label = nh_gtk_new_and_pack(gtk_label_new("̾��:"), hbox, "", FALSE, FALSE, NH_PAD);
#else
  label = nh_gtk_new_and_pack(gtk_label_new("Name:"), hbox, "", FALSE, FALSE, NH_PAD);
#endif

  entry_plname = nh_gtk_new_and_pack(gtk_entry_new_with_max_length(PL_NSIZ),
			      hbox, "", FALSE, FALSE, NH_PAD);
  gtk_entry_set_text(GTK_ENTRY(entry_plname), (const gchar *)plname);

  hbox2 = nh_gtk_new_and_pack(gtk_hbox_new(FALSE, 0), vbox, "", FALSE, FALSE, NH_PAD);
#ifdef JNETHACK
  radio_m = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(female_group, "��"),
      hbox2, "", FALSE, FALSE, NH_PAD);
#else
  radio_m = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(female_group, "Male"),
      hbox2, "", FALSE, FALSE, NH_PAD);
#endif
  female_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_m));

#ifdef JNETHACK
  radio_f = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(female_group, "��"),
      hbox2, "", FALSE, FALSE, NH_PAD);
#else
  radio_f = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(female_group, "Female"),
      hbox2, "", FALSE, FALSE, NH_PAD);
#endif
  female_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_f));

  gtk_widget_set_sensitive(GTK_WIDGET(label), !option_lock);
  gtk_widget_set_sensitive(GTK_WIDGET(radio_f), !option_lock);
  gtk_widget_set_sensitive(GTK_WIDGET(radio_m), !option_lock);

  return frame;
}

static GtkWidget *
nh_option_pet_kitten_new()
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

#ifdef JNETHACK
  frame = gtk_frame_new("ǭ");
#else
  frame = gtk_frame_new("Kitten");
#endif
  vbox = nh_gtk_new_and_add(
      gtk_vbox_new(FALSE, 0), frame, "");

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0),
      vbox, "", FALSE, FALSE, NH_PAD);

#ifdef JNETHACK
  label = nh_gtk_new_and_pack(
      gtk_label_new("̾��:"),
      hbox, "", FALSE, FALSE, NH_PAD);
#else
  label = nh_gtk_new_and_pack(
      gtk_label_new("Name:"),
      hbox, "", FALSE, FALSE, NH_PAD);
#endif
  entry_catname = nh_gtk_new_and_pack(
      gtk_entry_new_with_max_length(PL_NSIZ),
      hbox, "", FALSE, FALSE, NH_PAD);

  gtk_entry_set_text(GTK_ENTRY(entry_catname), (const gchar *)catname);

  gtk_widget_set_sensitive(GTK_WIDGET(label), !option_lock);

  return frame;
}

static GtkWidget *
nh_option_pet_dog_new()
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

#ifdef JNETHACK
  frame = gtk_frame_new("��");
#else
  frame = gtk_frame_new("Dog");
#endif
  vbox = nh_gtk_new_and_add(
      gtk_vbox_new(FALSE, 0), frame, "");

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0),
      vbox, "", FALSE, FALSE, NH_PAD);

#ifdef JNETHACK
  label = nh_gtk_new_and_pack(
      gtk_label_new("̾��:"),
      hbox, "", FALSE, FALSE, NH_PAD);
#else
  label = nh_gtk_new_and_pack(
      gtk_label_new("Name:"),
      hbox, "", FALSE, FALSE, NH_PAD);
#endif
  entry_dogname = nh_gtk_new_and_pack(
      gtk_entry_new_with_max_length(PL_NSIZ),
      hbox, "", FALSE, FALSE, NH_PAD);

  gtk_entry_set_text(GTK_ENTRY(entry_dogname), (const gchar *)dogname);

  gtk_widget_set_sensitive(GTK_WIDGET(label), !option_lock);

  return frame;
}

static GtkWidget *
nh_option_pet_new()
{
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox, *hbox2;
  GtkWidget *kitten;
  GtkWidget *dog;
  GSList    *pet_group = NULL;

#ifdef JNETHACK
  frame = gtk_frame_new("�ڥå�");
#else
  frame = gtk_frame_new("Pet");
#endif
/*  gtk_container_border_width(GTK_CONTAINER(w), NH_PAD);*/

  
  vbox = nh_gtk_new_and_add(
      gtk_vbox_new(FALSE, 0), frame, "");

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0), vbox, "",
      FALSE, FALSE, NH_PAD);

  kitten = nh_gtk_new_and_pack(
      nh_option_pet_kitten_new(), hbox, "",
      FALSE, FALSE, NH_PAD);

  dog = nh_gtk_new_and_pack(
      nh_option_pet_dog_new(), hbox, "",
      FALSE, FALSE, NH_PAD);

  hbox2 = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0), vbox, "",
      FALSE, FALSE, NH_PAD);

#ifdef JNETHACK
  label = nh_gtk_new_and_pack(
      gtk_label_new("�������Ȼ��Υڥå�:"), hbox2, "",
      FALSE, FALSE, NH_PAD);
#else
  label = nh_gtk_new_and_pack(
      gtk_label_new("Start with:"), hbox2, "",
      FALSE, FALSE, NH_PAD);
#endif

#ifdef JNETHACK
  radio_k = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(pet_group, "ǭ"), hbox2, "",
      FALSE, FALSE, NH_PAD);
  pet_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_k));

  radio_d = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(pet_group, "��"), hbox2, "",
      FALSE, FALSE, NH_PAD);
  pet_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_d));

  radio_r = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(pet_group, "������"), hbox2, "",
      FALSE, FALSE, NH_PAD);
  pet_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_r));
#else
  radio_k = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(pet_group, "Kitten"), hbox2, "",
      FALSE, FALSE, NH_PAD);
  pet_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_k));

  radio_d = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(pet_group, "Dog"), hbox2, "",
      FALSE, FALSE, NH_PAD);
  pet_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_d));

  radio_r = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(pet_group, "Random"), hbox2, "",
      FALSE, FALSE, NH_PAD);
  pet_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_r));
#endif

  gtk_widget_set_sensitive(GTK_WIDGET(radio_k), !option_lock);
  gtk_widget_set_sensitive(GTK_WIDGET(radio_d), !option_lock);
  gtk_widget_set_sensitive(GTK_WIDGET(radio_r), !option_lock);

  return frame;
}

static GtkWidget *
nh_option_fruit_new()
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

#ifdef JNETHACK
  frame = gtk_frame_new("��ʪ�β�ʪ");
#else
  frame = gtk_frame_new("Fruit");
#endif
  vbox = nh_gtk_new_and_add(
      gtk_vbox_new(FALSE, 0), frame, "");

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0),
      vbox, "", FALSE, FALSE, NH_PAD);

#ifdef JNETHACK
  label = nh_gtk_new_and_pack(
      gtk_label_new("̾��:"),
      hbox, "", FALSE, FALSE, NH_PAD);
#else
  label = nh_gtk_new_and_pack(
      gtk_label_new("Name:"),
      hbox, "", FALSE, FALSE, NH_PAD);
#endif
  entry_fruit = nh_gtk_new_and_pack(
      gtk_entry_new_with_max_length(PL_NSIZ),
      hbox, "", FALSE, FALSE, NH_PAD);

  gtk_entry_set_text(GTK_ENTRY(entry_fruit), (const gchar *)pl_fruit);

  return frame;
}

#ifdef NH_EXTENSION_REPORT
static GtkWidget *
nh_option_url_new()
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  frame = gtk_frame_new("������������URL");

  vbox = nh_gtk_new_and_add(
      gtk_vbox_new(FALSE, 0), frame, "");

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0),
      vbox, "", FALSE, FALSE, NH_PAD);

  label = nh_gtk_new_and_pack(
      gtk_label_new("URL:"),
      hbox, "", FALSE, FALSE, NH_PAD);

  entry_url = nh_gtk_new_and_pack(
      gtk_entry_new_with_max_length(128),
      hbox, "", TRUE, TRUE, NH_PAD);

  gtk_entry_set_text(GTK_ENTRY(entry_url), (const gchar *)get_homeurl());

  return frame;
}

static GtkWidget *
nh_option_proxy_new()
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  frame = gtk_frame_new("�ץ���");

  vbox = nh_gtk_new_and_add(
      gtk_vbox_new(FALSE, 0), frame, "");

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0),
      vbox, "", FALSE, FALSE, NH_PAD);

  label = nh_gtk_new_and_pack(
      gtk_label_new("������:"),
      hbox, "", FALSE, FALSE, NH_PAD);

  entry_proxy = nh_gtk_new_and_pack(
      gtk_entry_new_with_max_length(128),
      hbox, "", TRUE, TRUE, NH_PAD);

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0),
      vbox, "", FALSE, FALSE, NH_PAD);

  label = nh_gtk_new_and_pack(
      gtk_label_new("�ݡ���:"),
      hbox, "", FALSE, FALSE, NH_PAD);

  entry_proxy_port = nh_gtk_new_and_pack(
      gtk_entry_new_with_max_length(6),
      hbox, "", FALSE, FALSE, NH_PAD);

  {
      char *proxy;
      char port[16];
      sprintf(port, "%d", get_proxy_port());

      proxy = get_proxy_host();

      gtk_entry_set_text(GTK_ENTRY(entry_proxy), (const gchar *)proxy);
      if(*proxy)
	  gtk_entry_set_text(GTK_ENTRY(entry_proxy_port), (const gchar *)port);
  }

  return frame;
}
#endif

static GtkWidget *
nh_option_menu_new()
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GSList    *menu_group = NULL;

#ifdef JNETHACK
  frame = gtk_frame_new("��˥塼");
#else
  frame = gtk_frame_new("Menu style");
#endif

  vbox = nh_gtk_new_and_add(
      gtk_vbox_new(FALSE, 0), frame, "");

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0),
      vbox, "", FALSE, FALSE, NH_PAD);

  radio_menu_t = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "Traditional"), hbox, "",
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_menu_t));

  radio_menu_p = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "Partial"), hbox, "",
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_menu_t));

  radio_menu_c = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "Combination"), hbox, "",
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_menu_t));

  radio_menu_f = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "Full"), hbox, "",
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_menu_t));

  return frame;
}


static GtkWidget *
nh_option_player_new()
{
  GtkWidget *vbox;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(vbox), NH_PAD);

  nh_gtk_new_and_pack(
      nh_option_plname_new(), vbox, "",
      FALSE, FALSE, NH_PAD);

  nh_gtk_new_and_pack(
      nh_option_pet_new(), vbox, "",
      FALSE, FALSE, NH_PAD);

  nh_gtk_new_and_pack(
      nh_option_fruit_new(), vbox, "",
      FALSE, FALSE, NH_PAD);

#ifdef NH_EXTENSION_REPORT
  nh_gtk_new_and_pack(
      nh_option_url_new(), vbox, "",
      FALSE, FALSE, NH_PAD);
#endif

  return vbox;
}

static GtkWidget *
nh_option_game_new()
{
  int i;
  GtkWidget *htmp;
  GtkWidget *ltmp;
  GtkWidget *stmp;
  GtkWidget *tbl;
  struct GTK_Option *p;

  tbl = gtk_table_new(sizeof(gtk_option)/sizeof(struct GTK_Option)*2, 3, FALSE);
  gtk_container_border_width(GTK_CONTAINER(tbl), NH_PAD);

  for(i=0 ; i<sizeof(gtk_option)/sizeof(struct GTK_Option) ; ++i){
      p = &gtk_option[i];
      p->group = NULL;
      if(p->opt_name){
	  htmp = nh_gtk_new_and_attach(
	      gtk_hbox_new(FALSE, 0), tbl, "",
	      0, 1, i*2, i*2 + 1);
	  ltmp = nh_gtk_new_and_pack(
	      gtk_label_new(p->opt_name), htmp, "",
	      FALSE, FALSE, NH_PAD);
	  
	  htmp = nh_gtk_new_and_attach(
	      gtk_hbox_new(FALSE, 0), tbl, "",
	      1, 2, i*2, i*2 +1);
	  
	  p->radio1 = nh_gtk_new_and_pack(
	      gtk_radio_button_new_with_label(p->group, p->on), htmp, "",
	      FALSE, FALSE, 0);
	  p->group = gtk_radio_button_group(GTK_RADIO_BUTTON(p->radio1));
	  
	  htmp = nh_gtk_new_and_attach(
	      gtk_hbox_new(FALSE, 0), tbl, "",
	      2, 3, i*2, i*2 +1);
	  
	  p->radio2 = nh_gtk_new_and_pack(
	      gtk_radio_button_new_with_label(p->group, p->off), htmp, "", 
	      FALSE, FALSE, 0);
	  p->group = gtk_radio_button_group(GTK_RADIO_BUTTON(p->radio2));
      }
      else{
	  stmp = nh_gtk_new_and_attach(
	      gtk_hseparator_new(), tbl, "",
	      0, 3, i*2 +1, i*2 +2);
      }
  }
  return tbl;
}

static GtkWidget *
nh_option_visual_new()
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GSList    *menu_group = NULL;

#ifdef JNETHACK
  frame = gtk_frame_new("�ޥå�");
#else
  frame = gtk_frame_new("Map visual");
#endif

  vbox = nh_gtk_new_and_add(
      gtk_vbox_new(FALSE, 0), frame, "");

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0),
      vbox, "", FALSE, FALSE, NH_PAD);

#ifdef JNETHACK
  radio_visual_monji = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "ʸ��"), hbox, "", 
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_visual_monji));

  radio_visual_tile = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "������"), hbox, "", 
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_visual_tile));

#ifdef BIGTILE
  radio_visual_bigtile = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "������(��)"), hbox, "", 
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_visual_bigtile));
#endif
#ifdef BIG3DTILE
  radio_visual_big3dtile = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "3D������(��)"), hbox, "", 
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_visual_bigtile));
#endif

#else
  radio_visual_monji = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "Characters"), hbox, "", 
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_visual_monji));

  radio_visual_tile = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "Tiles"), hbox, "", 
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_visual_tile));

#ifdef BIGTILE
  radio_visual_bigtile = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "Big tiles"), hbox, "", 
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_visual_tile));
#endif
#ifdef BIG3DTILE
  radio_visual_big3dtile = nh_gtk_new_and_pack(
      gtk_radio_button_new_with_label(menu_group, "Big 3D tiles"), hbox, "", 
      FALSE, FALSE, NH_PAD);
  menu_group = gtk_radio_button_group(GTK_RADIO_BUTTON(radio_visual_tile));
#endif
#endif

  switch(nh_get_map_visual()){
  case 0:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_visual_monji), TRUE);
      break;
  case 1:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_visual_tile), TRUE);
      break;
  case 2:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_visual_bigtile), TRUE);
      break;
  case 3:
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_visual_big3dtile), TRUE);
      break;
  }

  return frame;
}

static GtkWidget *
nh_option_display_new()
{
  GtkWidget *vbox;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(vbox), NH_PAD);

#ifdef NH_EXTENSION_REPORT
  nh_gtk_new_and_pack(
      nh_option_proxy_new(), vbox, "",
      FALSE, FALSE, NH_PAD);
#endif

  nh_gtk_new_and_pack(
      nh_option_menu_new(), vbox, "",
      FALSE, FALSE, NH_PAD);

  nh_gtk_new_and_pack(
      nh_option_visual_new(), vbox, "",
      FALSE, FALSE, NH_PAD);

  return vbox;

}


void
nh_option_new()
{
  guint	hid;	
  GtkWidget *w;
  GtkWidget *note;
  GtkWidget *vbox, *hbox;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *button3;

  w = gtk_window_new(GTK_WINDOW_DIALOG);
  gtk_container_border_width(GTK_CONTAINER(w), NH_PAD);
  gtk_signal_connect(
      GTK_OBJECT(w), "key_press_event",
      GTK_SIGNAL_FUNC(default_key_press), NULL);
  hid = gtk_signal_connect(
      GTK_OBJECT(w), "destroy",
      GTK_SIGNAL_FUNC(default_destroy), &hid);
  gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);

  vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), w, "");

  note = nh_gtk_new_and_pack(
      gtk_notebook_new(), vbox, "",
      FALSE, FALSE, NH_PAD);

  nh_gtk_new_and_add(nh_option_player_new(), note, "");
  nh_gtk_new_and_add(nh_option_game_new(), note, "");
  nh_gtk_new_and_add(nh_option_display_new(), note, "");

#ifdef JNETHACK
  gtk_notebook_set_tab_label_text(
      GTK_NOTEBOOK(note),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(note), 0), 
      "�ץ쥤�䡼");

  gtk_notebook_set_tab_label_text(
      GTK_NOTEBOOK(note),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(note), 1), 
      "������");

  gtk_notebook_set_tab_label_text(
      GTK_NOTEBOOK(note),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(note), 2), 
      "����¾");
#else
  gtk_notebook_set_tab_label_text(
      GTK_NOTEBOOK(note),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(note), 0), 
      "Player");

  gtk_notebook_set_tab_label_text(
      GTK_NOTEBOOK(note),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(note), 1), 
      "Game");

  gtk_notebook_set_tab_label_text(
      GTK_NOTEBOOK(note),
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(note), 2), 
      "Misc");
#endif

  hbox = nh_gtk_new_and_pack(
      gtk_hbox_new(FALSE, 0), vbox, "",
      FALSE, FALSE, NH_PAD);

#ifdef JNETHACK
  button1 = nh_gtk_new_and_pack(
      gtk_button_new_with_label("��λ"), hbox, "",
      FALSE, FALSE, NH_PAD);
  button3 = nh_gtk_new_and_pack(
      gtk_button_new_with_label("�ܺ�����"), hbox, "",
      FALSE, FALSE, NH_PAD);
  button2 = nh_gtk_new_and_pack(
      gtk_button_new_with_label("����󥻥�"), hbox, "",
      FALSE, FALSE, NH_PAD);
#else
  button1 = nh_gtk_new_and_pack(
      gtk_button_new_with_label("OK"), hbox, "",
      FALSE, FALSE, NH_PAD);
  button3 = nh_gtk_new_and_pack(
      gtk_button_new_with_label("More Option"), hbox, "",
      FALSE, FALSE, NH_PAD);
  button2 = nh_gtk_new_and_pack(
      gtk_button_new_with_label("Cancell"), hbox, "",
      FALSE, FALSE, NH_PAD);
#endif

  gtk_signal_connect(
      GTK_OBJECT(button1), "clicked",
      GTK_SIGNAL_FUNC(default_clicked), (gpointer)'\n');

  gtk_signal_connect(
      GTK_OBJECT(button2), "clicked",
      GTK_SIGNAL_FUNC(default_clicked), (gpointer)'\033');

  gtk_signal_connect(
      GTK_OBJECT(button3), "clicked",
      GTK_SIGNAL_FUNC(default_clicked), (gpointer)'m');

  nh_option_set();

  gtk_entry_set_editable(GTK_ENTRY(entry_plname), !option_lock);
  gtk_entry_set_editable(GTK_ENTRY(entry_dogname), !option_lock);
  gtk_entry_set_editable(GTK_ENTRY(entry_catname), !option_lock);
  gtk_widget_set_sensitive(GTK_WIDGET(entry_plname), !option_lock);
  gtk_widget_set_sensitive(GTK_WIDGET(entry_dogname), !option_lock);
  gtk_widget_set_sensitive(GTK_WIDGET(entry_catname), !option_lock);

  gtk_widget_show_all(w);

  gtk_grab_add(w);

  main_hook();

  if(keysym == '\n'){
      Strcpy(plname, gtk_entry_get_text(GTK_ENTRY(entry_plname)));
      Strcpy(catname, gtk_entry_get_text(GTK_ENTRY(entry_catname)));
      Strcpy(dogname, gtk_entry_get_text(GTK_ENTRY(entry_dogname)));
      Strcpy(pl_fruit, gtk_entry_get_text(GTK_ENTRY(entry_fruit)));
#ifdef NH_EXTENSION
      set_homeurl(gtk_entry_get_text(GTK_ENTRY(entry_url)));
      {
	  char buf[BUFSIZ];
	  char port[16];

	  sprintf(port, "%s", gtk_entry_get_text(GTK_ENTRY(entry_proxy_port)));
	  if(*port)
	      snprintf(buf, BUFSIZ, "%s:%s", 
		       gtk_entry_get_text(GTK_ENTRY(entry_proxy)), port);
	  else
	      snprintf(buf, BUFSIZ, "%s", 
		       gtk_entry_get_text(GTK_ENTRY(entry_proxy)));

	  set_proxy(buf);
      }
#endif
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_f)))
	  flags.female = 1;
      else
	  flags.female = 0;

      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_k)))
	  preferred_pet = 'c';
      else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_d)))
	  preferred_pet = 'd';
      else
	  preferred_pet = 0;

      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_menu_t)))
	  flags.menu_style = MENU_TRADITIONAL;
      else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_menu_p)))
	  flags.menu_style = MENU_COMBINATION;
      else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_menu_c)))
	  flags.menu_style = MENU_PARTIAL;
      else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_menu_f)))
	  flags.menu_style = MENU_FULL;

      {
	  int i;
	  struct GTK_Option *p;

	  for(i=0 ; i<sizeof(gtk_option)/sizeof(struct GTK_Option) ; ++i){
	      p = &gtk_option[i];
	      if(p->opt_name){
		  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->radio1)))
		      *p->opt_p = !p->not;
		  else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->radio2)))
		      *p->opt_p = !!p->not;
	      }
	  }
      }
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_visual_monji)))
	  nh_set_map_visual(0);
      else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_visual_tile)))
	  nh_set_map_visual(1);
      else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_visual_bigtile)))
	  nh_set_map_visual(2);
      else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_visual_big3dtile)))
	  nh_set_map_visual(3);
  }
  nh_status_index_update();

  if(hid > 0){
      gtk_signal_disconnect(GTK_OBJECT(w), hid);

      gtk_widget_destroy(w);
  }
}

void
nh_option_lock()
{
    option_lock = TRUE;
}
