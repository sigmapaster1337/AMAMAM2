#include "ESP.h"

#include "../Groups/Groups.h"
#include "../../Players/PlayerUtils.h"
#include "../../Spectate/Spectate.h"
#include "../../Simulation/MovementSimulation/MovementSimulation.h"
#include "../../Simulation/ProjectileSimulation/ProjectileSimulation.h"

MAKE_SIGNATURE(CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot, "client.dll", "48 89 6C 24 ? 56 41 54 41 55 41 56 41 57 48 83 EC", 0x0);
MAKE_SIGNATURE(CEconItemView_GetItemName, "client.dll", "40 53 48 83 EC ? 48 8B D9 C6 81 ? ? ? ? ? E8 ? ? ? ? 48 8B 8B", 0x0);

static inline const char* GetWeaponNameByID(int iWeaponID, int iItemDefIndex)
{
	// First try to get name by specific item definition index for unique weapons
	switch (iItemDefIndex)
	{
	case Scout_m_ForceANature:
	case Scout_m_FestiveForceANature: { return "Force-A-Nature"; }
	case Scout_m_FestiveScattergun: { return "Scattergun"; }
	case Scout_m_BackcountryBlaster: { return "Back Scatter"; }
	case Scout_s_MutatedMilk: { return "Mad Milk"; }
	case Scout_s_TheWinger: { return "Winger"; }
	case Scout_s_FestiveBonk:
	case Scout_s_BonkAtomicPunch: { return "Bonk Atomic Punch"; }
	case Scout_s_PrettyBoysPocketPistol: { return "Pocket Pistol"; }
	case Scout_s_CritaCola: { return "Crit A Cola"; }
	case Scout_t_FestiveBat: { return "Bat"; }
	case Scout_t_FestiveHolyMackerel: { return "Holy Mackerel"; }
	case Scout_t_TheAtomizer: { return "Atomizer"; }
	case Scout_t_TheCandyCane: { return "Candy Cane"; }
	case Scout_t_TheFanOWar: { return "Fan O' War"; }
	case Scout_t_SunonaStick: { return "Sun On A Stick"; }
	case Scout_t_TheBostonBasher: { return "Boston Basher"; }
	case Scout_t_ThreeRuneBlade: { return "Three Rune Blade"; }
	case Scout_t_TheFreedomStaff: { return "Freedom Staff"; }
	case Scout_t_TheBatOuttaHell: { return "Bat Outta Hell"; }
	case Scout_s_Lugermorph:
	case Scout_s_VintageLugermorph: { return "Lugermorph"; }
	case Scout_s_TheCAPPER: { return "C.A.P.P.E.R"; }
	case Scout_t_UnarmedCombat: { return "Unarmed Combat"; }
	case Scout_t_Batsaber: { return "Batsaber"; }
	case Scout_t_TheHamShank: { return "Ham Shank"; }
	case Scout_t_TheNecroSmasher: { return "Necro Smasher"; }
	case Scout_t_TheConscientiousObjector: { return "Conscientious Objector"; }
	case Scout_t_TheCrossingGuard: { return "Crossing Guard"; }
	case Scout_t_TheMemoryMaker: { return "Memory Maker"; }

	case Soldier_m_FestiveRocketLauncher: { return "Rocket Launcher"; }
	case Soldier_m_RocketJumper: { return "Rocket Jumper"; }
	case Soldier_m_TheAirStrike: { return "Air Strike"; }
	case Soldier_m_TheLibertyLauncher: { return "Liberty Launcher"; }
	case Soldier_m_TheOriginal: { return "Original"; }
	case Soldier_m_FestiveBlackBox:
	case Soldier_m_TheBlackBox: { return "Black Box"; }
	case Soldier_m_TheBeggarsBazooka: { return "Beggar's Bazooka"; }
	case Soldier_s_FestiveShotgun: { return "Shotgun"; }
	case Soldier_s_FestiveBuffBanner: { return "Buff Banner"; }
	case Soldier_s_TheConcheror: { return "Concheror"; }
	case Soldier_s_TheBattalionsBackup: { return "Battalions Backup"; }
	case Soldier_s_PanicAttack: { return "Panic Attack"; }
	case Soldier_t_TheMarketGardener: { return "Market Gardener"; }
	case Soldier_t_TheDisciplinaryAction: { return "Disciplinary Action"; }
	case Soldier_t_TheEqualizer: { return "Equalizer"; }
	case Soldier_t_ThePainTrain: { return "Pain Train"; }
	case Soldier_t_TheHalfZatoichi: { return "Half Zatoichi"; }

	case Pyro_m_FestiveFlameThrower: { return "Flame Thrower"; }
	case Pyro_m_ThePhlogistinator: { return "Phlogistinator"; }
	case Pyro_m_FestiveBackburner:
	case Pyro_m_TheBackburner: { return "Backburner"; }
	case Pyro_m_TheRainblower: { return "Rainblower"; }
	case Pyro_m_TheDegreaser: { return "Degreaser"; }
	case Pyro_m_NostromoNapalmer: { return "Nostromo Napalmer"; }
	case Pyro_s_FestiveFlareGun: { return "Flare Gun"; }
	case Pyro_s_TheScorchShot: { return "Scorch Shot"; }
	case Pyro_s_TheDetonator: { return "Detonator"; }
	case Pyro_s_TheReserveShooter: { return "Reserve Shooter"; }
	case Pyro_t_TheFestiveAxtinguisher:
	case Pyro_t_TheAxtinguisher: { return "Axtinguisher"; }
	case Pyro_t_Homewrecker: { return "Homewrecker"; }
	case Pyro_t_ThePowerjack: { return "Powerjack"; }
	case Pyro_t_TheBackScratcher: { return "Back Scratcher"; }
	case Pyro_t_TheThirdDegree: { return "Third Degree"; }
	case Pyro_t_ThePostalPummeler: { return "Postal Pummeler"; }
	case Pyro_t_PrinnyMachete: { return "Prinny Machete"; }
	case Pyro_t_SharpenedVolcanoFragment: { return "Volcano Fragment"; }
	case Pyro_t_TheMaul: { return "Maul"; }
	case Pyro_t_TheLollichop: { return "Lollichop"; }

	case Demoman_m_FestiveGrenadeLauncher: { return "Grenade Launcher"; }
	case Demoman_m_TheIronBomber: { return "Iron Bomber"; }
	case Demoman_m_TheLochnLoad: { return "Loch-n-Load"; }
	case Demoman_s_FestiveStickybombLauncher: { return "Stickybomb Launcher"; }
	case Demoman_s_StickyJumper: { return "Sticky Jumper"; }
	case Demoman_s_TheQuickiebombLauncher: { return "Quickiebomb Launcher"; }
	case Demoman_s_TheScottishResistance: { return "Scottish Resistance"; }
	case Demoman_t_HorselessHeadlessHorsemannsHeadtaker: { return "Horsemann's Headtaker"; }
	case Demoman_t_TheScottishHandshake: { return "Scottish Handshake"; }
	case Demoman_t_FestiveEyelander:
	case Demoman_t_TheEyelander: { return "Eyelander"; }
	case Demoman_t_TheScotsmansSkullcutter: { return "Scotsman's Skullcutter"; }
	case Demoman_t_ThePersianPersuader: { return "Persian Persuader"; }
	case Demoman_t_NessiesNineIron: { return "Nessie's Nine Iron"; }
	case Demoman_t_TheClaidheamhMor: { return "Claidheamh Mòr"; }

	case Heavy_m_IronCurtain: { return "Iron Curtain"; }
	case Heavy_m_FestiveMinigun: { return "Minigun"; }
	case Heavy_m_Tomislav: { return "Tomislav"; }
	case Heavy_m_TheBrassBeast: { return "Brass Beast"; }
	case Heavy_m_Natascha: { return "Natascha"; }
	case Heavy_m_TheHuoLongHeaterG:
	case Heavy_m_TheHuoLongHeater: { return "Huo-Long Heater"; }
	case Heavy_s_TheFamilyBusiness: { return "Family Business"; }
	case Heavy_s_FestiveSandvich:
	case Heavy_s_RoboSandvich:
	case Heavy_s_Sandvich: { return "Sandvich"; }
	case Heavy_s_Fishcake: { return "Fishcake"; }
	case Heavy_s_SecondBanana: { return "Second Banana"; }
	case Heavy_s_TheDalokohsBar: { return "Dalokohs Bar"; }
	case Heavy_s_TheBuffaloSteakSandvich: { return "Buffalo Steak Sandvich"; }
	case Heavy_t_FistsofSteel: { return "Fists of Steel"; }
	case Heavy_t_TheHolidayPunch: { return "Holiday Punch"; }
	case Heavy_t_WarriorsSpirit: { return "Warrior's Spirit"; }
	case Heavy_t_TheEvictionNotice: { return "Eviction Notice"; }
	case Heavy_t_TheKillingGlovesofBoxing: { return "Killing Gloves of Boxing"; }
	case Heavy_t_ApocoFists: { return "Apoco-Fists"; }
	case Heavy_t_FestiveGlovesofRunningUrgently:
	case Heavy_t_GlovesofRunningUrgently: { return "Gloves of Running Urgently"; }
	case Heavy_t_TheBreadBite: { return "Bread Bite"; }

	case Engi_m_FestiveFrontierJustice: { return "Frontier Justice"; }
	case Engi_m_TheWidowmaker: { return "Widowmaker"; }
	case Engi_s_TheGigarCounter:
	case Engi_s_FestiveWrangler: { return "Wrangler"; }
	case Engi_s_TheShortCircuit: { return "Short Circuit"; }
	case Engi_t_FestiveWrench: { return "Wrench"; }
	case Engi_t_GoldenWrench: { return "Golden Wrench"; }
	case Engi_t_TheGunslinger: { return "Gunslinger"; }
	case Engi_t_TheJag: { return "Jag"; }
	case Engi_t_TheEurekaEffect: { return "Eureka Effect"; }
	case Engi_t_TheSouthernHospitality: { return "Southern Hospitality"; }

	case Medic_m_FestiveCrusadersCrossbow: { return "Crusader's Crossbow"; }
	case Medic_m_TheOverdose: { return "Overdose"; }
	case Medic_m_TheBlutsauger: { return "Blutsauger"; }
	case Medic_s_FestiveMediGun: { return "Medi Gun"; }
	case Medic_s_TheQuickFix: { return "Quick-Fix"; }
	case Medic_s_TheKritzkrieg: { return "Kritzkrieg"; }
	case Medic_s_TheVaccinator: { return "Vaccinator"; }
	case Medic_t_FestiveBonesaw: { return "Bonesaw"; }
	case Medic_t_FestiveUbersaw:
	case Medic_t_TheUbersaw: { return "Ubersaw"; }
	case Medic_t_TheVitaSaw: { return "Vita-Saw"; }
	case Medic_t_TheSolemnVow: { return "Solemn Vow"; }
	case Medic_t_Amputator: { return "Amputator"; }

	case Sniper_m_FestiveSniperRifle: { return "Sniper Rifle"; }
	case Sniper_m_FestiveHuntsman:
	case Sniper_m_TheHuntsman: { return "Huntsman"; }
	case Sniper_m_TheMachina: { return "Machina"; }
	case Sniper_m_TheAWPerHand: { return "AWPer Hand"; }
	case Sniper_m_TheHitmansHeatmaker: { return "Hitman's Heatmaker"; }
	case Sniper_m_TheSydneySleeper: { return "Sydney Sleeper"; }
	case Sniper_m_ShootingStar: { return "Shooting Star"; }
	case Sniper_s_FestiveJarate: { return "Jarate"; }
	case Sniper_s_TheSelfAwareBeautyMark: { return "Jarate"; }
	case Sniper_s_FestiveSMG: { return "SMG"; }
	case Sniper_t_TheBushwacka: { return "Bushwacka"; }
	case Sniper_t_KukriR:
	case Sniper_t_Kukri: { return "Kukri"; }
	case Sniper_t_TheShahanshah: { return "Shahanshah"; }
	case Sniper_t_TheTribalmansShiv: { return "Tribalman's Shiv"; }

	case Spy_m_FestiveRevolver: { return "Revolver"; }
	case Spy_m_FestiveAmbassador:
	case Spy_m_TheAmbassador: { return "Ambassador"; }
	case Spy_m_BigKill: { return "Big Kill"; }
	case Spy_m_TheDiamondback: { return "Diamondback"; }
	case Spy_m_TheEnforcer: { return "Enforcer"; }
	case Spy_m_LEtranger: { return "L'Etranger"; }
	case Spy_s_Sapper:
	case Spy_s_SapperR:
	case Spy_s_FestiveSapper: { return "Sapper"; }
	case Spy_s_TheRedTapeRecorder:
	case Spy_s_TheRedTapeRecorderG: { return "Red-Tape Recorder"; }
	case Spy_s_TheApSapG: { return "AP-Sap"; }
	case Spy_s_TheSnackAttack: { return "Snack Attack"; }
	case Spy_t_FestiveKnife: { return "Knife"; }
	case Spy_t_ConniversKunai: { return "Conniver's Kunai"; }
	case Spy_t_YourEternalReward: { return "Your Eternal Reward"; }
	case Spy_t_TheBigEarner: { return "Big Earner"; }
	case Spy_t_TheSpycicle: { return "Spycicle"; }
	case Spy_t_TheSharpDresser: { return "Sharp Dresser"; }
	case Spy_t_TheWangaPrick: { return "Wanga Prick"; }
	case Spy_t_TheBlackRose: { return "Black Rose"; }

	case Heavy_m_Deflector_mvm: { return "Deflector"; }
	case Misc_t_FryingPan: { return "Frying Pan"; }
	case Misc_t_GoldFryingPan: { return "Golden Frying Pan"; }
	case Misc_t_Saxxy: { return "Saxxy"; }
	}

	switch (iWeaponID)
	{
		//scout
	case TF_WEAPON_SCATTERGUN: { return "Scattergun"; }
	case TF_WEAPON_HANDGUN_SCOUT_PRIMARY: { return "Shortstop"; }
	case TF_WEAPON_HANDGUN_SCOUT_SECONDARY: { return "Pistol"; }
	case TF_WEAPON_SODA_POPPER: { return "Soda Popper"; }
	case TF_WEAPON_PEP_BRAWLER_BLASTER: { return "Baby Face's Blaster"; }
	case TF_WEAPON_PISTOL_SCOUT: { return "Pistol"; }
	case TF_WEAPON_JAR_MILK: { return "Mad Milk"; }
	case TF_WEAPON_CLEAVER: { return "Cleaver"; }
	case TF_WEAPON_BAT: { return "Bat"; }
	case TF_WEAPON_BAT_WOOD: { return "Sandman"; }
	case TF_WEAPON_BAT_FISH: { return "Holy Mackerel"; }
	case TF_WEAPON_BAT_GIFTWRAP: { return "Wrap Assassin"; }

							   //soldier
	case TF_WEAPON_ROCKETLAUNCHER: { return "Rocket Launcher"; }
	case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT: { return "Direct Hit"; }
	case TF_WEAPON_PARTICLE_CANNON: { return "Cow Mangler 5000"; }
	case TF_WEAPON_SHOTGUN_SOLDIER: { return "Shotgun"; }
	case TF_WEAPON_BUFF_ITEM: { return "Buff Banner"; }
	case TF_WEAPON_RAYGUN: { return "Righteous Bison"; }
	case TF_WEAPON_SHOVEL: { return "Shovel"; }

						 //pyro
	case TF_WEAPON_FLAMETHROWER: { return "Flame Thrower"; }
	case TF_WEAPON_FLAME_BALL: { return "Dragon's Fury"; }
	case TF_WEAPON_SHOTGUN_PYRO: { return "Shotgun"; }
	case TF_WEAPON_FLAREGUN: { return "Flaregun"; }
	case TF_WEAPON_FLAREGUN_REVENGE: { return "Manmelter"; }
	case TF_WEAPON_JAR_GAS: { return "Gas Passer"; }
	case TF_WEAPON_ROCKETPACK: { return "Thermal Thruster"; }
	case TF_WEAPON_FIREAXE: { return "Fire Axe"; }
	case TF_WEAPON_BREAKABLE_SIGN: { return "Neon Annihilator"; }
	case TF_WEAPON_SLAP: { return "Hot Hand"; }

					   //demoman
	case TF_WEAPON_GRENADELAUNCHER: { return "Grenade Launcher"; }
	case TF_WEAPON_PIPEBOMBLAUNCHER: { return "Stickybomb Launcher"; }
	case TF_WEAPON_CANNON: { return "Loose Cannon"; }
	case TF_WEAPON_BOTTLE: { return "Bottle"; }
	case TF_WEAPON_SWORD: { return "Sword"; }
	case TF_WEAPON_STICKBOMB: { return "Ullapool Caber"; }

							//heavy
	case TF_WEAPON_MINIGUN: { return "Minigun"; }
	case TF_WEAPON_SHOTGUN_HWG: { return "Shotgun"; }
	case TF_WEAPON_LUNCHBOX: { return "Lunchbox"; }
	case TF_WEAPON_FISTS: { return "Fists"; }

						//engineer
	case TF_WEAPON_SHOTGUN_PRIMARY: { return "Shotgun"; }
	case TF_WEAPON_SHOTGUN_BUILDING_RESCUE: { return "Rescue Ranger"; }
	case TF_WEAPON_SENTRY_REVENGE: { return "Frontier Justice"; }
	case TF_WEAPON_DRG_POMSON: { return "Pomson 6000"; }
	case TF_WEAPON_PISTOL: { return "Pistol"; }
	case TF_WEAPON_LASER_POINTER: { return "Wrangler"; }
	case TF_WEAPON_MECHANICAL_ARM: { return "Mechanical Arm"; }
	case TF_WEAPON_WRENCH: { return "Wrench"; }
	case TF_WEAPON_PDA_ENGINEER_DESTROY: { return "Destruction PDA"; }
	case TF_WEAPON_PDA_ENGINEER_BUILD: { return "Construction PDA"; }
	case TF_WEAPON_BUILDER: { return "Toolbox"; }

						  //medic
	case TF_WEAPON_SYRINGEGUN_MEDIC: { return "Syringe Gun"; }
	case TF_WEAPON_CROSSBOW: { return "Crossbow"; }
	case TF_WEAPON_MEDIGUN: { return "Medi Gun"; }
	case TF_WEAPON_BONESAW: { return "Bonesaw"; }

						  //sniper
	case TF_WEAPON_SNIPERRIFLE: { return "Sniper Rifle"; }
	case TF_WEAPON_COMPOUND_BOW: { return "Compound Bow"; }
	case TF_WEAPON_SNIPERRIFLE_DECAP: { return "Bazaar Bargain"; }
	case TF_WEAPON_SNIPERRIFLE_CLASSIC: { return "Classic"; }
	case TF_WEAPON_SMG: { return "SMG"; }
	case TF_WEAPON_CHARGED_SMG: { return "Cleaner's Carbine"; }
	case TF_WEAPON_JAR: { return "Jarate"; }
	case TF_WEAPON_CLUB: { return "Club"; }

					   //spy
	case TF_WEAPON_REVOLVER: { return "Revolver"; }
	case TF_WEAPON_PDA_SPY_BUILD: { return "Sapper"; }
	case TF_WEAPON_KNIFE: { return "Knife"; }
	case TF_WEAPON_PDA_SPY: { return "Disguise Kit"; }
	case TF_WEAPON_INVIS: { return "Invis Watch"; }

	case TF_WEAPON_GRAPPLINGHOOK: { return "Grappling Hook"; }

	default: break;
	}
	return "Unknown";
}

