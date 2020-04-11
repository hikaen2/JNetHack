/*	SCCS Id: @(#)role.c	3.3	1999/11/26	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"


/*** Table of all roles ***/
/* According to AD&D, HD for some classes (ex. Wizard) should be smaller
 * (4-sided for wizards).  But this is not AD&D, and using the AD&D
 * rule here produces an unplayable character.  Thus I have used a minimum
 * of an 10-sided hit die for everything.  Another AD&D change: wizards get
 * a minimum strength of 4 since without one you can't teleport or cast
 * spells. --KAA
 *
 * As the wizard has been updated (wizard patch 5 jun '96) their HD can be
 * brought closer into line with AD&D. This forces wizards to use magic more
 * and distance themselves from their attackers. --LSZ
 *
 * With the introduction of races, some hit points and energy
 * has been reallocated for each race.  The values assigned
 * to the roles has been reduced by the amount allocated to
 * humans.  --KMH
 *
 * God names use a leading underscore to flag goddesses.
 */
/*JP
  Ê¸»úÎó¤ÎºÇ½é¤Î°ìÊ¸»úÌÜ¤ò¸«¤ë¥³¡¼¥É¤¬¿ï½ê¤ËÂ¸ºß¤¹¤ë¤Î¤Ç¡¢
  ±Ñ¸ìÌ¾¤ò»Ä¤·¤Æ¤ª¤¯¡£ (see you.h)
*/

