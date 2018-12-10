#include "decode.h"
#include "huffman.h"
#include "unpack.h"
#include "qzz.h"
#include "dct.h"
#include "tiff.h"
#include "conv.h"
#include "upsampler.h"
#include "downsampler.h"
#include "library.h"

#define  PI     3.1415926535897932384626433832795028841971               //定义圆周率值
struct compx {float real,imag;};                                    //定义一个复数结构


/* Extract and decode a whole JPEG file */
static void read_jpeg(struct jpeg_data *ojpeg, bool *error);

/* Read a TIFF file's image data */
static void read_tiff(struct jpeg_data *ojpeg, bool *error);

/* Extract and decode raw JPEG data */
static void scan_jpeg(struct bitstream *stream, struct jpeg_data *jpeg, bool *error);

/* histogram balance*/
static void histogram_balance(struct jpeg_data *jpeg, bool *error);

static void fft(struct jpeg_data *jpeg, bool *error);

static void laplace(struct jpeg_data *jpeg, bool *error);

void tripow(struct jpeg_data *jpeg, bool *error);

/*
 * Generic quantification table
 * Source : http://www-ljk.imag.fr/membres/Valerie.Perrier/SiteWeb/node10.html
 */
static const uint8_t generic_qt[64];


/* Extract raw image data */
void read_image(struct jpeg_data *jpeg, bool *error)
{
        if (jpeg == NULL || jpeg->path == NULL || *error) {
                *error = true;
                return;
        }

        /* Read input jpeg file */
        if (is_valid_jpeg(jpeg->path))
                read_jpeg(jpeg, error);

        /* Read input tiff file */
        else if (is_valid_tiff(jpeg->path))
                read_tiff(jpeg, error);

        else
                *error = true;

        /* 
         * Initialize jpeg quantification tables
         * and component informations
         */
        if (!*error) {
                uint8_t i_q = 0;
                uint8_t *qtable = (uint8_t*)&jpeg->qtables[0];

                /* Gimp QTables codes (disabled) */
                // uint8_t *Y_qtable = (uint8_t*)&jpeg->qtables[0];
                // uint8_t *CbCr_qtable = (uint8_t*)&jpeg->qtables[1];


                /* Initialize table indexes */
                for (uint8_t i = 0; i < jpeg->nb_comps; i++) {

                        /* Use one AC/DC tree per component */
                        jpeg->comps[i].i_dc = i;
                        jpeg->comps[i].i_ac = i;


                        /* Gimp QTables codes (disabled) */
                        //if (i > 0)
                        //        i_q = 1;

                        jpeg->comps[i].i_q = i_q;
                }

                quantify_qtable(qtable, generic_qt, jpeg->compression);

                /* Gimp QTables codes (disabled) */
                // quantify_qtable(Y_qtable, Y_gimp, jpeg->compression);
                // quantify_qtable(CbCr_qtable, CbCr_gimp, jpeg->compression);


                /* Initialize components's SOS order */
                for (uint8_t i = 0; i < MAX_COMPS; i++)
                        jpeg->comp_order[i] = i;
        }
}

/* Extract and decode a JPEG file */
static void read_jpeg(struct jpeg_data *ojpeg, bool *error)
{
        if (ojpeg == NULL || ojpeg->path == NULL || *error)
                *error = true;

        else {
                struct jpeg_data jpeg;
                memset(&jpeg, 0, sizeof(jpeg));


                struct bitstream *stream = create_bitstream(ojpeg->path, RDONLY);

                if (stream != NULL) {

                        /* Read jpeg header data */
                        read_header(stream, &jpeg, error);
                        ojpeg->nb_comps = jpeg.nb_comps;
                        
                        /* Detect MCU informations from the jpeg structure */
                        detect_mcu(&jpeg, error);

                        /* Initialize output information */
                        ojpeg->mcu.h = jpeg.mcu.h;
                        ojpeg->mcu.v = jpeg.mcu.v;
                        ojpeg->height = jpeg.height;
                        ojpeg->width = jpeg.width;

                        /*
                         * Compute MCU informations :
                         * Number of MCUs, size, Y / Cb / Cr dimensions
                         */
                        compute_mcu(ojpeg, error);


                        /* Extract and decode raw JPEG data */
                        scan_jpeg(stream, &jpeg, error);

                        ojpeg->raw_data = jpeg.raw_data;

                        if(g_task == BALANCE){
							/* Histogram balance*/
							histogram_balance(ojpeg, error);
                        }

                        if(g_task == FFT){
                        	fft(ojpeg, error);
                        }

                        if(g_task ==LAPLACE){
                        	laplace(ojpeg,error);
                        }
    
                        free_bitstream(stream);
                        free_jpeg_data(&jpeg);


                        if (*error)
                                printf("ERROR : invalid input JPEG file\n");

                } else
                        *error = true;
        }
}

