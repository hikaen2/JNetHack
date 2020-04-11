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
/*JP	        "水が泡立つ音が聞いた．",
		"金貨の上に水が落ちる音が聞いた．",
		"水の精が水をはねる音が聞いた．",
		"炭酸泉の音が聞いた．",*/
	        "ゴボゴボと言う音を聞いた．",
		"ピチャピチャと言う音を聞いた．",
		"バシャバシャと言う音を聞いた．",
		"シューと言う音を聞いた．"
	};
	You(fountain_msg[rn2(3)+hallu]);
    }
#ifdef SINK
    if (level.flags.nsinks && !rn2(300)) {
	static const char *sink_msg[3] = {
/*JP		"hear a slow drip.",
		"hear a gurgling noise.",
		"hear dishes being washed!",*/
	        "水がぽたぽたと落ちる音を聞いた．",
		"がらがらと言う音を聞いた．",
		"皿を洗う音を聞いた．",
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
	        "上品な話し声を聞いた．",
		"裁判で笏を突く音を聞いた．",
		"だれかが「そのものの首を跳ねよ」と叫ぶ声を聞いた．",
                "ベルサイユ女王の猫の声を聞いた．",
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
		"蚊の羽音を聞いた．",
		"腐った匂いがした！",	/* so it's a smell...*/
		"ドナルドダックの声を聞いた！",
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
			    You(!hallu ? "誰かがお金を数えている音を聞いた．":
				"クォータバックが指示をする声を聞いた．");
			else
/*JP			    You("hear someone searching.");*/
			    You("誰かが探索している音を聞いた．");
			break;
		    }
		    /* fall into... (yes, even for hallucination) */
		}
		case 0:
/*JP		    You("hear the footsteps of a guard on patrol.");*/
		    You("警備員のパトロールする音を聞いた．");
		    break;
		case 2:
