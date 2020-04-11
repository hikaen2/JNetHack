/*	SCCS Id: @(#)sounds.c	3.1	93/03/14	*/
/*	Copyright (c) 1989 Janet Walz, Mike Threepoint */
/* NetHack may be freely redistributed.  See license for details. */

/*
**	Japanese version Copyright
**	(c) Issei Numata, Naoki Hamada, Shigehiro Miyashita, 1994
**	changing point is marked `JP' (94/6/7)
**	JNetHack may be freely redistributed.  See license for details. 
*/

#include "hack.h"
#include "edog.h"

#ifdef OVLB

static int FDECL(domonnoise,(struct monst *));
static int NDECL(dochat);

#endif /* OVLB */

#ifdef SOUNDS

#ifdef OVL0

void
dosounds()
{
    register xchar hallu;
    register struct mkroom *sroom;
    register int vx, vy;
#if defined(AMIGA) && defined(AZTEC_C_WORKAROUND)
    int xx;
#endif

    hallu = Hallucination ? 1 : 0;

    if(!flags.soundok || u.uswallow || Underwater) return;

    if (level.flags.nfountains && !rn2(400)) {
	static const char *fountain_msg[4] = {
/*JP		"hear bubbling water.",
		"hear water falling on coins.",
		"hear the splashing of a naiad.",
		"hear a soda fountain!",*/
/*JP	        "�夬ˢΩ�Ĳ���ʹ������",
		"��ߤξ�˿夬����벻��ʹ������",
		"����������Ϥͤ벻��ʹ������",
		"ú�����β���ʹ������",*/
	        "���ܥ��ܤȸ�������ʹ������",
		"�ԥ���ԥ���ȸ�������ʹ������",
		"�Х���Х���ȸ�������ʹ������",
		"���塼�ȸ�������ʹ������"
	};
	You(fountain_msg[rn2(3)+hallu]);
    }
#ifdef SINK
    if (level.flags.nsinks && !rn2(300)) {
	static const char *sink_msg[3] = {
/*JP		"hear a slow drip.",
		"hear a gurgling noise.",
		"hear dishes being washed!",*/
	        "�夬�ݤ��ݤ�������벻��ʹ������",
		"���餬��ȸ�������ʹ������",
		"������������ʹ������",
	};
	You(sink_msg[rn2(2)+hallu]);
    }
#endif
    if (level.flags.has_court && !rn2(200)) {
	static const char *throne_msg[4] = {
/*JP		"hear the tones of courtly conversation.",
		"hear a sceptre pounded in judgment.",
		"Someone shouts \"Off with %s head!\"",
		"hear Queen Beruthiel's cats!",*/
	        "���ʤ��ä�����ʹ������",
		"��Ƚ�������ͤ�����ʹ������",
		"���줫���֤��Τ�Τμ��ķ�ͤ�פȶ�������ʹ������",
                "�٥륵���������ǭ������ʹ������",
	};
	int which = rn2(3)+hallu;
	if (which != 2) You(throne_msg[which]);
	else		pline(throne_msg[2], his[flags.female]);
	return;
    }
    if (level.flags.has_swamp && !rn2(200)) {
	static const char *swamp_msg[3] = {
/*JP		"hear mosquitoes!",
		"smell marsh gas!",	* so it's a smell...*
		"hear Donald Duck!",*/
		"��α�����ʹ������",
		"��ä�������������",	/* so it's a smell...*/
		"�ɥʥ�ɥ��å�������ʹ������",
	};
	You(swamp_msg[rn2(2)+hallu]);
	return;
    }
    if (level.flags.has_vault && !rn2(200)) {
	if (!(sroom = search_special(VAULT))) {
	    /* strange ... */
	    level.flags.has_vault = 0;
	    return;
	}
	if(gd_sound())
	    switch (rn2(2)+hallu) {
		case 1: {
		    boolean gold_in_vault = FALSE;

		    for (vx = sroom->lx;vx <= sroom->hx; vx++)
			for (vy = sroom->ly; vy <= sroom->hy; vy++)
			    if (g_at(vx, vy))
				gold_in_vault = TRUE;
#if defined(AMIGA) && defined(AZTEC_C_WORKAROUND)
		    /* Bug in aztec assembler here. Workaround below */
		    xx = ROOM_INDEX(sroom) + ROOMOFFSET;
		    xx = (xx != vault_occupied(u.urooms));
		    if(xx)
#else
		    if (vault_occupied(u.urooms) != 
			 (ROOM_INDEX(sroom) + ROOMOFFSET))
#endif /* AZTEC_C_WORKAROUND */
		    {
			if (gold_in_vault)
/*JP			    You(!hallu ? "hear someone counting money." :
				"hear the quarterback calling the play.");*/
			    You(!hallu ? "ï�������������Ƥ��벻��ʹ������":
				"���������Хå����ؼ��򤹤�����ʹ������");
			else
/*JP			    You("hear someone searching.");*/
			    You("ï����õ�����Ƥ��벻��ʹ������");
			break;
		    }
		    /* fall into... (yes, even for hallucination) */
		}
		case 0:
/*JP		    You("hear the footsteps of a guard on patrol.");*/
		    You("�������Υѥȥ��뤹�벻��ʹ������");
		    break;
		case 2:
/*JP		    You("hear Ebenezer Scrooge!");*/
		    You("���٥ͥ����������롼��������ʹ������");
		    break;
	    }
	return;
    }
    if (level.flags.has_beehive && !rn2(200)) {
	switch (rn2(2)+hallu) {
	    case 0:
/*JP		You("hear a low buzzing.");*/
	        You("�֡���ȸ�������ʹ������");
		break;
	    case 1:
/*JP		You("hear an angry drone.");*/
		You("��ʳ����ͺ�Х��β���ʹ������");
		break;
	    case 2:
/*JP		You("hear bees in your %sbonnet!",
		    uarmh ? "" : "(nonexistent) ");*/
		You("�ϥ������ʤ���%s˹�Ҥ���ˤ��벻��ʹ������",
		    uarmh ? "" : "(¸�ߤ��ʤ�)");
		break;
	}
	return;
    }
    if (level.flags.has_morgue && !rn2(200)) {
	switch (rn2(2)+hallu) {
	    case 0:
/*JP		You("suddenly realize it is unnaturally quiet.");*/
	        You("�Լ����ʤ��餤�Ť��ʤΤ����������Ĥ�����");
		break;
	    case 1:
/*JP		pline("The hair on the back of your %s stands up.",*/
		pline("���ʤ���%s�Τ�������Ӥ��դ��ä���",
			body_part(NECK));
		break;
	    case 2:
/*JP		pline("The hair on your %s seems to stand up.",*/
		pline("���ʤ���%s���Ӥϵդ��ä���",
			body_part(HEAD));
		break;
	}
	return;
    }
#ifdef ARMY
    if (level.flags.has_barracks && !rn2(200)) {
	static const char *barracks_msg[4] = {
/*JP		"hear blades being honed.",
		"hear loud snoring.",
		"hear dice being thrown.",
		"hear General MacArthur!",*/
	        "��ʪ�򸦤�����ʹ������",
		"�礭�ʤ��Ӥ���ʹ������",
		"�������������벻��ʹ������",
		"�ޥå����������Ĥ�����ʹ������",
	};
	You(barracks_msg[rn2(3)+hallu]);
	return;
    }
#endif /* ARMY */
    if (level.flags.has_zoo && !rn2(200)) {
	static const char *zoo_msg[3] = {
/*JP		"hear a sound reminiscent of an elephant stepping on a peanut.",
		"hear a sound reminiscent of a seal barking.",
		"hear Doctor Doolittle!",*/
	        "�ݤ��ԡ��ʥåĤξ���٤�褦�ʲ���ʹ������",
		"���������ʤ���褦�ʲ���ʹ������",
		"�ɥ�ȥ�����������ʹ������",
	};
	You(zoo_msg[rn2(2)+hallu]);
	return;
    }
    if (level.flags.has_shop && !rn2(200)) {
	if (!(sroom = search_special(ANY_SHOP))) {
	    /* strange... */
	    level.flags.has_shop = 0;
	    return;
	}
	if (tended_shop(sroom) &&
		!index(u.ushops, ROOM_INDEX(sroom) + ROOMOFFSET)) {
	    static const char *shop_msg[3] = {
/*JP		    "hear someone cursing shoplifters.",
		    "hear the chime of a cash register.",
		    "hear Neiman and Marcus arguing!",*/
		    "ï����ť����ΤΤ�������ʹ������",
		    "�쥸�Υ�����ȸ�������ʹ������",
		    "�ͥ��ޥ�ȥޥ륯���ε�����ʹ������",
	    };
	    You(shop_msg[rn2(2)+hallu]);
	}
	return;
    }
}

