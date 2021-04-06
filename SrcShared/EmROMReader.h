/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2000-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#ifndef EmROMReader_h
#define EmROMReader_h

#include "EmPalmStructs.h"

// Define this to 1 if you want to dump a list of databases
// to the log file as the ROM is being enumerated.  This is
// handy when trying to find something in the ROM that differentiates
// it from other ROMs.

#define LOG_DATABASES_HACK	0

#if LOG_DATABASES_HACK
#include "Logging.h"		// LogAppendMsg
#endif

#include <vector>
#include <map>
#include <string>

template <class A>
class EmROMReader
{
	public:
		typedef typename A::ptr_type ptr_type;
		
		class ChunkType {
			friend class EmROMReader;
			unsigned long	fSize;
			bool			fFree;
			unsigned long	fSizeAdj;
			int				fLockCount;
			int				fHOffset;
			int 			fOwner;
			ptr_type		fPtr;
			unsigned long	fAddress;
			int	fHdrSize;
			
			
		public:
			bool			Free(void)			{ return fFree; }
			int				Owner(void)			{ return fOwner; }
			unsigned long	TotalSize(void)		{ return fSize; }
			size_t			Size(void)			{ return fSize - fHdrSize - fSizeAdj; }
			ptr_type		GetHdrPtr(void)		{ return fPtr; }
			ptr_type		GetPtr(void)		{ return A::add(fPtr, (int)(fHdrSize)); }
			UInt32			LocalID(void)		{ return fAddress; }

			bool			Contains(ptr_type pos, size_t len=1)
												{ return (A::diff(pos, fPtr) >= 0) &&
													((A::diff(pos, fPtr) + len) <= fSize); }
		
			enum {
				typeOther,
				typeRecord,
				typeDBHeader,
				typeDBTable,
				typeFree
			} type;
			unsigned long	recordNumber;
			unsigned long	dbNumber;

		public:
			ChunkType()
			{
				fSize = fSizeAdj = 0;
				fFree = false;
				fAddress = 0;
				fOwner = fHOffset = fLockCount = 0;
				fHdrSize = 0;

				recordNumber = 0;
				type = typeOther;
				dbNumber = 0;
			}
			
			ChunkType(const EmAliasROMHeapChunkHdr2Type<A>& chunk, UInt32 addr)
			{
				fPtr = chunk.GetPtr();
				fSize = (chunk.long1 >> 0) & 0x00FFFFFFU;
				if (fSize == 0)
				{
					fFree = 0;
					fSizeAdj = 0;
					fLockCount = 0;
					fOwner = 0;
					fHdrSize = 0;
					fAddress = 0;
				}
				else
				{
					fFree = (chunk.long1 >> 31) & 0x0001;
					fSizeAdj = (chunk.long1 >> 24) & 0x000F;
					fLockCount = (chunk.long2 >> 28) & 0x000F;
					fHOffset = (chunk.long2 >> 0) & 0x00FFFFFFU;
					fOwner = (chunk.long2 >> 24) & 0x000F;
					fHdrSize = chunk.GetSize();
					fAddress = addr + fHdrSize;
				}

				recordNumber = 0;
				dbNumber = 0;
				type = fFree ? typeFree : typeOther;
			}

			ChunkType(const EmAliasROMHeapChunkHdr1Type<A>& chunk, UInt32 addr)
			{
				fPtr = chunk.GetPtr();
				fSize = chunk.size;
				if (fSize == 0)
				{
					fFree = 0;
					fSizeAdj = 0;
					fLockCount = 0;
					fOwner = 0;
					fHdrSize = 0;
					fAddress = 0;
				}
				else
				{
					fFree = !!(chunk.flags & 0x80);
					fSizeAdj = chunk.flags & 0x0F;
					fLockCount = chunk.lockOwner >> 4;
					fHOffset = chunk.hOffset;
					fOwner = chunk.lockOwner & 0x0F;
					fHdrSize = chunk.GetSize();
					fAddress = addr + fHdrSize;
				}

				recordNumber = 0;
				dbNumber = 0;
				type = fFree ? typeFree : typeOther;
			}
			
