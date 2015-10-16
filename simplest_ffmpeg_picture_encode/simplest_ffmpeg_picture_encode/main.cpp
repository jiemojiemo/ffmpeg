/**
 * 最简单的基于FFmpeg的图像编码器
 * Simplest FFmpeg Picture Encoder
 *
 * 本程序实现了YUV420P像素数据编码为JPEG图片。是最简单的FFmpeg编码方面的教程。
 * 通过学习本例子可以了解FFmpeg的编码流程。
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};
#else

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#ifdef __cplusplus
};
#endif
#endif

int main(int argc, char *argv[])
{
    AVFormatContext* pFormatCtx = nullptr;
    AVOutputFormat* fmt = nullptr;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    
    uint8_t *picture_buf;
    AVFrame* picture;
    AVPacket pkt;
    int y_size = 0;
    int got_picture = 0;
    int size = 0;
    
    int ret = 0;
    
    // YUV source
    FILE *in_file = nullptr;
    int in_w = 480, in_h = 272;
    const char* out_file = "cuc_view_encode.jpg";
    
    in_file = fopen("cuc_view_480x272.yuv", "rb");
    
    av_register_all();
    
    pFormatCtx = avformat_alloc_context();
    // guess format
    fmt = av_guess_format("mjpeg", NULL, NULL);
    pFormatCtx->oformat = fmt;
    if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Could not open output file.\n");
        return -1;
    }
//    video_st = avformat_new_stream(pFormatCtx, 0);
//    if (video_st==NULL){
//        return -1;
//    }
//    pCodecCtx = video_st->codec;
//    pCodecCtx->codec_id = fmt->video_codec;
//    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
//    pCodecCtx->pix_fmt = PIX_FMT_YUVJ420P;
//    
//    pCodecCtx->width = in_w;
//    pCodecCtx->height = in_h;
//    
//    pCodecCtx->time_base.num = 1;
//    pCodecCtx->time_base.den = 25;
//    //Output some information
//    av_dump_format(pFormatCtx, 0, out_file, 1);
//    
//    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    
    video_st = avformat_new_stream(pFormatCtx, 0);
    
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = PIX_FMT_YUVJ420P;
    
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    
    av_dump_format(pFormatCtx, 0, out_file, 1);
    
    //找到对应格式的编码器
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    
    if (!pCodec){
        printf("Codec not found.");
        return -1;
    }
    //打开编码器
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0){
        printf("Could not open codec.");
        return -1;
    }
    
    picture = av_frame_alloc();
    size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *)av_malloc(size);
    
    avpicture_fill((AVPicture*)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    
    avformat_write_header(pFormatCtx, NULL);
    
    y_size = pCodecCtx->width * pCodecCtx->height;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    
    if (fread(picture_buf, 1, y_size*3/2, in_file) <=0)
    {
        printf("Could not read input file.");
        return -1;
    }
    picture->data[0] = picture_buf;  // 亮度Y
    picture->data[1] = picture_buf+ y_size;  // U
    picture->data[2] = picture_buf+ y_size*5/4; // V
    
    ret = avcodec_encode_video2(pCodecCtx, &pkt, picture, &got_picture);
    if (got_picture) {
        pkt.stream_index = video_st->index;
        ret = av_write_frame(pFormatCtx, &pkt);
    }
    
    av_free_packet(&pkt);
    av_free_packet(&pkt);
    //Write Trailer
    av_write_trailer(pFormatCtx);
    
    printf("Encode Successful.\n");
    
    if (video_st){
        avcodec_close(video_st->codec);
        av_free(picture);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    
    fclose(in_file);
    
    return 0;
}


























