
/*  A Bison parser, made from lev_comp.y  */

#define	CHAR	258
#define	INTEGER	259
#define	BOOLEAN	260
#define	MESSAGE_ID	261
#define	MAZE_ID	262
#define	LEVEL_ID	263
#define	LEV_INIT_ID	264
#define	GEOMETRY_ID	265
#define	NOMAP_ID	266
#define	OBJECT_ID	267
#define	MONSTER_ID	268
#define	TRAP_ID	269
#define	DOOR_ID	270
#define	DRAWBRIDGE_ID	271
#define	MAZEWALK_ID	272
#define	WALLIFY_ID	273
#define	REGION_ID	274
#define	FILLING	275
#define	RANDOM_OBJECTS_ID	276
#define	RANDOM_MONSTERS_ID	277
#define	RANDOM_PLACES_ID	278
#define	ALTAR_ID	279
#define	LADDER_ID	280
#define	STAIR_ID	281
#define	NON_DIGGABLE_ID	282
#define	NON_PASSWALL_ID	283
#define	ROOM_ID	284
#define	PORTAL_ID	285
#define	TELEPRT_ID	286
#define	BRANCH_ID	287
#define	LEV	288
#define	CHANCE_ID	289
#define	CORRIDOR_ID	290
#define	GOLD_ID	291
#define	ENGRAVING_ID	292
#define	FOUNTAIN_ID	293
#define	POOL_ID	294
#define	SINK_ID	295
#define	NONE	296
#define	RAND_CORRIDOR_ID	297
#define	DOOR_STATE	298
#define	LIGHT_STATE	299
#define	CURSE_TYPE	300
#define	ENGRAVING_TYPE	301
#define	DIRECTION	302
#define	RANDOM_TYPE	303
#define	O_REGISTER	304
#define	M_REGISTER	305
#define	P_REGISTER	306
#define	A_REGISTER	307
#define	ALIGNMENT	308
#define	LEFT_OR_RIGHT	309
#define	CENTER	310
#define	TOP_OR_BOT	311
#define	ALTAR_TYPE	312
#define	UP_OR_DOWN	313
#define	SUBROOM_ID	314
#define	NAME_ID	315
#define	FLAGS_ID	316
#define	FLAG_TYPE	317
#define	MON_ATTITUDE	318
#define	MON_ALERTNESS	319
#define	MON_APPEARANCE	320
#define	STRING	321
#define	MAP_ID	322

#line 1 "lev_comp.y"

/*	SCCS Id: @(#)lev_comp.c	3.1	93/05/15	*/
/*	Copyright (c) 1989 by Jean-Christophe Collet */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file contains the Level Compiler code
 * It may handle special mazes & special room-levels
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

#include "hack.h"
#include "sp_lev.h"
#ifndef O_WRONLY
# include <fcntl.h>
#endif
#ifndef O_CREAT	/* some older BSD systems do not define O_CREAT in <fcntl.h> */
# include <sys/file.h>
#endif
#ifndef O_BINARY	/* used for micros, no-op for others */
# define O_BINARY 0
#endif

#ifdef MICRO
# define OMASK FCMASK
#else
# define OMASK 0644
#endif

#define MAX_REGISTERS	10
#define ERR		(-1)

#define New(type)		(type *) alloc(sizeof(type))
#define NewTab(type, size)	(type **) alloc(sizeof(type *) * size)

#ifdef MICRO
# undef exit
extern void FDECL(exit, (int));
#endif

extern void FDECL(yyerror, (const char *));
extern void FDECL(yywarning, (const char *));
extern int NDECL(yylex);
int NDECL(yyparse);

extern char *FDECL(dup_string,(char *));
extern int FDECL(get_floor_type, (CHAR_P));
extern int FDECL(get_room_type, (char *));
extern int FDECL(get_trap_type, (char *));
extern int FDECL(get_monster_id, (char *, CHAR_P));
extern int FDECL(get_object_id, (char *));
extern boolean FDECL(check_monster_char, (CHAR_P));
extern boolean FDECL(check_object_char, (CHAR_P));
extern char FDECL(what_map_char, (CHAR_P));
extern void FDECL(scan_map, (char *));
extern void NDECL(wallify_map);
extern boolean NDECL(check_subrooms);
extern void FDECL(check_coord, (int, int, const char *));
extern void NDECL(store_part);
extern void NDECL(store_room);
extern void FDECL(write_maze, (int, specialmaze *));
extern void FDECL(write_lev, (int, splev *));
extern void FDECL(free_rooms, (room **, int));

static struct reg {
	int x1, y1;
	int x2, y2;
}		current_region;

static struct coord {
	int x;
	int y;
}		current_coord, current_align;

static struct size {
	int height;
	int width;
}		current_size;

char tmpmessage[256];
altar *tmpaltar[256];
lad *tmplad[256];
stair *tmpstair[256];
digpos *tmpdig[256];
digpos *tmppass[32];
char *tmpmap[ROWNO];
region *tmpreg[256];
lev_region *tmplreg[32];
door *tmpdoor[256];
room_door *tmprdoor[256];
trap *tmptrap[256];
monster *tmpmonst[256];
object *tmpobj[256];
drawbridge *tmpdb[256];
walk *tmpwalk[256];
gold *tmpgold[256];
fountain *tmpfountain[256];
sink *tmpsink[256];
pool *tmppool[256];
engraving *tmpengraving[256];
mazepart *tmppart[10];
room *tmproom[MAXNROFROOMS*2];
corridor *tmpcor[256];

static specialmaze maze;
static splev special_lev;
static lev_init init_lev;

static char olist[MAX_REGISTERS], mlist[MAX_REGISTERS];
static struct coord plist[MAX_REGISTERS];

int n_olist = 0, n_mlist = 0, n_plist = 0;

unsigned int nlreg = 0, nreg = 0, ndoor = 0, ntrap = 0, nmons = 0, nobj = 0;
unsigned int ndb = 0, nwalk = 0, npart = 0, ndig = 0, nlad = 0, nstair = 0;
unsigned int naltar = 0, ncorridor = 0, nrooms = 0, ngold = 0, nengraving = 0;
unsigned int nfountain = 0, npool = 0, nsink = 0, npass = 0;

static unsigned long lev_flags = 0;

unsigned int max_x_map, max_y_map;

static xchar in_room;

extern int fatal_error;
extern int want_warnings;
extern const char *fname;


#line 144 "lev_comp.y"
typedef union
{
	int	i;
	char*	map;
	struct {
		xchar room;
		xchar wall;
		xchar door;
	} corpos;
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



#define	YYFINAL		461
#define	YYFLAG		-32768
#define	YYNTBASE	74

#define YYTRANSLATE(x) ((unsigned)(x) <= 322 ? yytranslate[x] : 188)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    68,
    69,     2,     2,    66,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    67,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    70,     2,    71,     2,     2,     2,     2,     2,     2,     2,
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
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
    72,    73
};

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   181,   182,   185,   186,   189,   190,   193,   228,   270,   282,
   287,   302,   303,   306,   310,   317,   321,   327,   328,   331,
   347,   348,   351,   362,   375,   392,   395,   396,   399,   400,
   403,   411,   412,   415,   426,   437,   447,   451,   457,   477,
   497,   501,   507,   517,   523,   532,   538,   543,   549,   554,
   560,   561,   564,   565,   566,   567,   568,   569,   570,   571,
   572,   573,   574,   575,   576,   579,   588,   601,   617,   618,
   621,   622,   625,   626,   629,   641,   645,   651,   652,   655,
   661,   677,   689,   695,   696,   699,   700,   703,   704,   707,
   718,   733,   746,   753,   762,   769,   778,   785,   792,   795,
   796,   799,   800,   801,   802,   803,   804,   805,   806,   807,
   808,   809,   810,   811,   812,   813,   814,   815,   816,   817,
   818,   821,   848,   853,   854,   857,   861,   865,   869,   873,
   880,   904,   909,   913,   922,   933,   934,   937,   938,   944,
   958,   971,  1009,  1019,  1025,  1038,  1051,  1060,  1076,  1085,
  1098,  1107,  1116,  1127,  1136,  1149,  1153,  1159,  1163,  1183,
  1195,  1204,  1213,  1224,  1235,  1297,  1311,  1324,  1338,  1339,
  1343,  1346,  1347,  1351,  1354,  1355,  1361,  1362,  1368,  1375,
  1378,  1387,  1390,  1394,  1398,  1404,  1405,  1406,  1412,  1413,
  1416,  1417,  1420,  1421,  1422,  1428,  1429,  1432,  1441,  1450,
  1459,  1468,  1471,  1482,  1494,  1497,  1498,  1504,  1505,  1508,
  1509,  1512,  1523
};