			ChunkType & operator=(const ChunkType& other)
			{
				fPtr = other.fPtr;
				fSize = other.fSize;
				fSizeAdj = other.fSizeAdj;
				fLockCount = other.fLockCount;
				fHOffset = other.fHOffset;
				fOwner = other.fOwner;
				fAddress = other.fAddress;
				fFree = other.fFree;
				fHdrSize = other.fHdrSize;
				recordNumber = other.recordNumber;
				dbNumber = other.dbNumber;
				type = other.type;
				
				return *this;
			}
			
		};
		
		class DatabaseItemType {
			friend class EmROMReader;
			
		private:
			bool			fRecord;
			unsigned long	fID;
			unsigned long	fType;
			int				fCategory;
			bool			fDeleted;
			bool			fDirty;
			bool			fBusy;
			bool			fSecret;
			UInt32			fChunkLID;
			
			ChunkType *	fChunk;

		public:
		
			bool			Deleted(void)	{ return fDeleted; }
			bool			Dirty(void)		{ return fDirty; }
			bool			Busy(void)		{ return fBusy; }
			bool			Secret(void)	{ return fSecret; }
			int				Category(void)	{ return fCategory; }
			unsigned long	ID(void)		{ return fID; }
			unsigned long	Type(void)		{ return fType; }
			
			bool			Record(void)	{ return fRecord; }
			
			ChunkType *		Chunk(void) 	{ return fChunk; }

		public:
			
			DatabaseItemType(void)
			{
				fDeleted = fDirty = fBusy = fSecret = false;
				fCategory = fType = fID = fChunkLID = 0;
                                                                        				
				fChunk = NULL;
			}
			
			DatabaseItemType(const EmAliasROMDBRecordHeader1Type<A> record, EmROMReader * rom)
			{
				UInt32 bits = record.recordID;
				
				fRecord = true;
				
				fDeleted = !!((bits >> 31) & 1);
				fDirty = !!((bits >> 30) & 1);
				fBusy = !!((bits >> 29) & 1);
				fSecret = !!((bits >> 28) & 1);
				fCategory = (bits >> 24) & 0xF;
				fID = bits & 0x00FFFFFFU;
				
				fType = 0;
				
				fChunkLID = record.chunkLID;
                                                                        				
				ChunkList::iterator block = rom->Chunks().find(fChunkLID);
				if (block != rom->Chunks().end())
					fChunk = &block->second;
				else
					fChunk = NULL;
				
			}
			
			DatabaseItemType(const EmAliasROMDBResourceHeader1Type<A> resource, EmROMReader * rom)
			{
				fRecord = false;
				
				fType = resource.type;
				fID = resource.id;
				
				fDeleted = fDirty = fBusy = fSecret = false;
				fCategory = 0;
				
				fChunkLID = resource.chunkLID;
				
				ChunkList::iterator block = rom->Chunks().find(fChunkLID);
				if (block != rom->Chunks().end())
					fChunk = &block->second;
				else
					fChunk = NULL;

			}
			
		
		};
		
		typedef std::vector<DatabaseItemType>	DatabaseItemList;

		class DatabaseType {
			friend class EmROMReader;

		private:
			
			DatabaseItemList	fItems;
			
			std::string		fName;
			unsigned long	fCreator, fType;
			
			unsigned long	fCreationTime, fModificationTime, fBackupTime;
			
			unsigned int	fFlags, fVersion;
			
			unsigned long	fUniqueIDSeed;
			
			ChunkType *		fAppInfoChunk;
			ChunkType *		fSortInfoChunk;

		public:
		
			std::string &		Name(void)				{ return fName; }
			const std::string &	Name(void)	const		{ return fName; } 
			DatabaseItemList &	Items(void)				{ return fItems; }
			
			unsigned long 		CreationTime(void)		{ return fCreationTime; }
			unsigned long		ModificationTime(void)	{ return fModificationTime; }
			unsigned long		BackupTime(void)		{ return fBackupTime; }
			unsigned int		Flags(void)				{ return fFlags; }
			unsigned int		Version(void)			{ return fVersion; }
			unsigned long		UniqueIDSeed(void)		{ return fUniqueIDSeed; }
			
			unsigned long		Creator(void)			{ return fCreator; }
			unsigned long		Type(void)				{ return fType; }
			
		public:
		
