
#include <stdio.h>
#include "md5.h"

/* Example to compute the MD5 digest for the content in a buffer */

#define MD5_DIGEST_LENGTH 16

void example_compute_MD5(char *buf, int len) {
        int n;
        MD5_CTX c;
        unsigned char out[MD5_DIGEST_LENGTH];

	/* Always needed to initialize the external MD5 library */
        MD5_Init(&c);

	/* If needed, this function can be called multiple times, on consecutive buffer segments */
	MD5_Update(&c, buf, len);

	/* Call this function at the end to get the final MD5 value */
        MD5_Final(out, &c);

	/* Example about how to print the MD5 digest in text form */
        for(n=0; n<MD5_DIGEST_LENGTH; n++)
                printf("%02x", out[n]);
        printf("\n");

	return;
}

