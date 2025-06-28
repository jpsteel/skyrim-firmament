;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 28
Scriptname QF__080FDA54 Extends Quest Hidden

;BEGIN FRAGMENT Fragment_25
Function Fragment_25()
;BEGIN CODE
float oldValue = Survival_ExhaustionNeedRate.GetValue()
float oldCacheValue = CachedExhaustionNeedRate.GetValue()

float newValue = oldValue * 0.8
float newCacheValue = oldCacheValue * 0.8

Survival_ExhaustionNeedRate.SetValue(newValue)
CachedExhaustionNeedRate.SetValue(newCacheValue)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_27
Function Fragment_27()
;BEGIN CODE
float oldValue = Survival_ExhaustionNeedRate.GetValue()
float oldCacheValue = CachedExhaustionNeedRate.GetValue()

float newValue = oldValue * 0.75
float newCacheValue = oldCacheValue * 0.75

Survival_ExhaustionNeedRate.SetValue(newValue)
CachedExhaustionNeedRate.SetValue(newCacheValue)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment


GlobalVariable Property Survival_ExhaustionNeedRate  Auto  

GlobalVariable Property CachedExhaustionNeedRate  Auto  