			DatabaseType(void)
			{
				fAppInfoChunk = fSortInfoChunk = NULL;
			}
			
			DatabaseType(ChunkType * hdr, EmROMReader * rom)
			{
				int i;
				
				if (hdr->Size() < EmAliasROMDBHeader1Type<A>::GetSize())
					return;

				EmAliasROMDBHeader1Type<A>	db(hdr->GetPtr());
				
				fName = std::string(32, '\x0');
				A::BlockCopy(fName.begin(), db.name.GetPtr(), 32);

				fName.resize(strlen(fName.c_str()));
				
				fCreationTime = db.creationTime;
				fModificationTime = db.modificationTime;
				fBackupTime = db.backupTime;
				fType = db.type;
				fCreator = db.creator;
				fVersion = db.version;
				fUniqueIDSeed = db.uniqueIDSeed;
				fFlags = db.flags;
				fVersion = db.version;
					
				fAppInfoChunk = 0;
				if (db.appinfoLID != 0) {
					ChunkList::iterator ai = rom->Chunks().find(db.appinfoLID);
					if (ai != rom->Chunks().end())
						fAppInfoChunk = &ai->second;
				}
				
				fSortInfoChunk = 0;
				if (db.sortinfoLID != 0) {
					ChunkList::iterator si = rom->Chunks().find(db.sortinfoLID);
					if (si != rom->Chunks().end())
						fSortInfoChunk = &si->second;
				}
				
				for (i=0; i<db.recordCount; i++)
					if (db.flags & 1)
						fItems.push_back(DatabaseItemType(db.resources[i], rom));
					else
						fItems.push_back(DatabaseItemType(db.records[i], rom));
			}
			
		};

	private:
	
		ptr_type		fPtr;
		size_t			fSize;
		ptr_type		fEndPtr;
		
		UInt32			fCardAddress;

		typedef std::map<unsigned long, ChunkType> ChunkList;
		ChunkList		fChunks;
		
		typedef std::vector<DatabaseType> DatabaseList;
		DatabaseList	fDatabases;

		typedef std::map<unsigned long, std::map<int, int> > FeatureList;
		FeatureList		fFeatures;

		bool			fFlag328;
		bool			fFlagEZ;
		bool			fFlagVZ;

		UInt16			fCardVersion;
		std::string		fCardName, fCardManuf;

		UInt16			fStoreVersion;
		
		UInt32			fCompanyID, fHalID;
		UInt32			fRomVersion;
		std::string		fRomVersionString;
		
		
		
		ChunkType *		fSplashChunk;

	public:

		ChunkList & 	Chunks(void)	{ return fChunks; }
		DatabaseList &	Databases(void)	{ return fDatabases; }
		FeatureList &	Features(void)	{ return fFeatures; }
		ChunkType * GetSplashChunk(void) const { return fSplashChunk; }
		ptr_type GetCardHeaderPtr(void) const { return fPtr; }
		ptr_type GetCardStorePtr(void)  const { return A::add(fPtr, EmAliasROMCardHeader5Type<A>::GetSize()); }
		bool GetFlag328(void)			const { return fFlag328; }

#ifdef SONY_ROM
		bool GetFlagEZ(void)			const 
		{ 
			if ((!fCardManuf.compare(0, 16, Sony_MANUF) || fCardManuf == "Sony") && ContainsDB(SONY_PEGN700C_DB))
			{
				return false;
			}
			return fFlagEZ; 
		}

		bool GetFlagVZ(void)			const 
		{ 
			if ((!fCardManuf.compare(0, 16, Sony_MANUF) || fCardManuf == "Sony") && ContainsDB(SONY_PEGN700C_DB))
			{
				return true;
			}
			return fFlagVZ; 
		}
#else
		bool GetFlagEZ(void)			const { return fFlagEZ; }
		bool GetFlagVZ(void)			const { return fFlagVZ; }
#endif // SONY_ROM

		UInt16 GetCardVersion(void)		const { return fCardVersion; }
		std::string GetCardName(void)	const { return fCardName; }
		std::string GetCardManufacturer(void) const { return fCardManuf; }
		UInt16 GetStoreVersion(void)	const { return fStoreVersion; }
		UInt32 GetCompanyID(void)		const { return fCompanyID; }
		UInt32 GetHalID(void)			const { return fHalID; }
		UInt32 GetRomVersion(void)		const { return fRomVersion; }
		std::string GetRomVersionString(void) const { return fRomVersionString; }
	
