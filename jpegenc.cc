#include <stdio.h>
#include <jpeglib.h>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <memory>

#define YV12_U(uaddr, width, x, y) (uaddr[(y)/2*(width)/2 + (x)/2])
#define YV12_V(vaddr, width, x, y) (vaddr[(y)/2*(width)/2 + (x)/2])
#define NV12_U(uvaddr, width, x, y) (uvaddr[(y)/2*(width) + (x)/2*2])
#define NV12_V(uvaddr, width, x, y) (uvaddr[(y)/2*(width) + (x)/2*2+1])

void WriteJpegFile(char *filename, int quality, 
		uint8_t *data, int image_width, int image_height, bool is_nv12) {
	uint8_t *src_y = data;
	uint8_t *src_u = data + image_width * image_height;
	uint8_t *src_v = is_nv12 ? src_u : (data + image_width * image_height * 5 / 4);
	int src_ystride = image_width;
	int src_ustride = image_width / 2;
	int src_vstride = image_width / 2;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *outfile;			 /* target file */
	JSAMPROW row_pointer[2]; /* pointer to JSAMPLE row[s] */

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	if ((outfile = fopen(filename, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = image_width; /* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_YCbCr; /* colorspace of input image */
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	jpeg_start_compress(&cinfo, TRUE);

	std::vector<uint8_t> row_buf(image_width * 3 * 2); // two line yuv888
	row_pointer[0] = row_buf.data();
	row_pointer[1] = row_buf.data() + image_width * 3;
	while (cinfo.next_scanline < cinfo.image_height) {
		if (is_nv12) {
			for (int i = 0; i < cinfo.image_width; i+=2) {
				int line = cinfo.next_scanline;
				int dst0_offset = i * 3;
				int dst1_offset = (i + 1) * 3;
				row_pointer[0][dst0_offset] = src_y[line * src_ystride + i];  // y00
				row_pointer[0][dst0_offset + 1] = NV12_U(src_u, image_width, i, line); // u00
				row_pointer[0][dst0_offset + 2] = NV12_V(src_v, image_width, i, line); // v00
				row_pointer[0][dst1_offset] = src_y[line * src_ystride + i + 1]; // y01
				row_pointer[0][dst1_offset + 1] = NV12_U(src_u, image_width, i+1, line); // u01
				row_pointer[0][dst1_offset + 2] = NV12_V(src_v, image_width, i+1, line); // v01
				row_pointer[1][dst0_offset] = src_y[(line + 1) * src_ystride + i];  // y10
				row_pointer[1][dst0_offset + 1] = NV12_U(src_u, image_width, i, line+1); // u10
				row_pointer[1][dst0_offset + 2] = NV12_V(src_v, image_width, i, line+1); // v10
				row_pointer[1][dst1_offset] = src_y[(line + 1) * src_ystride + i + 1]; // y11
				row_pointer[1][dst1_offset + 1] = NV12_U(src_u, image_width, i+1, line+1); // u11
				row_pointer[1][dst1_offset + 2] = NV12_V(src_v, image_width, i+1, line+1); // v11
			}
		}
		else {
			for (int i = 0; i < cinfo.image_width; i+=2) {
				int line = cinfo.next_scanline;
				int dst0_offset = i * 3;
				int dst1_offset = (i + 1) * 3;
				row_pointer[0][dst0_offset] = src_y[line * src_ystride + i];  // y00
				row_pointer[0][dst0_offset + 1] = YV12_U(src_u, image_width, i, line); // u00
				row_pointer[0][dst0_offset + 2] = YV12_V(src_v, image_width, i, line); // v00
				row_pointer[0][dst1_offset] = src_y[line * src_ystride + i + 1]; // y01
				row_pointer[0][dst1_offset + 1] = YV12_U(src_u, image_width, i+1, line); // u01
				row_pointer[0][dst1_offset + 2] = YV12_V(src_v, image_width, i+1, line); // v01
				row_pointer[1][dst0_offset] = src_y[(line + 1) * src_ystride + i];  // y10
				row_pointer[1][dst0_offset + 1] = YV12_U(src_u, image_width, i, line+1); // u10
				row_pointer[1][dst0_offset + 2] = YV12_V(src_v, image_width, i, line+1); // v10
				row_pointer[1][dst1_offset] = src_y[(line + 1) * src_ystride + i + 1]; // y11
				row_pointer[1][dst1_offset + 1] = YV12_U(src_u, image_width, i+1, line+1); // u11
				row_pointer[1][dst1_offset + 2] = YV12_V(src_v, image_width, i+1, line+1); // v11
			}
		}
		jpeg_write_scanlines(&cinfo, row_pointer, 2);
	}

	jpeg_finish_compress(&cinfo);
	/* After finish_compress, we can close the output file. */
	fclose(outfile);

	jpeg_destroy_compress(&cinfo);
}

int main(int argc, char **argv) {
	if (argc != 7) {
		fprintf(stderr, "usage: %s <yuv_file> <yv12/nv12> <width> <height> <jpeg_file> <quality>\n", argv[0]);
		return -1;
	}
	char *yuv_file = argv[1];
	int width = std::atoi(argv[3]);
	int height = std::atoi(argv[4]);
	char *jpeg_file = argv[5];
	int quality = std::atoi(argv[6]);
	bool is_nv12 = !strcmp(argv[2], "nv12");

	FILE *fp = fopen(yuv_file, "rb");
	if (fp == nullptr) {
		fprintf(stderr, "can't open %s\n", yuv_file);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	std::size_t file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	std::unique_ptr<uint8_t[]> file_buf(new uint8_t[file_size]);	
	fread(file_buf.get(), 1, file_size, fp);
	fclose(fp);

	if (file_size != width * height * 3 / 2) {
		fprintf(stderr, "expect file size: %d, actual: %d\n", width * height * 3, (int)file_size);
		return -1;
	}
	WriteJpegFile(jpeg_file, quality, file_buf.get(), width, height, is_nv12);
	return 0;
}