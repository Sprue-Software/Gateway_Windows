/*!****************************************************************************
 *
 * \file UPG_Firmware.h
 *
 * \author Kevin Duffy
 *
 * \brief Enso Upgrade Firmware
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#ifndef HUB_FIRMWARE_FIRMWARE_H_INCLUDED
#define HUB_FIRMWARE_FIRMWARE_H_INCLUDED

/** 
 *  A firmware file consists of a header and a number of parts.
 *  Each part is divided into segments.
 *
 *  The file is saved as:
 *
 *  [HDR]
 *   [PART]
 *    [SEGMENT]
 *    [SEGMENT]
 *   [PART]
 *  ..
 *  [PUBSIG]
 *  ---
 *  [SEGMENT_DATA]
 *  [SEGMENT_DATA]
 *
 *  The header (above the ---) is 32Kbytes long. [PUBSIG] is a 4K-long
 *  public key signature of the header under either the upgrade key
 *  (for upgrades) or the asset key (for assets).
 *
 *  The checksum given at install time applies to the header block.
 *  The header block then gives hashes for everything else.
 *
 *  AES-256 is the current cipher of choice, for purely security
 *  theatre-related reasons.
 *
 *  SHA-256 is the current hash function (it is "good enough" in
 *   all ways, and has the slightly nice property that hashes
 *   are the same size as keys).
 *
 *  Data is encrypted in counter mode; the hashes prevent attacks
 * in depth.
 *
 *  Each part can be downloaded (and checked) individually.
 *
 *  We hash and then encrypt - this does leak information about
 *   the plaintext, but hopefully not by enough to be all that
 *   meaningful - it would allow you to see if the firmware being
 *   upgraded to is the same firmware as some other firmware you
 *   have in your hand, so we have salt in the start of the
 *   main header, and doing it this way allows us to use the hash 
 *   as IV and prevents attacks in depth on the hash.
 * 
 *  The sub-hashes are protected by the encryption on the header,
 *   so don't require salt.
 *
 *  The header is the first 32kbytes of the firmware file, and
 *  is 0xFF-padded.
 *
 *  Assets are just upgrade files with a single asset part 
 *  a single containing asset segment. The asset is saved using a
 *  name given to it by the platform when the download instruction
 *  is given - the name in the asset file is ignored.
 */

#include <stdint.h>

/* Upgrader */
#define FIRMWARE_FOURCC_UPGRADE "UPGR"
/* Operational software set */
#define FIRMWARE_FOURCC_OPERATIONAL "OPER"
/* Recovery software set */
#define FIRMWARE_FOURCC_RECOVERY    "RECV"
/* Bootloader */
#define FIRMWARE_FOURCC_BOOTLOADER  "BTLD"

/* For the header */
#define FIRMWARE_FOURCC_HDR   "SJHF"


#define FIRMWARE_FOURCC_BINARY  "BIN "
#define FIRMWARE_FOURCC_KERNEL  "KRNL"
#define FIRMWARE_FOURCC_ROOTFS  "RTFS"
#define FIRMWARE_FOURCC_DTB     "DTB "

/* Asset file */
#define FIRMWARE_FOURCC_ASSET_PART     "ASST"
#define FIRMWARE_FOURCC_ASSET_SEGMENT  "FILE"
    
#define FIRMWARE_HEADER_BYTES  (32<<10)
// Although we only request FIRMWARE_HEADER_BYTES from the server,
// curl can return some extra bytes which we have to cater for.
#define FIRMWARE_HEADER_EXTRA  256

/* 4k at the end of the header are reserved for the 
 * pubkey signature. This starts with an LE16 signature
 * length and then contains a signature for the 
 * whole header (all 28Kbytes, as encrypted).
 */
#define FIRMWARE_PUBKEY_OFFSET  (FIRMWARE_HEADER_BYTES - (4<<10))
#define FIRMWARE_PUBKEY_BYTES  ((4<<10) - 2)

#define FIRMWARE_ENC_BLK_BYTES  (64)

#define FIRMWARE_MAJOR  1
#define FIRMWARE_MINOR  0

typedef struct firmware_fourcc_struct
{
    char fourcc[4];
} firmware_fourcc_t;

/** Length of a hash */
#define FIRMWARE_HASH_BYTES 0x20
/** Length of a key */
#define FIRMWARE_KEY_BYTES 0x20

typedef struct firmware_hash_struct
{
    // 256 bits == 32 bytes.
    uint8_t hash_data[32];
} firmware_hash_t;

typedef struct firmware_key_struct 
{
    uint8_t key_data[32];
} firmware_key_t;