	public:

		EmROMReader(ptr_type arenaPtr, size_t arenaSize) :
			fPtr(arenaPtr),
			fSize(arenaSize),
			fEndPtr(A::add(arenaPtr, arenaSize)),
			fCardAddress(0),
			fCardVersion(0),
			fStoreVersion(0),
			fCompanyID(0),
			fHalID(0),
			fRomVersion(0),
			fRomVersionString(),
			fSplashChunk(0)
		{ 

		}

		~EmROMReader()
		{
		}
		
		bool AcquireCardHeader(void)
		{	
			// Make sure the buffer is big enough for a card header
			
			if (fSize < EmAliasROMCardHeader5Type<A>::GetSize())
				return false;
		
			EmAliasROMCardHeader5Type<A>	cardHdr(fPtr);
		
			// Make sure the stack size isn't something ludicrous.
		
			if ((cardHdr.initStack == 0) || 
			    (cardHdr.initStack > 0x000FFFFFL))  // 1 Meg
				return false;
			
			if (cardHdr.signature != sysCardSignature)
				return false;
		
			// Make sure the header version isn't ludicrous.
		
			if ((cardHdr.hdrVersion == 0) ||
			    (cardHdr.hdrVersion > 255))        // arbitrary val
				return false;
		
			// Make sure this isn't a "RAM only" card.
		
			if (cardHdr.flags & memCardHeaderFlagRAMOnly)
				return false;
			
			char buffer[33];
			
			fFlag328 = (cardHdr.flags & 0x0010) != 0;
			fFlagEZ =  (cardHdr.flags & 0x0020) != 0;
			fFlagVZ =  (cardHdr.flags & 0x0040) != 0;
			fCardVersion = cardHdr.hdrVersion;
			
			buffer[32] = '\0';
			
			A::BlockCopy(buffer, cardHdr.cardName.GetPtr(), 32);
			fCardName = buffer;
			A::BlockCopy(buffer, cardHdr.cardManuf.GetPtr(), 32);
			fCardManuf = buffer;

			fCompanyID = cardHdr.companyID;
			fHalID = cardHdr.halID;
			fRomVersion = cardHdr.romVersionNumber;
			A::BlockCopy(buffer, cardHdr.romVersionString.GetPtr(), 32);
			fRomVersionString = buffer;
			
			SetCardAddress(GuessCardAddress());
			
			unsigned long bigROM = (fCardVersion < 2) ? 0x3000 : cardHdr.bigrom;
			
			if ((fCardAddress & 0x0FFFFF) != (bigROM & 0x0FFFFF)) {
				/* Whoops, we probably just read a Small ROM. Skip to the
				   probable Big ROM, and continue from there. */

				fPtr = A::add(fPtr, bigROM & 0x0FFFFF);
				fSize -= (bigROM & 0x0FFFFF);

				return AcquireCardHeader();
			}
			
			return true; // Card header verified properly
		}

		UInt32 GuessCardAddress(void)
		{
			EmAliasROMCardHeader5Type<A>	cardHdr(fPtr);

			return (unsigned long)cardHdr.ramlist
				- 0x200;
		}

		void SetCardAddress(UInt32 address)
		{
			fCardAddress = address;
		}
	