const struct Role roles[] = {
{	{"Archeologist", 0}, 
	{"¹Í¸Å³Ø¼Ô", 0}, {
#if 0
	{"Digger",      0},
	{"Field Worker",0},
	{"Investigator",0},
	{"Exhumer",     0},
	{"Excavator",   0},
	{"Spelunker",   0},
	{"Speleologist",0},
	{"Collector",   0},
	{"Curator",     0} },
#endif
	{"¹Û°÷",	0},
	{"Ï«Æ¯¼Ô",      0},
	{"Ä´ºº¼Ô",      0},
	{"È¯·¡¼Ô",	0},
	{"·¡ºï¼Ô",	0},
	{"Ãµ¸¡¼Ô",	0},
	{"Æ¶·¢³Ø¼Ô",    0},
	{"Èþ½Ñ¼ý½¸¼Ô",	0},
	{"´ÛÄ¹",	0} },

	"Quetzalcoatl", "Camaxtli", "Huhetotl", /* Central American */
	"Arc", "¹Í¸Å³ØÂç³Ø", "¥È¥ë¥Æ¥Ã¥¯²¦¤ÎÊè",
	PM_ARCHEOLOGIST, NON_PM, NON_PM,
	PM_LORD_CARNARVON, PM_STUDENT, PM_MINION_OF_HUHETOTL,
	NON_PM, PM_HUMAN_MUMMY, S_SNAKE, S_MUMMY,
	ART_ORB_OF_DETECTION,
	MH_HUMAN|MH_DWARF|MH_GNOME | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   7, 10, 10,  7,  7,  7 },
	{  20, 20, 20, 10, 20, 10 },
	/* Init   Lower  Higher */
	{ 11, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },14,	/* Energy */
	10, 5, 0, 2, 10, A_INT, SPE_MAGIC_MAPPING,   -4
},
{	{"Barbarian", 0}, 
	{"ÌîÈÚ¿Í", 0}, {
#if 0
	{"Plunderer",   "Plunderess"},
	{"Pillager",    0},
	{"Bandit",      0},
	{"Brigand",     0},
	{"Raider",      0},
	{"Reaver",      0},
	{"Slayer",      0},
	{"Chieftain",   "Chieftainess"},
	{"Conqueror",   "Conqueress"} },
#endif
	{"ÅðÂ±",	"½÷ÅðÂ±"},
	{"Î¬Ã¥¼Ô",	0},
	{"°­´Á",	0},
	{"»³Â±",	0},
	{"¿¯Î¬¼Ô",	0},
	{"¶¯Åð",	0},
	{"»¦Ù¤¼Ô",	0},
	{"¼óÎÎ",	"½÷¼óÎÎ"},
        {"À¬Éþ²¦",	0} },
	"Mitra", "Crom", "Set", /* Hyborian */
	"Bar", "¥Ç¥å¥¢¥êÂ²¤Î¥­¥ã¥ó¥×", "¥Ç¥å¥¢¥êÂ²¤Î¥ª¥¢¥·¥¹",
	PM_BARBARIAN, NON_PM, NON_PM,
	PM_PELIAS, PM_CHIEFTAIN, PM_THOTH_AMON,
	PM_OGRE, PM_TROLL, S_OGRE, S_TROLL,
	ART_HEART_OF_AHRIMAN,
	MH_HUMAN|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  16,  7,  7, 15, 16,  6 },
	{  30,  6,  7, 20, 30,  7 },
	/* Init   Lower  Higher */
	{ 14, 0,  0,10,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	10, 14, 0, 0,  8, A_INT, SPE_HASTE_SELF,      -4
},
{	{"Caveman", "Cavewoman"}, 
	{"Æ¶·¢¿Í", 0}, {
#if 0
	{"Troglodyte",  0},
	{"Aborigine",   0},
	{"Wanderer",    0},
	{"Vagrant",     0},
	{"Wayfarer",    0},
	{"Roamer",      0},
	{"Nomad",       0},
	{"Rover",       0},
	{"Pioneer",     0} },
#endif
	{"·êµï¿Í",	0},
	{"¸¶½»Ì±",	0},
	{"ÊüÏ²¼Ô",	0},
	{"ÉâÏ²¼Ô",	0},
	{"Î¹¹Ô¼Ô",	0},
	{"ÊüÍ·¼Ô",	0},
	{"Í·ËÒÌ±",	0},
	{"Î®Ï²¼Ô",	0},
	{"Àè¶î¼Ô",	0} },
	"Anu", "_Ishtar", "Anshar", /* Babylonian */
	"Cav", "ÂÀ¸Å¤ÎÆ¶·¢", "Îµ¤Î±£¤ì²È",
	PM_CAVEMAN, PM_CAVEWOMAN, PM_LITTLE_DOG,
	PM_SHAMAN_KARNOV, PM_NEANDERTHAL, PM_CHROMATIC_DRAGON,
	PM_BUGBEAR, PM_HILL_GIANT, S_HUMANOID, S_GIANT,
	ART_SCEPTRE_OF_MIGHT,
	MH_HUMAN|MH_DWARF|MH_GNOME | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{  10,  7,  7,  7,  8,  6 },
	{  30,  6,  7, 20, 30,  7 },
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	0, 12, 0, 1,  8, A_INT, SPE_DIG,             -4
},
#ifdef FIGHTER
{	{"Fighter", 0}, 
	{"Àï»Î", 0}, {
	{"¥Þ¡¼¥­¥å¥ê¡¼",0},
	{"¥Ó¡¼¥Ê¥¹",	0},
	{"¥Þ¡¼¥º",	0},
	{"¥¸¥å¥Ô¥¿¡¼",	0},
	{"¥µ¥¿¡¼¥ó",	0},
	{"¥¦¥é¥Ì¥¹",	0},
	{"¥Í¥×¥Á¥å¡¼¥ó",0},
	{"¥×¥ë¡¼¥È",	0},
	{"¥à¡¼¥ó",	0} },
	"Selene", "Helios", "Eos",
	"Fig", "·î¤ÎµÜÅÂ",
	    "ÃÏµå",
	PM_FIGHTER, NON_PM, PM_KITTEN,
	 PM_PRINCESS_OF_MOON, PM_PLANETARY_FIGHTER, PM_JEDEITE,
    	PM_EARTH_ELEMENTAL, PM_SNAKE, S_SNAKE, S_ZOMBIE,
	ART_SILVER_CRYSTAL,
	MH_HUMAN | ROLE_FEMALE |
	  ROLE_LAWFUL| ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  10, 10,  6, 12, 12, 18 },
	{  30, 10, 10, 20, 20, 10 },
	/* Init   Lower  Higher */
	{ 12, 0,  0, 8,  1, 0 },	/* Hit points */
	{  2, 2,  0, 2,  0, 2 },10,	/* Energy */
	0, 1, 1, 10, 20, A_CHA, SPE_CHARM_MONSTER, -4
},
#endif
{	{"Healer", 0}, 
	{"Ìô»Õ", 0}, {
#if 0
	{"Rhizotomist",    0},
	{"Empiric",        0},
	{"Embalmer",       0},
	{"Dresser",        0},
	{"Medici ossium",  0},
	{"Herbalist",      0},
	{"Magister",       0},
	{"Physician",      0},
	{"Chirurgeon",     0} },
#endif
	{"¸«½¬¤¤",      0},
	{"°å»Õ¸«½¬¤¤",	0},
	{"´Ç¸î»Õ",	"´Ç¸îÉØ"},
	{"°å»Õ½õ¼ê",	0},
	{"ÌôÊª¼çÇ¤",	0},
	{"°å»Õ¼çÇ¤",	"´Ç¸î¼çÇ¤"},
	{"´ÁÊý°å",	0},
	{"Æâ²Ê°å",	0},
	{"³°²Ê°å",	0} },
	"_Athena", "Hermes", "Poseidon", /* Greek */
	"Hea", "¥¨¥Ô¥À¥¦¥é¥¹»û±¡", "¥³¥ª¥¹»û±¡",
	PM_HEALER, NON_PM, NON_PM,
	PM_HIPPOCRATES, PM_ATTENDANT, PM_CYCLOPS,
	PM_GIANT_RAT, PM_SNAKE, S_RODENT, S_YETI,
	ART_STAFF_OF_AESCULAPIUS,
	MH_HUMAN|MH_GNOME | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   7,  7, 13,  7, 11, 16 },
	{  15, 20, 20, 15, 25, 10 },
	/* Init   Lower  Higher */
	{ 11, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 4,  0, 1,  0, 2 },20,	/* Energy */
	10, 3,-3, 2, 10, A_WIS, SPE_CURE_SICKNESS,   -4
},
{	{"Knight", 0}, 
	{"µ³»Î", 0}, {
#if 0
	{"Gallant",     0},
	{"Esquire",     0},
	{"Bachelor",    0},
	{"Sergeant",    0},
	{"Knight",      0},
	{"Banneret",    0},
	{"Chevalier",   0},
	{"Seignieur",   0},
	{"Paladin",     0} },
#endif
	{"¸«½¬¤¤",	0},
	{"ÊâÊ¼",	0},
	{"Àï»Î",	"½÷Àï»Î"},
	{"µ³Ê¼",	0},
	{"½ÅÀï»Î",	0},
	{"µ³»Î",	0},
	{"½Åµ³»Î",	0},
	{"·®µ³»Î",	0},
	{"À»µ³»Î",	0} },
	"Lugh", "_Brigit", "Manannan Mac Lir", /* Celtic */
	"Kni", "¥­¥ã¥á¥í¥Ã¥È¾ë", "¥¬¥é¥¹¤ÎÅç",
	PM_KNIGHT, NON_PM, PM_PONY,
	PM_KING_ARTHUR, PM_PAGE, PM_IXOTH,
	PM_QUASIT, PM_OCHRE_JELLY, S_IMP, S_JELLY,
	ART_MAGIC_MIRROR_OF_MERLIN,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{  13,  7, 14,  8, 10, 17 },
	{  20, 15, 15, 10, 20, 10 },
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 4,  0, 1,  0, 2 },10,	/* Energy */
	10, 8,-2, 0,  9, A_WIS, SPE_TURN_UNDEAD,     -4
},
{	{"Monk", 0}, 
	{"ÉðÆ®²È", 0}, {
#if 0
	{"Candidate",                 0},
	{"Novice",                    0},
	{"Initiate",                  0},
	{"Student of the Stone path", 0},
	{"Student of the Waters",     0},
	{"Student of Metals",         0},
	{"Student of the Winds",      0},
	{"Student of Fire",           0},
	{"Master",                    0} },
#endif
	{"ÆþÌç´õË¾¼Ô",	0},
	{"½é¿´¼Ô",	0},
	{"ÆþÌç¼ÔÅÁ",	0},
	{"ÅÚ¤Î½¬¤¤¼ê",	0},
	{"¿å¤Î½¬¤¤¼ê",	0},
	{"¶â¤Î½¬¤¤¼ê",	0},
	{"ÌÚ¤Î½¬¤¤¼ê",	0},
	{"²Ð¤Î½¬¤¤¼ê",	0},
	{"ÌÈµö³§ÅÁ",	0} },
	"Shan Lai Ching", "Chih Sung-tzu", "Huan Ti", /* Chinese */
	"Mon", "¥Á¥ã¥ó¡¦¥¹¡¼½¤Æ»±¡",
	  "ÃÏ²¦¤Î½¤Æ»±¡",
	PM_MONK, NON_PM, NON_PM,
	PM_GRAND_MASTER, PM_ABBOT, PM_MASTER_KAEN,
	PM_EARTH_ELEMENTAL, PM_XORN, S_ELEMENTAL, S_XORN,
	ART_EYES_OF_THE_OVERWORLD,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  10,  7,  8,  8,  7,  7 },
	{  25, 10, 20, 20, 15, 10 },
	/* Init   Lower  Higher */
	{ 12, 0,  0, 8,  1, 0 },	/* Hit points */
	{  2, 2,  0, 2,  0, 2 },10,	/* Energy */
	10, 8,-2, 2, 20, A_WIS, SPE_RESTORE_ABILITY, -4
},
{	{"Priest", "Priestess"}, 
	{"ÁÎÎ·", "ÆôÁÎ"}, {
#if 0
	{"Aspirant",    0},
	{"Acolyte",     0},
	{"Adept",       0},
	{"Priest",      "Priestess"},
	{"Curate",      0},
	{"Canon",       "Canoness"},
	{"Lama",        0},
	{"Patriarch",   "Matriarch"},
	{"High Priest", "High Priestess"} },
#endif
	{"½¤Æ»¼Ô",	"½¤Æ»½÷"},
	{"»ø¼Ô",	0},
	{"»øº×",	0},
	{"ÁÎÎ·",	"ÆôÁÎ"},
	{"½õÇ¤»Êº×",	0},
	{"À»¼Ô",	"À»½÷"},
	{"»Ê¶µ",	0},
	{"Âç»Ê¶µ",	0},
	{"ÂçÁÎ¾å",      0} },
	0, 0, 0,	/* chosen randomly from among the other roles */
	"Pri", "°ÎÂç¤Ê¤ë»û±¡", "¥Ê¥ë¥¾¥¯»û±¡",
	PM_PRIEST, PM_PRIESTESS, NON_PM,
	PM_ARCH_PRIEST, PM_ACOLYTE, PM_NALZOK,
	PM_HUMAN_ZOMBIE, PM_WRAITH, S_ZOMBIE, S_WRAITH,
	ART_MITRE_OF_HOLINESS,
	MH_HUMAN|MH_ELF | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   7,  7, 10,  7,  7,  7 },
	{  15, 10, 30, 15, 20, 10 },
	/* Init   Lower  Higher */
	{ 12, 0,  0, 8,  1, 0 },	/* Hit points */
	{  4, 3,  0, 2,  0, 2 },10,	/* Energy */
	0, 3,-2, 2, 10, A_WIS, SPE_REMOVE_CURSE,    -4
},
  /* Note:  Rogue precedes Ranger so that use of `-R' on the command line
     retains its traditional meaning. */
{	{"Rogue", 0}, 
	{"ÅðÂ±", 0}, {
#if 0
	{"Footpad",     0},
	{"Cutpurse",    0},
	{"Rogue",       0},
	{"Pilferer",    0},
	{"Robber",      0},
	{"Burglar",     0},
	{"Filcher",     0},
	{"Magsman",     "Magswoman"},
	{"Thief",       0} },
#endif
	{"ÄÉ¤¤¤Ï¤®",	0},
	{"¤Ò¤Ã¤¿¤¯¤ê",	0},
	{"¥¹¥ê",	0},
	{"¤´¤í¤Ä¤­",	0},
	{"¤³¤½¤É¤í",	0},
	{"¶õÁã",	0},
	{"Å¥ËÀ",	"½÷Å¥ËÀ"},
	{"¶¯Åð",	0},
	{"ÂçÅ¥ËÀ",	0} },
	"Issek", "Mog", "Kos", /* Nehwon */
	"Rog", "ÅðÂ±¥®¥ë¥É", "°Å»¦¼Ô¥®¥ë¥É",
	PM_ROGUE, NON_PM, NON_PM,
	PM_MASTER_OF_THIEVES, PM_THUG, PM_MASTER_ASSASSIN,
	PM_LEPRECHAUN, PM_GUARDIAN_NAGA, S_NYMPH, S_NAGA,
	ART_MASTER_KEY_OF_THIEVERY,
	MH_HUMAN|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   7,  7,  7, 10,  7,  6 },
	{  20, 10, 10, 30, 20, 10 },
	/* Init   Lower  Higher */
	{ 10, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },11,	/* Energy */
	10, 8, 0, 1,  9, A_INT, SPE_DETECT_TREASURE, -4
},
{	{"Ranger", 0}, 
	{"¥ì¥ó¥¸¥ã¡¼", 0}, {
#if 0	/* OBSOLETE */
	{"Edhel",       "Elleth"},
	{"Edhel",       "Elleth"},      /* elf-maid */
	{"Ohtar",       "Ohtie"},       /* warrior */
	{"Kano",			/* commander (Q.) ['a] */
			"Kanie"},	/* educated guess, until further research- SAC */
	{"Arandur",			/* king's servant, minister (Q.) - guess */
			"Aranduriel"},	/* educated guess */
	{"Hir",         "Hiril"},       /* lord, lady (S.) ['ir] */
	{"Aredhel",     "Arwen"},       /* noble elf, maiden (S.) */
	{"Ernil",       "Elentariel"},  /* prince (S.), elf-maiden (Q.) */
	{"Elentar",     "Elentari"},	/* Star-king, -queen (Q.) */
	"Solonor Thelandira", "Aerdrie Faenya", "Lolth", /* Elven */
#endif
#if 0
	{"Tenderfoot",    0},
	{"Lookout",       0},
	{"Trailblazer",   0},
	{"Reconnoiterer", "Reconnoiteress"},
	{"Scout",         0},
	{"Arbalester",    0},	/* One skilled at crossbows */
	{"Archer",        0},
	{"Sharpshooter",  0},
	{"Marksman",      "Markswoman"} },
#endif
	{"¿·ÊÆ",	0},
	{"¸«Ä¥¤ê",	0},
	{"ÀèÆ³",	0},
	{"Äå»¡",	0},
	{"ÀÍ¸õ",	0},
	{"µÝÊ¼",	0},	/* One skilled at crossbows */
	{"ÃæµéµÝÊ¼",	0},
	{"¾åµéµÝÊ¼",	0},
	{"Ì¾¿Í",	0} },
	"Mercury", "_Venus", "Mars", /* Roman/planets */
	"Ran", "¥ª¥ê¥ª¥ó¤Î¥­¥ã¥ó¥×", "¥ï¥ó¥Ñ¥¹¤ÎÆ¶·¢",
	PM_RANGER, NON_PM, PM_LITTLE_DOG /* Orion & canis major */,
	PM_ORION, PM_HUNTER, PM_SCORPIUS,
	PM_FOREST_CENTAUR, PM_SCORPION, S_CENTAUR, S_SPIDER,
	ART_LONGBOW_OF_ARTEMIS,
	MH_HUMAN|MH_ELF|MH_GNOME|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  13, 13, 13,  9, 13,  7 },
	{  30, 10, 10, 20, 20, 10 },
	/* Init   Lower  Higher */
	{ 13, 0,  0, 6,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },12,	/* Energy */
	10, 9, 2, 1, 10, A_INT, SPE_INVISIBILITY,   -4
},
{	{"Samurai", 0}, 
	{"»ø", 0}, {
#if 0
	{"Hatamoto",    0},  /* Banner Knight */
	{"Ronin",       0},  /* no allegiance */
	{"Ninja",       0},  /* secret society */
	{"Joshu",       0},  /* heads a castle */
	{"Ryoshu",      0},  /* has a territory */
	{"Kokushu",     0},  /* heads a province */
	{"Daimyo",      0},  /* a samurai lord */
	{"Kuge",        0},  /* Noble of the Court */
	{"Shogun",      0} },/* supreme commander, warlord */
#endif
	{"´úËÜ",	0},  /* Banner Knight */
	{"Ï²¿Í",	0},  /* no allegiance */
	{"Ç¦¼Ô",	"¤¯¥Î°ì"},  /* secret society */
	{"¾ë¼ç",	0},  /* heads a castle */
	{"ÎÎ¼ç",	0},  /* has a territory */
	{"¹ñ¼ç",	0},  /* heads a province */
	{"ÂçÌ¾",	"¹ø¸µ"},  /* a samurai lord */
	{"¸ø²È",	0},  /* Noble of the Court */
	{"¾­·³",	"Âç±ü"} },  /* supreme commander, warlord */
	"_Amaterasu Omikami", "Raijin", "Susanowo", /* Japanese */
	"Sam", "ÂÀÏº°ìÂ²¤Î¾ë", "¾­·³¤Î¾ë",
	PM_SAMURAI, NON_PM, PM_LITTLE_DOG,
	PM_LORD_SATO, PM_ROSHI, PM_ASHIKAGA_TAKAUJI,
	PM_WOLF, PM_STALKER, S_DOG, S_ELEMENTAL,
	ART_TSURUGI_OF_MURAMASA,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{  10,  8,  7, 10, 17,  6 },
	{  30, 10, 10, 30, 14, 10 },
	/* Init   Lower  Higher */
	{ 13, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },11,	/* Energy */
	10, 10, 0, 0,  8, A_INT, SPE_CLAIRVOYANCE,    -4
},
#ifdef TOURIST
{	{"Tourist", 0}, 
	{"´Ñ¸÷µÒ", 0}, {
#if 0
	{"Rambler",     0},
	{"Sightseer",   0},
	{"Excursionist",0},
	{"Peregrinator","Peregrinatrix"},
	{"Traveler",    0},
	{"Journeyer",   0},
	{"Voyager",     0},
	{"Explorer",    0},
	{"Adventurer",  0} },
#endif
	{"¥×¡¼ÂÀÏº",	"¥×¡¼»Ò"},
	{"´Ñ¸÷µÒ",	0},
	{"¼þÍ·Î¹¹Ô¼Ô",  0},
	{"Ê×Îò¼Ô",      0},
	{"Î¹¹Ô¼Ô",	0},
	{"Î¹¿Í",	0},
	{"¹Ò³¤¼Ô",	0},
	{"Ãµ¸¡²È",	0},
	{"ËÁ¸±¼Ô",	0} },
	"Blind Io", "_The Lady", "Offler", /* Discworld */
	"Tou", "Î¹¹Ô¥È¥é¥Ö¥ë¥»¥ó¥¿¡¼", "ÅðÂ±¥®¥ë¥É",
	PM_TOURIST, NON_PM, NON_PM,
	PM_TWOFLOWER, PM_GUIDE, PM_MASTER_OF_THIEVES,
	PM_GIANT_SPIDER, PM_FOREST_CENTAUR, S_SPIDER, S_CENTAUR,
	ART_YENDORIAN_EXPRESS_CARD,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   7, 10,  6,  7,  7, 10 },
	{  15, 10, 10, 15, 30, 20 },
	/* Init   Lower  Higher */
	{  8, 0,  0, 8,  0, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },14,	/* Energy */
	0, 5, 1, 2, 10, A_INT, SPE_CHARM_MONSTER,   -4
},
#endif
{	{"Valkyrie", 0}, 
	{"¥ï¥ë¥­¥å¡¼¥ì", 0}, {
#if 0
	{"Stripling",   0},
	{"Skirmisher",  0},
	{"Fighter",     0},
	{"Man-at-arms", "Woman-at-arms"},
	{"Warrior",     0},
	{"Swashbuckler",0},
	{"Hero",        "Heroine"},
	{"Champion",    0},
	{"Lord",        "Lady"} },
#endif
	{"¸«½¬¤¤",	0},
	{"ÊâÊ¼",	0},
	{"Àï»Î",	"½÷Àï»Î"},
	{"µ³Ê¼",      "½÷½Åµ³Ê¼"},
	{"ÀïÆ®Ê¼",	0},
	{"¹¶·âÊ¼",      0},
	{"±ÑÍº",	0},
	{"Æ®»Î",	"½÷Æ®»Î"},
	{"¥í¡¼¥É",	"¥ì¥Ç¥£"} },
	"Tyr", "Odin", "Loki", /* Norse */
	"Val", "±¿Ì¿¤ÎÀ»Æ²", "¥µ¡¼¥¿¡¼¤ÎÆ¶·¢",
	PM_VALKYRIE, NON_PM, NON_PM /*PM_WINTER_WOLF_CUB*/,
	PM_NORN, PM_WARRIOR, PM_LORD_SURTUR,
	PM_FIRE_ANT, PM_FIRE_GIANT, S_ANT, S_GIANT,
	ART_ORB_OF_FATE,
	MH_HUMAN|MH_DWARF | ROLE_FEMALE | ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{  10,  7,  7,  7, 10,  7 },
	{  30,  6,  7, 20, 30,  7 },
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	0, 10,-2, 0,  9, A_WIS, SPE_CONE_OF_COLD,    -4
},
{	{"Wizard", 0}, 
	{"ËâË¡»È¤¤", 0}, {
#if 0
	{"Evoker",      0},
	{"Conjurer",    0},
	{"Thaumaturge", 0},
	{"Magician",    0},
	{"Enchanter",   "Enchantress"},
	{"Sorcerer",    "Sorceress"},
	{"Necromancer", 0},
	{"Wizard",      0},
	{"Mage",        0} },
#endif
	{"¼êÉÊ»Õ",	0},
	{"´ñ½Ñ»Õ",	0},
	{"Àê¤¤»Õ",	0},
	{"Îî´¶»Õ",	0},
	{"¾¤´­»Õ",	0},
	{"ÍÅ½Ñ»Õ",      0},
        {"Ëâ½Ñ»Õ",      0},
	{"ËâË¡»È¤¤",	"Ëâ½÷"},
	{"ÂçËâË¡»È¤¤",	0} },
	"Ptah", "Thoth", "Anhur", /* Egyptian */
	"Wiz", "Ä´ÏÂ¤ÎÅã", "°Å¹õ¤ÎÅã",
	PM_WIZARD, NON_PM, PM_KITTEN,
	PM_WIZARD_OF_BALANCE, PM_APPRENTICE, PM_DARK_ONE,
	PM_VAMPIRE_BAT, PM_XORN, S_BAT, S_WRAITH,
	ART_EYE_OF_THE_AETHIOPICA,
	MH_HUMAN|MH_ELF|MH_GNOME|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   7, 10,  7,  7,  7,  7 },
	{  10, 30, 10, 20, 20, 10 },
	/* Init   Lower  Higher */
	{ 10, 0,  0, 8,  1, 0 },	/* Hit points */
	{  4, 3,  0, 2,  0, 3 },12,	/* Energy */
	0, 1, 0, 3, 10, A_INT, SPE_MAGIC_MISSILE,   -4
},
/* Array terminator */
{{0, 0}}
};