#endif /* OVL0 */
#ifdef OVLB

static const char *h_sounds[] = {
/*JP    "beep", "boing", "sing", "belche", "creak", "cough", "rattle",
    "ululate", "pop", "jingle", "sniffle", "tinkle", "eep"*/
    "�ԡ��ä��Ĥ���","�������Ƥ�","�Τä�","�����������Ĥ���",
    "���������","�������Ĥä�","�ۡ��ۡ��Ĥ���","�ݥ���Ĥ���",
    "����󥬥����Ĥ���","���󥯥��Ĥ���","�����������Ĥ���",
    "�����ä��Ĥ���"
};

void
growl(mtmp)
register struct monst *mtmp;
{
    register const char *growl_verb = 0;

    if (mtmp->msleep || !mtmp->mcanmove || !mtmp->data->msound) return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        growl_verb = h_sounds[rn2(SIZE(h_sounds))];
    else switch (mtmp->data->msound) {
	case MS_MEW:
	case MS_HISS:
/*JP	    growl_verb = "hisse";	* hisseS */
	    growl_verb = "�����ä��Ĥ���";	/* hisseS */
	    break;
	case MS_BARK:
	case MS_GROWL:
/*JP	    growl_verb = "growl";*/
	    growl_verb = "�Ϥ������ʤ���";
	    break;
	case MS_ROAR:
/*JP	    growl_verb = "roar";*/
	    growl_verb = "�ʤ���";
	    break;
	case MS_BUZZ:
/*JP        growl_verb = "buzze";*/
	    growl_verb = "�֡��ä��Ĥ���";
	    break;
	case MS_SQEEK:
/*JP        growl_verb = "squeal";*/
	    growl_verb = "���������Ĥ���";
	    break;
	case MS_SQAWK:
/*JP	    growl_verb = "screeche";*/
	    growl_verb = "���ڤ�����Ω�Ƥ�";
	    break;
	case MS_NEIGH:
/*JP	    growl_verb = "neigh";*/
	    growl_verb = "���ʤʤ���";
	    break;
	case MS_WAIL:
/*JP	    growl_verb = "wail";*/
	    growl_verb = "�ᤷ���Ĥ���";
	    break;
    }
/*JP    if (growl_verb) pline("%s %ss!", Monnam(mtmp), growl_verb);*/
    if (growl_verb) pline("%s��%s��", Monnam(mtmp), growl_verb);
}

