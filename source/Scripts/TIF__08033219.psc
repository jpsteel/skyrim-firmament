;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 1
Scriptname TIF__08033219 Extends TopicInfo Hidden

;BEGIN FRAGMENT Fragment_0
Function Fragment_0(ObjectReference akSpeakerRef)
Actor akSpeaker = akSpeakerRef as Actor
;BEGIN CODE
SendModEvent("FNS_EXP_WayfarerEvent")
WayfarerDaysPassed.SetValue(GameDaysPassed.GetValue() + 1.0)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

GlobalVariable Property WayfarerDaysPassed  Auto  

GlobalVariable Property GameDaysPassed  Auto  