/* The player's role, created at runtime from initial
 * choices.  This may be munged in role_init().
 */
struct Role urole =
{	{"Undefined", 0}, {"Ææ", 0}, { {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	"L", "N", "C", "Xxx", "home", "locate",
	NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM,
	NON_PM, NON_PM, 0, 0, 0, 0,
	/* Str Int Wis Dex Con Cha */
	{   7,  7,  7,  7,  7,  7 },
	{  20, 15, 15, 20, 20, 10 },
	/* Init   Lower  Higher */
	{ 10, 0,  0, 8,  1, 0 },	/* Hit points */
	{  2, 0,  0, 2,  0, 3 },14,	/* Energy */
	0, 10, 0, 0,  4, A_INT, 0, -3
};



/* Table of all races */
const struct Race races[] = {
/*JP
  Ê¸»úÎó¤ÎºÇ½é¤Î°ìÊ¸»úÌÜ¤ò¸«¤ë¥³¡¼¥É¤¬¿ï½ê¤ËÂ¸ºß¤¹¤ë¤Î¤Ç¡¢
  ±Ñ¸ìÌ¾¤ò»Ä¤·¤Æ¤ª¤¯¡£ (see you.h)
*/
{/*JP	"human", "human", "humanity", "Hum",
	{"man", "woman"},
 */
	"human", "¿Í´Ö", "human", "humanity", "Hum",
	{"ÃË", "½÷"},
	PM_HUMAN, NON_PM, PM_HUMAN_MUMMY, PM_HUMAN_ZOMBIE,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	MH_HUMAN, 0, MH_GNOME|MH_ORC,
	/*    Str     Int Wis Dex Con Cha */
	{      3,      3,  3,  3,  3,  3 },
	{ STR18(100), 18, 18, 18, 18, 18 },
	/* Init   Lower  Higher */
	{  2, 0,  0, 2,  1, 0 },	/* Hit points */
	{  1, 0,  2, 0,  2, 0 }		/* Energy */
},
{/*JP	"elf", "elven", "elvenkind", "Elf",*/
	"elf", "¥¨¥ë¥Õ", "elven", "elvenkind", "Elf",
	{0, 0},
	PM_ELF, NON_PM, PM_ELF_MUMMY, PM_ELF_ZOMBIE,
	MH_ELF | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_ELF, MH_ELF, MH_ORC,
	/*  Str    Int Wis Dex Con Cha */
	{    3,     3,  3,  3,  3,  3 },
	{   18,    20, 20, 18, 16, 18 },
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  1, 0 },	/* Hit points */
	{  2, 0,  3, 0,  3, 0 }		/* Energy */
},
{/*JP	"dwarf", "dwarven", "dwarvenkind", "Dwa",*/
	"dwarf", "¥É¥ï¡¼¥Õ", "dwarven", "dwarvenkind", "Dwa",
	{0, 0},
	PM_DWARF, NON_PM, PM_DWARF_MUMMY, PM_DWARF_ZOMBIE,
	MH_DWARF | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	MH_DWARF, MH_DWARF|MH_GNOME, MH_ORC,
	/*    Str     Int Wis Dex Con Cha */
	{      3,      3,  3,  3,  3,  3 },
	{ STR18(100), 16, 16, 20, 20, 16 },
	/* Init   Lower  Higher */
	{  4, 0,  0, 3,  2, 0 },	/* Hit points */
	{  0, 0,  0, 0,  0, 0 }		/* Energy */
},
{/*JP	"gnome", "gnomish", "gnomehood", "Gno",*/
	"gnome", "¥Î¡¼¥à", "gnomish", "gnomehood", "Gno",
	{0, 0},
	PM_GNOME, NON_PM, PM_GNOME_MUMMY, PM_GNOME_ZOMBIE,
	MH_GNOME | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	MH_GNOME, MH_DWARF|MH_GNOME, MH_HUMAN,
	/*  Str    Int Wis Dex Con Cha */
	{    3,     3,  3,  3,  3,  3 },
	{STR18(50),19, 18, 18, 18, 18 },
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  0, 0 },	/* Hit points */
	{  2, 0,  2, 0,  2, 0 }		/* Energy */
},
{/*JP	"orc", "orcish", "orcdom", "Orc",*/
	"orc", "¥ª¡¼¥¯", "orcish", "orcdom", "Orc",
	{0, 0},
	PM_ORC, NON_PM, PM_ORC_MUMMY, PM_ORC_ZOMBIE,
	MH_ORC | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_ORC, 0, MH_HUMAN|MH_ELF|MH_DWARF,
	/*  Str    Int Wis Dex Con Cha */
	{   3,      3,  3,  3,  3,  3 },
	{STR18(50),16, 16, 18, 18, 16 },
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  0, 0 },	/* Hit points */
	{  1, 0,  1, 0,  1, 0 }		/* Energy */
},
/* Array terminator */
{ 0, 0, 0, 0 }};