		bool AcquireROMHeap(void)
		{
			// Local IDs cannot be understood without card address

			if (!fCardAddress)
				return false;
			
			EmAliasROMStoreHeader4Type<A>	storeHdr(GetCardStorePtr());
			
			if (storeHdr[1].GetPtr() >= fEndPtr)
				return false;	// Buffer big enough for store header?

			if (storeHdr.signature != sysStoreSignature)
				return false;

			fStoreVersion = storeHdr.version;
			
			ptr_type romHeapListPtr = A::add(fPtr, (UInt32)storeHdr.heapAddr-fCardAddress);
			
			EmAliasUInt16<A> romHeapCount(romHeapListPtr);
			
			if (romHeapListPtr < fPtr)
				return false;	// Heap list within buffer?
			if (romHeapCount[1].GetPtr() >= fEndPtr)
				return false;	// Buffer big enough for heap list?
			
			EmAliasUInt32<A> romHeapList(romHeapCount[1].GetPtr());

			fChunks.clear();

			fDatabases.clear();
			
			fFeatures.clear();
			
			/* Heap arrangements past version 1 will only have
			   a single heap. */
			
			for (int heap=0; heap<romHeapCount; heap++) {
			
				if (romHeapList[heap+1].GetPtr() >= fEndPtr)
					return false;	// Buffer big enough for heap list?

				ptr_type romHeapPtr = A::add(fPtr, (UInt32)romHeapList[heap]-fCardAddress);
				unsigned long checkSize;

				if (romHeapPtr < fPtr)
					return false;	// Heap within buffer?

				EmAliasROMHeapHeader1Type<A>	heapHdr1(romHeapPtr);
			
				if (heapHdr1.flags & 0x2000) { // Version 4 heap
					EmAliasROMHeapHeader3Type<A>	heapHdr3(romHeapPtr);

					checkSize = heapHdr3[1].GetSize();

					if (heapHdr3[1].GetPtr() >= fEndPtr)
						return false;	// Buffer big enough for heap header?

					ptr_type chunkPtr = heapHdr3[1].GetPtr();
					unsigned long addr = A::diff(chunkPtr, fPtr) + fCardAddress;

					for (;;) {
			
						if (chunkPtr >= fEndPtr)
							goto fail;

						EmAliasROMHeapChunkHdr2Type<A> chunkHdr(chunkPtr);
						ChunkType ct(chunkHdr, addr);
				
						if (ct.TotalSize() == 0)
							break;
					
						/* store a ChunkType record mapped
						   by the "local ID" of each chunk */
					
						fChunks[ct.LocalID()] = ct;
					
						chunkPtr = A::add(chunkPtr, ct.TotalSize());
						addr += ct.TotalSize();
						checkSize += ct.TotalSize();
				
					}
 
					checkSize += 4; /* zero-length tail "chunk" */
				
					if (checkSize != heapHdr3.size)
						goto fail;	// Double-check of heap size failed
	
					// If a free chunk is listed, perform verification
	
					if ((UInt32)heapHdr3.freeChunk) {
	
						UInt32 freeLID = (UInt32)romHeapList + ((UInt32)heapHdr3.freeChunk * 2) + 8;
	
						if (fChunks.find(freeLID) == fChunks.end())
							goto fail;	// Unable to find "free" chunk
		
						if (!fChunks[freeLID].Free())
							goto fail;	// Supposedly free chunk isn't
					}

				} else if (heapHdr1.flags & 0x4000) { // Version 3 heap

					EmAliasROMHeapHeader3Type<A>	heapHdr3(romHeapPtr);
				
					checkSize = heapHdr3[1].GetSize();

					if (heapHdr3[1].GetPtr() >= fEndPtr)
						return false;	// Buffer big enough for heap header?

					ptr_type chunkPtr = heapHdr3[1].GetPtr();
					unsigned long addr = A::diff(chunkPtr, fPtr) + fCardAddress;

					for (;;) {
			
						if (chunkPtr >= fEndPtr)
							goto fail;

						EmAliasROMHeapChunkHdr2Type<A> chunkHdr(chunkPtr);
						ChunkType ct(chunkHdr, addr);
				
						if (ct.TotalSize() == 0)
							break;
					
						/* store a ChunkType record mapped
						   by the "local ID" of each chunk */
					
						fChunks[ct.LocalID()] = ct;
					
						chunkPtr = A::add(chunkPtr, ct.TotalSize());
						addr += ct.TotalSize();
						checkSize += ct.TotalSize();
				
					}

					checkSize += 4; /* zero-length tail "chunk" */
				
					if (checkSize != heapHdr3.size)
						goto fail;	// Double-check of heap size failed
	
					// If a free chunk is listed, perform verification
	
					if ((UInt32)heapHdr3.freeChunk) {
	
						UInt32 freeLID = (UInt32)romHeapList + ((UInt32)heapHdr3.freeChunk * 2) + 8;
	
						if (fChunks.find(freeLID) == fChunks.end())
							goto fail;	// Unable to find "free" chunk
	
						if (!fChunks[freeLID].Free())
							goto fail;	// Supposedly free chunk isn't
					}

				} else if (heapHdr1.flags & 0x8000) { // Version 2 heap
					EmAliasROMHeapHeader2Type<A>	heapHdr2(romHeapPtr);

					checkSize = heapHdr2[1].GetSize();

					if (heapHdr2[1].GetPtr() >= fEndPtr)
						return false;	// Buffer big enough for heap header?

					ptr_type chunkPtr = heapHdr2[1].GetPtr();
					unsigned long addr = A::diff(chunkPtr, fPtr) + fCardAddress;

					for (;;) {
			
						if (chunkPtr >= fEndPtr)
							goto fail;

						EmAliasROMHeapChunkHdr2Type<A> chunkHdr(chunkPtr);
						ChunkType ct(chunkHdr, addr);
				
						if (ct.TotalSize() == 0)
							break;
					
						/* store a ChunkType record mapped
						   by the "local ID" of each chunk */

						fChunks[ct.LocalID()] = ct;

						chunkPtr = A::add(chunkPtr, ct.TotalSize());
						addr += ct.TotalSize();
						checkSize += ct.TotalSize();
				
					}

					checkSize += 4; /* zero-length tail "chunk" */
				
					if (checkSize != heapHdr2.size)
						goto fail;	// Double-check of heap size failed

				} else { // Version 1 heap

					EmAliasROMHeapHeader1Type<A>	heapHdr1(romHeapPtr);

					checkSize = heapHdr1[1].GetSize();

					if (heapHdr1[1].GetPtr() >= fEndPtr)
						return false;	// Buffer big enough for heap header?

					ptr_type chunkPtr = heapHdr1[1].GetPtr();
					unsigned long addr = A::diff(chunkPtr, fPtr) + fCardAddress;

					for (;;) {
			
						if (chunkPtr >= fEndPtr)
							goto fail;

						EmAliasROMHeapChunkHdr1Type<A> chunkHdr(chunkPtr);
						ChunkType ct(chunkHdr, addr);
				
						if (ct.TotalSize() == 0)
							break;
					
						/* store a ChunkType record mapped
						   by the "local ID" of each chunk */

						fChunks[ct.LocalID()] = ct;

						chunkPtr = A::add(chunkPtr, ct.TotalSize());
						addr += ct.TotalSize();
						checkSize += ct.TotalSize();
				
					}

					checkSize += 2; /* zero-length tail "chunk" */

					if ((UInt16)(checkSize & 0xFFFF) != (UInt16)heapHdr1.size)
						goto fail;	// Double-check of heap size failed
	
				}
			
			} // next heap
			
			return true; // Sucessfull acquisition
			
		fail:
			fChunks.clear();
			return false;	// Failure
				
		}

