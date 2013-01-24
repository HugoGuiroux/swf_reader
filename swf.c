#include "swf.h"
#include <assert.h>
#include <string.h>
#include <ctype.h>

void convert_bit(void* data, UI32 from, UI32 length, void* output)
{
    UI32 i;

    for (i = 0; i < length; i++)
        ((UI8*)output)[i] = (*(UI8*)(data + (from + i)/8) & (1 << (from + i)%8)) >> ((from + i)%8);
}

EncodedU32 GetEncodedU32(unsigned char** pos)
{
    int result = (*pos)[0];
    if (!(result & 0x00000080))
    {
        (*pos)++;
        return result;
    }
    result = (result & 0x0000007f) | (*pos)[1]<<7;
    if (!(result & 0x00004000))
    {
        *pos += 2;
        return result;
    }
    result = (result & 0x00003fff) | (*pos)[2]<<14;
    if (!(result & 0x00200000))
    {
        *pos += 3;
        return result;
    }
    result = (result & 0x001fffff) | (*pos)[3]<<21;
    if (!(result & 0x10000000))
    {
        *pos += 4;
        return result;
    }
    result = (result & 0x0fffffff) | (*pos)[4]<<28;
    *pos += 5;
    return result;
}

FLOAT get_float_from_fixed16(UI16 x)
{
    return ((0xFF00 & x) >> 8) + (0xFF & x) / (0xFF);
}

SI32 get_ui32_from_bytes(SB* bytes, UI32 size)
{
    int i = 0;
    SI32 result = 0;

    for (i  = 0; i < size; i++)
    {
        result = result | (bytes[i] << i);
    }
    printf("\n");

    result = (result << (32 - size)) >> (32 - size);

    return (SI32)result;
}

/* Read rectangle structure */
RECT* swf_read_rect(Swf* swf)
{
    RECT* rect = NULL;
    void* data = NULL;
    UI32 size;

    rect = malloc(sizeof(RECT));
    assert(rect != NULL);
    
    assert(fread(&(rect->NBits), sizeof(UI8), 1, swf->stream) == 1);
    // Lis le nombre de bit
    rect->NBits = (rect->NBits & 0xF8) >> 3;
    fseek(swf->stream, -1, SEEK_CUR);

    rect->Xmin = malloc(sizeof(SB) * rect->NBits);
    rect->Xmax = malloc(sizeof(SB) * rect->NBits);
    rect->Ymin = malloc(sizeof(SB) * rect->NBits);
    rect->Ymax = malloc(sizeof(SB) * rect->NBits);    

    assert(rect->Xmin != NULL &&
           rect->Xmax != NULL &&
           rect->Ymin != NULL &&
           rect->Ymax != NULL);
    
    size = (rect->NBits * 4 + 5) / (sizeof(UI8) * 8);
    if ((rect->NBits * 4 + 5) % (sizeof(UI8) * 8) != 0)
        size++;

    data = malloc(sizeof(SB) * size);

    assert(data != NULL);
    assert(fread(data, size, 1, swf->stream) == 1);

    convert_bit(data, 5 + rect->NBits * 0, rect->NBits, rect->Xmin);
    convert_bit(data, 5 + rect->NBits * 1, rect->NBits, rect->Xmax);
    convert_bit(data, 5 + rect->NBits * 2, rect->NBits, rect->Ymin);
    convert_bit(data, 5 + rect->NBits * 3, rect->NBits, rect->Ymax);
    free(data);    

    return rect;
}

Swf* swf_open(const char* name)
{
    Swf* swf = NULL;
    int i = 0;
    
    assert((swf = malloc(sizeof(Swf_Header))) != NULL);
    assert((swf->stream = fopen(name, "r")) != NULL);
    swf->header = NULL;

    for (i = 0; i < TagMax; i++)
        handlers[i] = handle_none;

    handlers[FileAttributes] = handle_files_attributes;
    handlers[Metadata] = handle_metadata;
    handlers[ScriptLimits] = handle_script_limits;
    handlers[SetBackgroundColor] = handle_set_background_color;
    handlers[SerialNumber] = handle_serial_number;
    handlers[FrameLabel] = handle_frame_label;
    handlers[SymbolClass] = handle_symbol_class;
    handlers[DefineBinaryData] = handle_define_binary_data;
    handlers[DoABC] = handle_do_abc;
    
    return swf;
}