/* The player's race, created at runtime from initial
 * choices.  This may be munged in role_init().
 */
struct Race urace =
{	"something", "Ææ", "undefined", "something", "Xxx",
	{0, 0},
	NON_PM, NON_PM, NON_PM, NON_PM,
	0, 0, 0, 0,
	/*    Str     Int Wis Dex Con Cha */
	{      3,      3,  3,  3,  3,  3 },
	{ STR18(100), 18, 18, 18, 18, 18 },
	/* Init   Lower  Higher */
	{  2, 0,  0, 2,  1, 0 },	/* Hit points */
	{  1, 0,  2, 0,  2, 0 }		/* Energy */
};


/* Table of all genders */
const struct Gender genders[] = {
/*JP
  Ê¸»úÎó¤ÎºÇ½é¤Î°ìÊ¸»úÌÜ¤ò¸«¤ë¥³¡¼¥É¤¬¿ï½ê¤ËÂ¸ºß¤¹¤ë¤Î¤Ç¡¢
  ±Ñ¸ìÌ¾¤ò»Ä¤·¤Æ¤ª¤¯¡£ (see you.h)
*/
#if 0
	{"male",	"he",	"him",	"his",	"Mal",	ROLE_MALE},
	{"female",	"she",	"her",	"her",	"Fem",	ROLE_FEMALE},
	{"neuter",	"it",	"it",	"its",	"Ntr",	ROLE_NEUTER}
#endif
	{"male",	"ÃËÀ­",	"he",	"him",	"his",	"Mal",	ROLE_MALE},
	{"female",	"½÷À­",	"she",	"her",	"her",	"Fem",	ROLE_FEMALE},
	{"neuter",	"ÃæÀ­",	"it",	"it",	"its",	"Ntr",	ROLE_NEUTER}
};


