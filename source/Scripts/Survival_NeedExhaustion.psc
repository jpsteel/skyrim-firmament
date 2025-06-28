scriptname Survival_NeedExhaustion extends Survival_NeedBase

import Survival_GlobalFunctions

Message property Survival_HelpExhaustionHigh auto
GlobalVariable property Survival_HelpShown_Exhaustion auto

ReferenceAlias property PlayerAlias auto
FormList property Survival_ExhaustionResistRacesMajor auto
FormList property Survival_ExhaustionResistRacesMinor auto
GlobalVariable property Survival_RacialBonusMajor auto
GlobalVariable property Survival_RacialBonusMinor auto
GlobalVariable property Survival_AfflictionExhaustionChance auto

Spell property Survival_AfflictionAddled auto
Spell property Survival_DiseaseBrownRot auto
Spell property Survival_DiseaseBrownRot2 auto
Spell property Survival_DiseaseBrownRot3 auto

Spell property VampireVampirism auto
Spell property WerewolfImmunity auto
Message property Survival_AfflictionAddledMsg auto

; Original PlayerSleepQuest properties
Quest property PlayerSleepQuest auto
Quest property RelationshipMarriageFIN auto
Quest property BYOHRelationshipAdoption auto
Spell property Rested auto
Spell property WellRested auto
Spell property MarriageSleepAbility auto
Spell property BYOHAdoptionSleepAbilityMale auto
Spell property BYOHAdoptionSleepAbilityFemale auto
Spell property pDoomLoverAbility auto
Keyword property LocTypeInn auto
Keyword property LocTypePlayerHouse auto
ReferenceAlias Property LoveInterest Auto
LocationAlias Property CurrentHomeLocation Auto
Message property BeastBloodMessage auto
Message property MarriageRestedMessage auto
Message property WellRestedMessage auto
Message property RestedMessage auto
Message property BYOHAdoptionRestedMessageMale auto
Message property BYOHAdoptionRestedMessageFemale auto
CompanionsHousekeepingScript Property CHScript Auto

; Special non-Resist Disease affecting need spell variants
Spell[] property needSpellsNoDisease auto

bool property playerSleeping = false auto hidden

function StartNeed()
	; Remove any existing Rested bonuses
    RemoveAllNeedSpells()

    oldStage = 99
    parent.StartNeed()

	; Unregister the default Sleep quest so we can manage this ourselves.
	PlayerSleepQuest.UnregisterForSleep()

    PlayerAlias.ForceRefIfEmpty(PlayerRef)
endFunction

function SetNeedStageValues()
                          ; Rested / Well Rested / Lover's Comfort (20pts/hr * 4 hours = 80pts)
    needStage1Value = 80  ; Refreshed
    needStage2Value = 160 ; Drained
    needStage3Value = 340 ; Tired
    needStage4Value = 560 ; Exhausted
    needStage5Value = 800 ; Debilitated
endFunction

function StopNeed()
    WaitForUnlock()
	parent.StopNeed()
    PlayerAlias.Clear()

	; Re-register the default Sleep quest.
	PlayerSleepQuest.RegisterForSleep()
endFunction