/* Read a whole JPEG header */
void read_header(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        uint8_t marker = ANY;

        if (stream == NULL || error == NULL || *error || jpeg == NULL)
                return;

        /* SOI check */
        marker = read_section(stream, SOI, NULL, error);

        if (marker != SOI)
                printf("ERROR : all JPEG files must start with an SOI section\n");

        /* Read all sections until SOS is reached */
        while (!*error && marker != SOS) {
                marker = read_section(stream, ANY, jpeg, error);
                *error |= end_of_bitstream(stream);
        }
}

/* Read a jpeg section */
uint8_t read_section(struct bitstream *stream, enum jpeg_section section,
                        struct jpeg_data *jpeg, bool *error)
{
        uint8_t byte;
        uint8_t marker = ANY;

        if (stream == NULL || error == NULL || *error)
                return marker;


        /* Check for SECTION_HEAD 0xFF */
        *error |= read_byte(stream, &byte);

        if (byte != SECTION_HEAD)
                *error = true;

        /* Retrieve section marker */
        *error |= read_byte(stream, &marker);

        /* If the read marker is not the right one, error */
        if (section && section != marker)
                *error = true;

        /* Nothing to do when SOI or EOI */
        if (marker == SOI || marker == EOI)
                return marker;


        uint16_t size;
        int32_t unread;

        /* Read section size */
        *error |= read_short_BE(stream, &size);

        /*
         * Number of bytes to skip (unused bytes)
         * Updated when reading
         */
        unread = size - sizeof(size);