/*JP		    You("hear Ebenezer Scrooge!");*/
		    You("エベネザー・スクルージの声を聞いた！");
		    break;
	    }
	return;
    }
    if (level.flags.has_beehive && !rn2(200)) {
	switch (rn2(2)+hallu) {
	    case 0:
/*JP		You("hear a low buzzing.");*/
	        You("ぶーんと言う音を聞いた．");
		break;
	    case 1:
/*JP		You("hear an angry drone.");*/
		You("興奮した雄バチの音を聞いた．");
		break;
	    case 2:
/*JP		You("hear bees in your %sbonnet!",
		    uarmh ? "" : "(nonexistent) ");*/
		You("ハチがあなたの%s帽子の中にいる音を聞いた！",
		    uarmh ? "" : "(存在しない)");
		break;
	}
	return;
    }
    if (level.flags.has_morgue && !rn2(200)) {
	switch (rn2(2)+hallu) {
	    case 0:
/*JP		You("suddenly realize it is unnaturally quiet.");*/
	        You("不自然なくらい静かなのに突然気がついた．");
		break;
	    case 1:
/*JP		pline("The hair on the back of your %s stands up.",*/
		pline("あなたの%sのうしろの毛が逆だった．",
			body_part(NECK));
		break;
	    case 2:
/*JP		pline("The hair on your %s seems to stand up.",*/
		pline("あなたの%sの毛は逆だった．",
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
	        "刃物を研ぐ音を聞いた．",
		"大きないびきを聞いた．",
		"ダイスが振られる音を聞いた．",
		"マッカーサー提督の声を聞いた．",
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
	        "象がピーナッツの上で踊るような音を聞いた．",
		"アシカが吠えるような音を聞いた．",
		"ドリトル先生の声を聞いた．",
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
		    "誰かが泥棒をののしる声を聞いた．",
		    "レジのチーンと言う音を聞いた．",
		    "ネイマンとマルクスの議論を聞いた！",
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
    "ピーッと鳴いた","騒ぎたてた","歌った","キーキーと鳴いた",
    "せき込んだ","ゴロゴロ鳴った","ホーホー鳴いた","ポンと鳴いた",
    "ガランガランと鳴いた","クンクン鳴いた","チリンチリンと鳴いた",
    "イーッと鳴いた"
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
	    growl_verb = "シーッと鳴いた";	/* hisseS */
	    break;
	case MS_BARK:
	case MS_GROWL:
/*JP	    growl_verb = "growl";*/
	    growl_verb = "はげしく吠えた";
	    break;
	case MS_ROAR:
/*JP	    growl_verb = "roar";*/
	    growl_verb = "吠えた";
	    break;
	case MS_BUZZ:
/*JP        growl_verb = "buzze";*/
	    growl_verb = "ブーッと鳴いた";
	    break;
	case MS_SQEEK:
/*JP        growl_verb = "squeal";*/
	    growl_verb = "キーキー鳴いた";
	    break;
	case MS_SQAWK:
/*JP	    growl_verb = "screeche";*/
	    growl_verb = "金切り声を立てた";
	    break;
	case MS_NEIGH:
/*JP	    growl_verb = "neigh";*/
	    growl_verb = "いなないた";
	    break;
	case MS_WAIL:
/*JP	    growl_verb = "wail";*/
	    growl_verb = "悲しく鳴いた";
	    break;
    }
/*JP    if (growl_verb) pline("%s %ss!", Monnam(mtmp), growl_verb);*/
    if (growl_verb) pline("%sは%s！", Monnam(mtmp), growl_verb);
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
	    yelp_verb = "悲しく鳴いた";
	    break;
	case MS_BARK:
	case MS_GROWL:
/*JP	    yelp_verb = "yelp";*/
	    yelp_verb = "キャンキャン鳴いた";
	    break;
	case MS_ROAR:
/*JP	    yelp_verb = "snarl";*/
	    yelp_verb = "うなった";
	    break;
	case MS_SQEEK:
/*JP	    yelp_verb = "squeal";*/
	    yelp_verb = "キーキー鳴いた";
	    break;
	case MS_SQAWK:
/*JP	    yelp_verb = "screak";*/
	    yelp_verb = "金切り声を立てた";
	    break;
	case MS_WAIL:
/*JP	    yelp_verb = "wail";*/
	    yelp_verb = "悲しく鳴いた";
	    break;
    }
/*JP    if (yelp_verb) pline("%s %ss!", Monnam(mtmp), yelp_verb);*/
    if (yelp_verb) pline("%sは%s！", Monnam(mtmp), yelp_verb);
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
	    whimper_verb = "クンクン鳴いた";
	    break;
	case MS_BARK:
/*JP	    whimper_verb = "whine";*/
	    whimper_verb = "クンクン鳴いた";
	    break;
	case MS_SQEEK:
/*JP	    whimper_verb = "squeal";*/
	    whimper_verb = "キーキー鳴いた";
	    break;
    }