static const char * const yytname[] = {     0,
"error","$illegal.","CHAR","INTEGER","BOOLEAN","MESSAGE_ID","MAZE_ID","LEVEL_ID","LEV_INIT_ID","GEOMETRY_ID",
"NOMAP_ID","OBJECT_ID","MONSTER_ID","TRAP_ID","DOOR_ID","DRAWBRIDGE_ID","MAZEWALK_ID","WALLIFY_ID","REGION_ID","FILLING",
"RANDOM_OBJECTS_ID","RANDOM_MONSTERS_ID","RANDOM_PLACES_ID","ALTAR_ID","LADDER_ID","STAIR_ID","NON_DIGGABLE_ID","NON_PASSWALL_ID","ROOM_ID","PORTAL_ID",
"TELEPRT_ID","BRANCH_ID","LEV","CHANCE_ID","CORRIDOR_ID","GOLD_ID","ENGRAVING_ID","FOUNTAIN_ID","POOL_ID","SINK_ID",
"NONE","RAND_CORRIDOR_ID","DOOR_STATE","LIGHT_STATE","CURSE_TYPE","ENGRAVING_TYPE","DIRECTION","RANDOM_TYPE","O_REGISTER","M_REGISTER",
"P_REGISTER","A_REGISTER","ALIGNMENT","LEFT_OR_RIGHT","CENTER","TOP_OR_BOT","ALTAR_TYPE","UP_OR_DOWN","SUBROOM_ID","NAME_ID",
"FLAGS_ID","FLAG_TYPE","MON_ATTITUDE","MON_ALERTNESS","MON_APPEARANCE","','","':'","'('","')'","'['",
"']'","STRING","MAP_ID","file"
};
#endif

static const short yyr1[] = {     0,
    74,    74,    75,    75,    76,    76,    77,    78,    79,    80,
    80,    81,    81,    82,    82,    83,    83,    84,    84,    85,
    86,    86,    87,    87,    88,    88,    89,    89,    90,    90,
    91,    92,    92,    93,    93,    94,    95,    95,    96,    97,
    98,    98,    99,    99,   100,   100,   101,   101,   102,   102,
   103,   103,   104,   104,   104,   104,   104,   104,   104,   104,
   104,   104,   104,   104,   104,   105,   106,   107,   108,   108,
   109,   109,   110,   110,   111,   112,   112,   113,   113,   114,
   115,   115,   116,   117,   117,   118,   118,   119,   119,   120,
   120,   120,   121,   121,   122,   122,   123,   124,   123,   125,
   125,   126,   126,   126,   126,   126,   126,   126,   126,   126,
   126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
   126,   128,   127,   129,   129,   130,   130,   130,   130,   130,
   132,   131,   133,   133,   133,   134,   134,   135,   135,   136,
   137,   138,   139,   140,   141,   142,   144,   143,   146,   145,
   148,   149,   147,   151,   150,   152,   152,   153,   153,   154,
   155,   156,   157,   158,   159,   160,   161,   162,   163,   163,
   163,   164,   164,   164,   165,   165,   166,   166,   167,   167,
   168,   168,   169,   169,   169,   170,   170,   170,   171,   171,
   172,   172,   173,   173,   173,   174,   174,   175,   176,   177,
   178,   179,   180,   181,   182,   183,   183,   184,   184,   185,
   185,   186,   187
};

static const short yyr2[] = {     0,
     0,     1,     1,     2,     1,     1,     5,     7,     3,     0,
    13,     1,     1,     0,     3,     3,     1,     0,     2,     3,
     0,     2,     3,     3,     0,     1,     1,     2,     1,     1,
     1,     0,     2,     5,     5,     7,     2,     2,    12,    12,
     0,     2,     5,     1,     5,     1,     5,     1,     5,     1,
     0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     3,     3,     9,     1,     1,
     1,     1,     1,     1,     5,     1,     1,     1,     2,     3,
     1,     2,     5,     1,     1,     1,     1,     0,     2,     3,
     3,     3,     1,     3,     1,     3,     1,     0,     4,     0,
     2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     0,     9,     0,     2,     2,     2,     2,     2,     3,
     0,     9,     0,     4,     6,     1,     1,     1,     1,     5,
     5,     7,     5,     1,     5,     5,     0,     8,     0,     8,
     0,     0,     8,     0,     6,     0,     2,     1,    10,     3,
     3,     3,     3,     3,     8,     7,     5,     7,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     0,     2,     4,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     4,     4,     4,
     4,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     5,     9
};

static const short yydefact[] = {     1,
     0,     0,     2,     3,     5,     6,    14,    14,     0,     0,
     4,     0,    10,    10,   205,     0,     9,     0,     0,    18,
    18,     0,    17,    15,     0,     0,    21,    18,     0,    76,
    77,    75,     0,     0,     0,    25,    19,     0,    81,     7,
    78,    88,     0,    16,     0,    20,     0,     0,     0,     0,
    22,    32,    26,    27,    51,    51,     0,    79,   100,    82,
     0,     0,     0,     0,     0,    31,     8,    29,    30,    28,
    38,    37,    84,    85,     0,     0,     0,     0,    89,    80,
     0,   204,    23,    93,   203,    24,    95,   182,     0,   181,
     0,     0,    33,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    52,    53,    54,    55,
    56,    57,    58,    65,    60,    61,    62,    59,    63,    64,
     0,     0,     0,     0,     0,     0,     0,   144,     0,     0,
     0,     0,     0,     0,     0,     0,   101,   102,   103,   104,
   105,   106,   114,   115,   116,   117,   108,   109,   110,   111,
   113,   120,   121,   107,   112,   118,   119,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    87,    86,    83,    90,
    92,     0,    91,    97,   202,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    94,    96,   191,
   192,     0,     0,     0,     0,   173,     0,     0,   174,   172,
   170,     0,     0,   171,   169,   180,     0,   179,    69,    70,
     0,   188,     0,     0,   187,   186,     0,    67,   208,   209,
     0,     0,   160,   162,   161,    66,     0,     0,   189,   190,
     0,     0,     0,     0,     0,     0,     0,     0,   147,   158,
   163,   164,   149,   151,   154,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    44,     0,     0,    46,
     0,     0,     0,    35,    34,     0,   178,     0,   177,     0,
   176,     0,   175,   141,     0,     0,   195,     0,   193,     0,
   194,   146,   167,   210,   211,     0,     0,    99,   140,     0,
   143,     0,     0,   145,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   199,     0,   200,     0,
     0,   198,     0,     0,     0,   212,     0,     0,     0,     0,
     0,     0,     0,   152,   155,     0,     0,    48,     0,     0,
     0,    50,     0,     0,     0,   131,   122,    71,    72,     0,
     0,   197,   196,   166,   168,   142,     0,   183,     0,     0,
     0,   156,    12,    13,    11,     0,     0,     0,     0,     0,
     0,    73,    74,     0,   133,   124,     0,   201,     0,     0,
   165,     0,   148,   150,     0,   153,    43,     0,    41,    45,
     0,    41,    36,     0,   132,   123,    68,     0,   184,     0,
   157,     0,     0,    40,     0,    39,   137,   136,     0,     0,
     0,   125,     0,     0,     0,    47,    42,    49,     0,     0,
   127,   128,     0,   129,   126,   213,   185,     0,   138,   139,
   134,     0,   130,   159,     0,   207,   206,   135,     0,     0,
     0
};

static const short yydefgoto[] = {   459,
     3,     4,     5,     6,     7,    20,   385,    13,    24,    27,
    28,    36,    51,    52,    53,    67,    68,    69,    93,   205,
    54,    55,    56,   424,   289,   292,   360,   364,    71,   107,
   108,   109,   110,   221,   370,   394,     8,    32,    40,    41,
    42,    43,    75,   179,    59,    79,    83,    86,   183,   238,
    80,   137,   111,   396,   416,   432,   112,   395,   415,   430,
   451,   140,   113,   142,   143,   144,   145,   114,   147,   282,
   148,   283,   149,   284,   382,   150,   285,   406,   249,   115,
   116,   117,   152,   153,   154,   118,   119,   120,   213,   208,
   302,   298,   217,    89,   401,   227,   241,   202,   310,   374,
   225,   209,   214,   311,   184,    87,    84,    90,   458,   231,
   316,   226,   250
};