        /* Process each section */
        switch (marker) {

        case APP0:
                if (!*error) {
                        char jfif[5];

                        for (uint8_t i = 0; i < sizeof(jfif); i++) {
                                *error |= read_byte(stream, (uint8_t*)&jfif[i]);
                                unread--;
                        }

                        /* JFIF header check */
                        if (strcmp(jfif, "JFIF"))
                                *error = true;
                }

                /* Skip unused header data */
                skip_bitstream(stream, unread);
                break;

        case COM:
                /* Skip the comment section */
                skip_bitstream(stream, unread);
                break;

        case DQT:
                if (jpeg != NULL) {

                        /* Read all quantification tables */
                        do {
                                read_byte(stream, &byte);
                                unread--;
                                
                                /* Baseline accuracy is 0 */
                                bool accuracy = ((uint8_t)byte) >> 4;

                                if (accuracy) {
                                        printf("ERROR : this baseline JPEG decoder"\
                                               " only supports 8 bits accuracy\n");
                                        *error = true;
                                } else {
                                        uint8_t i_q = byte & 0xF;

                                        /*
                                         * Read one quantification table
                                         */
                                        if (i_q < MAX_QTABLES) {
                                                uint8_t *qtable = (uint8_t*)&jpeg->qtables[i_q];

                                                for (uint8_t i = 0; i < BLOCK_SIZE; i++) {
                                                        *error |= read_byte(stream, &qtable[i]);
                                                        unread--;
                                                }
                                        } else
                                                *error = true;

                                        /* Update jpeg status */
                                        jpeg->state |= DQT_OK;
                                }

                        } while (unread > 0 && !*error);
                } else
                        *error = true;

                break;

        case SOF0:
                if (jpeg != NULL) {
                        uint8_t accuracy;
                        read_byte(stream, &accuracy);

                        if (accuracy != 8) {
                                printf("ERROR : this baseline JPEG decoder"\
                                       " only supports 8 bits accuracy\n");
                                *error = true;
                        }

                        /* Read jpeg information */
                        read_short_BE(stream, &jpeg->height);
                        read_short_BE(stream, &jpeg->width);

                        read_byte(stream, &jpeg->nb_comps);

                        /* Only RGB & Gray JPEG images are possible */
                        if (jpeg->nb_comps != 3 && jpeg->nb_comps != 1)
                                *error = true;

                        else {
                                /* Read all component information */
                                for (uint8_t i = 0; i < jpeg->nb_comps; i++) {
                                        uint8_t i_c, i_q;
                                        uint8_t h_sampling_factor;
                                        uint8_t v_sampling_factor;

                                        *error |= read_byte(stream, &i_c);

                                        /*
                                         * Component index must range
                                         * in 1 - 3 or 0 - 2
                                         */
                                        if (i_c > 3)
                                                *error = true;

                                        /*
                                         * When indexes range in 1 - 3,
                                         * convert them to 0 - 2
                                         */
                                        if (i_c != i && i_c > 0)
                                                --i_c;

                                        *error |= read_byte(stream, &byte);
                                        h_sampling_factor = byte >> 4;
                                        v_sampling_factor = byte & 0xF;

                                        *error |= read_byte(stream, &i_q);

                                        if (i_q >= MAX_QTABLES)
                                                *error = true;

                                        /* Initialize component informations */
                                        if (!*error) {
                                                jpeg->comps[i_c].nb_blocks_h = h_sampling_factor;
                                                jpeg->comps[i_c].nb_blocks_v = v_sampling_factor;
                                                jpeg->comps[i_c].i_q = i_q;
                                        }

                                        /* Update jpeg status */
                                        jpeg->state |= SOF0_OK;
                                }
                        }
                } else
                        *error = true;

                break;

        case DHT:
                if (jpeg != NULL) {
                        uint8_t unused, type, i_h;

                        /* Read all Huffman tables */
                        while (unread > 0 && !*error) {

                                *error |= read_byte(stream, &byte);
                                unread--;

                                unused = byte >> 5;

                                /*
                                 * 0 for DC
                                 * 1 for AC
                                 */
                                type = (byte >> 4) & 1;

                                i_h = byte & 0xF;


                                /*
                                 * Unused must always be zero
                                 * Never more than 4 tables for each AC/DC type
                                 */
                                if (unused || i_h > 3 || type > 1)
                                        *error = true;

                                /* Read one Huffman Table */
                                uint16_t nb_byte_read;
                                struct huff_table *table;

                                table = load_huffman_table(stream, &nb_byte_read);

                                if (nb_byte_read == (uint16_t)-1 || table == NULL)
                                        *error = true;


                                if (!*error)
                                        jpeg->htables[type][i_h] = table;

                                else
                                        free_huffman_table(table);


                                unread -= nb_byte_read;
                                
                                /* Update jpeg status */
                                jpeg->state |= DHT_OK;
                        }
                } else
                        *error = true;

                break;

        /* Start Of Scan */
        case SOS:
                if (jpeg != NULL) {
                        uint8_t nb_comps, i_c;

                        read_byte(stream, &nb_comps);

                        /* 
                         * Check that the number of component is 
                         * the same as in the SOF Section
                         */
                        if (nb_comps != jpeg->nb_comps) {
                                *error = true;
                                return SOS;
                        }

                        /* Read component informations */
                        for (uint8_t i = 0; i < nb_comps; i++) {
                                read_byte(stream, &byte);

                                /*
                                 * When indexes range in 1 - 3,
                                 * convert them to 0 - 2
                                 */
                                i_c = byte;
                                if(i_c != i && i_c > 0)
                                        --i_c;

                                jpeg->comp_order[i] = i_c;

                                /* Read Huffman table indexes */
                                read_byte(stream, &byte);
                                jpeg->comps[i_c].i_dc = byte >> 4;
                                jpeg->comps[i_c].i_ac = byte & 0xF;
                        }

                        /* Skip unused SOS data */
                        read_byte(stream, &byte);
                        read_byte(stream, &byte);
                        read_byte(stream, &byte);
                } else
                        *error = true;

                break;


        default:
                printf("Unsupported marker : %02X\n", marker);
                skip_bitstream(stream, unread);
        }


