;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 1
Scriptname TIF__0815DD75 Extends TopicInfo Hidden

;BEGIN FRAGMENT Fragment_0
Function Fragment_0(ObjectReference akSpeakerRef)
Actor akSpeaker = akSpeakerRef as Actor
;BEGIN CODE
akSpeaker.AddToFaction(FNS_AlreadyAdvisedFaction)
int amount = AdvisorLow.GetValueInt()
Game.GetPlayer().additem(Gold001, amount)
CustomSkills.AdvanceSkill("Philosophy", amount)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

Faction Property FNS_AlreadyAdvisedFaction  Auto  

GlobalVariable Property AdvisorLow  Auto  

MiscObject Property Gold001  Auto  