		bool AcquireDatabases(void)
		{
			if (fChunks.size() == 0)
				return false;	// Must get chunks first
			
			EmAliasROMStoreHeader4Type<A>	storeHdr(GetCardStorePtr());
			
			ChunkList::iterator dbHdrChunk = fChunks.find(storeHdr.dbLID);
			if (dbHdrChunk == fChunks.end())
				return false; // Could not find database directory chunk

			dbHdrChunk->second.type = ChunkType::typeDBTable;
			
			fDatabases.clear();
			
			fFeatures.clear();

			ptr_type dbHdrPtr = dbHdrChunk->second.GetPtr();
			
			
#if LOG_DATABASES_HACK
			LogAppendMsg ("-----");
#endif
			for (;;) {
			
				EmAliasROMDBDir1Type<A>	dbHdr(dbHdrPtr);
				
				for (int i=0; i<dbHdr.count; i++) {

					ChunkList::iterator dbChunk = fChunks.find(dbHdr.dbLID[i]);
					if (dbChunk == fChunks.end())
						continue; // Could not find database directory chunk

					dbChunk->second.type = ChunkType::typeDBHeader;

#if LOG_DATABASES_HACK
					DatabaseType	dbInfo(&dbChunk->second, this);

					LogAppendMsg ("%s", dbInfo.Name ().c_str ());
#endif

					fDatabases.push_back(DatabaseType(&dbChunk->second, this));
						
					for (unsigned int j=0; j<fDatabases[i].Items().size(); j++) {
						ChunkType * chunk = fDatabases[i].Items()[j].Chunk();
						if (chunk) {
							chunk->type = ChunkType::typeRecord;
							chunk->dbNumber = i;
							chunk->recordNumber = j;
						}
					}

				}
				
				if (dbHdr.nextTable == 0)
					break;
				else {
					dbHdrChunk = fChunks.find(dbHdr.nextTable);
					if (dbHdrChunk == fChunks.end())
						break; // Could not find database directory chunk
					dbHdrPtr = dbHdrChunk->second.GetPtr();
				}				
			}
			
			return true;
		}

