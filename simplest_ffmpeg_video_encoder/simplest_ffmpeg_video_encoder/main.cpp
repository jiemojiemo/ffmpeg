/**
 * 最简单的基于FFmpeg的视频编码器
 * Simplest FFmpeg Video Encoder
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序实现了YUV像素数据编码为视频码流（H264，MPEG2，VP8等等）。
 * 是最简单的FFmpeg视频编码方面的教程。
 * 通过学习本例子可以了解FFmpeg的编码流程。
 * This software encode YUV420P data to H.264 bitstream.
 * It's the simplest video encoding software based on FFmpeg.
 * Suitable for beginner of FFmpeg
 */
#include <stdio.h>
#include "scopeguard.h"

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//windows
extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};
#else
//linux
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#ifdef __cplusplus
};
#endif
#endif

int flush_encoder(AVFormatContext* fmt_ctx, unsigned int stream_index)
{
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & CODEC_CAP_DELAY)) {
        return 0;
    }
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt, NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0) {
            break;
        }
        if (!got_frame) {
            ret = 0;
            break;
        }
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;

    }
    return ret;
}

int main()
{
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVPacket pkt;
    uint8_t* picture_buffer;
    AVFrame* pFrame;
    int picture_size;
    int y_size;
    int framecnt = 0;
    //Input raw YUV data
    FILE* in_file = fopen( "/Users/zj-dt0095/Desktop/problem_movie/ds_480x272.yuv","rb" );
    ON_SCOPE_EXIT([&](){fclose(in_file);});
    if( in_file == nullptr )
    {
        fprintf(stderr, "Error:Could not open the file.\n");
        exit(-1);
    }
    //Input data's width and height
    int in_w = 480, in_h = 272;
    //Frames to encode
    int framenum = 100;
    const char* outFileName = "/Users/zj-dt0095/Desktop/problem_movie/ds_480x272.h264";
    
    //注册复用器、编码器
    av_register_all();
    //分配一个AVFormatContext
    pFormatCtx = avformat_alloc_context();
    ON_SCOPE_EXIT([&](){avformat_free_context(pFormatCtx);});
    
    //Guess Format
    fmt = av_guess_format(NULL, outFileName, NULL);
    pFormatCtx->oformat = fmt;
    
    //Open output URL
    if (avio_open(&pFormatCtx->pb, outFileName, AVIO_FLAG_READ_WRITE) < 0) {
        fprintf(stderr, "Error:Failed to open output file!\n");
        exit(-1);
    }
    ON_SCOPE_EXIT([&](){avio_close(pFormatCtx->pb);});
    
    video_st = avformat_new_stream(pFormatCtx, 0);
    video_st->time_base.num = 1;
    video_st->time_base.den = 25;
    
    if (video_st == NULL) {
        return -1;
    }
    
    //Param that must set
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = PIX_FMT_YUV420P;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 250;
    //H264
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;
    //Optional Param
    pCodecCtx->max_b_frames = 3;
    //Set Option
    AVDictionary* param = 0;
    //H.264
    if(pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        //av_dict_set(&param, "profile", "main", 0);
        
    }
    //H.265
    if(pCodecCtx->codec_id == AV_CODEC_ID_H265){
        
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zero-latency", 0);
    }
    
    //Print some information
    av_dump_format(pFormatCtx, 0, outFileName, 1);
    
    //Init Codec
    //find the encodec
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        fprintf(stderr, "Error:Cannot find encoder!\n");
        exit(-1);
    }
    //open AVCodecContext
    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
        fprintf(stderr, "Error:Cannot open encoder!\n");
        exit(-1);
    }
    //alloc frame
    pFrame = av_frame_alloc();
    
    picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buffer = (uint8_t*)av_malloc(picture_size);
    //将buffer 与 pFrame 绑定在一起
    avpicture_fill((AVPicture*)pFrame, picture_buffer, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    
    //Write File Header
    avformat_write_header(pFormatCtx, NULL);
    
    //av_new_packet(&pkt, picture_size);
    av_init_packet(&pkt);
    
    y_size = pCodecCtx->width * pCodecCtx->height;
    
    for (int i = 0; i < framenum; ++i) {
        //Read raw YUV data
        if (fread(picture_buffer, 1, y_size*3/2, in_file)< 0) {
            fprintf(stderr, "Error:Cannot read raw data!\n");
            exit(-1);
        }
        else if(feof(in_file)){
            break;
        }
        
        pFrame->data[0] = picture_buffer;               //Y
        pFrame->data[1] = picture_buffer + y_size;      //U
        pFrame->data[2] = picture_buffer + y_size*5/4;  //V
        
        //PTS
        pFrame->pts = i;
        int got_picture = 0;
        //Encode raw data from pFrame and writes the output pkt
        int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
        if (ret < 0) {
            fprintf(stderr, "Error:Failed to encode!\n");
            exit(-1);
        }
        if (1 == got_picture) {
            printf("Succeed to encode frame: %5d\tsize:%5d\n",framecnt++, pkt.size);
            pkt.stream_index = video_st->index;
            ret = av_write_frame(pFormatCtx, &pkt);
            av_free_packet(&pkt);
        }
    }
    //Flush Encoder
    int ret = flush_encoder(pFormatCtx, 0);
    if (ret < 0) {
        fprintf(stderr, "Error:Failed to Flushing encoder!\n");
        exit(-1);
    }
    
    //Write file trailer
    av_write_trailer(pFormatCtx);
    
    //Clean
    if (video_st) {
        avcodec_close(video_st->codec);
        av_frame_free(&pFrame);
        av_free(picture_buffer);
    }
    

    



}









