void swf_close(Swf* swf)
{
    free(swf->header->FrameSize->Xmin);
    free(swf->header->FrameSize->Xmax);
    free(swf->header->FrameSize->Ymin);
    free(swf->header->FrameSize->Ymax);
    free(swf->header->FrameSize);
    free(swf->header);
    fclose(swf->stream);
    free(swf);
}

void swf_read_header(Swf* swf)
{
    swf->header = malloc(sizeof(Swf_Header));
    assert(swf->header != NULL);

    fseek(swf->stream, 0, SEEK_SET);
    assert(fread(swf->header->Signature, sizeof(UI8), 3, swf->stream) == 3);
    assert(fread(&(swf->header->Version), sizeof(UI8), 1, swf->stream) == 1);
    assert(fread(&(swf->header->FileLength), sizeof(UI32), 1, swf->stream) == 1);
    swf->header->FrameSize = swf_read_rect(swf);
    assert(fread(&(swf->header->FrameRate), sizeof(UI16), 1, swf->stream) == 1);
    assert(fread(&(swf->header->FrameCount), sizeof(UI16), 1, swf->stream) == 1);
    swf->startFile = ftell(swf->stream);
}

void swf_print_header(Swf* swf)
{
    assert(swf->header != NULL);
    printf("Signature = %c%c%c\n", 
           swf->header->Signature[2], 
           swf->header->Signature[1], 
           swf->header->Signature[0]);
    printf("Version = %u\n", swf->header->Version);
    printf("File Length = %u\n", swf->header->FileLength);
    printf("FrameSize :\n");
    printf("\tNBits = %u\n", swf->header->FrameSize->NBits);
    printf("FrameRate = %f\n", get_float_from_fixed16(swf->header->FrameRate));
    printf("FrameCount = %u\n", swf->header->FrameCount);
}

#define CHECK_TAG(X) {                          \
    if (type == X)                              \
    {                                           \
        printf(# X);                            \
        return 1;                               \
    }                                           \
}

int swf_print_tag_type(UI16 type)
{
    CHECK_TAG(End);
    CHECK_TAG(ShowFrame);
    CHECK_TAG(DefineShape);
    CHECK_TAG(PlaceObject);
    CHECK_TAG(RemoveObject);
    CHECK_TAG(DefineBits);
    CHECK_TAG(DefineButton);
    CHECK_TAG(JPEGTables);
    CHECK_TAG(SetBackgroundColor);
    CHECK_TAG(DefineFont);
    CHECK_TAG(DefineText);
    CHECK_TAG(DoAction);
    CHECK_TAG(DefineFontInfo);
    CHECK_TAG(DefineSound);
    CHECK_TAG(StartSound);
    CHECK_TAG(DefineButtonSound);
    CHECK_TAG(SoundStreamHead);
    CHECK_TAG(SoundStreamBlock);
    CHECK_TAG(DefineBitsLossless);
    CHECK_TAG(DefineBitsJPEG2);
    CHECK_TAG(DefineShape2);
    CHECK_TAG(DefineButtonCxform);
    CHECK_TAG(Protect);
    CHECK_TAG(PlaceObject2);
    CHECK_TAG(RemoveObject2);
    CHECK_TAG(DefineShape3);
    CHECK_TAG(DefineText2);
    CHECK_TAG(DefineButton2);
    CHECK_TAG(DefineBitsJPEG3);
    CHECK_TAG(DefineBitsLossless2);
    CHECK_TAG(DefineEditText);
    CHECK_TAG(DefineSprite);
    CHECK_TAG(SerialNumber);
    CHECK_TAG(FrameLabel);
    CHECK_TAG(SoundStreamHead2);
    CHECK_TAG(DefineMorphShape);
    CHECK_TAG(DefineFont2);
    CHECK_TAG(ExportAssets);
    CHECK_TAG(ImportAssets);
    CHECK_TAG(EnableDebugger);
    CHECK_TAG(DoInitAction);
    CHECK_TAG(DefineVideoStream);
    CHECK_TAG(VideoFrame);
    CHECK_TAG(DefineFontInfo2);
    CHECK_TAG(EnableDebugger2);
    CHECK_TAG(ScriptLimits);
    CHECK_TAG(SetTabIndex);
    CHECK_TAG(FileAttributes);
    CHECK_TAG(PlaceObject3);
    CHECK_TAG(ImportAssets2);
    CHECK_TAG(DefineFontAlignZones);
    CHECK_TAG(CSMTextSettings);
    CHECK_TAG(DefineFont3);
    CHECK_TAG(SymbolClass);
    CHECK_TAG(Metadata);
    CHECK_TAG(DefineScalingGrid);
    CHECK_TAG(DoABC);
    CHECK_TAG(DefineShape4);
    CHECK_TAG(DefineMorphShape2);
    CHECK_TAG(DefineSceneAndFrameLabelData);
    CHECK_TAG(DefineBinaryData);
    CHECK_TAG(DefineFontName);
    CHECK_TAG(StartSound2);
    CHECK_TAG(DefineBitsJPEG4);
    CHECK_TAG(DefineFont4);

    return 0;
}


