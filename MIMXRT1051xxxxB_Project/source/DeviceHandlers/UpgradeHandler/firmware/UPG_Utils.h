#ifndef HUB_FIRMWARE_FWUTILS_H_INCLUDED
#define HUB_FIRMWARE_FWUTILS_H_INCLUDED
/*!****************************************************************************
 *
 * \file UPG_Utilities.h
 *
 * \author Kevin Duffy
 *
 * \brief Enso Upgrade Utilities
 *
 * \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include "firmware/UPG_Firmware.h"
#include <stdbool.h>
#include <sys/types.h>
#include <openssl/ossl_typ.h>

/**
 * \name data_buffer_tag
 *
 * \brief Details of a buffer used to hold data.
 */
typedef struct data_buffer_tag
{
    uint8_t *ptr;   //! Pointer to start of buffer
    size_t size;    //! Buffer size
    size_t bytes;   //! Number of bytes used
    bool adopted;
} data_buffer_t;

bool data_buffer_init(data_buffer_t * data_buffer, uint8_t *bytes,
                          const size_t in_bytes, const bool adopted);
void data_buffer_free(data_buffer_t * data_buffer);


typedef struct sub_buffer_tag
{
    data_buffer_t *buffer;
    size_t offset;
    size_t bytes;
} sub_buffer_t;

bool FileExists(const char * inFilename, int64_t *nrBytes);
bool FileContentsAreEqual(const char *in_a, const char * in_b, bool weak_equality);
bool CopyFile(const char *inFilename, const char *outFilename);
ssize_t SafeRead(int fd, void *buf, size_t count);
ssize_t SafeWrite(int fd, const void *buf, size_t count);
size_t hex_decode(const char* in_string, uint8_t *raw, size_t raw_length);


// The hash is used as IV ..
bool sub_buffer_encrypt(sub_buffer_t *sub_buffer,
             const firmware_key_t *k,
             const firmware_hash_t *h);
bool sub_buffer_decrypt(sub_buffer_t *sub_buffer,
             const firmware_key_t *k,
             const firmware_hash_t *h);

void sub_buffer_hash(sub_buffer_t * sub_buffer, firmware_hash_t *h);


typedef struct part_tag
{
    firmware_part_t part;
    data_buffer_t *data;
} part_t;
        
void part_set_fourcc(part_t *part, const firmware_fourcc_t fourcc);

/** If we have data, compute the hash of that data */
void part_hash(part_t *part, firmware_hash_t *computed);

/** Here is data from a segment. Encrypt or decrypt it. */
bool part_encrypt(part_t *part, int seg_index,
             uint8_t *seg_ptr,
             firmware_hash_t *computed_hash,
             const bool do_decrypt);

/** How many bytes does a segment occupy? This is usually
 *  > its length in bytes, because we pad to the end of the
 *  block.
 */
size_t part_bytes_occupied_by(part_t *part, int seg_index);

/** Read in the part's header */
void part_read_header(part_t *part, const uint8_t *ptr);

/** Write the part's header */
void part_write_header(part_t *part, uint8_t *ptr);

/* Encrypt or decrypt all */
void part_encrypt_all(part_t *part, data_buffer_t *data, bool is_encrypt,
                      bool check_hashes);
/* Compute segment hashes */
void part_compute_segment_hashes(part_t *part);

#define NO_PARTS 16 // TODO: check this value

typedef struct fw_tag
{
    firmware_intro_t intro;
    part_t parts[NO_PARTS];
    uint8_t parts_count;
} fw_t;

/** Read a private key so you can sign */
void load_private_key(const char* in_file);

/** Read a public key so you can verify */
EVP_PKEY * load_public_key();

/** Read a header from some bytes
 *
 * @param[out] computed  On exit, the computed hash
 *                        is placed here. You need
 *                        to do the actual hash check
 *                        if you want to.
 * @param[out] verifies  on exit, true if the signature
 *                      verifies.
 * @param[in] key        If non-NULL use this key to
 *                        decrypt.
 */
void fw_from_bytes(fw_t *fw,
                const uint8_t *bytes,
                const size_t size,
                const firmware_key_t *key,
                const firmware_hash_t *inhash,
                firmware_hash_t *computed,
                bool *verifies);

/** Initialise for a new piece of firmware */
void fw_initialise(fw_t *fw);

void fw_clear(fw_t * fw);

/** Read just the public parts of the header */
bool fw_from_bytes_public(fw_t *fw, const uint8_t *bytes);


/** Read the header, and optionally verify it */
void fw_read_header(fw_t *fw,
                 int fd,
                 const firmware_key_t *key,
                 const firmware_hash_t *in_hash,
                 firmware_hash_t *computed,
                 bool* verifies);

/* Read all parts */
void read_parts(fw_t fw, int fd);

/** Write out to an (encrypted) buffer, giving us
 *  the key as you do so, and changing the salt.
 *
 * @param[in] sign  If true, sign.
 * @param[in] k If k is NULL, we don't encrypt.
 */
void write_header(fw_t fw,
                  uint8_t *ptr,
                  firmware_hash_t * out_hash,
                  const firmware_key_t * k,
                  const bool sign);

/** Assigns keys and offsets to all parts, ready
 *  for write_to()
 */
void assign_offsets(fw_t fw);

/* Write the entire thing (including data) out to
 *  a file.
 */
void write_to(fw_t fw,
              int fd,
              firmware_hash_t *out_hash,
              const firmware_key_t *k,
              const bool sign);

void fw_push_back(fw_t *fw, part_t *p);

void hash_to_string(const firmware_hash_t *f, char *buf);

/** @return true if this hardware spec matches the h/w id you passed. */
bool fw_hw_spec_match(const char *id,
                   const firmware_hardware_spec_t *spec,
                   int nr);

firmware_fourcc_t fw_make_fourcc(const char *p);

bool fw_invent_salt(uint8_t *salt, int nr_bytes);

bool fw_hashes_equal(const firmware_hash_t *a,
                     const firmware_hash_t *b);

/* Needs to be called at the start of your program */
void global_init(void);

#endif