        return marker;
}

/* Read a TIFF file's image data */
static void read_tiff(struct jpeg_data *ojpeg, bool *error)
{
        if (ojpeg == NULL || ojpeg->path == NULL || *error)
                *error = true;

        else {
                struct tiff_file_desc *file = NULL;
                uint32_t width, height;
                
                /* Read TIFF header */
                file = init_tiff_read(ojpeg->path, &width, &height);

                if (file != NULL) {

                        /* Initialize output JPEG informations */
                        ojpeg->nb_comps = 3;
                        ojpeg->is_plain_image = true;
                        ojpeg->width = width;
                        ojpeg->height = height;

                        /* Read all raw image data */
                        if (!*error) {
                                uint32_t *line = NULL;

                                const uint32_t nb_pixels_max = ojpeg->width * ojpeg->height;
                                ojpeg->raw_data = malloc(nb_pixels_max * sizeof(uint32_t));

                                if (ojpeg->raw_data == NULL)
                                        *error = true;

                                else {
                                        /* Read all RGB lines */
                                        for (uint32_t i = 0; i < ojpeg->height; i++) {

                                                /* Read one RGB line */
                                                line = &(ojpeg->raw_data[i * ojpeg->width]);

                                                *error |= read_tiff_line(file, line);
                                        }
                                }
                        }

                        close_tiff_file(file);

                } else {
                        printf("ERROR : invalid input TIFF file\n");
                        *error = true;
                }
        }
}


/* Extract and decode raw JPEG data */
static void scan_jpeg(struct bitstream *stream, struct jpeg_data *jpeg, bool *error)
{
        if (stream == NULL || error == NULL || *error || jpeg == NULL)
                return;


        /* Retrieve MCU data */
        uint8_t mcu_h = jpeg->mcu.h;
        uint8_t mcu_v = jpeg->mcu.v;
        uint8_t mcu_h_dim = jpeg->mcu.h_dim;
        uint8_t mcu_v_dim = jpeg->mcu.v_dim;
        uint32_t nb_mcu = jpeg->mcu.nb;

        /* Scan variables */
        uint8_t nb_blocks_h, nb_blocks_v, nb_blocks;
        uint8_t i_c, i_q, i_dc, i_ac;
        int32_t *last_DC;

        uint8_t idct[mcu_h_dim * mcu_v_dim][BLOCK_SIZE];
        int32_t block[BLOCK_SIZE];
        int32_t iqzz[BLOCK_SIZE];
        uint8_t *upsampled;

        uint32_t mcu_size = mcu_h * mcu_v;
        uint32_t *mcu_RGB = NULL;
        uint8_t data_YCbCr[MAX_COMPS][mcu_size];
        uint8_t *mcu_YCbCr[MAX_COMPS] = {
                (uint8_t*)&data_YCbCr[0],
                (uint8_t*)&data_YCbCr[1],
                (uint8_t*)&data_YCbCr[2]
        };

        /* Buffer to store raw jpeg data */
        const uint32_t nb_pixels_max = mcu_size * nb_mcu;
        jpeg->raw_data = malloc(nb_pixels_max * sizeof(uint32_t));

        if (jpeg->raw_data == NULL) {
                *error = true;
                return;
        }

        /* Extract and decode all MCUs */
        for (uint32_t i = 0; i < nb_mcu; i++) {
                mcu_RGB = &jpeg->raw_data[i * mcu_size];

                /* Retrieve each component */
                for (uint8_t j = 0; j < jpeg->nb_comps; j++) {

                        /* Retrieve component informations */
                        i_c = jpeg->comp_order[j];

                        nb_blocks_h = jpeg->comps[i_c].nb_blocks_h;
                        nb_blocks_v = jpeg->comps[i_c].nb_blocks_v;
                        nb_blocks = nb_blocks_h * nb_blocks_v;

                        i_dc = jpeg->comps[i_c].i_dc;
                        i_ac = jpeg->comps[i_c].i_ac;
                        i_q = jpeg->comps[i_c].i_q;

                        last_DC = &jpeg->comps[i_c].last_DC;

                        /* Retrieve MCUs from the JPEG file */
                        for (uint8_t n = 0; n < nb_blocks; n++) {

                                /* Retrieve one block from the JPEG file */
                                unpack_block(stream, jpeg->htables[0][i_dc], last_DC,
                                                     jpeg->htables[1][i_ac], block);

                                /* Convert raw data to Y, Cb or Cr MCU data */
                                iqzz_block(block, iqzz, (uint8_t*)&jpeg->qtables[i_q]);
                                idct_block(iqzz, (uint8_t*)&idct[n]);
                        }

                        /* Upsample current MCUs */
                        upsampled = mcu_YCbCr[i_c];
                        upsampler((uint8_t*)idct, nb_blocks_h, nb_blocks_v, 
                                  upsampled, mcu_h_dim, mcu_v_dim);
                }

                /* Convert YCbCr to RGB for color images */
                if (jpeg->nb_comps == 3)
                        YCbCr_to_ARGB(mcu_YCbCr, mcu_RGB, mcu_h_dim, mcu_v_dim);

                /* Convert Y to RGB for grayscale images */
                else if (jpeg->nb_comps == 1)
                        Y_to_ARGB(mcu_YCbCr[0], mcu_RGB, mcu_h_dim, mcu_v_dim);

                else
                        *error = true;
        }
       
}


