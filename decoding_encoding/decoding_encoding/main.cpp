/*
 * Copyright (c) 2001 Fabrice Bellard
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
 * libavcodec API use example.
 *
 * @example decoding_encoding.c
 * Note that libavcodec only handles codecs (mpeg, mpeg4, etc...),
 * not file formats (avi, vob, mp4, mov, mkv, mxf, flv, mpegts, mpegps, etc...). See library 'libavformat' for the
 * format handling
 */

#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

#define LOG printf
#define LOGE fprintf


/*  以 H.264 进行视频编码
 *  大致流程如下
 *  avcodec_find_encoder()   找到编解码器
 *  avcodec_alloc_context()  申请编解码器上下文
 *  avcodec_open2()          打开编解码器
 *  av_frame_alloc()         申请 帧 对象
 *  av_image_alloc()         为 帧 申请空间存放数据
 *  av_init_packet()         申请 包 对象
 *  avcodec_encode_video2()  进行编码
 *  write data to file       将编码后的数据写入文件
 *  free resource            释放资源
 */
static void video_encode_example(const char *fileName, enum AVCodecID codecID)
{
    const int STREAM_DURATION = 1; //5S video
    const int STREAM_FRAME_RATE = 25; // 25 frames per second
    const int STREAM_TOTAL_FRAMES = STREAM_DURATION * STREAM_FRAME_RATE;
    AVCodec *codec=nullptr;
    AVCodecContext *c=nullptr;
    int i = 0;
    int ret = 0;
    int x = 0;
    int y = 0;
    int got_output = 0;
    FILE *f = nullptr;
    AVFrame *frame = nullptr;
    AVPacket pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    
    LOG("Encode video file %s\n", fileName);
    
    //find the mpeg1 video encoder
    codec = avcodec_find_encoder(codecID);
    if (!codec) {
        LOGE(stderr,"Error:Could not find the Codec.\n");
        exit(-1);
    }
    
    //allocate vedio codec context
    c = avcodec_alloc_context3(codec);
    if (!c) {
        LOGE(stderr,"Error:Could not allocate video codec context.\n");
        exit(-1);
    }
    // put sample parameters
    c->bit_rate = 400000;
    // resolution must be a multiple of two
    c->width = 352;
    c->height = 288;
    // frames per second
    c->time_base = (AVRational){1,STREAM_FRAME_RATE};
    /* emit one intra frame every ten frame
     * check frame pich_type befor passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    //每10个frame，发送一个I frame
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    
    if (codecID == AV_CODEC_ID_H264) {
        //https://trac.ffmpeg.org/wiki/Encode/H.264 详细
        av_opt_set(c->priv_data, "preset", "slow", 0);
    }
    
    // open codec
    if (avcodec_open2(c, codec, NULL)<0) {
        LOGE(stderr, "Error:Could not open codec.\n");
        exit(-1);
    }
    
    // open file
    f = fopen(fileName, "wb");
    if (!f) {
        LOGE(stderr, "Error:Could not open %s\n", fileName);
        exit(-1);
    }

    //allocate frame
    frame = av_frame_alloc();
    if (!frame) {
        LOGE(stderr, "Error:Could not allocate frame.\n");
        exit(-1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
    
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
    
    if (ret < 0) {
        LOGE(stderr, "Error:Could not allocate raw picture buffer\n");
        exit(-1);
    }
    
    /* encode 1 second of video */
    av_init_packet(&pkt);
    for (i = 0; i < STREAM_TOTAL_FRAMES; i++) {
        pkt.data = NULL; //packet data will be allocated by the encoder
        pkt.size = 0;
        
        fflush(stdout);
        // prepate a dummy image
        // Y
        for (y = 0; y < c->height; ++y) {
            for (x = 0; x < c->width; ++x) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }
        
        // Y and V
        for (y = 0; y < c->height/2; ++y) {
            for (x = 0; x < c->width/2; ++x) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + y + i * 2;
            }
        }
        
        frame->pts = i;
        // encode the image
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            LOGE(stderr, "Error: Encoding frame failed.\n");
            exit(-1);
        }
        
        if(got_output){
            LOG("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }
    
    // get the delayed frames
    for (got_output = 1; got_output; ++i) {
        fflush(stdout);
        
        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            LOGE(stderr, "Error: Encoding frame failed.\n");
            exit(-1);
        }
        
        LOG("Write frame %3d (size=%5d) delayed\n", i, pkt.size);
        fwrite(pkt.data, 1, pkt.size, f);
        av_free_packet(&pkt);
        
    }//end for
    
    //add sequence end code to have a real mpeg file
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);
    
    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    LOG("\n");
    
}



