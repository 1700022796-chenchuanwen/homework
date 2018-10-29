#include <gtk/gtk.h>
#include <stdio.h>



//指定显示分辨率
#define dest_width 320
#define dest_height 240


GtkWidget *window;
GtkWidget *vbox;   //布局方式


void sig_fun(GtkWidget *win)
{
	g_signal_connect_swapped(G_OBJECT(win),"destroy" ,G_CALLBACK(gtk_main_quit),NULL);
}
void show_pic(gpointer argv)
{
	GtkWidget *win,*img;


	win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	img=gtk_image_new_from_file(argv);
	gtk_window_set_default_size(GTK_WINDOW(win),880,600);
	gtk_widget_set_size_request(img,800,600);
	gtk_window_set_title(GTK_WINDOW(win),argv);
	gtk_container_add(GTK_CONTAINER(win),img);
	gtk_widget_show_all(win);
	sig_fun(win);

}


void show_pic2(GtkWidget w, gpointer argv)
{

	GtkWidget *win,*img;
	win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	img=gtk_image_new_from_file(argv);
	gtk_window_set_default_size(GTK_WINDOW(win),880,600);
	gtk_widget_set_size_request(img,800,600);
	gtk_window_set_title(GTK_WINDOW(win),argv);
	gtk_container_add(GTK_CONTAINER(win),img);
	gtk_widget_show_all(win);
	sig_fun(win);

}


void show_image(GtkWidget* win, gpointer filename){
	GtkWidget *frame;
	GtkWidget *fixed;
	GtkWidget *img,*image;

    const GdkPixbuf *src_pixbuf;
    GdkPixbuf *dest_pixbuf;

//	char* filename ="G:/work/homework/imageProcess/image/test.jpg";
	img =gtk_image_new_from_file(filename);
	gtk_widget_set_size_request(img,50,50);



	 //读取图片参数
	src_pixbuf = gdk_pixbuf_new_from_file(filename, NULL);


	//将src_pixbuf设置成屏幕大小
	dest_pixbuf = gdk_pixbuf_scale_simple(src_pixbuf,
		dest_width, dest_height, GDK_INTERP_HYPER);

	//从dest_pixbuf中读取图片存于image中
	image = gtk_image_new_from_pixbuf(dest_pixbuf);


//创建一个无标签框架
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_OUT);

	//创建一个固定窗口并设置窗口大小，返回固定布局容器指针
	fixed = gtk_fixed_new();
	//
//	gtk_fixed_set_has_window(GTK_FIXED(fixed),TRUE);
	//设置固定窗口大小
	gtk_widget_set_size_request(fixed,100,100);
	//设置图片及显示位置

	gtk_fixed_put(GTK_FIXED(fixed),image,20,20);
	//加入框架中
	gtk_container_add(GTK_CONTAINER(frame),fixed);
	//框架加入盒子中
	gtk_box_pack_start(GTK_BOX(vbox),frame,TRUE,TRUE,5);

	gtk_widget_show_all(window);
}

void openfile(GtkWidget * widget, gpointer *win){
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	dialog = gtk_file_chooser_dialog_new ("Open File",
									      (GtkWindow*)window,
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
	    show_image(window, filename);
	    g_free (filename);
	  }

	gtk_widget_destroy (dialog);
}




int main(int argc, char *argv[])
{
	gtk_init(&argc,&argv);
//	show_pic("G:/work/homework/imageProcess/image/test.jpg");
//	show_pic(argv[1]);


	GtkWidget *menubar;   //菜单栏
	GtkWidget *filemenu;  //菜单
	GtkWidget *file;      //容器
	GtkWidget *quit;      //退出菜单条
	GtkWidget *open;
	GtkWidget *help;


	gtk_init(&argc,&argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window),500,300);
	gtk_window_set_title(GTK_WINDOW(window),"menu");

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	gtk_container_add(GTK_CONTAINER(window),vbox);

	menubar = gtk_menu_bar_new();
	filemenu = gtk_menu_new();

	file = gtk_menu_item_new_with_label("File");
	open= gtk_menu_item_new_with_label("Open");
	quit = gtk_menu_item_new_with_label("Quit");
	help = gtk_menu_item_new_with_label("Help");


	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file),filemenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(filemenu),open);
	gtk_menu_shell_append(GTK_MENU_SHELL(filemenu),quit);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),file);
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar),help);

	gtk_box_pack_start(GTK_BOX(vbox),menubar,FALSE,FALSE,3);

	g_signal_connect_swapped(G_OBJECT(window),"destroy",G_CALLBACK(gtk_main_quit),NULL);
	g_signal_connect(G_OBJECT(quit),"activate",G_CALLBACK(gtk_main_quit),NULL);


	g_signal_connect(G_OBJECT(open),"activate",G_CALLBACK(openfile), window);

	gtk_widget_show_all(window);



	gtk_main();
	return 0;
}