/* (1 + (x + y + 1)) QTable */
static const uint8_t generic_qt[64] =
{
         2,  3,  3,  4,  4,  4,  5,  5,
         5,  5,  6,  6,  6,  6,  6,  7,
         7,  7,  7,  7,  7,  8,  8,  8,
         8,  8,  8,  8,  9,  9,  9,  9,
         9,  9,  9,  9, 10, 10, 10, 10,
        10, 10, 10, 11, 11, 11, 11, 11,
        11, 12, 12, 12, 12, 12, 13, 13,
        13, 13, 14, 14, 14, 15, 15, 16
};

/* (x + y + 1) QTable */
// const uint8_t generic_qt[64] =
// {
//          1,  2,  2,  3,  3,  3,  4,  4,
//          4,  4,  5,  5,  5,  5,  5,  6,
//          6,  6,  6,  6,  6,  7,  7,  7,
//          7,  7,  7,  7,  8,  8,  8,  8,
//          8,  8,  8,  8,  9,  9,  9,  9,
//          9,  9,  9, 10, 10, 10, 10, 10,
//         10, 11, 11, 11, 11, 11, 12, 12,
//         12, 12, 13, 13, 13, 14, 14, 15
// };


/* Subject's gimp Luminance table */
// const uint8_t Y_gimp[64] =
// {
//         0x05, 0x03, 0x03, 0x05, 0x07, 0x0c, 0x0f, 0x12,
//         0x04, 0x04, 0x04, 0x06, 0x08, 0x11, 0x12, 0x11,
//         0x04, 0x04, 0x05, 0x07, 0x0c, 0x11, 0x15, 0x11,
//         0x04, 0x05, 0x07, 0x09, 0x0f, 0x1a, 0x18, 0x13,
//         0x05, 0x07, 0x0b, 0x11, 0x14, 0x21, 0x1f, 0x17,
//         0x07, 0x0b, 0x11, 0x13, 0x18, 0x1f, 0x22, 0x1c,
//         0x0f, 0x13, 0x17, 0x1a, 0x1f, 0x24, 0x24, 0x1e,
//         0x16, 0x1c, 0x1d, 0x1d, 0x22, 0x1e, 0x1f, 0x1e
// };

/* Subject's gimp Chrominance table */
// const uint8_t CbCr_gimp[64] =
// {
//         0x05, 0x05, 0x07, 0x0e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x05, 0x06, 0x08, 0x14, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x07, 0x08, 0x11, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x0e, 0x14, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
//         0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e
// };

