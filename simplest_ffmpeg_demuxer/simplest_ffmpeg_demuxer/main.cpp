/**
 * 最简单的基于FFmpeg的视音频分离器
 * Simplest FFmpeg Demuxer
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序可以将封装格式中的视频码流数据和音频码流数据分离出来。
 * 在该例子中， 将MPEG2TS的文件分离得到H.264视频码流文件和AAC
 * 音频码流文件。
 *
 * This software split a media file (in Container such as
 * MKV, FLV, AVI...) to video and audio bitstream.
 * In this example, it demux a MPEG2TS file to H.264 bitstream
 * and AAC bitstream.
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavformat/avformat.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include "libavformat/avformat.h"
#ifdef __cplusplus
};
#endif
#endif

/*
 FIX: H.264 in some container format (FLV, MP4, MKV etc.) need
 "h264_mp4toannexb" bitstream filter (BSF)
 *Add SPS,PPS in front of IDR frame
 *Add start code ("0,0,0,1") in front of NALU
 H.264 in some container (MPEG2TS) don't need this BSF.
 */
//'1': Use H.264 Bitstream Filter
#define USE_H264BSF 0

int main(int argc, char* argv[])
{
    AVOutputFormat *ofmt_a = NULL, *ofmt_v = NULL;
    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx_a = NULL, *ofmt_ctx_v = NULL;
    AVPacket pkt;
    int ret, i;
    int videoindex = -1, audioindex = -1;
    int frame_index = 0;
    
    const char *in_filename = "cuc_ieschool.ts";
    //const char *in_filename = "cuc_ieshool.mkv";
    const char *out_filename_v = "cuc_ieschool.h264";
    //const *out_filename_a = "cuc_ieschool.mp3";
    const char *out_filename_a = "cuc_ieschool.aac";
    
    av_register_all();
    
    //read input file headers
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        
        printf( "Could not open input file.");
        goto end;
    }

    
    //find stream information
    avformat_find_stream_info(ifmt_ctx, NULL);
    
    //Output
    //video
    avformat_alloc_output_context2(&ofmt_ctx_v, NULL, NULL, out_filename_v);
    ofmt_v = ofmt_ctx_v->oformat;
    //audio
    avformat_alloc_output_context2(&ofmt_ctx_a, NULL, NULL, out_filename_a);
    ofmt_a = ofmt_ctx_a->oformat;
    
    for (i = 0; i < ifmt_ctx->nb_streams; ++i) {
        AVFormatContext *ofmt_ctx;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVStream *out_stream = NULL;
        
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {

            
            printf("%s\n",avcodec_get_name(currentStream->codec->codec_id));
            videoindex = i;
            out_stream = avformat_new_stream(ofmt_ctx_v, in_stream->codec->codec);
            ofmt_ctx = ofmt_ctx_v;
        }
        else if(ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            printf("%s\n",avcodec_get_name(currentStream->codec->codec_id));
            audioindex = i;
            out_stream = avformat_new_stream(ofmt_ctx_a, in_stream->codec->codec);
            ofmt_ctx = ofmt_ctx_a;
        }
        else
            break;
        
        //Copy the settings of AVCodecContext
        
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            printf( "Failed to copy context from input to output stream codec context\n");
        }
        out_stream->codec->codec_tag = 0;
        
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    }
    
    //Dump Format------------------
    
    printf("\n==============Input Video=============\n");
    
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    printf("\n==============Output Video============\n");
    av_dump_format(ofmt_ctx_v, 0, out_filename_v, 1);
    printf("\n==============Output Audio============\n");
    av_dump_format(ofmt_ctx_a, 0, out_filename_a, 1);
    printf("\n======================================\n");

    //open output file
    if (!(ofmt_v->flags & AVFMT_NOFILE)) {
        avio_open(&ofmt_ctx_v->pb, out_filename_v, AVIO_FLAG_WRITE);
    }
    
    if (!(ofmt_a->flags & AVFMT_NOFILE)) {
        avio_open(&ofmt_ctx_a->pb, out_filename_a, AVIO_FLAG_WRITE);
    }
    
    //write file header
    avformat_write_header(ofmt_ctx_v, NULL);
    avformat_write_header(ofmt_ctx_a, NULL);
    
#if USE_H264BSF
    
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
#endif
    

    while (1) {
        AVFormatContext *ofmt_ctx;
        AVStream *in_stream, *out_stream;
        //get an packet
        if(av_read_frame(ifmt_ctx, &pkt) < 0)
            break;
        
        in_stream = ifmt_ctx->streams[pkt.stream_index];
        
        if(pkt.stream_index == videoindex){
            out_stream = ofmt_ctx_v->streams[0];
            ofmt_ctx = ofmt_ctx_v;
            printf("Write Video Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
#if USE_H264BSF
            
            av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
        }
        else if(pkt.stream_index == audioindex){
            out_stream = ofmt_ctx_a->streams[0];
            ofmt_ctx = ofmt_ctx_a;
            printf("Write Audio Packet. size:%d\tpts:%lld\n",pkt.size,pkt.pts);
        }
        else
            continue;
        
        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        //必须置零
        pkt.stream_index = 0;
        
        //Write
        av_interleaved_write_frame(ofmt_ctx, &pkt);
        
        av_free_packet(&pkt);
    }
#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
    
    
    //Write file trailer
    
    av_write_trailer(ofmt_ctx_a);
    
    av_write_trailer(ofmt_ctx_v);

    
end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx_a && !(ofmt_a->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx_a->pb);
    
    if (ofmt_ctx_v && !(ofmt_v->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx_v->pb);
    
    avformat_free_context(ofmt_ctx_a);
    avformat_free_context(ofmt_ctx_v);
    
    
    if (ret < 0 && ret != AVERROR_EOF) {
        printf( "Error occurred.\n");
        return -1;
    }
    return 0;

    
}




































