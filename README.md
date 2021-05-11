# Qt With FFMPEG

尝试实现一个最简单的播放器，基于Qt5和ffmpeg这两个库。

这个文档主要记录编写过程遇到的一些问题和记录一下学习过程。

> env：
>
> Qt == 5.12.10
>
> FFmpeg == 4.4
>
> Compiler == g++
>
> System == Ubuntu

[TOC]

## 库的链接顺序

这个是个坑爹的问题，我被这个卡了半天。这个版本的FFmpeg我找到的所有博客或者问答中**没有找到一个是对的**，这个好像不同版本FFmpeg不同的链接顺序，可以根据自己情况适当穷举链接顺序。

在我的环境下主要是

```makefile
# pro file config
INCLUDEPATH += ./include
DEPENDPATH += ./include

LIBS += -L$$PWD/lib -lavformat -lavcodec -lavutil -lswscale -lavdevice -lavfilter -lswresample
```

## 解码流程和思想

这里感谢[雷神](https://blog.csdn.net/leixiaohua1020?type=blog)的无私分享，虽然教程内很多api到现在这个版本已经被弃用或者被修改了，但是仍旧帮我解决了很多问题。提供了一个具体了解思路。

对于FFmpeg解码方式，雷神有[FFmpeg源代码结构图 - 解码](https://blog.csdn.net/leixiaohua1020/article/details/44220151)

这篇博客解释了FFmpeg的一个大体解码流程，有一个比较系统性的了解。

另外就是一些关键性的结构体，例如：**AVFrame, AVFormatContext, AVCodecContext, AVIOContext, AVCodec, AVStream, AVPacket**等等，这些非常关键的结构体，这里通过雷神[FFMPEG中最关键的结构体之间的关系](https://blog.csdn.net/leixiaohua1020/article/details/11693997)博客有进行解释。

但是到具体编码环节的时候，雷神的博客完成时间比较早，FFmepg很多东西都发生了变动，通过参考

[ffmpeg-libav-tutorial](https://github.com/leandromoreira/ffmpeg-libav-tutorial/)

很好一个入门教程，跟随这篇教程，我完成了首次的音视频解码

### 解码流程API

#### avformat_open_input

```c
/**
 * Open an input stream and read the header. The codecs are not opened.
 * The stream must be closed with avformat_close_input().
 *
 * @param ps Pointer to user-supplied AVFormatContext (allocated by avformat_alloc_context).
 *           May be a pointer to NULL, in which case an AVFormatContext is allocated by this
 *           function and written into ps.
 *           Note that a user-supplied AVFormatContext will be freed on failure.
 * @param url URL of the stream to open.
 * @param fmt If non-NULL, this parameter forces a specific input format.
 *            Otherwise the format is autodetected.
 * @param options  A dictionary filled with AVFormatContext and demuxer-private options.
 *                 On return this parameter will be destroyed and replaced with a dict containing
 *                 options that were not found. May be NULL.
 *
 * @return 0 on success, a negative AVERROR on failure.
 *
 * @note If you want to use custom IO, preallocate the format context and set its pb field.
 */
int avformat_open_input(AVFormatContext **ps, const char *url,
                        const AVInputFormat *fmt, AVDictionary **options);
```

这个函数是FFmpeg提供用来打开文件数据流，其中

```c
ps：这个是一个AVFormatContext指针，需要注意的是，这个值需要AVFormatContext *pFormatContext = NULL;
url：该值就是文件路径
剩余两个参数暂时给NULL即可
return：如果文件打开成功，并且文件是符合音视频格式的，那么将会返回0，同时，*ps将会被指向一块分配好的内存，存储了音视频文件封装格式；如果小于0，那么就是文件有错误，且*ps将会被赋值NULL。
```

#### avformat_close_input

```c
/**
 * Close an opened input AVFormatContext. Free it and all its contents
 * and set *s to NULL.
 */
void avformat_close_input(AVFormatContext **s);
```

上一个API是打开音视频文件，并获取**AVFormatContext**封装格式，那么同时就需要关闭和相关内存的释放，这个和**avformat_open_input**是必须成对使用的。

#### avformat_find_stream_info

```c
/**
 * Read packets of a media file to get stream information. This
 * is useful for file formats with no headers such as MPEG. This
 * function also computes the real framerate in case of MPEG-2 repeat
 * frame mode.
 * The logical file position is not changed by this function;
 * examined packets may be buffered for later processing.
 *
 * @param ic media file handle
 * @param options  If non-NULL, an ic.nb_streams long array of pointers to
 *                 dictionaries, where i-th member contains options for
 *                 codec corresponding to i-th stream.
 *                 On return each dictionary will be filled with options that were not found.
 * @return >=0 if OK, AVERROR_xxx on error
 *
 * @note this function isn't guaranteed to open all the codecs, so
 *       options being non-empty at return is a perfectly normal behavior.
 *
 * @todo Let the user decide somehow what information is needed so that
 *       we do not waste time getting stuff the user does not need.
 */
int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);
```

在获取到封装格式后，需要分析封装中存储了哪些数据流，以及解析出这些数据流的信息

```c
ic：就是上面获得的封装格式
options：这里可以先给NULL
return：如果是 >=0 的数，那么就是查找数据流成功，小于0就是封装格式有问题
```

#### av_dump_format

```c
/**
 * Print detailed information about the input or output format, such as
 * duration, bitrate, streams, container, programs, metadata, side data,
 * codec and time base.
 *
 * @param ic        the context to analyze
 * @param index     index of the stream to dump information about
 * @param url       the URL to print, such as source or destination file
 * @param is_output Select whether the specified context is an input(0) or output(1)
 */
void av_dump_format(AVFormatContext *ic,
                    int index,
                    const char *url,
                    int is_output);
```

FFmpeg支持将刚刚查找到的封装格式打印出来

调用方式非常简单：

```c++
char outputBuff[4096] = {0};
av_dump_format(pFormatContext, 0, outputBuff, 0);
std::cout << outputBuff;
```

