//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//					        FAT16/32 File IO Library
//								    V2.6
// 	  							 Rob Riglar
//						    Copyright 2003 - 2010
//
//   					  Email: rob@robriglar.com
//
//								License: GPL
//   If you would like a version with a more permissive license for use in
//   closed source commercial applications please contact me for details.
//-----------------------------------------------------------------------------
//
// This file is part of FAT File IO Library.
//
// FAT File IO Library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// FAT File IO Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FAT File IO Library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "fat_defs.h"
#include "fat_access.h"
#include "fat_table.h"
#include "fat_write.h"
#include "fat_string.h"
#include "fat_misc.h"

//-----------------------------------------------------------------------------
// fatfs_init: Load FAT Parameters
//-----------------------------------------------------------------------------
int fatfs_init(struct fatfs *fs)
{
	unsigned char   num_of_fats;
	UINT16 reserved_sectors;
	UINT32 FATSz;
	UINT32 root_dir_sectors;
	UINT32 total_sectors;
	UINT32 data_sectors;
	UINT32 count_of_clusters;
	UINT32 partition_size = 0;
	unsigned char   valid_partition = 0;

	fs->currentsector.address = FAT32_INVALID_CLUSTER;
	fs->currentsector.dirty = 0;

	fs->next_free_cluster = 0; // Invalid

	fatfs_fat_init(fs);

	// Make sure we have read and write functions
	if (!fs->disk_io.read_sector || !fs->disk_io.write_sector)
		return FAT_INIT_MEDIA_ACCESS_ERROR;

	// MBR: Sector 0 on the disk
	// NOTE: Some removeable media does not have this.

	// Load MBR (LBA 0) into the 512 byte buffer
	if (!fs->disk_io.read_sector(0, fs->currentsector.sector))
		return FAT_INIT_MEDIA_ACCESS_ERROR;
	
	// Make Sure 0x55 and 0xAA are at end of sector
	// (this should be the case regardless of the MBR or boot sector)
	if (fs->currentsector.sector[SIGNATURE_POSITION] != 0x55 || fs->currentsector.sector[SIGNATURE_POSITION+1] != 0xAA)
		return FAT_INIT_INVALID_SIGNATURE;

	// Now check again using the access function to prove endian conversion function
	if (GET_16BIT_WORD(fs->currentsector.sector, SIGNATURE_POSITION) != SIGNATURE_VALUE) 
		return FAT_INIT_ENDIAN_ERROR;

	// Check the partition type code
	switch(fs->currentsector.sector[PARTITION1_TYPECODE_LOCATION])
	{
		case 0x0B: 
		case 0x06: 
		case 0x0C: 
		case 0x0E: 
		case 0x0F: 
		case 0x05: 
			valid_partition = 1;
		break;
		case 0x00:
			valid_partition = 0;
			break;
		default:
			if (fs->currentsector.sector[PARTITION1_TYPECODE_LOCATION] <= 0x06)
				valid_partition = 1;
		break;
	}

	if (valid_partition)
	{
		// Read LBA Begin for the file system
		fs->lba_begin = GET_32BIT_WORD(fs->currentsector.sector, PARTITION1_LBA_BEGIN_LOCATION);
		partition_size = GET_32BIT_WORD(fs->currentsector.sector, PARTITION1_SIZE_LOCATION);
	}
	// Else possibly MBR less disk
	else
		fs->lba_begin = 0;

	// Load Volume 1 table into sector buffer
	// (We may already have this in the buffer if MBR less drive!)
	if (!fs->disk_io.read_sector(fs->lba_begin, fs->currentsector.sector))
		return FAT_INIT_MEDIA_ACCESS_ERROR;

	// Make sure there are 512 bytes per cluster
	if (GET_16BIT_WORD(fs->currentsector.sector, 0x0B) != FAT_SECTOR_SIZE) 
		return FAT_INIT_INVALID_SECTOR_SIZE;

	// Load Parameters of FAT partition	 
	fs->sectors_per_cluster = fs->currentsector.sector[BPB_SECPERCLUS];
	reserved_sectors = GET_16BIT_WORD(fs->currentsector.sector, BPB_RSVDSECCNT);
	num_of_fats = fs->currentsector.sector[BPB_NUMFATS];
	fs->root_entry_count = GET_16BIT_WORD(fs->currentsector.sector, BPB_ROOTENTCNT);

	if(GET_16BIT_WORD(fs->currentsector.sector, BPB_FATSZ16) != 0)
		fs->fat_sectors = GET_16BIT_WORD(fs->currentsector.sector, BPB_FATSZ16);
	else
		fs->fat_sectors = GET_32BIT_WORD(fs->currentsector.sector, BPB_FAT32_FATSZ32);
	
	// For FAT32 (which this may be)
	fs->rootdir_first_cluster = GET_32BIT_WORD(fs->currentsector.sector, BPB_FAT32_ROOTCLUS);
	fs->fs_info_sector = GET_16BIT_WORD(fs->currentsector.sector, BPB_FAT32_FSINFO);

	// For FAT16 (which this may be), rootdir_first_cluster is actuall rootdir_first_sector
	fs->rootdir_first_sector = reserved_sectors + (num_of_fats * fs->fat_sectors);
	fs->rootdir_sectors = ((fs->root_entry_count * 32) + (FAT_SECTOR_SIZE - 1)) / FAT_SECTOR_SIZE;

	// First FAT LBA address
	fs->fat_begin_lba = fs->lba_begin + reserved_sectors;

	// The address of the first data cluster on this volume
	fs->cluster_begin_lba = fs->fat_begin_lba + (num_of_fats * fs->fat_sectors);

	if (GET_16BIT_WORD(fs->currentsector.sector, 0x1FE) != 0xAA55) // This signature should be AA55
		return FAT_INIT_INVALID_SIGNATURE;

	// Calculate the root dir sectors
	root_dir_sectors = ((GET_16BIT_WORD(fs->currentsector.sector, BPB_ROOTENTCNT) * 32) + (GET_16BIT_WORD(fs->currentsector.sector, BPB_BYTSPERSEC) - 1)) / GET_16BIT_WORD(fs->currentsector.sector, BPB_BYTSPERSEC);
	
	if(GET_16BIT_WORD(fs->currentsector.sector, BPB_FATSZ16) != 0)
		FATSz = GET_16BIT_WORD(fs->currentsector.sector, BPB_FATSZ16);
	else
		FATSz = GET_32BIT_WORD(fs->currentsector.sector, BPB_FAT32_FATSZ32);  

	if(GET_16BIT_WORD(fs->currentsector.sector, BPB_TOTSEC16) != 0)
		total_sectors = GET_16BIT_WORD(fs->currentsector.sector, BPB_TOTSEC16);
	else
		total_sectors = GET_32BIT_WORD(fs->currentsector.sector, BPB_TOTSEC32);

	data_sectors = total_sectors - (GET_16BIT_WORD(fs->currentsector.sector, BPB_RSVDSECCNT) + (fs->currentsector.sector[BPB_NUMFATS] * FATSz) + root_dir_sectors);

	// Find out which version of FAT this is...
	if (fs->sectors_per_cluster != 0)
	{
		count_of_clusters = data_sectors / fs->sectors_per_cluster;

		if(count_of_clusters < 4085) 
			// Volume is FAT12 
			return FAT_INIT_WRONG_FILESYS_TYPE;
		else if(count_of_clusters < 65525) 
		{
			// Clear this FAT32 specific param
			fs->rootdir_first_cluster = 0;

			// Volume is FAT16
			fs->fat_type = FAT_TYPE_16;
			return FAT_INIT_OK;
		}
		else
		{
			// Volume is FAT32
			fs->fat_type = FAT_TYPE_32;
			return FAT_INIT_OK;
		}
	}
	else
		return FAT_INIT_WRONG_FILESYS_TYPE;	
}
//-----------------------------------------------------------------------------
// fatfs_lba_of_cluster: This function converts a cluster number into a sector / 
// LBA number.
//-----------------------------------------------------------------------------
UINT32 fatfs_lba_of_cluster(struct fatfs *fs, UINT32 Cluster_Number)
{
	if (fs->fat_type == FAT_TYPE_16)
		return (fs->cluster_begin_lba + (fs->root_entry_count * 32 / FAT_SECTOR_SIZE) + ((Cluster_Number-2) * fs->sectors_per_cluster));
	else
		return ((fs->cluster_begin_lba + ((Cluster_Number-2)*fs->sectors_per_cluster)));
}
//-----------------------------------------------------------------------------
// fatfs_sector_reader: From the provided startcluster and sector offset
// Returns True if success, returns False if not (including if read out of range)
//-----------------------------------------------------------------------------
int fatfs_sector_reader(struct fatfs *fs, UINT32 Startcluster, UINT32 offset, unsigned char *target)
{
	UINT32 SectortoRead = 0;
	UINT32 ClustertoRead = 0;
	UINT32 ClusterChain = 0;
	UINT32 i;
	UINT32 lba;

	// FAT16 Root directory
	if (fs->fat_type == FAT_TYPE_16 && Startcluster == 0)
	{
		if (offset < fs->rootdir_sectors)
			lba = fs->lba_begin + fs->rootdir_first_sector + offset;
		else
			return 0;
	}
	// FAT16/32 Other
	else
	{
		// Set start of cluster chain to initial value
		ClusterChain = Startcluster;

		// Find parameters
		ClustertoRead = offset / fs->sectors_per_cluster;	  
		SectortoRead = offset - (ClustertoRead*fs->sectors_per_cluster);

		// Follow chain to find cluster to read
		for (i=0; i<ClustertoRead; i++)
			ClusterChain = fatfs_find_next_cluster(fs, ClusterChain);

		// If end of cluster chain then return false
		if (ClusterChain == FAT32_LAST_CLUSTER) 
			return 0;

		// Calculate sector address
		lba = fatfs_lba_of_cluster(fs, ClusterChain)+SectortoRead;
	}

	// User provided target array
	if (target)
		return fs->disk_io.read_sector(lba, target);
	// Else read sector if not already loaded
	else if (lba != fs->currentsector.address)
	{
		fs->currentsector.address = lba;
		return fs->disk_io.read_sector(fs->currentsector.address, fs->currentsector.sector);
	}
	else
		return 1;
}
//-----------------------------------------------------------------------------
// fatfs_sector_writer: Write to the provided startcluster and sector offset
// Returns True if success, returns False if not 
//-----------------------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fatfs_sector_writer(struct fatfs *fs, UINT32 Startcluster, UINT32 offset, unsigned char *target)
{
 	UINT32 SectortoWrite = 0;
	UINT32 ClustertoWrite = 0;
	UINT32 ClusterChain = 0;
	UINT32 LastClusterChain = FAT32_INVALID_CLUSTER;
	UINT32 i;

	// FAT16 Root directory
	if (fs->fat_type == FAT_TYPE_16 && Startcluster == 0)
	{
		UINT32 lba;

		// In FAT16 we cannot extend the root dir!
		if (offset < fs->rootdir_sectors)
			lba = fs->lba_begin + fs->rootdir_first_sector + offset;
		else
			return 0;

		// User target buffer passed in
		if (target)
		{
			// Write to disk
			return fs->disk_io.write_sector(lba, target);
		}
		else
		{
			// Calculate write address
			fs->currentsector.address = lba;

			// Write to disk
			return fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector);
		}
	}
	// FAT16/32 Other
	else
	{
		// Set start of cluster chain to initial value
		ClusterChain = Startcluster;

		// Find parameters
		ClustertoWrite = offset / fs->sectors_per_cluster;	  
		SectortoWrite = offset - (ClustertoWrite*fs->sectors_per_cluster);

		// Follow chain to find cluster to read
		for (i=0; i<ClustertoWrite; i++)
		{
			// Find next link in the chain
			LastClusterChain = ClusterChain;
	  		ClusterChain = fatfs_find_next_cluster(fs, ClusterChain);

			// Dont keep following a dead end
			if (ClusterChain == FAT32_LAST_CLUSTER)
				break;
		}

		// If end of cluster chain 
		if (ClusterChain == FAT32_LAST_CLUSTER) 
		{
			// Add another cluster to the last good cluster chain
			if (!fatfs_add_free_space(fs, &LastClusterChain))
				return 0;

			ClusterChain = LastClusterChain;
		}

		// User target buffer passed in
		if (target)
		{
			// Calculate write address
			UINT32 lba = fatfs_lba_of_cluster(fs, ClusterChain) + SectortoWrite;

			// Write to disk
			return fs->disk_io.write_sector(lba, target);
		}
		else
		{
			// Calculate write address
			fs->currentsector.address = fatfs_lba_of_cluster(fs, ClusterChain)+SectortoWrite;

			// Write to disk
			return fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector);
		}
	}
}
#endif
//-----------------------------------------------------------------------------
// fatfs_show_details: Show the details about the filesystem
//-----------------------------------------------------------------------------
void fatfs_show_details(struct fatfs *fs)
{
	FAT_PRINTF(("\r\nCurrent Disc FAT details\r\n------------------------\r\nRoot Dir First Cluster = "));   
	FAT_PRINTF(("0x%x",fs->rootdir_first_cluster));
	FAT_PRINTF(("\r\nFAT Begin LBA = "));
	FAT_PRINTF(("0x%x",fs->fat_begin_lba));
	FAT_PRINTF(("\r\nCluster Begin LBA = "));
	FAT_PRINTF(("0x%x",fs->cluster_begin_lba));
	FAT_PRINTF(("\r\nSectors Per Cluster = "));
	FAT_PRINTF(("%d",fs->sectors_per_cluster));
	FAT_PRINTF(("\r\n\r\nFormula for conversion from Cluster num to LBA is;"));
	FAT_PRINTF(("\r\nLBA = (cluster_begin_lba + ((Cluster_Number-2)*sectors_per_cluster)))\r\n"));
}
//-----------------------------------------------------------------------------
// fatfs_get_root_cluster: Get the root dir cluster
//-----------------------------------------------------------------------------
UINT32 fatfs_get_root_cluster(struct fatfs *fs)
{
	// NOTE: On FAT16 this will be 0 which has a special meaning...
	return fs->rootdir_first_cluster;
}
//-------------------------------------------------------------
// fatfs_get_file_entry: Find the file entry for a filename
//-------------------------------------------------------------
UINT32 fatfs_get_file_entry(struct fatfs *fs, UINT32 Cluster, char *nametofind, FAT32_ShortEntry *sfEntry)
{
	unsigned char item=0;
	UINT16 recordoffset = 0;
	unsigned char i=0;
	int x=0;
	char *LongFilename;
	char ShortFilename[13];
	struct lfn_cache lfn;
	int dotRequired = 0;
	FAT32_ShortEntry *directoryEntry;

	fatfs_lfn_cache_init(&lfn, TRUE);

	// Main cluster following loop
	while (TRUE)
	{
		// Read sector
		if (fatfs_sector_reader(fs, Cluster, x++, FALSE)) // If sector read was successfull
		{
			// Analyse Sector
			for (item = 0; item < 16; item++)
			{
				// Create the multiplier for sector access
				recordoffset = (32*item);

				// Overlay directory entry over buffer
				directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector+recordoffset);

				// Long File Name Text Found
				if (fatfs_entry_lfn_text(directoryEntry) ) 
					fatfs_lfn_cache_entry(&lfn, fs->currentsector.sector+recordoffset);

				// If Invalid record found delete any long file name information collated
				else if (fatfs_entry_lfn_invalid(directoryEntry) ) 
					fatfs_lfn_cache_init(&lfn, FALSE);

				// Normal SFN Entry and Long text exists 
				else if (fatfs_entry_lfn_exists(&lfn, directoryEntry) ) 
				{
					LongFilename = fatfs_lfn_cache_get(&lfn);

					// Compare names to see if they match
					if (fatfs_compare_names(LongFilename, nametofind)) 
					{
						memcpy(sfEntry,directoryEntry,sizeof(FAT32_ShortEntry));
						return 1;
					}

		 			fatfs_lfn_cache_init(&lfn, FALSE);
				}

				// Normal Entry, only 8.3 Text		 
				else if (fatfs_entry_sfn_only(directoryEntry) )
				{
					memset(ShortFilename, 0, sizeof(ShortFilename));

					// Copy name to string
					for (i=0; i<8; i++) 
						ShortFilename[i] = directoryEntry->Name[i];

					// Extension
					dotRequired = 0;
					for (i=8; i<11; i++) 
					{
						ShortFilename[i+1] = directoryEntry->Name[i];
						if (directoryEntry->Name[i] != ' ')
							dotRequired = 1;
					}

					// Dot only required if extension present
					if (dotRequired)
					{
						// If not . or .. entry
						if (ShortFilename[0]!='.')
							ShortFilename[8] = '.';
						else
							ShortFilename[8] = ' ';
					}
					else
						ShortFilename[8] = ' ';
		  			
					// Compare names to see if they match
					if (fatfs_compare_names(ShortFilename, nametofind)) 
					{
						memcpy(sfEntry,directoryEntry,sizeof(FAT32_ShortEntry));
						return 1;
					}

					fatfs_lfn_cache_init(&lfn, FALSE);
				}
			} // End of if
		} 
		else
			break;
	} // End of while loop

	return 0;
}
//-------------------------------------------------------------
// fatfs_sfn_exists: Check if a short filename exists.
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
//-------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fatfs_sfn_exists(struct fatfs *fs, UINT32 Cluster, char *shortname)
{
	unsigned char item=0;
	UINT16 recordoffset = 0;
	int x=0;
	FAT32_ShortEntry *directoryEntry;

	// Main cluster following loop
	while (TRUE)
	{
		// Read sector
		if (fatfs_sector_reader(fs, Cluster, x++, FALSE)) // If sector read was successfull
		{
			// Analyse Sector
			for (item = 0; item < 16; item++)
			{
				// Create the multiplier for sector access
				recordoffset = (32*item);

				// Overlay directory entry over buffer
				directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector+recordoffset);

				// Long File Name Text Found
				if (fatfs_entry_lfn_text(directoryEntry) ) 
					;

				// If Invalid record found delete any long file name information collated
				else if (fatfs_entry_lfn_invalid(directoryEntry) ) 
					;

				// Normal Entry, only 8.3 Text		 
				else if (fatfs_entry_sfn_only(directoryEntry) )
				{
					if (strncmp((const char*)directoryEntry->Name, shortname, 11)==0)
						return 1;
				}
			} // End of if
		} 
		else
			break;
	} // End of while loop

	return 0;
}
#endif
//-------------------------------------------------------------
// fatfs_update_file_length: Find a SFN entry and update it 
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
//-------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fatfs_update_file_length(struct fatfs *fs, UINT32 Cluster, char *shortname, UINT32 fileLength)
{
	unsigned char item=0;
	UINT16 recordoffset = 0;
	int x=0;
	FAT32_ShortEntry *directoryEntry;

	// Main cluster following loop
	while (TRUE)
	{
		// Read sector
		if (fatfs_sector_reader(fs, Cluster, x++, FALSE)) // If sector read was successfull
		{
			// Analyse Sector
			for (item = 0; item < 16; item++)
			{
				// Create the multiplier for sector access
				recordoffset = (32*item);

				// Overlay directory entry over buffer
				directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector+recordoffset);

				// Long File Name Text Found
				if (fatfs_entry_lfn_text(directoryEntry) ) 
					;

				// If Invalid record found delete any long file name information collated
				else if (fatfs_entry_lfn_invalid(directoryEntry) ) 
					;

				// Normal Entry, only 8.3 Text		 
				else if (fatfs_entry_sfn_only(directoryEntry) )
				{
					if (strncmp((const char*)directoryEntry->Name, shortname, 11)==0)
					{
						directoryEntry->FileSize = fileLength;
						// TODO: Update last write time

						// Update sfn entry
						memcpy((unsigned char*)(fs->currentsector.sector+recordoffset), (unsigned char*)directoryEntry, sizeof(FAT32_ShortEntry));					

						// Write sector back
						return fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector);
					}
				}
			} // End of if
		} 
		else
			break;
	} // End of while loop

	return 0;
}
#endif
//-------------------------------------------------------------
// fatfs_mark_file_deleted: Find a SFN entry and mark if as deleted 
// NOTE: shortname is XXXXXXXXYYY not XXXXXXXX.YYY
//-------------------------------------------------------------
#ifdef FATFS_INC_WRITE_SUPPORT
int fatfs_mark_file_deleted(struct fatfs *fs, UINT32 Cluster, char *shortname)
{
	unsigned char item=0;
	UINT16 recordoffset = 0;
	int x=0;
	FAT32_ShortEntry *directoryEntry;

	// Main cluster following loop
	while (TRUE)
	{
		// Read sector
		if (fatfs_sector_reader(fs, Cluster, x++, FALSE)) // If sector read was successfull
		{
			// Analyse Sector
			for (item = 0; item < 16; item++)
			{
				// Create the multiplier for sector access
				recordoffset = (32*item);

				// Overlay directory entry over buffer
				directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector+recordoffset);

				// Long File Name Text Found
				if (fatfs_entry_lfn_text(directoryEntry) ) 
					;

				// If Invalid record found delete any long file name information collated
				else if (fatfs_entry_lfn_invalid(directoryEntry) ) 
					;

				// Normal Entry, only 8.3 Text		 
				else if (fatfs_entry_sfn_only(directoryEntry) )
				{
					if (strncmp((const char *)directoryEntry->Name, shortname, 11)==0)
					{
						// Mark as deleted
						directoryEntry->Name[0] = 0xE5; 

						// Update sfn entry
						memcpy((unsigned char*)(fs->currentsector.sector+recordoffset), (unsigned char*)directoryEntry, sizeof(FAT32_ShortEntry));					

						// Write sector back
						return fs->disk_io.write_sector(fs->currentsector.address, fs->currentsector.sector);
					}
				}
			} // End of if
		} 
		else
			break;
	} // End of while loop

	return 0;
}
#endif
//-----------------------------------------------------------------------------
// fatfs_list_directory_start: Initialise a directory listing procedure
//-----------------------------------------------------------------------------
void fatfs_list_directory_start(struct fatfs *fs, struct fs_dir_list_status *dirls, UINT32 StartCluster)
{
	dirls->cluster = StartCluster;
	dirls->sector = 0;
	dirls->offset = 0;
}
//-----------------------------------------------------------------------------
// fatfs_list_directory_next: Get the next entry in the directory.
// Returns: 1 = found, 0 = end of listing
//-----------------------------------------------------------------------------
int fatfs_list_directory_next(struct fatfs *fs, struct fs_dir_list_status *dirls, struct fs_dir_ent *entry)
{
	unsigned char i,item;
	UINT16 recordoffset;
	unsigned char LFNIndex=0;
	UINT32 x=0;
	FAT32_ShortEntry *directoryEntry;
	char *LongFilename;
	char ShortFilename[13];
	struct lfn_cache lfn;
	int dotRequired = 0;
	int result = 0;
 
	// Initialise LFN cache first
	fatfs_lfn_cache_init(&lfn, FALSE);

	while (TRUE)
	{
		// If data read OK
		if (fatfs_sector_reader(fs, dirls->cluster, dirls->sector, FALSE))
		{
			LFNIndex=0;

			// Maximum of 16 directory entries
			for (item = dirls->offset; item < 16; item++)
			{
				// Increase directory offset 
				recordoffset = (32*item);

				// Overlay directory entry over buffer
				directoryEntry = (FAT32_ShortEntry*)(fs->currentsector.sector+recordoffset);
		 
				// Long File Name Text Found
				if ( fatfs_entry_lfn_text(directoryEntry) )   
					fatfs_lfn_cache_entry(&lfn, fs->currentsector.sector+recordoffset);
				 	 
				// If Invalid record found delete any long file name information collated
				else if ( fatfs_entry_lfn_invalid(directoryEntry) ) 	
					fatfs_lfn_cache_init(&lfn, FALSE);

				// Normal SFN Entry and Long text exists 
				else if (fatfs_entry_lfn_exists(&lfn, directoryEntry) ) 
				{
					// Get text
					LongFilename = fatfs_lfn_cache_get(&lfn);
					strncpy(entry->filename, LongFilename, FATFS_MAX_LONG_FILENAME-1);

		 			if (fatfs_entry_is_dir(directoryEntry)) 
						entry->is_dir = 1; 
					else
						entry->is_dir = 0;

					entry->size = directoryEntry->FileSize;
					entry->cluster = (directoryEntry->FstClusHI<<16) | directoryEntry->FstClusLO;

					// Next starting position
					dirls->offset = item + 1;
					result = 1;
		 			return 1;
				}			
				// Normal Entry, only 8.3 Text		 
				else if ( fatfs_entry_sfn_only(directoryEntry) )
				{
       				fatfs_lfn_cache_init(&lfn, FALSE);
					
					memset(ShortFilename, 0, sizeof(ShortFilename));

					// Copy name to string
					for (i=0; i<8; i++) 
						ShortFilename[i] = directoryEntry->Name[i];

					// Extension
					dotRequired = 0;
					for (i=8; i<11; i++) 
					{
						ShortFilename[i+1] = directoryEntry->Name[i];
						if (directoryEntry->Name[i] != ' ')
							dotRequired = 1;
					}

					// Dot only required if extension present
					if (dotRequired)
					{
						// If not . or .. entry
						if (ShortFilename[0]!='.')
							ShortFilename[8] = '.';
						else
							ShortFilename[8] = ' ';
					}
					else
						ShortFilename[8] = ' ';
		  			
					strncpy(entry->filename, ShortFilename, FATFS_MAX_LONG_FILENAME-1);

		 			if (fatfs_entry_is_dir(directoryEntry)) 
						entry->is_dir = 1; 
					else
						entry->is_dir = 0;

					entry->size = directoryEntry->FileSize;
					entry->cluster = (directoryEntry->FstClusHI<<16) | directoryEntry->FstClusLO;

					// Next starting position
					dirls->offset = item + 1;
					result = 1;
					return 1;			
				}
			}// end of for

			// If reached end of the dir move onto next sector
			dirls->sector++;
			dirls->offset = 0;
		}
		else
			break;
	}

	return result;
} 
