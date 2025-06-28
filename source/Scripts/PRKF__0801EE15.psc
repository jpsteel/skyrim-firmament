;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 11
Scriptname PRKF__0801EE15 Extends Perk Hidden

;BEGIN FRAGMENT Fragment_8
Function Fragment_8(ObjectReference akTargetRef, Actor akActor)
;BEGIN CODE
int offeringAmount = Survival_ShrineGoldOfferingAmount.GetValueInt()

int choice = Survival_ShrineGoldOfferingMessage.Show(offeringAmount)
if choice == 0
    if akActor.GetItemCount(Gold001) < offeringAmount
        Survival_ShrineNotEnoughGoldMessage.Show()
        return
    endif
    SendModEvent("FNS_EXP_PilgrimEvent")
    akActor.RemoveItem(Gold001, offeringAmount)
    
    TempleBlessingScript baseShrine = akTargetRef as TempleBlessingScript
    DLC2TempleShrineScript dlc2Shrine = akTargetRef as DLC2TempleShrineScript
    if baseShrine
    	baseShrine.TempleBlessing.Cast(akActor, akActor)
    	baseShrine.AltarRemoveMsg.Show()
    	baseShrine.BlessingMessage.Show()
    elseif dlc2Shrine
    	dlc2Shrine.TempleBlessing.Cast(akActor, akActor)
    	dlc2Shrine.AltarRemoveMsg.Show()
    	dlc2Shrine.BlessingMessage.Show()
    endif
endif
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

MiscObject Property Gold001  Auto  

GlobalVariable Property Survival_ShrineGoldOfferingAmount  Auto  

Message Property Survival_ShrineGoldOfferingMessage  Auto  

Message Property Survival_ShrineNotEnoughGoldMessage  Auto  
