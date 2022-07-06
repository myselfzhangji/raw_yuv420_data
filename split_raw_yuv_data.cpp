/*
 * This project is to generate a dst raw data based on a given src raw data.
 *
 * Note:
 * (1) Both src raw data and dst raw data are 16 bits per perxel.
 * Width is in pixel unit; Pitch is in byte unit.
 * If RAW is 1920x1080p@16bits, Width = 1920, Pitch = 1920 x 2
 *
 * History:
 *	2020/11/01 - [Ji Zhang] created file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <windows.h>
using namespace std;

typedef unsigned int       u32; /*!< UNSIGNED 32-bit data type */
typedef unsigned char      u8;  /*!< UNSIGNED 8-bit data type */

#define FILE_PATH      ".\\"
#define INPUT_NAME      "custom_aisp_c2y_extraw_1920x1088_3840.raw"
#define INPUT_YUV_NAME  "yuv_1920x1088_YV12_with_pm.yuv"
#define INPUT_FILE_LENGTH 120
#define OUTPUT_FILE_LENGTH 120
#define OUTPUT_FILE_NAME_LENGTH 120
#define SRC_RAW_WIDTH   1920
#define SRC_RAW_HEIGHT  1088
#define SRC_RAW_SIZE    (SRC_RAW_WIDTH * 2 * SRC_RAW_HEIGHT)
#define DST_RAW_WIDTH   1920
#define DST_RAW_HEIGHT  (1088 / 2)
#define DST_RAW_SIZE    (DST_RAW_WIDTH * 2 * DST_RAW_HEIGHT)  // dst raw = src raw interlace (odd line or even line)

#define SRC_YUV_WIDTH   1920
#define SRC_YUV_HEIGHT  1088
#define SRC_YUV_SIZE    (SRC_YUV_WIDTH * SRC_YUV_HEIGHT * 3 >> 1)
#define SRC_YUV_Y_SIZE  (SRC_YUV_WIDTH * SRC_YUV_HEIGHT)
#define SRC_YUV_UV_SIZE (SRC_YUV_WIDTH * SRC_YUV_HEIGHT >> 1)
#define PM_FILE_WIDTH   256
#define PM_FILE_HEIGHT  SRC_YUV_HEIGHT
#define PM_SIZE         (PM_FILE_WIDTH * PM_FILE_HEIGHT)

#define PM_START_X 0
#define PM_START_Y 0
#define PM_RECT_WIDTH   544
#define PM_RECT_HEIGHT  512

/*! @struct iav_rect
 *  @brief IAV rectangle
 */
struct pm_rect {
	u32 x; //!< Offset X
	u32 y; //!< Offset Y
	u32 width; //!< Width
	u32 height; //!< Height
};

static int read_src_yuv_data(u8* src_yuv_data)
{
	FILE* fp = NULL;
	char* input_file = NULL;
	int ret = 0, i = 0, height = 0;

	input_file = (char*)malloc(INPUT_FILE_LENGTH);
	if (input_file == NULL) {
		printf("malloc error!\n");
		return -1;
	}
	memset(input_file, 0, INPUT_FILE_LENGTH);

	strcpy_s(input_file, strlen(FILE_PATH) + 1, FILE_PATH);
	strcat_s(input_file, strlen(FILE_PATH) + strlen(INPUT_YUV_NAME) + 1, INPUT_YUV_NAME);
	printf("input yuv file : %s\n", input_file);

	fopen_s(&fp, input_file, "rb");
	if (fp == NULL) {
		printf("File is not exist, %s!\n", input_file);
		return -1;
	}

#if 0
	// read luma first line by line, YUV420
	for (i = 0; i < SRC_YUV_HEIGHT; ++i) {
		fread(&src_yuv_data[i * SRC_YUV_WIDTH], SRC_YUV_WIDTH, 1, fp);
	}

	// read chroma first line by line, YUV420
	for (i = 0; i < SRC_YUV_HEIGHT / 2; ++i) {
		height = SRC_YUV_HEIGHT + i;
		fread(&src_yuv_data[height * SRC_YUV_WIDTH], SRC_YUV_WIDTH, 1, fp);
	}
#else
	fread(src_yuv_data, SRC_YUV_SIZE, 1, fp);
#endif

	if (fp != NULL) {
		fclose(fp);
	}
	if (input_file != NULL) {
		free(input_file);
		input_file = NULL;
	}

	printf("YUV data first 8 bytes:\n");
	for (i = 0; i < 8; i++) {
		if (i % 8 == 0) printf("\n");
		printf("\t0x%d", src_yuv_data[i]);
	}
	//src_yuv_data[0] = 0x71;
	printf("\n");

	return 0;
}

