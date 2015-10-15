/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libswscale API use example.
 * @example scaling_video.c
 */

/*  将 YUV420P 转换为 RGB24
 *  大致流程如下
 *  sws_getContext      获取 转换 的上下文
 *  av_image_alloc      申请空间存放数据
 *  sws_scale           进行变换
 */

#ifdef __cplusplus
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
extern "C"
{
#endif
#include "libavutil/imgutils.h"
#include "libavutil/parseutils.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif
#define YUV_FILE_NAME "yuvout.yuv"
static void fill_yuv_image( uint8_t *data[4], int linesize[4], int width, int height, int frame_index)
{
    int x;
    int y;
    // Y
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; ++x) {
            data[0][y * linesize[0] + x] = x + y + frame_index *3;
        }
    }
    // U V
    for (y = 0; y < height / 2; ++y) {
        for (x = 0; x < width / 2; ++x) {
            data[1][y * linesize[1] + x] = 128 + y + frame_index * 2;
            data[2][y * linesize[2] + x] = 64 + x + frame_index * 5;
        }
    }
}

static void saveYUV420P(unsigned char *buf, int wrap, int xsize, int ysize)
{
    FILE *f = NULL;
    int i;
    
    if (buf == NULL) {
        fprintf(stderr, "buf == NULL\n");
        return;
    }
    
    f=fopen(YUV_FILE_NAME, "ab+");
    for (i = 0; i < ysize; ++i) {
        fwrite(buf + i*wrap, 1, xsize, f);
    }
    fflush(f);
    fclose(f);
    f = NULL;
}

int main( int argc, char* argv[] )
{
    uint8_t *src_data[4], *dst_data[4];
    int src_linesize[4], dst_linesize[4];
    int src_w = 320, src_h = 240, dst_w, dst_h;
    enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_RGB24;
    const char *dst_size = nullptr;
    const char *dst_filename = nullptr;
    FILE *dst_file = nullptr;
    int dst_buffer;
    struct SwsContext *sws_ctx;
    int i, ret;
    
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s output_file output_size\n"
                "API example program to show how to scale an image with libswscale.\n"
                "This program generates a series of pictures, rescales them to the given "
                "output_size and saves them to an output file named output_file\n."
                "\n", argv[0]);
        exit(1);
    }
    dst_filename = argv[1];
    dst_size     = argv[2];
    
    if (av_parse_video_size(&dst_w, &dst_h, dst_size) < 0) {
        fprintf(stderr, "Invalid size '%s', must be in the form WxH or a valid size abbreviation\n",dst_size);
        exit(1);
    }
    
    dst_file = fopen(dst_filename, "wb");
    if (!dst_file) {
        fprintf(stderr, "Could not open destination file %s\n", dst_filename);
        exit(1);
    }
    
    // create scaling context
    sws_ctx = sws_getContext(src_w, src_h, src_pix_fmt, dst_w, dst_h, dst_pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);
    
    if (!sws_ctx) {
        fprintf(stderr,
                "Impossible to create scale context for the conversion "
                "fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
                av_get_pix_fmt_name(src_pix_fmt), src_w, src_h,
                av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h);
        ret = AVERROR(EINVAL);
        //goto end;
    }
    // allocate source and destination image buffers
    if ((ret = av_image_alloc(src_data, src_linesize, src_w, src_h, src_pix_fmt, 16)) < 0) {
        fprintf(stderr, "Could not allocate source image\n");
       // goto end;
    }
    
    if ((ret = av_image_alloc(dst_data, dst_linesize, dst_w, dst_h, dst_pix_fmt, 1)) < 0) {
        fprintf(stderr, "Could not allocate source image\n");
       // goto end;
    }
    
    dst_buffer = ret;
    FILE* yuvFile = fopen("yuvout.yuv", "wb");
    for (i = 0; i < 100; ++i) {
        // generate synthetix video
        fill_yuv_image(src_data, src_linesize, src_w, src_h, i);
//        saveYUV420P(src_data[0], src_linesize[0], src_w, src_h);
//        saveYUV420P(src_data[1], src_linesize[1], src_w/2, src_h/2);
//        saveYUV420P(src_data[2], src_linesize[2], src_w/2, src_h/2);
        fwrite(src_data[0], 1, src_w*src_h, yuvFile);
        fwrite(src_data[1], 1, src_w*src_h/4, yuvFile);
        fwrite(src_data[2], 1, src_w*src_h/4, yuvFile);
        // convert to destination format
        sws_scale(sws_ctx, (const uint8_t * const*)src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
        
        // write scaled image to file
        fwrite(dst_data[0], 1, dst_buffer, dst_file);
    }
    fclose(yuvFile);
    fprintf(stderr, "Scaling succeeded. Play the output file with the command:\n"
            "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
            av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h, dst_filename);
    
    
end:
    fclose(dst_file);
    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);
    sws_freeContext(sws_ctx);
    return ret < 0;
    
}














































