/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1999-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "Marshal.h"

#include "Byteswapping.h"		// Canonical
#include "ROMStubs.h"			// NetHToNL, NetHToNS

#if PLATFORM_UNIX || PLATFORM_MAC
#include <netinet/in.h>			// ntohl, ntohs
#endif

// -------------------------
// ----- Binary buffer -----
// -------------------------

void* Marshal::GetBuffer (emuptr p, long len)
{
	void*	result = NULL;

	if (p)
	{
		result = Platform::AllocateMemory (len);

		if (result)
		{
			uae_memcpy (result, p, len);
		}
	}

	return result;
}

#if 0	// inline
template <class T>
void Marshal::PutBuffer (emuptr p, T*& buf, long len)
{
	if (p)
	{
		uae_memcpy (p, (void*) buf, len);
		Platform::DisposeMemory (buf);
		buf = NULL;
	}
}
#endif


// ---------------------
// ----- EventType -----
// ---------------------

EventType Marshal::GetEventType (emuptr p)
{
	EventType	event;
	memset (&event, 0, sizeof (event));

	if (p)
	{
		event.eType		= (eventsEnum) EmMemGet16 (p + offsetof (EventType, eType));
		event.penDown	= EmMemGet8 (p + offsetof (EventType, penDown));
		event.screenX	= EmMemGet16 (p + offsetof (EventType, screenX));
		event.screenY	= EmMemGet16 (p + offsetof (EventType, screenY));

		// Aargh! Need to do all the union structs!
	}

	return event;
}


void Marshal::PutEventType (emuptr p, EventType& event)
{
	if (p)
	{
		EmMemPut16 (p + offsetof (EventType, eType),		event.eType);
		EmMemPut8 (p + offsetof (EventType, penDown),	event.penDown);
		EmMemPut16 (p + offsetof (EventType, screenX),	event.screenX);
		EmMemPut16 (p + offsetof (EventType, screenY),	event.screenY);

		// Aargh! Need to do all the union structs!
	}
}


// -----------------------------
// ----- NetSocketAddrType -----
// -----------------------------

NetSocketAddrType Marshal::GetNetSocketAddrType (emuptr p)
{
	NetSocketAddrType	netSocketAddr;
	memset (&netSocketAddr, 0, sizeof (netSocketAddr));

	if (p)
	{
		// We should be able to pretty much just block-copy this sucker without
		// having to worry too much about byte-swapping the contents.  A generic
		// address block is a 2-byte family in HBO followed by 14 bytes of data.
		// For Internet addresses, this 14 bytes of data consists of an IP address
		// and a socket in NBO, so no byteswapping is needed.  For Raw addresses,
		// Palm OS has defined the contents to be a 2-byte instance and a 4-byte
		// creator; there's no mention of their byte-ordering, so for now let's
		// assume NBO as well.
		//
		// By doing a blind memcpy of the contents, we relieve ourselves of the
		// responsibity of determining if they're actually initialized (if they're
		// not initialized, the family field may contain garbage, and we may go
		// off in the weeds trying to interpret the rest of the contents).

		switch (EmMemGet16 (p))
		{
			case netSocketAddrRaw:
				uae_memcpy ((void*) &netSocketAddr, p, sizeof (NetSocketAddrRawType));
				break;

			case netSocketAddrINET:
				uae_memcpy ((void*) &netSocketAddr, p, sizeof (NetSocketAddrINType));
				break;

			default:
				// Do the best we can...
				uae_memcpy ((void*) &netSocketAddr, p, sizeof (NetSocketAddrType));
				break;
		}

		Canonical (netSocketAddr.family);
	}

	return netSocketAddr;
}


