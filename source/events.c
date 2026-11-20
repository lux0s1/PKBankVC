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
	// ================= GEN 1 (14 Eventos) =================
	{
		"g1_mew_us", "MEW TOUR EEUU", "MEW TOUR EEUU", 1, EVENT_CAT_GEN1,
		151, 5, { 1, 0, 0, 0 },
		8, { "YOSHIRA", "YOSHIRB", "LUIGIA", "LUIGIB", "LINKW", "LINKE", "MARIO", "PEACH" },
		EVENT_TID_RANDOM, 0, EVENT_DV_FIXED, { 10, 1, 5, 12 }, 0x3C, false,
		{ "EE.UU. TOURS", "1999-2000", "MAQUINA TOUR EEUU", "CLASSIC", "FIDELIDAD HISTORICA GEN 1/2" }
	},
	{
		"g1_mew_mx", "MEW CLUB NINT. MX", "MEW MEXICO ASH", 1, EVENT_CAT_GEN1,
		151, 5, { 1, 0, 0, 0 },
		1, { "ASH" },
		EVENT_TID_FIXED, 45515, EVENT_DV_FIXED, { 10, 1, 5, 12 }, 0x3C, false,
		{ "LATAM MEXICO", "2000", "CLUB NINTENDO MEXICO", "CLASSIC", "FIDELIDAD HISTORICA GEN 1/2" }
	},
	{
		"g1_mew_eu", "MEW TOUR EUROPA", "MEW EUROPA", 1, EVENT_CAT_GEN1,
		151, 5, { 1, 0, 0, 0 },
		7, { "EUROPE", "SPAIN", "UK", "GERMANY", "FRANCE", "ITALY", "FINLAND" },
		EVENT_TID_RANDOM, 0, EVENT_DV_FIXED, { 10, 1, 5, 12 }, 0x3C, false,
		{ "EUROPA TOURS", "2000", "REPARTO TOUR EUROPA", "CLASSIC", "FIDELIDAD HISTORICA GEN 1/2" }
	},
	{
		"g1_mew_shiny", "MEW SHINY GLITCH", "MEW SHINY GLIT.", 1, EVENT_CAT_GEN1,
		151, 7, { 1, 0, 0, 0 },
		0, { NULL },
		EVENT_TID_PLAYER, 0, EVENT_DV_SHINY_GEN2, { 15, 10, 10, 10 }, 0x2D, false,
		{ "EVENTO PROPIO", "GEN 1/2", "GLITCH CERULEAN OT JUG.", "SHINY", "OT/ID DE TU PARTIDA - SHINY GEN 2" }
	},
	{
		"g1_pika_surf", "PIKACHU SURF NP", "PIKACHU SURF", 1, EVENT_CAT_GEN1,
		25, 5, { 84, 45, 57, 0 },
		0, { NULL },
		EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0xA3, false,
		{ "AMERICA/EUROPA", "1999", "CONCURSO NINTENDO POWER", "EV.MOVE", "EXCLUSIVO SURF + BOLA LUMINOSA" }
	},
	{
		"g1_psyduck_amnesia", "PSYDUCK AMNESIA", "PSYDUCK AMNESIA", 1, EVENT_CAT_GEN1,
		54, 15, { 10, 133, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA8, false,
		{ "N64 STADIUM", "2000", "SALON FAMA 151 STADIUM", "STADIUM", "SOLO FIDELIDAD GEN 1/2 (AMNESIA)" }
	},
	{
		"g1_stad_bulba", "BULBASAUR STADIUM", "BULBASAUR STAD.", 1, EVENT_CAT_GEN1,
		1, 5, { 33, 45, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_char", "CHARMANDER STAD.", "CHARMANDER STAD.", 1, EVENT_CAT_GEN1,
		4, 5, { 10, 45, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_squirt", "SQUIRTLE STADIUM", "SQUIRTLE STAD.", 1, EVENT_CAT_GEN1,
		7, 5, { 33, 39, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_hitmonlee", "HITMONLEE STADIUM", "HITMONLEE STAD.", 1, EVENT_CAT_GEN1,
		106, 20, { 24, 96, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_hitmonchan", "HITMONCHAN STAD.", "HITMONCHAN STAD.", 1, EVENT_CAT_GEN1,
		107, 20, { 4, 97, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_eevee", "EEVEE STADIUM", "EEVEE STADIUM", 1, EVENT_CAT_GEN1,
		133, 25, { 33, 39, 28, 98 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_omanyte", "OMANYTE STADIUM", "OMANYTE STADIUM", 1, EVENT_CAT_GEN1,
		138, 20, { 55, 110, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},
	{
		"g1_stad_kabuto", "KABUTO STADIUM", "KABUTO STADIUM", 1, EVENT_CAT_GEN1,
		140, 20, { 10, 106, 0, 0 },
		1, { "STADIUM" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA7, false,
		{ "N64 STADIUM", "2000", "CASTILLO LIDERES R1", "STADIUM", "RECOMPENSA OFICIAL POKEMON STADIUM" }
	},

	// ================= GEN 2 (5 Eventos) =================
	{
		"g2_celebi_tour", "CELEBI TOUR EU/US", "CELEBI TOUR", 2, EVENT_CAT_GEN2,
		251, 5, { 141, 93, 105, 215 },
		7, { "EUROPE", "SPAIN", "UK", "GERMANY", "FRANCE", "ITALY", "AMERICA" },
		EVENT_TID_RANDOM, 0, EVENT_DV_RANDOM_NON_SHINY_GEN2, { 0 }, 0x6D, false,
		{ "EUROPA/EE.UU.", "2001", "FUN FEST / TOURS OFIC.", "CLASSIC", "EVENTO HISTORICO CELEBI GEN 2" }
	},
	{
		"g2_celebi_ilex", "CELEBI ENCINAR VC", "CELEBI ENCINAR", 2, EVENT_CAT_GEN2,
		251, 30, { 215, 219, 246, 248 },
		0, { NULL },
		EVENT_TID_PLAYER, 0, EVENT_DV_SHINY_GEN2, { 15, 10, 10, 10 }, 0x00, false,
		{ "CRISTAL VC", "GS BALL", "ENCINAR ILEX OT JUGADOR", "SHINY", "LEGAL EN CRISTAL VC (GS BALL)" }
	},
	{
		"g2_stad2_farfetchd", "FARFETCHD RELEVO", "FARFETCHD REL.", 2, EVENT_CAT_GEN2,
		83, 5, { 226, 97, 14, 163 },
		1, { "Stadium" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xAE, false,
		{ "N64 STADIUM 2", "2001", "TRANSFER PAK RIVAL R1", "STADIUM", "MOVIMIENTO EXCLUSIVO RELEVO GEN 2" }
	},
	{
		"g2_stad2_gligar", "GLIGAR TERREMOTO", "GLIGAR TERREM.", 2, EVENT_CAT_GEN2,
		207, 5, { 89, 40, 68, 17 },
		1, { "Stadium" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0x96, false,
		{ "N64 STADIUM 2", "2001", "TRANSFER PAK RIVAL R2", "STADIUM", "MOVIMIENTO EXCLUSIVO TERREMOTO GEN 2" }
	},
	{
		"g2_stad2_pikachu", "PIKACHU STADIUM 2", "PIKACHU STAD.2", 2, EVENT_CAT_GEN2,
		25, 5, { 84, 45, 57, 0 },
		1, { "Stadium" },
		EVENT_TID_FIXED, 2000, EVENT_DV_RANDOM, { 0 }, 0xA3, false,
		{ "N64 STADIUM 2", "2001", "HALL FAMA STADIUM 2", "EV.MOVE", "SURF + BOLA LUMINOSA EQUIPADA" }
	},

	// ================= PCNY SHINY (12 Eventos) =================
	{
		"pcny_charizard", "CHARIZARD PCNY", "CHARIZARD PCNY", 2, EVENT_CAT_PCNY,
		6, 40, { 53, 17, 163, 83 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_blastoise", "BLASTOISE PCNY", "BLASTOISE PCNY", 2, EVENT_CAT_PCNY,
		9, 40, { 56, 44, 110, 130 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_venusaur", "VENUSAUR PCNY", "VENUSAUR PCNY", 2, EVENT_CAT_PCNY,
		3, 40, { 75, 79, 77, 76 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_tyranitar", "TYRANITAR PCNY", "TYRANITAR PCNY", 2, EVENT_CAT_PCNY,
		248, 55, { 242, 157, 103, 63 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_dragonite", "DRAGONITE PCNY", "DRAGONITE PCNY", 2, EVENT_CAT_PCNY,
		149, 55, { 200, 17, 86, 63 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY OFICIAL PCNY" }
	},
	{
		"pcny_suicune", "SUICUNE PCNY", "SUICUNE PCNY", 2, EVENT_CAT_PCNY,
		245, 40, { 62, 54, 243, 59 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_raikou", "RAIKOU PCNY", "RAIKOU PCNY", 2, EVENT_CAT_PCNY,
		243, 40, { 209, 115, 242, 87 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_entei", "ENTEI PCNY", "ENTEI PCNY", 2, EVENT_CAT_PCNY,
		244, 40, { 52, 46, 83, 126 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_lugia", "LUGIA PCNY", "LUGIA PCNY", 2, EVENT_CAT_PCNY,
		249, 50, { 177, 105, 56, 219 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_hooh", "HO-OH PCNY", "HO-OH PCNY", 2, EVENT_CAT_PCNY,
		250, 50, { 221, 105, 219, 126 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0xAE, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY MACHINE GEN 2", "SHINY", "EVENTO SHINY LEGENDARIO PCNY" }
	},
	{
		"pcny_mew", "MEW PCNY SHINY", "MEW PCNY SHINY", 2, EVENT_CAT_PCNY,
		151, 5, { 1, 144, 118, 94 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0x6D, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY STATION SINGULAR", "SHINY", "SOLO FIDELIDAD GEN 2 PCNY" }
	},
	{
		"pcny_celebi", "CELEBI PCNY SHINY", "CELEBI PCNY SH.", 2, EVENT_CAT_PCNY,
		251, 5, { 141, 93, 105, 215 },
		4, { "PCNYa", "PCNYb", "PCNYc", "PCNYd" },
		EVENT_TID_RANDOM, 0, EVENT_DV_SHINY_GEN2, { 0 }, 0x6D, false,
		{ "EE.UU. PCNY", "2001-2003", "PCNY STATION SINGULAR", "SHINY", "SOLO FIDELIDAD GEN 2 PCNY" }
	},

	// ================= HUEVOS PCNY (17 Eventos) =================
	{
		"egg_bulbasaur", "HUEVO BULBASAUR", "HUEVO BULBASAUR", 2, EVENT_CAT_EGGS,
		1, 5, { 246, 33, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO PODER PASADO (5 CICLOS)" }
	},
	{
		"egg_charmander", "HUEVO CHARMANDER", "HUEVO CHARMAND.", 2, EVENT_CAT_EGGS,
		4, 5, { 242, 10, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO TRITURAR (5 CICLOS)" }
	},
	{
		"egg_squirtle", "HUEVO SQUIRTLE", "HUEVO SQUIRTLE", 2, EVENT_CAT_EGGS,
		7, 5, { 192, 33, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO ELECTROCANON (5 CICLOS)" }
	},
	{
		"egg_chikorita", "HUEVO CHIKORITA", "HUEVO CHIKORITA", 2, EVENT_CAT_EGGS,
		152, 5, { 80, 33, 45, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO DANZA PETALO (5 CICLOS)" }
	},
	{
		"egg_cyndaquil", "HUEVO CYNDAQUIL", "HUEVO CYNDAQUIL", 2, EVENT_CAT_EGGS,
		155, 5, { 38, 33, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO DOBLE FILO (5 CICLOS)" }
	},
	{
		"egg_totodile", "HUEVO TOTODILE", "HUEVO TOTODILE", 2, EVENT_CAT_EGGS,
		158, 5, { 66, 10, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO SUMISION (5 CICLOS)" }
	},
	{
		"egg_pichu_sing", "HUEVO PICHU CANTO", "HUEVO PICHU CAN.", 2, EVENT_CAT_EGGS,
		172, 5, { 47, 84, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO CANTO + ENCANTO" }
	},
	{
		"egg_pichu_petal", "HUEVO PICHU PETAL.", "HUEVO PICHU PET.", 2, EVENT_CAT_EGGS,
		172, 5, { 80, 84, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO DANZA PETALO" }
	},
	{
		"egg_pichu_dizzy", "HUEVO PICHU MAREO", "HUEVO PICHU MAR.", 2, EVENT_CAT_EGGS,
		172, 5, { 146, 84, 204, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO PUNO MAREO" }
	},
	{
		"egg_snorlax_kiss", "HUEVO SNORLAX BESO", "HUEVO SNORLAX B.", 2, EVENT_CAT_EGGS,
		143, 5, { 142, 33, 133, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SAN VALENTIN", "HUEVO", "EXCLUSIVO BESO AMOROSO" }
	},
	{
		"egg_snorlax_splash", "HUEVO SNORLAX SAL.", "HUEVO SNORLAX S.", 2, EVENT_CAT_EGGS,
		143, 5, { 150, 186, 33, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY NORMAL WEEK", "HUEVO", "SALPICADURA + BESO DULCE" }
	},
	{
		"egg_eevee", "HUEVO EEVEE DESAR.", "HUEVO EEVEE DES.", 2, EVENT_CAT_EGGS,
		133, 5, { 74, 33, 39, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SEMANA RARA", "HUEVO", "EXCLUSIVO DESARROLLO GEN 2" }
	},
	{
		"egg_dratini", "HUEVO DRATINI HID.", "HUEVO DRATINI H.", 2, EVENT_CAT_EGGS,
		147, 5, { 56, 35, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY DRAGON WEEK", "HUEVO", "EXCLUSIVO HIDROBOMBA GEN 2" }
	},
	{
		"egg_larvitar", "HUEVO LARVITAR", "HUEVO LARVITAR", 2, EVENT_CAT_EGGS,
		246, 5, { 99, 44, 43, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2002-2003", "PCNY GOTTA CATCH EM ALL", "HUEVO", "EXCLUSIVO FURIA GEN 2" }
	},
	{
		"egg_omanyte", "HUEVO OMANYTE", "HUEVO OMANYTE", 2, EVENT_CAT_EGGS,
		138, 5, { 88, 132, 110, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SEMANA RARA FOSIL", "HUEVO", "EXCLUSIVO LANZARROCAS GEN 2" }
	},
	{
		"egg_kabuto", "HUEVO KABUTO", "HUEVO KABUTO", 2, EVENT_CAT_EGGS,
		140, 5, { 88, 10, 106, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
		{ "EE.UU. PCNY", "2003", "PCNY SEMANA RARA FOSIL", "HUEVO", "EXCLUSIVO LANZARROCAS GEN 2" }
	},
	{
		"egg_aerodactyl", "HUEVO AERODACTYL", "HUEVO AERODACT.", 2, EVENT_CAT_EGGS,
		142, 5, { 88, 17, 97, 0 },
		0, { NULL }, EVENT_TID_PLAYER, 0, EVENT_DV_RANDOM, { 0 }, 0x00, true,
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
	return (ev->dv_mode == EVENT_DV_RANDOM || ev->dv_mode == EVENT_DV_RANDOM_NON_SHINY_GEN2);
}

const char* eventGetDvLabel(const HistoricalEvent* ev, bool forceShiny)
{
	if (!ev) return "DVS";
	if (ev->dv_mode == EVENT_DV_FIXED) return "FIJOS";
	if (ev->dv_mode == EVENT_DV_SHINY_GEN2 || (forceShiny && eventAllowsManualShiny(ev)))
		return "SHINY GEN 2";
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
	if (ev->dv_mode == EVENT_DV_FIXED)
	{
		atk = ev->fixed_dvs[0] & 0x0F;
		def = ev->fixed_dvs[1] & 0x0F;
		spe = ev->fixed_dvs[2] & 0x0F;
		spc = ev->fixed_dvs[3] & 0x0F;
	}
	else if (ev->dv_mode == EVENT_DV_SHINY_GEN2 || (forceShiny && eventAllowsManualShiny(ev)))
	{
		static const uint8_t shinyAtk[8] = { 2, 3, 6, 7, 10, 11, 14, 15 };
		atk = shinyAtk[eventNextRand() & 7];
		def = 10;
		spe = 10;
		spc = 10;
	}
	else if (ev->dv_mode == EVENT_DV_RANDOM_NON_SHINY_GEN2)
	{
		do {
			uint32_t r = eventNextRand();
			atk = (uint8_t)((r >> 0) & 0x0F);
			def = (uint8_t)((r >> 4) & 0x0F);
			spe = (uint8_t)((r >> 8) & 0x0F);
			spc = (uint8_t)((r >> 12) & 0x0F);
		} while (areDvsShiny(atk, def, spe, spc));
	}
	else // EVENT_DV_RANDOM
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
