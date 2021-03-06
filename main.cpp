#include <iostream>
#include <map>
#include <pthread.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include "libavdevice/avdevice.h"
#include <SDL2/SDL.h>
}
using namespace std;

static AVFormatContext *fmt_ctx = NULL;
static AVCodec *videoCodec = NULL;
static AVCodecContext *codec_ctx_video = NULL;
static int mWidth, mHeight;
static int index_video_stream = -1;
//输入视频流
static int index_audio_stream=-1;
//输入音频流

//SDL
SDL_Rect rect;
Uint32 pixformat;
SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Window *sdl_window;

int init_sdl();

int free_sdl();

int main() {
    cout << "FFmpeg, 解封装" << endl;
    //av_register_all();
    //4.0之后的FFMPEG已弃用，可以不声明
    avformat_network_init();
    //avcodec_register_all();
    //4.0之后的FFMPEG已弃用，可以不声明
    string source_url = "/Users/wubin/Documents/Vscode/untitled/cmake-build-release/test.mp4";

    if (avformat_open_input(&fmt_ctx, source_url.c_str(), NULL, NULL)) { // 打开媒体源，构建AVFormatContext
        cerr << "could not open source file:" << source_url << endl;
        exit(1);
    }

    // 找到所有流,初始化一些基本参数
    index_video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    index_audio_stream= av_find_best_stream(fmt_ctx,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    mWidth = fmt_ctx->streams[index_video_stream]->codecpar->width;
    mHeight = fmt_ctx->streams[index_video_stream]->codecpar->height;

    // 找到并打开解码器
    videoCodec = avcodec_find_decoder(fmt_ctx->streams[index_video_stream]->codecpar->codec_id);
    codec_ctx_video = avcodec_alloc_context3(videoCodec); // 根据解码器类型分配解码器上下文内存
    avcodec_parameters_to_context(codec_ctx_video, fmt_ctx->streams[index_video_stream]->codecpar); // 拷贝参数
    codec_ctx_video->thread_count = 8; // 解码线程数量
    cout<<fmt_ctx->duration/1000000/60<<"分"<<fmt_ctx->duration/1000000%60<<"秒"<<endl;
    //输出视频总时长
    cout<<"视频编码："<<avcodec_get_name(fmt_ctx->streams[index_video_stream]->codecpar->codec_id)<<endl;
    //输出视频编码
    cout<<"音频编码："<<avcodec_get_name(fmt_ctx->streams[index_audio_stream]->codecpar->codec_id)<<endl;
    //输出音频编码
    cout<<"音频通道数："<<fmt_ctx->streams[index_audio_stream]->codecpar->channels<<endl;
    cout << "thread_count = " << codec_ctx_video->thread_count << endl;
    for(unsigned i=0;i<fmt_ctx->nb_streams;i++){
        cout<<i<<endl;
        cout<<av_get_media_type_string(fmt_ctx->streams[i]->codec->codec_type)<<endl;
    }

    if ((avcodec_open2(codec_ctx_video, videoCodec, NULL)) < 0) {
        cout << "cannot open specified audio codec" << endl;
    }

    init_sdl();
//
    //分配解码后的数据存储位置
    AVPacket *avPacket = av_packet_alloc();
    AVFrame *avFrame = av_frame_alloc();





    // 读帧
    while (true) {
        int result = av_read_frame(fmt_ctx, avPacket);
        if (result < 0) {
            cout << "文件结束" << endl;
//            avcodec_send_packet(codec_ctx_video, NULL); // TODO有一个最后几帧收不到的问题需要这段代码调用解决
            break;
        }

        if (avPacket->stream_index != index_video_stream) {
            // 目前只显示视频数据,监测到非视频流则抛弃
            av_packet_unref(avPacket);
            // 注意清理，容易造成内存泄漏
            cout<<"丢弃音频流"<<endl;
            continue;
        }
        // 发送到解码器线程解码， avPacket会被复制一份，所以avPacket可以直接清理掉
        result = avcodec_send_packet(codec_ctx_video, avPacket);
        av_packet_unref(avPacket); // 注意清理，容易造成内存泄漏

        if (result != 0) {
            cout << "av_packet_unref failed" << endl;
            continue;
        }
        while (avcodec_receive_frame(codec_ctx_video, avFrame) == 0) {

            SDL_UpdateYUVTexture(texture, NULL,
                                 avFrame->data[0], avFrame->linesize[0],
                                 avFrame->data[1], avFrame->linesize[1],
                                 avFrame->data[2], avFrame->linesize[2]);

            // Set Size of Window
            rect.x = 0;
            rect.y = 0;
            rect.w = mWidth;
            rect.h = mHeight;

            //展示
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_RenderPresent(renderer);
            SDL_Delay(1000/30); // 防止显示过快，如果要实现倍速播放，只需要调整delay时间就可以了。

        }

        SDL_Event event;
        SDL_PollEvent(&event);
        if (event.type==SDL_QUIT) {
            return 0;
        }
        if (event.type==SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_SPACE) { //空格键暂停
                cout<<"space"<<endl;
                SDL_RenderPresent(renderer);

            }
        }

    }

    free_sdl();
    return 0;
}
int init_sdl() {
    // 初始化sdl
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    sdl_window = SDL_CreateWindow("Media Player",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  mWidth, mHeight,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    //创建SDL窗口
    if (!sdl_window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL");
        return -1;
    }
    //创建渲染器
    renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
        return -1;
    }

    pixformat = SDL_PIXELFORMAT_IYUV;//YUV格式
    // 创建纹理
    texture = SDL_CreateTexture(renderer,
                                pixformat,
                                SDL_TEXTUREACCESS_STREAMING,
                                mWidth,
                                mHeight);
}

int free_sdl() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 0;
}