static const short yypact[] = {   133,
   -16,   -10,-32768,   133,-32768,-32768,     1,     1,    25,    25,
-32768,     4,    72,    72,-32768,    45,-32768,    60,    66,   150,
   150,    17,    94,-32768,   163,   105,-32768,   150,   152,-32768,
-32768,-32768,    60,   108,   106,    23,-32768,   109,-32768,-32768,
   152,-32768,   107,-32768,   174,-32768,   112,   114,   115,   116,
-32768,   142,-32768,     9,-32768,-32768,   110,-32768,   128,-32768,
   119,   183,   184,   -13,   -13,-32768,-32768,-32768,   153,-32768,
     3,     3,-32768,-32768,   124,   122,   125,   126,-32768,   100,
   186,-32768,-32768,   129,-32768,-32768,   130,-32768,   131,-32768,
   132,   127,-32768,   134,   135,   136,   137,   138,   139,   140,
   141,   143,   144,   145,   146,   147,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   113,   183,   184,   148,   151,   154,   155,-32768,   156,   157,
   158,   159,   160,   161,   162,   164,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   149,   183,   184,
   104,   104,   165,    21,    16,    12,    28,    91,    91,   195,
    24,    91,    91,    91,    91,    25,-32768,-32768,-32768,-32768,
-32768,   196,-32768,   166,-32768,     6,    91,    91,   167,    91,
    -1,   167,   167,    -7,    -7,    -7,   204,-32768,-32768,-32768,
-32768,   168,   170,   213,   171,-32768,   169,   172,-32768,-32768,
-32768,   173,   175,-32768,-32768,-32768,   176,-32768,-32768,-32768,
   178,-32768,   177,   179,-32768,-32768,   180,-32768,-32768,-32768,
   182,   185,-32768,-32768,-32768,-32768,   188,   190,-32768,-32768,
   191,   192,   193,   215,   194,   197,   181,   216,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,   198,    40,    75,   199,    10,
   226,    35,   236,    48,    91,     6,   246,    41,   203,    91,
    58,   248,   148,    91,   219,   220,   202,   104,   211,   258,
   205,   206,   207,   208,   209,   104,-32768,   266,   210,-32768,
   273,   212,   232,-32768,-32768,   214,-32768,   217,-32768,   218,
-32768,   221,-32768,-32768,   222,   223,-32768,   225,-32768,   224,
-32768,-32768,-32768,-32768,-32768,   227,   228,-32768,-32768,   230,
-32768,   276,   233,-32768,   234,   277,    -7,    -7,    -7,    -7,
   235,   237,    86,   238,    87,   239,-32768,    91,-32768,    91,
   123,-32768,   278,    -2,    25,-32768,     6,   240,   -13,   280,
    92,   241,   242,-32768,-32768,    29,   287,-32768,   110,   243,
   288,-32768,   294,   244,    26,-32768,-32768,-32768,-32768,   245,
   231,-32768,-32768,-32768,-32768,-32768,   308,   247,   249,   256,
    25,   250,-32768,-32768,-32768,   251,   252,    87,   253,   255,
    25,-32768,-32768,   254,   259,-32768,    26,-32768,   260,   297,
-32768,   315,-32768,-32768,   269,-32768,-32768,   113,   262,-32768,
   320,   262,-32768,     8,-32768,   263,-32768,   326,   265,   267,
-32768,   268,   327,-32768,   270,-32768,-32768,-32768,   272,   274,
    38,-32768,   275,   329,   331,-32768,-32768,-32768,    27,    27,
-32768,-32768,    25,-32768,-32768,-32768,-32768,   279,-32768,-32768,
-32768,   281,-32768,-32768,     7,-32768,-32768,-32768,   341,   342,
-32768
};

static const short yypgoto[] = {-32768,
   339,-32768,-32768,-32768,-32768,   332,-32768,   337,   316,    64,
-32768,-32768,-32768,-32768,   296,-32768,-32768,-32768,-32768,    93,
-32768,-32768,-32768,   -61,-32768,-32768,-32768,   -36,   298,-32768,
-32768,-32768,-32768,-32768,-32768,   -42,-32768,-32768,   317,-32768,
-32768,-32768,    -3,   -51,-32768,-32768,  -101,   -87,    88,-32768,
-32768,-32768,   282,-32768,-32768,-32768,   283,-32768,-32768,-32768,
   -81,-32768,   284,-32768,-32768,-32768,-32768,   285,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,  -183,   286,
-32768,-32768,-32768,-32768,-32768,   289,   290,   291,-32768,-32768,
-32768,-32768,-32768,   -63,-32768,  -165,  -260,  -157,   -71,-32768,
-32768,-32768,-32768,-32768,-32768,   229,   257,    -9,-32768,-32768,
-32768,  -120,   -94
};


#define	YYLAST		434


static const short yytable[] = {    16,
    17,    91,   224,   185,   203,   305,   232,   233,   234,   235,
   253,   254,   255,   294,    94,    95,    96,    97,    85,    30,
   180,   242,   243,    82,   246,   247,    98,   229,    99,   392,
   449,   247,   219,   383,    88,   181,   100,    49,   101,   102,
   103,   104,   105,    47,    48,   372,   222,   456,   239,   223,
     9,    49,   427,   240,   373,   428,    10,   198,    15,   216,
   244,    12,   106,   211,    31,   212,   248,    50,   206,   207,
    18,   230,   199,   393,   450,   220,   384,   204,   457,   429,
    19,    50,   297,    15,    29,   307,   376,   287,   307,   308,
   309,    37,   308,   309,   245,   301,    15,   251,   252,   304,
   441,   442,   443,   314,   313,   315,    15,   288,   319,    15,
    22,    94,    95,    96,   125,   126,   127,   128,   129,    15,
   323,    23,   290,    98,   130,   131,   132,   133,   331,   134,
   135,   136,    25,   358,   362,   101,   102,   103,   222,     1,
     2,   223,   291,   352,   353,   354,   355,   200,    76,    77,
    78,   201,   185,   359,   363,    26,   218,   377,   182,    33,
   346,    38,    39,    73,    74,    34,   236,   177,   178,   368,
   369,    35,   366,    45,   367,    57,    61,    46,    62,    60,
    63,    64,    65,    66,    81,    82,    85,    92,   122,   121,
   158,   123,   124,   163,   159,   160,   161,   162,   228,   237,
   164,   165,   166,   167,   168,   169,   170,   171,   256,   172,
   173,   174,   175,   176,   197,   182,   259,   186,   277,   281,
   187,   188,   189,   190,   191,   192,   193,   194,   195,   296,
   196,   -98,   204,   257,   244,   258,   260,   262,   261,   300,
   264,   265,   263,   266,   268,   269,   267,   270,   280,   306,
   271,   317,   299,   272,   303,   273,   274,   275,   276,   278,
   312,   325,   279,   286,   293,   320,   321,   322,   324,   332,
   326,   327,   328,   329,   330,   333,   334,   335,   336,   348,
   351,   371,   338,   379,   337,   378,   340,   341,   339,   344,
   386,   389,   345,   342,   343,   347,   346,   390,   349,   350,
   356,   398,   357,   361,   365,   377,   380,   381,   388,   391,
   397,   399,   400,   403,   402,   405,   419,   408,   420,   407,
   411,   410,   413,   425,   414,   418,   421,   423,   431,   433,
   434,   437,   435,   447,   448,   375,   436,   439,   438,   440,
   460,   461,    11,   446,    14,    21,   455,   454,    44,    70,
   426,   409,   295,    72,   417,   387,   422,    58,   452,   444,
   318,   138,   139,   141,   146,   151,     0,     0,   155,   156,
   157,   404,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   412,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   215,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   210,   445,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   453
};

