/*
 * Hashing function for CUPS.
 *
 * Copyright © 2015-2019 by Apple Inc.
 *
 * Licensed under Apache License v2.0.  See the file "LICENSE" for more
 * information.
 */

/*
 * Include necessary headers...
 */

#include "cups-private.h"
#ifdef __APPLE__
#  include <CommonCrypto/CommonDigest.h>
#elif defined(HAVE_GNUTLS)
#  include <gnutls/crypto.h>
#  include "md5-internal.h"
#else
#  include "md5-internal.h"
#endif /* __APPLE__ */


/*
 * 'cupsHashData()' - Perform a hash function on the given data.
 *
 * The "algorithm" argument can be any of the registered, non-deprecated IPP
 * hash algorithms for the "job-password-encryption" attribute, including
 * "sha" for SHA-1, "sha-256" for SHA2-256, etc.
 *
 * The "hash" argument points to a buffer of "hashsize" bytes and should be at
 * least 64 bytes in length for all of the supported algorithms.
 *
 * The returned hash is binary data.
 *
 * @since CUPS 2.2/macOS 10.12@
 */

ssize_t					/* O - Size of hash or -1 on error */
cupsHashData(const char    *algorithm,	/* I - Algorithm name */
             const void    *data,	/* I - Data to hash */
             size_t        datalen,	/* I - Length of data to hash */
             unsigned char *hash,	/* I - Hash buffer */
             size_t        hashsize)	/* I - Size of hash buffer */
{
  if (!algorithm || !data || datalen == 0 || !hash || hashsize == 0)
  {
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, _("Bad arguments to function"), 1);
    return (-1);
  }

#ifdef __APPLE__
  if (!strcmp(algorithm, "md5"))
  {
   /*
    * MD5 (deprecated but widely used...)
    */

    CC_MD5_CTX	ctx;			/* MD5 context */

    if (hashsize < CC_MD5_DIGEST_LENGTH)
      goto too_small;

    CC_MD5_Init(&ctx);
    CC_MD5_Update(&ctx, data, (CC_LONG)datalen);
    CC_MD5_Final(hash, &ctx);

    return (CC_MD5_DIGEST_LENGTH);
  }
  else if (!strcmp(algorithm, "sha"))
  {
   /*
    * SHA-1...
    */

    CC_SHA1_CTX	ctx;			/* SHA-1 context */

    if (hashsize < CC_SHA1_DIGEST_LENGTH)
      goto too_small;

    CC_SHA1_Init(&ctx);
    CC_SHA1_Update(&ctx, data, (CC_LONG)datalen);
    CC_SHA1_Final(hash, &ctx);

    return (CC_SHA1_DIGEST_LENGTH);
  }
#ifdef CC_SHA224_DIGEST_LENGTH
  else if (!strcmp(algorithm, "sha2-224"))
  {
    CC_SHA256_CTX	ctx;		/* SHA-224 context */

    if (hashsize < CC_SHA224_DIGEST_LENGTH)
      goto too_small;

    CC_SHA224_Init(&ctx);
    CC_SHA224_Update(&ctx, data, (CC_LONG)datalen);
    CC_SHA224_Final(hash, &ctx);

    return (CC_SHA224_DIGEST_LENGTH);
  }
#endif /* CC_SHA224_DIGEST_LENGTH */
  else if (!strcmp(algorithm, "sha2-256"))
  {
    CC_SHA256_CTX	ctx;		/* SHA-256 context */

    if (hashsize < CC_SHA256_DIGEST_LENGTH)
      goto too_small;

    CC_SHA256_Init(&ctx);
    CC_SHA256_Update(&ctx, data, (CC_LONG)datalen);
    CC_SHA256_Final(hash, &ctx);

    return (CC_SHA256_DIGEST_LENGTH);
  }
  else if (!strcmp(algorithm, "sha2-384"))
  {
    CC_SHA512_CTX	ctx;		/* SHA-384 context */

    if (hashsize < CC_SHA384_DIGEST_LENGTH)
      goto too_small;

    CC_SHA384_Init(&ctx);
    CC_SHA384_Update(&ctx, data, (CC_LONG)datalen);
    CC_SHA384_Final(hash, &ctx);

    return (CC_SHA384_DIGEST_LENGTH);
  }
  else if (!strcmp(algorithm, "sha2-512"))
  {
    CC_SHA512_CTX	ctx;		/* SHA-512 context */

    if (hashsize < CC_SHA512_DIGEST_LENGTH)
      goto too_small;

    CC_SHA512_Init(&ctx);
    CC_SHA512_Update(&ctx, data, (CC_LONG)datalen);
    CC_SHA512_Final(hash, &ctx);

    return (CC_SHA512_DIGEST_LENGTH);
  }
#ifdef CC_SHA224_DIGEST_LENGTH
  else if (!strcmp(algorithm, "sha2-512_224"))
  {
    CC_SHA512_CTX	ctx;		/* SHA-512 context */
    unsigned char	temp[CC_SHA512_DIGEST_LENGTH];
                                        /* SHA-512 hash */

   /*
    * SHA2-512 truncated to 224 bits (28 bytes)...
    */

    if (hashsize < CC_SHA224_DIGEST_LENGTH)
      goto too_small;

    CC_SHA512_Init(&ctx);
    CC_SHA512_Update(&ctx, data, (CC_LONG)datalen);
    CC_SHA512_Final(temp, &ctx);

    memcpy(hash, temp, CC_SHA224_DIGEST_LENGTH);

    return (CC_SHA224_DIGEST_LENGTH);
  }