function NeedUpdateGameTime()
	SurvivalLogMessage("{{EXHAUSTION}} Update Start")
    bool wasSleeping = detectedSleepEvent

    if conditions.isInPlaneOfOblivion
        SurvivalLogMessage("{{EXHAUSTION}} In Oblivion, skipping NeedUpdateGameTime.")
        lastTimeInGameHours = Utility.GetCurrentGameTime() * 24 ; currentTimeInGameHours
        return
    endif

	if playerSleeping
		SurvivalLogMessage("{{EXHAUSTION}} Player was sleeping, cancelling this update.")
        playerSleeping = false
        lastTimeInGameHours = Utility.GetCurrentGameTime() * 24 ; currentTimeInGameHours
		return
	endif

    Race playerRace = PlayerRef.GetActorBase().GetRace()
    float racialBonus
    if Survival_ExhaustionResistRacesMajor.HasForm(playerRace)
        racialBonus = Survival_RacialBonusMajor.GetValue()
    elseif Survival_ExhaustionResistRacesMinor.HasForm(playerRace)
        racialBonus = Survival_RacialBonusMinor.GetValue()
    endif

	float newExhaustionValue = IncrementNeedByTick(racialBonus)
	
    ApplyExhaustionStage(newExhaustionValue, lastValue, CanGetRestedBonus())

    if !wasSleeping && newExhaustionValue >= needStage5Value && !PlayerRef.HasSpell(Survival_AfflictionAddled)
        ; Addled
        SurvivalLogMessage("{{EXHAUSTION}} Player eligible for Addled. Rolling...")
        float result = Utility.RandomFloat()
        SurvivalLogMessage("{{EXHAUSTION}}     Result: " + result)
        if result <= Survival_AfflictionExhaustionChance.GetValue()
            SurvivalLogMessage("{{EXHAUSTION}}     Player is now Addled!")
            Survival_AfflictionAddledMsg.Show()
            PlayerRef.AddSpell(Survival_AfflictionAddled, false)
        endif
    endif

	lastValue = newExhaustionValue
endFunction

function IncreaseExhaustion(float amount)
    if conditions.isInPlaneOfOblivion
        SurvivalLogMessage("{{EXHAUSTION}} In Oblivion, skipping IncreaseExhaustion.")
        return
    endif

    SurvivalLogMessage("{{EXHAUSTION}} IncreaseExhaustion Start, amount " + amount)
    float currentNeedValue = NeedValue.GetValue()
    float newExhaustionValue = IncrementNeed(currentNeedValue, amount)
    
    ApplyExhaustionStage(newExhaustionValue, lastValue, CanGetRestedBonus())
    lastValue = newExhaustionValue
endFunction

function DecreaseExhaustion(float amount, bool qualitySleep = false)
    if conditions.isInPlaneOfOblivion
        SurvivalLogMessage("{{EXHAUSTION}} In Oblivion, skipping DecreaseExhaustion.")
        return
    endif

    SurvivalLogMessage("{{EXHAUSTION}} DecreaseExhaustion Start, amount " + amount + " qualitySleep " + qualitySleep)

	; Apply applicable Adoption bonus regardless
    ApplyAdoptionBonus()

	float newExhaustionValue
    float currentNeedValue = NeedValue.GetValue()
    amount *= GetBrownRotEffectMult()

	SurvivalLogMessage("{{EXHAUSTION}} Player got quality sleep.")
    newExhaustionValue = DecrementNeed(currentNeedValue, amount)

    ApplyExhaustionStage(newExhaustionValue, lastValue, CanGetRestedBonus(showMessages = true))
    lastValue = newExhaustionValue
endFunction

