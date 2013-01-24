#ifndef __SWF_H__
#define __SWF_H__

/* Standard SWF types. */
#include <stdint.h>
#include <iconv.h>
#include <stdlib.h>
#include <stdio.h>

#define reverse_2(x) ((((x)&0xFF)<<8)|(((x)>>8)&0xFF))
#define reverse_4(x) ((((x)&0xFF)<<24)|((((x)>>8)&0xFF)<<16)|	\
		      ((((x)>>16)&0xFF)<<8)|(((x)>>24)&0xFF))

/* Type for 8-bit quantity. */
typedef int8_t SI8;
typedef uint8_t UI8;

/* Type for 16-bit quantity. */
typedef int16_t SI16;
typedef uint16_t UI16;

/* Type for 32-bit quantity. */
typedef int32_t SI32;
typedef uint32_t UI32;

/* Type for 64-bit quantity. */
typedef int64_t SI64;
typedef uint64_t UI64;

/* Type for Fixed-Point numbers. */
typedef uint32_t FIXED;
typedef uint16_t FIXED8;

/* Type for Floating-point numbers. */
typedef uint16_t FLOAT16;
typedef float FLOAT;
typedef double DOUBLE;

/* Type for Encoded integers. */
typedef uint32_t EncodedU32;

/* Type for Bit values. */
typedef SI8 SB;
typedef UI8 UB;
typedef UI8 FB;

/* Type for strings. */

typedef UI8* String;
#define StringEnd '\0'

typedef enum
{
    NO_LANGUAGE = 0,
    LATIN,
    JAPANESE,
    KOREAN,
    SIMPLFIED_CHINESE,
    TRADITIONAL_CHINESE
} LanguageCode;

/* Type for color record. */
typedef struct
{
    UI8 Red;
    UI8 Green;
    UI8 Blue;
} RGB;

typedef struct
{
    UI8 Red;
    UI8 Green;
    UI8 Blue;
    UI8 Alpha;
} RGBA;

typedef struct
{
    UI8 Alpha;
    UI8 Red;
    UI8 Green;
    UI8 Blue;
} ARGB;


/* Type for Rectangle record. */

typedef struct
{
    UI32 NBits;
    SB* Xmin;
    SB* Xmax;
    SB* Ymin;
    SB* Ymax;
} RECT;


/* Color transformat functions */
// TODO : make color transform data


/* File structure */
typedef struct
{
    UI8 Signature[3];
    UI8 Version;
    UI32 FileLength;
    RECT* FrameSize;
    UI16 FrameRate;
    UI16 FrameCount;
} Swf_Header;

typedef struct
{
    FILE* stream;
    Swf_Header* header;
    UI32 startFile;
} Swf;

/* Tag structure */
typedef struct
{
    UI16 TagCodeAndLength;
    SI32 Length;
} RECORDHEADER;
#define RECORDHEADER_TAG_TYPE(X) (X >> 6)
#define RECORDHEADER_TAG_LENGTH(X) (X & 0x3F)

#define RECORDHEADER_LENGTH_FULL 0x3F

#define End 0
#define ShowFrame 1
#define DefineShape 2
#define PlaceObject 4
#define RemoveObject 5
#define DefineBits 6
#define DefineButton 7
#define JPEGTables 8
#define SetBackgroundColor 9
#define DefineFont 10
#define DefineText 11
#define DoAction 12
#define DefineFontInfo 13
#define DefineSound 14
#define StartSound 15
#define DefineButtonSound 17
#define SoundStreamHead 18
#define SoundStreamBlock 19
#define DefineBitsLossless 20
#define DefineBitsJPEG2 21
#define DefineShape2 22
#define DefineButtonCxform 23
#define Protect 24
#define PlaceObject2 26
#define RemoveObject2 28
#define DefineShape3 32
#define DefineText2 33
#define DefineButton2 34
#define DefineBitsJPEG3 35
#define DefineBitsLossless2 36
#define DefineEditText 37
#define DefineSprite 39
#define SerialNumber 41
#define FrameLabel 43
#define SoundStreamHead2 45
#define DefineMorphShape 46
#define DefineFont2 48
#define ExportAssets 56
#define ImportAssets 57
#define EnableDebugger 58
#define DoInitAction 59
#define DefineVideoStream 60
#define VideoFrame 61
#define DefineFontInfo2 62
#define EnableDebugger2 64
#define ScriptLimits 65
#define SetTabIndex 66
#define FileAttributes 69
#define PlaceObject3 70
#define ImportAssets2 71
#define DefineFontAlignZones 73
#define CSMTextSettings 74
#define DefineFont3 75
#define SymbolClass 76
#define Metadata 77
#define DefineScalingGrid 78
#define DoABC 82
#define DefineShape4 83
#define DefineMorphShape2 84
#define DefineSceneAndFrameLabelData 86
#define DefineBinaryData 87
#define DefineFontName 88
#define StartSound2 89
#define DefineBitsJPEG4 90
#define DefineFont4 91
#define TagMax (DefineFont4 + 1)




Swf* swf_open(const char* name);
void swf_close(Swf* swf);
void swf_read_header(Swf* swf);
void swf_print_header(Swf* swf);
void swf_print_tag(Swf* swf);
void swf_extract_binary_data(Swf* swf, FILE* output);

typedef void (*handle_fct)(Swf*, void*, UI16);
handle_fct handlers[TagMax];

void handle_none(Swf* swf, void* data, UI16 size);
void handle_files_attributes(Swf* swf, void* data, UI16 size);
void handle_metadata(Swf* swf, void* data, UI16 size);
void handle_script_limits(Swf* swf, void* data, UI16 size);
void handle_set_background_color(Swf* swf, void* data, UI16 size);
void handle_serial_number(Swf* swf, void* data, UI16 size);
void handle_frame_label(Swf* swf, void* data, UI16 size);
void handle_symbol_class(Swf* swf, void* data, UI16 size);
void handle_define_binary_data(Swf* swf, void* data, UI16 size);
void handle_do_abc(Swf* swf, void* data, UI16 size);

#endif // __SWF_H__