void hex_dump(size_t size, void* data)
{
    UI32 i = 0;
    UI32 j = 0;
    UI32 k = 0;
    
    for (i = 0; i < size;)
    {
        printf("  0x%08x ", i);
        j = i;
        k = 1;
        while (j < i + sizeof(UI32) * 4)
        {
            if (j < size)
            {
                
                printf("%02x", *(UI8*)(data + j));
            }
            else
            {
                printf("  ");
            }

            if (k != 1 && k%4 == 0)
                printf(" ");
            j++;
            k++;
        }

        j = i;
        while (j < i + sizeof(UI32) * 4)
        {
            if (isprint(*(char*)(data + j)))
                printf("%c", *(char*)(data + j));
            else
                printf(".");
            j++;
        }

        i += sizeof(int) * 4;
        printf("\n");
    }

}

void swf_print_tag(Swf* swf)
{
    RECORDHEADER tag;
    size_t resultFread = 0;
    void *data = NULL;

    fseek(swf->stream, swf->startFile, SEEK_SET);
    printf("StartFile = %d\n", swf->startFile);
    
    resultFread = fread(&(tag.TagCodeAndLength), sizeof(UI16), 1, swf->stream);
    while (resultFread == 1)
    {
        if (RECORDHEADER_TAG_LENGTH(tag.TagCodeAndLength) == RECORDHEADER_LENGTH_FULL)
            assert(fread(&(tag.Length), sizeof(SI32), 1, swf->stream) == 1);
        else
            tag.Length = RECORDHEADER_TAG_LENGTH(tag.TagCodeAndLength);
        
        printf("========================================================\n");
        printf("Tag ");
        if (swf_print_tag_type(RECORDHEADER_TAG_TYPE(tag.TagCodeAndLength)) == 0)
        {
            printf("Unknown %u\t Length %d\n", RECORDHEADER_TAG_TYPE(tag.TagCodeAndLength), tag.Length);
            assert((data = malloc(tag.Length)) != NULL);
            if (tag.Length != 0)
                assert(fread(data, tag.Length, 1, swf->stream) == 1);
            
            hex_dump((size_t)tag.Length, data);
            free(data);
        }
        else
        {
            printf(" (%u)\t Length %d\n", RECORDHEADER_TAG_TYPE(tag.TagCodeAndLength), tag.Length);
            assert((data = malloc(tag.Length)) != NULL);
            if (tag.Length != 0)
                assert(fread(data, tag.Length, 1, swf->stream) == 1);
        
            printf("--------------------------------------------------------\n");
            handlers[RECORDHEADER_TAG_TYPE(tag.TagCodeAndLength)](swf, data, tag.Length);
            printf("========================================================\n");
        
            free(data);
        }
        
        resultFread = fread(&(tag.TagCodeAndLength), sizeof(UI16), 1, swf->stream);
    }
}

