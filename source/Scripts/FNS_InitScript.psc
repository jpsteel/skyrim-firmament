Scriptname FNS_InitScript extends ReferenceAlias  

Perk Property FNS_EXP_Survival_TempleBlessingCostPerk Auto

GlobalVariable Property CachedHungerNeedRate Auto
GlobalVariable Property CachedExhaustionNeedRate Auto
GlobalVariable Property Survival_ExhaustionNeedRate Auto
GlobalVariable Property Survival_HungerNeedRate Auto

GlobalVariable Property SkillHorsemanLevel Auto
GlobalVariable Property SkillExplorationLevel Auto
GlobalVariable Property SkillPhilosophyLevel Auto

int Property CurrentVersion = 1 AutoReadOnly
int KnownVersion = 0

Event OnInit()
	RegisterForSingleUpdate(1.0)
EndEvent

Event OnPlayerLoadGame()
	if KnownVersion != CurrentVersion
		DoUpdate()
	endif
EndEvent

Event OnUpdate()
	DoUpdate()
EndEvent

Function DoUpdate()
	Actor actorRef = self.GetActorReference()
	if KnownVersion < 1
		DoNewInstall()

		actorRef.AddPerk(FNS_EXP_Survival_TempleBlessingCostPerk)
		;if (CachedHungerNeedRate.GetValue() == 0)
		;	CachedHungerNeedRate.SetValue(Survival_HungerNeedRate.GetValue())
		;endif
		;if (CachedExhaustionNeedRate.GetValue() == 0)
		;	CachedExhaustionNeedRate.SetValue(Survival_ExhaustionNeedRate.GetValue())
		;endif
	endif

	KnownVersion = CurrentVersion
EndFunction

Function DoNewInstall()
	int startingLevel = Game.GetGameSettingInt("iAVDSkillStart")
	SkillHorsemanLevel.SetValue(startingLevel)
	SkillExplorationLevel.SetValue(startingLevel)
	SkillPhilosophyLevel.SetValue(startingLevel)
EndFunction