/*******************************************************************
  函数原型：struct compx EE(struct compx b1,struct compx b2)
  函数功能：对两个复数进行乘法运算
  输入参数：两个以联合体定义的复数a,b
  输出参数：a和b的乘积，以联合体的形式输出
 *******************************************************************/
struct compx EE(struct compx a,struct compx b)
{
	struct compx c;

	c.real=a.real*b.real-a.imag*b.imag;
	c.imag=a.real*b.imag+a.imag*b.real;

	return(c);
}
                                                   //定义福利叶变换的点数


static void fft(struct jpeg_data *jpeg, bool *error){
        if (error == NULL || *error || jpeg == NULL)
                return;
        uint32_t *raw = jpeg->raw_data;
        uint32_t scale =  jpeg->height * jpeg->width;
        uint32_t FFT_N =1;
        int i;
//        int FFT_N = jpeg->height * jpeg->width;

        while(FFT_N*2 <= scale){
        	FFT_N *=2;    //必须为2的N次方
        }
        i=0;
        while(i<scale){
        	raw[i] = (RED(raw[i]) + BLUE(raw[i]) +GREEN(raw[i]))/3 ;
//        	raw[i] *= pow(-1,(i%jpeg->width + i/jpeg->width));
        	i++;
        }

        struct compx s[FFT_N];

        trace(">> count fft：\n");

        for(i = 0; i < FFT_N; i++)                                        //给结构体赋值
		{
			s[i].real = 1 + 2*sin(2*3.141592653589793 * raw[i] / FFT_N);         //实部为正弦波FFT_N点采样，赋值为1
			s[i].imag = 0;                                                //虚部为0
		}

        trace(">> fft begin：\n");

        struct compx *xin =s;
    	int f , m, nv2, nm1, k, l, j = 0;
    	struct compx u,w,t;

    	nv2 = FFT_N / 2;                   //变址运算，即把自然顺序变成倒位序，采用雷德算法
    	nm1 = FFT_N - 1;
    	for(i = 0; i < nm1; i++)
    	{
    		if(i < j)                      //如果i<j,即进行变址
    		{
    			t = xin[j];
    			xin[j] = xin[i];
    			xin[i] = t;
    		}
    		k = nv2;                       //求j的下一个倒位序
    		while( k <= j)                 //如果k<=j,表示j的最高位为1
    		{
    			j = j - k;                 //把最高位变成0
    			k = k / 2;                 //k/2，比较次高位，依次类推，逐个比较，直到某个位为0
    		}
    		j = j + k;                     //把0改为1
    	}

    	{
    		int le , lei, ip;                            //FFT运算核，使用蝶形运算完成FFT运算

    		f = FFT_N;
    		for(l = 1; (f=f/2)!=1; l++)                  //计算l的值，即计算蝶形级数
    			;
    		for( m = 1; m <= l; m++)                         // 控制蝶形结级数
    		{                                        //m表示第m级蝶形，l为蝶形级总数l=log（2）N
    			le = 2 << (m - 1);                            //le蝶形结距离，即第m级蝶形的蝶形结相距le点
    			lei = le / 2;                               //同一蝶形结中参加运算的两点的距离
    			u.real = 1.0;                             //u为蝶形结运算系数，初始值为1
    			u.imag = 0.0;
    			w.real = cos(PI / lei);                     //w为系数商，即当前系数与前一个系数的商
    			w.imag = -sin(PI / lei);
    			for(j = 0;j <= lei - 1; j++)                   //控制计算不同种蝶形结，即计算系数不同的蝶形结
    			{
    				for(i = j; i <= FFT_N - 1; i = i + le)            //控制同一蝶形结运算，即计算系数相同蝶形结
    				{
    					ip = i + lei;                           //i，ip分别表示参加蝶形运算的两个节点
    					t = EE(xin[ip], u);                    //蝶形运算，详见公式
    					xin[ip].real = xin[i].real - t.real;
    					xin[ip].imag = xin[i].imag - t.imag;
    					xin[i].real = xin[i].real + t.real;
    					xin[i].imag = xin[i].imag + t.imag;
    				}
    				u = EE(u, w);                           //改变系数，进行下一个蝶形运算
    			}
    		}
    	}

        for(i = 0; i < FFT_N; i++)
		{
        	jpeg->raw_data[i] = s[i].real = sqrt(s[i].real*s[i].real + s[i].imag * s[i].imag);
		}

        i=0;
        while(i<scale){
//        	raw[i] = log(1+raw[i]);
        	i++;
        }

}