/* Table of all alignments */
const struct Align aligns[] = {
	{"law",		"Ãá½ø",	"lawful",	"Law",	ROLE_LAWFUL,	A_LAWFUL},
	{"balance",	"ÃæÎ©",	"neutral",	"Neu",	ROLE_NEUTRAL,	A_NEUTRAL},
	{"chaos",	"º®ÆÙ",	"chaotic",	"Cha",	ROLE_CHAOTIC,	A_CHAOTIC},
	{"evil",	"Ìµ¿´",	"unaligned",	"Una",	0,		A_NONE}
};


boolean
validrole(rolenum)
	int rolenum;
{
	return (rolenum >= 0 && rolenum < SIZE(roles)-1);
}


int
randrole()
{
	return (rn2(SIZE(roles)-1));
}


int
str2role(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return (-1);

	/* "@" returns a random role */
	if (str[0] == '@')
		return (randrole());

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; roles[i].name.m; i++) {
	    /* Does it match the male name? */
	    if (!strncmpi(str, roles[i].name.m, len))
	    	return (i);
	    /* Or the female name? */
	    if (roles[i].name.f && !strncmpi(str, roles[i].name.f, len))
	    	return (i);
	    /* Or the filecode? */
	    if (!strcmpi(str, roles[i].filecode))
	    	return (i);
	}

	/* Couldn't find anything appropriate */
	return (-1);
}