#endif /* CC_SHA224_DIGEST_LENGTH */
  else if (!strcmp(algorithm, "sha2-512_256"))
  {
    CC_SHA512_CTX	ctx;		/* SHA-512 context */
    unsigned char	temp[CC_SHA512_DIGEST_LENGTH];
                                        /* SHA-512 hash */

   /*
    * SHA2-512 truncated to 256 bits (32 bytes)...
    */

    if (hashsize < CC_SHA256_DIGEST_LENGTH)
      goto too_small;

    CC_SHA512_Init(&ctx);
    CC_SHA512_Update(&ctx, data, (CC_LONG)datalen);
    CC_SHA512_Final(temp, &ctx);

    memcpy(hash, temp, CC_SHA256_DIGEST_LENGTH);

    return (CC_SHA256_DIGEST_LENGTH);
  }

#elif defined(HAVE_GNUTLS)
  gnutls_digest_algorithm_t alg = GNUTLS_DIG_UNKNOWN;
					/* Algorithm */
  unsigned char	temp[64];		/* Temporary hash buffer */
  size_t	tempsize = 0;		/* Truncate to this size? */


  if (!strcmp(algorithm, "md5"))
  {
   /*
    * Some versions of GNU TLS disable MD5 without warning...
    */

    _cups_md5_state_t	state;		/* MD5 state info */

    if (hashsize < 16)
      goto too_small;

    _cupsMD5Init(&state);
    _cupsMD5Append(&state, data, (int)datalen);
    _cupsMD5Finish(&state, hash);

    return (16);
  }
  else if (!strcmp(algorithm, "sha"))
    alg = GNUTLS_DIG_SHA1;
  else if (!strcmp(algorithm, "sha2-224"))
    alg = GNUTLS_DIG_SHA224;
  else if (!strcmp(algorithm, "sha2-256"))
    alg = GNUTLS_DIG_SHA256;
  else if (!strcmp(algorithm, "sha2-384"))
    alg = GNUTLS_DIG_SHA384;
  else if (!strcmp(algorithm, "sha2-512"))
    alg = GNUTLS_DIG_SHA512;
  else if (!strcmp(algorithm, "sha2-512_224"))
  {
    alg      = GNUTLS_DIG_SHA512;
    tempsize = 28;
  }
  else if (!strcmp(algorithm, "sha2-512_256"))
  {
    alg      = GNUTLS_DIG_SHA512;
    tempsize = 32;
  }

  if (alg != GNUTLS_DIG_UNKNOWN)
  {
    if (tempsize > 0)
    {
     /*
      * Truncate result to tempsize bytes...
      */

      if (hashsize < tempsize)
        goto too_small;

      gnutls_hash_fast(alg, data, datalen, temp);
      memcpy(hash, temp, tempsize);

      return ((ssize_t)tempsize);
    }

    if (hashsize < gnutls_hash_get_len(alg))
      goto too_small;

    gnutls_hash_fast(alg, data, datalen, hash);

    return ((ssize_t)gnutls_hash_get_len(alg));
  }

#else
 /*
  * No hash support beyond MD5 without CommonCrypto or GNU TLS...
  */

  if (!strcmp(algorithm, "md5"))
  {
    _cups_md5_state_t	state;		/* MD5 state info */

    if (hashsize < 16)
      goto too_small;

    _cupsMD5Init(&state);
    _cupsMD5Append(&state, data, (int)datalen);
    _cupsMD5Finish(&state, hash);

    return (16);
  }
  else if (hashsize < 64)
    goto too_small;
#endif /* __APPLE__ */

 /*
  * Unknown hash algorithm...
  */

  _cupsSetError(IPP_STATUS_ERROR_INTERNAL, _("Unknown hash algorithm."), 1);

  return (-1);

 /*
  * We get here if the buffer is too small.
  */

  too_small:

  _cupsSetError(IPP_STATUS_ERROR_INTERNAL, _("Hash buffer too small."), 1);
  return (-1);
}


/*
 * 'cupsHashString()' - Format a hash value as a hexadecimal string.
 *
 * The passed buffer must be at least 2 * hashsize + 1 characters in length.
 *
 * @since CUPS 2.2.7@
 */

const char *				/* O - Formatted string */
cupsHashString(
    const unsigned char *hash,		/* I - Hash */
    size_t              hashsize,	/* I - Size of hash */
    char                *buffer,	/* I - String buffer */
    size_t		bufsize)	/* I - Size of string buffer */
{
  char		*bufptr = buffer;	/* Pointer into buffer */
  static const char *hex = "0123456789abcdef";
					/* Hex characters (lowercase!) */


 /*
  * Range check input...
  */

  if (!hash || hashsize < 1 || !buffer || bufsize < (2 * hashsize + 1))
  {
    if (buffer)
      *buffer = '\0';
    return (NULL);
  }

 /*
  * Loop until we've converted the whole hash...
  */

  while (hashsize > 0)
  {
    *bufptr++ = hex[*hash >> 4];
    *bufptr++ = hex[*hash & 15];

    hash ++;
    hashsize --;
  }

  *bufptr = '\0';

  return (buffer);
}