function ApplyExhaustionStage(float newExhaustionValue, float oldExhaustionValue, bool canGetRestedBonus)
    bool increasing = newExhaustionValue > oldExhaustionValue
    SurvivalLogMessage("{{EXHAUSTION}}     Applying exhaustion stage. New value: " + newExhaustionValue + ", Old value: " + oldExhaustionValue + ", increasing: " + increasing)

    int newStage
    if hasBonus
        newStage = 0
    elseif Between(newExhaustionValue, needStage2Value, NeedMinValue)
        newStage = 1
    elseif Between(newExhaustionValue, needStage3Value, needStage2Value)
        newStage = 2
    elseif Between(newExhaustionValue, needStage4Value, needStage3Value)
        newStage = 3
    elseif Between(newExhaustionValue, needStage5Value, needStage4Value)
        newStage = 4
    elseif Between(newExhaustionValue, NeedMaxValue.GetValue(), needStage5Value, inclusiveUpperBound = true)
        newStage = 5
    endif

    if newStage != oldStage || newExhaustionValue == NeedMinValue
        SurvivalLogMessage("{{EXHAUSTION}}     Applying Stage " + newStage)
        if newStage == 0
            if canGetRestedBonus
                RemoveAllNeedSpells()
                ApplyNormalRestedBonus()
            else
                ApplyNeedStagePlayerEffects(increasing, needSpells[1], needMessagesWhenIncreasing[1], needMessagesWhenDecreasing[1])
            endif
        elseif newStage >= 3 && PlayerIsVampireOrWerewolf()
            ApplyNeedStagePlayerEffects(increasing, needSpellsNoDisease[newStage], needMessagesWhenIncreasing[newStage], needMessagesWhenDecreasing[newStage])
        else
            ApplyNeedStagePlayerEffects(increasing, needSpells[newStage], needMessagesWhenIncreasing[newStage], needMessagesWhenDecreasing[newStage])
        endif
        ApplyNeedSFX(increasing, needSoundFXs[newStage], needSoundFXsFemale[newStage], needPlaySoundFXsOnImprove[newStage])
        ApplyNeedRumble(increasing, needRumbleSmallMotorStrengths[newStage], needRumbleBigMotorStrengths[newStage], needRumbleDurations[newStage], needPlayRumblesOnImprove[newStage])
    endif

    oldStage = newStage

    if newStage >= 2 && Survival_HelpShown_Exhaustion.GetValueInt() == 0 && !PlayerRef.IsInCombat()
        Survival_HelpExhaustionHigh.Show()
        Survival_HelpShown_Exhaustion.SetValueInt(1)
    endif
endFunction

float function IncrementNeedByTick(float rateReductionMultiplier = 0.0)
    Utility.Wait(1)
    SurvivalLogMessage("{{EXHAUSTION}}     IncrementNeedByTick with rateReductionMultiplier = " + rateReductionMultiplier)

    float currentNeedValue = NeedValue.GetValue()
    float newNeedValue
    float currentTimeInGameHours = Utility.GetCurrentGameTime() * 24
    float currentTimeInRealSeconds = Utility.GetCurrentRealTime()

    if firstUpdate
        SurvivalLogMessage("{{EXHAUSTION}}     Ignoring the first update and storing update time.")
        lastTimeInGameHours = currentTimeInGameHours
        return currentNeedValue + 1
    endif

    int ticks = GetTicks(currentTimeInGameHours, lastTimeInGameHours)
    float amountToIncrementBy = GetAmountToIncrementBy(ticks, rateReductionMultiplier)
    
    ; Overencumbered
    if PlayerRef.IsOverEncumbered()
        amountToIncrementBy *= Survival_ExhaustionOverEncumberedMult.GetValue()
    endif

    newNeedValue = IncrementNeed(currentNeedValue, amountToIncrementBy)
    SurvivalLogMessage("{{EXHAUSTION}}     Incrementing need by " + amountToIncrementBy)

    lastTimeInGameHours = currentTimeInGameHours
    SurvivalLogMessage("{{EXHAUSTION}}     New value is " + newNeedValue)
    return newNeedValue
endFunction

bool function CanGetRestedBonus(bool showMessages = false)
	if CHScript.PlayerHasBeastBlood
		if showMessages
			BeastBloodMessage.Show()
		endif
		return false
	elseif PlayerRef.HasSpell(pDoomLoverAbility)
		return false
	else
		return true
	endif
endFunction

bool function PlayerIsVampireOrWerewolf()
    if PlayerRef.HasSpell(VampireVampirism) || PlayerRef.HasSpell(WerewolfImmunity)
        return true
    else
        return false
    endif
endFunction

function SelectCorrectEffects()
    if PlayerIsVampireOrWerewolf()
        SwitchToNoDiseaseEffects()
    else
        SwitchToDiseaseEffects()
    endif
endFunction

function SwitchToNoDiseaseEffects()
    SurvivalLogMessage("{{EXHAUSTION}}     Switching to no disease effects.")
    int i = 3
    while i < needSpells.length
        if PlayerRef.HasSpell(needSpells[i])
            PlayerRef.RemoveSpell(needSpells[i])
            PlayerRef.AddSpell(needSpellsNoDisease[i], false)
        endif
        i += 1
    endWhile