boolean
validrace(rolenum, racenum)
	int rolenum, racenum;
{
	/* Assumes validrole */
	return (racenum >= 0 && racenum < SIZE(races)-1 &&
		(roles[rolenum].allow & races[racenum].allow & ROLE_RACEMASK));
}


int
randrace(rolenum)
	int rolenum;
{
	int i, n = 0;

	/* Count the number of valid races */
	for (i = 0; races[i].noun; i++)
	    if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK)
	    	n++;

	/* Pick a random race */
	/* Use a factor of 100 in case of bad random number generators */
	if (n) n = rn2(n*100)/100;
	for (i = 0; races[i].noun; i++)
	    if (roles[rolenum].allow & races[i].allow & ROLE_RACEMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role has no permitted races? */
	return (rn2(SIZE(races)-1));
}


int
str2race(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return (-1);

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; races[i].noun; i++) {
	    /* Does it match the noun? */
	    if (!strncmpi(str, races[i].noun, len))
	    	return (i);
	    /* Or the filecode? */
	    if (!strcmpi(str, races[i].filecode))
	    	return (i);
	}

	/* Couldn't find anything appropriate */
	return (-1);
}


boolean
validgend(rolenum, racenum, gendnum)
	int rolenum, racenum, gendnum;
{
	/* Assumes validrole and validrace */
	return (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		(roles[rolenum].allow & races[racenum].allow &
		 genders[gendnum].allow & ROLE_GENDMASK));
}


