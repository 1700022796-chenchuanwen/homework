#include "stdio.h"
#include "malloc.h"
#include "stdlib.h"

#define HUFFMAN_BITS_SIZE 256
#define HUFFMAN_HASH_NBITS 9 //程序中查找码字长度
#define HUFFMAN_HASH_SIZE (1UL<<HUFFMAN_HASH_NBITS)

enum{
    COMPONENTS=3,
    HUFFMAN_TABLES=256,
    MAX_URL_LENGTH = 128,
};

enum{
    SOI =0xffdb,
}

struct huffman_table{
    short int lookup[HUFFMAN_HASH_SIZE];
    /*得到解码时读取的符号位数 */
    unsigned char code_size[HUFFMAN_HASH_SIZE];
    /*一些地方存放的值不在查找表编码,如果256个值编码足以存储所有的值*/
    uint16_t slowtable[16-HUFFMAN_HASH_NBITS][256];
};

struct component{
    unsigned int Hfactor; /*水平采样因子*/
    unsigned int Vfactor;
    float *Q_table;
    struct huffman_table *AC_table;
    struct huffman_table *DC_table;
    short int previous_DC;
    short int DCT[64];

#if SANITY_CHECK
    unsigned int cid;
#endif
};

/*Allocate a new tinyjpeg decoder object*/

struct jdec_private{

    /* public variables*/
    uint8_t *compnents[COMPONENTS];
    unsigned int width, height;  /*size of  the image*/
    unsigned int flags;

    /*private variables*/
    const unsigned char *stream_begin, *stream_end;
    unsigned int stream_length;

    const unsigned char *stream;
    unsigned int reservior, nbits_in_reservoir;

    struct component component_infos[COMPONENTS];
    float Q_tables[COMPONENTS][64];
    struct huffman_table HTDC[HUFFMAN_TABLES];
    struct huffman_table HTAC[HUFFMAN_TABLES];
    int default_huffman_table_initialized;
    int restart_interval;
    int restarts_to_go;
    int last_rst_marker_seen;

    /* Temp space used after the IDCT to store each components */
    uint8_t Y[64*4], Cr[64], Cb[64];

    //jmp_buf jump_state;
    /* Internal Pointer use for colorspace conversion , do not moidfy it !!!*/
    uint8_t *plane[COMPONENTS];

    /* */
    int tempY[4];
    int tempU, tempV;
    int DQT_table_num;

};

struct jdec_private *tinyjpeg_init(void){
    struct jdec_private *priv;

    priv = (struct jdec_private *)calloc(1, sizeof(struct jdec_private));
    if(priv == NULL)
        return NULL;
    priv->DQT_table_num=0;
    return priv;
};


void exitmessage(char *msg){
    printf("%s", msg);
    exit(0);
}

char* tinyjpeg_get_errorstring(struct jdec_private *priv){
    return "error";
}


int parse_JFIF(struct jdec_private  *priv, const unsigned char *stream){
    int chuck_len;
    int marker;
    int sos_marker_found = 0;
    int dnt_marker_found = 0;

    const unsigned char *next_chunck;

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
            fprintf(param_trace, "> Unknown marker %2.2x\n", marker);
            fflush(param_trace);
#endif
            break;
       }; 

       //解下一个segment
       stream  = next_chunck;
    }

    if(!dht_marker_found){
#if TRACE_PARAM
        fprintf(param_trace, "No Huffman table loaded, using the default one\n");
        fflush(param_trace);
#endif
        build_default_huffman_tables(priv);

        return 0;

bogus_jpeg_format:
        fprintf(param_trace, "Bogus jpeg format\n");
        fflush(param_trace);
        return -1;
    }


}



int tinyjpeg_parse_header(struct jdec_private *priv, const unsigned char *buf, unsigned int size){
    int ret;

    //是否是JPEG文件
    if((buf[0]!=0xFF) || (buf[1] !=SOI)){
        snprintf(error_string, sizeof(error_string),"Not a JPG file ?\n");
    }
    char temp_str[MAX_URL_LENGTH];
    sprintf(temp_str, "0x %X %X", buf[0], buf[1]);

    priv->stream_begin = buf+2;
    priv->stream_length = size -2;
    priv->stream_end = priv->stream_begin + priv->stream_length;
    
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

   if(tinyjpeg_parse_header(jdec, buf, length_of_file) <0){
       exitmessage(tinyjpeg_get_errorstring(jdec));
   }
}



void main(){
    char* infilename="/home/chen/sandbox/image/test.jpg";
    char* outfilename="/home/chen/sandbox/image/test_reverse.jpg";

    convert_one_image(infilename, outfilename);

    printf("\n%s", "ending...");
}