static inline void StorePlayer(CTFPlayer* pPlayer, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, PlayerCache_t>& mCache)
{
	int iIndex = pPlayer->entindex();

	if (int iObserverMode = pLocal->m_iObserverMode(); iObserverMode == OBS_MODE_FIRSTPERSON || iObserverMode == OBS_MODE_THIRDPERSON
		? iObserverMode == OBS_MODE_FIRSTPERSON && pLocal->m_hObserverTarget().GetEntryIndex() == iIndex
		: !I::Input->CAM_IsThirdPerson() && iIndex == I::EngineClient->GetLocalPlayer())
		return;

	auto pWeapon = pPlayer->m_hActiveWeapon()->As<CTFWeaponBase>();
	auto pResource = H::Entities.GetResource();
	bool bLocal = pPlayer->entindex() == I::EngineClient->GetLocalPlayer();
	int iClassNum = pPlayer->m_iClass();

	PlayerCache_t& tCache = mCache[pPlayer];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = F::Groups.GetColor(pPlayer, pGroup).Alpha(255);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;
	tCache.m_bBones = pGroup->m_iESP & ESPEnum::Bones;

	if (pGroup->m_iESP & ESPEnum::Distance && !bLocal)
	{
		Vec3 vDelta = pPlayer->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pResource)
	{
		if (pGroup->m_iESP & ESPEnum::Name)
			tCache.m_vText.emplace_back(ALIGN_TOP, F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pGroup->m_iESP & (ESPEnum::Labels | ESPEnum::Priority) && !pResource->IsFakePlayer(iIndex))
		{
			uint32_t uAccountID = pResource->m_iAccountID(iIndex);

			if (pGroup->m_iESP & ESPEnum::Priority)
			{
				std::vector<std::tuple<std::string, Color_t, int>> vPriorityTags = {};

				// Get all priority tags (non-label tags) from player
				for (auto& iID : F::PlayerUtils.GetPlayerTags(uAccountID))
				{
					auto pTag = F::PlayerUtils.GetTag(iID);
					if (pTag && !pTag->m_bLabel)
						vPriorityTags.emplace_back(pTag->m_sName, pTag->m_tColor, pTag->m_iPriority);
				}
				// Add default priority tags (Friend, Party, F2P) if they're not labels
				if (H::Entities.IsFriend(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(FRIEND_TAG)];
					if (!pTag->m_bLabel)
						vPriorityTags.emplace_back(pTag->m_sName, pTag->m_tColor, pTag->m_iPriority);
				}
				if (auto iParty = H::Entities.GetParty(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(PARTY_TAG)];
					if (int iPartyCount = H::Entities.GetPartyCount() + 1; !pTag->m_bLabel)
					{
						if (iParty == 1)
						{
							vPriorityTags.emplace_back(pTag->m_sName, pTag->m_tColor, pTag->m_iPriority);
						}
						else if (Vars::Colors::PartyESP.Value)
						{
							vPriorityTags.emplace_back(
								std::format("{}: {}", pTag->m_sName, iParty - 1),
								pTag->m_tColor.HueShift((iParty - 1) * 360.f / iPartyCount),
								pTag->m_iPriority
							);
						}
					}
				}
				if (H::Entities.IsF2P(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(F2P_TAG)];
					if (!pTag->m_bLabel)
						vPriorityTags.emplace_back(pTag->m_sName, pTag->m_tColor, pTag->m_iPriority);
				}

				// Sort by priority ascending (lower priority number first)
				if (!vPriorityTags.empty())
				{
					std::sort(vPriorityTags.begin(), vPriorityTags.end(), [&](const auto a, const auto b) -> bool
						{
							return std::get<2>(a) < std::get<2>(b);
						});

					for (auto& [sName, tColor, _] : vPriorityTags)
						// Store priority tags in a separate vector to be drawn after weapon info
						tCache.m_vPriorityText.emplace_back(sName, tColor);
				}
			}
			if (pGroup->m_iESP & ESPEnum::Labels)
			{
				std::vector<std::tuple<std::string, Color_t, int>> vLabels = {};

				for (auto& iID : F::PlayerUtils.GetPlayerTags(uAccountID))
				{
					auto pTag = F::PlayerUtils.GetTag(iID);
					if (pTag && pTag->m_bLabel)
						vLabels.emplace_back(pTag->m_sName, pTag->m_tColor, pTag->m_iPriority);
				}
				if (H::Entities.IsFriend(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(FRIEND_TAG)];
					if (pTag->m_bLabel)
						vLabels.emplace_back(pTag->m_sName, pTag->m_tColor, pTag->m_iPriority);
				}
				if (auto iParty = H::Entities.GetParty(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(PARTY_TAG)];
					if (int iPartyCount = H::Entities.GetPartyCount() + 1; pTag->m_bLabel)
					{
						if (iParty == 1)
						{
							vLabels.emplace_back(pTag->m_sName, pTag->m_tColor, pTag->m_iPriority);
						}
						else if (Vars::Colors::PartyESP.Value)
						{
							vLabels.emplace_back(
								std::format("{}: {}", pTag->m_sName, iParty - 1),
								pTag->m_tColor.HueShift((iParty - 1) * 360.f / iPartyCount),
								pTag->m_iPriority
							);
						}
					}
				}
				if (H::Entities.IsF2P(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(F2P_TAG)];
					if (pTag->m_bLabel)
						vLabels.emplace_back(pTag->m_sName, pTag->m_tColor, pTag->m_iPriority);
				}

				if (!vLabels.empty())
				{
					std::sort(vLabels.begin(), vLabels.end(), [&](const auto a, const auto b) -> bool
						{
							if (std::get<2>(a) != std::get<2>(b))
								return std::get<2>(a) > std::get<2>(b);
							return std::get<0>(a) < std::get<0>(b);
						});

					for (auto& [sName, tColor, _] : vLabels)
						tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, sName, tColor, tColor.IsColorDark() ? Color_t(255, 255, 255) : Color_t(0, 0, 0));
				}
			}
		}
	}

	float flHealth = pPlayer->m_iHealth(), flMaxHealth = pPlayer->GetMaxHealth();
	if (pGroup->m_iESP & ESPEnum::HealthBar)
	{
		tCache.m_flHealth = flHealth > flMaxHealth
			? 1.f + std::clamp((flHealth - flMaxHealth) / (floorf(flMaxHealth / 10.f) * 5), 0.f, 1.f)
			: std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

		Color_t tColor;

		float flHealthClamped = std::clamp(tCache.m_flHealth, 0.f, 1.f);

		if (flHealthClamped < 0.5f)
		{
			// 0% to 50%: Lerp from Bad to Mid
			tColor = Vars::Colors::IndicatorBad.Value.Lerp(
				Vars::Colors::IndicatorMid.Value,
				flHealthClamped / 0.5f);
		}
		else
		{
			// 50% to 100%: Lerp from Mid to Good
			tColor = Vars::Colors::IndicatorMid.Value.Lerp(
				Vars::Colors::IndicatorGood.Value,
				(flHealthClamped - 0.5f) / 0.5f);
		}

		tCache.m_vBars.emplace_back(ALIGN_LEFT, tCache.m_flHealth, tColor, Vars::Colors::IndicatorMisc.Value);
	}
	if (pGroup->m_iESP & ESPEnum::HealthText)
		tCache.m_vText.emplace_back(ALIGN_LEFT, std::format("{}", flHealth), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

	if (pGroup->m_iESP & (ESPEnum::UberBar | ESPEnum::UberText) && iClassNum == TF_CLASS_MEDIC)
	{
		auto pMediGun = pPlayer->GetWeaponFromSlot(SLOT_SECONDARY);
		if (pMediGun && pMediGun->GetClassID() == ETFClassID::CWeaponMedigun)
		{
			float flUber = std::clamp(pMediGun->As<CWeaponMedigun>()->m_flChargeLevel(), 0.f, 1.f);
			if (pGroup->m_iESP & ESPEnum::UberBar)
				tCache.m_vBars.emplace_back(ALIGN_BOTTOM, flUber, Color_t(255, 0, 255, 255), Color_t(), false);
			if (pGroup->m_iESP & ESPEnum::UberText)
				tCache.m_vText.emplace_back(ALIGN_BOTTOMRIGHT, std::format("{:.0f}%", flUber * 100), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		}
	}

	if (pGroup->m_iESP & ESPEnum::ClassIcon)
		tCache.m_iClassIcon = iClassNum;
	if (pGroup->m_iESP & ESPEnum::ClassText)
		tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, SDK::GetClassByIndex(iClassNum, false), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

	if (pGroup->m_iESP & ESPEnum::WeaponIcon && pWeapon)
		tCache.m_pWeaponIcon = pWeapon->GetWeaponIcon();
	if (pGroup->m_iESP & ESPEnum::WeaponText && pWeapon)
	{
		const char* szWeaponName = GetWeaponNameByID(pWeapon->GetWeaponID(), pWeapon->m_iItemDefinitionIndex());
		tCache.m_vText.emplace_back(ALIGN_BOTTOM, szWeaponName, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pGroup->m_iESP & ESPEnum::LagCompensation && !pPlayer->IsDormant() && !bLocal)
	{
		if (H::Entities.GetLagCompensation(iIndex))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Lagcomp", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pGroup->m_iESP & ESPEnum::Ping && pResource && !bLocal)
	{
		int iPing = pResource->m_iPing(iIndex);
		if (iPing && (iPing >= 200 || iPing <= 5))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("{} ms", iPing), Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pGroup->m_iESP & ESPEnum::KDR && pResource && !bLocal)
	{
		int iKills = pResource->m_iScore(iIndex), iDeaths = pResource->m_iDeaths(iIndex);
		if (iKills >= 20)
		{
			int iKDR = iKills / std::max(iDeaths, 1);
			if (iKDR >= 10)
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("High KD [{} / {}]", iKills, iDeaths), Vars::Colors::IndicatorTextMid.Value, Vars::Menu::Theme::Background.Value);
		}
	}

	// Buffs
	if (pGroup->m_iESP & ESPEnum::Buffs)
	{
		if (pPlayer->InCond(TF_COND_INVULNERABLE) ||
			pPlayer->InCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED) ||
			pPlayer->InCond(TF_COND_INVULNERABLE_USER_BUFF) ||
			pPlayer->InCond(TF_COND_INVULNERABLE_CARD_EFFECT))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Uber", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
		else if (pPlayer->InCond(TF_COND_MEGAHEAL))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Megaheal", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
		else if (pPlayer->InCond(TF_COND_PHASE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Bonk", Vars::Colors::IndicatorTextMid.Value, Vars::Menu::Theme::Background.Value);

		bool bCrits = pPlayer->IsCritBoosted(), bMiniCrits = pPlayer->IsMiniCritBoosted();
		if (pWeapon)
		{
			if (bMiniCrits && SDK::AttribHookValue(0, "minicrits_become_crits", pWeapon)
				|| SDK::AttribHookValue(0, "crit_while_airborne", pWeapon) && pPlayer->InCond(TF_COND_BLASTJUMPING))
				bCrits = true, bMiniCrits = false;
			if (bCrits && SDK::AttribHookValue(0, "crits_become_minicrits", pWeapon))
				bCrits = false, bMiniCrits = true;
		}
		if (bCrits)
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Crits", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
		else if (bMiniCrits)
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Mini-crits", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);

		/* vaccinator effects */
		if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST) || pPlayer->InCond(TF_COND_BULLET_IMMUNE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Bullet++", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
		else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_BULLET_RESIST))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Bullet+", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BLAST_RESIST) || pPlayer->InCond(TF_COND_BLAST_IMMUNE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Blast++", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
		else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_BLAST_RESIST))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Blast+", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_FIRE_RESIST) || pPlayer->InCond(TF_COND_FIRE_IMMUNE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Fire++", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
		else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_FIRE_RESIST))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Fire+", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_OFFENSEBUFF))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Banner", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_DEFENSEBUFF))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Battalions", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_REGENONDAMAGEBUFF))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Conch", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_RUNE_STRENGTH))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Strength", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_HASTE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Haste", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_REGEN))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Regen", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_RESIST))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Resistance", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_VAMPIRE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Vampire", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_REFLECT))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Reflect", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_PRECISION))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Precision", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_AGILITY))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Agility", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_KNOCKOUT))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Knockout", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_KING))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "King", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_PLAGUE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Plague", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_RUNE_SUPERNOVA))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Supernova", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		if (pPlayer->InCond(TF_COND_POWERUPMODE_DOMINANT))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Dominant", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			auto pWeapon = pPlayer->GetWeaponFromSlot(i)->As<CTFSpellBook>();
			if (!pWeapon || pWeapon->GetWeaponID() != TF_WEAPON_SPELLBOOK || !pWeapon->m_iSpellCharges())
				continue;

			switch (pWeapon->m_iSelectedSpellIndex())
			{
			case 0: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Fireball", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 1: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Bats", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 2: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Heal", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 3: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Pumpkins", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 4: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Jump", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 5: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Stealth", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 6: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Teleport", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 7: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Lightning", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 8: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Minify", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 9: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Meteors", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 10: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Monoculus", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 11: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Skeletons", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 12: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Glove", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 13: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Parachute", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 14: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Heal", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			case 15: tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Bomb", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value); break;
			}
		}

		if (pPlayer->InCond(TF_COND_RADIUSHEAL) ||
			pPlayer->InCond(TF_COND_HEALTH_BUFF) ||
			pPlayer->InCond(TF_COND_RADIUSHEAL_ON_DAMAGE) ||
			pPlayer->InCond(TF_COND_HALLOWEEN_QUICK_HEAL) ||
			pPlayer->InCond(TF_COND_HALLOWEEN_HELL_HEAL) ||
			pPlayer->InCond(TF_COND_KING_BUFFED))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "HP++", Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value);
		else if (pPlayer->InCond(TF_COND_HEALTH_OVERHEALED))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "HP+", Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value);

		//if (pPlayer->InCond(TF_COND_BLASTJUMPING))
		//	tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Blastjump", Vars::Colors::IndicatorTextMid.Value, Vars::Menu::Theme::Background.Value);
	}

	// Debuffs
	if (pGroup->m_iESP & ESPEnum::Debuffs)
	{
		if (pPlayer->InCond(TF_COND_MARKEDFORDEATH)
			|| pPlayer->InCond(TF_COND_MARKEDFORDEATH_SILENT)
			|| pPlayer->InCond(TF_COND_PASSTIME_PENALTY_DEBUFF))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Marked", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_URINE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Jarate", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_MAD_MILK))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Milk", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_STUNNED))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Stun", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_BURNING))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Burn", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_BLEEDING))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Bleed", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	// Misc
	if (pGroup->m_iESP & ESPEnum::Flags)
	{
		if (pPlayer->m_bFeignDeathReady())
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Deadringer", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		else if (pPlayer->InCond(TF_COND_FEIGN_DEATH))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Invisible", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (float flInvis = pPlayer->GetEffectiveInvisibilityLevel())
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("Invisible {:.0f}%", flInvis * 100), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_DISGUISED))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Disguised", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pPlayer->InCond(TF_COND_AIMING) || pPlayer->InCond(TF_COND_ZOOMED))
		{
			switch (pWeapon ? pWeapon->GetWeaponID() : -1)
			{
			case TF_WEAPON_MINIGUN:
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Revved", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				break;
			case TF_WEAPON_SNIPERRIFLE:
			case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			case TF_WEAPON_SNIPERRIFLE_DECAP:
			{
				if (bLocal)
				{
					tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("Charge {:.0f}%", Math::RemapVal(pWeapon->As<CTFSniperRifle>()->m_flChargedDamage(), 0.f, 150.f, 0.f, 100.f)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
					break;
				}
				else
				{
					auto fGetSniperDot = [](CBaseEntity* pEntity) -> CSniperDot*
						{
							for (auto pDot : H::Entities.GetGroup(EntityEnum::SniperDots))
							{
								if (pDot->m_hOwnerEntity().Get() == pEntity)
									return pDot->As<CSniperDot>();
							}
							return nullptr;
						};
					if (CSniperDot* pPlayerDot = fGetSniperDot(pPlayer))
					{
						float flChargeTime = std::max(SDK::AttribHookValue(3.f, "mult_sniper_charge_per_sec", pWeapon), 1.5f);
						tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("Charge {:.0f}%", Math::RemapVal(TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick) - pPlayerDot->m_flChargeStartTime() - 0.3f, 0.f, flChargeTime, 0.f, 100.f)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
						break;
					}
				}
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Zoomed", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				break;
			}
			case TF_WEAPON_COMPOUND_BOW:
				if (bLocal)
				{
					tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("Charging {:.0f}%", Math::RemapVal(TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick) - pWeapon->As<CTFPipebombLauncher>()->m_flChargeBeginTime(), 0.f, 1.f, 0.f, 100.f)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
					break;
				}
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Charging", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				break;
			default:
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Charging", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
			}
		}

		if (pPlayer->InCond(TF_COND_SHIELD_CHARGE))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Charging", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (Vars::Visuals::Removals::Taunts.Value && pPlayer->InCond(TF_COND_TAUNTING))
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Taunt", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (Vars::Debug::Info.Value && !pPlayer->IsDormant() && !bLocal)
		{
			int iAverage = TIME_TO_TICKS(F::MoveSim.GetPredictedDelta(pPlayer));
			int iCurrent = H::Entities.GetChoke(iIndex);
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("Lag {}, {}", iAverage, iCurrent), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		}
	}
}

