/* Copyright (C) 2011 NORDUnet A/S
 * See LICENSE for information about licensing.
 */

#include <stdio.h>		/* For sprintf().  */
#include <string.h>
#include <nettle/sha.h>
#include <nettle/hmac.h>

#include <regex.h>
#include <pthread.h>
#include <sys/time.h>
#include "list.h"
#include "radsecproxy.h"
#include "debug.h"

#include "fticks.h"

static void
format_hash(const uint8_t *hash, size_t out_len, uint8_t *out)
{
    int i;

    for (i = 0; i < out_len / 2; i++)
	sprintf((char *) out + i*2, "%02x", hash[i % SHA256_DIGEST_SIZE]);
}

static void
hash(const uint8_t *in,
     const uint8_t *key,
     size_t out_len,
     uint8_t *out)
{
    if (key == NULL) {
	struct sha256_ctx ctx;
	uint8_t hash[SHA256_DIGEST_SIZE];

	sha256_init(&ctx);
	sha256_update(&ctx, strlen((char *) in), in);
	sha256_digest(&ctx, sizeof(hash), hash);
	format_hash(hash, out_len, out);
    }
    else {
	struct hmac_sha256_ctx ctx;
	uint8_t hash[SHA256_DIGEST_SIZE];

	hmac_sha256_set_key(&ctx, strlen((char *) key), key);
	hmac_sha256_update(&ctx, strlen((char *) in), in);
	hmac_sha256_digest(&ctx, sizeof(hash), hash);
	format_hash(hash, out_len, out);
    }
}

/** Hash the MAC in \a IN, keying with \a KEY if it's not NULL.

    \a IN and \a KEY are NULL terminated strings.

    \a IN is sanitised by lowercasing it, removing all but [0-9a-f]
    and truncating it at first ';' (due to RADIUS praxis with tacking
    on SSID to MAC in Calling-Station-Id).  */
void
fticks_hashmac(const uint8_t *in,
	       const uint8_t *key,
	       size_t out_len,
	       uint8_t *out)
{
    /* TODO: lowercase */
    /* TODO: s/[!0-9a-f]//1 */
    /* TODO: truncate after first ';', if any */

    hash(in, key, out_len, out);
}

void
fticks_log(const struct options *options,
	   const struct client *client,
	   const struct radmsg *msg,
	   const struct rqout *rqout)
{
    unsigned char *username = NULL;
    unsigned char *realm = NULL;
    uint8_t visinst[8+40+1+1]; /* Room for 40 octets of VISINST.  */
    uint8_t *macin = NULL;
    uint8_t macout[2*32+1]; /* Room for ASCII representation of SHA256.  */

    username = radattr2ascii(radmsg_gettype(rqout->rq->msg,
					    RAD_Attr_User_Name));
    if (username != NULL) {
	realm = (unsigned char *) strrchr((char *) username, '@');
	if (realm != NULL)
	    realm++;
	else
	    realm = (unsigned char *) "";
    }

    memset(visinst, 0, sizeof(visinst));
    if (options->fticks_reporting == RSP_FTICKS_REPORTING_FULL)
	snprintf((char *) visinst, sizeof(visinst), "VISINST=%s#",
		 client->conf->name);

#define BOGUS_MAC "00:00:00:00:00:00" /* FIXME: Is there a standard
				       * for bogus MAC addresses?  */
    memset(macout, 0, sizeof(macout));
    strncpy((char *) macout, BOGUS_MAC, sizeof(macout) - 1);
    if (options->fticks_mac != RSP_FTICKS_MAC_STATIC) {
	macin = radattr2ascii(radmsg_gettype(rqout->rq->msg,
					     RAD_Attr_Calling_Station_Id));
    }
#if RS_TESTING || 1
    if (macin == NULL)
	macin = (uint8_t *) strdup(BOGUS_MAC);
#endif	/* RS_TESTING */

    switch (options->fticks_mac)
    {
    case RSP_FTICKS_MAC_STATIC:
	memcpy(macout, BOGUS_MAC, sizeof(BOGUS_MAC));
	break;
    case RSP_FTICKS_MAC_ORIGINAL:
	memcpy(macout, macin, sizeof(macout));
	break;
    case RSP_FTICKS_MAC_VENDOR_HASHED:
	fticks_hashmac(macin + 3, NULL, sizeof(macout), macout);
	break;
    case RSP_FTICKS_MAC_VENDOR_KEY_HASHED:
	fticks_hashmac(macin + 3, options->fticks_key, sizeof(macout),
		       macout);
	break;
    case RSP_FTICKS_MAC_FULLY_HASHED:
	fticks_hashmac(macin, NULL, sizeof(macout), macout);
	break;
    case RSP_FTICKS_MAC_FULLY_KEY_HASHED:
	fticks_hashmac(macin, options->fticks_key, sizeof(macout), macout);
	break;
    default:
	debugx(2, DBG_ERR, "invalid fticks mac configuration: %d",
	       options->fticks_mac);
    }
    debug(0xff,
	  "F-TICKS/eduroam/1.0#REALM=%s#VISCOUNTRY=%s#%sCSI=%s#RESULT=%s#",
	  realm,
	  client->conf->fticks_viscountry,
	  visinst,
	  macout,
	  msg->code == RAD_Access_Accept ? "OK" : "FAIL");
    if (macin != NULL)
	free(macin);
    if (username != NULL)
	free(username);
}

/* Local Variables: */
/* c-file-style: "stroustrup" */
/* End: */