void Marshal::PutNetSocketAddrType (emuptr p, NetSocketAddrType& netSocketAddr)
{
	if (p)
	{
		Int16	family = netSocketAddr.family;

		Canonical (netSocketAddr.family);

		switch (family)
		{
			case netSocketAddrRaw:
				uae_memcpy (p, (void*) &netSocketAddr, sizeof (NetSocketAddrRawType));
				break;

			case netSocketAddrINET:
				uae_memcpy (p, (void*) &netSocketAddr, sizeof (NetSocketAddrINType));
				break;

			default:
				// Do the best we can...
				uae_memcpy (p, (void*) &netSocketAddr, sizeof (NetSocketAddrType));
				break;
		}

		Canonical (netSocketAddr.family);
	}
}


// --------------------------
// ----- NetIOParamType -----
// --------------------------

NetIOParamType Marshal::GetNetIOParamType (emuptr p)
{
	NetIOParamType	netIOParam;
	memset (&netIOParam, 0, sizeof (netIOParam));

	if (p)
	{
		netIOParam.addrLen			= EmMemGet16 (p + offsetof (NetIOParamType, addrLen));
		netIOParam.iovLen			= EmMemGet16 (p + offsetof (NetIOParamType, iovLen));
		netIOParam.accessRightsLen	= EmMemGet16 (p + offsetof (NetIOParamType, accessRightsLen));

		netIOParam.addrP			= (UInt8*) GetBuffer (EmMemGet32 (p + offsetof (NetIOParamType, addrP)), netIOParam.addrLen);
		if (netIOParam.addrP)
			Canonical (((NetSocketAddrType*) (netIOParam.addrP))->family);

		netIOParam.accessRights		= (UInt8*) GetBuffer (EmMemGet32 (p + offsetof (NetIOParamType, accessRights)), netIOParam.accessRightsLen);

		netIOParam.iov				= (NetIOVecPtr) Platform::AllocateMemory (netIOParam.iovLen * sizeof (NetIOVecType));
		emuptr iov					= EmMemGet32 (p + offsetof (NetIOParamType, iov));
		for (UInt16 ii = 0; ii < netIOParam.iovLen; ++ii)
		{
			netIOParam.iov[ii].bufLen	= EmMemGet16 (iov + ii * sizeof (NetIOVecType) + offsetof (NetIOVecType, bufLen));
			netIOParam.iov[ii].bufP		= (UInt8*) GetBuffer (iov + ii * sizeof (NetIOVecType) + offsetof (NetIOVecType, bufP), netIOParam.iov[ii].bufLen);
		}
	}

	return netIOParam;
}


void Marshal::PutNetIOParamType (emuptr p, NetIOParamType& netIOParam)
{
	if (p)
	{
		EmMemPut16 (p + offsetof (NetIOParamType, addrLen),			netIOParam.addrLen);
		EmMemPut16 (p + offsetof (NetIOParamType, iovLen),			netIOParam.iovLen);
		EmMemPut16 (p + offsetof (NetIOParamType, accessRightsLen),	netIOParam.accessRightsLen);

		if (netIOParam.addrP)
			Canonical (((NetSocketAddrType*) (netIOParam.addrP))->family);
		PutBuffer (EmMemGet32 (p + offsetof (NetIOParamType, addrP)),	netIOParam.addrP,			netIOParam.addrLen);
		if (netIOParam.addrP)
			Canonical (((NetSocketAddrType*) (netIOParam.addrP))->family);

//		PutBuffer (p + offsetof (NetIOParamType, iov),				netIOParam.iov,				netIOParam.iovLen);
		PutBuffer (EmMemGet32 (p + offsetof (NetIOParamType, accessRights)),	netIOParam.accessRights,	netIOParam.accessRightsLen);

		emuptr iov = EmMemGet32 (p + offsetof (NetIOParamType, iov));
		for (UInt16 ii = 0; ii < netIOParam.iovLen; ++ii)
		{
			EmMemPut16 (iov + ii * sizeof (NetIOVecType) + offsetof (NetIOVecType, bufLen), netIOParam.iov[ii].bufLen);
			PutBuffer (iov + ii * sizeof (NetIOVecType) + offsetof (NetIOVecType, bufP), netIOParam.iov[ii].bufP, netIOParam.iov[ii].bufLen);
		}
		Platform::DisposeMemory (netIOParam.iov);
	}
}


