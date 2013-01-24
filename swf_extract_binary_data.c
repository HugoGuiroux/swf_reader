#include "swf.h"
#include <assert.h>

int main(int argc, char **argv)
{
    Swf* swf = NULL;
    FILE* output = NULL;

    if (argc < 3)
    {
        fprintf(stderr, "Usage : %s <swf_filename> <out_filename>\n", argv[0]);
        return 1;
    }

    swf = swf_open(argv[1]);
    swf_read_header(swf);
    if (swf->header->Signature[0] != 'F')
    {
        fprintf(stderr, "Only uncompressed file are accepted\n");
        return 0;
    }

    assert((output = fopen(argv[2], "w")) != NULL);

    swf_extract_binary_data(swf, output);
    swf_close(swf);
    fclose(output);

    printf("Extract success !\n");
    
    return 0;
}
