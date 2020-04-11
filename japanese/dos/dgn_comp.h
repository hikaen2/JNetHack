typedef union
{
	int	i;
	char*	str;
} YYSTYPE;
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


extern YYSTYPE yylval;