// ------------------------------
// ----- NetHostInfoBufType -----
// ------------------------------

/*
typedef struct {
	NetHostInfoType	hostInfo;
	{
		Char*			nameP; -------------------------+
		Char**			nameAliasesP;-------------------|---+
		UInt16			addrType;                       |   |
		UInt16			addrLen;                        |   |
		UInt8**		addrListP;----------------------|---|---+
	}                                                   |   |   |
										                |   |   |
	Char			name[netDNSMaxDomainName+1];   <----+   |   |
                                                            |   |
	Char*			aliasList[netDNSMaxAliases+1];   <------+   |
	Char			aliases[netDNSMaxAliases][netDNSMaxDomainName+1];
                                                                |
	NetIPAddr*		addressList[netDNSMaxAddresses];   <--------+
	NetIPAddr		address[netDNSMaxAddresses];

	} NetHostInfoBufType, *NetHostInfoBufPtr;
*/

NetHostInfoBufType	Marshal::GetNetHostInfoBufType (emuptr p)
{
	NetHostInfoBufType	netHostInfoBufType;
	memset (&netHostInfoBufType, 0, sizeof (netHostInfoBufType));

	if (p)
	{
	}

	return netHostInfoBufType;
}


void Marshal::PutNetHostInfoBufType (emuptr p, NetHostInfoBufType& netHostInfoBuf)
{
	if (p)
	{
		int	ii;	// Goddampieceoshit VC++ doesn't scope loop variables right!

		// First copy over the easy fields (the ones with real values in them).

			// NetHostInfoBufType.hostInfo.addrType
		EmMemPut16 (p + offsetof (NetHostInfoBufType, hostInfo.addrType),
					netHostInfoBuf.hostInfo.addrType);

			// NetHostInfoBufType.hostInfo.addrLen
		EmMemPut16 (p + offsetof (NetHostInfoBufType, hostInfo.addrLen),
					netHostInfoBuf.hostInfo.addrLen);

			// NetHostInfoBufType.name
		uae_strcpy (p + offsetof (NetHostInfoBufType, name), netHostInfoBuf.name);

			// NetHostInfoBufType.aliases
		uae_memcpy (p + offsetof (NetHostInfoBufType, aliases),
					(void*) netHostInfoBuf.aliases,
					sizeof (netHostInfoBuf.aliases));

			// NetHostInfoBufType.address
			// Copy them one at a time to sort out endian issues.  Just how this
			// is supposed to be done is not clear (NetLib documentation says that
			// the addresses are in HBO, while the WinSock header says that the
			// addresses are supplied in NBO but returned in HBO), but the following
			// seems to work.
		for (ii = 0; ii < netDNSMaxAddresses; ++ii)
		{
			NetIPAddr	addr = ntohl (netHostInfoBuf.address[ii]);
			EmMemPut32 (p + offsetof (NetHostInfoBufType, address[ii]), NetHToNL (addr));
		}

		// Second, set up the pointers to those values.

			// NetHostInfoType.hostInfo.nameP
		EmMemPut32 (p + offsetof (NetHostInfoBufType, hostInfo.nameP),
					p + offsetof (NetHostInfoBufType, name));

			// NetHostInfoType.hostInfo.nameAliasesP
		EmMemPut32 (p + offsetof (NetHostInfoBufType, hostInfo.nameAliasesP),
					p + offsetof (NetHostInfoBufType, aliasList));

			// NetHostInfoType.hostInfo.addrListP
		EmMemPut32 (p + offsetof (NetHostInfoBufType, hostInfo.addrListP),
					p + offsetof (NetHostInfoBufType, addressList));

			// NetHostInfoType.aliasList
		for (ii = 0; ii < netDNSMaxAliases+1; ++ii)
		{
			if (netHostInfoBuf.aliasList[ii] != NULL)
			{
				EmMemPut32 (p + offsetof (NetHostInfoBufType, aliasList[ii]),
					p + netHostInfoBuf.aliasList[ii] - (Char*) &netHostInfoBuf);
			}
			else
			{
				EmMemPut32 (p + offsetof (NetHostInfoBufType, aliasList[ii]), EmMemNULL);
				break;
			}
		}

			// NetHostInfoType.addressList
		for (ii = 0; ii < netDNSMaxAddresses; ++ii)
		{
			if (netHostInfoBuf.addressList[ii] != NULL)
			{
				EmMemPut32 (p + offsetof (NetHostInfoBufType, addressList[ii]),
							p + offsetof (NetHostInfoBufType, address[ii]));
			}
			else
			{
				EmMemPut32 (p + offsetof (NetHostInfoBufType, addressList[ii]),
							EmMemNULL);
				break;
			}
		}
	}
}


