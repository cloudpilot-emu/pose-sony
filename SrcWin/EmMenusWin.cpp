/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmMenusWin.h"

#include "Platform.h"			// Platform::AllocateMemory


// ---------------------------------------------------------------------------
//		¥ HostCreatePopupMenu
// ---------------------------------------------------------------------------
// Create a popup menu based on the given menu hierarchy.

HMENU HostCreatePopupMenu (const EmMenuItemList& menu)
{
	HMENU	result = ::CreatePopupMenu ();

	EmMenuItemList::const_iterator	iter = menu.begin ();
	while (iter != menu.end ())
	{
		if (iter->GetIsDivider ())
		{
			::AppendMenu (result, MF_SEPARATOR, NULL, NULL);
		}
		else
		{
			const EmMenuItemList&	children = iter->GetChildren ();

			if (children.size () == 0)
			{
				string	title (iter->GetTitle ());

				if (iter->GetShortcut ())
				{
					title += "\tAlt+";
					title += iter->GetShortcut ();
				}

				UINT	flags	= MF_STRING;

				if (!iter->GetIsActive ())
				{
					flags |= MF_GRAYED;
				}

				if (iter->GetIsChecked ())
				{
					flags |= MF_CHECKED;
				}

				::AppendMenu (result, flags,
					(UINT_PTR) iter->GetCommand (), title.c_str ());
			}
			else
			{
				string	title	= iter->GetTitle ();
				HMENU	subMenu = HostCreatePopupMenu (children);

				::AppendMenu (result, MF_POPUP,
					(UINT_PTR) subMenu, title.c_str ());
			}
		}

		++iter;
	}

	return result;
}


// ---------------------------------------------------------------------------
//		¥ PrvCollectKeys
// ---------------------------------------------------------------------------
// Create a collection of all the menu items containing shortcut keys in the
// given menu hierarchy.

void PrvCollectKeys (const EmMenuItemList& menu, EmMenuItemList& result)
{
	EmMenuItemList::const_iterator	iter = menu.begin ();
	while (iter != menu.end ())
	{
		if (!iter->GetIsDivider ())
		{
			const EmMenuItemList&	children = iter->GetChildren ();

			if (children.size () != 0)
			{
				::PrvCollectKeys (children, result);
			}
			else
			{
				if (iter->GetShortcut ())
				{
					result.push_back (*iter);
				}
			}
		}

		++iter;
	}
}


// ---------------------------------------------------------------------------
//		¥ HostCreateAcceleratorTable
// ---------------------------------------------------------------------------
// Create a Windows accelerator table from the shortcut keys in the given
// menu.

HACCEL	HostCreateAcceleratorTable	(const EmMenuItemList& menu)
{
	// Collect and flatten all the menu items with shortcut keys.

	EmMenuItemList	keys;
	::PrvCollectKeys (menu, keys);

	// Create an ACCEL table large enough to hold the results.

	size_t	tableSize = keys.size () * sizeof (ACCEL);
	ACCEL*	accel = (ACCEL*) Platform::AllocateMemory (tableSize);

	// Fill in the table.

	int							index = 0;
	EmMenuItemList::iterator	iter = keys.begin ();
	while (iter != keys.end ())
	{
		char	ch = iter->GetShortcut ();

		accel[index].fVirt	= FALT | FVIRTKEY;
		accel[index].key	= LOBYTE (::VkKeyScan (ch));
		accel[index].cmd	= iter->GetCommand ();

		++index;
		++iter;
	}

	// Create the HACCEL handle for the system.

	HACCEL	hAccel = ::CreateAcceleratorTable (accel, keys.size ());

	// Clean up the table we used to create the HACCEL.

	Platform::FreeMemory (accel);

	return hAccel;
}