/** Represents a hardware spec.
 *
 * Alphabetic (A-H) hardware codes are represented with 4 bits per letter: 
 *  0000 == A, 0001 == B, etc.
 *
 * The mask also has four bits per letter. An MSB of 1 means this is a != 
 *  match, an MSB of 0 means this is an == match.
 *
 * The remaining three bits are mask.
 *
 * To find out if an upgrade is legal for this hardware:
 *
 *  - Construct a binary h/w spec by converting your h/w code into
 *    a uint32_t, as above, first letter is MSBits of code.
 *  - AND it with the mask.
 *  - For each nybble, check MSB of mask. 
 *     If 1,  and this digit != the corresponding value digit, return true.
 *     If 0,  and this digit == the corresponding value digit, return true
 *  Otherwise, try the next hardware spec.
 *
 *  This is cunningly constructed so that all-0s (the default) means
 * "allow everything"
 */
typedef struct firmware_hardware_spec_struct
{
    uint32_t mask;
    uint32_t test;
} firmware_hardware_spec_t;

#define FIRMWARE_HW_SPEC_BYTES 0x08

/* Maximum number of segents in an upgrade file */
#define FIRMWARE_NR_SEGS 16

// These need to be multiples of 0x40 bytes so SHA-256 will
// have a whole # of blocks to work with. In fact, we round up
// from 0x1C0 to 0x200.
#define FIRMWARE_INTRO_BYTES (0x200)

/* The first 0x80 bytes of the intro are not encrypted, so
 *  that you can easily tell that this is an upgrade file.
 */
#define FIRMWARE_PLAINTEXT_BYTES 0x80

#define FIRMWARE_PART_BYTES 0x880
#define FIRMWARE_SEGMENT_BYTES 0x80

#define FIRMWARE_SALT_BYTES 0x20
#define FIRMWARE_NAME_BYTES 0x40
#define FIRMWARE_HWVER_BYTES 0x8

/* Maximum  number h/w specs per file */
#define FIRMWARE_FILE_NR_SPECS 24

/* Maximum number of h/w specs per segment */
#define FIRMWARE_SEGMENT_NR_SPECS 8

/* Size in file = 0x100 */
typedef struct firmware_intro_struct
{
    // Some random bytes. The salt helps to guard against chosen-plaintext attacks.
    uint8_t salt[FIRMWARE_SALT_BYTES];
    /* FourCC: Offset 0x20 - this is FIRMWARE_FOURCC_HDR */
    char fourcc[4];
    /* The name of this firmware - a human-readable title for this upgrade.  Offset 0x24 */
    char name[64];
    /* Major version number: Offset 0x64 */
    int major; 
    /* Minor version number: Offset 0x68 */
    int minor;
    /* Deployment Index: 0x6C  */
    int di;
    /* Descriptive h/w string for current file */
    char hwver[8];
    /* Number of parts in this file.  0x78 */
    int nr_parts;
    /* Hardware spec for the file as a whole. 0x80 */
    firmware_hardware_spec_t hw_spec[FIRMWARE_FILE_NR_SPECS];
    // == 0x13C + 0x20 = 0x15C bytes used.
} firmware_intro_t;


/**  Size in file = 0x80 (padded with 0xFF) 
 */
typedef struct firmware_segment_struct
{
    /* FourCC - this will be e.g. FIRMWARE_FOURCC_OPERATIONAL, FIRMWARE_FOURCC_RECOVERY . : 0x00 */
    char fourcc[4];
    /* Offset into part where segment starts: 0x04 */
    uint32_t offset; 
    /* # bytes in segment: 0x08 */
    uint32_t bytes;
    /* Some spare bytes for alignment; fill with some random bit of data: 0x0C */
    uint32_t spare;
    /* Hardware specs */
    firmware_hardware_spec_t hw_spec[FIRMWARE_SEGMENT_NR_SPECS];
    /* 0x10+ .. : Hash of the raw data - needed for verification later */
    firmware_hash_t hash;
} firmware_segment_t;

/** Size in file = 0x880 
 *
 * A quick note on padding: our hash and cipher require whole blocks.
 * We therefore pad out the data in each part so as to give them whole
 *  blocks.
 *
 * Thus, the data for each part is in fact from 
 *  part.offset 
 *  to
 *  next_part.offset
 *
 *  Of which [ part.offset .. part.offset + bytes ] is actual data
 *  and the rest is padding.
 *
 *  We must pad to FIRMWARE_ENC_BLK_BYTES.
 */
typedef struct firmware_part_struct
{
    /* Offset into the file at which this part starts: @ 0x00 */
    uint32_t offset;
    /* Length of the data for this part @  0x04 */
    uint32_t bytes;
    /* FOURCC for this part @ 0x08 */
    char fourcc[4];
    /* Segment list: 0x10 * 0x80 = 0x800 @ 0x10 */
    firmware_segment_t segs[FIRMWARE_NR_SEGS];
    /* Hash of the part : 0x810 */
    firmware_hash_t hash;
    /* Key for this part : 0x830 */
    firmware_key_t key;
    // Total = 0x850
} firmware_part_t;

#endif

/* End file */