static inline void StoreBuilding(CBaseObject* pBuilding, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, BuildingCache_t>& mCache)
{
	auto pOwner = pBuilding->m_hBuilder().Get();
	int iIndex = pOwner ? pOwner->entindex() : -1;

	bool bIsMini = pBuilding->m_bMiniBuilding();

	BuildingCache_t& tCache = mCache[pBuilding];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = F::Groups.GetColor(pOwner ? pOwner : pBuilding, pGroup).Alpha(255);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;

	if (pGroup->m_iESP & ESPEnum::Distance)
	{
		Vec3 vDelta = pBuilding->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pGroup->m_iESP & ESPEnum::Name)
	{
		const char* sName = "Building";
		switch (pBuilding->GetClassID())
		{
		case ETFClassID::CObjectSentrygun: sName = bIsMini ? "Mini-Sentry" : "Sentrygun"; break;
		case ETFClassID::CObjectDispenser: sName = "Dispenser"; break;
		case ETFClassID::CObjectTeleporter: sName = pBuilding->m_iObjectMode() ? "Teleport Exit" : "Teleport Entrance"; break;
		}
		std::string sDisplayName = sName;
		if (pGroup->m_iESP & ESPEnum::Level && !bIsMini)
			sDisplayName += std::format(" (lvl {})", pBuilding->m_iUpgradeLevel());

		tCache.m_vText.emplace_back(ALIGN_TOP, sDisplayName, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	float flHealth = pBuilding->m_iHealth(), flMaxHealth = pBuilding->m_iMaxHealth();
	if (pGroup->m_iESP & ESPEnum::HealthBar)
	{
		tCache.m_flHealth = flHealth > flMaxHealth
			? 1.f + std::clamp((flHealth - flMaxHealth) / (floorf(flMaxHealth / 10.f) * 5), 0.f, 1.f)
			: std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

		Color_t tColor = Vars::Colors::IndicatorBad.Value.Lerp(Vars::Colors::IndicatorGood.Value, std::clamp(tCache.m_flHealth, 0.f, 1.f));
		tCache.m_vBars.emplace_back(ALIGN_LEFT, tCache.m_flHealth, tColor, Vars::Colors::IndicatorMisc.Value);
	}
	if (pGroup->m_iESP & ESPEnum::HealthText)
		tCache.m_vText.emplace_back(ALIGN_LEFT, std::format("{}", flHealth), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

	if (pGroup->m_iESP & (ESPEnum::AmmoBars | ESPEnum::AmmoText) && pBuilding->IsSentrygun() && !pBuilding->m_bBuilding())
	{
		int iShells, iMaxShells, iRockets, iMaxRockets; pBuilding->As<CObjectSentrygun>()->GetAmmoCount(iShells, iMaxShells, iRockets, iMaxRockets);

		if (pGroup->m_iESP & ESPEnum::AmmoBars)
		{
			tCache.m_vBars.emplace_back(ALIGN_BOTTOM, float(iShells) / iMaxShells, Vars::Colors::SentryAmmo.Value, Color_t(), false);
			if (iMaxRockets)
				tCache.m_vBars.emplace_back(ALIGN_BOTTOM, float(iRockets) / iMaxRockets, Vars::Colors::SentryRockets.Value, Color_t(), false);
		}
		if (pGroup->m_iESP & ESPEnum::AmmoText)
		{
			tCache.m_vText.emplace_back(ALIGN_BOTTOMRIGHT, std::format("{}", iShells), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
			if (iMaxRockets)
				tCache.m_vText.back().m_sText += std::format(", {}", iRockets);
		}
	}

	if (pGroup->m_iESP & ESPEnum::Owner && !pBuilding->m_bWasMapPlaced() && pOwner)
	{
		if (auto pResource = H::Entities.GetResource(); pResource)
			tCache.m_vText.emplace_back(ALIGN_TOP, F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pGroup->m_iESP & ESPEnum::Flags)
	{
		if (!pBuilding->IsDormant() && pBuilding->m_bBuilding())
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("{:.0f}%", pBuilding->m_flPercentageConstructed() * 100), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pBuilding->IsSentrygun() && pBuilding->As<CObjectSentrygun>()->m_bPlayerControlled())
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Wrangled", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);

		if (pBuilding->m_bHasSapper())
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Sapped", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		else if (pBuilding->m_bDisabled())
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Disabled", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}
}

static inline const char* GetProjectileName(CBaseEntity* pProjectile)
{
	const char* sReturn = "Projectile";
	switch (pProjectile->GetClassID())
	{
	case ETFClassID::CTFWeaponBaseMerasmusGrenade: sReturn = "Bomb"; break;
	case ETFClassID::CTFGrenadePipebombProjectile: sReturn = pProjectile->As<CTFGrenadePipebombProjectile>()->HasStickyEffects() ? "Sticky" : "Pipe"; break;
	case ETFClassID::CTFStunBall: sReturn = "Baseball"; break;
	case ETFClassID::CTFBall_Ornament: sReturn = "Bauble"; break;
	case ETFClassID::CTFProjectile_Jar: sReturn = "Jarate"; break;
	case ETFClassID::CTFProjectile_Cleaver: sReturn = "Cleaver"; break;
	case ETFClassID::CTFProjectile_JarGas: sReturn = "Gas"; break;
	case ETFClassID::CTFProjectile_JarMilk:
	case ETFClassID::CTFProjectile_ThrowableBreadMonster: sReturn = "Milk"; break;
	case ETFClassID::CTFProjectile_SpellBats:
	case ETFClassID::CTFProjectile_SpellKartBats: sReturn = "Bats"; break;
	case ETFClassID::CTFProjectile_SpellMeteorShower: sReturn = "Meteors"; break;
	case ETFClassID::CTFProjectile_SpellMirv:
	case ETFClassID::CTFProjectile_SpellPumpkin: sReturn = "Pumpkin"; break;
	case ETFClassID::CTFProjectile_SpellSpawnBoss: sReturn = "Monoculus"; break;
	case ETFClassID::CTFProjectile_SpellSpawnHorde:
	case ETFClassID::CTFProjectile_SpellSpawnZombie: sReturn = "Skeleton"; break;
	case ETFClassID::CTFProjectile_SpellTransposeTeleport: sReturn = "Teleport"; break;
	case ETFClassID::CTFProjectile_Arrow: sReturn = pProjectile->As<CTFProjectile_Arrow>()->m_iProjectileType() == TF_PROJECTILE_BUILDING_REPAIR_BOLT ? "Repair" : "Arrow"; break;
	case ETFClassID::CTFProjectile_GrapplingHook: sReturn = "Grapple"; break;
	case ETFClassID::CTFProjectile_HealingBolt: sReturn = "Heal"; break;
	case ETFClassID::CTFProjectile_Rocket:
	case ETFClassID::CTFProjectile_EnergyBall:
	case ETFClassID::CTFProjectile_SentryRocket: sReturn = "Rocket"; break;
	case ETFClassID::CTFProjectile_BallOfFire: sReturn = "Fire"; break;
	case ETFClassID::CTFProjectile_MechanicalArmOrb: sReturn = "Short circuit"; break;
	case ETFClassID::CTFProjectile_SpellFireball: sReturn = "Fireball"; break;
	case ETFClassID::CTFProjectile_SpellLightningOrb: sReturn = "Lightning"; break;
	case ETFClassID::CTFProjectile_SpellKartOrb: sReturn = "Fist"; break;
	case ETFClassID::CTFProjectile_Flare: sReturn = "Flare"; break;
	case ETFClassID::CTFProjectile_EnergyRing: sReturn = "Energy"; break;
	}
	return sReturn;
}
static inline void StoreProjectile(CBaseEntity* pProjectile, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, EntityCache_t>& mCache)
{
	auto pOwner = F::ProjSim.GetEntities(pProjectile).second;
	int iIndex = pOwner ? pOwner->entindex() : -1;

	EntityCache_t& tCache = mCache[pProjectile];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = F::Groups.GetColor(pOwner ? pOwner : pProjectile, pGroup);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;

	if (pGroup->m_iESP & ESPEnum::Distance)
	{
		Vec3 vDelta = pProjectile->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pGroup->m_iESP & ESPEnum::Name)
		tCache.m_vText.emplace_back(ALIGN_TOP, GetProjectileName(pProjectile), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

	if (pGroup->m_iESP & ESPEnum::Owner && pOwner)
	{
		if (auto pResource = H::Entities.GetResource(); pResource)
			tCache.m_vText.emplace_back(ALIGN_TOP, F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex)), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pGroup->m_iESP & ESPEnum::Flags)
	{
		switch (pProjectile->GetClassID())
		{
		case ETFClassID::CTFWeaponBaseGrenadeProj:
		case ETFClassID::CTFWeaponBaseMerasmusGrenade:
		case ETFClassID::CTFGrenadePipebombProjectile:
		case ETFClassID::CTFStunBall:
		case ETFClassID::CTFBall_Ornament:
		case ETFClassID::CTFProjectile_Jar:
		case ETFClassID::CTFProjectile_Cleaver:
		case ETFClassID::CTFProjectile_JarGas:
		case ETFClassID::CTFProjectile_JarMilk:
		case ETFClassID::CTFProjectile_SpellBats:
		case ETFClassID::CTFProjectile_SpellKartBats:
		case ETFClassID::CTFProjectile_SpellMeteorShower:
		case ETFClassID::CTFProjectile_SpellMirv:
		case ETFClassID::CTFProjectile_SpellPumpkin:
		case ETFClassID::CTFProjectile_SpellSpawnBoss:
		case ETFClassID::CTFProjectile_SpellSpawnHorde:
		case ETFClassID::CTFProjectile_SpellSpawnZombie:
		case ETFClassID::CTFProjectile_SpellTransposeTeleport:
		case ETFClassID::CTFProjectile_Throwable:
		case ETFClassID::CTFProjectile_ThrowableBreadMonster:
		case ETFClassID::CTFProjectile_ThrowableBrick:
		case ETFClassID::CTFProjectile_ThrowableRepel:
			if (pProjectile->As<CTFWeaponBaseGrenadeProj>()->m_bCritical())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Crit", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			if (pProjectile->As<CTFWeaponBaseGrenadeProj>()->m_iDeflected() && (pProjectile->GetClassID() != ETFClassID::CTFGrenadePipebombProjectile || !pProjectile->GetAbsVelocity().IsZero()))
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			break;
		case ETFClassID::CTFProjectile_Arrow:
		case ETFClassID::CTFProjectile_GrapplingHook:
		case ETFClassID::CTFProjectile_HealingBolt:
			if (pProjectile->As<CTFProjectile_Arrow>()->m_bCritical())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Crit", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			if (pProjectile->As<CTFBaseRocket>()->m_iDeflected())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			if (pProjectile->As<CTFProjectile_Arrow>()->m_bArrowAlight())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Alight", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			break;
		case ETFClassID::CTFProjectile_Rocket:
		case ETFClassID::CTFProjectile_BallOfFire:
		case ETFClassID::CTFProjectile_MechanicalArmOrb:
		case ETFClassID::CTFProjectile_SentryRocket:
		case ETFClassID::CTFProjectile_SpellFireball:
		case ETFClassID::CTFProjectile_SpellLightningOrb:
		case ETFClassID::CTFProjectile_SpellKartOrb:
			if (pProjectile->As<CTFProjectile_Rocket>()->m_bCritical())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Crit", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			if (pProjectile->As<CTFBaseRocket>()->m_iDeflected())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			break;
		case ETFClassID::CTFProjectile_EnergyBall:
			if (pProjectile->As<CTFProjectile_EnergyBall>()->m_bChargedShot())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Charge", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			if (pProjectile->As<CTFBaseRocket>()->m_iDeflected())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			break;
		case ETFClassID::CTFProjectile_Flare:
			if (pProjectile->As<CTFProjectile_Flare>()->m_bCritical())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Crit", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			if (pProjectile->As<CTFBaseRocket>()->m_iDeflected())
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Reflected", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			break;
		}
	}
}

static inline void StoreObjective(CBaseEntity* pObjective, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, EntityCache_t>& mCache)
{
	auto pOwner = pObjective->m_hOwnerEntity()->As<CTFPlayer>();
	if (pOwner == pLocal)
		return;

	EntityCache_t& tCache = mCache[pObjective];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = F::Groups.GetColor(pObjective, pGroup);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;

	if (pGroup->m_iESP & ESPEnum::Distance)
	{
		Vec3 vDelta = pObjective->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	switch (pObjective->GetClassID())
	{
	case ETFClassID::CCaptureFlag:
	{
		auto pIntel = pObjective->As<CCaptureFlag>();

		if (pGroup->m_iESP & ESPEnum::Name)
			tCache.m_vText.emplace_back(ALIGN_TOP, "Intel", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);

		if (pGroup->m_iESP & ESPEnum::Flags)
		{
			switch (pIntel->m_nFlagStatus())
			{
			case TF_FLAGINFO_HOME:
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Home", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				break;
			case TF_FLAGINFO_DROPPED:
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Dropped", Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
				break;
			default:
				tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, "Stolen", Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value);
			}
		}

		if (pGroup->m_iESP & ESPEnum::IntelReturnTime && pIntel->m_nFlagStatus() == TF_FLAGINFO_DROPPED)
		{
			float flReturnTime = std::max(pIntel->m_flResetTime() - TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick), 0.f);
			tCache.m_vText.emplace_back(ALIGN_TOPRIGHT, std::format("Return {:.1f}s", pIntel->m_flResetTime() - TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick)).c_str(), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
		}

		break;
	}
	}
}

static inline void StoreMisc(CBaseEntity* pEntity, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, EntityCache_t>& mCache)
{
	EntityCache_t& tCache = mCache[pEntity];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = F::Groups.GetColor(pEntity, pGroup);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;

	if (pGroup->m_iESP & ESPEnum::Distance)
	{
		Vec3 vDelta = pEntity->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value);
	}

	if (pGroup->m_iESP & ESPEnum::Name)
	{
		const char* sName = "Unknown";
		switch (pEntity->GetClassID())
		{
		case ETFClassID::CTFBaseBoss: sName = "NPC"; break;
		case ETFClassID::CTFTankBoss: sName = "Tank"; break;
		case ETFClassID::CMerasmus: sName = "Merasmus"; break;
		case ETFClassID::CEyeballBoss: sName = "Monoculus"; break;
		case ETFClassID::CHeadlessHatman: sName = "Horseless Headless Horsemann"; break;
		case ETFClassID::CZombie: sName = "Skeleton"; break;
		case ETFClassID::CBaseAnimating:
		{
			auto uHash = H::Entities.GetModel(pEntity->entindex());
			if (H::Entities.IsHealth(uHash))
				sName = "Health";
			else if (H::Entities.IsAmmo(uHash))
				sName = "Ammo";
			else if (H::Entities.IsSpellbook(uHash))
				sName = "Spellbook";
			else if (H::Entities.IsPowerup(uHash))
			{
				sName = "Powerup";
				switch (uHash)
				{
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_agility.mdl"): sName = "Agility"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_crit.mdl"): sName = "Revenge"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_defense.mdl"): sName = "Resistance"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_haste.mdl"): sName = "Haste"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_king.mdl"): sName = "King"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_knockout.mdl"): sName = "Knockout"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_plague.mdl"): sName = "Plague"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_precision.mdl"): sName = "Precision"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_reflect.mdl"): sName = "Reflect"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_regen.mdl"): sName = "Regeneration"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_strength.mdl"): sName = "Strength"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_supernova.mdl"): sName = "Supernova"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_vampire.mdl"): sName = "Vampire";
				}
			}
			break;
		}
		case ETFClassID::CTFAmmoPack: sName = "Ammo"; break;
		case ETFClassID::CCurrencyPack: sName = "Money"; break;
		case ETFClassID::CTFGenericBomb:
		case ETFClassID::CTFPumpkinBomb: sName = "Bomb"; break;
		case ETFClassID::CHalloweenGiftPickup: sName = "Gargoyle"; break;
		}

		tCache.m_vText.emplace_back(ALIGN_TOP, sName, pGroup->m_tColor, Vars::Menu::Theme::Background.Value);
	}
}

