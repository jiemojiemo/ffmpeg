//
//  main.cpp
//  save_video_to_ppm
//
//  Created by meitu on 15/10/15.
//  Copyright © 2015年 meitu. All rights reserved.
//
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif
static void SaveFrame(AVFrame* pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[32];
    int y;
    
    //Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL) {
        fprintf(stderr, "Could not open the ppm file.\n");
        return;
    }
    //Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    
    //Write pixel data
    for (y = 0; y < height; ++y) {
        fwrite(pFrame->data[0] + y*pFrame->linesize[0], 1, width*3, pFile);
    }
    
    fclose(pFile);
}
int main(int argc, char *argv[])
{
    av_register_all();
    
    AVFormatContext *pFormatCtx = NULL;
    
    //Open video file and allocate format context
    //读取文件的头部信息，并且保持这些信息到AVFormatContext中
    //如果AVFormatContext是NULL,这个函数将会同时申请一个空间
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) < 0) {
        av_log(pFormatCtx, AV_LOG_ERROR, "Could not open file.\n");
        return 1;
    }
    //检查在文件的流的信息
    //这个函数为 pFormatCtx-streams填充正确的信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        av_log(pFormatCtx, AV_LOG_ERROR, "Could not find stream info.\n");
        return 1;
    }
    
    //输出信息
    av_dump_format(pFormatCtx, 0, argv[1], 0);
    
    //找到视频流
    int videoStream = 0;
    videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStream == -1) {
        av_log(pFormatCtx, AV_LOG_ERROR, "Could not find stream index.\n");
        return 1;
    }
    
    AVCodecContext *pCodecCtx = NULL;
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodec *pCodec = NULL;
    
    pCodec =avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        av_log(pCodec, AV_LOG_ERROR, "Could not find decoder.\n");
        return 1;
    }
    
    //open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        av_log(pCodecCtx, AV_LOG_ERROR, "Could open decoder.\n");
        return 1;
    }
    
    AVFrame *pFrameRGB = NULL;
    pFrameRGB = av_frame_alloc();
    if (!pFrameRGB) {
        av_log(pFrameRGB , AV_LOG_ERROR, "Could allocate frame.\n");
        return 1;
    }
    
    uint8_t *buffer;
    int numBytes = 0;
    
    /*
        之前我们申请了一个 帧 对象，当转换的时候，我们仍然需要一个地方来放置原始的数据。
        我们使用avpicture_get_size来获取大小，然后av_malloc申请大小
        avpicture_fill将申请的帧和我们新申请的内存结合起来
     */
//    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
//    buffer = (uint8_t *)av_malloc(numBytes);
//    avpicture_fill((AVPicture*)pFrameRGB, buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    
    //使用av_image_alloc代替上面注释的三行代码
    av_image_alloc(pFrameRGB->data, pFrameRGB->linesize, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, 1);
    
    //接下来，读入整个视频流，然后把它解码成帧，最后转换格式并且保存
    int frameFinished;
    AVPacket pkt;

    //用来存储解码后的原始数据
    AVFrame *pFrame = av_frame_alloc();
    
    int i = 0;
    auto img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    while (av_read_frame(pFormatCtx, &pkt) >= 0) {
        //Is this a packet from the video stream?
        if (pkt.stream_index == videoStream) {
            //Decode video frame
            //packet -----> frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &pkt);
            if (frameFinished) {
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, (uint8_t *const *)pFrameRGB->data, pFrameRGB->linesize);
                if( ++i <= 100 )
                    SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
            }
            av_free_packet(&pkt);
        }
        
        
    }
    sws_freeContext(img_convert_ctx);
    
    //free the rgb image
    //av_free(buffer);
    av_free(pFrameRGB);
    
    //free the yuv frame
    av_free(pFrame);
    
    //close the codec
    avcodec_close(pCodecCtx);
    
    //close the video file
    avformat_close_input(&pFormatCtx);
    return 0;
}