void swf_extract_binary_data(Swf* swf, FILE* output)
{
    RECORDHEADER tag;
    size_t resultFread = 0;
    void *data = NULL;

    fseek(swf->stream, swf->startFile, SEEK_SET);
    
    resultFread = fread(&(tag.TagCodeAndLength), sizeof(UI16), 1, swf->stream);
    while (resultFread == 1)
    {
        if (RECORDHEADER_TAG_LENGTH(tag.TagCodeAndLength) == RECORDHEADER_LENGTH_FULL)
            assert(fread(&(tag.Length), sizeof(SI32), 1, swf->stream) == 1);
        else
            tag.Length = RECORDHEADER_TAG_LENGTH(tag.TagCodeAndLength);

        assert((data = malloc(tag.Length)) != NULL);
        if (tag.Length != 0)
        {
            assert(fread(data, tag.Length, 1, swf->stream) == 1);
        
            if (RECORDHEADER_TAG_TYPE(tag.TagCodeAndLength) == DefineBinaryData)
            {
                assert(fwrite(data + (sizeof(UI32) + sizeof(UI16)), tag.Length - (sizeof(UI32) + sizeof(UI16)), 1, output) == 1);
                free(data);
                return;
            }        
        }
        free(data);
        resultFread = fread(&(tag.TagCodeAndLength), sizeof(UI16), 1, swf->stream);
    }
}

void handle_files_attributes(Swf* swf, void* data, UI16 size)
{
    UI32 flags = *(UI32*)data;

    if(flags & 0x1) printf(" usenetwork");
    if(flags & 0x8) printf(" as3");
    if(flags & 0x10) printf(" metadata");
    if(flags & 0x20) printf(" hardware-gpu");
    if(flags & 0x40) printf(" accelerated-blit");
    printf("\n");
}

void handle_metadata(Swf* swf, void* data, UI16 size)
{
    printf("%s\n", (char*)data);
}

void handle_script_limits(Swf* swf, void* data, UI16 size)
{
    printf("MaxRecursionDepth = %u\n", ((UI16*)data)[0]);
    printf("ScriptTimeoutSeconds = %u\n", ((UI16*)data)[1]);
}

void handle_set_background_color(Swf* swf, void* data, UI16 size)
{
    RGB color = *(RGB*)data;

    printf("RGB 0x%X%X%X\n", color.Red, color.Green, color.Blue);
}

void handle_serial_number(Swf* swf, void* data, UI16 size)
{
    printf("Hexdump : \n");
    hex_dump(size, data);
}
    

void handle_frame_label(Swf* swf, void* data, UI16 size)
{
    printf("%s\n", (char*)data);
}

void handle_symbol_class(Swf* swf, void* data, UI16 size)
{
    UI16 NumSymbols = *(UI16*)data;
    UI16 i;
    UI32 pos = sizeof(UI16);
    char* string = NULL;
    
    for (i = 0; i < NumSymbols; i++)
    {
        printf("Tag = %u\t", *(UI16*)(data + pos));
        pos += sizeof(UI16);
        string = (char*)(data + pos);
        pos += strlen(string) + 1;
        printf("Value = '%s'\n", string);
    }
}

void handle_define_binary_data(Swf* swf, void* data, UI16 size)
{
    printf("Tag = %u\n", *(UI16*)data);

    printf("Hexdump : \n");    
    hex_dump(size - (sizeof(UI16) + sizeof(UI32)), data + sizeof(UI16) + sizeof(UI32));
}

void handle_do_abc(Swf* swf, void* data, UI16 size)
{
    char* string = NULL;
    UI32 pos = 0;

    printf("Flags = %X\n", *(UI32*)data);
    pos += sizeof(UI32);

    string = (char*)(data + pos);
    pos += strlen(string) + 1;
    printf("Name = %s\n", string); 

    printf("Hexdump : \n");    
    hex_dump(size - pos, data + pos);
}

void handle_none(Swf* swf, void* data, UI16 size)
{
    swf = swf; 
    data = data; 
    size = size;
}