//拉普拉斯算子
static void laplace(struct jpeg_data *jpeg, bool *error){
    if (error == NULL || *error || jpeg == NULL)
            return;

    uint32_t *raw = jpeg->raw_data; //从jpeg中读取的像素数据，按mcu排列

    uint32_t nb_h = jpeg->mcu.nb_h; //mcu水平总数
    uint32_t nb_v = jpeg->mcu.nb_v; //mcu垂直总数

    uint8_t mcu_h = jpeg->mcu.h; //单位mcu水平像元
    uint8_t mcu_v = jpeg->mcu.v; //单位mcu垂直像元
    uint32_t mcu_size = mcu_h*mcu_v;   //单个mcu像元数

    uint32_t *mcu_blocks = NULL;  //mcu块

    int width = jpeg->width, height = jpeg->height;  //jpeg图像的宽和长

    //转化成矩阵
    int32_t matrix[height][width];
    int32_t matrix_laplace[height][width];


    for(int i=0;i<nb_v;i++){
    	for(int j=0;j<nb_h;j++){
			 mcu_blocks = &raw[(i*nb_h+j)*mcu_size];  //获取一个mcu
			 for(int v=0;v<mcu_v;v++){ //mcu的高
				 for(int h=0;h<mcu_h && i*mcu_v+v <height && j*mcu_h+h<width;h++){ //mcu的宽
					 matrix[i*mcu_v+v][j*mcu_h+h] = mcu_blocks[v*mcu_h+ h];
				 }
			 }
    	}

    }
    trace("height=%d, width=%d\n", height, width);  //debug
     //拉普拉斯转换
    for(int j=1;j<height-1;j++){
    	for(int k=1;k<width-1;k++){
			matrix_laplace[j][k] = (BLUE(matrix[j][k-1])
					+BLUE(matrix[j][k+1])
					+BLUE(matrix[j-1][k])
					+BLUE(matrix[j+1][k])
					- 4*BLUE(matrix[j][k]));

//			if(matrix_laplace[j][k]<0){
//				matrix_laplace[j][k] =0;
//			}
//			if(matrix_laplace[j][k]>255){
//				matrix_laplace[j][k] =255;
//			}

			matrix_laplace[j][k] = matrix[j][k] - matrix_laplace[j][k];

			if(matrix_laplace[j][k]<0){
				matrix_laplace[j][k] =0;
			}
//			if(matrix_laplace[j][k]>255){
//				matrix_laplace[j][k] =255;
//			}
    	}
    }

    //写回原图

    for(int i=0;i<nb_v;i++){
    	for(int j=0;j<nb_h;j++){
			 mcu_blocks = &raw[(i*nb_h+j)*mcu_size];  //获取一个mcu
			 for(int v=0;v<mcu_v;v++){ //mcu的高
				 for(int h=0;h<mcu_h&& i*mcu_v+v <height && j*mcu_h+h<width;h++){ //mcu的宽
					  mcu_blocks[v*mcu_h+ h]= matrix_laplace[i*mcu_v+v][j*mcu_h+h];
				 }
			 }
    	}

    }

}

