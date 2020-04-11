/*
**
**	$Id: jconj.c,v 1.3 1994/06/28 03:29:31 issei Exp issei $
**
*/

/* Copyright (c) Issei Numata 1994 */
/* JNetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <ctype.h>
#include "hack.h"

#define EUC	0
#define SJIS	1
#define JIS	2

/* internal kcode */
/* IC=0 EUC */
/* IC=0 SJIS */
#define IC ((unsigned char)("漢"[0])==0x8a)

/* default input kcode */
#ifndef INPUT_KCODE
# ifdef MSDOS
#  define INPUT_KCODE SJIS
# else
#  define INPUT_KCODE EUC
# endif
#endif

/* default output kcode */
#ifndef OUTPUT_KCODE
# ifdef MSDOS
#  define OUTPUT_KCODE SJIS
# else
#  define OUTPUT_KCODE EUC
# endif
#endif

static int	output_kcode = OUTPUT_KCODE;
static int	input_kcode = INPUT_KCODE;

#define J_A	0
#define J_KA	(1*5)
#define J_SA	(2*5)
#define J_TA	(3*5)
#define J_NA	(4*5)
#define J_HA	(5*5)
#define J_MA	(6*5)
#define J_YA	(7*5)
#define J_RA	(8*5)
#define J_WA	(9*5)

#define J_GA	(10*5)
#define J_ZA	(11*5)
#define J_DA	(12*5)
#define J_BA	(13*5)
#define J_PA	(14*5)

static unsigned char hira_tab[][2]={
  {0xa4, 0xa2}, {0xa4, 0xa4}, {0xa4, 0xa6}, {0xa4, 0xa8}, {0xa4, 0xaa}, 
  {0xa4, 0xab}, {0xa4, 0xad}, {0xa4, 0xaf}, {0xa4, 0xb1}, {0xa4, 0xb3}, 
  {0xa4, 0xb5}, {0xa4, 0xb7}, {0xa4, 0xb9}, {0xa4, 0xbb}, {0xa4, 0xbd}, 
  {0xa4, 0xbf}, {0xa4, 0xc1}, {0xa4, 0xc4}, {0xa4, 0xc6}, {0xa4, 0xc8}, 
  {0xa4, 0xca}, {0xa4, 0xcb}, {0xa4, 0xcc}, {0xa4, 0xcd}, {0xa4, 0xce}, 
  {0xa4, 0xcf}, {0xa4, 0xd2}, {0xa4, 0xd5}, {0xa4, 0xd8}, {0xa4, 0xdb}, 
  {0xa4, 0xde}, {0xa4, 0xdf}, {0xa4, 0xe0}, {0xa4, 0xe1}, {0xa4, 0xe2}, 
  {0xa4, 0xe4}, {0xa4, 0xa4}, {0xa4, 0xe6}, {0xa4, 0xa8}, {0xa4, 0xe8}, 
  {0xa4, 0xe9}, {0xa4, 0xea}, {0xa4, 0xeb}, {0xa4, 0xec}, {0xa4, 0xed}, 
  {0xa4, 0xef}, {0xa4, 0xa4}, {0xa4, 0xa6}, {0xa4, 0xa8}, {0xa4, 0xaa}, 
  {0xa4, 0xac}, {0xa4, 0xae}, {0xa4, 0xb0}, {0xa4, 0xb2}, {0xa4, 0xb4}, 
  {0xa4, 0xb6}, {0xa4, 0xb8}, {0xa4, 0xba}, {0xa4, 0xbc}, {0xa4, 0xbe}, 
  {0xa4, 0xc0}, {0xa4, 0xc2}, {0xa4, 0xc5}, {0xa4, 0xc7}, {0xa4, 0xc9}, 
  {0xa4, 0xd0}, {0xa4, 0xd3}, {0xa4, 0xd6}, {0xa4, 0xd9}, {0xa4, 0xdc}, 
  {0xa4, 0xd1}, {0xa4, 0xd4}, {0xa4, 0xd7}, {0xa4, 0xda}, {0xa4, 0xdd},
};

#define FIFTH	0
#define UPPER	1
#define LOWER	2
#define SAHEN	3
#define KAHEN	4
#define NAHEN	5

#define NORMAL	0
#define SOKUON	1
#define HATSUON	2	
#define ION	3
	