int
randgend(rolenum, racenum)
	int rolenum, racenum;
{
	int i, n = 0;

	/* Count the number of valid genders */
	for (i = 0; i < ROLE_GENDERS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		genders[i].allow & ROLE_GENDMASK)
	    	n++;

	/* Pick a random gender */
	if (n) n = rn2(n);
	for (i = 0; i < ROLE_GENDERS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		genders[i].allow & ROLE_GENDMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role/race has no permitted genders? */
	return (rn2(ROLE_GENDERS));
}


int
str2gend(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return (-1);

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; i < ROLE_GENDERS; i++) {
	    /* Does it match the adjective? */
	    if (!strncmpi(str, genders[i].adj, len))
	    	return (i);
	    /* Or the filecode? */
	    if (!strcmpi(str, genders[i].filecode))
	    	return (i);
	}

	/* Couldn't find anything appropriate */
	return (-1);
}


boolean
validalign(rolenum, racenum, alignnum)
	int rolenum, racenum, alignnum;
{
	/* Assumes validrole and validrace */
	return (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		(roles[rolenum].allow & races[racenum].allow &
		 aligns[alignnum].allow & ROLE_ALIGNMASK));
}


int
randalign(rolenum, racenum)
	int rolenum, racenum;
{
	int i, n = 0;

	/* Count the number of valid alignments */
	for (i = 0; i < ROLE_ALIGNS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		aligns[i].allow & ROLE_ALIGNMASK)
	    	n++;

	/* Pick a random alignment */
	if (n) n = rn2(n);
	for (i = 0; i < ROLE_ALIGNS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		aligns[i].allow & ROLE_ALIGNMASK) {
	    	if (n) n--;
	    	else return (i);
	    }

	/* This role/race has no permitted alignments? */
	return (rn2(ROLE_ALIGNS));
}


int
str2align(str)
	char *str;
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return (-1);

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; i < ROLE_ALIGNS; i++) {
	    /* Does it match the adjective? */
	    if (!strncmpi(str, aligns[i].adj, len))
	    	return (i);
	    /* Or the filecode? */
	    if (!strcmpi(str, aligns[i].filecode))
	    	return (i);
	}

	/* Couldn't find anything appropriate */
	return (-1);
}


void
plnamesuffix()
{
	char *sptr, *eptr;
	int i;

	/* Look for tokens delimited by '-' */
	if ((eptr = index(plname, '-')) != (char *) 0)
	    *eptr++ = '\0';
	while (eptr) {
	    /* Isolate the next token */
	    sptr = eptr;
	    if ((eptr = index(sptr, '-')) != (char *)0)
	    	*eptr++ = '\0';

	    /* Try to match it to something */
	    if ((i = str2role(sptr)) >= 0)
	    	flags.initrole = i;
	    else if ((i = str2race(sptr)) >= 0)
	    	flags.initrace = i;
	    else if ((i = str2gend(sptr)) >= 0)
	    	flags.initgend = i;
	    else if ((i = str2align(sptr)) >= 0)
	    	flags.initalign = i;
	}
	if(!plname[0]) {
	    askname();
	    plnamesuffix();
	}
	return;
}


/*
 *	Special setup modifications here:
 *
 *	Unfortunately, this is going to have to be done
 *	on each newgame or restore, because you lose the permonst mods
 *	across a save/restore.  :-)
 *
 *	1 - The Rogue Leader is the Tourist Nemesis.
 *	2 - Priests start with a random alignment - convert the leader and
 *	    guardians here.
 *	3 - Elves can have one of two different leaders, but can't work it
 *	    out here because it requires hacking the level file data (see
 *	    sp_lev.c).
 *
 * This code also replaces quest_init().
 */
void
role_init()
{
	int alignmnt;

	/* Strip the role letter out of the player name.
	 * This is included for backwards compatibility.
	 */
	plnamesuffix();

	/* Check for a valid role.  Try flags.initrole first. */
	if (!validrole(flags.initrole)) {
	    /* Try the player letter second */
	    if ((flags.initrole = str2role(pl_character)) < 0)
	    	/* None specified; pick a random role */
	    	flags.initrole = randrole();
	}

	/* We now have a valid role index.  Copy the role name back. */
	/* This should become OBSOLETE */
	Strcpy(pl_character, roles[flags.initrole].name.m);
	pl_character[PL_CSIZ-1] = '\0';

	/* Check for a valid race */
	if (!validrace(flags.initrole, flags.initrace))
	    flags.initrace = randrace(flags.initrole);

	/* Check for a valid gender.  Try flags.igend first. */
	if (!validgend(flags.initrole, flags.initrace, flags.initgend))
	    /* Use flags.female second.  Note that there is no way
	     * to check for an unspecified gender.
	     */
	    flags.initgend = flags.female;
	/* Don't change flags.female; this may be a restore */

	/* Check for a valid alignment */
	if (!validalign(flags.initrole, flags.initrace, flags.initalign))
	    /* Pick a random alignment */
	    flags.initalign = randalign(flags.initrole, flags.initrace);
	alignmnt = aligns[flags.initalign].value;

	/* Initialize urole and urace */
	urole = roles[flags.initrole];
	urace = races[flags.initrace];

	/* Fix up the quest leader */
	if (urole.ldrnum != NON_PM) {
	    mons[urole.ldrnum].msound = MS_LEADER;
	    mons[urole.ldrnum].mflags2 |= (M2_PEACEFUL);
	    mons[urole.ldrnum].mflags3 = M3_CLOSE;
	    mons[urole.ldrnum].maligntyp = alignmnt * 3;
	}

	/* Fix up the quest guardians */
	if (urole.guardnum != NON_PM) {
	    mons[urole.guardnum].mflags2 |= (M2_PEACEFUL);
	    mons[urole.guardnum].maligntyp = alignmnt * 3;
	}

	/* Fix up the quest nemesis */
	if (urole.neminum != NON_PM) {
	    mons[urole.neminum].msound = MS_NEMESIS;
	    mons[urole.neminum].mflags2 &= ~(M2_PEACEFUL);
	    mons[urole.neminum].mflags2 |= (M2_NASTY|M2_STALK|M2_HOSTILE);
	    mons[urole.neminum].mflags3 = M3_WANTSARTI | M3_WAITFORU;
	}

	/* Fix up the god names */
	if (flags.pantheon == -1) {		/* new game */
	    flags.pantheon = flags.initrole;	/* use own gods */
	    while (!roles[flags.pantheon].lgod)	/* unless they're missing */
		flags.pantheon = randrole();
	}
	if (!urole.lgod) {
	    urole.lgod = roles[flags.pantheon].lgod;
	    urole.ngod = roles[flags.pantheon].ngod;
	    urole.cgod = roles[flags.pantheon].cgod;
	}

	/* Fix up infravision */
	if (mons[urace.malenum].mflags3 & M3_INFRAVISION) {
	    /* although an infravision intrinsic is possible, infravision
	     * is purely a property of the physical race.  This means that we
	     * must put the infravision flag in the player's current race
	     * (either that or have separate permonst entries for
	     * elven/non-elven members of each class).  The side effect is that
	     * all NPCs of that class will have (probably bogus) infravision,
	     * but since infravision has no effect for NPCs anyway we can
	     * ignore this.
	     */
	    mons[urole.malenum].mflags3 |= M3_INFRAVISION;
	    if (urole.femalenum != NON_PM)
	    	mons[urole.femalenum].mflags3 |= M3_INFRAVISION;
	}

	/* Artifacts are fixed in hack_artifacts() */

	/* Success! */
	return;
}

