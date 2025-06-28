Scriptname FNS_HRS_ChargeImpactScript extends activemagiceffect  

Import Game
Import Math
Import Utility

Spell Property ParalysisSpell Auto
Spell Property StaggerSpell Auto
Perk Property FNS_HRS_CavalryCharge Auto
Topic Property HurtDialogue  Auto  

; Compensate for script lag by having collision occur prematurely.
Float Property PrecognitionTime = 0.1 AutoReadOnly
; Ignore collision when heading angle is greater than this value.
Float Property AngleTolerance = 40.0 AutoReadOnly
; Force reduction percentage when target is at maximum heading angle.
Float Property AngleScaleFactor = 0.666 AutoReadOnly
; Duration of camera and controller shake on impact, in seconds.
Float Property ShakeDuration = 0.25 AutoReadOnly

Actor property PlayerRef auto

bool hasEffect
bool isCollision
float mountScale
float mountWidth
float mountLength
float mountHeight
float mountWidthExtent
float mountLengthExtent
float targetScale
float targetWidth
float targetLength
float targetHeight
float targetWidthExtent
float targetLengthExtent
float mountPositionZ
float mountTopZ
float mountHeading
float mountHeadingExtent
float mountHeadingAbs
float mountSpeed
float targetPositionZ
float targetTopZ
float targetHeading
float targetHeadingExtent
float collisionDistance
float currentDistance
float headingAngleMult
float forceMagnitude

Event OnEffectStart(Actor akTarget, Actor akCaster)
	hasEffect = true
	; Hitbox size data.
	mountScale = akCaster.GetScale()
	mountWidth = akCaster.GetWidth() * mountScale
	mountLength = akCaster.GetLength() * mountScale
	mountHeight = akCaster.GetHeight() * mountScale
	mountWidthExtent = mountWidth * 0.5
	mountLengthExtent = mountLength * 0.5
	targetScale = akTarget.GetScale()
	targetWidth = akTarget.GetWidth() * targetScale
	targetLength = akTarget.GetLength() * targetScale
	targetHeight = akTarget.GetHeight() * targetScale
	targetWidthExtent = targetWidth * 0.5
	targetLengthExtent = targetLength * 0.5

	; Wait for collision while refreshing position data.
	while !isCollision
		if !self || !hasEffect
			return
		endif
		mountPositionZ = akCaster.GetPositionZ()
		mountTopZ = mountPositionZ + mountHeight
		targetPositionZ = akTarget.GetPositionZ()
		targetTopZ = targetPositionZ + targetHeight
		mountHeading = akCaster.GetHeadingAngle(akTarget)
		mountHeadingAbs = abs(mountHeading)
		targetHeading = akTarget.GetHeadingAngle(akCaster)
		currentDistance = akCaster.GetDistance(akTarget)
		mountHeadingExtent = Sqrt(Pow(mountWidthExtent * Sin(mountHeading), 2.0) + Pow(mountLengthExtent * Cos(mountHeading), 2.0))
		targetHeadingExtent = Sqrt(Pow(targetWidthExtent * Sin(targetHeading), 2.0) + Pow(targetLengthExtent * Cos(targetHeading), 2.0))
		mountSpeed = akCaster.GetAnimationVariableFloat("Speed")
		collisionDistance = mountHeadingExtent + targetHeadingExtent + mountSpeed * PrecognitionTime
		if akCaster.IsSprinting()
			; Collision.
			if currentDistance <= collisionDistance && mountHeadingAbs <= AngleTolerance && mountPositionZ < targetTopZ && targetPositionZ < mountTopZ
				; Weaken factor that increases with heading angle (force is strongest at 0° heading and weakens as heading angle increases).
				headingAngleMult = 1.0 - ((mountHeadingAbs * AngleScaleFactor) / AngleTolerance)
				; Shake camera proportionally.
				ShakeCamera(akTarget, headingAngleMult, ShakeDuration)
				; Stagger effect
				StaggerSpell.Cast(akCaster, akTarget)
				akTarget.Say(HurtDialogue)
				;Chance to paralyze if player has Cavalry Charge perk
				if PlayerRef.HasPerk(FNS_HRS_CavalryCharge)
					if Utility.RandomFloat(0.0, 1.0) <= 0.25
						ParalysisSpell.Cast(akCaster, akTarget)
					endif
				endif
				; Shake controller proportionally.
				ShakeController(headingAngleMult, headingAngleMult, ShakeDuration)
				; Exits loop.
				isCollision = true
			endif
		; Sprint polling interval.
		else
			wait(0.3)
		endif
	endwhile
EndEvent

Event OnEffectFinish(Actor akTarget, Actor akCaster)
	hasEffect = false
EndEvent