		bool AcquireFeatures(void)
		{
			/* Nothing to see here... */
			if (fDatabases.size() == 0)
				return false;	// Must get databases first

			fFeatures.clear();
			
			DatabaseList::iterator dbiter = fDatabases.begin();
			for (; dbiter != fDatabases.end(); dbiter++)
			{
				if (dbiter->Creator() == 'psys')
				{
					for (DatabaseItemList::iterator iter = dbiter->Items().begin();
						iter != dbiter->Items().end();
						iter++)
					{
						if (iter->Type() == 'feat')
						{
							EmAliasROMFtrTableType<A> table(iter->Chunk()->GetPtr());
							
							ptr_type 	pos = table.creator.GetPtr();
							
							for (int i=0; i<table.numEntries; i++)
							{
								EmAliasROMFtrCreatorType<A> creator(pos);
								
								int j;
								for (j=0; j<creator.numEntries; j++)
								{
									fFeatures[creator.creator][creator.feature[j].num] 
										= creator.feature[j].value;
								}
								pos = creator.feature[j].GetPtr();
							}
						}
					}
				}
			}

			return true;
		}
		
		bool AcquireSplashDB(void)
		{
			fSplashChunk = 0;
			
			for (DatabaseList::iterator iter = fDatabases.begin();
				iter != fDatabases.end();
				iter++)
			{
				if (iter->Creator() == 'psys')
				{
					for (EmROMReader<A>::DatabaseItemList::iterator jter = iter->Items().begin();
						jter != iter->Items().end();
						jter++)
					{
						if ((jter->Type() == 'Tbsb') &&
							(jter->ID() == 19001))
						{
							fSplashChunk = jter->Chunk();
							return true;
						}
					}
				}
			}
			
			return false;
		}
		
		bool	ContainsDB(std::string	dbName)	const
		{
			for (	EmROMReader<A>::DatabaseList::const_iterator i = fDatabases.begin();
				i != fDatabases.end();
				i++) {
				if (i->Name() == dbName)
					return true;
			}
			return false;
		}

		void	Version(int & major,
				int & minor,
				int & fix,
				int & stage,
				int & build)	const
		{
			major = minor = fix = stage = build = 0;
			
			FeatureList::const_iterator psys = fFeatures.find('psys');
			if (psys == fFeatures.end())
				return;

			std::map<int, int>::const_iterator psys_1 = psys->second.find(1);
			if (psys_1 == psys->second.end())
				return;

			unsigned long ver = psys_1->second;
			major = (ver >> 24) & 0x00FF;
			minor = (ver >> 20) & 0x000F;
			fix   = (ver >> 16) & 0x000F;
			stage = (ver >> 12) & 0x000F;
			build = (ver >>  0) & 0x0FFF;

		}

		unsigned long	Version(void) const
		{
			int major, minor, fix, state, build;
			Version(major, minor, fix, state, build);
			
			return (major << 16) | (minor << 8) | fix;
		}

		static int IsBitmapColor (ChunkType & chunk, bool bootScreen)
		{
			ptr_type pos = chunk.GetPtr();

			if (bootScreen)
				pos = A::add(pos, 6);

			for (;;) {
				if (!chunk.Contains(pos, EmAliasBitmapType<A>::GetSize()))
					return -1;

				EmAliasBitmapType<A> bitmap(pos);
				
				if (bitmap.pixelSize >= 8)
					return 1;
				
				if (!bitmap.nextDepthOffset)
					break;
				
				unsigned long offset = bitmap.nextDepthOffset;

				pos = A::add(pos, offset * 4);
			}
			return 0;
		}
};


#endif	// EmROMReader_h
