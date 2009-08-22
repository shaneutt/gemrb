# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id$


# GUIMG.py - scripts to control mage spells windows from the GUIMG winpack

###################################################

import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from GUICommon import CloseOtherWindow, HasTOB
from GUICommonWindows import *

MageWindow = None
MageSpellInfoWindow = None
MageSpellLevel = 0
MageSpellUnmemorizeWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None
BookType = None

def OpenMageWindow ():
	global MageWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenMageWindow):
		if MageWindow:
			MageWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		MageWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIMG", 640, 480)

	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindowObject (0)
	MarkMenuButton (OptionsWindow)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenMageWindow")
	OptionsWindow.SetFrame ()
	OldPortraitWindow = GUICommonWindows.PortraitWindow

	PortraitWindow = OpenPortraitWindow (0)
	SetupMageWindow()
	SetSelectionChangeHandler (SetupMageWindow)
	return

def SetupMageWindow ():
	global MageWindow
	global BookType

	pc = GemRB.GameGetSelectedPCSingle ()
	BookType = 0
	if (ClassSkillsTable.GetValue (GemRB.GetPlayerStat (pc, IE_CLASS), 8)==2):
		BookType = 1
		
	if MageWindow:
		MageWindow.Unload()
		MageWindow = None

	if BookType:
		MageWindow = Window = GemRB.LoadWindowObject (8)
	else:
		MageWindow = Window = GemRB.LoadWindowObject (2)
	GemRB.SetVar ("OtherWindow", MageWindow.ID)
	
	if BookType:
		label = Window.GetControl (0x10000040)
		label.SetText (61256) #spells can cast

	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "MagePrevLevelPress")

	Button = Window.GetControl (2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "MageNextLevelPress")

	#unknown usage
	Button = Window.GetControl (55)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	#setup level buttons
	for i in range (9):
		Button = Window.GetControl (56 + i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RefreshMageLevel")
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (9):
		Button = Window.GetControl (56 + i)
		Button.SetVarAssoc ("MageSpellLevel", i)

	# Setup memorized spells buttons
	if not BookType:
		for i in range (12):
			Button = Window.GetControl (3 + i)
			Button.SetBorder (0,0,0,0,0,0,0,0,64,0,1)
			Button.SetSprites ("SPELFRAM",0,0,0,0,0)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			Button.SetState (IE_GUI_BUTTON_LOCKED)

	# Setup book spells buttons
	for i in range (24):
		Button = Window.GetControl (27 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	UpdateMageWindow ()
	OptionsWindow.SetVisible (1)
	#bringing the window front
	Window.SetVisible (3)
	PortraitWindow.SetVisible (1)
	return

def UpdateMageWindow ():
	global MageMemorizedSpellList, MageKnownSpellList

	MageMemorizedSpellList = []
	MageKnownSpellList = []

	Window = MageWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	type = IE_SPELL_TYPE_WIZARD
	level = MageSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level, 1)

	Label = Window.GetControl (0x10000032)
	GemRB.SetToken("SPELLLEVEL", str(level+1) )
	Label.SetText (10345)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = Window.GetControl (0x10000035)
	Label.SetText (Name)

	if not BookType:
		mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level)
		for i in range (12):
			Button = Window.GetControl (3 + i)
			if i < mem_cnt:
				ms = GemRB.GetMemorizedSpell (pc, type, level, i)
				Button.SetSpellIcon (ms['SpellResRef'], 0)
				Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
				if ms['Flags']:
					Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMageSpellUnmemorizeWindow")
				else:
					Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnMageUnmemorizeSpell")
				Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenMageSpellInfoWindow")
				spell = GemRB.GetSpell (ms['SpellResRef'])
				Button.SetTooltip (spell['SpellName'])
				MageMemorizedSpellList.append (ms['SpellResRef'])
				Button.SetVarAssoc ("SpellButton", i)
				Button.EnableBorder (0, ms['Flags'] == 0)
			else:
				if i < max_mem_cnt:
					Button.SetFlags (IE_GUI_BUTTON_NORMAL, OP_SET)
				else:
					Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "")
				Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
				Button.SetTooltip ('')
				Button.EnableBorder (0, 0)

	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (24):
		Button = Window.GetControl (27 + i)
		if i < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i)
			Button.SetSpellIcon (ks['SpellResRef'], 0)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnMageMemorizeSpell")
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenMageSpellInfoWindow")
			spell = GemRB.GetSpell (ks['SpellResRef'])
			Button.SetTooltip (spell['SpellName'])
			MageKnownSpellList.append (ks['SpellResRef'])
			Button.SetVarAssoc ("SpellButton", 100 + i)

		else:
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "")
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			Button.SetTooltip ('')
			Button.EnableBorder (0, 0)

	if (ClassSkillsTable.GetValue (GemRB.GetPlayerStat (pc, IE_CLASS), 2)=="*"):
		Window.SetVisible (2)
	else:
		Window.SetVisible (1)
	return

