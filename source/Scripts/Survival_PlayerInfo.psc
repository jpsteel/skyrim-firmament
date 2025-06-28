scriptname Survival_PlayerInfo extends Quest

import Survival_GlobalFunctions

Survival_ConditionsScript property conditions auto
Survival_NeedCold property cold auto
Survival_NeedExhaustion property exhaustion auto
Survival_NeedHunger property hunger auto

Actor property PlayerRef auto
FormList property Survival_FastTravelDragonbornBlackBooks auto
FormList property Survival_OblivionAreas auto
FormList property Survival_OblivionCells auto
FormList property Survival_OblivionLocations auto
FormList property Survival_TravelObjects auto
GlobalVariable property Survival_PlayerLastKnownDaysJailed auto
GlobalVariable property Survival_WasLastNearbyTravelRef auto
Message property Survival_OblivionAreaMessage auto
Quest property DA16 auto
ReferenceAlias property PlayerAlias auto
Perk property FNS_EXP_LayOfTheLand auto
GlobalVariable property LayOfTheLandDaysPassed auto
GlobalVariable property GameDaysPassed auto

bool wasInPlaneOfOblivion = false
bool wasInDreamstride = false

function StartUpdating()
	if !self.IsRunning()
		self.Start()
	endif
	PlayerAlias.ForceRefIfEmpty(PlayerRef)

	RegisterForSingleUpdate(5)
endFunction

function StopUpdating()
	if self.IsRunning()
		self.Stop()
	endif
	PlayerAlias.Clear()
	Game.EnableFastTravel()
endFunction

Event OnUpdate()
	if self.IsRunning()
		Update()
		RegisterForSingleUpdate(5)
	endif
EndEvent

function Update()
	CheckFastTravel()
	StorePlayerLastKnownDaysJailed()
	HandleIsInPlaneOfOblivion()
endFunction

function CheckFastTravel()
	; Fast Travel is disabled in Survival Mode, unless in a circumstance where it must be enabled.

	; Enable when near the black books (Dragonborn)
	ObjectReference override = Game.FindClosestReferenceOfAnyTypeInListFromRef(Survival_FastTravelDragonbornBlackBooks, PlayerRef,  650.0)
	float perkCooldown = LayOfTheLandDaysPassed.GetValue() - GameDaysPassed.GetValue()
	if override
		SurvivalLogMessage("Enabling fast travel; near an Apocrypha Black Book.")
		EnableFastTravelIfDisabled()
		return
	endif

	; Enable when riding a dragon
	if conditions.isRidingDragon
		SurvivalLogMessage("Enabling fast travel; riding a dragon.")
		EnableFastTravelIfDisabled()
		return
	endif
	
	;Enable is has "Lay of the Land" perk
	if PlayerRef.HasPerk(FNS_EXP_LayOfTheLand)
		if perkCooldown < 0
			SurvivalLogMessage("Enabling fast travel; Lay of the Land perk unlocked.")
			EnableFastTravelIfDisabled()
			return
		endif
	endif

	; Otherwise, disable fast travel
	DisableFastTravelIfEnabled()
endFunction

function EnableFastTravelIfDisabled()
	if !Game.IsFastTravelControlsEnabled()
		Game.EnableFastTravel()
		conditions.isFastTravelControlsEnabled = true
	endif
endFunction

function DisableFastTravelIfEnabled()
	if Game.IsFastTravelControlsEnabled()
		SurvivalLogMessage("Disabling fast travel.")
		Game.EnableFastTravel(false)
		conditions.isFastTravelControlsEnabled = false
	endif
endFunction

function StorePlayerLastKnownDaysJailed()
	Survival_PlayerLastKnownDaysJailed.SetValueInt(Game.QueryStat("Days Jailed"))
endFunction

bool function CheckIsInPlaneOfOblivion(Worldspace akCurrentWorldspace = None, bool abCheckDreamstride = true, bool abInDreamstride = false)
	location playerLocation = PlayerRef.GetCurrentLocation()
	Cell playerCell = PlayerRef.GetParentCell()
	if !akCurrentWorldspace
		akCurrentWorldspace = PlayerRef.GetWorldSpace()
	endif

	bool inDreamstride
	if abCheckDreamstride
		; Check if the player is currently in the Dreamstride.
		inDreamstride = PlayerInDreamstride()
	else
		; Just take the value we were given.
		inDreamstride = abInDreamstride
	endif

	; The Dreamstride is unique in that it is entered into, and exited from, an interior
	; based on a quest stage, something we would ordinarily ignore. So, keep track of this uniquely.
	wasInDreamstride = inDreamstride

	return Survival_OblivionLocations.HasForm(playerLocation) || Survival_OblivionAreas.HasForm(akCurrentWorldspace) || Survival_OblivionCells.HasForm(playerCell) || inDreamstride
endFunction

function HandleIsInPlaneOfOblivion()
	Worldspace playerWorldSpace = PlayerRef.GetWorldSpace()
	bool inDreamstride = PlayerInDreamstride()

	if !playerWorldSpace && !inDreamstride && !wasInDreamstride
		; Don't store a result in the case that the player transitions between
		; interiors in an Oblivion worldspace.
		; Store a value if the player is, or was, in the Dreamstride.
		return
	endif

	bool inOblivion = CheckIsInPlaneOfOblivion(playerWorldSpace, false, inDreamstride)
	if inOblivion
		EnterOblivion()
	else
		ExitOblivion()
	endif
endFunction

function EnterOblivion()
	conditions.isInPlaneOfOblivion = true
	if !wasInPlaneOfOblivion
		wasInPlaneOfOblivion = true

		Survival_OblivionAreaMessage.Show()

		hunger.SetInOblivion()
		exhaustion.SetInOblivion()
		cold.SetInOblivion()
		cold.SetUITemperatureLevel(0)
	endif
endFunction

function ExitOblivion()
	conditions.isInPlaneOfOblivion = false
	if wasInPlaneOfOblivion
		wasInPlaneOfOblivion = false

		hunger.SetInOblivion(false)
		exhaustion.SetInOblivion(false)
		cold.SetInOblivion(false)
	endif
endFunction

bool function PlayerInDreamstride()
	int DA16Stage = DA16.GetStage()
	return DA16Stage >= 145 && DA16Stage < 160
endFunction