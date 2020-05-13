#include "bitmap.hpp"

enum {
	DRAW_NOP,
	DRAW_COPY,
	COPY_BITS,
};

typedef enum ImageType {

	IMAGE_TYPE_BITMAP,
	IMAGE_TYPE_QUIC,
	IMAGE_TYPE_RESERVED,
	IMAGE_TYPE_LZ_PLT = 100,
	IMAGE_TYPE_LZ_RGB,
	IMAGE_TYPE_GLZ_RGB,
	IMAGE_TYPE_FROM_CACHE,
	IMAGE_TYPE_SURFACE,
	IMAGE_TYPE_JPEG,
	IMAGE_TYPE_FROM_CACHE_LOSSLESS,
	IMAGE_TYPE_ZLIB_GLZ_RGB,
	IMAGE_TYPE_JPEG_ALPHA,
	IMAGE_TYPE_LZ4,

	SPICE_IMAGE_TYPE_ENUM_END
} ImageType;

enum {
	CHUNKS_FLAGS_UNSTABLE = (1 << 0),
	CHUNKS_FLAGS_FREE = (1 << 1)
};

typedef enum ImageScaleMode {
	IMAGE_SCALE_MODE_INTERPOLATE,
	IMAGE_SCALE_MODE_NEAREST,

	IMAGE_SCALE_MODE_ENUM_END
} ImageScaleMode;

typedef enum Ropd {
	ROPD_INVERS_SRC = (1 << 0),
	ROPD_INVERS_BRUSH = (1 << 1),
	ROPD_INVERS_DEST = (1 << 2),
	ROPD_OP_PUT = (1 << 3),
	ROPD_OP_OR = (1 << 4),
	ROPD_OP_AND = (1 << 5),
	ROPD_OP_XOR = (1 << 6),
	ROPD_OP_BLACKNESS = (1 << 7),
	ROPD_OP_WHITENESS = (1 << 8),
	ROPD_OP_INVERS = (1 << 9),
	ROPD_INVERS_RES = (1 << 10),

	ROPD_MASK = 0x7ff

} Ropd;
typedef enum EffectType
{
	EFFECT_BLEND = 0,
	EFFECT_OPAQUE = 1,
	EFFECT_REVERT_ON_DUP = 2,
	EFFECT_BLACKNESS_ON_DUP = 3,
	EFFECT_WHITENESS_ON_DUP = 4,
	EFFECT_NOP_ON_DUP = 5,
	EFFECT_NOP = 6,
	EFFECT_OPAQUE_BRUSH = 7
} EffectType;

typedef struct SpiceChunk {
    unsigned char* data;
    unsigned int len;
} SpiceChunk;
typedef struct SpiceChunks {
    unsigned int     data_size;
    unsigned int     num_chunks;
    unsigned int     flags;
	SpiceChunk*   chunk;
} SpiceChunks;

typedef struct SpicePoint {
    int x;
    int y;
} SpicePoint;

typedef struct SpiceRect {
    int left;
    int top;
    int right;
    int bottom;
} SpiceRect;

typedef struct SpiceImageDescriptor {
    unsigned long id;
    unsigned char type;
    unsigned char flags;
    unsigned int width;
    unsigned int height;
} SpiceImageDescriptor;

typedef struct SpicePalette {
    unsigned long unique;
    unsigned short num_ents;
    unsigned int* ents;
} SpicePalette;

typedef struct SpiceBitmap {
    unsigned char format;
    unsigned char flags;
    unsigned int x;
    unsigned int y;
    unsigned int stride;
	SpicePalette* palette;
    unsigned long palette_id;
	SpiceChunks* data;
} SpiceBitmap;

typedef struct SpiceZlibGlzRGBData {
    unsigned int glz_data_size;
    unsigned int data_size;
	SpiceChunks* data;
} SpiceZlibGlzRGBData;

typedef struct SpiceQUICData {
    unsigned int data_size;
	SpiceChunks* data;
} SpiceLZRGBData, SpiceJPEGData;

typedef struct SpiceImage {
	SpiceImageDescriptor descriptor;
	union {
		SpiceBitmap         bitmap;
		SpiceLZRGBData      lz_rgb;
		SpiceZlibGlzRGBData zlib_glz;
		SpiceJPEGData       jpeg;
	} u;
} SpiceImage;


typedef struct SpiceQMask {
    unsigned char flags;
	SpicePoint pos;
	SpiceImage* bitmap;
} SpiceQMask;


typedef struct SpiceCopy {
	SpiceImage* src_bitmap;
	SpiceRect src_area;
    unsigned short rop_descriptor;
    unsigned char scale_mode;
	SpiceQMask mask;
} SpiceCopy;

typedef struct RedDrawable {
	int refs;
    unsigned int surface_id;
    unsigned char effect;
    unsigned char type;
	SpiceRect bbox;
    unsigned int mm_time;
	union {
		SpiceCopy copy;
		struct {
			SpicePoint src_pos;
		} copy_bits;
	} u;
	//GlzImageRetention glz_retention;
} RedDrawable;

void chunks_destroy(SpiceChunks* chunks);

SpiceChunks* chunks_new_linear(unsigned char* data, unsigned int len);

static inline RedDrawable* red_drawable_ref(RedDrawable* drawable)
{
	drawable->refs++;
	return drawable;
}

void red_drawable_unref(RedDrawable* red_drawable);

void red_drawable_get(RedDrawable* red_drawable, SpiceRect* rect, unsigned char* data, unsigned int time);
