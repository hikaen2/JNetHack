/*
**
**	$Id: jtrns.c,v 1.4 1996/07/17 06:03:45 issei Exp issei $
**
*/

/* Copyright (c) Issei Numata 1994-1996 */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#include <ctype.h>
#ifdef NULL
#undef NULL
#define NULL ((void *)0)
#endif

#include "jdata.h"

/* mode
**
**  0 - English(Original)
**  1 - Japanese
*/
static int lang_mode = 1;
static int trns_flg = 1;

static int 
hash_val(key)
     unsigned char *key;
{
  int v=0;

  ++key;/* skip first char */
  while(*key)
    v = (v * 3 + *(key++)) % 509;
  return v;
}

/*
**	English -> Japanese
*/
static const unsigned char *
  get(key, type)
unsigned char *key;
int type;
{
  int i = hash_tab[hash_val(key)];

  while ( 0 <= i )
    {
      const struct _jtrns_tab *p = &jtrns_tab[i];
      if ((type == ' ' || p->type == type) &&
	  !strcmp((char *)p->key, (char *)key))
	return p->val;
      i = p->next;
    }

  return NULL;
}

/*
**	Japanese -> English
*/
static const unsigned char *
  rev_get(val, type)
unsigned char *val;
int type;
{
  int i = rhash_tab[hash_val(val)];

  while ( 0 <= i )
    {
      const struct _jtrns_tab *p = &jtrns_tab[i];
      if ((type == ' ' || p->type == type) &&
	  !strcmp((char *)p->val, (char *)val))
	return p->key;
      i = p->rev_next;
    }

  return NULL;
}

void
init_jtrns()
{
  ;
}

int
dotogglelang()
{
  lang_mode = (!lang_mode);

  switch(lang_mode){
  case 0:
    pline("���ꥸ�ʥ�⡼��");
    break;
  case 1:
    pline("���ܸ�⡼��");
    break;
  }
  return 0;
}

int
query_lang_mode()
{
  return lang_mode;
}
static void
set_lang_mode(ln)
     int ln;
{
  lang_mode = ln;
}
void
set_trns_mode(flg)
     int flg;
{
  trns_flg = flg;
}
const char *
jtrns_mon(name, female)
     const char *name;
     int female;
{
  const char *ret;

  if(name==NULL)
    return NULL;

  if(!trns_flg)
    return name;

  ret = get(name, '@');

  if( ret==NULL )
    return name;

  if(female < 0)
    return ret;

  if(female){
    if(!strcmp(name, "werejackal") || !strcmp(name, "Werejackal"))
      return "����å����";
    else if(!strcmp(name, "werewolf") || !strcmp(name, "Werewolf"))
      return "ϵ��";
    else if(!strcmp(name, "wererat") || !strcmp(name, "Wererat"))
      return "�ͤ��߽�";
  }
  else{
    if(!strcmp(name, "werejackal") || !strcmp(name, "Werejackal"))
      return "����å�����";
    else if(!strcmp(name, "werewolf") || !strcmp(name, "Werewolf"))
      return "ϵ��";
    else if(!strcmp(name, "wererat") || !strcmp(name, "Wererat"))
      return "�ͤ�����";
  }

  return ret;
}
const char *
jtrns_obj(type,name)
     const int type;
     const char *name;
{
  const char *ret;

  if(name==NULL)
    return NULL;

  if(!trns_flg)
    return name;

  ret = (const char *)get(name, type);
  if( ret==NULL )
    return name;
  else
  return ret;
}
#if 0
/*
** this 2 functions does not transform
** if mode is original.
*/
static const char *
jtrns_mon2(name)
     const char *name;
{
  const char *ret;

  if(!lang_mode || !trns_flg)	/* original mode */
    return name;

  if(name==NULL)
    return NULL;

  ret =  (const char *)get(name, '@');
  if( ret==NULL )
    return name;
  return ret;
}
static const char *
jtrns_obj2(type,name)
     const int type;
     const char *name;
{
  const char *ret;

  if(!lang_mode || !trns_flg)	/* original mode */
    return name;

  if(name==NULL)
    return NULL;

  ret = (const char *)get(name, type);
  if( ret==NULL )
    return name;
  else
  return ret;
}
#endif
/*
**
*/
const char *
etrns_mon(name)
     const char *name;
{
  const char *ret;

  if(name==NULL)
    return NULL;

  ret = (const char *)rev_get(name, '@');
  if( ret==NULL )
    return name;
  else
    return ret;
}

const char *
etrns_obj(type, name)
     const int type;
     const char *name;
{
  const char *ret;

  if(name==NULL)
    return NULL;

  ret = (const char *)rev_get(name, type);
  if( ret==NULL )
    return name;
  else
    return ret;
}