// ------------------------------
// ----- NetServInfoBufType -----
// ------------------------------

NetServInfoBufType	Marshal::GetNetServInfoBufType (emuptr p)
{
	NetServInfoBufType	netServInfoBuf;
	memset (&netServInfoBuf, 0, sizeof (netServInfoBuf));

	if (p)
	{
	}

	return netServInfoBuf;
}

void Marshal::PutNetServInfoBufType (emuptr p, NetServInfoBufType& netServInfoBuf)
{
	if (p)
	{
		// =======================================================
		// Convert NetHostInfoBufType.hostInfo
		// =======================================================

		// Get a handy pointer to the servInfo field.
		emuptr	p2 = p + offsetof (NetServInfoBufType, servInfo);

		// -------------------------------------------------------
		// NetServInfoBufType.servInfo.nameP = &NetServInfoBufType.name
		// -------------------------------------------------------

		EmMemPut32 (p2 + offsetof (NetServInfoType, nameP), p + offsetof (NetServInfoBufType, name));

		// -------------------------------------------------------
		// NetServInfoBufType.servInfo.nameAliasesP = &NetServInfoBufType.aliasList
		// -------------------------------------------------------

		EmMemPut32 (p2 + offsetof (NetServInfoType, nameAliasesP), p + offsetof (NetServInfoBufType, aliasList));

		UInt16	port = ntohs (netServInfoBuf.servInfo.port);
		EmMemPut16 (p2 + offsetof (NetServInfoType, port), NetHToNS (port));

		// -------------------------------------------------------
		// NetServInfoBufType.servInfo.protoP = &NetServInfoBufType.protoName
		// -------------------------------------------------------

		EmMemPut32 (p2 + offsetof (NetServInfoType, protoP), p + offsetof (NetServInfoBufType, protoName));


		// =======================================================
		// Convert remaining NetHostInfoBufType fields
		// =======================================================

		// -------------------------------------------------------
		// name, protoName
		// -------------------------------------------------------

		uae_memcpy (p + offsetof (NetServInfoBufType, name), (void*) &netServInfoBuf.name, netServMaxName+1);
		uae_memcpy (p + offsetof (NetServInfoBufType, protoName), (void*) &netServInfoBuf.protoName, netProtoMaxName+1);

		// -------------------------------------------------------
		// aliasList, addressList
		// -------------------------------------------------------

		long	offset;
		long	index = 0;
		while (netServInfoBuf.aliasList[index])
		{
			// aliasList contains a list of string addresses.  Calculate the offset
			// of that string from the beginning of the NetHostInfoBufType.
			offset = ((char*) &netServInfoBuf.aliasList[index]) - (char*) &netServInfoBuf;

			// Use that offset to calculate the address of the corresponding location
			// in the destination.
			EmMemPut32 (p + offsetof (NetServInfoBufType, aliasList) + index * sizeof (Char*), p + offset);

			// Copy over the string.
			uae_strcpy (p + offset, netServInfoBuf.aliasList[index]);

			++index;
		}
		EmMemPut32 (p + offsetof (NetServInfoBufType, aliasList) + index * sizeof (Char*), EmMemNULL);
	}
}

#ifdef SONY_ROM
#include "SonyShared\MarshalSony.inl"
#endif

