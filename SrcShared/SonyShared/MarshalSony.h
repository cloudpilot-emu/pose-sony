		static ExpCardInfoType		GetCardInfoBufType (emuptr p);
		static void					PutCardInfoBufType (emuptr p, ExpCardInfoType& Info);

		static VolumeInfoType		GetVolumeInfoBufType (emuptr p);
		static void					PutVolumeInfoBufType (emuptr p, VolumeInfoType& vinfo);

		static FileInfoType			GetFileInfoBufType (emuptr p);
		static void					PutFileInfoBufType (emuptr p, FileInfoType& finfo);

		static VFSAnyMountParamType	GetVFSAnyMountParamType (emuptr p);
		static void					PutVFSAnyMountParamType (emuptr p, VFSAnyMountParamType& mparam);

		static CardMetricsType		GetCardMetricsType (emuptr p);
		static void					PutCardMetricsType (emuptr p, CardMetricsType& metrics);

		inline static void PutParamVal (long loc, ExpCardInfoType& v)	// for Sony & ExpansionMgr
			{ PutCardInfoBufType (loc, v); }

		inline static void GetParamVal (long loc, ExpCardInfoType& v)	// for Sony & ExpansionMgr
			{ v = GetCardInfoBufType (loc); }

		inline static void PutParamVal (long loc, VolumeInfoType& v)	// for Sony & ExpansionMgr
			{ PutVolumeInfoBufType (loc, v); }

		inline static void GetParamVal (long loc, VolumeInfoType& v)	// for Sony & ExpansionMgr
			{ v = GetVolumeInfoBufType (loc); }

		inline static void PutParamVal (long loc, FileInfoType& v)		// for Sony & VFSMgr
			{ PutFileInfoBufType (loc, v); }

		inline static void GetParamVal (long loc, FileInfoType& v)		// for Sony & VFSMgr
			{ v = GetFileInfoBufType (loc); }

		inline static void PutParamVal (long loc, VFSAnyMountParamType& v)		// for Sony & VFSMgr
			{ PutVFSAnyMountParamType (loc, v); }

		inline static void GetParamVal (long loc, VFSAnyMountParamType& v)		// for Sony & VFSMgr
			{ v = GetVFSAnyMountParamType (loc); }

		inline static void PutParamVal (long loc, CardMetricsType& v)		// for Sony & SlotDriver Lib
			{ PutCardMetricsType (loc, v); }

		inline static void GetParamVal (long loc, CardMetricsType& v)		// for Sony & SlotDriver Lib
			{ v = GetCardMetricsType (loc); }