static const short yycheck[] = {     9,
    10,    65,   168,   124,   162,   266,   172,   173,   174,   175,
   194,   195,   196,     4,    12,    13,    14,    15,     3,     3,
   122,   187,   188,     3,   190,    33,    24,     4,    26,     4,
     4,    33,     5,     5,    48,   123,    34,    29,    36,    37,
    38,    39,    40,    21,    22,    48,    48,    41,    43,    51,
    67,    29,    45,    48,    57,    48,    67,   159,    72,    48,
    68,    61,    60,    48,    48,    50,    68,    59,    48,    49,
    67,    48,   160,    48,    48,    48,    48,    68,    72,    72,
     9,    59,    48,    72,    21,    48,   347,    48,    48,    52,
    53,    28,    52,    53,   189,    48,    72,   192,   193,   265,
    63,    64,    65,    46,   270,    48,    72,    68,   274,    72,
    66,    12,    13,    14,    15,    16,    17,    18,    19,    72,
   278,    62,    48,    24,    25,    26,    27,    28,   286,    30,
    31,    32,    67,    48,    48,    36,    37,    38,    48,     7,
     8,    51,    68,   327,   328,   329,   330,    44,    21,    22,
    23,    48,   273,    68,    68,     6,   166,    66,    68,    66,
    69,    10,    11,    54,    55,     3,   176,    55,    56,    47,
    48,    67,   338,    66,   340,    67,     3,    72,    67,    73,
    67,    67,    67,    42,    66,     3,     3,    35,    67,    66,
     5,    67,    67,    67,    66,    66,    66,    66,     4,     4,
    67,    67,    67,    67,    67,    67,    67,    67,     5,    67,
    67,    67,    67,    67,    66,    68,     4,    67,     4,     4,
    67,    67,    67,    67,    67,    67,    67,    67,    67,     4,
    67,    66,    68,    66,    68,    66,    66,    66,    70,     4,
    66,    66,    70,    66,    66,    66,    70,    66,    68,     4,
    66,     4,   262,    66,   264,    66,    66,    66,    66,    66,
    58,     4,    66,    66,    66,    47,    47,    66,    58,     4,
    66,    66,    66,    66,    66,    66,     4,    66,    47,     4,
     4,     4,    66,     4,    71,   349,    66,    66,    71,    66,
     4,     4,    66,    71,    70,    66,    69,     4,    66,    66,
    66,    71,    66,    66,    66,    66,    66,    66,    66,    66,
    66,     4,    66,    58,    66,    66,    20,    66,     4,    69,
    66,    69,    69,     4,    66,    66,    58,    66,    66,     4,
    66,     5,    66,     5,     4,   345,    69,    66,    69,    66,
     0,     0,     4,    69,     8,    14,    66,    69,    33,    54,
   412,   388,   260,    56,   397,   359,   408,    41,   440,   431,
   273,    80,    80,    80,    80,    80,    -1,    -1,    80,    80,
    80,   381,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,   391,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,   165,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
   164,   431,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,   443
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

case 7:
#line 194 "lev_comp.y"
{
			  int fout, i;

			if (fatal_error > 0) {
				fprintf(stderr,
			      "%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
				char lbuf[20];
				Strcpy(lbuf, yyvsp[-4].map);
				Strcat(lbuf, LEV_EXT);
#ifdef THINK_C
				fout = open(lbuf, O_WRONLY|O_CREAT|O_BINARY);
#else
				fout = open(lbuf, O_WRONLY|O_CREAT|O_BINARY, OMASK);
#endif
				if (fout < 0) {
					yyerror("Can't open output file!!");
					exit(1);
				}
				maze.flags = yyvsp[-3].i;
				memcpy(&(maze.init_lev), &(init_lev),
				       sizeof(lev_init));
				maze.numpart = npart;
				maze.parts = NewTab(mazepart, npart);
				for(i=0;i<npart;i++)
				    maze.parts[i] = tmppart[i];
				write_maze(fout, &maze);
				(void) close(fout);
				npart = 0;
			}
		  ;
    break;}
case 8:
#line 229 "lev_comp.y"
{
			int fout, i;

			if (fatal_error > 0) {
			    fprintf(stderr,
			      "%s : %d errors detected. No output created!\n",
					fname, fatal_error);
			} else {
				char lbuf[20];
				Strcpy(lbuf, yyvsp[-6].map);
				Strcat(lbuf, LEV_EXT);
#ifdef THINK_C
				fout = open(lbuf, O_WRONLY|O_CREAT|O_BINARY);
#else
				fout = open(lbuf, O_WRONLY|O_CREAT|O_BINARY, OMASK);
#endif
				if (fout < 0) {
					yyerror("Can't open output file!!");
					exit(1);
				}
				special_lev.flags = yyvsp[-5].i;
				memcpy(&(special_lev.init_lev), &(init_lev),
				       sizeof(lev_init));
				special_lev.nroom = nrooms;
				special_lev.rooms = NewTab(room, nrooms);
				for(i=0; i<nrooms; i++)
				    special_lev.rooms[i] = tmproom[i];
				special_lev.ncorr = ncorridor;
				special_lev.corrs = NewTab(corridor, ncorridor);
				for(i=0; i<ncorridor; i++)
				    special_lev.corrs[i] = tmpcor[i];
				if (check_subrooms())
				    write_lev(fout, &special_lev);
				free_rooms(special_lev.rooms,special_lev.nroom);
				nrooms = 0;
				ncorridor = 0;
				(void) close(fout);
			}
		  ;
    break;}
case 9:
#line 271 "lev_comp.y"
{
			if (index(yyvsp[0].map, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if (strlen(yyvsp[0].map) > 8)
			    yyerror("Level names limited to 8 characters.");
			yyval.map = yyvsp[0].map;
			special_lev.nrobjects = 0;
			special_lev.nrmonst = 0;
		  ;
    break;}
case 10:
#line 283 "lev_comp.y"
{
			init_lev.init_present = FALSE;
			yyval.i = 0;
		  ;
    break;}
case 11:
#line 288 "lev_comp.y"
{
			init_lev.init_present = TRUE;
			if((init_lev.fg = what_map_char(yyvsp[-10].i)) == INVALID_TYPE)
			    yyerror("Invalid foreground type.");
			if((init_lev.bg = what_map_char(yyvsp[-8].i)) == INVALID_TYPE)
			    yyerror("Invalid background type.");
			init_lev.smoothed = yyvsp[-6].i;
			init_lev.joined = yyvsp[-4].i;
			init_lev.lit = yyvsp[-2].i;
			init_lev.walled = yyvsp[0].i;
			yyval.i = 1;
		  ;
    break;}
case 14:
#line 307 "lev_comp.y"
{
			yyval.i = 0;
		  ;
    break;}
case 15:
#line 311 "lev_comp.y"
{
			yyval.i = lev_flags;
			lev_flags = 0;	/* clear for next user */
		  ;
    break;}
case 16:
#line 318 "lev_comp.y"
{
			lev_flags |= yyvsp[-2].i;
		  ;
    break;}
case 17:
#line 322 "lev_comp.y"
{
			lev_flags |= yyvsp[0].i;
		  ;
    break;}
case 20:
#line 332 "lev_comp.y"
{
			int i, j;

			i = strlen(yyvsp[0].map) + 1;
			j = tmpmessage[0] ? strlen(tmpmessage) : 0;
			if(i+j > 255) {
			   yyerror("Message string too long (>256 characters)");
			} else {
			    if(j) tmpmessage[j++] = '\n';
			    strncpy(tmpmessage+j, yyvsp[0].map, i-1);
			    tmpmessage[j+i-1] = 0;
			}
		  ;
    break;}
case 23:
#line 352 "lev_comp.y"
{
			if(special_lev.nrobjects) {
			    yyerror("Object registers already initialized!");
			} else {
			    special_lev.nrobjects = n_olist;
			    special_lev.robjects = (char *) alloc(n_olist);
			    (void) memcpy((genericptr_t)special_lev.robjects,
					  (genericptr_t)olist, n_olist);
			}
		  ;
    break;}
case 24:
#line 363 "lev_comp.y"
{
			if(special_lev.nrmonst) {
			    yyerror("Monster registers already initialized!");
			} else {
			    special_lev.nrmonst = n_mlist;
			    special_lev.rmonst = (char *) alloc(n_mlist);
			    (void) memcpy((genericptr_t)special_lev.rmonst,
					  (genericptr_t)mlist, n_mlist);
			  }
		  ;
    break;}
case 25:
#line 376 "lev_comp.y"
{
			tmproom[nrooms] = New(room);
			(void) memset((genericptr_t) tmproom[nrooms], 0,
					sizeof *tmproom[nrooms]);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->parent = (char *) 0;
			tmproom[nrooms]->rtype = 0;
			tmproom[nrooms]->rlit = 0;
			tmproom[nrooms]->xalign = ERR;
			tmproom[nrooms]->yalign = ERR;
			tmproom[nrooms]->x = 0;
			tmproom[nrooms]->y = 0;
			tmproom[nrooms]->w = 2;
			tmproom[nrooms]->h = 2;
			in_room = 1;
		  ;
    break;}
case 31:
#line 404 "lev_comp.y"
{
			tmpcor[0] = New(corridor);
			tmpcor[0]->src.room = -1;
			ncorridor = 1;
		  ;
    break;}
case 34:
#line 416 "lev_comp.y"
{
			tmpcor[ncorridor] = New(corridor);
			tmpcor[ncorridor]->src.room = yyvsp[-2].corpos.room;
			tmpcor[ncorridor]->src.wall = yyvsp[-2].corpos.wall;
			tmpcor[ncorridor]->src.door = yyvsp[-2].corpos.door;
			tmpcor[ncorridor]->dest.room = yyvsp[0].corpos.room;
			tmpcor[ncorridor]->dest.wall = yyvsp[0].corpos.wall;
			tmpcor[ncorridor]->dest.door = yyvsp[0].corpos.door;
			ncorridor++;
		  ;
    break;}
case 35:
#line 427 "lev_comp.y"
{
			tmpcor[ncorridor]->src.room = yyvsp[-2].corpos.room;
			tmpcor[ncorridor]->src.wall = yyvsp[-2].corpos.wall;
			tmpcor[ncorridor]->src.door = yyvsp[-2].corpos.door;
			tmpcor[ncorridor]->dest.room = -1;
			tmpcor[ncorridor]->dest.wall = yyvsp[0].i;
			ncorridor++;
		  ;
    break;}
case 36:
#line 438 "lev_comp.y"
{
			if (yyvsp[-5].i >= nrooms)
			    yyerror("Wrong room number!");
			yyval.corpos.room = yyvsp[-5].i;
			yyval.corpos.wall = yyvsp[-3].i;
			yyval.corpos.door = yyvsp[-1].i;
		  ;
    break;}
case 37:
#line 448 "lev_comp.y"
{
			store_room();
		  ;
    break;}
case 38:
#line 452 "lev_comp.y"
{
			store_room();
		  ;
    break;}
case 39:
#line 458 "lev_comp.y"
{
			tmproom[nrooms] = New(room);
			(void) memset((genericptr_t) tmproom[nrooms], 0,
					sizeof *tmproom[nrooms]);
			tmproom[nrooms]->parent = dup_string(yyvsp[-1].map);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->rtype = yyvsp[-9].i;
			tmproom[nrooms]->rlit = yyvsp[-7].i;
			tmproom[nrooms]->filled = yyvsp[0].i;
			tmproom[nrooms]->xalign = ERR;
			tmproom[nrooms]->yalign = ERR;
			tmproom[nrooms]->x = current_coord.x;
			tmproom[nrooms]->y = current_coord.y;
			tmproom[nrooms]->w = current_size.width;
			tmproom[nrooms]->h = current_size.height;
			in_room = 1;
		  ;
    break;}
case 40:
#line 478 "lev_comp.y"
{
			tmproom[nrooms] = New(room);
			(void) memset((genericptr_t) tmproom[nrooms], 0,
					sizeof *tmproom[nrooms]);
			tmproom[nrooms]->name = (char *) 0;
			tmproom[nrooms]->parent = (char *) 0;
			tmproom[nrooms]->rtype = yyvsp[-9].i;
			tmproom[nrooms]->rlit = yyvsp[-7].i;
			tmproom[nrooms]->filled = yyvsp[0].i;
			tmproom[nrooms]->xalign = current_align.x;
			tmproom[nrooms]->yalign = current_align.y;
			tmproom[nrooms]->x = current_coord.x;
			tmproom[nrooms]->y = current_coord.y;
			tmproom[nrooms]->w = current_size.width;
			tmproom[nrooms]->h = current_size.height;
			in_room = 1;
		  ;
    break;}
case 41:
#line 498 "lev_comp.y"
{
			yyval.i = 1;
		  ;
    break;}
case 42:
#line 502 "lev_comp.y"
{
			yyval.i = yyvsp[0].i;
		  ;
    break;}
case 43:
#line 508 "lev_comp.y"
{
			if ( yyvsp[-3].i < 1 || yyvsp[-3].i > 5 ||
			    yyvsp[-1].i < 1 || yyvsp[-1].i > 5 ) {
			    yyerror("Room position should be between 1 & 5!");
			} else {
			    current_coord.x = yyvsp[-3].i;
			    current_coord.y = yyvsp[-1].i;
			}
		  ;
    break;}
case 44:
#line 518 "lev_comp.y"
{
			current_coord.x = current_coord.y = ERR;
		  ;
    break;}
case 45:
#line 524 "lev_comp.y"
{
			if ( yyvsp[-3].i < 0 || yyvsp[-1].i < 0) {
			    yyerror("Invalid subroom position !");
			} else {
			    current_coord.x = yyvsp[-3].i;
			    current_coord.y = yyvsp[-1].i;
			}
		  ;
    break;}
case 46:
#line 533 "lev_comp.y"
{
			current_coord.x = current_coord.y = ERR;
		  ;
    break;}
case 47:
#line 539 "lev_comp.y"
{
			current_align.x = yyvsp[-3].i;
			current_align.y = yyvsp[-1].i;
		  ;
    break;}
case 48:
#line 544 "lev_comp.y"
{
			current_align.x = current_align.y = ERR;
		  ;
    break;}
case 49:
#line 550 "lev_comp.y"
{
			current_size.width = yyvsp[-3].i;
			current_size.height = yyvsp[-1].i;
		  ;
    break;}
case 50:
#line 555 "lev_comp.y"
{
			current_size.height = current_size.width = ERR;
		  ;
    break;}
case 66:
#line 580 "lev_comp.y"
{
			if (tmproom[nrooms]->name)
			    yyerror("This room already has a name!");
			else
			    tmproom[nrooms]->name = dup_string(yyvsp[0].map);
		  ;
    break;}
case 67:
#line 589 "lev_comp.y"
{
			if (tmproom[nrooms]->chance)
			    yyerror("This room already assigned a chance!");
			else if (tmproom[nrooms]->rtype == OROOM)
			    yyerror("Only typed rooms can have a chance!");
			else if (yyvsp[0].i < 1 || yyvsp[0].i > 99)
			    yyerror("The chance is supposed to be precentile.");
			else
			    tmproom[nrooms]->chance = yyvsp[0].i;
		   ;
    break;}
case 68:
#line 602 "lev_comp.y"
{
			/* ERR means random here */
			if (yyvsp[-2].i == ERR && yyvsp[0].i != ERR) {
		     yyerror("If the door wall is random, so must be its pos!");
			} else {
			    tmprdoor[ndoor] = New(room_door);
			    tmprdoor[ndoor]->secret = yyvsp[-6].i;
			    tmprdoor[ndoor]->mask = yyvsp[-4].i;
			    tmprdoor[ndoor]->wall = yyvsp[-2].i;
			    tmprdoor[ndoor]->pos = yyvsp[0].i;
			    ndoor++;
			}
		  ;
    break;}
case 75:
#line 630 "lev_comp.y"
{
			maze.filling = yyvsp[0].i;
			if (index(yyvsp[-2].map, '.'))
			    yyerror("Invalid dot ('.') in level name.");
			if (strlen(yyvsp[-2].map) > 8)
			    yyerror("Level names limited to 8 characters.");
			yyval.map = yyvsp[-2].map;
			in_room = 0;
		  ;
    break;}
case 76:
#line 642 "lev_comp.y"
{
			yyval.i = get_floor_type((char)yyvsp[0].i);
		  ;
    break;}
case 77:
#line 646 "lev_comp.y"
{
			yyval.i = -1;
		  ;
    break;}
case 80:
#line 656 "lev_comp.y"
{
			store_part();
		  ;
    break;}
case 81:
#line 662 "lev_comp.y"
{
			tmppart[npart] = New(mazepart);
			tmppart[npart]->halign = 1;
			tmppart[npart]->valign = 1;
			tmppart[npart]->nrobjects = 0;
			tmppart[npart]->nloc = 0;
			tmppart[npart]->nrmonst = 0;
			tmppart[npart]->xsize = 1;
			tmppart[npart]->ysize = 1;
			tmppart[npart]->map = (char **) alloc(sizeof(char *));
			tmppart[npart]->map[0] = (char *) alloc(1);
			tmppart[npart]->map[0][0] = STONE;
			max_x_map = COLNO-1;
			max_y_map = ROWNO;
		  ;
    break;}
case 82:
#line 678 "lev_comp.y"
{
			tmppart[npart] = New(mazepart);
			tmppart[npart]->halign = yyvsp[-1].i % 10;
			tmppart[npart]->valign = yyvsp[-1].i / 10;
			tmppart[npart]->nrobjects = 0;
			tmppart[npart]->nloc = 0;
			tmppart[npart]->nrmonst = 0;
			scan_map(yyvsp[0].map);
		  ;
    break;}
case 83:
#line 690 "lev_comp.y"
{
			yyval.i = yyvsp[-2].i + (yyvsp[0].i * 10);
		  ;
    break;}
case 90:
#line 708 "lev_comp.y"
{
			if (tmppart[npart]->nrobjects) {
			    yyerror("Object registers already initialized!");
			} else {
			    tmppart[npart]->robjects = (char *)alloc(n_olist);
			    (void) memcpy((genericptr_t)tmppart[npart]->robjects,
					  (genericptr_t)olist, n_olist);
			    tmppart[npart]->nrobjects = n_olist;
			}
		  ;
    break;}
case 91:
#line 719 "lev_comp.y"
{
			if (tmppart[npart]->nloc) {
			    yyerror("Location registers already initialized!");
			} else {
			    register int i;
			    tmppart[npart]->rloc_x = (char *) alloc(n_plist);
			    tmppart[npart]->rloc_y = (char *) alloc(n_plist);
			    for(i=0;i<n_plist;i++) {
				tmppart[npart]->rloc_x[i] = plist[i].x;
				tmppart[npart]->rloc_y[i] = plist[i].y;
			    }
			    tmppart[npart]->nloc = n_plist;
			}
		  ;
    break;}
case 92:
#line 734 "lev_comp.y"
{
			if (tmppart[npart]->nrmonst) {
			    yyerror("Monster registers already initialized!");
			} else {
			    tmppart[npart]->rmonst = (char *) alloc(n_mlist);
			    (void) memcpy((genericptr_t)tmppart[npart]->rmonst,
					  (genericptr_t)mlist, n_mlist);
			    tmppart[npart]->nrmonst = n_mlist;
			}
		  ;
    break;}
case 93:
#line 747 "lev_comp.y"
{
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = yyvsp[0].i;
			else
			    yyerror("Object list too long!");
		  ;
    break;}
case 94:
#line 754 "lev_comp.y"
{
			if (n_olist < MAX_REGISTERS)
			    olist[n_olist++] = yyvsp[-2].i;
			else
			    yyerror("Object list too long!");
		  ;
    break;}
case 95:
#line 763 "lev_comp.y"
{
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = yyvsp[0].i;
			else
			    yyerror("Monster list too long!");
		  ;
    break;}
case 96:
#line 770 "lev_comp.y"
{
			if (n_mlist < MAX_REGISTERS)
			    mlist[n_mlist++] = yyvsp[-2].i;
			else
			    yyerror("Monster list too long!");
		  ;
    break;}
case 97:
#line 779 "lev_comp.y"
{
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  ;
    break;}
case 98:
#line 786 "lev_comp.y"
{
			if (n_plist < MAX_REGISTERS)
			    plist[n_plist++] = current_coord;
			else
			    yyerror("Location list too long!");
		  ;
    break;}
case 122:
#line 822 "lev_comp.y"
{
			tmpmonst[nmons] = New(monster);
			tmpmonst[nmons]->x = current_coord.x;
			tmpmonst[nmons]->y = current_coord.y;
			tmpmonst[nmons]->class = yyvsp[-4].i;
			tmpmonst[nmons]->peaceful = -1; /* no override */
			tmpmonst[nmons]->asleep = -1;
			tmpmonst[nmons]->align = - MAX_REGISTERS - 2;
			tmpmonst[nmons]->name = (char *) 0;
			tmpmonst[nmons]->appear = 0;
			tmpmonst[nmons]->appear_as = (char *) 0;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Monster");
			if (!yyvsp[-2].map)
			    tmpmonst[nmons]->id = -1;
			else {
				int token = get_monster_id(yyvsp[-2].map, (char) yyvsp[-4].i);
				if (token == ERR) {
				    yywarning("Illegal monster name!  Making random monster.");
				    tmpmonst[nmons]->id = -1;
				} else
				    tmpmonst[nmons]->id = token;
			}
		  ;
    break;}
case 123:
#line 848 "lev_comp.y"
{
			nmons++;
		  ;
    break;}
case 126:
#line 858 "lev_comp.y"
{
			tmpmonst[nmons]->name = dup_string(yyvsp[0].map);
		  ;
    break;}
case 127:
#line 862 "lev_comp.y"
{
			tmpmonst[nmons]->peaceful = yyvsp[0].i;
		  ;
    break;}
case 128:
#line 866 "lev_comp.y"
{
			tmpmonst[nmons]->asleep = yyvsp[0].i;
		  ;
    break;}
case 129:
#line 870 "lev_comp.y"
{
			tmpmonst[nmons]->align = yyvsp[0].i;
		  ;
    break;}
case 130:
#line 874 "lev_comp.y"
{
			tmpmonst[nmons]->appear = yyvsp[-1].i;
			tmpmonst[nmons]->appear_as = dup_string(yyvsp[0].map);
		  ;
    break;}
case 131:
#line 881 "lev_comp.y"
{
			tmpobj[nobj] = New(object);
			tmpobj[nobj]->x = current_coord.x;
			tmpobj[nobj]->y = current_coord.y;
			tmpobj[nobj]->class = yyvsp[-4].i;
			tmpobj[nobj]->corpsenm = -1;	/* init as none */
			tmpobj[nobj]->curse_state = -1;
			tmpobj[nobj]->name = (char *) 0;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Object");
			if (!yyvsp[-2].map)
			    tmpobj[nobj]->id = -1;
			else {
				int token = get_object_id(yyvsp[-2].map);
				if (token == ERR) {
				    yywarning("Illegal object name!  Making random object.");
				    tmpobj[nobj]->id = -1;
				} else
				    tmpobj[nobj]->id = token;
			}
		  ;
    break;}
case 132:
#line 904 "lev_comp.y"
{
			nobj++;
		  ;
    break;}
case 133:
#line 910 "lev_comp.y"
{
			tmpobj[nobj]->spe = -127;
		  ;
    break;}
case 134:
#line 914 "lev_comp.y"
{
			int token = get_monster_id(yyvsp[-2].map, (char)0);
			if (token == ERR)	/* "random" */
			    tmpobj[nobj]->corpsenm = -2;
			else
			    tmpobj[nobj]->corpsenm = token;
			tmpobj[nobj]->spe = yyvsp[0].i;
		  ;
    break;}
case 135:
#line 923 "lev_comp.y"
{
			tmpobj[nobj]->curse_state = yyvsp[-4].i;
			tmpobj[nobj]->spe = yyvsp[-2].i;
			if (yyvsp[0].map)
			    tmpobj[nobj]->name = dup_string(yyvsp[0].map);
			else
			    tmpobj[nobj]->name = (char *) 0;
		  ;
    break;}
case 139:
#line 939 "lev_comp.y"
{
			yyval.i = -127;
		  ;
    break;}
case 140:
#line 945 "lev_comp.y"
{
			tmpdoor[ndoor] = New(door);
			tmpdoor[ndoor]->x = current_coord.x;
			tmpdoor[ndoor]->y = current_coord.y;
			tmpdoor[ndoor]->mask = yyvsp[-2].i;
			if(current_coord.x >= 0 && current_coord.y >= 0 &&
			   tmpmap[current_coord.y][current_coord.x] != DOOR &&
			   tmpmap[current_coord.y][current_coord.x] != SDOOR)
			    yyerror("Door decl doesn't match the map");
			ndoor++;
		  ;
    break;}
case 141:
#line 959 "lev_comp.y"
{
			tmptrap[ntrap] = New(trap);
			tmptrap[ntrap]->x = current_coord.x;
			tmptrap[ntrap]->y = current_coord.y;
			tmptrap[ntrap]->type = yyvsp[-2].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Trap");
			ntrap++;
		  ;
    break;}
case 142:
#line 972 "lev_comp.y"
{
		        int x, y, dir;

			tmpdb[ndb] = New(drawbridge);
			x = tmpdb[ndb]->x = current_coord.x;
			y = tmpdb[ndb]->y = current_coord.y;
			/* convert dir from a DIRECTION to a DB_DIR */
			dir = yyvsp[-2].i;
			switch(dir) {
			case W_NORTH: dir = DB_NORTH; y--; break;
			case W_SOUTH: dir = DB_SOUTH; y++; break;
			case W_EAST:  dir = DB_EAST;  x++; break;
			case W_WEST:  dir = DB_WEST;  x--; break;
			default:
			    yyerror("Invalid drawbridge direction");
			    break;
			}
			tmpdb[ndb]->dir = dir;
			if (current_coord.x >= 0 && current_coord.y >= 0 &&
			    !IS_WALL(tmpmap[y][x])) {
			    char ebuf[60];
			    Sprintf(ebuf,
				    "Wall needed for drawbridge (%02d, %02d)",
				    current_coord.x, current_coord.y);
			    yyerror(ebuf);
			}

			if ( yyvsp[0].i == D_ISOPEN )
			    tmpdb[ndb]->db_open = 1;
			else if ( yyvsp[0].i == D_CLOSED )
			    tmpdb[ndb]->db_open = 0;
			else
			    yyerror("A drawbridge can only be open or closed!");
			ndb++;
		   ;
    break;}
case 143:
#line 1010 "lev_comp.y"
{
			tmpwalk[nwalk] = New(walk);
			tmpwalk[nwalk]->x = current_coord.x;
			tmpwalk[nwalk]->y = current_coord.y;
			tmpwalk[nwalk]->dir = yyvsp[0].i;
			nwalk++;
		  ;
    break;}
case 144:
#line 1020 "lev_comp.y"
{
			wallify_map();
		  ;
    break;}
case 145:
#line 1026 "lev_comp.y"
{
			tmplad[nlad] = New(lad);
			tmplad[nlad]->x = current_coord.x;
			tmplad[nlad]->y = current_coord.y;
			tmplad[nlad]->up = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Ladder");
			nlad++;
		  ;
    break;}
case 146:
#line 1039 "lev_comp.y"
{
			tmpstair[nstair] = New(stair);
			tmpstair[nstair]->x = current_coord.x;
			tmpstair[nstair]->y = current_coord.y;
			tmpstair[nstair]->up = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Stairway");
			nstair++;
		  ;
    break;}
case 147:
#line 1052 "lev_comp.y"
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  ;
    break;}
case 148:
#line 1061 "lev_comp.y"
{
			tmplreg[nlreg]->del_islev = yyvsp[-2].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			if(yyvsp[0].i)
			    tmplreg[nlreg]->rtype = LR_UPSTAIR;
			else
			    tmplreg[nlreg]->rtype = LR_DOWNSTAIR;
			tmplreg[nlreg]->rname = 0;
			nlreg++;
		  ;
    break;}
case 149:
#line 1077 "lev_comp.y"
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  ;
    break;}
case 150:
#line 1086 "lev_comp.y"
{
			tmplreg[nlreg]->del_islev = yyvsp[-2].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			tmplreg[nlreg]->rtype = LR_PORTAL;
			tmplreg[nlreg]->rname = yyvsp[0].map;
			nlreg++;
		  ;
    break;}
case 151:
#line 1099 "lev_comp.y"
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  ;
    break;}
case 152:
#line 1108 "lev_comp.y"
{
			tmplreg[nlreg]->del_islev = yyvsp[0].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
		  ;
    break;}
case 153:
#line 1116 "lev_comp.y"
{
			switch(yyvsp[0].i) {
			case -1: tmplreg[nlreg]->rtype = LR_TELE; break;
			case 0: tmplreg[nlreg]->rtype = LR_DOWNTELE; break;
			case 1: tmplreg[nlreg]->rtype = LR_UPTELE; break;
			}
			tmplreg[nlreg]->rname = 0;
			nlreg++;
		  ;
    break;}
case 154:
#line 1128 "lev_comp.y"
{
			tmplreg[nlreg] = New(lev_region);
			tmplreg[nlreg]->in_islev = yyvsp[0].i;
			tmplreg[nlreg]->inarea.x1 = current_region.x1;
			tmplreg[nlreg]->inarea.y1 = current_region.y1;
			tmplreg[nlreg]->inarea.x2 = current_region.x2;
			tmplreg[nlreg]->inarea.y2 = current_region.y2;
		  ;
    break;}
case 155:
#line 1137 "lev_comp.y"
{
			tmplreg[nlreg]->del_islev = yyvsp[0].i;
			tmplreg[nlreg]->delarea.x1 = current_region.x1;
			tmplreg[nlreg]->delarea.y1 = current_region.y1;
			tmplreg[nlreg]->delarea.x2 = current_region.x2;
			tmplreg[nlreg]->delarea.y2 = current_region.y2;
			tmplreg[nlreg]->rtype = LR_BRANCH;
			tmplreg[nlreg]->rname = 0;
			nlreg++;
		  ;
    break;}
case 156:
#line 1150 "lev_comp.y"
{
			yyval.i = -1;
		  ;
    break;}
case 157:
#line 1154 "lev_comp.y"
{
			yyval.i = yyvsp[0].i;
		  ;
    break;}
case 158:
#line 1160 "lev_comp.y"
{
			yyval.i = 0;
		  ;
    break;}
case 159:
#line 1164 "lev_comp.y"
{
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if (yyvsp[-7].i <= 0 || yyvsp[-7].i >= COLNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-5].i < 0 || yyvsp[-5].i >= ROWNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-3].i <= 0 || yyvsp[-3].i >= COLNO)
				yyerror("Region out of level range!");
			else if (yyvsp[-1].i < 0 || yyvsp[-1].i >= ROWNO)
				yyerror("Region out of level range!");
			current_region.x1 = yyvsp[-7].i;
			current_region.y1 = yyvsp[-5].i;
			current_region.x2 = yyvsp[-3].i;
			current_region.y2 = yyvsp[-1].i;
			yyval.i = 1;
		  ;
    break;}
