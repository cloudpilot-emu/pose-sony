
#ifndef	_SLOTDRVLIB_H
#define _SLOTDRVLIB_H

#define	CardReservedSectorRangePtr	UInt32
typedef struct CardMetricsTag {
	UInt32	physicalSectorStart;
	UInt32	numSectors;
	UInt16	bytesPerSector;
	UInt8	sectorsPerCluster;
	UInt8	reserved;
	UInt16	sectorsPerHead;
	UInt16	headsPerCylinder;
	UInt32	numReservedRanges;
	CardReservedSectorRangePtr reservedRangesP;
} CardMetricsType, *CardMetricsPtr;

#endif	// _SLOTDRVLIB_H

