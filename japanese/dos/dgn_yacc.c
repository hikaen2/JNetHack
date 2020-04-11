
/*  A Bison parser, made from dgn_comp.y  */

#define	INTEGER	258
#define	A_DUNGEON	259
#define	BRANCH	260
#define	CHBRANCH	261
#define	LEVEL	262
#define	RNDLEVEL	263
#define	CHLEVEL	264
#define	RNDCHLEVEL	265
#define	UP_OR_DOWN	266
#define	PROTOFILE	267
#define	DESCRIPTION	268
#define	DESCRIPTOR	269
#define	LEVELDESC	270
#define	ALIGNMENT	271
#define	LEVALIGN	272
#define	ENTRY	273
#define	STAIR	274
#define	NO_UP	275
#define	NO_DOWN	276
#define	PORTAL	277
#define	STRING	278

#line 1 "dgn_comp.y"

/*	SCCS Id: @(#)dgn_comp.c	3.1	93/05/15	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/*	Copyright (c) 1990 by M. Stephenson				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the Dungeon Compiler code
 */

/* In case we're using bison in AIX.  This definition must be
 * placed before any other C-language construct in the file
 * excluding comments and preprocessor directives (thanks IBM
 * for this wonderful feature...).
 *
 * Note: some cpps barf on this 'undefined control' (#pragma).
 * Addition of the leading space seems to prevent barfage for now,
 * and AIX will still see the directive in its non-standard locale.
 */

#ifdef _AIX
 #pragma alloca		/* keep leading space! */
#endif

#include "config.h"
#include "dgn_file.h"

void FDECL(yyerror, (const char *));
void FDECL(yywarning, (const char *));
int NDECL(yylex);
int NDECL(yyparse);
int FDECL(getchain, (char *));
int NDECL(check_dungeon);
int NDECL(check_branch);
int NDECL(check_level);
void NDECL(init_dungeon);
void NDECL(init_branch);
void NDECL(init_level);
void NDECL(output_dgn);

#ifdef AMIGA
# undef	printf
#ifndef	LATTICE
# define    memset(addr,val,len)    setmem(addr,len,val)
#endif
#endif

#ifdef MICRO
# undef exit
extern void FDECL(exit, (int));
#endif

#undef NULL

#define ERR		(-1)

static struct couple couple;
static struct tmpdungeon tmpdungeon[MAXDUNGEON];
static struct tmplevel tmplevel[LEV_LIMIT];
static struct tmpbranch tmpbranch[BRANCH_LIMIT];

static int in_dungeon = 0, n_dgns = -1, n_levs = -1, n_brs = -1;

extern int fatal_error;
extern const char *fname;


#line 69 "dgn_comp.y"
typedef union
{
	int	i;
	char*	str;
} YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __STDC__
#define const
#endif



#define	YYFINAL		108
#define	YYFLAG		-32768
#define	YYNTBASE	30

#define YYTRANSLATE(x) ((unsigned)(x) <= 278 ? yytranslate[x] : 53)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    27,
    29,     2,    26,    28,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    24,     2,     2,
     2,     2,     2,    25,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23
};

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    84,    85,    91,    92,    95,    96,    97,    98,   101,   117,
   121,   127,   128,   129,   132,   138,   141,   148,   157,   163,
   164,   165,   166,   167,   170,   184,   201,   216,   234,   241,
   250,   266,   285,   302,   322,   323,   326,   339,   353,   357,
   361,   365,   369,   375,   379,   402,   439
};

static const char * const yytname[] = {     0,
"error","$illegal.","INTEGER","A_DUNGEON","BRANCH","CHBRANCH","LEVEL","RNDLEVEL","CHLEVEL","RNDCHLEVEL",
"UP_OR_DOWN","PROTOFILE","DESCRIPTION","DESCRIPTOR","LEVELDESC","ALIGNMENT","LEVALIGN","ENTRY","STAIR","NO_UP",
"NO_DOWN","PORTAL","STRING","':'","'@'","'+'","'('","','","')'","file"
};
#endif

static const short yyr1[] = {     0,
    30,    30,    31,    31,    32,    32,    32,    32,    33,    34,
    34,    35,    35,    35,    36,    37,    38,    38,    39,    40,
    40,    40,    40,    40,    41,    41,    42,    42,    43,    43,
    44,    44,    45,    45,    46,    46,    47,    48,    49,    49,
    49,    49,    49,    50,    50,    51,    52
};

