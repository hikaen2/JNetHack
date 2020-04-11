/*
**
**	$Id: jlib.h,v 1.1.3.1 1999/11/17 06:35:05 issei Exp issei $
**
*/

/* Copyright (c) Issei Numata 1994-2000 */
/* JNetHack may be freely redistributed.  See license for details. */

#include <stdio.h>
#include <ctype.h>
#include "hack.h"

#define EUC	0
#define SJIS	1
#define JIS	2

/* internal kcode */
/* IC=0 EUC */
/* IC=1 SJIS */
#define IC ((unsigned char)("´Á"[0])==0x8a)

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
    p1 = jconvchar(*s++);
    while(*p1)
      *(p0++) = *(p1++);
  }
  *p0 = '\0';
  return (char *)buf;
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
  if(pos<0)
    return 1;
  else
    return 0;
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
/*
** split string(str) including japanese before pos and return to
** str1, str2.
*/
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
      if(!strncmp(str+i-j,"¡¡",2)){
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
      if(!strncmp(str+i-j,"¡ª",2)||
	 !strncmp(str+i-j,"¡©",2)||
	 !strncmp(str+i-j,"¡¢",2)||
	 !strncmp(str+i-j,"¡£",2)||
	 !strncmp(str+i-j,"¡¤",2)||
	 !strncmp(str+i-j,"¡¥",2)){
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

  while(!strncmp(str+i-j,"¡ª",2) ||
	!strncmp(str+i-j,"¡©",2) ||
	!strncmp(str+i-j,"¡¢",2) ||
	!strncmp(str+i-j,"¡£",2) ||
	!strncmp(str+i-j,"¡¤",2) ||
	!strncmp(str+i-j,"¡¥",2))
    --j;

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

void 
jrndm_replace(c)
     char *c;
{
  unsigned char cc[3];

  if(IC)
    memcpy(cc, (char *)sj2e(c), 2);
  else
    memcpy(cc, c, 2);

  cc[0] &= 0x7f;
  cc[1] &= 0x7f;

  switch(cc[0]){
  case 0x21:
    cc[1] = rn2(94) + 0x21;
    break;
  case 0x23:
    if(cc[1] <= 0x39) /* £°¡Á£¹ */
      cc[1] = rn2(10) + 0x30;
    else if(cc[1] <= 0x5A) /* £Á¡Á£Ú */
      cc[1] = rn2(26) + 0x41;
    else if(cc[2] <= 0x7A) /* £á¡Á£ú */
      cc[1] = rn2(26) + 0x61;
    break;
  case 0x24:
  case 0x25:
    cc[1] = rn2(83) + 0x21; /* ¤¢¡Á¤ó or ¥¢¡Á¥ó */
    break;
  case 0x26:
    if(cc[1] <= 0x30)
      cc[1] = rn2(24) + 0x21; /* ¦¡¡Á¦¸ ¥®¥ê¥·¥ãÊ¸»ú */
    else
      cc[1] = rn2(24) + 0x41; /* ¦Á¡Á¦Ø ¥®¥ê¥·¥ãÊ¸»ú */
    break;
  case 0x27:
    if(cc[1] <= 0x40)
      cc[1] = rn2(33) + 0x21; /* §¡¡Á§Á ¥í¥·¥¢Ê¸»ú */
    else
      cc[1] = rn2(33) + 0x51; /* §Ñ¡Á§ñ ¥í¥·¥¢Ê¸»ú */
    break;
  case 0x4f:
    cc[1] = rn2(51) + 0x21; /* Ï¡¡Á ÏÓ */
    break;
  case 0x74:
    cc[1] = rn2(4) + 0x21; /* ô¡ ô¢ ô£ ô¤ ¤Î4Ê¸»ú*/
    break;
  default:
    if(cc[0] >= 0x30 && cc[1] <= 0x74)
      cc[1] = rn2(94) + 0x21;
    break;
  }

  cc[0] |= 0x80;
  cc[1] |= 0x80;

  memcpy(c, str2ic(cc), 2);
}