def MagePrevLevelPress ():
	global MageSpellLevel

	if MageSpellLevel > 0:
		MageSpellLevel = MageSpellLevel - 1
		UpdateMageWindow ()
	return

def MageNextLevelPress ():
	global MageSpellLevel

	if MageSpellLevel < 8:
		MageSpellLevel = MageSpellLevel + 1
		UpdateMageWindow ()
	return

def RefreshMageLevel ():
	global MageSpellLevel

	MageSpellLevel = GemRB.GetVar ("MageSpellLevel")
	UpdateMageWindow ()
	return

def OpenMageSpellInfoWindow ():
	global MageSpellInfoWindow

	if MageSpellInfoWindow != None:
		if MageSpellInfoWindow:
			MageSpellInfoWindow.Unload ()
		MageSpellInfoWindow = None
		return

	MageSpellInfoWindow = Window = GemRB.LoadWindowObject (3)

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMageSpellInfoWindow")

	#erase
	index = GemRB.GetVar ("SpellButton")
	if HasTOB():
		Button = Window.GetControl (6)
		if index < 100:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "")
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		else:
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMageSpellRemoveWindow")
			Button.SetText (63668)
	if index < 100:
		ResRef = MageMemorizedSpellList[index]
	else:
		ResRef = MageKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	Label = Window.GetControl (0x0fffffff)
	Label.SetText (spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (ResRef, 1)

	Text = Window.GetControl (3)
	Text.SetText (spell['SpellDesc'])

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnMageMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	type = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, type, level, index):
		UpdateMageWindow ()
	return

def CloseMageSpellUnmemorizeWindow ():
	global MageSpellUnmemorizeWindow

	if MageSpellUnmemorizeWindow:
		MageSpellUnmemorizeWindow.Unload ()
	MageSpellUnmemorizeWindow = None
	return

def OpenMageSpellRemoveWindow ():
	global MageSpellUnmemorizeWindow

	MageSpellUnmemorizeWindow = Window = GemRB.LoadWindowObject (101)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (63745)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnMageRemoveSpell")
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseMageSpellUnmemorizeWindow")
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenMageSpellUnmemorizeWindow ():
	global MageSpellUnmemorizeWindow

	MageSpellUnmemorizeWindow = Window = GemRB.LoadWindowObject (101)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnMageUnmemorizeSpell")
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseMageSpellUnmemorizeWindow")
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnMageUnmemorizeSpell ():
	if MageSpellUnmemorizeWindow:
		CloseMageSpellUnmemorizeWindow()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	type = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, type, level, index):
		UpdateMageWindow ()
	return

def OnMageRemoveSpell ():
	CloseMageSpellUnmemorizeWindow()
	OpenMageSpellInfoWindow()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	type = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton")-100

	#remove spell from book
	GemRB.RemoveSpell (pc, type, level, index)
	UpdateMageWindow ()
	return

###################################################
# End of file GUIMG.py