struct _jconj_tab {
  char *main;
  int column;
/* 0: fifth conj. 1:upper conj. 2:lower conj. 3:SAHEN 4:KAHEN */
  int katsuyo_type;
/* 0: normal 1: sokuon 2: hatson 3: ion */
  int onbin_type;
} jconj_tab[] = {
  {"来る", J_KA, KAHEN, NORMAL}, 
  {"する", J_SA, SAHEN, NORMAL}, 
  {"食べる", J_HA, LOWER, NORMAL}, 
  {"読む", J_MA, FIFTH, SOKUON},
  {"脱ぐ", J_GA, FIFTH, ION},
  {"着る", J_KA, UPPER, NORMAL},
  {"身につける", J_KA, LOWER, NORMAL},
  {"はずす", J_SA, FIFTH, NORMAL},
  {"外す", J_SA, FIFTH, NORMAL},
  {"捧げる", J_KA, LOWER, NORMAL},
  {"書く", J_KA, FIFTH, ION},
  {"こする", J_RA, FIFTH, HATSUON},
  {"投げる", J_GA, LOWER, NORMAL},
  {"落す", J_SA, FIFTH, NORMAL},
  {"置く", J_KA, FIFTH, ION},
  {"殺す", J_SA, FIFTH, NORMAL},
  {"死ぬ", J_NA, FIFTH, SOKUON},
  {"落ちる", J_TA, UPPER, NORMAL},
  {"入れる", J_RA, LOWER, NORMAL},
  {"いれる", J_RA, LOWER, NORMAL},
  {"出す", J_SA, FIFTH, NORMAL},
  {"拾う", J_WA, FIFTH, ION},
  {"飲む", J_MA, FIFTH, SOKUON},
  {"錆びる", J_BA, UPPER, NORMAL},
  {"濡らす", J_SA, FIFTH, NORMAL},
  {"浸す", J_SA, FIFTH, NORMAL},
  {"使う", J_WA, FIFTH, HATSUON},
  {"打つ", J_TA, FIFTH, HATSUON},
  {"浮く", J_KA, FIFTH, ION},
  {"飛ぶ", J_BA, FIFTH, SOKUON},
  {"滑る", J_RA, FIFTH, HATSUON},
  {"出る", J_NA, LOWER, NORMAL},
  {"はいずる", J_RA, FIFTH, HATSUON},
  {"踏む", J_MA, FIFTH, SOKUON},
  {"つまずく", J_KA, FIFTH, ION},
  {"かける", J_KA, UPPER, NORMAL},
  {"あける", J_KA, LOWER, NORMAL},
  {"塗る", J_RA, FIFTH, NORMAL},
  {"加える", J_A, LOWER, NORMAL},
  {"刻む", J_MA, FIFTH, SOKUON},
  {"こます", J_SA, FIFTH, NORMAL},
  {"名づける", J_KA, LOWER, NORMAL},
  {"呼ぶ", J_BA, FIFTH, NORMAL},
  {"焼く", J_KA, FIFTH, ION},
  {"つける", J_KA, LOWER, NORMAL},
  {"壊す", J_SA, FIFTH, NORMAL},
  {(void*)0,0,0,0},
};

/*
**	Kanji code library....
*/
void
setkcode(c)
     int c;
{
  if(c == 'E' || c == 'e' )
    output_kcode = EUC;
  else if(c == 'J' || c == 'j')
    output_kcode = JIS;
  else if(c == 'S' || c == 's')
    output_kcode = SJIS;
  else{
    fprintf(stderr,"kcode error! use default.\n");
    output_kcode = OUTPUT_KCODE;
  }
  input_kcode = output_kcode;
}
/*
**	EUC->SJIS
*/
static unsigned char *
e2sj(s)
     unsigned char *s;
{
  unsigned char h,l;
  static unsigned char sw[2];

  h = s[0] & 0x7f;
  l = s[1] & 0x7f;

  sw[0] = ((h - 1) >> 1)+ ((h <= 0x5e) ? 0x71 : 0xb1);
  sw[1] = l + ((h & 1) ? ((l < 0x60) ? 0x1f : 0x20) : 0x7e);

  return sw;
}
/*
**	SJIS->EUC
*/
static unsigned char *
sj2e(s)
     unsigned char *s;
{
  unsigned int h,l;
  static unsigned char sw[2];

  h = s[0];
  l = s[1];

  h = h + h - ((h <=0x9f) ? 0x00e1 : 0x0161);
  if( l<0x9f )
    l = l - ((l > 0x7f) ? 0x20 : 0x1f);
  else{
    l = l-0x7e;
    ++h;
  }
  sw[0] = h | 0x80;
  sw[1] = l | 0x80;
  return sw;
}
/*
**	translate string to internal kcode
*/
const char *
str2ic(s)
     const char *s;
{
  static unsigned char buf[1024];
  unsigned char *u;
  unsigned char *p,*pp;
  int kin;

  if(!s)
    return s;

  buf[0] = '\0';

  if( IC==input_kcode ){
    strcpy((char *)buf, s);
    return (char *)buf;
  }

  p = buf;
  if( !IC && input_kcode == SJIS ){
    while(*s){
      u = (unsigned char *)s;
      if( *u & 0x80 ){
	pp = sj2e((unsigned char *)s);
	*(p++) = pp[0];
	*(p++) = pp[1];
	s += 2;
      }
      else
	*(p++) = (unsigned char)*(s++);
    }
  }
  else if( !IC && input_kcode == JIS ){
    kin = 0;
    while(*s){
      if(s[0] == 033 && s[1] == '$' && (s[2] == 'B' || s[3] == '@')){
	kin = 1;
	s += 3;
      }
      else if(s[0] == 033 && s[1] == '(' && (s[2] == 'B' || s[3] == 'J')){
	kin = 0;
	s += 3;
      }
      else if( kin )
	*(p++) = (*(s++) | 0x80);
      else
	*(p++) = *(s++);
    }
  }
  else
      return (char *)buf;

  *(p++) = '\0';
  return (char *)buf;
}
/*
**	low level function
*/
static int kmode;