/*  以 MPEG1 进行解码
 *  大致流程如下
 *  avcodec_find_decoder()          找到编解码器
 *  avcodec_alloc_context3()        申请编解码器上下文
 *  avcodec_open2()                 打开编解码器
 *  av_frame_alloc()                申请 帧 对象
 *  av_init_packet()                申请 包 对象
 *  while( get data -> packet )     获取数据
 *  {
 *      avcodec_decode_video2()     进行解码
 *      pgm_save()                  保存解码结果的帧
 *  }
 *  free resource                   释放资源
 */
static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; ++i) {
        fwrite(buf + i*wrap, 1, xsize, f);
    }
    
    fclose(f);
}
static int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
                              AVFrame *frame, int *frame_cout, AVPacket *pkt, int last)
{
    int len, got_frame;
    char buf[1024];
    //编码的同时，为 frame 申请了空间
    len = avcodec_decode_video2(avctx, frame, &got_frame, pkt);
    if(len < 0){
        LOGE(stderr, "Error:while decoding frame %d\n", *frame_cout);
        return len;
    }
    
    if (got_frame) {
        LOG("Saving %s frame %3d\n", last ? "last " : "", *frame_cout);
        fflush(stdout);
        
        // the picture is allocated by the decoder, no need to free it
        snprintf(buf, sizeof(buf), outfilename, *frame_cout);
        pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
        (*frame_cout)++;
    }
    if(pkt->data){
        pkt->size -= len;
        pkt->data += len;
    }
    return 0;
}

static void video_decode_example(const char *outFileName,const char *fileName )
{
    AVCodec *codec = nullptr;
    AVCodecContext *c = nullptr;
    int frame_count = 0;
    FILE *f = nullptr;
    AVFrame *frame=nullptr;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    AVPacket pkt;
    
    av_init_packet(&pkt);
    
    // set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams)
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    
    LOG("Decode video file %s to %s\n", fileName, outFileName);
    
    // find the mpeg1 video decoder
    codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
    if (!codec) {
        LOGE(stderr,"Error:Could not find the Codec.\n");
        exit(-1);
    }
    
    //allocate vedio codec context
    c = avcodec_alloc_context3(codec);
    if (!c) {
        LOGE(stderr,"Error:Could not allocate video codec context.\n");
        exit(-1);
    }
    
    if (codec->capabilities & AV_CODEC_CAP_TRUNCATED) {
        c->flags |= AV_CODEC_FLAG_TRUNCATED;
    }
    
    // open codec
    if (avcodec_open2(c, codec, NULL) < 0) {
        LOGE(stderr, "Error:Could not open codec.\n");
        exit(-1);
    }
    
    //open file
    f = fopen(fileName, "rb");
    if (!f) {
        LOGE(stderr, "Error:Could not open %s\n", fileName);
        exit(1);
    }
    
    //allocate frame
    frame = av_frame_alloc();
    if (!frame) {
        LOGE(stderr, "Error:Could not allocate video frame\n");
        exit(-1);
    }
    
    frame_count = 0;
    for (; ; ) {
        pkt.size = fread(inbuf, 1, INBUF_SIZE, f);
        if (pkt.size == 0) {
            break;
        }
        
        pkt.data = inbuf;
        while (pkt.size > 0) {
            if (decode_write_frame(outFileName, c, frame, &frame_count, &pkt, 0) < 0) {
                exit(-1);
            }
        }
    }
    
    pkt.data = NULL;
    pkt.size = 0;
    decode_write_frame(outFileName, c, frame, &frame_count, &pkt, 1);
    
    fclose(f);
    
    avcodec_close(c);
    av_free(c);
    av_frame_free(&frame);
    printf("\n");
    
    
}

//audio decode and encode
static int check_sample_fmt(AVCodec *codec, enum AVSampleFormat fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == fmt) {
            return 1;
        }
        p++;
    }
    return 0;
}
//just pick the highest supported sample rate
static int select_sample_rate(AVCodec *codec)
{
    const int *p=nullptr;
    int best_sampleRate = 0;
    
    if (!codec->supported_samplerates) {
        return 44100;
    }
    
    p = codec->supported_samplerates;
    //寻找最大的sample rate
    while (*p) {
        best_sampleRate = FFMAX(*p, best_sampleRate);
        p++;
    }
    
    return best_sampleRate;
}

