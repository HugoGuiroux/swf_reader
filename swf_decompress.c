#include "swf.h"
#include <zlib.h>
#include <assert.h>

#define CHUNK 16384

void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

int main(int argc, char **argv)
{
    Swf* swf = NULL;
    FILE* output = NULL;
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    if (argc < 3)
    {
        fprintf(stderr, "Usage : %s <swf_filename> <output_filename>\n", argv[0]);
        return 1;
    }

    swf = swf_open(argv[1]);
    swf_read_header(swf);
    if (swf->header->Signature[0] != 'C')
    {
        fprintf(stderr, "Only compressed file are accepted\n");
        return 0;
    }
    swf->header->Signature[0] = 'F';

    assert((output = fopen(argv[2], "w")) != NULL);
    
    assert(fwrite(swf->header->Signature, sizeof(UI8), 3, output) == 3);
    assert(fwrite(&(swf->header->Version), sizeof(UI8), 1, output) == 1);
    assert(fwrite(&(swf->header->FileLength), sizeof(UI32), 1, output) == 1);

    fseek(swf->stream, 8, SEEK_SET);
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    assert(ret == Z_OK);
    
    do
    {
        strm.avail_in = fread(in, 1, CHUNK, swf->stream);
        assert(!ferror(swf->stream));

        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                zerr(ret);
                return 1;
            }

            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, output) != have || ferror(output)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);

    swf_close(swf);
    fclose(output);
    printf("Deflate Success\n");
    
    return 0;
}
