/*
**
**	$Id: jtrns.c,v 1.3 1994/06/28 03:29:31 issei Exp issei $
**
*/

/* Copyright (c) Issei Numata 1994 */
/* JNetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#ifdef _MSC_VER
#include "emalloc.h"
#define	alloc(s) emalloc(s)
extern int strcmp(const char *, const char *);
extern char *strcpy(char *, const char *);
static int ems_result;
#endif
#include <ctype.h>
#ifdef NULL
#undef NULL
#define NULL ((void *)0)
#endif
#ifdef _MSC_VER
static const
#endif
#include "jdata.h"

struct _jtrns_rec{
  unsigned char *key;
  unsigned char *val;
  char type;
  struct _jtrns_rec *next;
};

#ifdef _MSC_VER
static struct _jtrns_rec *jtrns_montab[509];
static struct _jtrns_rec *jtrns_objtab[509];
#else
static struct _jtrns_rec *jtrns_montab[509];
static struct _jtrns_rec *jtrns_objtab[509];
#endif

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
    v = (v*3 + *(key++))%509;
  return v;
}
static void
add(tab,key,val,type)
     struct _jtrns_rec **tab;
     unsigned char *key;
     unsigned char *val;
     int type;
{
  struct _jtrns_rec **p;

  p = &(tab[hash_val(key)]);
  while((*p)!=NULL)
    p = &((*p)->next);

  *p = (struct _jtrns_rec *)alloc(sizeof(struct _jtrns_rec));
  (*p)->key = (unsigned char *)alloc(strlen((char *)key)+1);
  strcpy((char *)(*p)->key,(char *)key);
  (*p)->val = (unsigned char *)alloc(strlen((char *)val)+1);
  strcpy((char *)(*p)->val,(char *)val);
  (*p)->next = NULL;
  (*p)->type = type;

/*  if(*key=='%')
    fprintf( stderr,"%s:%s\n",key,val);*/
}
static const unsigned char *
get(tab,key,type)
     struct _jtrns_rec **tab;
     unsigned char *key;
     int type;
{
  struct _jtrns_rec **p;

  p = &(tab[hash_val(key)]);
  while((*p)!=NULL)
    if((type == ' ' || (*p)->type == type) &&
       !strcmp((char *)((*p)->key),(char *)key))
      return (*p)->val;
    else
      p = &((*p)->next);

  return NULL;
}
void
init_jtrns()
{
  struct _jtrns_tab *p;
#ifdef _MSC_VER
  if ((ems_result = detect_ems()) == 0)
  {
    fprintf(stderr, "物体とモンスターは日本語で表示出来ません。\n");
    lang_mode = 0;
    return;
  }
  jtrns_montab = (struct _jtrns_rec **)
    alloc(sizeof(struct _jtrns_rec *) * 509);
  jtrns_objtab = (struct _jtrns_rec **)
    alloc(sizeof(struct _jtrns_rec *) * 509);
#endif
  p = jtrns_tab;
  while(p->type != '\0'){
    if(p->type=='@')
      add(jtrns_montab,p->key,p->val,p->type);
    else
      add(jtrns_objtab,p->key,p->val,p->type);
    ++p;
  }
}
int
dotogglelang()
{
#ifdef _MSC_VER
  if (ems_result == 0)
  {
    pline("EMS が使用出来ないので、日本語モードにする事は出来ません");
    return 0;
  }
#endif
  lang_mode = (!lang_mode);

  switch(lang_mode){
  case 0:
    pline("オリジナルモード");
    break;
  case 1:
    pline("日本語モード");
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
jtrns_mon(name)
     const char *name;
{
  const char *ret;

  if(name==NULL)
    return NULL;

  if(!trns_flg)
    return name;

  ret =  (const char *)get(jtrns_montab,name,'@');
  if( ret==NULL )
    return name;
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

  ret = (const char *)get(jtrns_objtab,name,type);
  if( ret==NULL )
    return name;
  else
  return ret;
}
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

  ret =  (const char *)get(jtrns_montab,name,'@');
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

  ret = (const char *)get(jtrns_objtab,name,type);
  if( ret==NULL )
    return name;
  else
  return ret;
}
