/**
* BSD 2-Clause License
*
* Copyright (c) 2022-2023, Manas Kamal Choudhury
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
**/

#include <Fs/vdisk.h>
#include <Fs/_gpt.h>
#include <Mm/kmalloc.h>
#include <_null.h>
#include <aucon.h>
#include <Hal\serial.h>
#include <Mm/pmmngr.h>
#include <string.h>
#include <Fs/Fat/fat.h>

AuVDisk *VdiskArray[MAX_VDISK_DEVICES];


/*
 * AuVDiskInitialise -- initialise the vdisk 
 */
void AuVDiskInitialise() {
	for (int i = 0; i < MAX_VDISK_DEVICES; i++)
		VdiskArray[i] = NULL;
}

/*
 * AuVDiskGetIndex -- returns a vdisk index
 */
uint8_t AuVDiskGetIndex() {
	for (uint8_t i = 0; i < MAX_VDISK_DEVICES; i++){
		if (!VdiskArray[i])
			return i;
	}
}


/*
 * AuCreateVDisk -- creates a vdisk and
 * return to the caller
 */
AuVDisk *AuCreateVDisk(){
	AuVDisk* vdisk = (AuVDisk*)kmalloc(sizeof(AuVDisk));
	memset(vdisk, 0, sizeof(AuVDisk));
	return vdisk;
}

/*
 * AuVDiskRead -- reads a disk block from registered disk
 * @param disk -- Pointer to vdsik structure
 * @param lba -- Linear block address to read
 * @param count -- number of blocks to read
 * @param buffer -- Buffer, where to store the data
 */
size_t AuVDiskRead(AuVDisk *disk, uint64_t lba, uint32_t count, uint64_t* buffer) {
	if (disk->Read) 
		return disk->Read(disk, disk->startingLBA + lba, count, buffer);
	return 0;
}


/*
* AuVDiskWrite -- reads a disk block from registered disk
* @param disk -- Pointer to vdsik structure
* @param lba -- Linear block address to read
* @param count -- number of blocks to read
* @param buffer -- Buffer, where to store the data
*/
size_t AuVDiskWrite(AuVDisk* disk, uint64_t lba, uint32_t count, uint64_t* buffer) {
	if (disk->Write)
		return disk->Write(disk, disk->startingLBA + lba, count, buffer);
	return 0;
}


/* 
 * AuVDiskRegisterPartition - Gether all informations about the partition
 * for now, only GUID Partition is supported
 * @param vdisk -- VDisk structure pointer
 *
 */
void AuVDiskRegisterPartition(AuVDisk* vdisk){
	uint64_t* buffer = (uint64_t*)AuPmmngrAlloc();
	memset(buffer, 0, 4096);
	vdisk->Read(vdisk,1, 1, buffer);
	uint8_t* aligned_buf = (uint8_t*)buffer;

	GPTHeader* header = (GPTHeader*)aligned_buf;

	/* check if it's Efi partition */
	if (strcmp(header->sig, "EFI PART") != 0)
		return;
	for (int i = 0; i < 8; i++)
		SeTextOut("%c", aligned_buf[i]);

	uint64_t part_lba = header->part_table_lba;

	for (int i = 0; i < header->num_part_entries; i++) {
		memset(buffer, 0, 4096);
		vdisk->Read(vdisk, part_lba, 1, buffer);
		aligned_buf = (uint8_t*)buffer;
		GPTPartition* part = (GPTPartition*)aligned_buf;
		if (part->first_lba != 0) {
			vdisk->startingLBA = part->first_lba;
			vdisk->currentLBA = vdisk->startingLBA;
			vdisk->num_partition = 1;
			vdisk->part_guid.Data1 = part->part_guid.Data1;
			vdisk->part_guid.Data2 = part->part_guid.Data2;
			vdisk->part_guid.Data3 = part->part_guid.Data3;
			for (int m = 0; m < 8; m++)
				vdisk->part_guid.Data4[m] = part->part_guid.Data4[m];

			vdisk->part_unique_guid.Data1 = part->part_unique_guid.Data1;
			vdisk->part_unique_guid.Data2 = part->part_unique_guid.Data2;
			vdisk->part_unique_guid.Data3 = part->part_unique_guid.Data3;
			for (int m = 0; m < 8; m++)
				vdisk->part_unique_guid.Data4[m] = part->part_guid.Data4[m];

			for (int j = 0; j < 70; j++)
				AuTextOut("%c", part->part_name[j]);
		}
		part_lba++;
	}
	AuTextOut("\n");
	AuTextOut("VDisk partition created startLBA -> %d \n", vdisk->startingLBA);
	AuTextOut("vDisk partition guid : ");
	AuTextOut("0x%x-0x%x-0x%x-0x", vdisk->part_guid.Data1, vdisk->part_guid.Data2, vdisk->part_guid.Data3);
	for (int k = 0; k < 8; k++)
		AuTextOut("%x", vdisk->part_guid.Data4[k]);

	AuTextOut("\n");
	/* call gpt file system verifier to load
	 * the desired file system 
	 */
	AuGPTInitialise_FileSystem(vdisk);

	AuTextOut("\n");
	AuPmmngrFree(buffer);
}

/*
* AuVDiskRegister -- adds a vdisk service to the list
* @param disk -- disk to add
*/
void AuVDiskRegister(AuVDisk* disk) {
	uint8_t _index = AuVDiskGetIndex();
	/* check for last time, for any errors */
	if (VdiskArray[_index])
		return;

	VdiskArray[_index] = disk;
	AuTextOut("Vdisk registered name -> %s, serial -> %s \n", disk->diskname,
		disk->serialNumber);

	disk->__VDiskID = _index;
	disk->num_partition = 1;
	/* Register a partition and initialise the file system*/
	AuVDiskRegisterPartition(disk);
}

/*
 * AuVDiskDestroy -- destroy's a vdisk 
 * @param vdisk -- pointer to vdisk
 */
void AuVDiskDestroy(AuVDisk *vdisk) {
	uint8_t _index = 0;
	for (uint8_t i = 0; i < MAX_VDISK_DEVICES; i++){
		AuVDisk *disk = VdiskArray[i];
		if (disk == vdisk){
			_index = i;
			break;
		}
	}

	VdiskArray[_index] = NULL;
	kfree(vdisk);
}
