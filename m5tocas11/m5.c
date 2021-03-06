/*
 *        Sord M5 CAS and WAV generator/converter
 *
 *        $Id: m5.c,v 1.6 2016-06-26 00:46:55 aralbrec Exp $
 */

#include "appmake.h"
extern char samp11khz;
extern char samp22khz;

static char             *binname      = NULL;
static char             *crtfile      = NULL;
static char             *outfile      = NULL;
static char             *blockname    = NULL;
static int               origin       = -1;
static char              help         = 0;
static char              audio        = 0;
static char              casinput     = 0;
//static char              samp12khz    = 1;
static char              fast         = 0;
static char              debug         = 0;
unsigned long            checksum;


/* Options that are available for this module */
option_t m5_options[] = {
    { 'h', "help",     "Display this help",          OPT_BOOL,  &help},
    { 'b', "binfile",  "Linked binary file",         OPT_STR,   &binname },
    { 'c', "crt0file", "crt0 file used in linking",  OPT_STR,   &crtfile },
    { 'o', "output",   "Name of output file",        OPT_STR,   &outfile },
    {  0,  "audio",    "Create also a WAV file",     OPT_BOOL,  &audio },
    {  0,  "casinput", "input binfile is a cas file",OPT_BOOL,  &casinput },
    {  0,  "samp11khz", "input binfile is a cas file",OPT_BOOL,  &samp11khz },	
    {  0,  "samp22khz", "input binfile is a cas file",OPT_BOOL,  &samp22khz },
    {  0,  "fast",     "Create a fast loading WAV",  OPT_BOOL,  &fast },
    {  0 , "org",      "Origin of the binary",       OPT_INT,   &origin },
    {  0 , "debug",      "debug block prints",       OPT_BOOL,   &debug },	
    {  0 , "blockname", "Name of the code block in tap file", OPT_STR, &blockname},
    {  0 ,  NULL,       NULL,                        OPT_NONE,  NULL }
};

void m5_bit(FILE* fpout, unsigned char bit)
{
    int i, period, period1;

    /* strangely long period = 0, short period = 1 */

if (samp11khz) {
//11KHz
    if (bit) {
        // '1'
        if (fast) {
            period = 2;
            period1 = 2;
        } else {
            period = 2;
            period1 = 2;
        }
    } else {
        // '0'
        if (fast) {
            period = 4;
            period1 = 4;
        } else {
            period = 4;
            period1 = 4;
        }
    }
} else 

if (samp22khz) {
//11KHz
    if (bit) {
        // '1'
        if (fast) {
            period = 4;
            period1 = 4;
        } else {
            period = 4;
            period1 = 4;
        }
    } else {
        // '0'
        if (fast) {
            period = 8;
            period1 = 8;
        } else {
            period = 8;
            period1 = 8;
        }
    }
} else {

//44KHz	   
    if (bit) {
        // '1'
        if (fast) {
            period = 7;
            period1 = 7;
        } else {
            period = 8;
            period1 = 7;
        }
    } else {
        // '0' 
        if (fast) {
            period = 13;
            period1 = 13;
        } else {
            period = 15;
            period1 = 14;
        }
    }
}

    for (i = 0; i < period1; i++)
        fputc(0x20, fpout);
    for (i = 0; i < period; i++)
        fputc(0xe0, fpout);
}