case 160:
#line 1184 "lev_comp.y"
{
			tmpfountain[nfountain] = New(fountain);
			tmpfountain[nfountain]->x = current_coord.x;
			tmpfountain[nfountain]->y = current_coord.y;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Fountain");
			nfountain++;
		  ;
    break;}
case 161:
#line 1196 "lev_comp.y"
{
			tmpsink[nsink] = New(sink);
			tmpsink[nsink]->x = current_coord.x;
			tmpsink[nsink]->y = current_coord.y;
			nsink++;
		  ;
    break;}
case 162:
#line 1205 "lev_comp.y"
{
			tmppool[npool] = New(pool);
			tmppool[npool]->x = current_coord.x;
			tmppool[npool]->y = current_coord.y;
			npool++;
		  ;
    break;}
case 163:
#line 1214 "lev_comp.y"
{
			tmpdig[ndig] = New(digpos);
			tmpdig[ndig]->x1 = current_region.x1;
			tmpdig[ndig]->y1 = current_region.y1;
			tmpdig[ndig]->x2 = current_region.x2;
			tmpdig[ndig]->y2 = current_region.y2;
			ndig++;
		  ;
    break;}
case 164:
#line 1225 "lev_comp.y"
{
			tmppass[npass] = New(digpos);
			tmppass[npass]->x1 = current_region.x1;
			tmppass[npass]->y1 = current_region.y1;
			tmppass[npass]->x2 = current_region.x2;
			tmppass[npass]->y2 = current_region.y2;
			npass++;
		  ;
    break;}
