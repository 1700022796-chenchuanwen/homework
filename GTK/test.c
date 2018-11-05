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

//关闭窗口
void sig_fun(GtkWidget *win)
{
	g_signal_connect_swapped(G_OBJECT(win),"destroy" ,G_CALLBACK(gtk_main_quit),NULL);
}

//显示图片控件
void show_image(GtkWidget* win, char* filename){
	g_argv[1] = g_filename;
    g_argc=2;

  //	char* filename ="G:/work/homework/imageProcess/image/test.jpg";
	img =gtk_image_new_from_file(filename);
	gtk_widget_set_size_request(img,50,50);

	 //读取图片参数
	src_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

	//将src_pixbuf设置成屏幕大小
//	dest_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf,
//		dest_width, dest_height, GDK_INTERP_HYPER);

	//从dest_pixbuf中读取图片存于image中
	image = gtk_image_new_from_pixbuf(src_pixbuf);

	//设置图片及显示位置
	gtk_fixed_put(GTK_FIXED(fixed),image,20,20);

	gtk_widget_show_all(window);
	gtk_widget_show_now(window);

	trace("show image filename:%s\n", g_argv[1]);

}

void show_image2(GtkWidget* win, gpointer filename){
  //	char* filename ="G:/work/homework/imageProcess/image/test.jpg";

	img2 =gtk_image_new_from_file(filename);
	gtk_widget_set_size_request(img2,50,50);

	 //读取图片参数
	src_pixbuf2 = gdk_pixbuf_new_from_file(filename, NULL);

	//将src_pixbuf设置成屏幕大小
//	dest_pixbuf2 = gdk_pixbuf_scale_simple(src_pixbuf2,
//		dest_width, dest_height, GDK_INTERP_HYPER);

	//从dest_pixbuf中读取图片存于image中
	image2 = gtk_image_new_from_pixbuf(src_pixbuf2);
	gtk_fixed_put(GTK_FIXED(fixed2),image2,20,20);

	gtk_widget_show_now(fixed2);
	gtk_widget_show_all(window);
}

void openfile(GtkWidget * widget, gpointer *win){

	trace("openfile ............\n");

	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	dialog = gtk_file_chooser_dialog_new ("Open File",
									      NULL,
	                                      action,
	                                      "_Cancel",
	                                      GTK_RESPONSE_CANCEL,
	                                      "_Open",
	                                      GTK_RESPONSE_ACCEPT,
	                                      NULL);

	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	  {
	    char *filename;
	    GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
	    filename = gtk_file_chooser_get_filename (chooser);
	    if(g_filename){
	    	free(g_filename);
	    }

    	g_filename = malloc(sizeof(filename)+1);
       	strcpy(g_filename, filename);


	    show_image(window, g_filename);
	    g_free (filename);


	  }

	gtk_widget_destroy (dialog);

}

void encodeImage(GtkWidget * widget, int task){
	g_task = task;

    trace("encoding Image filename:%s\n", g_argv[1]);

	g_argv[2] ="-o";
	g_output = g_argv[3] = "task10.jpg";
	g_argc=4;

	bool error = false;

	char *l_argv[g_argc];

	trace("l_argv size:%d\n",g_argc);

	for(int i =0;i<g_argc;i++){
		l_argv[i] = malloc(sizeof(g_argv[i])+1);
		strcpy(l_argv[i],g_argv[i]);
		trace("l_argv %d----> %s\n", i,l_argv[i]);
	}



	/* Parse arguments and exit on failure */
	if (parse_args(g_argc, l_argv, &options)){
			trace("parse args failed:%d",EXIT_FAILURE);
			return ;
	}


	int ret = EXIT_SUCCESS;

    /* JPEG Encoding */
     if (options.encode) {
             struct bitstream *stream = create_bitstream(options.output, WRONLY);

             if (stream != NULL) {

                     /* Initializes the jpeg structure with 0s */
                     struct jpeg_data jpeg;
                     memset(&jpeg, 0, sizeof(jpeg));

                     /* Retrieve options */
                     jpeg.path = options.input;
                     jpeg.compression = options.compression;
                     jpeg.mcu.h = options.mcu_h;
                     jpeg.mcu.v = options.mcu_v;


                     /* Read input image */
                     read_image(&jpeg, &error);


                     /* Enable specific options */
                     process_options(&options, &jpeg, &error);



                     /* Zoom input image  add by chuanwen 181015*/
                     if(g_task == ZOOM){
                    	 trace(">> zoom image begin ...");
                    	 zoom_image(&jpeg, &error);
                     }

                     /* Compute Huffman tables */
                     compute_jpeg(&jpeg, &error);

                     /* Free raw image data */
                     SAFE_FREE(jpeg.raw_data);


                     /* Write JPEG header */
                     write_header(stream, &jpeg, &error);

                     /* Write computed JPEG data */
                     write_blocks(stream, &jpeg, &error);

                     /* End JPEG file */
                     write_section(stream, EOI, NULL, &error);

                     /* Close output file */
                     free_bitstream(stream);


                     if (error) {
                             printf("JPEG compression failed\n");
                             ret = EXIT_FAILURE;

                             /* Remove the invalid created file */
                             remove(options.output);
                     }

                     else
                             printf("JPEG successfully encoded\n");


                     /* Free compressed JPEG data */
                     SAFE_FREE(jpeg.mcu_data);

                     /* Free JPEG Huffman trees */
                     free_jpeg_data(&jpeg);
             }

     /* JPEG Decoding / TIFF Reencoding */
     } else {

             /* Initializes the jpeg structure with 0s */
             struct jpeg_data jpeg;
             memset(&jpeg, 0, sizeof(struct jpeg_data));

             /* Retrieve options */
             jpeg.path = options.input;
             jpeg.mcu.h = options.mcu_h;
             jpeg.mcu.v = options.mcu_v;

             /* Read input image */
             read_image(&jpeg, &error);

             /* Enable specific options */
             process_options(&options, &jpeg, &error);


             /* Specify output path */
             jpeg.path = options.output;

             /* Export as TIFF file */
             export_tiff(&jpeg, &error);

             /* Free raw image data */
             SAFE_FREE(jpeg.raw_data);


             if (error)
                     printf("ERROR : unsupported input format\n");

             else
                     printf("Input file successfully decoded\n");


             /* Free any allocated JPEG data */
             free_jpeg_data(&jpeg);
     }

     trace("encode result:%d\n", ret);
     return;
}