void CESP::Store(CTFPlayer* pLocal)
{
	m_mPlayerCache.clear();
	m_mBuildingCache.clear();
	m_mEntityCache.clear();
	if (!pLocal || !F::Groups.GroupsActive())
		return;

	for (auto& [pEntity, pGroup] : F::Groups.GetGroup(false))
	{
		if (!pGroup->m_iESP)
			continue;

		if (pEntity->IsPlayer())
			StorePlayer(pEntity->As<CTFPlayer>(), pLocal, pGroup, m_mPlayerCache);
		else if (pEntity->IsBuilding())
			StoreBuilding(pEntity->As<CBaseObject>(), pLocal, pGroup, m_mBuildingCache);
		else if (pEntity->IsProjectile())
			StoreProjectile(pEntity, pLocal, pGroup, m_mEntityCache);
		else if (pEntity->GetClassID() == ETFClassID::CCaptureFlag)
			StoreObjective(pEntity, pLocal, pGroup, m_mEntityCache);
		else
			StoreMisc(pEntity, pLocal, pGroup, m_mEntityCache);
	}
}

static matrix3x4 s_aBones[MAXSTUDIOBONES];

void CESP::Draw()
{
	DrawWorld();
	DrawBuildings();
	DrawPlayers();
}

void CESP::DrawPlayers()
{
	if (m_mPlayerCache.empty())
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const int nTall = fFont.m_nTall + H::Draw.Scale(2);
	for (auto& [pEntity, tCache] : m_mPlayerCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;
		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });

		if (tCache.m_bBones)
		{
			auto pPlayer = pEntity->As<CTFPlayer>();
			if (pPlayer->SetupBones(s_aBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, I::GlobalVars->curtime))
			{
				int iHead = pPlayer->GetBaseToHitbox(HITBOX_HEAD);
				int iSpine2 = pPlayer->GetBaseToHitbox(HITBOX_SPINE2);
				int iPelvis = pPlayer->GetBaseToHitbox(HITBOX_PELVIS);
				int iLeftUpperarm = pPlayer->GetBaseToHitbox(HITBOX_LEFT_UPPERARM);
				int iLeftForearm = pPlayer->GetBaseToHitbox(HITBOX_LEFT_FOREARM);
				int iLeftHand = pPlayer->GetBaseToHitbox(HITBOX_LEFT_HAND);
				int iRightUpperarm = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_UPPERARM);
				int iRightForearm = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_FOREARM);
				int iRightHand = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_HAND);
				int iLeftThigh = pPlayer->GetBaseToHitbox(HITBOX_LEFT_THIGH);
				int iLeftCalf = pPlayer->GetBaseToHitbox(HITBOX_LEFT_CALF);
				int iLeftFoot = pPlayer->GetBaseToHitbox(HITBOX_LEFT_FOOT);
				int iRightThigh = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_THIGH);
				int iRightCalf = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_CALF);
				int iRightFoot = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_FOOT);

				DrawBones(pPlayer, s_aBones, { iHead, iSpine2, iPelvis }, tCache.m_tColor);
				DrawBones(pPlayer, s_aBones, { iSpine2, iLeftUpperarm, iLeftForearm, iLeftHand }, tCache.m_tColor);
				DrawBones(pPlayer, s_aBones, { iSpine2, iRightUpperarm, iRightForearm, iRightHand }, tCache.m_tColor);
				DrawBones(pPlayer, s_aBones, { iPelvis, iLeftThigh, iLeftCalf, iLeftFoot }, tCache.m_tColor);
				DrawBones(pPlayer, s_aBones, { iPelvis, iRightThigh, iRightCalf, iRightFoot }, tCache.m_tColor);
			}
		}

		for (auto& [iMode, flPercent, tColor, tOverfill, bAdjust] : tCache.m_vBars)
		{
			auto fDrawBar = [&](int x, int y, int w, int h, EAlign eAlign = ALIGN_LEFT)
				{
					if (flPercent > 1.f)
					{
						H::Draw.FillRectPercent(x, y, w, h, 1.f, tColor, { 0, 0, 0, 255 }, eAlign, bAdjust);
						H::Draw.FillRectPercent(x, y, w, h, flPercent - 1.f, tOverfill, { 0, 0, 0, 0 }, eAlign, bAdjust);
					}
					else
						H::Draw.FillRectPercent(x, y, w, h, flPercent, tColor, { 0, 0, 0, 255 }, eAlign, bAdjust);
				};

			int iSpace = H::Draw.Scale(4);
			int iThickness = H::Draw.Scale(2, Scale_Round);
			switch (iMode)
			{
			case ALIGN_LEFT:
				fDrawBar(x - iSpace - iThickness - lOffset, y, iThickness, h, ALIGN_BOTTOM);
				lOffset += iSpace + iThickness;
				break;
			case ALIGN_BOTTOM:
			{
				bool bIsUberBar = (tColor.r == 255 && tColor.g == 0 && tColor.b == 255 && !bAdjust);

				int iSizeX = w;
				int iSizeY = H::Draw.Scale(3, Scale_Round);
				int iPosX = x;
				int iPosY = y + h + iSpace + bOffset;

				if (bIsUberBar)
				{
					float flRatio = std::clamp(flPercent, 0.f, 1.f);

					H::Draw.LineRect(iPosX, iPosY, iSizeX + 1, iSizeY + 1, { 0, 0, 0, 255 });

					if (flRatio > 0.f)
					{
						int iFillSizeX = iSizeX - 2;
						int iFillSizeY = iSizeY - 2;
						int iFillPosX = iPosX + 1;
						int iFillPosY = iPosY + 1;

						H::Draw.FillRect(
							iFillPosX,
							iFillPosY,
							int((iFillSizeX + 1) * flRatio),
							iFillSizeY + 1,
							{ 255, 0, 255, 255 }
						);
					}
				}
				else
				{
					fDrawBar(iPosX, iPosY, iSizeX, iThickness);
				}

				bOffset += iSpace + iSizeY;
				break;
			}
			}
		}

		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ALIGN_LEFT:
				H::Draw.StringOutlined(fFont, l + 6, y - H::Draw.Scale(2) + h - h * std::min(tCache.m_flHealth, 1.f), tColor, tOutline, ALIGN_TOPRIGHT, sText.c_str());
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
				break;
			case ALIGN_BOTTOMRIGHT:
				H::Draw.StringOutlined(fFont, r, y + h, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				break;
			}
		}

		if (tCache.m_iClassIcon)
		{
			const char* sTexture = "vgui/glyph_multiplayer.vtf";
			switch (tCache.m_iClassIcon)
			{
			case TF_CLASS_SCOUT: sTexture = "hud/leaderboard_class_scout.vtf"; break;
			case TF_CLASS_SOLDIER: sTexture = "hud/leaderboard_class_soldier.vtf"; break;
			case TF_CLASS_PYRO: sTexture = "hud/leaderboard_class_pyro.vtf"; break;
			case TF_CLASS_DEMOMAN: sTexture = "hud/leaderboard_class_demo.vtf"; break;
			case TF_CLASS_HEAVY: sTexture = "hud/leaderboard_class_heavy.vtf"; break;
			case TF_CLASS_ENGINEER: sTexture = "hud/leaderboard_class_engineer.vtf"; break;
			case TF_CLASS_MEDIC: sTexture = "hud/leaderboard_class_medic.vtf"; break;
			case TF_CLASS_SNIPER: sTexture = "hud/leaderboard_class_sniper.vtf"; break;
			case TF_CLASS_SPY: sTexture = "hud/leaderboard_class_spy.vtf"; break;
			}
			int iSize = H::Draw.Scale(18, Scale_Round);
			H::Draw.Texture(sTexture, m, t - tOffset, iSize, iSize, ALIGN_BOTTOM);
		}

		if (tCache.m_pWeaponIcon)
		{
			float flW = tCache.m_pWeaponIcon->Width(), flH = tCache.m_pWeaponIcon->Height();
			float flScale = H::Draw.Scale(std::min((w + 40) / 2.f, 80.f) / std::max(flW, flH * 2));

			// Calculate icon position
			float flIconX = m - flW / 2.f * flScale;
			float flIconY = b + bOffset;

			// Draw the icon
			H::Draw.DrawHudTexture(flIconX, flIconY, flScale, tCache.m_pWeaponIcon, Vars::Menu::Theme::Active.Value);

			// Add fixed spacing after icon (using a fixed pixel value instead of scaling with icon)
			float flSpacingAfterIcon = H::Draw.Scale(8); // Fixed 8 pixels of spacing after icon
			float flTextY = flIconY + (flH * flScale) + flSpacingAfterIcon;

			// Draw priority texts under weapon icon with fixed spacing between them
			for (auto& [sText, tColor] : tCache.m_vPriorityText)
			{
				H::Draw.StringOutlined(fFont, m, flTextY, tColor, { 0, 0, 0, 255 }, ALIGN_TOP, sText.c_str());
				flTextY += nTall; // Use the same line height as other ESP text
			}
		}
		else if (!tCache.m_vPriorityText.empty()) // If no weapon icon, draw priority texts normally
		{
			float flTextY = b + bOffset;
			for (auto& [sText, tColor] : tCache.m_vPriorityText)
			{
				H::Draw.StringOutlined(fFont, m, flTextY, tColor, { 0, 0, 0, 255 }, ALIGN_TOP, sText.c_str());
				flTextY += nTall;
			}
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

void CESP::DrawBuildings()
{
	if (m_mBuildingCache.empty())
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const int nTall = fFont.m_nTall + H::Draw.Scale(2);
	for (auto& [pEntity, tCache] : m_mBuildingCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;
		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });
		for (auto& [iMode, flPercent, tColor, tOverfill, bAdjust] : tCache.m_vBars)
		{
			auto fDrawBar = [&](int x, int y, int w, int h, EAlign eAlign = ALIGN_LEFT)
				{
					if (flPercent > 1.f)
					{
						H::Draw.FillRectPercent(x, y, w, h, 1.f, tColor, { 0, 0, 0, 255 }, eAlign, bAdjust);
						H::Draw.FillRectPercent(x, y, w, h, flPercent - 1.f, tOverfill, { 0, 0, 0, 0 }, eAlign, bAdjust);
					}
					else
						H::Draw.FillRectPercent(x, y, w, h, flPercent, tColor, { 0, 0, 0, 255 }, eAlign, bAdjust);
				};

			int iSpace = H::Draw.Scale(4);
			int iThickness = H::Draw.Scale(2, Scale_Round);
			switch (iMode)
			{
			case ALIGN_LEFT:
				fDrawBar(x - iSpace - iThickness - lOffset, y, iThickness, h, ALIGN_BOTTOM);
				lOffset += iSpace + iThickness;
				break;
			case ALIGN_BOTTOM:
			{
				float flRatio = std::clamp(flPercent, 0.f, 1.f);

				int iSizeX = w;
				int iSizeY = H::Draw.Scale(3, Scale_Round);
				int iPosX = x;
				int iPosY = y + h + iSpace + bOffset;

				H::Draw.LineRect(iPosX, iPosY, iSizeX + 1, iSizeY + 1, { 0, 0, 0, 255 });

				if (flRatio > 0.f)
				{
					int iFillSizeX = iSizeX - 2;
					int iFillSizeY = iSizeY - 2;
					int iFillPosX = iPosX + 1;
					int iFillPosY = iPosY + 1;

					H::Draw.FillRect(
						iFillPosX,
						iFillPosY,
						int((iFillSizeX + 1) * flRatio),
						iFillSizeY + 1,
						tColor
					);
				}

				bOffset += iSpace + iSizeY;
				break;
			}
			}
		}

		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ALIGN_LEFT:
				H::Draw.StringOutlined(fFont, l + 6, y - H::Draw.Scale(2) + h - h * std::min(tCache.m_flHealth, 1.f), tColor, tOutline, ALIGN_TOPRIGHT, sText.c_str());
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
				break;
			case ALIGN_BOTTOMRIGHT:
				H::Draw.StringOutlined(fFont, r, y + h, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				break;
			}
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

void CESP::DrawWorld()
{
	if (m_mEntityCache.empty())
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const int nTall = fFont.m_nTall + H::Draw.Scale(2);
	for (auto& [pEntity, tCache] : m_mEntityCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;
		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });


		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
				break;
			}
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