static unsigned char *
jconvchar(c)
     int c;
{
  unsigned char uc,*pb,*pp;
  unsigned char us[2];
  static unsigned char prev_char,buf[16];

  uc = (*((unsigned int *)&c)) & 0xff;
  pb = buf;

  if(c=='\0'){
    if(kmode && output_kcode==JIS ){
      *(pb++) = 033;
      *(pb++) = '(';
      *(pb++) = 'B';
    }
    kmode = 0;
    *(pb) = '\0';
    return buf;
  }

  if( output_kcode==IC ){
    *(pb++) = uc;
  }
  else{
    if( kmode==0 && uc>=0x80 ){
      if( output_kcode==JIS ){
	*(pb++) = 033;
	*(pb++) = '$';
	*(pb++) = 'B';
      }
      kmode = 1;
    }
    else if( kmode==1 && uc<0x80 ){
      if( output_kcode==JIS ){
	*(pb++) = 033;
	*(pb++) = '(';
	*(pb++) = 'B';
      }
      kmode = 0;
    }
    
    if( kmode==0 )
      *(pb++) = uc;
    else if( kmode==1 ){
      prev_char = uc;
      ++kmode;
    }
    else if( kmode==2 ){
      us[0] = prev_char;
      us[1] = uc;
      if(IC){ /* sjis */
	pp = sj2e(us);
	if( output_kcode==EUC ){
	  *(pb++) = pp[0];
	  *(pb++) = pp[1];
	}
	else{
	  *(pb++) = (pp[0]&0x7f);
	  *(pb++) = (pp[1]&0x7f);
	}
      }
      else{
	if( output_kcode==SJIS ){
	  pp = e2sj(us);
	  *(pb++) = pp[0];
	  *(pb++) = pp[1];
	}
	else{
	  *(pb++) = prev_char &0x7f;
	  *(pb++) = uc & 0x7f;
	}
      }
      kmode = 1;
    }
  }
  *pb = '\0';
  return buf;
}
void
cputc(c,fp)
     int c;
     FILE *fp;
{
  if(kmode!=0){
    jputchar('\0');
  }
  putc(c,fp);
}
void
cputchar(c)
     int c;
{
  cputc(c,stdout);
}
void
jputc(c,fp)
     int c;
     FILE *fp;
{
  fputs((char *)jconvchar(c),fp);
}
void
jputchar(c)
     int c;
{
  jputc(c,stdout);
}
void
jfputs(s,fp)
     const char *s;
     FILE *fp;
{
  while(*s)
    jputc((unsigned char)*s++,fp);
}
void
jputs(s)
     const char *s;
{
  while(*s)
    jputchar((unsigned char)*s++);
  putchar('\n');
}
const char *
jconvstr(s)
     const char *s;
{
  static unsigned char buf[4096];
  unsigned char *p0,*p1;

  p0 = buf;
  while(*s){
    p1 = jconvchar(s++);
    while(*p1)
      *(p0++) = *(p1++);
  }
  *p0 = '\0';
  return (char *)buf;
}

