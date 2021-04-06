
ExpCardInfoType Marshal::GetCardInfoBufType (emuptr p)		// fot Sony & MemoryStick
{
	ExpCardInfoType	Info;
	memset (&Info, 0, sizeof (ExpCardInfoType));

	if (p)
	{
		Info.capabilityFlags = get_long (p + offsetof (ExpCardInfoType, capabilityFlags));
		uae_memcpy ((void*) &Info.manufacturerStr,		p + offsetof (ExpCardInfoType, manufacturerStr),	expCardInfoStringMaxLen+1);
		uae_memcpy ((void*) &Info.productStr,			p + offsetof (ExpCardInfoType, productStr),			expCardInfoStringMaxLen+1);
		uae_memcpy ((void*) &Info.deviceClassStr,		p + offsetof (ExpCardInfoType, deviceClassStr),		expCardInfoStringMaxLen+1);
		uae_memcpy ((void*) &Info.deviceUniqueIDStr,	p + offsetof (ExpCardInfoType, deviceUniqueIDStr),	expCardInfoStringMaxLen+1);
	}
	return Info;
}

void Marshal::PutCardInfoBufType (emuptr p, ExpCardInfoType& Info)	// fot Sony & MemoryStick
{
	if (p)
	{
		put_long	(p + offsetof (ExpCardInfoType, capabilityFlags),	Info.capabilityFlags);
		uae_memcpy	(p + offsetof (ExpCardInfoType, manufacturerStr),	(void*) &Info.manufacturerStr,	 expCardInfoStringMaxLen+1);
		uae_memcpy	(p + offsetof (ExpCardInfoType, productStr),		(void*) &Info.productStr,		 expCardInfoStringMaxLen+1);
		uae_memcpy	(p + offsetof (ExpCardInfoType, deviceClassStr),	(void*) &Info.deviceClassStr,	 expCardInfoStringMaxLen+1);
		uae_memcpy	(p + offsetof (ExpCardInfoType, deviceUniqueIDStr),	(void*) &Info.deviceUniqueIDStr, expCardInfoStringMaxLen+1);
	}
}

VolumeInfoType Marshal::GetVolumeInfoBufType (emuptr p)		// fot Sony & MemoryStick
{
	VolumeInfoType	volInfo;
	memset (&volInfo, 0, sizeof (VolumeInfoType));

	if (p)
	{
		volInfo.attributes		= get_long (p + offsetof (VolumeInfoType, attributes));
		volInfo.fsType			= get_long (p + offsetof (VolumeInfoType, fsType));
		volInfo.fsCreator		= get_long (p + offsetof (VolumeInfoType, fsCreator));
		volInfo.mountClass		= get_long (p + offsetof (VolumeInfoType, mountClass));
		volInfo.slotRefNum		= get_word (p + offsetof (VolumeInfoType, slotRefNum));
		volInfo.slotLibRefNum	= get_word (p + offsetof (VolumeInfoType, slotLibRefNum));
		volInfo.mediaType		= get_long (p + offsetof (VolumeInfoType, mediaType));
		volInfo.reserved		= get_long (p + offsetof (VolumeInfoType, reserved));
	}
	return volInfo;
}

void Marshal::PutVolumeInfoBufType (emuptr p, VolumeInfoType& vinfo)	// fot Sony & MemoryStick
{
	if (p)
	{
		put_long	(p + offsetof (VolumeInfoType, attributes),		vinfo.attributes);
		put_long	(p + offsetof (VolumeInfoType, fsType),			vinfo.fsType);
		put_long	(p + offsetof (VolumeInfoType, fsCreator),		vinfo.fsCreator);
		put_long	(p + offsetof (VolumeInfoType, mountClass),		vinfo.mountClass);
		put_word	(p + offsetof (VolumeInfoType, slotLibRefNum),	vinfo.slotLibRefNum);
		put_word	(p + offsetof (VolumeInfoType, slotRefNum),		vinfo.slotRefNum);
		put_long	(p + offsetof (VolumeInfoType, mediaType),		vinfo.mediaType);
		put_long	(p + offsetof (VolumeInfoType, reserved),		vinfo.reserved);
	}
}

