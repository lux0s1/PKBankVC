#include "events.h"
#include "data.h"
#include "items.h"
#include "personal.h"
#include "pokedex.h"

#include <3ds.h>
#include <stdio.h>
#include <string.h>

static uint32_t eventRngState = 0;

static uint32_t eventNextRand(void)
{
	if (eventRngState == 0)
	{
		eventRngState = (uint32_t)(osGetTime() ^ svcGetSystemTick() ^ 0xA55A1999u);
		if (eventRngState == 0) eventRngState = 0x19960227u;
	}
	uint32_t x = eventRngState;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	eventRngState = x;
	return x;
}

static bool areDvsShiny(uint8_t atk, uint8_t def, uint8_t spe, uint8_t spc)
{
	return (def == 10 && spe == 10 && spc == 10 && ((atk & 2) == 2));
}

static const HistoricalEvent historicalEvents[] = {
	// ================= GEN 1 (15 Eventos) =================
	{
		"g1_mew_us", "MEW TOUR EEUU", "MEW TOUR EEUU", 1, EVENT_CAT_GEN1,
		151, 5, { 1, 0, 0, 0 },
		8, { "YOSHI", "LINK", "ASH", "LUIGI", "MARIO", "PEACH", "TOAD", "BOWSER" },
		EVENT_TID_RANDOM, 0, EVENT_DV_FIXED, 0, { 10, 1, 5, 12 }, 0x3C, false,
		{ "EE.UU. TOURS", "1999-2000", "MAQUINA TOUR EEUU", "CLASSIC", "TRASLADADOR EXIGE OT:GF ID:22796" }
	},
	{
		"g1_mew_mx", "MEW CLUB NINT. MX", "MEW MEXICO ASH", 1, EVENT_CAT_GEN1,
		151, 5, { 1, 0, 0, 0 },
		1, { "ASH" },
		EVENT_TID_FIXED, 45515, EVENT_DV_FIXED, 0, { 10, 1, 5, 12 }, 0x3C, false,
		{ "LATAM MEXICO", "2002", "CLUB NINTENDO MEXICO", "CLASSIC", "TRASLADADOR EXIGE OT:GF ID:22796" }
	},
	{
		"g1_mew_eu", "MEW TOUR EUROPA", "MEW EUROPA", 1, EVENT_CAT_GEN1,
		151, 5, { 1, 0, 0, 0 },
		5, { "EUROPE", "UK", "GERMANY", "FINLAND", "SPAIN" },
		EVENT_TID_RANDOM, 0, EVENT_DV_FIXED, 0, { 10, 1, 5, 12 }, 0x3C, false,
		{ "EUROPA TOURS", "2000-2001", "REPARTO TOUR EUROPA", "CLASSIC", "TRASLADADOR EXIGE OT:GF ID:22796" }
	},
	{
		"g1_mew_shiny", "MEW SHINY GLITCH", "MEW SHINY GLIT.", 1, EVENT_CAT_GEN1,
		151, 7, { 1, 0, 0, 0 },
		0, { NULL },
		EVENT_TID_PLAYER, 0, EVENT_DV_SHINY_GEN2, 0, { 15, 10, 10, 10 }, 0x2D, false,
		{ "EVENTO PROPIO", "GEN 1/2", "GLITCH CERULEAN OT JUG.", "SHINY", "OT/ID DE TU PARTIDA - SHINY GEN 2" }
	},
	{
		"g1_pika_surf", "PIKACHU SURF NP", "PIKACHU SURF", 1, EVENT_CAT_GEN1,
		25, 5, { 84, 45, 57, 0 },
		1, { "NINTENDO" },
		EVENT_TID_RANDOM, 0, EVENT_DV_RANDOM, 0, { 0 }, 0xA3, false,
		{ "AMERICA/EUROPA", "1999", "CONCURSO NINTENDO POWER", "EV.MOVE", "EXCLUSIVO SURF + BOLA LUMINOSA" }
	},
	{
		"g1_pika_stadium", "PIKACHU STADIUM", "PIKACHU STAD.", 1, EVENT_CAT_GEN1,
		25, 5, { 84, 45, 57, 0 },
		0, { NULL },
		EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0xA3, false,
		{ "N64 STADIUM", "2000", "COPA PRIME / HALL FAMA", "EV.MOVE", "SURF EN STADIUM SIN TRASLADADOR" }
	},
	{
		"g1_psyduck_amnesia", "PSYDUCK AMNESIA", "PSYDUCK AMNESIA", 1, EVENT_CAT_GEN1,
		54, 15, { 10, 39, 50, 133 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA8, false,
		{ "N64 STADIUM", "2000", "SALON FAMA 151 STADIUM", "STADIUM", "TRASLADADOR BLOQUEA AMNESIA G1" }
	},
	{
		"g1_stad_bulba", "BULBASAUR STADIUM", "BULBASAUR STAD.", 1, EVENT_CAT_GEN1,
		1, 5, { 33, 45, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_char", "CHARMANDER STAD.", "CHARMANDER STAD.", 1, EVENT_CAT_GEN1,
		4, 5, { 10, 45, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_squirt", "SQUIRTLE STADIUM", "SQUIRTLE STAD.", 1, EVENT_CAT_GEN1,
		7, 5, { 33, 39, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_hitmonlee", "HITMONLEE STADIUM", "HITMONLEE STAD.", 1, EVENT_CAT_GEN1,
		106, 20, { 24, 96, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_hitmonchan", "HITMONCHAN STAD.", "HITMONCHAN STAD.", 1, EVENT_CAT_GEN1,
		107, 20, { 4, 97, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_eevee", "EEVEE STADIUM", "EEVEE STADIUM", 1, EVENT_CAT_GEN1,
		133, 25, { 33, 39, 28, 98 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_omanyte", "OMANYTE STADIUM", "OMANYTE STADIUM", 1, EVENT_CAT_GEN1,
		138, 20, { 55, 110, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_kabuto", "KABUTO STADIUM", "KABUTO STADIUM", 1, EVENT_CAT_GEN1,
		140, 20, { 10, 106, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},

	// ================= GEN 2 (5 Eventos) =================
	{
		"g2_celebi_tour", "CELEBI TOUR EU/US", "CELEBI TOUR", 2, EVENT_CAT_GEN2,
		251, 5, { 73, 93, 215, 105 },
		5, { "CTOUR", "NINTENDO", "FRANCO", "JENS", "NICO" },
		EVENT_TID_RANDOM, 0, EVENT_DV_RANDOM_NON_SHINY_GEN2, 0, { 0 }, 0x6D, false,
		{ "EUROPA/EE.UU.", "2001-2002", "FUN FEST / TOURS OFIC.", "CLASSIC", "EVENTO HISTORICO CELEBI GEN 2" }
	},
	{
		"g2_celebi_ilex", "CELEBI ENCINAR VC", "CELEBI ENCINAR", 2, EVENT_CAT_GEN2,
		251, 30, { 215, 219, 246, 248 },
		0, { NULL },
		EVENT_TID_PLAYER, 0, EVENT_DV_SHINY_GEN2, 0, { 15, 10, 10, 10 }, 0x00, false,
		{ "CRISTAL VC", "GS BALL", "ENCINAR ILEX OT JUGADOR", "SHINY", "LEGAL EN CRISTAL VC (GS BALL)" }
	},
	{
		"g2_stad2_farfetchd", "FARFETCHD RELEVO", "FARFETCHD REL.", 2, EVENT_CAT_GEN2,
		83, 5, { 64, 28, 43, 226 },
		1, { "Stadium" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xAE, false,
		{ "N64 STADIUM 2", "2001", "TRANSFER PAK RIVAL R1", "STADIUM", "TRASLADADOR BLOQUEA RELEVO G2" }
	},
	{
		"g2_stad2_gligar", "GLIGAR TERREMOTO", "GLIGAR TERREM.", 2, EVENT_CAT_GEN2,
		207, 5, { 40, 28, 106, 89 },
		1, { "Stadium" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0x96, false,
		{ "N64 STADIUM 2", "2001", "CASTILLO LIDERES R2", "STADIUM", "TRASLADADOR BLOQUEA TERREMOTO G2" }
	},
	{
		"g2_stad2_pikachu", "PIKACHU STADIUM 2", "PIKACHU STAD.2", 2, EVENT_CAT_GEN2,
		25, 5, { 84, 45, 57, 0 },
		1, { "Stadium" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, 0, { 0 }, 0xA3, false,
		{ "N64 STADIUM 2", "2001", "HALL FAMA STADIUM 2", "EV.MOVE", "SURF + BOLA LUMINOSA EQUIPADA" }
	},

	// ================= PCNY ENTREGAS DIRECTAS (19 Eventos) =================
	{
		"pcny_articuno", "ARTICUNO PCNY", "ARTICUNO PCNY", 2, EVENT_CAT_PCNY,
		144, 50, { 54, 97, 58, 59 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "SOLO FIDELIDAD GEN 2 PCNY" }
	},
	{
		"pcny_zapdos", "ZAPDOS PCNY", "ZAPDOS PCNY", 2, EVENT_CAT_PCNY,
		145, 50, { 84, 97, 65, 113 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "SOLO FIDELIDAD GEN 2 PCNY" }
	},
	{
		"pcny_moltres", "MOLTRES PCNY", "MOLTRES PCNY", 2, EVENT_CAT_PCNY,
		146, 50, { 52, 83, 97, 17 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "SOLO FIDELIDAD GEN 2 PCNY" }
	},
	{
		"pcny_mewtwo", "MEWTWO PCNY", "MEWTWO PCNY", 2, EVENT_CAT_PCNY,
		150, 70, { 94, 129, 105, 112 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "SOLO FIDELIDAD GEN 2 PCNY" }
	},
	{
		"pcny_charizard", "CHARIZARD PCNY", "CHARIZARD PCNY", 2, EVENT_CAT_PCNY,
		6, 40, { 53, 17, 163, 83 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_blastoise", "BLASTOISE PCNY", "BLASTOISE PCNY", 2, EVENT_CAT_PCNY,
		9, 40, { 56, 44, 110, 130 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_venusaur", "VENUSAUR PCNY", "VENUSAUR PCNY", 2, EVENT_CAT_PCNY,
		3, 40, { 75, 79, 77, 76 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_meganium", "MEGANIUM PCNY", "MEGANIUM PCNY", 2, EVENT_CAT_PCNY,
		154, 40, { 75, 219, 113, 34 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_typhlosion", "TYPHLOSION PCNY", "TYPHLOSION PCNY", 2, EVENT_CAT_PCNY,
		157, 40, { 53, 172, 98, 108 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_feraligatr", "FERALIGATR PCNY", "FERALIGATR PCNY", 2, EVENT_CAT_PCNY,
		160, 40, { 55, 44, 184, 163 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_tyranitar", "TYRANITAR PCNY", "TYRANITAR PCNY", 2, EVENT_CAT_PCNY,
		248, 55, { 242, 157, 103, 63 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_dragonite", "DRAGONITE PCNY", "DRAGONITE PCNY", 2, EVENT_CAT_PCNY,
		149, 55, { 200, 17, 86, 63 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_suicune", "SUICUNE PCNY", "SUICUNE PCNY", 2, EVENT_CAT_PCNY,
		245, 40, { 62, 54, 243, 59 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_raikou", "RAIKOU PCNY", "RAIKOU PCNY", 2, EVENT_CAT_PCNY,
		243, 40, { 209, 115, 242, 87 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_entei", "ENTEI PCNY", "ENTEI PCNY", 2, EVENT_CAT_PCNY,
		244, 40, { 52, 46, 83, 126 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_lugia", "LUGIA PCNY", "LUGIA PCNY", 2, EVENT_CAT_PCNY,
		249, 50, { 177, 105, 56, 219 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_hooh", "HO-OH PCNY", "HO-OH PCNY", 2, EVENT_CAT_PCNY,
		250, 50, { 221, 105, 219, 126 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, 0, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_mew", "MEW PCNY", "MEW PCNY 0.5 PCT", 2, EVENT_CAT_PCNY,
		151, 5, { 1, 144, 118, 94 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_ODDS_SHINY_GEN2, 5, { 0 }, 0x6D, false,
		{ "EE.UU. PCNY", "NOV 2002", "CELEBI PRESENT SP (0.5)", "ODDS", "PROB. 0.5 PCT SHINY (FORZABLE)" }
	},
	{
		"pcny_celebi", "CELEBI PCNY", "CELEBI PCNY 1 PCT", 2, EVENT_CAT_PCNY,
		251, 5, { 73, 93, 215, 105 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_ODDS_SHINY_GEN2, 10, { 0 }, 0x6D, false,
		{ "EE.UU. PCNY", "2001-2003", "CHRISTMAS / CELEBI (1.0)", "ODDS", "PROB. 1.0 PCT SHINY (FORZABLE)" }
	},

	// ================= HUEVOS PCNY (65 Eventos por variante) =================
	{
		"egg_bulbasaur", "HUEVO BULBASAUR", "HUEVO BULBASAUR", 2, EVENT_CAT_EGGS,
		1, 5, { 246, 33, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO PODER PASADO (5 CICLOS)" }
	},
	{
		"egg_charmander", "HUEVO CHARMANDER", "HUEVO CHARMAND.", 2, EVENT_CAT_EGGS,
		4, 5, { 242, 10, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO TRITURAR (5 CICLOS)" }
	},
	{
		"egg_squirtle", "HUEVO SQUIRTLE", "HUEVO SQUIRTLE", 2, EVENT_CAT_EGGS,
		7, 5, { 192, 33, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO ELECTROCANON (5 CICLOS)" }
	},
	{
		"egg_chikorita", "HUEVO CHIKORITA", "HUEVO CHIKORITA", 2, EVENT_CAT_EGGS,
		152, 5, { 80, 33, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO DANZA PETALO (5 CICLOS)" }
	},
	{
		"egg_cyndaquil", "HUEVO CYNDAQUIL", "HUEVO CYNDAQUIL", 2, EVENT_CAT_EGGS,
		155, 5, { 38, 33, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO DOBLE FILO (5 CICLOS)" }
	},
	{
		"egg_totodile", "HUEVO TOTODILE", "HUEVO TOTODILE", 2, EVENT_CAT_EGGS,
		158, 5, { 66, 10, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO SUMISION (5 CICLOS)" }
	},
	{
		"egg_pichu_sing", "HUEVO PICHU CANTO", "PICHU CANTO", 2, EVENT_CAT_EGGS,
		172, 5, { 47, 84, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO CANTO + ENCANTO" }
	},
	{
		"egg_pichu_petal", "HUEVO PICHU PETALO", "PICHU PETALO", 2, EVENT_CAT_EGGS,
		172, 5, { 80, 84, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO DANZA PETALO" }
	},
	{
		"egg_pichu_dizzy", "HUEVO PICHU MAREO", "PICHU P. MAREO", 2, EVENT_CAT_EGGS,
		172, 5, { 146, 84, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO PUNO MAREO" }
	},
	{
		"egg_pichu_scary", "HUEVO PICHU SUSTO", "PICHU CARA SUSTO", 2, EVENT_CAT_EGGS,
		172, 5, { 184, 84, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO CARA SUSTO (5 CICLOS)" }
	},
	{
		"egg_cleffa_swift", "CLEFFA METEORO", "CLEFFA METEORO", 2, EVENT_CAT_EGGS,
		173, 5, { 129, 1, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PRIMAVERA/BEBES", "HUEVO", "EXCLUSIVO RAPIDEZ (SWIFT) GEN 2" }
	},
	{
		"egg_cleffa_petal", "CLEFFA DANZA PET.", "CLEFFA D.PETALO", 2, EVENT_CAT_EGGS,
		173, 5, { 80, 1, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PRIMAVERA/BEBES", "HUEVO", "EXCLUSIVO DANZA PETALO GEN 2" }
	},
	{
		"egg_igglybuff_mimic", "IGGLYBUFF MIMETICO", "IGGLYBUFF MIMET.", 2, EVENT_CAT_EGGS,
		174, 5, { 102, 47, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PRIMAVERA/BEBES", "HUEVO", "EXCLUSIVO MIMETICO (5 CICLOS)" }
	},
	{
		"egg_igglybuff_petal", "IGGLYBUFF PETALO", "IGGLYBUFF PET.", 2, EVENT_CAT_EGGS,
		174, 5, { 80, 47, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PRIMAVERA/BEBES", "HUEVO", "EXCLUSIVO DANZA PETALO" }
	},
	{
		"egg_smoochum", "SMOOCHUM METRON.", "SMOOCHUM METRON.", 2, EVENT_CAT_EGGS,
		238, 5, { 118, 1, 122, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PRIMAVERA/BEBES", "HUEVO", "EXCLUSIVO METRONOMO GEN 2" }
	},
	{
		"egg_elekid_pursuit", "ELEKID PERSECUC.", "ELEKID PERSEC.", 2, EVENT_CAT_EGGS,
		239, 5, { 228, 98, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PLANTA ENERGIA", "HUEVO", "EXCLUSIVO PERSECUCION GEN 2" }
	},
	{
		"egg_elekid_dizzy", "ELEKID PUNO MAREO", "ELEKID P.MAREO", 2, EVENT_CAT_EGGS,
		239, 5, { 146, 98, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PLANTA ENERGIA", "HUEVO", "EXCLUSIVO PUNO MAREO GEN 2" }
	},
	{
		"egg_magby", "HUEVO MAGBY FINTA", "MAGBY FINTA", 2, EVENT_CAT_EGGS,
		240, 5, { 185, 52, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PRIMAVERA/BEBES", "HUEVO", "EXCLUSIVO FINTA (5 CICLOS)" }
	},
	{
		"egg_magnemite", "MAGNEMITE AGIL.", "MAGNEMITE AGIL.", 2, EVENT_CAT_EGGS,
		81, 5, { 97, 33, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PLANTA ENERGIA", "HUEVO", "EXCLUSIVO AGILIDAD GEN 2" }
	},
	{
		"egg_voltorb", "VOLTORB AGILIDAD", "VOLTORB AGILID.", 2, EVENT_CAT_EGGS,
		100, 5, { 97, 33, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY PLANTA ENERGIA", "HUEVO", "EXCLUSIVO AGILIDAD GEN 2" }
	},
	{
		"egg_delibird", "DELIBIRD DIA PAGO", "DELIBIRD PAGO", 2, EVENT_CAT_EGGS,
		225, 5, { 6, 217, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "DIC 2001", "PCNY NAVIDAD 2001", "HUEVO", "EXCLUSIVO DIA DE PAGO + PRESENTE" }
	},
	{
		"egg_nidoranf_kiss", "NIDORAN F B.DULCE", "NIDORAN F DULCE", 2, EVENT_CAT_EGGS,
		29, 5, { 186, 45, 33, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO BESO DULCE GEN 2" }
	},
	{
		"egg_nidoranf_moon", "NIDORAN F LUZ LUN.", "NIDORAN F LUNA", 2, EVENT_CAT_EGGS,
		29, 5, { 236, 45, 33, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO LUZ LUNAR GEN 2" }
	},
	{
		"egg_nidoranm_kiss", "NIDORAN M B.DULCE", "NIDORAN M DULCE", 2, EVENT_CAT_EGGS,
		32, 5, { 186, 43, 33, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO BESO DULCE GEN 2" }
	},
	{
		"egg_nidoranm_morn", "NIDORAN M SOL MAT.", "NIDORAN M SOL", 2, EVENT_CAT_EGGS,
		32, 5, { 234, 43, 33, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO SOL MATINAL GEN 2" }
	},
	{
		"egg_bellsprout_sw", "BELLSPR. B.DULCE", "BELLSPR. DULCE", 2, EVENT_CAT_EGGS,
		69, 5, { 186, 22, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO BESO DULCE GEN 2" }
	},
	{
		"egg_bellsprout_lv", "BELLSPR. B.AMOR.", "BELLSPR. AMOR.", 2, EVENT_CAT_EGGS,
		69, 5, { 142, 22, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO BESO AMOROSO GEN 2" }
	},
	{
		"egg_marill_hydro", "MARILL HIDROBOMBA", "MARILL HIDROB.", 2, EVENT_CAT_EGGS,
		183, 5, { 56, 33, 111, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO HIDROBOMBA GEN 2" }
	},
	{
		"egg_marill_dizzy", "MARILL PUNO MAREO", "MARILL P.MAREO", 2, EVENT_CAT_EGGS,
		183, 5, { 146, 33, 111, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO PUNO MAREO GEN 2" }
	},
	{
		"egg_yanma", "YANMA ALA ACERO", "YANMA ALA ACERO", 2, EVENT_CAT_EGGS,
		193, 5, { 211, 33, 193, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO ALA DE ACERO GEN 2" }
	},
	{
		"egg_dunsparce", "DUNSPARCE FURIA", "DUNSPARCE FURIA", 2, EVENT_CAT_EGGS,
		206, 5, { 31, 99, 111, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "ATAQUE FURIA (SIN TALADRADORA G2)" }
	},
	{
		"egg_snubbull", "SNUBBULL B.AMOR.", "SNUBBULL B.AMOR", 2, EVENT_CAT_EGGS,
		209, 5, { 142, 33, 184, 39 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SAN VALENTIN", "HUEVO", "EXCLUSIVO BESO AMOROSO GEN 2" }
	},
	{
		"egg_qwilfish", "QWILFISH D.FILO", "QWILFISH D.FILO", 2, EVENT_CAT_EGGS,
		211, 5, { 38, 33, 40, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO DOBLE FILO GEN 2" }
	},
	{
		"egg_remoraid_amn", "REMORAID AMNESIA", "REMORAID AMNES.", 2, EVENT_CAT_EGGS,
		223, 5, { 133, 55, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO AMNESIA GEN 2" }
	},
	{
		"egg_remoraid_mist", "REMORAID NEBLINA", "REMORAID NEBL.", 2, EVENT_CAT_EGGS,
		223, 5, { 54, 55, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO NEBLINA GEN 2" }
	},
	{
		"egg_psyduck_petal", "PSYDUCK D.PETALO", "PSYDUCK PETALO", 2, EVENT_CAT_EGGS,
		54, 5, { 80, 10, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO DANZA PETALO GEN 2" }
	},
	{
		"egg_psyduck_tri", "PSYDUCK TRIATAQUE", "PSYDUCK TRIAT.", 2, EVENT_CAT_EGGS,
		54, 5, { 161, 10, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO TRIATAQUE GEN 2" }
	},
	{
		"egg_wooper", "WOOPER TAMBOR", "WOOPER TAMBOR", 2, EVENT_CAT_EGGS,
		194, 5, { 187, 55, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO TAMBOR GEN 2" }
	},
	{
		"egg_poliwag", "POLIWAG DESARR.", "POLIWAG DESARR.", 2, EVENT_CAT_EGGS,
		60, 5, { 74, 145, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO DESARROLLO GEN 2" }
	},
	{
		"egg_horsea", "HORSEA NEBLINA", "HORSEA NEBLINA", 2, EVENT_CAT_EGGS,
		116, 5, { 114, 145, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO NEBLINA GEN 2" }
	},
	{
		"egg_goldeen", "GOLDEEN D.ESPADA", "GOLDEEN ESPADA", 2, EVENT_CAT_EGGS,
		118, 5, { 14, 64, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO DANZA ESPADA GEN 2" }
	},
	{
		"egg_magikarp_rev", "MAGIKARP INVERS.", "MAGIKARP INVER.", 2, EVENT_CAT_EGGS,
		129, 5, { 179, 150, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO INVERSION GEN 2" }
	},
	{
		"egg_magikarp_bub", "MAGIKARP BURBUJA", "MAGIKARP BURB.", 2, EVENT_CAT_EGGS,
		129, 5, { 145, 150, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO BURBUJA GEN 2" }
	},
	{
		"egg_tentacool", "TENTACOOL R.CONF.", "TENTACOOL CONF.", 2, EVENT_CAT_EGGS,
		72, 5, { 109, 40, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO RAYO CONFUSO GEN 2" }
	},
	{
		"egg_lapras_bite", "LAPRAS MORDISCO", "LAPRAS MORDISCO", 2, EVENT_CAT_EGGS,
		131, 5, { 44, 55, 45, 47 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO MORDISCO GEN 2" }
	},
	{
		"egg_lapras_fut", "LAPRAS PREMONIC.", "LAPRAS PREMON.", 2, EVENT_CAT_EGGS,
		131, 5, { 248, 55, 45, 47 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO PREMONICION GEN 2" }
	},
	{
		"egg_chinchou", "CHINCHOU P.LUZ", "CHINCHOU P.LUZ", 2, EVENT_CAT_EGGS,
		170, 5, { 113, 145, 86, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO PANTALLA DE LUZ GEN 2" }
	},
	{
		"egg_mantine", "MANTINE TORNADO", "MANTINE TORNADO", 2, EVENT_CAT_EGGS,
		226, 5, { 16, 33, 145, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO TORNADO GEN 2" }
	},
	{
		"egg_chansey", "CHANSEY D.AROMA", "CHANSEY AROMA", 2, EVENT_CAT_EGGS,
		113, 5, { 230, 1, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO DULCE AROMA GEN 2" }
	},
	{
		"egg_kangaskhan", "KANGASKHAN FINTA", "KANGASKHAN FIN.", 2, EVENT_CAT_EGGS,
		115, 5, { 185, 4, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO FINTA GEN 2" }
	},
	{
		"egg_tauros", "TAUROS AT.RAPIDO", "TAUROS AT.RAP.", 2, EVENT_CAT_EGGS,
		128, 5, { 98, 33, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO ATAQUE RAPIDO GEN 2" }
	},
	{
		"egg_spearow", "SPEAROW B.SONICA", "SPEAROW SONICA", 2, EVENT_CAT_EGGS,
		21, 5, { 49, 64, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO BOMBA SONICA GEN 2" }
	},
	{
		"egg_farfetchd", "FARFETCHD C.FURIA", "FARFETCHD FURIA", 2, EVENT_CAT_EGGS,
		83, 5, { 210, 64, 28, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO CORTE FURIA GEN 2" }
	},
	{
		"egg_doduo", "DODUO PATADA BAJA", "DODUO P. BAJA", 2, EVENT_CAT_EGGS,
		84, 5, { 67, 64, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO PATADA BAJA GEN 2" }
	},
	{
		"egg_natu", "NATU VELO SAGRADO", "NATU V.SAGRADO", 2, EVENT_CAT_EGGS,
		177, 5, { 219, 64, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO VELO SAGRADO GEN 2" }
	},
	{
		"egg_murkrow", "MURKROW PALIZA", "MURKROW PALIZA", 2, EVENT_CAT_EGGS,
		198, 5, { 251, 64, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO PALIZA GEN 2" }
	},
	{
		"egg_skarmory", "SKARMORY C.FURIA", "SKARMORY FURIA", 2, EVENT_CAT_EGGS,
		227, 5, { 210, 43, 64, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO CORTE FURIA GEN 2" }
	},
	{
		"egg_miltank", "MILTANK MEGAPAT.", "MILTANK MEGAP.", 2, EVENT_CAT_EGGS,
		241, 5, { 25, 33, 0, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY CAMPANA TEMATICA", "HUEVO", "EXCLUSIVO MEGAPATADA GEN 2" }
	},
	{
		"egg_snorlax_kiss", "HUEVO SNORLAX BESO", "SNORLAX B.AMOR.", 2, EVENT_CAT_EGGS,
		143, 5, { 142, 33, 133, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SAN VALENTIN", "HUEVO", "EXCLUSIVO BESO AMOROSO" }
	},
	{
		"egg_snorlax_splash", "HUEVO SNORLAX SAL.", "SNORLAX SALPIC.", 2, EVENT_CAT_EGGS,
		143, 5, { 150, 186, 33, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY NORMAL WEEK", "HUEVO", "SALPICADURA + BESO DULCE" }
	},
	{
		"egg_eevee", "HUEVO EEVEE DESAR.", "EEVEE DESARR.", 2, EVENT_CAT_EGGS,
		133, 5, { 74, 33, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SEMANA RARA", "HUEVO", "EXCLUSIVO DESARROLLO GEN 2" }
	},
	{
		"egg_dratini", "HUEVO DRATINI HID.", "DRATINI HIDROB.", 2, EVENT_CAT_EGGS,
		147, 5, { 56, 35, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY DRAGON WEEK", "HUEVO", "EXCLUSIVO HIDROBOMBA GEN 2" }
	},
	{
		"egg_larvitar", "HUEVO LARVITAR", "LARVITAR FURIA", 2, EVENT_CAT_EGGS,
		246, 5, { 99, 44, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO FURIA GEN 2" }
	},
	{
		"egg_omanyte", "HUEVO OMANYTE", "OMANYTE L.ROCAS", 2, EVENT_CAT_EGGS,
		138, 5, { 88, 132, 110, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SEMANA RARA FOSIL", "HUEVO", "EXCLUSIVO LANZARROCAS GEN 2" }
	},
	{
		"egg_kabuto", "HUEVO KABUTO", "KABUTO L.ROCAS", 2, EVENT_CAT_EGGS,
		140, 5, { 88, 10, 106, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SEMANA RARA FOSIL", "HUEVO", "EXCLUSIVO LANZARROCAS GEN 2" }
	},
	{
		"egg_aerodactyl", "HUEVO AERODACTYL", "AERODACT. ROCAS", 2, EVENT_CAT_EGGS,
		142, 5, { 88, 17, 97, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, 0, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SEMANA RARA FOSIL", "HUEVO", "EXCLUSIVO LANZARROCAS GEN 2" }
	},
};

uint16_t eventsGetCount(void)
{
	return (uint16_t)(sizeof(historicalEvents) / sizeof(historicalEvents[0]));
}

const HistoricalEvent* eventsGet(uint16_t index)
{
	uint16_t n = eventsGetCount();
	if (index >= n) return &historicalEvents[0];
	return &historicalEvents[index];
}

uint16_t eventsGetByCategory(EventCategory cat, uint16_t* outIndices, uint16_t maxOut)
{
	uint16_t n = eventsGetCount();
	uint16_t cnt = 0;
	for (uint16_t i = 0; i < n && cnt < maxOut; i++)
	{
		if (historicalEvents[i].category == cat)
			outIndices[cnt++] = i;
	}
	return cnt;
}

bool eventAllowsManualShiny(const HistoricalEvent* ev)
{
	if (!ev) return false;
	return (ev->dv_mode == EVENT_DV_RANDOM ||
	        ev->dv_mode == EVENT_DV_RANDOM_NON_SHINY_GEN2 ||
	        ev->dv_mode == EVENT_DV_ODDS_SHINY_GEN2);
}

const char* eventGetDvLabel(const HistoricalEvent* ev, bool forceShiny)
{
	if (!ev) return "DVS";
	if (ev->dv_mode == EVENT_DV_FIXED) return "FIJOS";
	if (ev->dv_mode == EVENT_DV_SHINY_GEN2 || (forceShiny && eventAllowsManualShiny(ev)))
		return "SHINY GEN 2";
	if (ev->dv_mode == EVENT_DV_ODDS_SHINY_GEN2)
	{
		if (ev->shiny_chance_permille == 10) return "PROB. 1.0 PCT";
		if (ev->shiny_chance_permille == 5)  return "PROB. 0.5 PCT";
		return "PROB. SHINY GEN 2";
	}
	return "ALEATORIO";
}

const char* eventGetItemOrCrLabel(const HistoricalEvent* ev)
{
	if (!ev) return "NINGUNO";
	if (ev->gen == 1)
	{
		switch (ev->gen1_cr_or_g2_item)
		{
			case 0x3C: return "0x3C BAYA AMARGA";
			case 0x2D: return "0x2D BASE MEW";
			case 0xA3: return "0xA3 BOLALUMINOSA";
			case 0xA7: return "0xA7 CAJA NORMAL";
			case 0xA8: return "0xA8 CAJA LUJO";
			default:   return "CR ESTANDAR";
		}
	}
	else
	{
		switch (ev->gen1_cr_or_g2_item)
		{
			case 0xAE: return "BAYA DORADA";
			case 0x96: return "BAYA MISTERIO";
			case 0x6D: return "BAYA MILAGRO";
			case 0xA3: return "BOLALUMINOSA";
			case 0x00: return "NINGUNO";
			default:   return "OBJETO EVENTO";
		}
	}
}

void eventBuildPokemon(const HistoricalEvent* ev, uint8_t otIdx, bool forceShiny, SAV_Pokemon* outPkm)
{
	if (!ev || !outPkm) return;
	memset(outPkm, 0, sizeof(SAV_Pokemon));

	outPkm->nationalDex = ev->species_id;
	outPkm->species = pokedexGetIndex(ev->species_id);
	outPkm->speciesIndex = outPkm->species;
	outPkm->level = ev->level;
	outPkm->currentLevel = ev->level;
	outPkm->experience = saveCalcExpForLevel(ev->species_id, ev->level);
	outPkm->isEgg = ev->is_egg;

	// Moves & base PP (PPUps = 0)
	for (uint8_t i = 0; i < 4; i++)
	{
		uint8_t m = ev->moves[i];
		outPkm->moves[i] = m;
		outPkm->PPUps[i] = 0;
		if (m != 0 && m <= 251)
		{
			uint8_t basePP = PersonalMove(m)->PP;
			outPkm->PPs[i] = (basePP > 63 ? 63 : basePP);
		}
		else
		{
			outPkm->PPs[i] = 0;
		}
	}

	// EVs strictly 0x0000 for authentic untouched event distribution
	for (uint8_t i = 0; i < 5; i++)
		outPkm->EVs[i] = 0;

	// DVs resolution
	uint8_t atk = 0, def = 0, spe = 0, spc = 0;
	bool makeShiny = false;
	bool avoidShiny = false;

	if (ev->dv_mode == EVENT_DV_FIXED)
	{
		atk = ev->fixed_dvs[0] & 0x0F;
		def = ev->fixed_dvs[1] & 0x0F;
		spe = ev->fixed_dvs[2] & 0x0F;
		spc = ev->fixed_dvs[3] & 0x0F;
	}
	else if (ev->dv_mode == EVENT_DV_SHINY_GEN2 || (forceShiny && eventAllowsManualShiny(ev)))
	{
		makeShiny = true;
	}
	else if (ev->dv_mode == EVENT_DV_ODDS_SHINY_GEN2)
	{
		if (ev->shiny_chance_permille > 0 && (eventNextRand() % 1000u) < ev->shiny_chance_permille)
			makeShiny = true;
		else
			avoidShiny = true;
	}
	else if (ev->dv_mode == EVENT_DV_RANDOM_NON_SHINY_GEN2)
	{
		avoidShiny = true;
	}

	if (makeShiny)
	{
		static const uint8_t shinyAtk[8] = { 2, 3, 6, 7, 10, 11, 14, 15 };
		atk = shinyAtk[eventNextRand() & 7];
		def = 10;
		spe = 10;
		spc = 10;
	}
	else if (avoidShiny)
	{
		do {
			uint32_t r = eventNextRand();
			atk = (uint8_t)((r >> 0) & 0x0F);
			def = (uint8_t)((r >> 4) & 0x0F);
			spe = (uint8_t)((r >> 8) & 0x0F);
			spc = (uint8_t)((r >> 12) & 0x0F);
		} while (areDvsShiny(atk, def, spe, spc));
	}
	else if (ev->dv_mode == EVENT_DV_RANDOM)
	{
		uint32_t r = eventNextRand();
		atk = (uint8_t)((r >> 0) & 0x0F);
		def = (uint8_t)((r >> 4) & 0x0F);
		spe = (uint8_t)((r >> 8) & 0x0F);
		spc = (uint8_t)((r >> 12) & 0x0F);
	}

	outPkm->IVs[STAT_ATK] = atk;
	outPkm->IVs[STAT_DEF] = def;
	outPkm->IVs[STAT_SPE] = spe;
	outPkm->IVs[STAT_SPC] = spc;
	outPkm->IVs[STAT_HP] = ((atk & 1) << 3) | ((def & 1) << 2) | ((spe & 1) << 1) | (spc & 1);

	// TID resolution
	if (ev->tid_mode == EVENT_TID_PLAYER)
		outPkm->originalTID = saveGetTrainerID();
	else if (ev->tid_mode == EVENT_TID_FIXED)
		outPkm->originalTID = ev->fixed_tid;
	else
		outPkm->originalTID = (uint16_t)(eventNextRand() & 0xFFFF);

	// OT Name (canonical 0x50 padding, max 7 western chars + 0x50)
	memset(outPkm->nameOT, 0x50, sizeof(outPkm->nameOT));
	if (ev->ot_count == 0)
	{
		const char8_t* pOt = saveGetTrainerName();
		for (uint8_t i = 0; i < 10; i++)
		{
			outPkm->nameOT[i] = pOt[i];
			if (pOt[i] == 0x50) break;
		}
		outPkm->nameOT[10] = 0x50;
	}
	else
	{
		uint8_t idx = (otIdx < ev->ot_count) ? otIdx : 0;
		fontConvertString(outPkm->nameOT, ev->ot_options[idx]);
		bool term = false;
		for (uint8_t i = 0; i < 11; i++)
		{
			if (term) outPkm->nameOT[i] = 0x50;
			else if (outPkm->nameOT[i] == 0x50) term = true;
		}
		outPkm->nameOT[10] = 0x50;
	}

	// Nickname (HUEVO/EGG for Eggs, official species name otherwise, padded with 0x50)
	memset(outPkm->nameNK, 0x50, sizeof(outPkm->nameNK));
	if (ev->is_egg)
	{
		fontConvertString(outPkm->nameNK, (lgame == POKEMON_ES) ? "HUEVO" : "EGG");
		bool term = false;
		for (uint8_t i = 0; i < 11; i++)
		{
			if (term) outPkm->nameNK[i] = 0x50;
			else if (outPkm->nameNK[i] == 0x50) term = true;
		}
	}
	else
	{
		const char8_t* spName = dataSpecies(ev->species_id);
		for (uint8_t i = 0; i < 10; i++)
		{
			outPkm->nameNK[i] = spName[i];
			if (spName[i] == 0x50) break;
		}
	}
	outPkm->nameNK[10] = 0x50;

	// Catch Rate (Gen 1) vs Held Item (Gen 2) + Friendship/Hatch cycles
	if (ev->gen == 1)
	{
		outPkm->catchRate = ev->gen1_cr_or_g2_item;
		outPkm->heldItem = 0;
		outPkm->friendship = 70;
	}
	else
	{
		outPkm->heldItem = ev->is_egg ? 0 : ev->gen1_cr_or_g2_item;
		outPkm->catchRate = (outPkm->heldItem != 0) ? outPkm->heldItem : Personal(ev->species_id)->catchRate;
		outPkm->friendship = ev->is_egg ? 5 : 70; // 5 cycles (~1280 steps) for easy hatching on PCNY eggs
		if (strcmp(ev->id, "g2_celebi_ilex") == 0)
			outPkm->caughtData = 0x9E0E; // Crystal VC: Day (2), Met Lv.30, Ilex Forest (0x0E)
	}

	saveRecalculateStats(outPkm);
	outPkm->currentHP = outPkm->maxHP;
	outPkm->moved = true;
}

int16_t eventInject(const HistoricalEvent* ev, uint8_t otIdx, bool forceShiny, EventInjectDest dest, char* outMsg1, char* outMsg2)
{
	if (!ev) return -1;

	if ((dest == EVENT_DEST_PC_BOX || dest == EVENT_DEST_PARTY) && ev->gen == 2 && vgame <= POKEMON_YELLOW)
	{
		if (outMsg1) snprintf(outMsg1, 36, "EVENTO SOLO DE 2A GEN");
		if (outMsg2) snprintf(outMsg2, 36, "ABRE ORO, PLATA O CRISTAL");
		return -1;
	}

	if (dest == EVENT_DEST_BANK && ev->is_egg)
	{
		if (outMsg1) snprintf(outMsg1, 36, "HUEVO NO ADMITIDO EN BANCO");
		if (outMsg2) snprintf(outMsg2, 36, "USA DESTINO CAJA PC O EQUIPO");
		return -1;
	}

	SAV_Pokemon pkm;
	eventBuildPokemon(ev, otIdx, forceShiny, &pkm);

	if (dest == EVENT_DEST_PARTY)
	{
		if (!saveInjectPartyPkm(&pkm))
		{
			if (outMsg1) snprintf(outMsg1, 36, "EQUIPO LLENO (6/6)");
			if (outMsg2) snprintf(outMsg2, 36, "LIBERA UN HUECO O USA CAJA");
			return -1;
		}
		if (outMsg1) snprintf(outMsg1, 36, "EVENTO INYECTADO EN EQUIPO");
		if (outMsg2) snprintf(outMsg2, 36, "%s AHORA EN TU EQUIPO", ev->short_name);
		return 1;
	}
	else if (dest == EVENT_DEST_PC_BOX)
	{
		for (uint8_t b = 0; b < sgame.boxCount; b++)
		{
			for (uint8_t s = 0; s < sgame.boxes[b].capacity; s++)
			{
				if (saveIsPkmEmpty(&sgame.boxes[b].slots[s]))
				{
					sgame.boxes[b].slots[s] = pkm;
					sgame.boxes[b].count++;
					if (!pkm.isEgg)
					{
						pokedexAddOwned(pkm.nationalDex);
						pokedexAddSeen(pkm.nationalDex);
						bankStoreRecordSpecies(pkm.nationalDex, vgame);
					}
					if (outMsg1) snprintf(outMsg1, 36, "EVENTO EN CAJA PC %d", (int)(b + 1));
					if (outMsg2) snprintf(outMsg2, 36, "%s RECIBIDO", ev->short_name);
					return (int16_t)(b + 1);
				}
			}
		}
		if (outMsg1) snprintf(outMsg1, 36, "CAJAS DEL PC LLENAS");
		if (outMsg2) snprintf(outMsg2, 36, "USA EQUIPO O BANCO");
		return -1;
	}
	else // EVENT_DEST_BANK
	{
		for (uint8_t b = 0; b < sbank.boxCount; b++)
		{
			for (uint8_t s = 0; s < sbank.boxes[b].capacity; s++)
			{
				if (saveIsPkmEmpty(&sbank.boxes[b].slots[s]))
				{
					sbank.boxes[b].slots[s] = pkm;
					sbank.boxes[b].count++;
					if (!pkm.isEgg)
					{
						bankStoreRecordSpecies(pkm.nationalDex, (ev->gen == 1 ? POKEMON_YELLOW : POKEMON_CRYSTAL));
					}
					saveCommitBank();
					if (outMsg1) snprintf(outMsg1, 36, "EVENTO EN BANCO CAJA %d", (int)(b + 1));
					if (outMsg2) snprintf(outMsg2, 36, "%s DEPOSITADO", ev->short_name);
					return (int16_t)(b + 1);
				}
			}
		}
		if (outMsg1) snprintf(outMsg1, 36, "BANCO LLENO");
		if (outMsg2) snprintf(outMsg2, 36, "LIBERA ESPACIO EN EL BANCO");
		return -1;
	}
}