static const short yyr2[] = {     0,
     0,     1,     1,     2,     1,     1,     1,     1,     6,     0,
     1,     1,     1,     1,     3,     1,     3,     3,     3,     1,
     1,     1,     1,     1,     6,     7,     7,     8,     3,     3,
     7,     8,     8,     9,     1,     1,     7,     8,     0,     1,
     1,     1,     1,     0,     1,     5,     5
};

static const short yydefact[] = {     1,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     2,     3,     5,     6,    12,    13,    16,
    14,     8,    20,    21,    22,    23,    24,     7,    35,    36,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     4,     0,     0,     0,     0,     0,     0,
     0,    19,    17,    29,    18,    30,    15,     0,     0,     0,
     0,     0,     0,     0,     0,    10,     0,    39,     0,     0,
     0,     0,     0,     0,    11,     9,     0,    40,    41,    42,
    43,    44,    39,    25,     0,     0,     0,     0,     0,    45,
    37,    44,    27,    26,    31,     0,     0,     0,    38,    28,
    33,    32,    47,    46,    34,     0,     0,     0
};

static const short yydefgoto[] = {   106,
    14,    15,    16,    76,    17,    18,    19,    20,    21,    22,
    23,    24,    25,    26,    27,    28,    29,    30,    82,    91,
    68,    66
};

static const short yypact[] = {    -3,
   -16,   -13,    -8,     1,     2,     3,     4,     5,     6,     7,
     8,     9,    10,    -3,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    12,    13,    14,    15,    16,    17,    18,    19,    29,    30,
    31,    32,    44,-32768,    25,    24,    27,    28,    33,    34,
    35,-32768,-32768,-32768,-32768,-32768,-32768,    26,    36,    38,
    37,    40,    43,    45,    49,    51,    52,     0,    26,    36,
    36,    41,    46,    42,-32768,-32768,    47,-32768,-32768,-32768,
-32768,    48,     0,    57,    58,    26,    26,    66,    68,-32768,
-32768,    48,-32768,    70,    71,    73,    50,    53,-32768,-32768,
-32768,    74,-32768,-32768,-32768,    78,    80,-32768
};

static const short yypgoto[] = {-32768,
-32768,    67,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    11,    -9,
   -47,   -69
};


#define	YYLAST		94


static const short yytable[] = {    83,
     1,     2,     3,     4,     5,     6,     7,    31,     8,     9,
    32,    10,    11,    12,    13,    33,    95,    96,    78,    79,
    80,    81,    84,    85,    34,    35,    36,    37,    38,    39,
    40,    41,    42,    43,    45,    46,    47,    48,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
    61,    74,    65,    75,    77,    62,    63,    64,    90,    93,
    94,    70,    67,    69,    71,    72,    86,    73,    97,    88,
    98,    87,   100,   101,    89,   102,   105,   107,   103,   108,
    44,   104,    99,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    92
};

static const short yycheck[] = {    69,
     4,     5,     6,     7,     8,     9,    10,    24,    12,    13,
    24,    15,    16,    17,    18,    24,    86,    87,    19,    20,
    21,    22,    70,    71,    24,    24,    24,    24,    24,    24,
    24,    24,    24,    24,    23,    23,    23,    23,    23,    23,
    23,    23,    14,    14,    14,    14,     3,    23,    25,    23,
    23,     3,    27,     3,     3,    23,    23,    23,    11,     3,
     3,    25,    27,    26,    25,    23,    26,    23,     3,    28,
     3,    26,     3,     3,    28,     3,     3,     0,    29,     0,
    14,    29,    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    83
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* Not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__)
#include <alloca.h>
#endif /* Sparc.  */
#endif /* Not GNU C.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYIMPURE
#define YYLEX		yylex()
#endif

#ifndef YYPURE
#define YYLEX		yylex(&yylval, &yylloc)
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYIMPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* YYIMPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#line 131 "bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/

#define YYPOPSTACK   (yyvsp--, yysp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yysp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifndef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
#ifdef YYLSP_NEEDED
		 &yyls1, size * sizeof (*yylsp),
#endif
		 &yystacksize);

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Next token is %d (%s)\n", yychar, yytname[yychar1]);
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      if (yylen == 1)
	fprintf (stderr, "Reducing 1 value via rule %d (line %d), ",
		 yyn, yyrline[yyn]);
      else
	fprintf (stderr, "Reducing %d values via rule %d (line %d), ",
		 yylen, yyn, yyrline[yyn]);
    }
