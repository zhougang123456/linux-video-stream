#include <QCoreApplication>
#include <Xlib.h>
#include <extensions/Xfixes.h>
#include <extensions/Xdamage.h>
#include "video-stream.hpp"
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "image-encoders.hpp"

typedef unsigned char  BYTE;
typedef unsigned short	WORD;
typedef unsigned int  DWORD;

#define PACKED __attribute__(( packed, aligned(2)))

typedef struct tagBITMAPFILEHEADER{
     WORD     bfType;        //Linux此值为固定值，0x4d42
     DWORD    bfSize;        //BMP文件的大小，包含三部分
     WORD     bfReserved1;    //置0
     WORD     bfReserved2;
     DWORD    bfOffBits;     //文件起始位置到图像像素数据的字节偏移量

}PACKED BITMAPFILEHEADER;


typedef struct tagBITMAPINFOHEADER{
     DWORD    biSize;          //文件信息头的大小，40
     DWORD    biWidth;         //图像宽度
     DWORD    biHeight;        //图像高度
     WORD     biPlanes;        //BMP存储RGB数据，总为1
     WORD     biBitCount;      //图像像素位数，笔者RGB位数使用24
     DWORD    biCompression;   //压缩 0：不压缩  1：RLE8 2：RLE4
     DWORD    biSizeImage;     //4字节对齐的图像数据大小
     DWORD    biXPelsPerMeter; //水平分辨率  像素/米
     DWORD    biYPelsPerMeter;  //垂直分辨率  像素/米
     DWORD    biClrUsed;        //实际使用的调色板索引数，0：使用所有的调色板索引
     DWORD    biClrImportant;
}BITMAPINFOHEADER;

static void dump_bmp(char* buffer, int width, int height)
{
    static int id = 0;
    char file[200];
//    if (id > 50){
//        return;
//    }
    printf("creating bmp!\n");
    sprintf(file, "/home/zhougang/zhougang/x11-screenshot/%d.bmp", id++);

    FILE* f = fopen(file, "wb");
    if (!f) {
       printf("Error creating bmp!\n");
       return;
    }

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = 0;
    bi.biSizeImage = width*height*4;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    BITMAPFILEHEADER bf;
    bf.bfType = 0x4d42;
    bf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bi.biSizeImage;
    bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;

    fwrite(&bf,sizeof(BITMAPFILEHEADER),1,f);                      //写入文件头
    fwrite(&bi,sizeof(BITMAPINFOHEADER),1,f);                      //写入信息头
    fwrite(buffer,bi.biSizeImage,1,f);

    printf("width : %d height: %d\n", width, height);
    fclose(f);

}

static inline int get_time(void)
{
    timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    return (int)now.tv_sec * 1000 + (int)now.tv_nsec / 1000000;

}

//#define DUMP_JPEG
#ifdef DUMP_JPEG
static int jpeg_id = 0;
static void dump_jpeg(RedCompressBuf* buf, int data_size)
{
    char file_str[200];
    uint32_t id = ++jpeg_id;

    sprintf(file_str, "/tmp/tmpfs/%u.jpg", id);
    FILE* f;
    f = fopen(file_str, "wb");
    if (!f) {
        return;
    }
    size_t max = data_size;
    size_t now;
    do {
        if (buf == NULL) {
            break;
        }
        now = MIN(sizeof(buf->buf), max);
        max -= now;
        fwrite(buf->buf.bytes, 1, now, f);
        buf = buf->send_next;
    } while (max);

    fclose(f);
}
#endif

#define MIN_GLZ_WINDOW_SIZE_DEFAULT (1024 * 1024 * 12)

