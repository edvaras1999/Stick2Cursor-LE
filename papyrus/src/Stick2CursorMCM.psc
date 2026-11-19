Scriptname Stick2CursorMCM extends SKI_ConfigBase

int modeOpt
int sourceOpt
int speedOpt
int chordAOpt
int chordBOpt
int captureSlot = 0
int captureTicks = 0

int Function GetVersion()
	return 1
EndFunction

Event OnConfigInit()
	ModName = "Stick2Cursor LE"
	Pages = new string[1]
	Pages[0] = "Cursor"
EndEvent

Event OnPageReset(string page)
	SetCursorFillMode(TOP_TO_BOTTOM)
	AddHeaderOption("Activation")
	modeOpt = AddMenuOption("Activation mode", ModeText(Stick2CursorNative.GetInt("Mode")))
	sourceOpt = AddMenuOption("Gamepad input", SourceText(Stick2CursorNative.GetInt("Source")))
	AddEmptyOption()
	SetCursorPosition(1)
	AddHeaderOption("Cursor")
	speedOpt = AddSliderOption("Cursor speed", Stick2CursorNative.GetInt("Speed"), "%.0f")
	chordAOpt = AddTextOption("Chord button 1", ButtonText(Stick2CursorNative.GetInt("ChordA")), OPTION_FLAG_NONE)
	chordBOpt = AddTextOption("Chord button 2", ButtonText(Stick2CursorNative.GetInt("ChordB")), OPTION_FLAG_NONE)
	AddTextOption("Capture status", CaptureText(), OPTION_FLAG_DISABLED)
EndEvent

string Function ModeText(int value)
	if value == 1
		return "Hold chord"
	endif
	return "Toggle on press"
EndFunction

string Function SourceText(int value)
	if value == 1
		return "D-pad"
	endif
	return "Right stick"
EndFunction

string Function ButtonText(int value)
	if value == 0
		return "Not set"
	elseif value == 256
		return "LB"
	elseif value == 512
		return "RB"
	elseif value == 4096
		return "A"
	elseif value == 8192
		return "B"
	elseif value == 16384
		return "X"
	elseif value == 32768
		return "Y"
	elseif value == 1
		return "D-pad Up"
	elseif value == 2
		return "D-pad Down"
	elseif value == 4
		return "D-pad Left"
	elseif value == 8
		return "D-pad Right"
	endif
	return "Button " + value
EndFunction

string Function CaptureText()
	if captureSlot == 1
		return "Press gamepad button for chord 1"
	elseif captureSlot == 2
		return "Press gamepad button for chord 2"
	endif
	return "Select a chord row to capture"
EndFunction

Event OnOptionSelect(int option)
	if option == modeOpt
		int value = 1 - Stick2CursorNative.GetInt("Mode")
		Stick2CursorNative.SetInt("Mode", value)
		SetMenuOptionValue(modeOpt, ModeText(value))
	elseif option == sourceOpt
		int value = 1 - Stick2CursorNative.GetInt("Source")
		Stick2CursorNative.SetInt("Source", value)
		SetMenuOptionValue(sourceOpt, SourceText(value))
	elseif option == chordAOpt
		captureSlot = 1
		captureTicks = 0
		Stick2CursorNative.BeginCapture(1)
		SetTextOptionValue(chordAOpt, "Press a button...")
		RegisterForSingleUpdate(0.1)
	elseif option == chordBOpt
		captureSlot = 2
		captureTicks = 0
		Stick2CursorNative.BeginCapture(2)
		SetTextOptionValue(chordBOpt, "Press a button...")
		RegisterForSingleUpdate(0.1)
	endif
EndEvent

Event OnOptionSliderOpen(int option)
	if option == speedOpt
		SetSliderDialogStartValue(Stick2CursorNative.GetInt("Speed"))
		SetSliderDialogDefaultValue(900)
		SetSliderDialogRange(100, 3000)
		SetSliderDialogInterval(50)
	endif
EndEvent

Event OnOptionSliderAccept(int option, float value)
	if option == speedOpt
		Stick2CursorNative.SetInt("Speed", value as int)
		SetSliderOptionValue(speedOpt, value, "%.0f")
	endif
EndEvent

Event OnOptionHighlight(int option)
	if option == chordAOpt || option == chordBOpt
		SetInfoText("Choose a gamepad button. Capture is limited to the supported XInput buttons.")
	elseif option == modeOpt
		SetInfoText("Toggle switches cursor mode on/off; Hold keeps it active while the chord is held.")
	elseif option == sourceOpt
		SetInfoText("Choose the right stick or D-pad as the cursor source.")
	endif
EndEvent

Event OnUpdate()
	if captureSlot > 0
		captureTicks += 1
		int captured = Stick2CursorNative.Captured(captureSlot)
		if captured > 0
			if captureSlot == 1
				Stick2CursorNative.SetInt("ChordA", captured)
			else
				Stick2CursorNative.SetInt("ChordB", captured)
			endif
			captureSlot = 0
			ForcePageReset()
		elseif captureTicks < 100
			RegisterForSingleUpdate(0.1)
		else
			captureSlot = 0
			ForcePageReset()
		endif
	endif
EndEvent

Event OnConfigClose()
	UnregisterForUpdate()
EndEvent