/*
 * generate dst yuv data for seperated Y data, and UV data
 */
static int generate_dst_yuv_data(u8* src_yuv_data, u8* dst_yuv_y_data, u8* dst_yuv_uv_data)
{
	u32 i;

	memset(dst_yuv_y_data, 0, SRC_YUV_Y_SIZE);
	memset(dst_yuv_uv_data, 0, SRC_YUV_UV_SIZE);

	// gen y data
	for (i = 0; i < SRC_YUV_Y_SIZE; ++i) {
		dst_yuv_y_data[i] = src_yuv_data[i];
	}

	// gen UV data
	for (i = 0; i < SRC_YUV_UV_SIZE; ++i) {
		dst_yuv_uv_data[i] = src_yuv_data[i + SRC_YUV_Y_SIZE];
	}

	return 0;
}

static int read_src_raw_data(u8 *src_raw_data)
{
	FILE* fp = NULL;
	char* input_file = NULL;
	int ret = 0, i;

	input_file = (char*)malloc(INPUT_FILE_LENGTH);
	if (input_file == NULL) {
		printf("malloc error!\n");
		return -1;
	}
	memset(input_file, 0, INPUT_FILE_LENGTH);

	strcpy_s(input_file, strlen(FILE_PATH) + 1, FILE_PATH);
	strcat_s(input_file, strlen(FILE_PATH) + strlen(INPUT_NAME) + 1, INPUT_NAME);
	printf("prepare the input and output path\n");
	printf("input file : %s\n", input_file);

	fopen_s(&fp, input_file, "rb");
	if (fp == NULL) {
		printf("File is not exist, %s!\n", input_file);
		return -1;
	}

	ret = fread(src_raw_data, SRC_RAW_SIZE, 1, fp);
	if (ret < 0) {
		printf("Fail to fread data!\n");
		return -1;
	}

	if (fp != NULL) {
		fclose(fp);
	}
	if (input_file != NULL) {
		free(input_file);
		input_file = NULL;
	}

	for (i = 0; i < 8; i++) {
		printf("0x%02X ", src_raw_data[i]);
	}
	printf("\n");

	return 0;
}

/*
 * put src raw data to the top left corner of dst raw
 */
static int generate_dst_raw_data(u8 *src_raw_data, u8 *dst_raw_data)
{
	u32 i, j;
	int src_idx, dst_idx;
	u32 src_width_in_byte, dst_width_in_byte;

	/* RAW is 16 bits mode, so each line has WIDTH(unit: pixel) x 2 bytes */
	src_width_in_byte = SRC_RAW_WIDTH * 2;
	dst_width_in_byte = DST_RAW_WIDTH * 2;

	memset(dst_raw_data, 0, DST_RAW_SIZE);

	// copy odd line from src to dst
	for (i = 0; i < DST_RAW_HEIGHT; i++) {
		for (j = 0; j < src_width_in_byte; ++j) {
			dst_idx = i * dst_width_in_byte + j;
			src_idx = (2 * i + 1) * src_width_in_byte + j;
			dst_raw_data[dst_idx] = src_raw_data[src_idx];
		}
	}

	return 0;
}