/* the sounds of mistreated pets */
void
yelp(mtmp)
register struct monst *mtmp;
{
    register const char *yelp_verb = 0;

    if (mtmp->msleep || !mtmp->mcanmove || !mtmp->data->msound) return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        yelp_verb = h_sounds[rn2(SIZE(h_sounds))];
    else switch (mtmp->data->msound) {
	case MS_MEW:
/*JP	    yelp_verb = "yowl";*/
	    yelp_verb = "�ᤷ���Ĥ���";
	    break;
	case MS_BARK:
	case MS_GROWL:
/*JP	    yelp_verb = "yelp";*/
	    yelp_verb = "����󥭥���Ĥ���";
	    break;
	case MS_ROAR:
/*JP	    yelp_verb = "snarl";*/
	    yelp_verb = "���ʤä�";
	    break;
	case MS_SQEEK:
/*JP	    yelp_verb = "squeal";*/
	    yelp_verb = "���������Ĥ���";
	    break;
	case MS_SQAWK:
/*JP	    yelp_verb = "screak";*/
	    yelp_verb = "���ڤ�����Ω�Ƥ�";
	    break;
	case MS_WAIL:
/*JP	    yelp_verb = "wail";*/
	    yelp_verb = "�ᤷ���Ĥ���";
	    break;
    }
/*JP    if (yelp_verb) pline("%s %ss!", Monnam(mtmp), yelp_verb);*/
    if (yelp_verb) pline("%s��%s��", Monnam(mtmp), yelp_verb);
}