void m5_rawout(FILE* fpout, unsigned char b)
{
    /* bit order is reversed ! */
    static unsigned char c[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
    int i;

    /* byte */
    for (i = 0; i < 8; i++)
        m5_bit(fpout, b & c[i]);
}

int m5_exec(char* target)
{
    char filename[FILENAME_MAX + 1];
    char wavfile[FILENAME_MAX + 1];
    char name[11];
    FILE *fpin, *fpout;
    long pos;
    int c;
    int i;
    int len, blockid, blocklen;
	
	//printf("binname %s\n", binname);

    if (help)
        return -1;

/*    if (binname == NULL || (crtfile == NULL && origin == -1)) {
        return -1;
    } */
	 if (binname == NULL)
		return -1;
		
	//printf("outfile %s\n", outfile);
    if (outfile == NULL) {
        strcpy(filename, binname);
        suffix_change(filename, ".cas");
    } else {
        strcpy(filename, outfile);
    }

if (!casinput) {
    if (blockname == NULL)
        blockname = binname;

    if (origin != -1) {
        pos = origin;
    } else {
        if ((pos = get_org_addr(crtfile)) == -1) {
            myexit("Could not find parameter ZORG (not z88dk compiled?)\n", 1);
        }
    }

    if ((fpin = fopen_bin(binname, crtfile)) == NULL) {
        fprintf(stderr, "Can't open input file %s\n", binname);
        myexit(NULL, 1);
    }

    /*
 *        Now we try to determine the size of the file
 *        to be converted
 */
    if (fseek(fpin, 0, SEEK_END)) {
        fprintf(stderr, "Couldn't determine size of file\n");
        fclose(fpin);
        myexit(NULL, 1);
    }

    len = ftell(fpin);

    fseek(fpin, 0L, SEEK_SET);

    if ((fpout = fopen(filename, "wb")) == NULL) {
        fclose(fpin);
        myexit("Can't open output file\n", 1);
    }

    /* ===============
	     FILE HEADER
	   =============== */
    /* File identifier for emulators and tools */
    writestring("SORDM5", fpout);

    /* LEADER tone */
    for (i = 0; i < 10; i++)
        fputc(0, fpout);

    writebyte('H', fpout); /* HEADER */
    writebyte(31, fpout); /* HD len */
    checksum = 0;

    writebyte_cksum(3, fpout, &checksum); /* to be loaded with 'TAPE' command */

    /* Deal with the filename */
    if (strlen(blockname) >= 9) {
        strncpy(name, blockname, 9);
    } else {
        strcpy(name, blockname);
        strncat(name, "\0\0\0\0\0\0\0\0", 9 - strlen(blockname));
    }
    for (i = 0; i < 9; i++)
        writebyte_cksum(name[i], fpout, &checksum);

    /* pos */
    writeword_cksum(pos, fpout, &checksum);
    /* len */
    //writeword_cksum(len,fpout,&checksum);
    writeword_cksum(len, fpout, &checksum);
    /* start */
    writeword_cksum(pos, fpout, &checksum);

    /* ================
	     FILE CONTENT
	   ================ */

    /* LEADER tone */
    for (i = 0; i < 15; i++)
        writebyte_cksum(0, fpout, &checksum);

    for (i = 0; i < len; i++) {
        if (((i % 256) == 0) && (i != len)) {
            writebyte((checksum % 256), fpout);
            writebyte('D', fpout); /* DATA */
            writebyte(0, fpout); /* next block is 256 bytes long */
            checksum = 0;
        }
        c = getc(fpin);
        writebyte_cksum(c, fpout, &checksum);
    }
    /* last block filler */
    for (i = (len % 256); i < 256; i++)
        writebyte_cksum(0, fpout, &checksum);
    writebyte((checksum % 256), fpout);

    fclose(fpin);
    fclose(fpout);
}
    /* ***************************************** */
    /*  Now, if requested, create the audio file */
    /* ***************************************** */
    if (audio || casinput)  {
		if (outfile != NULL) {strcpy(filename, binname); suffix_change(filename, ".cas");}
        if ((fpin = fopen(filename, "rb")) == NULL) {
            fprintf(stderr, "Can't open file %s for wave conversion\n", filename);
            myexit(NULL, 1);
        }
		
        if (fseek(fpin, 0, SEEK_END)) {
            fprintf(stderr, "Couldn't determine size of file\n");
            fclose(fpin);
            myexit(NULL, 1);
        }
		if (!casinput)
        	len = ftell(fpin) - 16 - 32;
		else len = ftell(fpin) - 16;
		
	 if (debug) printf("len=%d\n",len);
        fseek(fpin, 16L, SEEK_SET);

		if (outfile != NULL) strcpy(filename, outfile); 
        strcpy(wavfile, filename);
        suffix_change(wavfile, ".RAW");
        if ((fpout = fopen(wavfile, "wb")) == NULL) {
            fprintf(stderr, "Can't open output raw audio file %s\n", wavfile);
            myexit(NULL, 1);
        }
		
		//int f;
		//if (samp22khz) f= 2;
		//else f =1;		

        // leading silence 
        for (i = 0; i < 4000; i++)
            //fputc(0xe0, fpout);
			fputc(0x80, fpout);

        // better making the first leader tone last longer
        for (i = 0; (i < 4000); i++)
            m5_bit(fpout, 1);



        /* Header and data blocks loop */
        while (ftell(fpin) < len) {
            c = getc(fpin);				// intial file block 0x48, subsequent blocks 0x44
			blockid=c;	  	  	  

		 if (debug) printf("[offset:value] firstbyte 0x%x:0x%x ", ftell(fpin)-1, c);	   	   	   	   
            blocklen = getc(fpin);
		 if (debug) printf("blocklen 0x%x:0x%x  ", ftell(fpin)-1, blocklen);  	  	  
            ungetc(blocklen, fpin);
            if (blocklen == 0)
                blocklen = 256;	
				  	  	  	  
            // leader tone
            if (fast) {
                for (i = 0; (i < 80); i++)  m5_bit(fpout, 1);
            } else {
				if (blockid == 0x48) {
					for (i = 0; (i < 800); i++) m5_bit(fpout, 1); // better making the first leader tone last longer	  	   	   	   
				}
                else for (i = 0; (i < 200); i++) m5_bit(fpout, 1);
            }

			
            // bytes
           for (i = 1; (i < (blocklen + 3)); i++) {
			  //printf("byte: 0x%x\n", c);	  	  	  	  
                m5_bit(fpout, 0); /* start bit */
                m5_rawout(fpout, c);
                m5_bit(fpout, 1); /* stop bit */
                c = getc(fpin);	   	   	   
            }	 	 	 
			if (debug) printf("lastbyte 0x%x:0x%x\n", ftell(fpin)-1, c);	  	    				
            /* last byte in block ?checksum?.. */
            m5_bit(fpout, 0); /* start bit */
            m5_rawout(fpout, c);
            /* ..has an oddly shaped termination */

			if (samp11khz) {	
	            for (i = 0; i < 2; i++)
	                fputc(0x20, fpout);
	            for (i = 0; i < 4; i++)
	                fputc(0xe0, fpout);
	        } else 	      	  	  
			if (samp22khz) {	
	            for (i = 0; i < 4; i++)
	                fputc(0x20, fpout);
	            for (i = 0; i < 8; i++)
	                fputc(0xe0, fpout);
	        } else {
	            for (i = 0; i < 8; i++)
	                fputc(0x20, fpout);
	            for (i = 0; i < 14; i++)
	                fputc(0xe0, fpout);	   	   	   	   
			}
		}
        /* trailing silence */
        for (i = 0; i < 0x500; i++)
            //fputc(0xe0, fpout);
			fputc(0x80, fpout);

        fclose(fpin);
        fclose(fpout);

        /* Now let's think at the WAV format */
        raw2wav(wavfile);
    }

    return 0;
}