static int select_channel_layout(AVCodec *codec)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels = 0;
    
    if (!codec->channel_layouts) {
        return AV_CH_LAYOUT_STEREO;
    }
    
    p = codec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels(*p);
        
        if (nb_channels > best_nb_channels) {
            best_nb_channels = nb_channels;
            best_ch_layout = *p;
        }
        ++p;
    }
    
    return best_ch_layout;
}
/*  以 mp2 进行音频编码
 *  大致流程如下
 *  avcodec_find_encoder()      找到编解码器
 *  avcodec_alloc_context3()    申请编解码器上下文
 *  avcodec_open2()             打开编解码器
 *  av_frame_alloc()            申请 帧 对象
 *  avcodec_fill_audio_frame()  为 帧 对象绑定内存空间
 *  av_init_packet()            申请 包 对象
 *  avcodec_encode_audio()      进行编码
 *  write data to file          将编码后的数据写入文件
 *  free resource               释放资源
 */
static void audio_encode_example(const char *fileName)
{
    AVCodec *codec = nullptr;
    AVCodecContext *c = nullptr;
    AVFrame *frame = nullptr;
    AVPacket pkt;
    int i,j,k;
    int ret = 0;
    int got_output = 0;
    int buffer_size = 0;
    FILE *f = nullptr;
    uint16_t *samples;
    float t, tincr;
    
    LOG("Encode audio file %s\n", fileName);
    
    //find the MP2 encoder
    //codec = avcodec_find_encoder(AV_CODEC_ID_MP2);
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        LOGE(stderr, "Error:Could not find MP2 Codec.\n");
        exit(-1);
    }
    
    c = avcodec_alloc_context3(codec);
    if (!c) {
        LOGE(stderr,"Error:Could not allocate audio codec context.\n");
        exit(-1);
    }
    // put sample parameters
    c->bit_rate = 64000;
    
    // check that the encoder support s16 pcm input
    c->sample_fmt = AV_SAMPLE_FMT_S16;
    if (!check_sample_fmt(codec, c->sample_fmt)) {
        LOGE(stderr, "Error:Encoder does not support sample format %s", av_get_sample_fmt_name(c->sample_fmt));
        exit(-1);
    }
    
    // select other audio parameters supported by encoder
    c->sample_rate = select_sample_rate(codec);
    c->channel_layout = select_channel_layout(codec);
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    
    // open codec
    if (avcodec_open2(c, codec, NULL) < 0) {
        LOGE(stderr, "Error:Could not open codec.\n");
        exit(1);
    }
    //open file
    f = fopen(fileName, "wb");
    if (!f) {
        LOGE(stderr, "Error:Could not allocate audio frame.\n");
        exit(1);
    }
    
    // frame containing input raw audio
    frame = av_frame_alloc();
    if (!frame) {
        LOGE(stderr, "Error:Could not allocate audio frame.\n");
        exit(-1);
    }
    frame->nb_samples = c->frame_size;
    frame->format = c->sample_fmt;
    frame->channel_layout = c->channel_layout;
    
    /* the codec gives us the frame size, in samples,
     * we calculate the size of the samples buffer in bytes
     */
    buffer_size = av_samples_get_buffer_size(NULL, c->channels, c->frame_size, c->sample_fmt, 0);
    if (buffer_size < 0) {
        LOGE(stderr, "Error:Could not get sample buffer size.\n");
        exit(1);
    }
    //获取空间，存放音频数据
    samples = (uint16_t*)av_malloc(buffer_size);
    if (!samples) {
        LOGE(stderr, "Error:Could not allocate %d bytes for samples buffer.\n",buffer_size);
        exit(-1);
    }
    ret = avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt, (uint8_t*)samples, buffer_size, 0);
    //av_samples_alloc(frame->data, frame->linesize, c->channels, c->frame_size, c->sample_fmt, 0);
    
    // encode a single tone sound
    t = 0;
    tincr = 2 * M_PI * 440.0 / c->sample_rate;
    av_init_packet(&pkt);
    for (i = 0; i < 200; ++i) {
        pkt.data = NULL;
        pkt.size = 0;
        
        for (j = 0; j < c->frame_size; ++j) {
            samples[2*j] = (int)(sin(t) * 10000);
            
            for (k = 1; k < c->channels; ++k) {
                samples[2*j + k] = samples[2*j];
            }
            t += tincr;
        }
        
        //encode the samples which we created
        ret = avcodec_encode_audio2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            LOGE(stderr, "Error:Could not encoding audio frame.\n");
            exit(-1);
        }
        //write data to file
        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }
    
    
    // get the delayed frames
    for (got_output = 1; got_output; ++i) {
        ret = avcodec_encode_audio2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            LOGE(stderr, "Error:Could not encoding audio frame.\n");
            exit(-1);
        }
        //write data to file
        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }
    
    //free resource
    fclose(f);
    
    av_freep(&samples);
    av_frame_free(&frame);
    avcodec_close(c);
    av_free(c);
    
}

