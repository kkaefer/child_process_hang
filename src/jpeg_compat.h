#include <jpeglib.h>
#include <jerror.h>

void init_mem_source(j_decompress_ptr cinfo) {
    // noop
}

int fill_mem_input_buffer (j_decompress_ptr cinfo) {
    static JOCTET mybuffer[4];

    /* The whole JPEG data is expected to reside in the supplied memory
     * buffer, so any request for more data beyond the given buffer size
     * is treated as an error.
     */
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
    mybuffer[0] = (JOCTET) 0xFF;
    mybuffer[1] = (JOCTET) JPEG_EOI;

    cinfo->src->next_input_byte = mybuffer;
    cinfo->src->bytes_in_buffer = 2;

    return TRUE;
}

/*
 * Prepare for input from a supplied memory buffer.
 * The buffer must contain the whole JPEG data.
 */

void jpeg_mem_src(j_decompress_ptr cinfo, unsigned char* inbuffer, unsigned long insize) {
    struct jpeg_source_mgr * src;

    if (inbuffer == NULL || insize == 0)    /* Treat empty input as fatal error */
      ERREXIT(cinfo, JERR_INPUT_EMPTY);

    /* The source object is made permanent so that a series of JPEG images
     * can be read from the same buffer by calling jpeg_mem_src only before
     * the first one.
     */
    if (cinfo->src == NULL) {    /* first time for this JPEG object? */
      cinfo->src = (struct jpeg_source_mgr *)
        (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                SIZEOF(struct jpeg_source_mgr));
    }

    src = cinfo->src;
    src->init_source = init_mem_source;
    src->fill_input_buffer = fill_mem_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->term_source = term_source;
    src->bytes_in_buffer = (size_t) insize;
    src->next_input_byte = (JOCTET *) inbuffer;
}