case 165:
#line 1236 "lev_comp.y"
{
			tmpreg[nreg] = New(region);
			tmpreg[nreg]->x1 = current_region.x1;
			tmpreg[nreg]->y1 = current_region.y1;
			tmpreg[nreg]->x2 = current_region.x2;
			tmpreg[nreg]->y2 = current_region.y2;
			tmpreg[nreg]->rlit = yyvsp[-3].i;
			tmpreg[nreg]->rtype = yyvsp[-1].i;
			if(yyvsp[0].i & 1) tmpreg[nreg]->rtype += MAXRTYPE+1;
			tmpreg[nreg]->rirreg = ((yyvsp[0].i & 2) != 0);
			if(current_region.x1 > current_region.x2 ||
			   current_region.y1 > current_region.y2)
			   yyerror("Region start > end!");
			if(tmpreg[nreg]->rtype == VAULT &&
			   (tmpreg[nreg]->rirreg ||
			    (tmpreg[nreg]->x2 - tmpreg[nreg]->x1 != 1) ||
			    (tmpreg[nreg]->y2 - tmpreg[nreg]->y1 != 1)))
				yyerror("Vaults must be exactly 2x2!");
			if(want_warnings && !tmpreg[nreg]->rirreg &&
			   current_region.x1 > 0 && current_region.y1 > 0 &&
			   current_region.x2 < max_x_map &&
			   current_region.y2 < max_y_map) {
			    /* check for walls in the room */
			    char ebuf[60];
			    register int x, y, nrock = 0;

			    for(y=current_region.y1; y<=current_region.y2; y++)
				for(x=current_region.x1;
				    x<=current_region.x2; x++)
				    if(IS_ROCK(tmpmap[y][x]) ||
				       IS_DOOR(tmpmap[y][x])) nrock++;
			    if(nrock) {
				Sprintf(ebuf,
					"Rock in room (%02d,%02d,%02d,%02d)?!",
					current_region.x1, current_region.y1,
					current_region.x2, current_region.y2);
				yywarning(ebuf);
			    }
			    if (
		!IS_ROCK(tmpmap[current_region.y1-1][current_region.x1-1]) ||
		!IS_ROCK(tmpmap[current_region.y2+1][current_region.x1-1]) ||
		!IS_ROCK(tmpmap[current_region.y1-1][current_region.x2+1]) ||
		!IS_ROCK(tmpmap[current_region.y2+1][current_region.x2+1])) {
				Sprintf(ebuf,
				"NonRock edge in room (%02d,%02d,%02d,%02d)?!",
					current_region.x1, current_region.y1,
					current_region.x2, current_region.y2);
				yywarning(ebuf);
			    }
			} else if(tmpreg[nreg]->rirreg &&
		!IS_ROOM(tmpmap[current_region.y1][current_region.x1])) {
			    char ebuf[60];
			    Sprintf(ebuf,
				    "Rock in irregular room (%02d,%02d)?!",
				    current_region.x1, current_region.y1);
			    yyerror(ebuf);
			}
			nreg++;
		  ;
    break;}
