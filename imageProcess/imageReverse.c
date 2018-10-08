#include "stdio.h"
#include "image.h"

void exitmessage(char *msg){
    printf("%s", msg);
    exit(0);
}

char* tinyjpeg_get_errorstring(struct jdec_private *priv){
    return "error";
}

int  be16_to_cpu(){
        return 1; 
}

void build_huffman_table(unsigned char* bits, unsigned char* vals, struct huffman_table *table){
    
}


void build_default_huffman_tables(struct jdec_private priv){
    if( (priv.flags & TINYJPG_FLAGS_MJPEG_TABLE) && priv.default_huffman_table_initialized){
        return;
    }

    struct unknown bits_dc_luminance;
    struct unknown bits_dc_chrominance;

    struct unknown val_dc_lumiance;
    struct unknown val_ac_lumiance;

    struct unknown val_dc_chromiance;
    struct unknown val_ac_chromiance;


    build_huffman_table(bits_dc_luminance.ptr, val_dc_lumiance.ptr, &priv.HTDC[0]);
    build_huffman_table(bits_dc_luminance.ptr, val_ac_lumiance.ptr, &priv.HTAC[0]);

    build_huffman_table(bits_dc_chrominance.ptr, val_dc_chromiance.ptr, &priv.HTDC[1]);
    build_huffman_table(bits_dc_chrominance.ptr, val_ac_chromiance.ptr, &priv.HTAC[1]);

    priv.default_huffman_table_initialized =1;
}


int parse_SOF(struct jdec_private  *priv, const unsigned char *stream){
    return -1;
}

int parse_JFIF(struct jdec_private  *priv, const unsigned char *stream){
    int chuck_len;
    int marker;
    int sos_marker_found = 0;
    int dnt_marker_found = 0;
    const unsigned char *next_chunck;
    char* param_trace;

    while(!sos_marker_found){
        if(*stream++ !=0xff){
            goto bogus_jpeg_format;
        }
        while(*stream ==0xff){
            stream++;
        }
        marker = *stream++;

        chuck_len = be16_to_cpu(stream);
       next_chunck = stream +chuck_len;

       switch(marker){
           case SOF:
               if(parse_SOF(priv, stream)<0){
                   return -1;
               }
            break;
           case DQT:
            break;
           case SOS:
            break;
           case DHT:
            break;
           default:
#if TRACE_PARAM
// fprintf(param_trace, "> Unknown marker %2.2x\n", marker);
  //          fflush(param_trace);
#endif
            break;
       }; 

       //解下一个segment
       stream  = next_chunck;
    }

    if(!dnt_marker_found){
#if TRACE_PARAM
        fprintf(param_trace, "No Huffman table loaded, using the default one\n");
        fflush(param_trace);
#endif
        build_default_huffman_tables(priv);

        return 0;

bogus_jpeg_format:
        //fprintf(param_trace, "Bogus jpeg format\n");
       // fflush(param_trace);
        return -1;
    }


}



int tinyjpeg_parse_header(struct jdec_private *priv, const unsigned char *buf, unsigned int size){
    int ret;
    char* error_string;

    //是否是JPEG文件
    if((buf[0]!=0xFF) || (buf[1] !=SOI)){
        snprintf(error_string, sizeof(error_string),"Not a JPG file ?\n");
    }
    //char temp_str[MAX_URL_LENGTH];
    //sprintf(temp_str, "0x %X %X", buf[0], buf[1]);

    priv->stream_begin = buf+2;
    priv->stream_length = size -2;
    priv->stream_end = priv->stream_begin + priv->stream_length; //定位到文件后
    
    ret = parse_JFIF(priv, priv->stream_begin);

    return ret;
}

// convert image

int convert_one_image(const char *infilename, const char *outfilename){
    FILE *fp;
    unsigned int length_of_file;
    unsigned int width, height;
    unsigned char *buf;
    struct jdec_private *jdec;
    unsigned char *components[3];

   //load the Jpeg to memory
   fp = fopen(infilename, "rb");
   if(fp == NULL){
       exitmessage("Cannot open filename\n");
   }

   length_of_file = filesize(fp); //得到文件大小
   buf = (unsigned char*)malloc(length_of_file +4);
   if(buf ==NULL){
       exitmessage("Not enough memory for loading file\n");
   }

   fread(buf, length_of_file, fp); //将文件里的jpg数据读到buf中
   fclose(fp);

   //解压
   jdec = tinyjpeg_init(); 
   if(jdec == NULL){
       extimessage("Not enough meory to alloc the structure need for decompressing\n");
   }

   if(tinyjpeg_parse_header(jdec, buf, length_of_file) <0){
       exitmessage(tinyjpeg_get_errorstring(jdec));
   }

   //Get the size of the image
   tinyjpeg_get_size(jdec, &width, &height);

   snprinf(error_string, sizeof(error_string), "Decoding JPEG image...\n");

   if(tinyjpeg_decode(jdec, output_format)<0){
       exitmessage(tinyjpeg_get_errorstring(jdec));
   }

   tinyjpeg_get_components(jdec, components);


   //------------
   tinyjpeg_free(jdec);
   free(buf);
   return 0;
}



void main(){
    char* infilename="/home/chen/sandbox/image/test.jpg";
    char* outfilename="/home/chen/sandbox/image/test_reverse.jpg";

    convert_one_image(infilename, outfilename);

    printf("\n%s", "ending...");
}