#endif


  switch (yyn) {

case 2:
#line 86 "dgn_comp.y"
{
			output_dgn();
		  ;
    break;}
case 9:
#line 102 "dgn_comp.y"
{
			init_dungeon();
			strcpy(tmpdungeon[n_dgns].name, yyvsp[-3].str);
			if (!strcmp(yyvsp[-2].str, "none"))
				tmpdungeon[n_levs].boneschar = '\0';
			else if (yyvsp[-2].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmpdungeon[n_dgns].boneschar = yyvsp[-2].str[0];
			tmpdungeon[n_dgns].lev.base = couple.base;
			tmpdungeon[n_dgns].lev.rand = couple.rand;
			tmpdungeon[n_dgns].chance = yyvsp[0].i;
		  ;
    break;}
case 10:
#line 118 "dgn_comp.y"
{
			yyval.i = 0;
		  ;
    break;}
case 11:
#line 122 "dgn_comp.y"
{
			yyval.i = yyvsp[0].i;
		  ;
    break;}
case 15:
#line 133 "dgn_comp.y"
{
			tmpdungeon[n_dgns].entry_lev = yyvsp[0].i;
		  ;
    break;}
case 17:
#line 142 "dgn_comp.y"
{
			if(yyvsp[0].i <= TOWN || yyvsp[0].i >= D_ALIGN_CHAOTIC)
			    yyerror("Illegal description - ignoring!");
			else
			    tmpdungeon[n_dgns].flags |= yyvsp[0].i ;
		  ;
    break;}
case 18:
#line 149 "dgn_comp.y"
{
			if(yyvsp[0].i && yyvsp[0].i < D_ALIGN_CHAOTIC)
			    yyerror("Illegal alignment - ignoring!");
			else
			    tmpdungeon[n_dgns].flags |= yyvsp[0].i ;
		  ;
    break;}
case 19:
#line 158 "dgn_comp.y"
{
			strcpy(tmpdungeon[n_dgns].protoname, yyvsp[0].str);
		  ;
    break;}
case 25:
#line 171 "dgn_comp.y"
{
			init_level();
			strcpy(tmplevel[n_levs].name, yyvsp[-3].str);
			if (!strcmp(yyvsp[-2].str, "none"))
				tmplevel[n_levs].boneschar = '\0';
			else if (yyvsp[-2].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmplevel[n_levs].boneschar = yyvsp[-2].str[0];
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmpdungeon[n_dgns].levels++;
		  ;
    break;}
case 26:
#line 185 "dgn_comp.y"
{
			init_level();
			strcpy(tmplevel[n_levs].name, yyvsp[-4].str);
			if (!strcmp(yyvsp[-3].str, "none"))
				tmplevel[n_levs].boneschar = '\0';
			else if (yyvsp[-3].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmplevel[n_levs].boneschar = yyvsp[-3].str[0];
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].rndlevs = yyvsp[0].i;
			tmpdungeon[n_dgns].levels++;
		  ;
    break;}
case 27:
#line 202 "dgn_comp.y"
{
			init_level();
			strcpy(tmplevel[n_levs].name, yyvsp[-4].str);
			if (!strcmp(yyvsp[-3].str, "none"))
				tmplevel[n_levs].boneschar = '\0';
			else if (yyvsp[-3].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmplevel[n_levs].boneschar = yyvsp[-3].str[0];
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].chance = yyvsp[0].i;
			tmpdungeon[n_dgns].levels++;
		  ;
    break;}
case 28:
#line 217 "dgn_comp.y"
{
			init_level();
			strcpy(tmplevel[n_levs].name, yyvsp[-5].str);
			if (!strcmp(yyvsp[-4].str, "none"))
				tmplevel[n_levs].boneschar = '\0';
			else if (yyvsp[-4].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmplevel[n_levs].boneschar = yyvsp[-4].str[0];
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].chance = yyvsp[-1].i;
			tmplevel[n_levs].rndlevs = yyvsp[0].i;
			tmpdungeon[n_dgns].levels++;
		  ;
    break;}
case 29:
#line 235 "dgn_comp.y"
{
			if(yyvsp[0].i >= D_ALIGN_CHAOTIC)
			    yyerror("Illegal description - ignoring!");
			else
			    tmplevel[n_levs].flags |= yyvsp[0].i ;
		  ;
    break;}
case 30:
#line 242 "dgn_comp.y"
{
			if(yyvsp[0].i && yyvsp[0].i < D_ALIGN_CHAOTIC)
			    yyerror("Illegal alignment - ignoring!");
			else
			    tmplevel[n_levs].flags |= yyvsp[0].i ;
		  ;
    break;}
case 31:
#line 251 "dgn_comp.y"
{
			init_level();
			strcpy(tmplevel[n_levs].name, yyvsp[-4].str);
			if (!strcmp(yyvsp[-3].str, "none"))
				tmplevel[n_levs].boneschar = '\0';
			else if (yyvsp[-3].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmplevel[n_levs].boneschar = yyvsp[-3].str[0];
			tmplevel[n_levs].chain = getchain(yyvsp[-2].str);
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			if(!check_level()) n_levs--;
			else tmpdungeon[n_dgns].levels++;
		  ;
    break;}
case 32:
#line 267 "dgn_comp.y"
{
			init_level();
			strcpy(tmplevel[n_levs].name, yyvsp[-5].str);
			if (!strcmp(yyvsp[-4].str, "none"))
				tmplevel[n_levs].boneschar = '\0';
			else if (yyvsp[-4].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmplevel[n_levs].boneschar = yyvsp[-4].str[0];
			tmplevel[n_levs].chain = getchain(yyvsp[-3].str);
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].rndlevs = yyvsp[0].i;
			if(!check_level()) n_levs--;
			else tmpdungeon[n_dgns].levels++;
		  ;
    break;}
case 33:
#line 286 "dgn_comp.y"
{
			init_level();
			strcpy(tmplevel[n_levs].name, yyvsp[-5].str);
			if (!strcmp(yyvsp[-4].str, "none"))
				tmplevel[n_levs].boneschar = '\0';
			else if (yyvsp[-4].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmplevel[n_levs].boneschar = yyvsp[-4].str[0];
			tmplevel[n_levs].chain = getchain(yyvsp[-3].str);
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].chance = yyvsp[0].i;
			if(!check_level()) n_levs--;
			else tmpdungeon[n_dgns].levels++;
		  ;
    break;}
case 34:
#line 303 "dgn_comp.y"
{
			init_level();
			strcpy(tmplevel[n_levs].name, yyvsp[-6].str);
			if (!strcmp(yyvsp[-5].str, "none"))
				tmplevel[n_levs].boneschar = '\0';
			else if (yyvsp[-5].str[1])
				yyerror("Bones marker must be a single char, or \"none\"!");
			else
				tmplevel[n_levs].boneschar = yyvsp[-5].str[0];
			tmplevel[n_levs].chain = getchain(yyvsp[-4].str);
			tmplevel[n_levs].lev.base = couple.base;
			tmplevel[n_levs].lev.rand = couple.rand;
			tmplevel[n_levs].chance = yyvsp[-1].i;
			tmplevel[n_levs].rndlevs = yyvsp[0].i;
			if(!check_level()) n_levs--;
			else tmpdungeon[n_dgns].levels++;
		  ;
    break;}
case 37:
#line 327 "dgn_comp.y"
{
			init_branch();
			strcpy(tmpbranch[n_brs].name, yyvsp[-4].str);
			tmpbranch[n_brs].lev.base = couple.base;
			tmpbranch[n_brs].lev.rand = couple.rand;
			tmpbranch[n_brs].type = yyvsp[-1].i;
			tmpbranch[n_brs].up = yyvsp[0].i;
			if(!check_branch()) n_brs--;
			else tmpdungeon[n_dgns].branches++;
		  ;
    break;}
case 38:
#line 340 "dgn_comp.y"
{
			init_branch();
			strcpy(tmpbranch[n_brs].name, yyvsp[-5].str);
			tmpbranch[n_brs].chain = getchain(yyvsp[-4].str);
			tmpbranch[n_brs].lev.base = couple.base;
			tmpbranch[n_brs].lev.rand = couple.rand;
			tmpbranch[n_brs].type = yyvsp[-1].i;
			tmpbranch[n_brs].up = yyvsp[0].i;
			if(!check_branch()) n_brs--;
			else tmpdungeon[n_dgns].branches++;
		  ;
    break;}
case 39:
#line 354 "dgn_comp.y"
{
			yyval.i = TBR_STAIR;	/* two way stair */
		  ;
    break;}
case 40:
#line 358 "dgn_comp.y"
{
			yyval.i = TBR_STAIR;	/* two way stair */
		  ;
    break;}
case 41:
#line 362 "dgn_comp.y"
{
			yyval.i = TBR_NO_UP;	/* no up staircase */
		  ;
    break;}
case 42:
#line 366 "dgn_comp.y"
{
			yyval.i = TBR_NO_DOWN;	/* no down staircase */
		  ;
    break;}
case 43:
#line 370 "dgn_comp.y"
{
			yyval.i = TBR_PORTAL;	/* portal connection */
		  ;
    break;}
case 44:
#line 376 "dgn_comp.y"
{
			yyval.i = 0;	/* defaults to down */
		  ;
    break;}
case 45:
#line 380 "dgn_comp.y"
{
			yyval.i = yyvsp[0].i;
		  ;
    break;}
case 46:
#line 403 "dgn_comp.y"
{
			if (yyvsp[-3].i < -MAXLEVEL || yyvsp[-3].i > MAXLEVEL) {
			    yyerror("Abs base out of dlevel range - zeroing!");
			    couple.base = couple.rand = 0;
			} else if (yyvsp[-1].i < -1 ||
				((yyvsp[-3].i < 0) ? (MAXLEVEL + yyvsp[-3].i + yyvsp[-1].i + 1) > MAXLEVEL :
					(yyvsp[-3].i + yyvsp[-1].i) > MAXLEVEL)) {
			    yyerror("Abs range out of dlevel range - zeroing!");
			    couple.base = couple.rand = 0;
			} else {
			    couple.base = yyvsp[-3].i;
			    couple.rand = yyvsp[-1].i;
			}
		  ;
    break;}
case 47:
#line 440 "dgn_comp.y"
{
			if (yyvsp[-3].i < -MAXLEVEL || yyvsp[-3].i > MAXLEVEL) {
			    yyerror("Rel base out of dlevel range - zeroing!");
			    couple.base = couple.rand = 0;
			} else {
			    couple.base = yyvsp[-3].i;
			    couple.rand = yyvsp[-1].i;
			}
		  ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 362 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) xmalloc(size + 15);
	  strcpy(msg, "parse error");

	  if (count < 5)
	    {
	      count = 0;
	      for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
		if (yycheck[x + yyn] == x)
		  {
		    strcat(msg, count == 0 ? ", expecting `" : " or `");
		    strcat(msg, yytname[x]);
		    strcat(msg, "'");
		    count++;
		  }
	    }
	  yyerror(msg);
	  free(msg);
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 450 "dgn_comp.y"


void
init_dungeon()
{
	if(++n_dgns > MAXDUNGEON) {
	    fprintf(stderr, "FATAL - Too many dungeons (limit: %d).\n",
		    MAXDUNGEON);
	    fprintf(stderr, "To increase the limit edit MAXDUNGEON in global.h\n");
	    exit(1);
	}

	in_dungeon = 1;
	tmpdungeon[n_dgns].lev.base = 0;
	tmpdungeon[n_dgns].lev.rand = 0;
	tmpdungeon[n_dgns].chance = 100;
	strcpy(tmpdungeon[n_dgns].name, "");
	strcpy(tmpdungeon[n_dgns].protoname, "");
	tmpdungeon[n_dgns].flags = 0;
	tmpdungeon[n_dgns].levels = 0;
	tmpdungeon[n_dgns].branches = 0;
	tmpdungeon[n_dgns].entry_lev = 0;
}

void
init_level()
{
	if(++n_levs > LEV_LIMIT) {

		yyerror("FATAL - Too many special levels defined.");
		exit(1);
	}
	tmplevel[n_levs].lev.base = 0;
	tmplevel[n_levs].lev.rand = 0;
	tmplevel[n_levs].chance = 100;
	tmplevel[n_levs].rndlevs = 0;
	tmplevel[n_levs].flags = 0;
	strcpy(tmplevel[n_levs].name, "");
	tmplevel[n_levs].chain = -1;
}

void
init_branch()
{
	if(++n_brs > BRANCH_LIMIT) {

		yyerror("FATAL - Too many special levels defined.");
		exit(1);
	}
	tmpbranch[n_brs].lev.base = 0;
	tmpbranch[n_brs].lev.rand = 0;
	strcpy(tmpbranch[n_brs].name, "");
	tmpbranch[n_brs].chain = -1;
}

int
getchain(s)
	char	*s;
{
	int i;

	if(strlen(s)) {

	    for(i = n_levs - tmpdungeon[n_dgns].levels + 1; i <= n_levs; i++)
		if(!strcmp(tmplevel[i].name, s)) return i;

	    yyerror("Can't locate the specified chain level.");
	    return(-2);
	}
	return(-1);
}

/*
 *	Consistancy checking routines:
 *
 *	- A dungeon must have a unique name.
 *	- A dungeon must have a originating "branch" command
 *	  (except, of course, for the first dungeon).
 *	- A dungeon must have a proper depth (at least (1, 0)).
 */

int
check_dungeon()
{
	int i;

	for(i = 0; i < n_dgns; i++)
	    if(!strcmp(tmpdungeon[i].name, tmpdungeon[n_dgns].name)) {
		yyerror("Duplicate dungeon name.");
		return(0);
	    }

	if(n_dgns)
	  for(i = 0; i < n_brs - tmpdungeon[n_dgns].branches; i++) {
	    if(!strcmp(tmpbranch[i].name, tmpdungeon[n_dgns].name)) break;

	    if(i >= n_brs - tmpdungeon[n_dgns].branches) {
		yyerror("Dungeon cannot be reached.");
		return(0);
	    }
	  }

	if(tmpdungeon[n_dgns].lev.base <= 0 ||
	   tmpdungeon[n_dgns].lev.rand < 0) {
		yyerror("Invalid dungeon depth specified.");
		return(0);
	}
	return(1);	/* OK */
}

/*
 *	- A level must have a unique level name.
 *	- If chained, the level used as reference for the chain
 *	  must be in this dungeon, must be previously defined, and
 *	  the level chained from must be "non-probabalistic" (ie.
 *	  have a 100% chance of existing).
 */

int
check_level()
{
	int i;

	if(!in_dungeon) {
		yyerror("Level defined outside of dungeon.");
		return(0);
	}

	for(i = 0; i < n_levs; i++)
	    if(!strcmp(tmplevel[i].name, tmplevel[n_levs].name)) {
		yyerror("Duplicate level name.");
		return(0);
	    }

	if(tmplevel[i].chain == -2) {
		yyerror("Invaild level chain reference.");
		return(0);
	} else if(tmplevel[i].chain != -1) {	/* there is a chain */
	    if(tmplevel[tmpbranch[i].chain].chance != 100) {
		yyerror("Level cannot chain from a probabalistic level.");
		return(0);
	    } else if(tmplevel[i].chain == n_levs) {
		yyerror("A level cannot chain to itself!");
		return(0);
	    }
	}
	return(1);	/* OK */
}

/*
 *	- A branch may not branch backwards - to avoid branch loops.
 *	- A branch name must be unique.
 *	  (ie. You can only have one entry point to each dungeon).
 *	- If chained, the level used as reference for the chain
 *	  must be in this dungeon, must be previously defined, and
 *	  the level chained from must be "non-probabalistic" (ie.
 *	  have a 100% chance of existing).
 */

int
check_branch()
{
	int i;

	if(!in_dungeon) {
		yyerror("Branch defined outside of dungeon.");
		return(0);
	}

	for(i = 0; i < n_dgns; i++)
	    if(!strcmp(tmpdungeon[i].name, tmpbranch[n_brs].name)) {

		yyerror("Reverse branching not allowed.");
		return(0);
	    }

	if(tmpbranch[i].chain == -2) {

		yyerror("Invaild branch chain reference.");
		return(0);
	} else if(tmpbranch[i].chain != -1) {	/* it is chained */

	    if(tmplevel[tmpbranch[i].chain].chance != 100) {
		yyerror("Branch cannot chain from a probabalistic level.");
		return(0);
	    }
	}
	return(1);	/* OK */
}

/*
 *	Output the dungon definition into a file.
 *
 *	The file will have the following format:
 *
 *	[ number of dungeons ]
 *	[ first dungeon struct ]
 *	[ levels for the first dungeon ]
 *	  ...
 *	[ branches for the first dungeon ]
 *	  ...
 *	[ second dungeon struct ]
 *	  ...
 */

void
output_dgn()
{
	int	nd, cl = 0, nl = 0,
		    cb = 0, nb = 0;

	if(++n_dgns <= 0) {

	    yyerror("FATAL - no dungeons were defined.");
	    exit(1);
	}

	fwrite((char *)(&n_dgns), sizeof(int), 1, stdout);
	for(nd = 0; nd < n_dgns; nd++) {

	    fwrite((char *)&tmpdungeon[nd], sizeof(struct tmpdungeon), 1,
								stdout);

	    nl += tmpdungeon[nd].levels;
	    for(; cl < nl; cl++)
		fwrite((char *)&tmplevel[cl], sizeof(struct tmplevel), 1,
								stdout);

	    nb += tmpdungeon[nd].branches;
	    for(; cb < nb; cb++)
		fwrite((char *)&tmpbranch[cb], sizeof(struct tmpbranch), 1,
								stdout);
	}
}
