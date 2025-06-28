;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 32
Scriptname QF__0810CD55 Extends Quest Hidden

;BEGIN FRAGMENT Fragment_28
Function Fragment_28()
;BEGIN CODE
float oldLarge = Survival_HungerRestoreLargeAmount.GetValue()
float oldMedium = Survival_HungerRestoreMediumAmount.GetValue()
float oldSmall = Survival_HungerRestoreSmallAmount.GetValue()
float oldVerySmall = Survival_HungerRestoreVerySmallAmount.GetValue()

float newLarge = oldLarge * 1.5
float newMedium = oldMedium * 1.5
float newSmall = oldSmall * 1.5
float newVerySmall = oldVerySmall * 1.5

Survival_HungerRestoreLargeAmount.SetValue(newLarge)
Survival_HungerRestoreMediumAmount.SetValue(newMedium)
Survival_HungerRestoreSmallAmount.SetValue(newSmall)
Survival_HungerRestoreVerySmallAmount.SetValue(newVerySmall)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_30
Function Fragment_30()
;BEGIN CODE
float oldLarge = Survival_HungerRestoreLargeAmount.GetValue()
float oldMedium = Survival_HungerRestoreMediumAmount.GetValue()
float oldSmall = Survival_HungerRestoreSmallAmount.GetValue()
float oldVerySmall = Survival_HungerRestoreVerySmallAmount.GetValue()

float newLarge = oldLarge * 1.33
float newMedium = oldMedium * 1.33
float newSmall = oldSmall * 1.33
float newVerySmall = oldVerySmall * 1.33

Survival_HungerRestoreLargeAmount.SetValue(newLarge)
Survival_HungerRestoreMediumAmount.SetValue(newMedium)
Survival_HungerRestoreSmallAmount.SetValue(newSmall)
Survival_HungerRestoreVerySmallAmount.SetValue(newVerySmall)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment


GlobalVariable Property Survival_HungerRestoreLargeAmount  Auto  

GlobalVariable Property Survival_HungerRestoreMediumAmount  Auto  

GlobalVariable Property Survival_HungerRestoreSmallAmount  Auto  

GlobalVariable Property Survival_HungerRestoreVerySmallAmount  Auto  