void reverseImage(GtkWidget * widget, gpointer *task){
	trace("reversing image...%s, gfilename:%s\n", g_argv[1], g_filename);
	encodeImage(widget, REVERSE);
	show_image2(window,g_output);
}

void zoomImage(GtkWidget * widget, gpointer *task){
	trace("zooming image...\n");
	encodeImage(widget, ZOOM);
	show_image2(window,g_output);
}

void balanceImage(GtkWidget * widget, gpointer *task){
	trace("balancing image...\n");
	encodeImage(widget, BALANCE);
	show_image2(window,g_output);
}

void powImage(GtkWidget * widget, gpointer *task){
	trace("powing image...\n");
	encodeImage(widget, POW);
	show_image2(window,g_output);
}

int main(int argc, char *argv[])
{
	g_argc = argc;
	g_argv = argv;

	gtk_init(&argc,&argv);
//	show_pic("G:/work/homework/imageProcess/image/test.jpg");
//	show_pic(argv[1]);


	GtkWidget *menubar;   //菜单栏
	GtkWidget *filemenu;  //菜单
	GtkWidget *file;      //容器
	GtkWidget *quit;      //退出菜单条
	GtkWidget *open;
	GtkWidget *menu_reverse, *menu_balance, *menu_zoom,*menu_pow;


	//初始化
	gtk_init(&argc,&argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window),600,400);
	gtk_window_set_title(GTK_WINDOW(window),"menu");

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	gtk_container_add(GTK_CONTAINER(window),vbox);

	//菜单---->>
	menubar = gtk_menu_bar_new();
	filemenu = gtk_menu_new();

	file = gtk_menu_item_new_with_label("文件");
	open= gtk_menu_item_new_with_label("打开");
	quit = gtk_menu_item_new_with_label("退出");
	menu_reverse = gtk_menu_item_new_with_label("图像取反运算");
	menu_balance = gtk_menu_item_new_with_label("直方图均衡化");
	menu_zoom = gtk_menu_item_new_with_label("缩放");
	menu_pow = gtk_menu_item_new_with_label("幂运算");

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file),filemenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(filemenu),open);
	gtk_menu_shell_append(GTK_MENU_SHELL(filemenu),quit);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),file);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),menu_reverse);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),menu_balance);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),menu_zoom);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),menu_pow);

	gtk_box_pack_start(GTK_BOX(vbox),menubar,FALSE,FALSE,3);

	g_signal_connect_swapped(G_OBJECT(window),"destroy",G_CALLBACK(gtk_main_quit),NULL);
	g_signal_connect(G_OBJECT(quit),"activate",G_CALLBACK(gtk_main_quit),NULL);
	g_signal_connect(G_OBJECT(menu_reverse),"button_press_event",G_CALLBACK(reverseImage),NULL);
	g_signal_connect(G_OBJECT(menu_balance),"button_press_event",G_CALLBACK(balanceImage),NULL);
	g_signal_connect(G_OBJECT(menu_zoom),"button_press_event",G_CALLBACK(zoomImage),NULL);
	g_signal_connect(G_OBJECT(menu_pow),"button_press_event",G_CALLBACK(powImage),NULL);

	g_signal_connect(G_OBJECT(open),"activate",G_CALLBACK(openfile), window);
	//菜单----<<

	//创建一个无标签框架
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_OUT);
	gtk_frame_set_label(GTK_FRAME(frame), "原图");
	frame2 = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame2),GTK_SHADOW_ETCHED_OUT);
	gtk_frame_set_label(GTK_FRAME(frame2), "变换后...");

	//框架加入盒子中
	gtk_box_pack_start(GTK_BOX(vbox),frame,TRUE,TRUE,5);
	gtk_box_pack_start(GTK_BOX(vbox),frame2,TRUE,TRUE,5);

	//创建一个固定窗口并设置窗口大小，返回固定布局容器指针
	fixed = gtk_fixed_new();
	fixed2 = gtk_fixed_new();
   //	gtk_fixed_set_has_window(GTK_FIXED(fixed),TRUE);
	//设置固定窗口大小
	gtk_widget_set_size_request(fixed,100,100);
	gtk_widget_set_size_request(fixed2,100,100);

	//加入框架中
	gtk_container_add(GTK_CONTAINER(frame),fixed);
	gtk_container_add(GTK_CONTAINER(frame2),fixed2);

	gtk_widget_show_all(window);



	gtk_main();
	return 0;
}