static void histogram_balance(struct jpeg_data *jpeg, bool *error){
        if (error == NULL || *error || jpeg == NULL)
                return;

        uint32_t *raw = jpeg->raw_data;
        uint32_t scale =  jpeg->height * jpeg->width;
        uint32_t i =0;
        uint32_t  R[scale], G[scale], B[scale];
          
        //get R,G,B component
        while(i<scale){
          R[i] = raw[i] >>16;
          G[i] = (raw[i] <<16)>>24;
          B[i] = (raw[i]<<24)>>24;
          i++;
        }

       //count R
       i=0;
       uint32_t RGB_freq[3][255] ={0};
       uint32_t RGB_count[3]={0};
       while(i<scale){
          RGB_freq[0][R[i]]++;  //red
          RGB_freq[1][G[i]]++;  //green
          RGB_freq[2][B[i]]++;  //blue
         i++;
       }
       i=0;
       trace(">rgb count:\n");
       while(i<255){
         RGB_count[0] += RGB_freq[0][i]; //red
         RGB_count[1] += RGB_freq[1][i]; //red
         RGB_count[2] += RGB_freq[2][i]; //red
         //trace("> %d:RGB_freq:%d ", i,RGB_freq[0][i]);
         i++;
       }
 
       //count probability
       float RGB_prob[3][255]={0};
       i=0;
       while(i<255){
         RGB_prob[0][i]= RGB_freq[0][i]*1.0/RGB_count[0];  //red
         RGB_prob[1][i]= RGB_freq[1][i]*1.0/RGB_count[1];  //green
         RGB_prob[2][i]= RGB_freq[2][i]*1.0/RGB_count[2];  //blue
       //  trace(">RGB_prob:%f ", RGB_prob[0][i]);
         i++;
       }       
       //cdf 
       float RGB_cdf[3][255]={0};
       i=1;
       RGB_cdf[0][0] = RGB_prob[0][0] ;
       RGB_cdf[1][0] = RGB_prob[1][0] ;
       RGB_cdf[2][0] = RGB_prob[2][0] ;
       while(i<255){
            RGB_cdf[0][i] = RGB_cdf[0][i-1]+RGB_prob[0][i];
            RGB_cdf[1][i] = RGB_cdf[1][i-1]+RGB_prob[1][i];
            RGB_cdf[2][i] = RGB_cdf[2][i-1]+RGB_prob[2][i];
           // trace(">RGB_cdf:%d ", RGB_cdf[0][i]);
         i++;
       } 
       i=0;
       uint32_t RGB_balance[0][255];
       while(i<255){
          RGB_balance[0][i]= RGB_cdf[0][i]*255; 
          RGB_balance[1][i]= RGB_cdf[1][i]*255; 
          RGB_balance[2][i]= RGB_cdf[2][i]*255; 
          //trace(">RGB_balance:%d ", RGB_balance[1][i]);
         i++;
       };

       trace(">\nrewrite:\n");
       //rewrite raw data
       i=0;
       uint32_t pixel=0;
       while(i<scale){
          pixel = raw[i];
          raw[i] =  RGB_balance[0][pixel>>16] <<16; //red
          raw[i] |=  (RGB_balance[1][(pixel<<16)>>24]) <<8; //green
          raw[i] |=  RGB_balance[2][(pixel<<24)>>24]; //blue
        // raw[i] = (raw[i]>>16)<<16;
         i++;
       }
}

// three pow of jpg  -- by chuanwen 2018-10-20
void tripow(struct jpeg_data *jpeg, bool *error){
    if( error==NULL || *error || jpeg==NULL){ 
        return;
    }

    uint32_t *raw = jpeg->raw_data;
    uint32_t scale = jpeg->width* jpeg->height;
    uint32_t R, G, B;
    uint8_t Y,Cb,Cr;

    uint32_t i=0;
    while(i<scale){
        R =  RED(raw[i]);
        G = (raw[i] <<16)>>24;
        B = (raw[i]<<24)>>24;

        Y  = 0.299 * R + 0.587 * G + 0.114 * B;
        Cb = -0.1687 * R - 0.3313 * G + 0.5 * B + 128;
        Cr = 0.5 * R - 0.4187 * G - 0.0813 * B + 128;

        Y = pow(Y,0.6);

        R = Y - 0.0009267 * (Cb - 128) + 1.4016868 * (Cr - 128);
        G = Y - 0.3436954 * (Cb - 128) - 0.7141690 * (Cr - 128);
        B = Y + 1.7721604 * (Cb - 128) + 0.0009902 * (Cr - 128);


        raw[i] = TRUNCATE(R)<<16 | TRUNCATE(G)<<8 | TRUNCATE(B);
        i++;
    }

    

}