/*
**	conjection verb word
**
**	Example
**	arg1	arg2	result
**	脱ぐ	ない	脱がない
**	脱ぐ	た	脱いだ
**
*/
static char *
jconjsub( tab,jverb,sfx )
     struct _jconj_tab *tab;
     char *jverb;
     char *sfx;
{
  int len;
  unsigned char *p;
  static unsigned char tmp[1024];

  len = strlen(jverb);
  strcpy((char *)tmp,jverb );

  if(!strncmp(sfx,"と",2)){
    strcat((char *)tmp,sfx);
    return (char *)tmp;
  }

  switch( tab->katsuyo_type ){
  case FIFTH:
    p = tmp+(len-2);
    if(!strcmp(sfx,"な")){
      if(!IC){
	p[0]= 0xa4;
	p[1]= hira_tab[tab->column][1];
      }
      else
	memcpy(p,e2sj(hira_tab[tab->column]),2);

      strcpy((char *)p+2,sfx);
      break;
    }
    else if(!strncmp(sfx,"た",2) || !strncmp(sfx,"て",2)){
      switch( tab->onbin_type ){
      case NORMAL:
	if(!IC)
	  p[1]=hira_tab[tab->column+1][1];
	else
	  memcpy(p,e2sj(hira_tab[tab->column+1]),2);
	break;
      case SOKUON:
	if(!IC)
	  p[1]= 0xf3;
	else
	  memcpy(p,"ん",2);
	break;
      case HATSUON:
	if(!IC)
	  p[1]= 0xc3;
	else
	  memcpy(p,"っ",2);
	break;
      case ION:
	if(!IC)
	  p[1]= 0xa4;
	else
	  memcpy(p,"い",2);
	break;
      }
      strcpy((char *)p+2,sfx);
      if(tab->onbin_type==SOKUON || (tab->onbin_type==ION &&tab->column>=J_GA)){
	if(!IC)
	  ++p[3];
	else
	  memcpy(p+2,e2sj(sj2e(p+2)+1),2);
      }
      break;
    }
    else if(!strncmp(sfx,"ば",2)){
      if(!IC)
	p[1]=hira_tab[tab->column+3][1];
      else
	memcpy(p,e2sj(hira_tab[tab->column+3]),2);
      strcpy((char *)p+2,sfx);
    }
    else if(!strncmp(sfx,"れば",4)){
      if(!IC)
	p[1]=hira_tab[tab->column+3][1];
      else
	memcpy(p,e2sj(hira_tab[tab->column+3]),2);

      strcpy((char *)p+2,sfx+2);
    }
    else if(!strncmp(sfx,"ま",2)) {
      if(!IC)
	p[1]=hira_tab[tab->column+1][1];
      else
	memcpy(p,e2sj(hira_tab[tab->column+1]),2);
      strcpy((char *)p+2,sfx);
      break;
    }
    break;
  case UPPER:
  case LOWER:
  case KAHEN:
    p = tmp+(len-2);
    if(!strncmp(sfx,"ば",2)){
      strcpy((char *)p,"れ");
      strcpy((char *)p+2,"れ");
    }
    else
      strcpy((char *)p,sfx);
    break;
  case SAHEN:
    p = tmp+(len-4);
    if(!strncmp(sfx,"な",2)||!strncmp(sfx,"ま",2)||!strncmp(sfx,"た",2)||!strncmp(sfx,"て",2)){
      strcpy((char *)p,"し");
      strcpy((char *)p+2,sfx);
    }
    else if(!strncmp(sfx,"ば",2)||!strncmp(sfx,"れば",4)){
      strcpy((char *)p,"すれば");
    }
    break;
  }
  return (char *)tmp;
}
const char *
jconj( jverb,sfx )
     const char *jverb;
     const char *sfx;
{
  struct _jconj_tab *tab;
  int len;

  len = strlen(jverb);
  for( tab=jconj_tab ; tab->main!=(void*)0 ;++tab )
    if(!strcmp(jverb,tab->main)){
      return jconjsub( tab,jverb,sfx );
    }

  for( tab=jconj_tab ; tab->main!=(void*)0 ;++tab )
    if(len-strlen(tab->main)>0&&!strcmp(jverb+(len-strlen(tab->main)),tab->main))
      return jconjsub( tab,jverb,sfx );

#ifdef JAPANESETEST
  fprintf( stderr,"I don't know such word \"%s\"\n");
#endif
  return jverb;
}
/*
**	conjection of adjective word
**
**	Example:
**
**	形容詞的用法	   副詞的用法
**
**	赤い		-> 赤く		(形容詞)
**	綺麗な		-> 綺麗に	(形容動詞)
**	綺麗だ		-> 綺麗に	(形容動詞)
*/
const char *
jconj_adj( jadj )
     const char *jadj;
{
  int len;
  static unsigned char tmp[1024];

  strcpy((char *)tmp,jadj);
  len = strlen((char *)tmp);

  if(!strcmp((char *)tmp+len-2,"い"))
    strcpy((char *)tmp+len-2,"く");
  else if(!strcmp((char *)tmp+len-2,"だ")||
	  !strcmp((char *)tmp+len-2,"な")||
	  !strcmp((char *)tmp+len-2,"の"))
    strcpy((char *)tmp+len-2,"に");

  return (char *)tmp;
}