/*JP
  ¤¢¤¤¤µ¤Ä¤ÏÆüËÜ¸ì¤È¤·¤Æ¼«Á³¤Ë¤Ê¤ë¤è¤¦Âç¤­¤¯»ÈÍÑ¤òÊÑ¹¹
 */
#if 0

const char *
Hello()
{
	switch (Role_switch) {
	case PM_KNIGHT:
	    return ("Salutations"); /* Olde English */
	case PM_SAMURAI:
	    return ("Konnichi wa"); /* Japanese */
#ifdef TOURIST
	case PM_TOURIST:
	    return ("¥¢¥í¡¼¥Ï");       /* Hawaiian */
#endif
	case PM_VALKYRIE:
	    return ("º²¤Î¼é¸î¼Ô");   /* Norse */
	default:
	    return ("¤è¤¦¤³¤½");
	}
}

const char *
Goodbye()
{
	switch (Role_switch) {
	case PM_KNIGHT:
	    return ("Fare thee well");  /* Olde English */
	case PM_SAMURAI:
	    return ("Sayonara");        /* Japanese */
#ifdef TOURIST
	case PM_TOURIST:
	    return ("Aloha");           /* Hawaiian */
#endif
	case PM_VALKYRIE:
	    return ("Farvel");          /* Norse */
	default:
	    return ("Goodbye");
	}
}
#endif

const char *
Hello(int nameflg)
{
    static char helo_buf[BUFSZ];


    switch (Role_switch) {
    case PM_KNIGHT:
	if(nameflg)
	    Sprintf(helo_buf, "¤è¤¯¤¾ÒÔ¤Ã¤¿%s¤è", plname);
	else
	    Sprintf(helo_buf, "¤è¤¯¤¾ÒÔ¤Ã¤¿");
	break;
    case PM_SAMURAI:
	if(nameflg)
	    Sprintf(helo_buf, "¤è¤¯¤¾»²¤é¤ì¤¿%s¤è", plname);
	else
	    Sprintf(helo_buf, "¤è¤¯¤¾»²¤é¤ì¤¿");
	break;
#ifdef TOURIST
    case PM_TOURIST:
	if(nameflg)
	    Sprintf(helo_buf, "¥¢¥í¡¼¥Ï%s", plname);
	else
	    Sprintf(helo_buf, "¥¢¥í¡¼¥Ï");
	break;
#endif
    case PM_VALKYRIE:
	if(nameflg)
	    Sprintf(helo_buf, "º²¤Î¼é¸î¼Ô%s¤è", plname);
	else
	    Sprintf(helo_buf, "º²¤Î¼é¸î¼Ô");
	break;
    default:
	if(nameflg)
	    Sprintf(helo_buf, "¤è¤¦¤³¤½%s", plname);
	else
	    Sprintf(helo_buf, "¤è¤¦¤³¤½");
	break;
    }

    return helo_buf;
}

const char *
Goodbye(int nameflg)
{
    static char helo_buf[BUFSZ];

    switch (Role_switch) {
    case PM_KNIGHT:
	if(nameflg)
	    Sprintf(helo_buf, "¤µ¤é¤Ð·ÉéÊ¤Êµ³»Î¤Î%s¤è", plname);
	else
	    Sprintf(helo_buf, "¤µ¤é¤Ð·ÉéÊ¤Êµ³»Î");
	break;
    case PM_SAMURAI:
	if(nameflg)
	    Sprintf(helo_buf, "¤µ¤é¤ÐÉð»ÎÆ»¤ò»Ö¤¹%s¤è", plname);
	else
	    Sprintf(helo_buf, "¤µ¤é¤ÐÉð»ÎÆ»¤ò»Ö¤¹");
	break;
#ifdef TOURIST
    case PM_TOURIST:
	if(nameflg)
	    Sprintf(helo_buf, "¥¢¥í¡¼¥Ï%s", plname);
	else
	    Sprintf(helo_buf, "¥¢¥í¡¼¥Ï");
	break;
#endif
    case PM_VALKYRIE:
	if(nameflg)
	    Sprintf(helo_buf, "¤µ¤é¤Ðº²¤Î¼é¸î¼Ô%s¤è", plname);
	else
	    Sprintf(helo_buf, "¤µ¤é¤Ðº²¤Î¼é¸î¼Ô");
	break;
    default:
	if(nameflg)
	    Sprintf(helo_buf, "¤µ¤è¤¦¤Ê¤é%s", plname);
	else
	    Sprintf(helo_buf, "¤µ¤è¤¦¤Ê¤é");
	break;
    }

    return helo_buf;
}
/* role.c */