/* the sounds of distressed pets */
void
whimper(mtmp)
register struct monst *mtmp;
{
    register const char *whimper_verb = 0;

    if (mtmp->msleep || !mtmp->mcanmove || !mtmp->data->msound) return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        whimper_verb = h_sounds[rn2(SIZE(h_sounds))];
    else switch (mtmp->data->msound) {
	case MS_MEW:
	case MS_GROWL:
/*JP	    whimper_verb = "whimper";*/
	    whimper_verb = "���󥯥��Ĥ���";
	    break;
	case MS_BARK:
/*JP	    whimper_verb = "whine";*/
	    whimper_verb = "���󥯥��Ĥ���";
	    break;
	case MS_SQEEK:
/*JP	    whimper_verb = "squeal";*/
	    whimper_verb = "���������Ĥ���";
	    break;
    }
/*JP    if (whimper_verb) pline("%s %ss.", Monnam(mtmp), whimper_verb);*/
    if (whimper_verb) pline("%s��%s��", Monnam(mtmp), whimper_verb);
}

/* pet makes "I'm hungry" noises */
void
beg(mtmp)
register struct monst *mtmp;
{
    if (mtmp->msleep || !mtmp->mcanmove ||
	!(carnivorous(mtmp->data) || herbivorous(mtmp->data))) return;
    /* presumably nearness and soundok checks have already been made */
    if (mtmp->data->msound != MS_SILENT && mtmp->data->msound <= MS_ANIMAL)
	(void) domonnoise(mtmp);
    else if (mtmp->data->msound >= MS_HUMANOID)
	verbalize("�Ϥ�ڤ����补");
}

#endif /* OVLB */

#endif /* SOUNDS */

#ifdef OVLB