static int write_dst_yuv_data_to_file(u8* dst_yuv_data, u8 type)
{
	FILE* fp = NULL;
	char* out_file = NULL;
	char* out_file_name = NULL;
	size_t size = type ? SRC_YUV_Y_SIZE : SRC_YUV_UV_SIZE;
	char name[32] = {0};

	out_file = (char*)malloc(OUTPUT_FILE_LENGTH);
	if (out_file == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	out_file_name = (char*)malloc(OUTPUT_FILE_NAME_LENGTH);
	if (out_file_name == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	switch (type) {
	case 0:
		size = SRC_YUV_UV_SIZE;
		strncpy_s(name, "uv_1920x544", 32);
		break;
	case 1:
		size = SRC_YUV_Y_SIZE;
		strncpy_s(name, "y_1920x1088", 32);
		break;
	default:
		return -1;
	}

	snprintf(out_file_name, OUTPUT_FILE_NAME_LENGTH, "src_yuv_%s.bin", name);

	strcpy_s(out_file, strlen(FILE_PATH) + 1, FILE_PATH);
	strcat_s(out_file, strlen(FILE_PATH) + strlen(out_file_name) + 1, out_file_name);

	fopen_s(&fp, out_file, "wb");
	if (fp == NULL) {
		printf("File is not exist, %s!\n", out_file);
		return -1;
	}

	fwrite(dst_yuv_data, 1, size, fp);
	printf("output yuv_y file name: %s\n", out_file);

	if (fp != NULL) {
		fclose(fp);
	}

	if (out_file != NULL) {
		free(out_file);
		out_file = NULL;
	}

	return 0;
}

static int write_src_raw_data_to_file(char* src_raw_data)
{
	FILE* fp = NULL;
	char* out_file = NULL;
	char* out_file_name = NULL;

	out_file = (char*)malloc(OUTPUT_FILE_LENGTH);
	if (out_file == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	out_file_name = (char*)malloc(OUTPUT_FILE_NAME_LENGTH);
	if (out_file_name == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	snprintf(out_file_name, OUTPUT_FILE_NAME_LENGTH, "src_cp_raw_%dx%d.raw", SRC_RAW_WIDTH, SRC_RAW_HEIGHT);

	strcpy_s(out_file, strlen(FILE_PATH) + 1, FILE_PATH);
	strcat_s(out_file, strlen(FILE_PATH) + strlen(out_file_name) + 1, out_file_name);

	fopen_s(&fp, out_file, "wb");
	if (fp == NULL) {
		printf("File is not exist, %s!\n", out_file);
		return -1;
	}

	fwrite(src_raw_data, 1, SRC_RAW_SIZE, fp);
	printf("output file name: %s\n", out_file);

	if (fp != NULL) {
		fclose(fp);
	}

	if (out_file != NULL) {
		free(out_file);
		out_file = NULL;
	}

	return 0;
}

static int write_dst_raw_data_to_file(u8 *dst_raw_data)
{
	FILE* fp = NULL;
	char* out_file = NULL;
	char* out_file_name = NULL;

	out_file = (char*)malloc(OUTPUT_FILE_LENGTH);
	if (out_file == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	out_file_name = (char*)malloc(OUTPUT_FILE_NAME_LENGTH);
	if (out_file_name == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	snprintf(out_file_name, OUTPUT_FILE_NAME_LENGTH, "dst_raw_from_odd_src_%dx%d_%d.bin",
		DST_RAW_WIDTH, DST_RAW_HEIGHT, DST_RAW_WIDTH * 2);

	strcpy_s(out_file, strlen(FILE_PATH) + 1, FILE_PATH);
	strcat_s(out_file, strlen(FILE_PATH) + strlen(out_file_name) + 1, out_file_name);

	fopen_s(&fp, out_file, "wb");
	if (fp == NULL) {
		printf("File is not exist, %s!\n", out_file);
		return -1;
	}

	fwrite(dst_raw_data, DST_RAW_SIZE, 1, fp);

	printf("output file name: %s\n", out_file);

	fclose(fp);
	if (out_file != NULL) {
		free(out_file);
		out_file = NULL;
	}

	return 0;
}

void draw_pm_line(u8* row, int left_x, int right_x)
{
	int width = 0;
	int left, right, left_start, right_remain, value;
	u8* column;
	int i = 0;

	if (left_x < 0) {
		left_x = 0;
	}

	width = right_x - left_x;
	if (width <= 0) {
		return;
	}

	value = 0xFF; //each bit = 1, means there is privacy mask for each pixel

	left = left_x >> 3;
	right = right_x >> 3;
	left_start = left_x & 0x7;
	right_remain = right_x & 0x7;

	if (left == right && (left_start + width < 8)) {
		/*This is for small PM width is within 8 pixel*/
		column = row + left;
		value = 0xFF;
		value <<= left_start;
		value &= (0xFF >> (8 - left_start - width));
		*column |= value;
	} else {
		/* This is for PM ocupies more than one byte(8 pixel) */
		column = row + left + 1;
		for (i = 0; i < right - left - 1; i++) {
			*column = value;
			column++;
		}

		column = row + left;
		*column |= (0xFF << left_start);

		column = row + right;
		if (right_remain) {
			*column |= (0xFF >> (8 - right_remain));
		}
	}

	return;
}

/* generae privacy mask data for amba dsp */
int generate_pm_data(u8 *dst_pm_data)
{
	struct pm_rect pm;
	u32 pitch = PM_FILE_WIDTH;
	u32 left = 0, right = 0;
	u32 i = 0;

	pm.x = PM_START_X;
	pm.y = PM_START_Y;
	pm.width = PM_RECT_WIDTH;
	pm.height = PM_RECT_HEIGHT;

	left = pm.x;
	right = pm.x + pm.width;

	memset(dst_pm_data, 0, PM_SIZE);

	for (i = 0; i < pm.height; ++i) {
		draw_pm_line(dst_pm_data, left, right);

		dst_pm_data += pitch;
	}

	return 0;
}

static int write_dst_pm_data_to_file(u8* dst_pm_data)
{
	FILE* fp = NULL;
	char* out_file = NULL;
	char* out_file_name = NULL;

	out_file = (char*)malloc(OUTPUT_FILE_LENGTH);
	if (out_file == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	out_file_name = (char*)malloc(OUTPUT_FILE_NAME_LENGTH);
	if (out_file_name == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	snprintf(out_file_name, OUTPUT_FILE_NAME_LENGTH, "dst_pm_%dx%d_%dx%d.bin",
		PM_FILE_WIDTH, PM_FILE_HEIGHT, PM_RECT_WIDTH, PM_RECT_HEIGHT);

	strcpy_s(out_file, strlen(FILE_PATH) + 1, FILE_PATH);
	strcat_s(out_file, strlen(FILE_PATH) + strlen(out_file_name) + 1, out_file_name);

	fopen_s(&fp, out_file, "wb");
	if (fp == NULL) {
		printf("File is not exist, %s!\n", out_file);
		return -1;
	}
	fwrite(dst_pm_data, PM_SIZE, 1, fp);

	printf("output file name: %s\n", out_file);

	fclose(fp);
	if (out_file != NULL) {
		free(out_file);
		out_file = NULL;
	}

	return 0;
}

int main()
{
	u8* src_raw_data = NULL;
	u8* dst_raw_data = NULL;
	u8* src_yuv_data = NULL;
	u8* dst_yuv_y_data = NULL;
	u8* dst_yuv_uv_data = NULL;
	u8* dst_pm_data = NULL;
	int ret = 0;

	src_yuv_data = (u8 *)malloc(SRC_YUV_SIZE);
	if (src_yuv_data == NULL) {
		printf("malloc error for src raw data!\n");
		return -1;
	}

	ret = read_src_yuv_data(src_yuv_data);
	if (ret < 0) {
		printf("Fail to read src yuv data!\n");
		return -1;
	}

	dst_yuv_y_data = (u8 *)malloc(SRC_YUV_Y_SIZE);
	if (dst_yuv_y_data == NULL) {
		printf("malloc error for dst_yuv_y_data!\n");
		return -1;
	}
	dst_yuv_uv_data = (u8 *)malloc(SRC_YUV_UV_SIZE);
	if (dst_yuv_uv_data == NULL) {
		printf("malloc error for dst_yuv_uv_data!\n");
		return -1;
	}

	ret = generate_dst_yuv_data(src_yuv_data, dst_yuv_y_data, dst_yuv_uv_data);
	if (ret < 0) {
		printf("Fail to generate dst yuv data!\n");
		return -1;
	}

	ret = write_dst_yuv_data_to_file(dst_yuv_y_data, 1);
	if (ret < 0) {
		printf("Fail to write yuv_y data!\n");
		return -1;
	}

	ret = write_dst_yuv_data_to_file(dst_yuv_uv_data, 0);
	if (ret < 0) {
		printf("Fail to write yuv_y data!\n");
		return -1;
	}

	dst_pm_data = (u8*)malloc(PM_SIZE);
	if (dst_yuv_uv_data == NULL) {
		printf("malloc error for dst_yuv_uv_data!\n");
		return -1;
	}

	ret = generate_pm_data(dst_pm_data);
	if (ret < 0) {
		printf("Fail to generate pm data!\n");
		return -1;
	}

	ret = write_dst_pm_data_to_file(dst_pm_data);
	if (ret < 0) {
		printf("Fail to write pm data!\n");
		return -1;
	}

	src_raw_data = (u8 *)malloc(SRC_RAW_SIZE);
	if (src_raw_data == NULL) {
		printf("malloc error for src raw data!\n");
		return -1;
	}

	ret = read_src_raw_data(src_raw_data);
	if (ret < 0) {
		printf("Fail to read src raw data!\n");
		return -1;
	}

	dst_raw_data = (u8 *)malloc(DST_RAW_SIZE);
	if (dst_raw_data == NULL) {
		printf("malloc error for dst raw data!\n");
		return -1;
	}

	ret = generate_dst_raw_data(src_raw_data, dst_raw_data);
	if (ret < 0) {
		printf("Fail to read src raw data!\n");
		return -1;
	}

	ret = write_dst_raw_data_to_file(dst_raw_data);
	if (ret < 0) {
		printf("Fail to write dst raw data!\n");
		return -1;
	}

	if (src_raw_data != NULL) {
		free(src_raw_data);
		src_raw_data = NULL;
	}

	if (dst_raw_data != NULL) {
		free(dst_raw_data);
		dst_raw_data = NULL;
	}

	Sleep(2 * 1000);
	return 0;
}