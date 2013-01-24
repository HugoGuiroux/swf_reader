#include "swf.h"

int main(int argc, char **argv)
{
    Swf* swf = NULL;

    if (argc < 2)
    {
        fprintf(stderr, "Usage : %s <swf_filename>\n", argv[0]);
        return 1;
    }

    swf = swf_open(argv[1]);
    swf_read_header(swf);
    if (swf->header->Signature[0] != 'F')
    {
        fprintf(stderr, "Only uncompressed file are accepted\n");
        return 0;
    }
    swf_print_header(swf);
    swf_print_tag(swf);
    swf_close(swf);
    
    return 0;
}
