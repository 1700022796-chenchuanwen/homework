
#include <gtk/gtk.h>
#include <stdio.h>
#include "common.h"
#include "bitstream.h"
#include "encode.h"
#include "decode.h"
#include "library.h"

//指定显示分辨率
#define dest_width 320
#define dest_height 241


GtkWidget *window;
GtkWidget *dialog;
GtkWidget *vbox;   //布局方式

int g_argc=0;
char **g_argv;
struct options options;

GtkWidget *frame, *frame2;
GtkWidget *fixed, *fixed2;
GtkWidget *img,*img2, *image, *image2;

GdkPixbuf *src_pixbuf, *src_pixbuf2;
GdkPixbuf *dest_pixbuf, *dest_pixbuf2;

static char*  g_filename;