FileInfoType Marshal::GetFileInfoBufType (emuptr p)
{
	FileInfoType	fileInfo;
	memset (&fileInfo, 0, sizeof (FileInfoType));

	if (p)
	{
		fileInfo.attributes	= get_long (p + offsetof (FileInfoType, attributes));
		fileInfo.nameBufLen	= get_word (p + offsetof (FileInfoType, nameBufLen));

		emuptr	nameAddr = get_long(p + offsetof (FileInfoType, nameP));
		if (nameAddr && fileInfo.nameBufLen > 0)
		{
			fileInfo.nameP = new char[fileInfo.nameBufLen];
			uae_memcpy ((void*) fileInfo.nameP, nameAddr, fileInfo.nameBufLen);
		}
	}

	return fileInfo;
}

void Marshal::PutFileInfoBufType (emuptr p, FileInfoType& finfo)	// for Sony & MemoryStick
{
	if (p)
	{
		put_long	(p + offsetof (FileInfoType, attributes),	finfo.attributes);
		put_word	(p + offsetof (FileInfoType, nameBufLen),	finfo.nameBufLen);

		emuptr	nameAddr = get_long(p + offsetof (FileInfoType, nameP));
		if (finfo.nameP)
		{
			uae_memcpy	(nameAddr,	(void*) finfo.nameP, finfo.nameBufLen);
			delete finfo.nameP;
			finfo.nameP = NULL;
		}
	}
}

VFSAnyMountParamType Marshal::GetVFSAnyMountParamType (emuptr p)
{
	VFSAnyMountParamType	mParam;
	memset (&mParam, 0, sizeof (VFSAnyMountParamType));

	if (p)
	{
		mParam.volRefNum	= get_word (p + offsetof (VFSAnyMountParamType, volRefNum));
		mParam.reserved		= get_word (p + offsetof (VFSAnyMountParamType, reserved));
		mParam.mountClass	= get_long (p + offsetof (VFSAnyMountParamType, mountClass));
	}

	return mParam;
}

void Marshal::PutVFSAnyMountParamType (emuptr p, VFSAnyMountParamType& mparam)	// for Sony & MemoryStick
{
	if (p)
	{
		put_word	(p + offsetof (VFSAnyMountParamType, volRefNum),	mparam.volRefNum);
		put_word	(p + offsetof (VFSAnyMountParamType, reserved),		mparam.reserved);
		put_long	(p + offsetof (VFSAnyMountParamType, mountClass),	mparam.mountClass);
	}
}

CardMetricsType Marshal::GetCardMetricsType (emuptr p)		// fot Sony & SlotDriver Lib
{
	CardMetricsType	metrics;
	memset (&metrics, 0, sizeof (CardMetricsType));

	if (p)
	{
		metrics.physicalSectorStart	= get_long (p + offsetof (CardMetricsType, physicalSectorStart));
		metrics.numSectors			= get_long (p + offsetof (CardMetricsType, numSectors));
		metrics.bytesPerSector		= get_word (p + offsetof (CardMetricsType, bytesPerSector));
		metrics.sectorsPerCluster	= get_byte (p + offsetof (CardMetricsType, sectorsPerCluster));
		metrics.reserved			= get_byte (p + offsetof (CardMetricsType, reserved));
		metrics.sectorsPerHead		= get_word (p + offsetof (CardMetricsType, sectorsPerHead));
		metrics.headsPerCylinder	= get_word (p + offsetof (CardMetricsType, headsPerCylinder));
		metrics.numReservedRanges	= get_long (p + offsetof (CardMetricsType, numReservedRanges));
		metrics.reservedRangesP		= get_long (p + offsetof (CardMetricsType, reservedRangesP));
	}
	return metrics;
}

void Marshal::PutCardMetricsType (emuptr p, CardMetricsType& metrics)	// fot Sony & MemoryStick
{
	if (p)
	{
		put_long	(p + offsetof (CardMetricsType, physicalSectorStart),	metrics.physicalSectorStart);
		put_long	(p + offsetof (CardMetricsType, numSectors),			metrics.numSectors);
		put_word	(p + offsetof (CardMetricsType, bytesPerSector),		metrics.bytesPerSector);
		put_byte	(p + offsetof (CardMetricsType, sectorsPerCluster),		metrics.sectorsPerCluster);
		put_byte	(p + offsetof (CardMetricsType, reserved),				metrics.reserved);
		put_word	(p + offsetof (CardMetricsType, sectorsPerHead),		metrics.sectorsPerHead);
		put_word	(p + offsetof (CardMetricsType, headsPerCylinder),		metrics.headsPerCylinder);
		put_long	(p + offsetof (CardMetricsType, numReservedRanges),		metrics.numReservedRanges);
		put_long	(p + offsetof (CardMetricsType, reservedRangesP),		metrics.reservedRangesP);
	}
}