int is_kanji2(s,pos)
const char *s;
int pos;
{
  unsigned char *str;

  str = (unsigned char *)s;
  while(*str && pos>0){
    if(*str>=0x80){
      str+=2;
      pos-=2;
    }
    else{
      ++str;
      --pos;
    }
  }
  if(!*str)
    return 0;
  else
    return -pos;	/* if pos<0 this character is 2nd byte */
}

int is_kanji1(s,pos)
const char *s;
int pos;
{
  unsigned char *str;

  str = (unsigned char *)s;
  while(*str && pos>0){
    if(*str>=0x80){
      str+=2;
      pos-=2;
    }
    else{
      ++str;
      --pos;
    }
  }
  if(!pos && *str>=0x80)
    return 1;
  else
    return 0;
}

/*
**	8bit through isspace for Japanese
*/
int
isspace_8(c)
     int c;
{
  unsigned int *u;

  u = (unsigned int *)&c;
  return *u<0x80 ? isspace(*u) : 0;
}
void
split_japanese( str, str1, str2, pos )
     char *str;
     char *str1;
     char *str2;
     int pos;
{
  int len, i, j, k;
  char *pstr;
  char *pnstr;

  len = strlen((char *)str);

  if( len < pos ){
    strcpy(str1,str);
    *str2 = '\0';
    return;
  }

  i = pos;
  if(is_kanji2(str, i))
    --i;
/* 1:
** search space character
*/
  j = 0;
  while( j<20 ){
    if(isspace_8(str[i-j])){
      str[i-j] = '\0';
      --j;
      goto found;
    }
    else if(is_kanji1(str,i-j)){
      if(!strncmp(str+i-j,"　",2)){
	str[i-j] = '\0';
	j -= 2;
      }
    }
    ++j;
  }
/* 2:
** search end of japanese
*/
  j = 0;
  while( j<20 ){
    if((is_kanji1(str,i-j) && !is_kanji2(str,i-j-1))||
       (is_kanji2(str,i-j-1) && !is_kanji1(str,i-j))){
      goto found;
    }
    ++j;
  }
/* 3:
** search japanese special character
*/
  j = 2;
  while( j<20 ){
    if(is_kanji1(str,i-j)){
      if(!strncmp(str+i-j,"！",2)||
	 !strncmp(str+i-j,"？",2)||
	 !strncmp(str+i-j,"、",2)||
	 !strncmp(str+i-j,"。",2)||
	 !strncmp(str+i-j,"，",2)||
	 !strncmp(str+i-j,"．",2)){
	j -= 2;
	goto found;
      }
    }
    ++j;
  }
/* 4:
** search second bytes of japanese
*/
  j = 0;
  while( j<20 ){
    if(is_kanji1(str,i-j)){
      goto found;
    }
    ++j;
  }
 found:

  pstr = str;

  pnstr = str1;
  for( k=0 ; k<i-j ; ++k )
    *(pnstr++) = *(pstr++);
  *(pnstr++) = '\0';

  pnstr = str2;
  for( ; str[k] ; ++k )
    *(pnstr++) = *(pstr++);
  *(pnstr++) = '\0';
}
#ifdef JAPANESETEST
main()
{
  struct _jconj_tab *tab;

  for( tab=jconj_tab ; tab->main!=(void*)0 ;++tab ){
    printf("%s %s\n",tab->main,jconj(tab->main,"ない"));
    printf("%s %s\n",tab->main,jconj(tab->main,"ます"));
    printf("%s %s\n",tab->main,jconj(tab->main,"た"));
    printf("%s %s\n",tab->main,jconj(tab->main,"れば"));
    printf("%s %s\n",tab->main,jconj(tab->main,"とき"));
  }
  printf("%s\n",jconj("徹夜でnethackの翻訳をする","ない"));
  printf("%s\n",jconj("徹夜でnethackの翻訳をする","ます"));
  printf("%s\n",jconj("徹夜でnethackの翻訳をする","た"));
  printf("%s\n",jconj("徹夜でnethackの翻訳をする","れば"));
  printf("%s\n",jconj("徹夜でnethackの翻訳をする","とき"));
}
#endif
