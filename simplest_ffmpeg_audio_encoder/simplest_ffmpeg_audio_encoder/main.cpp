/**
 * 最简单的基于FFmpeg的音频编码器（纯净版）
 * Simplest FFmpeg Audio Encoder Pure
 *
 *
 * 本程序实现了PCM采样数据编码为音频码流（AAC，mp3等等）。
 * 它仅仅使用了libavcodec（而没有使用libavformat）。
 * 是最简单的FFmpeg视频编码方面的教程。
 * 通过学习本例子可以了解FFmpeg的编码流程。
 * This software encode PCM data to audio bitstream
 * (Such as aac, mp3 etc).
 * It only uses libavcodec to encode video (without libavformat)
 * It's the simplest audio encoding software based on FFmpeg.
 * Suitable for beginner of FFmpeg.
 */

/*  将pcm音频文件编码为aac
 *  大致流程如下
 *  avcodec_register_all        注册所有的编解码器
 *  avcodec_find_encoder        找到我们需要的编解码器
 *  avcodec_alloc_context3      申请 编码器上下文
 *  set codecContext            设置 编码器上下文的参数
 *  avcodec_open2               打开编解码器
 *  av_frame_alloc              申请 帧 对象
 *  av_samples_alloc            申请存放音频的空间
 *  avcodec_fill_audio_frame    将存放音频的空间 和 帧 绑定
 *  read raw data               读入待编码数据
 *  avcodec_encoder_audio2      进行编码
 *  write encoded data to file  将编码后的数据写入文件
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
};
#endif
#endif


int main(int argc, char* argv[])
{
    AVCodec *pCodec;
    AVCodecContext *pCodecCtx= NULL;
    int i, ret, got_output;
    FILE *fp_in;
    FILE *fp_out;
    
    AVFrame *pFrame;
    uint8_t* frame_buf;
    int size=0;
    
    AVPacket pkt;
    int y_size;
    int framecnt=0;
    
    char filename_in[]="tdjm.pcm";
    
    AVCodecID codec_id=AV_CODEC_ID_AAC;
    char filename_out[]="tdjm.aac";
    
    int framenum=1000;
    
    avcodec_register_all();
    
    pCodec = avcodec_find_encoder(codec_id);
    if (!pCodec) {
        printf("Codec not found\n");
        return -1;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        printf("Could not allocate video codec context\n");
        return -1;
    }
    
    pCodecCtx->codec_id = codec_id;
    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    pCodecCtx->sample_rate= 44100;
    pCodecCtx->channel_layout=AV_CH_LAYOUT_STEREO;
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    pCodecCtx->bit_rate = 64000;
    
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        return -1;
    }
    
    pFrame = av_frame_alloc();
    pFrame->nb_samples= pCodecCtx->frame_size;
    pFrame->format= pCodecCtx->sample_fmt;
    av_samples_alloc(&pFrame->data[0], pFrame->linesize, pCodecCtx->channels, pCodecCtx->frame_size, pCodecCtx->sample_fmt, 1);
    //Fill AVFrame audio data and linesize pointers.
    //This function fills in frame->data,frame->extended_data, frame->linesize[0].
    avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt,pFrame->data[0], size, 1);
    
    //Input raw data
    fp_in = fopen(filename_in, "rb");
    if (!fp_in) {
        printf("Could not open %s\n", filename_in);
        return -1;
    }
    //Output bitstream
    fp_out = fopen(filename_out, "wb");
    if (!fp_out) {
        printf("Could not open %s\n", filename_out);
        return -1;
    }
    
    //Encode
   // av_init_packet(&pkt);
    for (i = 0; i < framenum; i++) {
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;
        //Read raw data
        if (fread(pFrame->data[0], 1, pFrame->linesize[0], fp_in) <= 0){
            printf("Failed to read raw data! \n");
            return -1;
        }else if(feof(fp_in)){
            break;
        }
        
        pFrame->pts = i;
        ret = avcodec_encode_audio2(pCodecCtx, &pkt, pFrame, &got_output);
        if (ret < 0) {
            printf("Error encoding frame\n");
            return -1;
        }
        if (got_output) {
            printf("Succeed to encode frame: %5d\tsize:%5d\n",framecnt,pkt.size);
            framecnt++;
            fwrite(pkt.data, 1, pkt.size, fp_out);
            av_free_packet(&pkt);
        }
    }
    //Flush Encoder
    for (got_output = 1; got_output; i++) {
        ret = avcodec_encode_audio2(pCodecCtx, &pkt, NULL, &got_output);
        if (ret < 0) {
            printf("Error encoding frame\n");
            return -1;
        }
        if (got_output) {
            printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",pkt.size);
            fwrite(pkt.data, 1, pkt.size, fp_out);
            av_free_packet(&pkt);
        }
    }
    
    fclose(fp_out);
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    av_freep(&pFrame->data[0]);
    av_frame_free(&pFrame);
    
    return 0;
}