case 166:
#line 1298 "lev_comp.y"
{
			tmpaltar[naltar] = New(altar);
			tmpaltar[naltar]->x = current_coord.x;
			tmpaltar[naltar]->y = current_coord.y;
			tmpaltar[naltar]->align = yyvsp[-2].i;
			tmpaltar[naltar]->shrine = yyvsp[0].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Altar");
			naltar++;
		  ;
    break;}
case 167:
#line 1312 "lev_comp.y"
{
			tmpgold[ngold] = New(gold);
			tmpgold[ngold]->x = current_coord.x;
			tmpgold[ngold]->y = current_coord.y;
			tmpgold[ngold]->amount = yyvsp[-2].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Gold");
			ngold++;
		  ;
    break;}
case 168:
#line 1325 "lev_comp.y"
{
			tmpengraving[nengraving] = New(engraving);
			tmpengraving[nengraving]->x = current_coord.x;
			tmpengraving[nengraving]->y = current_coord.y;
			tmpengraving[nengraving]->e.text = yyvsp[0].map;
			tmpengraving[nengraving]->etype = yyvsp[-2].i;
			if (!in_room)
			    check_coord(current_coord.x, current_coord.y,
					"Engraving");
			nengraving++;
		  ;
    break;}
case 170:
#line 1340 "lev_comp.y"
{
			yyval.i = - MAX_REGISTERS - 1;
		  ;
    break;}
