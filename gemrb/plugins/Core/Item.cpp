/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Item.cpp,v 1.6 2005/02/06 11:04:39 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Item.h"
#include "Interface.h"

ITMExtHeader::ITMExtHeader(void)
{
}

ITMExtHeader::~ITMExtHeader(void)
{
  delete [] features;
}

Item::Item(void)
{
	GroundIconBAM = NULL;
	ItemIconBAM = NULL;
	CarriedIconBAM = NULL;
}

Item::~Item(void)
{
  core->FreeITMExt( ext_headers, equipping_features );
	
	if (GroundIconBAM) {
		core->FreeInterface( GroundIconBAM );
		GroundIconBAM = NULL;
	}
	if (ItemIconBAM) {
		core->FreeInterface( ItemIconBAM );
		ItemIconBAM = NULL;
	}
	if (CarriedIconBAM) {
		core->FreeInterface( CarriedIconBAM );
		CarriedIconBAM = NULL;
	}
}
