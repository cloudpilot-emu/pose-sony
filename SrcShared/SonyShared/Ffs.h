/*---------------------------------------------------------------------------
 * Ffs.h
 *--------------------------------------------------------------------------*/

#ifndef _FFS_H_
#define _FFS_H_

/*---------------------------------------------------------------------------
 * Note: These types are the same types as in Ffslib.h except they have "ffs"
 *       pre-appended to them.  
 *
 *       These defines must match the cooresponding defines in Ffslib.h.  The
 *       names needed to be changed because they may match the platform's
 *       compiler name.
 *--------------------------------------------------------------------------*/

#include "PalmPack.h"

/*--------------------------------------------------------------------------
 * diskfree_t structure for FfsGetdiskfree()
 *--------------------------------------------------------------------------*/
typedef struct {
    UInt32  avail_clusters;       /* number of free clusters              */
    UInt32  total_clusters;       /* total number of clusters on drive    */
    UInt16  bytes_per_sector;     /* number bytes per sector              */
    UInt16  sectors_per_cluster;  /* number sectors per cluster           */
} ffs_diskfree_t;

/*---------------------------------------------------------------------------
 * ffblk structure for FfsFindfirst(), FfsFindnext()
 *--------------------------------------------------------------------------*/
typedef struct {
    char    ff_reserved[21];   /* used by system -- don't modify!         */
    char    ff_attrib;         /* DOS file attributes                     */
    Int16   ff_ftime;          /* creation time                           */
    Int16   ff_fdate;          /* creation date                           */
    Int32   ff_fsize;          /* file size                               */
    char    ff_name[13];       /* name in 8.3 format                      */
    char    ff_longname[256];  /* long file name                          */
} ffs_ffblk;

/*---------------------------------------------------------------------------
 * stat structure used by FfsStat(), FfsFstat(). Structure date_t is used
 * in stat_t (it is defined as a long in some versions).
 *--------------------------------------------------------------------------*/
typedef struct {
    UInt16  date;        
    UInt16  time;
} ffs_date_t;

typedef struct {
    Int16   st_dev;           /* drive (always 1)                           */
    Int16   st_ino;           /* not used                                   */
    UInt32  st_mode;          /* file mode information                      */
    Int16   st_nlink;         /* always 1                                   */
    Int16   st_uid;           /* not used                                   */
    Int16   st_gid;           /* not used                                   */
    Int16   st_rdev;          /* same as st_dev                             */
    UInt32  st_size;          /* file size                                  */
    ffs_date_t  st_atime;     /* creation date/time                         */
    ffs_date_t  st_mtime;     /* same as st_atime                           */
    ffs_date_t  st_ctime;     /* same as st_atime                           */
    UInt8   st_attr;          /* file attributes (non-standard)             */
} ffs_stat;

#define FFS_S_IREAD    0000200  // Read permitted. (Always true anyway)   
#define FFS_S_IWRITE   0000400  // Write permitted                        
#define FFS_S_IFCHR    0020000  // character special (unused)             
#define FFS_S_IFDIR    0040000  // subdirectory                           
#define FFS_S_IFBLK    0060000  // block special  (unused)                
#define FFS_S_IFREG    0100000  // regular file                           
#define FFS_S_IFMT     0170000  // type of file mask                      


/***************************************************************************
 * Special types for FFS access
 ***************************************************************************/
#define FA_ARCH       0x20              // archive                                  
#define FA_ALL        (UInt16)0x8000    // matches anything for FfsFindfirst()      

#include "PalmPackPop.h"


#endif