case 173:
#line 1348 "lev_comp.y"
{
			yyval.i = - MAX_REGISTERS - 1;
		  ;
    break;}
case 176:
#line 1356 "lev_comp.y"
{
			yyval.map = (char *) 0;
		  ;
    break;}
case 178:
#line 1363 "lev_comp.y"
{
			yyval.map = (char *) 0;
		  ;
    break;}
case 179:
#line 1369 "lev_comp.y"
{
			int token = get_trap_type(yyvsp[0].map);
			if (token == ERR)
				yyerror("Unknown trap type!");
			yyval.i = token;
		  ;
    break;}
case 181:
#line 1379 "lev_comp.y"
{
			int token = get_room_type(yyvsp[0].map);
			if (token == ERR) {
				yywarning("Unknown room type!  Making ordinary room...");
				yyval.i = OROOM;
			} else
				yyval.i = token;
		  ;
    break;}
case 183:
#line 1391 "lev_comp.y"
{
			yyval.i = 0;
		  ;
    break;}
case 184:
#line 1395 "lev_comp.y"
{
			yyval.i = yyvsp[0].i;
		  ;
    break;}
case 185:
#line 1399 "lev_comp.y"
{
			yyval.i = yyvsp[-2].i + (yyvsp[0].i << 1);
		  ;
    break;}
case 188:
#line 1407 "lev_comp.y"
{
			current_coord.x = current_coord.y = -MAX_REGISTERS-1;
		  ;
    break;}
case 195:
#line 1423 "lev_comp.y"
{
			yyval.i = - MAX_REGISTERS - 1;
		  ;
    break;}
case 198:
#line 1433 "lev_comp.y"
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				current_coord.x = current_coord.y = - yyvsp[-1].i - 1;
		  ;
    break;}
case 199:
#line 1442 "lev_comp.y"
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  ;
    break;}
case 200:
#line 1451 "lev_comp.y"
{
			if ( yyvsp[-1].i >= MAX_REGISTERS )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  ;
    break;}
case 201:
#line 1460 "lev_comp.y"
{
			if ( yyvsp[-1].i >= 3 )
				yyerror("Register Index overflow!");
			else
				yyval.i = - yyvsp[-1].i - 1;
		  ;
    break;}
case 203:
#line 1472 "lev_comp.y"
{
			if (check_monster_char((char) yyvsp[0].i))
				yyval.i = yyvsp[0].i ;
			else {
				yyerror("Unknown monster class!");
				yyval.i = ERR;
			}
		  ;
    break;}
case 204:
#line 1483 "lev_comp.y"
{
			char c = yyvsp[0].i;
			if (check_object_char(c))
				yyval.i = c;
			else {
				yyerror("Unknown char class!");
				yyval.i = ERR;
			}
		  ;
    break;}
case 207:
#line 1499 "lev_comp.y"
{
			yyval.map = (char *) 0;
		  ;
    break;}
case 212:
#line 1513 "lev_comp.y"
{
			if (!in_room && !init_lev.init_present &&
			    (yyvsp[-3].i < 0 || yyvsp[-3].i > max_x_map ||
			     yyvsp[-1].i < 0 || yyvsp[-1].i > max_y_map))
			    yyerror("Coordinates out of map range!");
			current_coord.x = yyvsp[-3].i;
			current_coord.y = yyvsp[-1].i;
		  ;
    break;}
case 213:
#line 1524 "lev_comp.y"
{
/* This series of if statements is a hack for MSC 5.1.  It seems that its
   tiny little brain cannot compile if these are all one big if statement. */
			if (yyvsp[-7].i < 0 || yyvsp[-7].i > max_x_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-5].i < 0 || yyvsp[-5].i > max_y_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-3].i < 0 || yyvsp[-3].i > max_x_map)
				yyerror("Region out of map range!");
			else if (yyvsp[-1].i < 0 || yyvsp[-1].i > max_y_map)
				yyerror("Region out of map range!");
			current_region.x1 = yyvsp[-7].i;
			current_region.y1 = yyvsp[-5].i;
			current_region.x2 = yyvsp[-3].i;
			current_region.y2 = yyvsp[-1].i;
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
#line 1542 "lev_comp.y"