endFunction

function SwitchToDiseaseEffects()
    SurvivalLogMessage("{{EXHAUSTION}}     Switching to disease effects.")
    int i = 3
    while i < needSpells.length
        if PlayerRef.HasSpell(needSpellsNoDisease[i])
            PlayerRef.RemoveSpell(needSpellsNoDisease[i])
            PlayerRef.AddSpell(needSpells[i], false)
        endif
        i += 1
    endWhile
endFunction

float function GetBrownRotEffectMult()
    if PlayerRef.HasSpell(Survival_DiseaseBrownRot)
        return 0.75
    elseif PlayerRef.HasSpell(Survival_DiseaseBrownRot2)
        return 0.50
    elseif PlayerRef.HasSpell(Survival_DiseaseBrownRot3)
        return 0.25
    else
        return 1.0
    endif
endFunction

;/
 / Required Extends
 /;

function RemoveAllNeedSpells()
	PlayerRef.RemoveSpell(Rested)
	PlayerRef.RemoveSpell(WellRested)
	PlayerRef.RemoveSpell(MarriageSleepAbility)
    int i = 0
    while i < needSpells.length
        PlayerRef.RemoveSpell(needSpells[i])
        i += 1
    endWhile
    i = 0
    while i < needSpellsNoDisease.length
        PlayerRef.RemoveSpell(needSpellsNoDisease[i])
        i += 1
    endWhile
endFunction

; Adapted from PlayerSleepQuestScript.psc/OnSleepStop()
function ApplyNormalRestedBonus()
    Location currentLocation = PlayerRef.GetCurrentLocation()
	if RelationshipMarriageFIN.IsRunning() && RelationshipMarriageFIN.GetStage() >= 10 && currentLocation == LoveInterest.GetActorRef().GetCurrentLocation()
		; Lover's Comfort if sleeping in same location as love interest
		MarriageRestedMessage.Show()
		PlayerRef.AddSpell(MarriageSleepAbility, false)
	elseif currentLocation.HasKeyword(LocTypeInn) || currentLocation.HasKeyword(LocTypePlayerHouse)
		; Well Rested if sleeping in an Inn or bed in owned House
		WellRestedMessage.Show()
		PlayerRef.AddSpell(WellRested, false)
	else
		; Otherwise, Rested
		RestedMessage.Show()
		PlayerRef.AddSpell(Rested, false)
	endif
endFunction

function ApplyAdoptionBonus()
	; Adoption bonuses
	if BYOHRelationshipAdoption.IsRunning() && PlayerRef.GetCurrentLocation() == CurrentHomeLocation.GetLocation()
		RemoveAdoptionRested()
		if PlayerRef.GetActorBase().GetSex() == 0
			;Player is a father.
			BYOHAdoptionRestedMessageMale.Show()
			PlayerRef.AddSpell(BYOHAdoptionSleepAbilityMale, false)	
		else
			;Player is a mother.
			BYOHAdoptionRestedMessageFemale.Show()
			PlayerRef.AddSpell(BYOHAdoptionSleepAbilityFemale, False)
		endif
	endif
endFunction

Function RemoveAdoptionRested()
	PlayerRef.RemoveSpell(BYOHAdoptionSleepAbilityMale)
	PlayerRef.RemoveSpell(BYOHAdoptionSleepAbilityFemale)
EndFunction

function UpdateAttributePenalty(float afNeedValue, string asAttributeAV = "", string asPenaltyAV = "")
    parent.UpdateAttributePenalty(afNeedValue, "Magicka", ExhaustionMagickaPenaltyAV())
endFunction

function ClearAttributePenalty(string asPenaltyAV = "")
    parent.ClearAttributePenalty(ExhaustionMagickaPenaltyAV())
endFunction