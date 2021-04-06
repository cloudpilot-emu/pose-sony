// SessionFile.h へ追加する関数
// セッションファイルへのメモリスティックサイズの Read/Write
Bool ReadMSSize (MSSizeType& v)
{
	Int32	size;
	Bool	result = fFile.ReadInt (kMSSize, size);

	if (result)
	{
		v = (MSSizeType) size;
	}

	return result;
}

void WriteMSSize (const MSSizeType& v)
{
	fFile.WriteInt (kMSSize, (uint32) v);

	fCfg.fMSSize = v;
}

Bool ReadMSfsInfo  (Chunk& chunk) { return fFile.ReadChunk (kMSfs, chunk); }
void WriteMSfsInfo (const Chunk& chunk) { fFile.WriteChunk (kMSfs, chunk); }

Bool ReadExpMgrInfo  (Chunk& chunk) { return fFile.ReadChunk (sysFileCExpansionMgr, chunk); }
void WriteExpMgrInfo (const Chunk& chunk) { fFile.WriteChunk (sysFileCExpansionMgr, chunk); }

Bool ReadLCDCtrlRegsType (HwrLCDCtrlType& lcdRegs)
{
	Chunk	chunk;
	if (fFile.ReadChunk (kLCDCtrlRegs, chunk))
	{
		memcpy (&lcdRegs, chunk.GetPointer (), sizeof (lcdRegs));
		return true;
	}
	return false;
}

Bool ReadLCDCtrlImage (void* image)
{
	long	result = this->ReadChunk (kLCDCtrlImage, image, kGzipCompression);

	return result;
}

Bool ReadLCDCtrlPalette (RGBType palette[256])
{
	Chunk	chunk;
	if (fFile.ReadChunk (kLCDCtrlPalette, chunk))
	{
		int	size = 256 * sizeof (RGBType);
		memcpy (palette, chunk.GetPointer (), size);
		return true;
	}
	return false;
}

void WriteLCDCtrlRegsType (const HwrLCDCtrlType& lcdRegs)
{
	fFile.WriteChunk (kLCDCtrlRegs, sizeof (lcdRegs), &lcdRegs);
}

void WriteLCDCtrlImage (const void* image, uint32 size)
{
	this->WriteChunk (kLCDCtrlImage, size, image, kGzipCompression);
}

void WriteLCDCtrlPalette (const RGBType palette[256])
{
	int	size = 256 * sizeof (RGBType);
	fFile.WriteChunk (kLCDCtrlPalette, size, palette);
}


