#include "stdio.h"
#include "image.h"

#define be16_to_cpu(x) (((x)[0]<<8)|(x)[1])
/* Global variable to return the last error found while deconding */
static char error_string[256];

#define error(fmt, args...) do { \
   snprintf(error_string, sizeof(error_string), fmt, ## args); \
   return -1; \
} while(0)

void exitmessage(char *msg){
    printf("%s", msg);
    exit(0);
}

char* tinyjpeg_get_errorstring(struct jdec_private *priv){
    return "error";
}


void build_huffman_table(const unsigned char* bits, const  unsigned char* vals, struct huffman_table *table){
    
}


void build_default_huffman_tables(struct jdec_private *priv){
    if( (priv->flags & TINYJPEG_FLAGS_MJPEG_TABLE) && priv->default_huffman_table_initialized){
        return;
    }

    build_huffman_table(bits_dc_luminance, val_dc_luminance, &priv->HTDC[0]);
    build_huffman_table(bits_dc_luminance, val_ac_luminance, &priv->HTAC[0]);

    build_huffman_table(bits_dc_chrominance, val_dc_chrominance, &priv->HTDC[1]);
    build_huffman_table(bits_dc_chrominance, val_ac_chrominance, &priv->HTAC[1]);

    priv->default_huffman_table_initialized =1;
}






void tinyjpeg_get_size(struct jdec_private* jdec, int* width, int* length){

}
int tinyjpeg_decode(struct jdec_private* jdec, int output_format){
    return -1;
}

void tinyjpeg_get_components(struct jdec_private* jdec,  unsigned char** components){

}

void tinyjpeg_free(struct jdec_private* jdec){

}
static void print_SOF(const unsigned char *stream)
{
  int width, height, nr_components, precision;
#if DEBUG
  const char *nr_components_to_string[] = {
     "????",
     "Grayscale",
     "????",
     "YCbCr",
     "CYMK"
  };
#endif

  precision = stream[2];
  height = be16_to_cpu(stream+3);
  width  = be16_to_cpu(stream+5);
  nr_components = stream[7];

  trace("> SOF marker\n");
  trace("Size:%dx%d nr_components:%d (%s)  precision:%d\n", 
      width, height,
      nr_components, nr_components_to_string[nr_components],
      precision);
}
static int parse_SOF(struct jdec_private *priv, const unsigned char *stream)
{
  int i, width, height, nr_components, cid, sampling_factor;
  int Q_table;
  struct component *c;

  trace("> SOF marker\n");
  print_SOF(stream);

  height = be16_to_cpu(stream+3);
  width  = be16_to_cpu(stream+5);
  nr_components = stream[7];
#if SANITY_CHECK
  if (stream[2] != 8)
    error("Precision other than 8 is not supported\n");
  if (width>JPEG_MAX_WIDTH || height>JPEG_MAX_HEIGHT)
    error("Width and Height (%dx%d) seems suspicious\n", width, height);
  if (nr_components != 3)
    error("We only support YUV images\n");
  if (height%16)
    error("Height need to be a multiple of 16 (current height is %d)\n", height);
  if (width%16)
    error("Width need to be a multiple of 16 (current Width is %d)\n", width);
#endif
  stream += 8;
  for (i=0; i<nr_components; i++) {
     cid = *stream++;
     sampling_factor = *stream++;
     Q_table = *stream++;
     c = &priv->component_infos[i];
#if SANITY_CHECK
     c->cid = cid;
     if (Q_table >= COMPONENTS)
       error("Bad Quantization table index (got %d, max allowed %d)\n", Q_table, COMPONENTS-1);
#endif
     c->Vfactor = sampling_factor&0xf;
     c->Hfactor = sampling_factor>>4;
     c->Q_table = priv->Q_tables[Q_table];
     trace("Component:%d  factor:%dx%d  Quantization table:%d\n",
           cid, c->Hfactor, c->Hfactor, Q_table );

  }
  priv->width = width;
  priv->height = height;

  trace("< SOF marker\n");

  return 0;
}

int parse_JFIF(struct jdec_private  *priv, const unsigned char *stream){
    int chuck_len;
    int marker;
    int sos_marker_found = 0;
    int dnt_marker_found = 0;
    const unsigned char *next_chunck;
    char* param_trace;

    trace("parsing_JFIF……\n");

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

       trace("marker:%x chuck_len:%d\n", marker, chuck_len);
       switch(marker){
           case SOF:
               if(parse_SOF(priv, stream)<0){
                   trace("%s", "parse_JFIF……");
                   return -1;
               }
            break;
           case DQT:
            return -1;
            break;
           case SOS:
            sos_marker_found = 1;
            break;
           case DHT:
            return -1;
            break;
           default:
            trace("> Unknown marker %2.2x\n", marker);
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
        trace("bogus_jpeg_format%s\n", "t");
        return -1;
    }

}

int tinyjpeg_parse_header(struct jdec_private *priv, const unsigned char *buf, unsigned int size){
    int ret;

    //是否是JPEG文件
    /* Identify the file */
    if ((buf[0] != 0xFF) || (buf[1] != SOI))
        error("Not a JPG file ?\n");

    priv->stream_begin = buf+2;
    priv->stream_length = size -2;
    priv->stream_end = priv->stream_begin + priv->stream_length; //定位到文件后
    trace("parsing header:%x%x\n", buf[0],buf[1]); 
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

    char* error_string;
    int  output_format;

   //load the Jpeg to memory
   fp = fopen(infilename, "rb");
   if(fp == NULL){
       exitmessage("Cannot open filename\n");
   }

   fseek(fp,0,SEEK_END);
   length_of_file = ftell(fp); //得到文件大小
   buf = (unsigned char*)malloc(length_of_file +4);
   if(buf ==NULL){
       exitmessage("Not enough memory for loading file\n");
   }

   fseek(fp,0,SEEK_SET);
   fread(buf,sizeof(unsigned char), length_of_file, fp); //将文件里的jpg数据读到buf中
   fclose(fp);

   //解压
   jdec = tinyjpeg_init(); 
   if(jdec == NULL){
       exitmessage("Not enough meory to alloc the structure need for decompressing\n");
   }

   trace("parse_header:%d buf:%d\n",length_of_file, sizeof(*buf));
   if(tinyjpeg_parse_header(jdec, buf, length_of_file) <0){
       exitmessage(tinyjpeg_get_errorstring(jdec));
   }

   //Get the size of the image
   tinyjpeg_get_size(jdec, &width, &height);

   snprintf(error_string, sizeof(error_string), "Decoding JPEG image...\n");

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

