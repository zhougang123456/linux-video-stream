#include "glz-encoder-dict.hpp"
struct GlzEncoderUsrContext {
	
	void* (*malloc)(GlzEncoderUsrContext* usr, int size);
	void (*free)(GlzEncoderUsrContext* usr, void* ptr);

	// get the next chunk of the image which is entered to the dictionary. If the image is down to
	// top, return it from the last line to the first one (stride should always be positive)
    int (*more_lines)(GlzEncoderUsrContext* usr, unsigned char** lines);

    // get the next chunk of the compressed buffer.return number of unsigned chars in the chunk.
    int (*more_space)(GlzEncoderUsrContext* usr, unsigned char** io_ptr);

	// called when an image is removed from the dictionary, due to the window size limit
	void (*free_image)(GlzEncoderUsrContext* usr, GlzUsrImageContext* image);

};

typedef struct rgb32_pixel_t {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char pad;
} rgb32_pixel_t;

typedef struct rgb24_pixel_t {
    unsigned char b;
    unsigned char g;
    unsigned char r;
} rgb24_pixel_t;

#define PIXEL rgb32_pixel_t

/* Holds a specific data for one encoder, and data that is relevant for the current image encoded */
class GlzEncoder 
{
public:
	GlzEncoderUsrContext* usr;
    unsigned char id;
	SharedDictionary* dict;

	struct {
		LzImageType type;
        unsigned int id;
        unsigned int first_win_seg;
	} cur_image;

	struct {
        unsigned char* start;
        unsigned char* now;
        unsigned char* end;
        size_t  bytes_count;
        unsigned char* last_copy;  // pointer to the last unsigned char in which copy count was written
	} io;

	GlzEncoder();
	~GlzEncoder();
    bool init(unsigned char id, SharedDictionary* dictionary, GlzEncoderUsrContext* usr_ctx);
	/*
        assumes width is in pixels and stride is in unsigned chars
		usr_context       : when an image is released from the window due to capacity overflow,
						    usr_context is given as a parameter to the free_image callback.
		o_enc_dict_context: if glz_enc_dictionary_remove_image is called, it should be
						    called with the o_enc_dict_context that is associated with
						    the image.
        return: the number of unsigned chars in the compressed data and sets o_enc_dict_context
        NOTE  : currently supports only rgb images in which width*unsigned chars_per_pixel = stride OR
                palette images in which stride equals the min number of unsigned chars to hold a line.
				The stride should be > 0
	*/
	int glz_encode(LzImageType type, int width, int height,
        int top_down, unsigned char* lines, unsigned int num_lines, int stride,
        unsigned char* io_ptr, unsigned int num_io_bytes, GlzUsrImageContext* usr_context,
		GlzEncDictImageContext** o_enc_dict_context);

private:
    bool encoder_reset(unsigned char* io_ptr, unsigned char* io_ptr_end);
    int more_io_bytes(void);
    void encode(unsigned char byte);
	void encode_32(unsigned int word);
    void encode_64(unsigned long word);
    void encode_copy_count(unsigned char copy_count);
    void update_copy_count(unsigned char copy_count);
	void compress_output_prev(void);
	void glz_rgb32_compress(void);
    void compress_seg(unsigned int seg_idx, PIXEL* from, int copied);
    void encode_match(unsigned int image_distance, size_t pixel_distance, size_t len);
    size_t do_match(SharedDictionary* dict,
					WindowImageSegment* ref_seg, const PIXEL* ref,
					const PIXEL* ref_limit,
					WindowImageSegment* ip_seg, const PIXEL* ip,
					const PIXEL* ip_limit,
                    int pix_per_byte,
                    size_t* o_image_dist, size_t* o_pix_distance);
};