/*JP    if (whimper_verb) pline("%s %ss.", Monnam(mtmp), whimper_verb);*/
    if (whimper_verb) pline("%sは%s．", Monnam(mtmp), whimper_verb);
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
	verbalize("はらぺこだよ．");
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
		pline_msg = "吠えた．";
	    } else if (mtmp->mpeaceful) {
		if (mtmp->mtame &&
		    (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
		     moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
/*JP		    pline_msg = "whines.";*/
		    pline_msg = "クンクン鳴いた．";
		else if (EDOG(mtmp)->hungrytime > moves + 1000)
/*JP		    pline_msg = "yips.";*/
		    pline_msg = "キャンキャン鳴いた．";
		else
/*JP		    pline_msg = "barks.";*/
		    pline_msg = "ワンワン吠えた．";
	    } else {
/*JP		pline_msg = "growls.";*/
		pline_msg = "激しく吠えた．";
	    }
	    break;
	case MS_MEW:
	    if (mtmp->mtame) {
		if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped ||
		    mtmp->mtame < 5)
/*JP		    pline_msg = "yowls.";*/
		    pline_msg = "悲しく鳴いた．";
		else if (moves > EDOG(mtmp)->hungrytime)
/*JP		    pline_msg = "miaos.";*/
		    pline_msg = "ニャーンと鳴いた．";
		else if (EDOG(mtmp)->hungrytime > moves + 1000)
/*JP		    pline_msg = "purrs.";*/
		    pline_msg = "ゴロゴロと鳴いた．";
		else
/*JP		    pline_msg = "mews.";*/
		    pline_msg = "ニャーニャー鳴いた．";
		break;
	    } /* else FALLTHRU */
	case MS_GROWL:
/*JP	    pline_msg = mtmp->mpeaceful ? "snarls." : "growls!";*/
	    pline_msg = mtmp->mpeaceful ? "うなった．" : "激しく吠えた．";
	    break;
	case MS_ROAR:
/*JP	    pline_msg = mtmp->mpeaceful ? "snarls." : "roars!";*/
	    pline_msg = mtmp->mpeaceful ? "うなった．" : "とても激しく吠えた．";
	    break;
	case MS_SQEEK:
/*JP	    pline_msg = "squeaks.";*/
	    pline_msg = "キーキー鳴いた．";
	    break;
	case MS_SQAWK:
/*JP	    pline_msg = "squawks.";*/
	    pline_msg = "キーキー鳴いた．";
	    break;
	case MS_HISS:
	    if (!mtmp->mpeaceful)
/*JP		pline_msg = "hisses!";*/
		pline_msg = "シーッと鳴いた！";
	    else return 0;	/* no sound */
	    break;
	case MS_BUZZ:
/*JP	    pline_msg = mtmp->mpeaceful ? "drones." : "buzzes angrily.";*/
	    pline_msg = mtmp->mpeaceful ? "ぶーんと鳴った．" : "ぶんぶん鳴った．";
	    break;
	case MS_GRUNT:
/*JP	    pline_msg = "grunts.";*/
	    pline_msg = "ぶーぶー鳴いた．";
	    break;
	case MS_NEIGH:
	    if (mtmp->mtame < 5)
/*JP		pline_msg = "neighs.";*/
	        pline_msg = "いなないた．";
	    else if (moves > EDOG(mtmp)->hungrytime)
/*JP		pline_msg = "whinnies.";*/
		pline_msg = "ヒヒーンと鳴いた．";
	    else
/*JP		pline_msg = "whickers.";*/
		pline_msg = "ヒヒヒーンと鳴いた．";
	    break;
	case MS_WAIL:
/*JP	    pline_msg = "wails mournfully.";*/
	    pline_msg = "悲しげに鳴いた．";
	    break;
	case MS_GURGLE:
/*JP	    pline_msg = "gurgles.";*/
	    pline_msg = "ごろごろ喉を鳴らした．";
	    break;
	case MS_BURBLE:
/*JP	    pline_msg = "burbles.";*/
	    pline_msg = "ぺちゃくちゃしゃべった．";
	    break;
	case MS_SHRIEK:
/*JP	    pline_msg = "shrieks.";*/
	    pline_msg = "金切り声をあげた．";
	    aggravate();
	    break;
	case MS_IMITATE:
/*JP	    pline_msg = "imitates you.";*/
	    pline_msg = "あなたの真似をした．";
	    break;
	case MS_BONES:
/*JP	    pline("%s rattles noisily.", Monnam(mtmp));
	    You("freeze for a moment.");*/
	    pline("%sはガタガタと騒ぎだした．",Monnam(mtmp));
	    You("しばらく動けない．");
	    nomul(-2);
	    break;
	case MS_LAUGH:
	    {
		static const char *laugh_msg[4] = {
/*JP		    "giggles.", "chuckles.", "snickers.", "laughs.",*/
		    "くすくす笑った．", "くすっすと笑った．",
		    "ばかにしたように笑った．", "笑った．",
		};
		pline_msg = laugh_msg[rn2(4)];
	    }
	    break;
	case MS_MUMBLE:
/*JP	    pline_msg = "mumbles incomprehensibly.";*/
	    pline_msg = "不可解な言葉をつぶやいた．";
	    break;
	case MS_DJINNI:
/*JP	    if (mtmp->mtame) verbalize("Thank you for freeing me!");
	    else if (mtmp->mpeaceful) verbalize("I'm free!");
	    else verbalize("This will teach you not to disturb me!");*/
	    if (mtmp->mtame) verbalize("私を助けてくれたことを感謝する！");
	    else if (mtmp->mpeaceful) verbalize("やっと自由になった！");
	    else verbalize("じゃまをしないでくれ！");
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
		pline_msg = "あなたと関係を持ちたくないようだ．";
	    else if (mtmp->mhp < mtmp->mhpmax/4)
/*JP		pline_msg = "moans.";*/
		pline_msg = "うめいた．";
	    else if (mtmp->mconf || mtmp->mstun)
/*JP		verbalize(!rn2(3) ? "Huh?" : rn2(2) ? "What?" : "Eh?");*/
		verbalize(!rn2(3) ? "へ？" : rn2(2) ? "なに？" : "え？");
	    else if (!mtmp->mcansee)
/*JP		verbalize("I can't see!");*/
		verbalize("何も見えない！");
	    else if (mtmp->mtrapped)
/*JP		verbalize("I'm trapped!");*/
		verbalize("罠にはまったしまった！");
	    else if (mtmp->mhp < mtmp->mhpmax/2)
/*JP		pline_msg = "asks for a potion of healing.";*/
		pline_msg = "回復の薬を持ってないか尋ねた";
	    else if (mtmp->mtame && moves > EDOG(mtmp)->hungrytime)
/*JP		verbalize("I'm hungry.");*/
		verbalize("腹が減ったな．");
	    /* Specific monster's interests */
	    else if (is_elf(mtmp->data))
/*JP		pline_msg = "curses orcs.";*/
		pline_msg = "オークを呪った．";
	    else if (is_dwarf(mtmp->data))
/*JP		pline_msg = "talks about mining.";*/
		pline_msg = "採掘について話した．";
	    else if (likes_magic(mtmp->data))
/*JP		pline_msg = "talks about spellcraft.";*/
		pline_msg = "魔力について話した．";
	    else if (carnivorous(mtmp->data))
/*JP		pline_msg = "discusses hunting.";*/
		pline_msg = "猟について議論した．";
	    else switch (monsndx(mtmp->data)) {
		case PM_HOBBIT:
		    pline_msg = (mtmp->mhpmax - mtmp->mhp >= 10) ?
/*JP
				"complains about unpleasant dungeon conditions."
				: "asks you about the One Ring.";
*/
				"不愉快な迷宮の状態について不満を述べた．"
				: "とある指輪について尋ねた．";
		    break;
		case PM_ARCHEOLOGIST:
/*JP    pline_msg = "describes a recent article in \"Spelunker Today\" magazine.";*/
    pline_msg = "「日刊洞窟」の最新の記事を執筆している．";
		    break;
# ifdef TOURIST
		case PM_TOURIST:
/*JP		    verbalize("Aloha.");*/
		    verbalize("アローハ．");
		    break;
# endif
		default:
/*JP		    pline_msg = "discusses dungeon exploration.";*/
		    pline_msg = "迷宮探検について議論した．";
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
			verbalize("こんにちは，船員さん．");
			break;
		case 1:
/*JP			pline_msg = "comes on to you.";*/
			pline_msg = "あなたのほうへやってきた．";
			break;
		default:
/*JP			pline_msg = "cajoles you.";*/
			pline_msg = "あなたをおだてた．";
	    }
	    break;
# ifdef KOPS
	case MS_ARREST:
	    if (mtmp->mpeaceful)
/*JP		verbalize("Just the facts, %s.",
		      flags.female ? "Ma'am" : "Sir");*/
		verbalize("これが事実だよ %s．",
		      flags.female ? "お嬢さん" : "きみ");
	    else {
		static const char *arrest_msg[3] = {
/*JP		    "Anything you say can be used against you.",
		    "You're under arrest!",
		    "Stop in the name of the Law!",*/
		    "おまえの言うことはおまえにとって不利となるぞ！",
		    "おまえを逮捕する！",
		    "法の名のもと直ちに中止せよ！",
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
		verbalize("武器をおさめなさい！それは人を傷つけるものよ！");
	    else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
		if (pl_character[0] == 'H')
/*JP		    verbalize("Doc, I can't help you unless you cooperate.");*/
		    verbalize("先生，あなたの協力なしではどうしようもありませんわ．");
		else
/*JP		    verbalize("Please undress so I can examine you.");*/
		    verbalize("服を脱いでください．あなたを診察しますわ．");
# ifdef TOURIST
	    else if (uarmu)
/*JP		verbalize("Take off your shirt, please.");*/
		verbalize("シャツを脱いでください．");
# endif
/*JP	    else verbalize("Relax, this won't hurt a bit.");*/
	    else verbalize("おちついて．ちっとも痛くないわよ．");
	    break;
	case MS_GUARD:
	    if (u.ugold)
/*JP		verbalize("Please drop that gold and follow me.");*/
		verbalize("金を置いてついてこい．");
	    else
/*JP		verbalize("Please follow me.");*/
		verbalize("ついてこい．");
	    break;
	case MS_SOLDIER:
	    {
		static const char *soldier_foe_msg[3] = {
/*JP		    "Resistance is useless!",
		    "You're dog meat!",
		    "Surrender!",*/
		    "抵抗は無用だ！",
		    "おまえなんか犬用の肉さ！",
		    "降伏しろ！",
		},		  *soldier_pax_msg[3] = {
/*JP		    "What lousy pay we're getting here!",
		    "The food's not fit for Orcs!",
		    "My feet hurt, I've been on them all day!",*/
		    "たっぷり金を置いてゆけ！",
		    "この食べものはオークにゃむりだ！",
		    "足を怪我した，どうしてくれる！",
		};
		verbalize(mtmp->mpeaceful ? soldier_pax_msg[rn2(3)]
					  : soldier_foe_msg[rn2(3)]);
	    }
	    break;
	case MS_RIDER:
	    if (mtmp->data == &mons[PM_DEATH] && mtmp->mpeaceful)
/*JP		pline_msg = "is busy reading a copy of Sandman #9.";
	    else verbalize("Who do you think you are, War?");*/
		pline_msg = "Sandmanの9章のコピーを読むのに忙しい";
	    else verbalize("自分が何物か考えたことがあるか？");
	    break;
#endif /* SOUNDS */
    }

#ifdef SOUNDS
    if (pline_msg) pline("%sは%s", Monnam(mtmp), pline_msg);
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
	pline("あなたは%sなので，話すことができない．", an(uasmon->mname));
	return(0);
    }
#endif
    if (Strangled) {
/*JP	You("can't speak.  You're choking!");*/
	You("話せない．あなたは首を絞められている！");
	return(0);
    }
    if (u.uswallow) {
/*	pline("They won't hear you out there.");*/
        You("外へ向って話をしたが，誰も聞きいれなかった．");
	return(0);
    }
    if (Underwater) {
/*	pline("Your speech is unintelligible underwater.");*/
	pline("水面下では，あなたの話はろくに理解できない．");
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
    (void) getdir("誰と話しますか？[どの方向]");

    if (u.dz) {
/*JP	pline("They won't hear you %s there.", u.dz < 0 ? "up" : "down");*/
	pline("%s向って話をしても意味がない．",
	      u.dz < 0 ? "上へ" : "下へ");
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
	    You("他の頭が会話をしているのに気がついた．");
	    return(1);
	}
#endif
*/
/*JP	pline("Talking to yourself is a bad habit for a dungeoneer.");*/
	pline("迷宮探検者にとって一人言は悪い癖だ．");
	return(0);
    }

    tx = u.ux+u.dx; ty = u.uy+u.dy;
    mtmp = m_at(tx, ty);
    if ((Blind && !Telepat) || !mtmp || mtmp->mundetected ||
		mtmp->m_ap_type == M_AP_FURNITURE ||
		mtmp->m_ap_type == M_AP_OBJECT) {
/*JP	pline("I see nobody there.");*/
	pline("ここにはなにもない．");
	return(0);
    }
    if (!mtmp->mcanmove || mtmp->msleep) {
/*JP	pline("%s seems not to notice you.", Monnam(mtmp));*/
	pline("%sはあなたに気がついてないようだ．",Monnam(mtmp));
	return(0);
    }

    if (mtmp->mtame && mtmp->meating) {
/*JP	pline("%s is eating noisily.", Monnam(mtmp));*/
	pline("%sはバリバリと物を食べている．", Monnam(mtmp));
	return (0);
    }

    return domonnoise(mtmp);
}

#endif /* OVLB */

/*sounds.c*/