/*  对 mp2 格式音频进行解码
 *  大致流程如下
 *  avcodec_find_encoder
 *  avcodec_alloc_context3
 *  avcodec_open2()
 *
 */
static void audio_decode_example(const char *outFileName, const char *fileName)
{
    AVCodec *codec=nullptr;
    AVCodecContext *c = nullptr;
    int len = 0;
    FILE *f = nullptr;
    FILE *outfile = nullptr;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    AVPacket pkt;
    AVFrame *frame = nullptr;
    
    av_init_packet(&pkt);
    
    LOG("Decode audio file %s to %s\n", fileName, outFileName);
    
    //find the mpeg audio decoder
    //codec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!codec) {
        LOGE(stderr, "Codec not found\n");
        exit(1);
    }
    //
    c = avcodec_alloc_context3(codec);
    if (!c) {
        LOGE(stderr, "Error:Could not allocate audio codec context\n");
        exit(1);
    }
    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        LOGE(stderr, "Error:Could not open codec\n");
        exit(-1);
    }
    
    f = fopen(fileName, "rb");
    if (!f) {
        LOGE(stderr, "Error:Could not open %s\n", fileName);
        exit(-1);
    }
    outfile = fopen(outFileName, "wb");
    if (!outfile) {
        av_free(c);
        exit(-1);
    }
    frame = av_frame_alloc();
    if (!frame) {
        LOGE(stderr, "Error:Could not allocate audio frame\n");
        exit(-1);
    }
    
    //decode until eof
    pkt.data = inbuf;
    pkt.size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f);
    while (pkt.size > 0) {
        int i;
        int ch;
        int got_frame = 0;
        
        len = avcodec_decode_audio4(c, frame, &got_frame, &pkt);
        if(len < 0){
            LOGE(stderr, "Error while decoding\n");
            exit(-1);
        }
        
        if (got_frame) {
            // if a frame has been decoded, ouput it
            int data_size = av_get_bytes_per_sample(c->sample_fmt);
            if (data_size < 0) {
                // This should not occuer, checking just for paranoia
                LOGE(stderr, "Error:Failed to calculate data size\n");
                exit(1);
            }
            for (i = 0; i < frame->nb_samples; ++i) {
                for (ch = 0; ch < c->channels; ++ch) {
                    fwrite(frame->data[ch] + data_size*i, 1, data_size, outfile);
                }
            }
            
            pkt.size -= len;
            pkt.data += len;
            pkt.dts = pkt.pts = AV_NOPTS_VALUE;
            if (pkt.size < AUDIO_REFILL_THRESH) {
                memmove(inbuf, pkt.data, pkt.size);
                pkt.data = inbuf;
                len = fread(pkt.data + pkt.size, 1, AUDIO_INBUF_SIZE - pkt.size, f);
                
                if (len > 0) {
                    pkt.size += len;
                }
            }
            
        }
        
    }
    fclose(outfile);
    fclose(f);
    
    avcodec_close(c);
    av_free(c);
    av_frame_free(&frame);
}






int main( int argc, char* argv[] )
{

    const char *output_type;
    
    //register all the codec
    avcodec_register_all();
    
    if( argc < 2 ){
        printf("usage: %s output_type\n"
                "API example program to decode/encode a media stream with libavcodec.\n"
                "This program generates a synthetic stream and encodes it to a file\n"
                "named test.h264, test.mp2 or test.mpg depending on output_type.\n"
                "The encoded stream is then decoded and written to a raw data output.\n"
                "output_type must be chosen between 'h264', 'mp2', 'mpg'.\n",
                argv[0]);
    }
    
    output_type = argv[1];
    
    if (!strcmp(output_type, "h264")) {
        video_encode_example("test.h264", AV_CODEC_ID_H264);
    }
    else if (!strcmp(output_type, "mp2")){
        audio_encode_example("test.mp2");
        audio_decode_example("test.pcm","test.mp2");
    }
    else if(!strcmp(output_type, "mpg")){
        video_encode_example("test.mpg", AV_CODEC_ID_MPEG1VIDEO);
        video_decode_example("test%02d.pgm", "test.mpg");
    }
    else {
        fprintf(stderr, "Invalid output type '%s', choose between 'h264', 'mp2', or 'mpg'\n",
                output_type);
        return 1;
    }
    
    return 0;
}