int main(int argc, char *argv[])
{

    Display * display = XOpenDisplay(":0.0");
    printf("server vender %s \n", ServerVendor(display));
    int screen = XDefaultScreen(display);
    Window win = RootWindow(display, screen);
    XWindowAttributes win_info;
    XGetWindowAttributes(display, win, &win_info);
    printf("width: %d height: %d\n", win_info.width, win_info.height);

    //XSelectInput(display, win, StructureNotifyMask);
    XSelectInput(display, win, ExposureMask | KeyPressMask |
                        ButtonPressMask | Button1MotionMask |
                Button2MotionMask | StructureNotifyMask);
    int error_base;
    int xfixes_event_base;
    if (!XFixesQueryExtension(display, &xfixes_event_base, &error_base)) {
       printf("XFixesQueryExtension failed\n");
       return 0;
    }
    XFixesSelectCursorInput(display, DefaultRootWindow(display), XFixesDisplayCursorNotifyMask);


    int xdamage_event_base;
    if (!XDamageQueryExtension(display, &xdamage_event_base, &error_base)) {
        printf("XDamageQueryExtension failed!\n");
        return 0;
    }
    Damage damage = XDamageCreate(display, win, XDamageReportNonEmpty);
    VideoStream* stream = new VideoStream();
    ImageEncoders* encoder = new ImageEncoders();
    ImageEncoderSharedData shared;
    encoder->image_encoders_init(&shared);
    encoder->image_encoders_get_glz_dictionary(0, MIN_GLZ_WINDOW_SIZE_DEFAULT / 4);
    encoder->image_encoders_glz_create(0);
    XEvent event;
    Window root, child;
    int r_x,r_y, c_x, c_y; unsigned int mask;
    while (1){

        int time = get_time();
        printf("tim %d\n", time);
        stream->Stream_Timeout(time);
        if (stream->Is_StreamStart()){
            //printf("stream start\n");
        } else {
            //printf("stream stop\n");
        }
        //printf("move x %d y %d\n", r_x, r_y);
        while (XPending(display)){
        XNextEvent(display, &event);
        printf("xdamge event %d\n", event.type);
        if (event.type == xfixes_event_base + 1) {
            XFixesCursorImage *cursor = XFixesGetCursorImage(display);
            //printf("cursor width: %d height: %d\n", cursor->width, cursor->height);
        } else if (event.type == ConfigureNotify){
            XConfigureEvent *config = &event.xconfigure;
            printf("win width: %d height: %d\n", config->width, config->height);
        } else if (event.type == xdamage_event_base + XDamageNotify){
            XDamageNotifyEvent *damage_event = reinterpret_cast<XDamageNotifyEvent*> (&event);
            auto region = XFixesCreateRegion(display, NULL, 0);
            XDamageSubtract(display, damage_event->damage, None, region);
            int count;
            XRectangle *area = XFixesFetchRegion(display, region, &count);
            //printf("rect count %d\n", count);
            if(area){
                for(int i = 0; i < count; i++){

                    //printf("win width: %u height: %u\n", area[i].width, area[i].height );
                    Rect rect;
                    rect.left = area[i].x;
                    rect.right = area[i].x + area[i].width;
                    rect.top = area[i].y;
                    rect.bottom = area[i].y + area[i].height;
                    stream->Add_Stream(&rect, time);

                    XImage *image = XGetImage(display, damage_event->drawable, area[i].x, area[i].y,
                                          area[i].width, area[i].height, AllPlanes, ZPixmap);

                                //dump_bmp(image->data, image->width, image->height);
//                    unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char) * image->width * image->height *4);
//                    memcpy(data, image->data, image->width * image->height * 4);
//                    SpiceRect spice_rect;
//                    spice_rect.left = rect.left; spice_rect.right = rect.right;
//                    spice_rect.top = rect.top; spice_rect.bottom = rect.bottom;
//                    RedDrawable *red_drawable = (RedDrawable *)malloc(sizeof(RedDrawable));
//                    memset(red_drawable, 0, sizeof(RedDrawable));
//                    red_drawable_get(red_drawable, &spice_rect, data, 0);
//                    SpiceImage* simage = red_drawable->u.copy.src_bitmap;
//                    SpiceBitmap* src_bitmap = &simage->u.bitmap;
//                    SpiceImage* dest_image = (SpiceImage *)malloc(sizeof(SpiceImage));
//                    dest_image->descriptor = simage->descriptor;
//                    dest_image->descriptor.flags = 0;
//                    compress_send_data_t comp_send_data = { 0 };
//                    encoder->image_encoders_compress_glz(dest_image, src_bitmap, red_drawable, NULL, &comp_send_data);
//                    //encoder->image_encoders_compress_jpeg(dest_image, src_bitmap, &comp_send_data);
//        #ifdef DUMP_JPEG
//                    dump_jpeg(comp_send_data.comp_buf, comp_send_data.comp_buf_size);
//        #endif

//                    printf("glz size %d\n", comp_send_data.comp_buf_size);

//                    red_drawable_unref(red_drawable);

//                    RedCompressBuf* buf = comp_send_data.comp_buf;
//                    RedCompressBuf* next = buf;
//                    while (next) {
//                        next = buf->send_next;
//                        free(buf);
//                        buf = next;
//                    }

//                    free(dest_image);
                    image->f.destroy_image(image);

                }
                XFree(area);
            }
            XFixesDestroyRegion(display, region);

        } else if  (event.type == MotionNotify) {
             XQueryPointer(display, win, &root, &child, &r_x, &r_y, &c_x, &c_y, &mask);
             //printf("move x %d y %d\n", r_x, r_y);
        }
        }
        usleep(30000);
    }

    XDamageDestroy(display, damage);
    return 0;
}