bool CESP::GetDrawBounds(CBaseEntity* pEntity, float& x, float& y, float& w, float& h)
{
	Vec3 vOrigin = pEntity->GetAbsOrigin();
	matrix3x4 mTransform = { { 1, 0, 0, vOrigin.x }, { 0, 1, 0, vOrigin.y }, { 0, 0, 1, vOrigin.z } };
	//if (pEntity->entindex() == I::EngineClient->GetLocalPlayer())
	Math::AngleMatrix({ 0.f, I::EngineClient->GetViewAngles().y, 0.f }, mTransform, false);

	float flLeft, flRight, flTop, flBottom;
	if (!SDK::IsOnScreen(pEntity, mTransform, &flLeft, &flRight, &flTop, &flBottom, true))
		return false;

	x = flLeft;
	y = flBottom;
	w = flRight - flLeft;
	h = flTop - flBottom;

	switch (pEntity->GetClassID())
	{
	case ETFClassID::CTFPlayer:
	case ETFClassID::CObjectSentrygun:
	case ETFClassID::CObjectDispenser:
	case ETFClassID::CObjectTeleporter:
		x += w * 0.125f;
		w *= 0.75f;
	}

	return !(x > H::Draw.m_nScreenW || x + w < 0 || y > H::Draw.m_nScreenH || y + h < 0);
}

void CESP::DrawBones(CTFPlayer* pPlayer, matrix3x4* aBones, std::vector<int> vecBones, Color_t clr)
{
	for (size_t n = 1; n < vecBones.size(); n++)
	{
		auto vBone1 = pPlayer->GetHitboxCenter(aBones, vecBones[n]);
		auto vBone2 = pPlayer->GetHitboxCenter(aBones, vecBones[n - 1]);

		Vec3 vScreen1, vScreen2;
		if (SDK::W2S(vBone1, vScreen1) && SDK::W2S(vBone2, vScreen2))
			H::Draw.Line(vScreen1.x, vScreen1.y, vScreen2.x, vScreen2.y, clr);
	}
}