static int
domonnoise(mtmp)
register struct monst *mtmp;
{
#ifdef SOUNDS
    register const char *pline_msg = 0;	/* Monnam(mtmp) will be prepended */
#endif

    /* presumably nearness and sleep checks have already been made */
    if (!flags.soundok) return(0);

    switch (mtmp->data->msound) {
	case MS_ORACLE:
	    return doconsult(mtmp);
	case MS_PRIEST:
	    priest_talk(mtmp);
	    break;
#ifdef MULDGN
	case MS_LEADER:
	case MS_NEMESIS:
	case MS_GUARDIAN:
	    quest_chat(mtmp);
	    break;
#endif
#ifdef SOUNDS
	case MS_SELL: /* pitch, pay, total */
	    shk_chat(mtmp);
	    break;
	case MS_SILENT:
	    break;
	case MS_BARK:
	    if (flags.moonphase == FULL_MOON && night()) {
/*JP		pline_msg = "howls.";*/
		pline_msg = "�ʤ�����";
	    } else if (mtmp->mpeaceful) {
		if (mtmp->mtame &&
		    (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
		     moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
/*JP		    pline_msg = "whines.";*/
		    pline_msg = "���󥯥��Ĥ�����";
		else if (EDOG(mtmp)->hungrytime > moves + 1000)
/*JP		    pline_msg = "yips.";*/
		    pline_msg = "����󥭥���Ĥ�����";
		else
/*JP		    pline_msg = "barks.";*/
		    pline_msg = "������ʤ�����";
	    } else {
/*JP		pline_msg = "growls.";*/
		pline_msg = "�㤷���ʤ�����";
	    }
	    break;
	case MS_MEW:
	    if (mtmp->mtame) {
		if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
		    mtmp->mtame < 5)
/*JP		    pline_msg = "yowls.";*/
		    pline_msg = "�ᤷ���Ĥ�����";
		else if (moves > EDOG(mtmp)->hungrytime)
/*JP		    pline_msg = "miaos.";*/
		    pline_msg = "�˥㡼����Ĥ�����";
		else if (EDOG(mtmp)->hungrytime > moves + 1000)
/*JP		    pline_msg = "purrs.";*/
		    pline_msg = "��������Ĥ�����";
		else
/*JP		    pline_msg = "mews.";*/
		    pline_msg = "�˥㡼�˥㡼�Ĥ�����";
		break;
	    } /* else FALLTHRU */
	case MS_GROWL:
/*JP	    pline_msg = mtmp->mpeaceful ? "snarls." : "growls!";*/
	    pline_msg = mtmp->mpeaceful ? "���ʤä���" : "�㤷���ʤ�����";
	    break;
	case MS_ROAR:
/*JP	    pline_msg = mtmp->mpeaceful ? "snarls." : "roars!";*/
	    pline_msg = mtmp->mpeaceful ? "���ʤä���" : "�ȤƤ�㤷���ʤ�����";
	    break;
	case MS_SQEEK:
/*JP	    pline_msg = "squeaks.";*/
	    pline_msg = "���������Ĥ�����";
	    break;
	case MS_SQAWK:
/*JP	    pline_msg = "squawks.";*/
	    pline_msg = "���������Ĥ�����";
	    break;
	case MS_HISS:
	    if (!mtmp->mpeaceful)
/*JP		pline_msg = "hisses!";*/
		pline_msg = "�����ä��Ĥ�����";
	    else return 0;	/* no sound */
	    break;
	case MS_BUZZ:
/*JP	    pline_msg = mtmp->mpeaceful ? "drones." : "buzzes angrily.";*/
	    pline_msg = mtmp->mpeaceful ? "�֡�����Ĥä���" : "�֤�֤��Ĥä���";
	    break;
	case MS_GRUNT:
/*JP	    pline_msg = "grunts.";*/
	    pline_msg = "�֡��֡��Ĥ�����";
	    break;
	case MS_NEIGH:
	    if (mtmp->mtame < 5)
/*JP		pline_msg = "neighs.";*/
	        pline_msg = "���ʤʤ�����";
	    else if (moves > EDOG(mtmp)->hungrytime)
/*JP		pline_msg = "whinnies.";*/
		pline_msg = "�ҥҡ�����Ĥ�����";
	    else
/*JP		pline_msg = "whickers.";*/
		pline_msg = "�ҥҥҡ�����Ĥ�����";
	    break;
	case MS_WAIL:
/*JP	    pline_msg = "wails mournfully.";*/
	    pline_msg = "�ᤷ�����Ĥ�����";
	    break;
	case MS_GURGLE:
/*JP	    pline_msg = "gurgles.";*/
	    pline_msg = "���������Ĥ餷����";
	    break;
	case MS_BURBLE:
/*JP	    pline_msg = "burbles.";*/
	    pline_msg = "�ڤ��㤯���㤷��٤ä���";
	    break;
	case MS_SHRIEK:
/*JP	    pline_msg = "shrieks.";*/
	    pline_msg = "���ڤ����򤢤�����";
	    aggravate();
	    break;
	case MS_IMITATE:
/*JP	    pline_msg = "imitates you.";*/
	    pline_msg = "���ʤ��ο����򤷤���";
	    break;
	case MS_BONES:
/*JP	    pline("%s rattles noisily.", Monnam(mtmp));
	    You("freeze for a moment.");*/
	    pline("%s�ϥ���������������������",Monnam(mtmp));
	    You("���Ф餯ư���ʤ���");
	    nomul(-2);
	    break;
	case MS_LAUGH:
	    {
		static const char *laugh_msg[4] = {
/*JP		    "giggles.", "chuckles.", "snickers.", "laughs.",*/
		    "���������Фä���", "�����ä��ȾФä���",
		    "�Ф��ˤ����褦�˾Фä���", "�Фä���",
		};
		pline_msg = laugh_msg[rn2(4)];
	    }
	    break;
	case MS_MUMBLE:
/*JP	    pline_msg = "mumbles incomprehensibly.";*/
	    pline_msg = "�ԲĲ�ʸ��դ�Ĥ֤䤤����";
	    break;
	case MS_DJINNI:
/*JP	    if (mtmp->mtame) verbalize("Thank you for freeing me!");
	    else if (mtmp->mpeaceful) verbalize("I'm free!");
	    else verbalize("This will teach you not to disturb me!");*/
	    if (mtmp->mtame) verbalize("�������Ƥ��줿���Ȥ򴶼դ��롪");
	    else if (mtmp->mpeaceful) verbalize("��äȼ�ͳ�ˤʤä���");
	    else verbalize("����ޤ򤷤ʤ��Ǥ��졪");
	    break;
	case MS_HUMANOID:
	    if (!mtmp->mpeaceful) {
		if (In_endgame(&u.uz) && is_mplayer(mtmp->data)) {
		    mplayer_talk(mtmp);
		    break;
		} else {
		    return 0;	/* no sound */
		}
	    }
	    /* Generic peaceful humanoid behaviour. */
	    if (mtmp->mflee)
/*JP		pline_msg = "wants nothing to do with you.";*/
		pline_msg = "���ʤ��ȴط�����������ʤ��褦����";
	    else if (mtmp->mhp < mtmp->mhpmax/4)
/*JP		pline_msg = "moans.";*/
		pline_msg = "���ᤤ����";
	    else if (mtmp->mconf || mtmp->mstun)
/*JP		verbalize(!rn2(3) ? "Huh?" : rn2(2) ? "What?" : "Eh?");*/
		verbalize(!rn2(3) ? "�ء�" : rn2(2) ? "�ʤˡ�" : "����");
	    else if (!mtmp->mcansee)
/*JP		verbalize("I can't see!");*/
		verbalize("���⸫���ʤ���");
	    else if (mtmp->mtrapped)
/*JP		verbalize("I'm trapped!");*/
		verbalize("櫤ˤϤޤä����ޤä���");
	    else if (mtmp->mhp < mtmp->mhpmax/2)
/*JP		pline_msg = "asks for a potion of healing.";*/
		pline_msg = "������������äƤʤ����Ҥͤ�";
	    else if (mtmp->mtame && moves > EDOG(mtmp)->hungrytime)
/*JP		verbalize("I'm hungry.");*/
		verbalize("ʢ�����ä��ʡ�");
	    /* Specific monster's interests */
	    else if (is_elf(mtmp->data))
/*JP		pline_msg = "curses orcs.";*/
		pline_msg = "����������ä���";
	    else if (is_dwarf(mtmp->data))
/*JP		pline_msg = "talks about mining.";*/
		pline_msg = "�η��ˤĤ����ä�����";
	    else if (likes_magic(mtmp->data))
/*JP		pline_msg = "talks about spellcraft.";*/
		pline_msg = "���ϤˤĤ����ä�����";
	    else if (carnivorous(mtmp->data))
/*JP		pline_msg = "discusses hunting.";*/
		pline_msg = "�ĤˤĤ��Ƶ���������";
	    else switch (monsndx(mtmp->data)) {
		case PM_HOBBIT:
		    pline_msg = (mtmp->mhpmax - mtmp->mhp >= 10) ?
/*JP
				"complains about unpleasant dungeon conditions."
				: "asks you about the One Ring.";
*/
				"���������µܤξ��֤ˤĤ���������Ҥ٤���"
				: "�Ȥ�����ؤˤĤ��ƿҤͤ���";
		    break;
		case PM_ARCHEOLOGIST:
/*JP    pline_msg = "describes a recent article in \"Spelunker Today\" magazine.";*/
    pline_msg = "������ƶ���פκǿ��ε�����ɮ���Ƥ��롥";
		    break;
# ifdef TOURIST
		case PM_TOURIST:
/*JP		    verbalize("Aloha.");*/
		    verbalize("�����ϡ�");
		    break;
# endif
		default:
/*JP		    pline_msg = "discusses dungeon exploration.";*/
		    pline_msg = "�µ�õ���ˤĤ��Ƶ���������";
	    }
	    break;
	case MS_SEDUCE:
# ifdef SEDUCE
	    if (mtmp->data->mlet != S_NYMPH &&
		could_seduce(mtmp, &youmonst, (struct attack *)0) == 1) {
			(void) doseduce(mtmp);
			break;
	    }
	    switch ((poly_gender() != mtmp->female) ? rn2(3) : 0) {
# else
	    switch ((poly_gender() == 0) ? rn2(3) : 0) {
# endif
		case 2:
/*JP			verbalize("Hello, sailor.");*/
			verbalize("����ˤ��ϡ���������");
			break;
		case 1:
/*JP			pline_msg = "comes on to you.";*/
			pline_msg = "���ʤ��Τۤ��ؤ�äƤ�����";
			break;
		default:
/*JP			pline_msg = "cajoles you.";*/
			pline_msg = "���ʤ��򤪤��Ƥ���";
	    }
	    break;
# ifdef KOPS
	case MS_ARREST:
	    if (mtmp->mpeaceful)
/*JP		verbalize("Just the facts, %s.",
		      flags.female ? "Ma'am" : "Sir");*/
		verbalize("���줬���¤��� %s��",
		      flags.female ? "�����" : "����");
	    else {
		static const char *arrest_msg[3] = {
/*JP		    "Anything you say can be used against you.",
		    "You're under arrest!",
		    "Stop in the name of the Law!",*/
		    "���ޤ��θ������ȤϤ��ޤ��ˤȤä������Ȥʤ뤾��",
		    "���ޤ������᤹�롪",
		    "ˡ��̾�Τ��ľ������ߤ��衪",
		};
		verbalize(arrest_msg[rn2(3)]);
	    }
	    break;
# endif
	case MS_BRIBE:
	    if (mtmp->mpeaceful && !mtmp->mtame) {
		(void) demon_talk(mtmp);
		break;
	    }
	    /* fall through */
	case MS_CUSS:
	    if (!mtmp->mpeaceful)
		cuss(mtmp);
	    break;
	case MS_NURSE:
	    if (uwep)
/*JP		verbalize("Put that weapon away before you hurt someone!");*/
		verbalize("���򤪤���ʤ���������Ͽͤ���Ĥ����Τ衪");
	    else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
		if (pl_character[0] == 'H')
/*JP		    verbalize("Doc, I can't help you unless you cooperate.");*/
		    verbalize("���������ʤ��ζ��Ϥʤ��ǤϤɤ����褦�⤢��ޤ���");
		else
/*JP		    verbalize("Please undress so I can examine you.");*/
		    verbalize("����æ���Ǥ������������ʤ���ǻ����ޤ��");
# ifdef TOURIST
	    else if (uarmu)
/*JP		verbalize("Take off your shirt, please.");*/
		verbalize("����Ĥ�æ���Ǥ���������");
# endif
/*JP	    else verbalize("Relax, this won't hurt a bit.");*/
	    else verbalize("�����Ĥ��ơ����äȤ��ˤ��ʤ���补");
	    break;
	case MS_GUARD:
	    if (u.ugold)
/*JP		verbalize("Please drop that gold and follow me.");*/
		verbalize("����֤��ƤĤ��Ƥ�����");
	    else
/*JP		verbalize("Please follow me.");*/
		verbalize("�Ĥ��Ƥ�����");
	    break;
	case MS_SOLDIER:
	    {
		static const char *soldier_foe_msg[3] = {
/*JP		    "Resistance is useless!",
		    "You're dog meat!",
		    "Surrender!",*/
		    "�񹳤�̵�Ѥ���",
		    "���ޤ��ʤ󤫸��Ѥ�������",
		    "��������",
		},		  *soldier_pax_msg[3] = {
/*JP		    "What lousy pay we're getting here!",
		    "The food's not fit for Orcs!",
		    "My feet hurt, I've been on them all day!",*/
		    "���äפ����֤��Ƥ椱��",
		    "���ο��٤�Τϥ������ˤ������",
		    "­����椷�����ɤ����Ƥ���롪",
		};
		verbalize(mtmp->mpeaceful ? soldier_pax_msg[rn2(3)]
					  : soldier_foe_msg[rn2(3)]);
	    }
	    break;
	case MS_RIDER:
	    if (mtmp->data == &mons[PM_DEATH] && mtmp->mpeaceful)
/*JP		pline_msg = "is busy reading a copy of Sandman #9.";
	    else verbalize("Who do you think you are, War?");*/
		pline_msg = "Sandman��9�ϤΥ��ԡ����ɤ�Τ�˻����";
	    else verbalize("��ʬ����ʪ���ͤ������Ȥ����뤫��");
	    break;
#endif /* SOUNDS */
    }

#ifdef SOUNDS
    if (pline_msg) pline("%s��%s", Monnam(mtmp), pline_msg);
#endif
    return(1);
}


int
dotalk()
{
    int result;
    boolean save_soundok = flags.soundok;
    flags.soundok = 1;	/* always allow sounds while chatting */
    result = dochat();
    flags.soundok = save_soundok;
    return result;
}

static int
dochat()
{
    register struct monst *mtmp;
    register int tx,ty;
    struct obj *otmp;

#ifdef POLYSELF
    if (uasmon->msound == MS_SILENT) {
/*JP	pline("As %s, you cannot speak.", an(uasmon->mname));*/
	pline("���ʤ���%s�ʤΤǡ��ä����Ȥ��Ǥ��ʤ���", an(uasmon->mname));
	return(0);
    }
#endif
    if (Strangled) {
/*JP	You("can't speak.  You're choking!");*/
	You("�ä��ʤ������ʤ��ϼ��ʤ���Ƥ��롪");
	return(0);
    }
    if (u.uswallow) {
/*	pline("They won't hear you out there.");*/
        You("���ظ��ä��ä򤷤�����ï��ʹ������ʤ��ä���");
	return(0);
    }
    if (Underwater) {
/*	pline("Your speech is unintelligible underwater.");*/
	pline("���̲��Ǥϡ����ʤ����äϤ�������Ǥ��ʤ���");
	return(0);
    }

    if (!Blind && (otmp = shop_object(u.ux, u.uy)) != (struct obj *)0) {
	/* standing on something in a shop and chatting causes the shopkeeper
	   to describe the price(s).  This can inhibit other chatting inside
	   a shop, but that shouldn't matter much.  shop_object() returns an
	   object iff inside a shop and the shopkeeper is present and willing
	   (not angry) and able (not asleep) to speak and the position contains
	   any objects other than just gold.
	*/
	price_quote(otmp);
	return(1);
    }

/*JP    (void) getdir("Talk to whom? [in what direction]");*/
    (void) getdir("ï���ä��ޤ�����[�ɤ�����]");

    if (u.dz) {
/*JP	pline("They won't hear you %s there.", u.dz < 0 ? "up" : "down");*/
	pline("%s���ä��ä򤷤Ƥ��̣���ʤ���",
	      u.dz < 0 ? "���" : "����");
	return(0);
    }

    if (u.dx == 0 && u.dy == 0) {
/*
 * Let's not include this.  It raises all sorts of questions: can you wear
 * 2 helmets, 2 amulets, 3 pairs of gloves or 6 rings as a marilith,
 * etc...  --KAA
#ifdef POLYSELF
	if (u.umonnum == PM_ETTIN) {
 *JP	    You("discover that your other head makes boring conversation.");*
	    You("¾��Ƭ�����ä򤷤Ƥ���Τ˵����Ĥ�����");
	    return(1);
	}
#endif
*/
/*JP	pline("Talking to yourself is a bad habit for a dungeoneer.");*/
	pline("�µ�õ���ԤˤȤäư�͸��ϰ����ʤ���");
	return(0);
    }

    tx = u.ux+u.dx; ty = u.uy+u.dy;
    mtmp = m_at(tx, ty);
    if ((Blind && !Telepat) || !mtmp || mtmp->mundetected ||
		mtmp->m_ap_type == M_AP_FURNITURE ||
		mtmp->m_ap_type == M_AP_OBJECT) {
/*JP	pline("I see nobody there.");*/
	pline("�����ˤϤʤˤ�ʤ���");
	return(0);
    }
    if (!mtmp->mcanmove || mtmp->msleep) {
/*JP	pline("%s seems not to notice you.", Monnam(mtmp));*/
	pline("%s�Ϥ��ʤ��˵����Ĥ��Ƥʤ��褦����",Monnam(mtmp));
	return(0);
    }

    if (mtmp->mtame && mtmp->meating) {
/*JP	pline("%s is eating noisily.", Monnam(mtmp));*/
	pline("%s�ϥХ�Х��ʪ�򿩤٤Ƥ��롥", Monnam(mtmp));
	return (0);
    }

    return domonnoise(mtmp);
}

#endif /* OVLB */

/*sounds.c*/
