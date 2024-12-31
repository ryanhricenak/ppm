/*
| read-ppm.c
| Read and copy a ppm file via structs and 2D arrays
| Ryan Hricenak
| 11/23/2024
*/

#include<stdio.h>
#include<ctype.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>

#define MAX_LEN 1024

#define NUM_THREADS 8

typedef struct Pixel8 {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} pixel8;

typedef struct arguments{
	pixel8 **image;
	pixel8 **copy;		

	int nrows;
	int ncols;
	
	int id;
	int start_row;
	int end_row;
} WorkerArgs;

void find_any_comments(FILE * fo_src, FILE * fo_dst) {
        char comment[MAX_LEN];

        char c;
        char * retval;
        do {
                // Strip any preceding whitespace:
                while (isspace(c = fgetc(fo_src)))
                ;
                ungetc(c, fo_src); // Put back the non-whitespace

                // Is there a comment?
                if ('#' != c) {
                        // no comment
                        retval = NULL;
                } else {
                        // comment
                        int lastpos;
                        do {
                                retval = fgets(comment, MAX_LEN, fo_src);
                                fputs(comment, stdout);
                                if (NULL != fo_dst)
                                        fputs(comment, fo_dst);
                                lastpos = strlen(comment) - 1;
                        } while (comment[lastpos] != '\n');
                }
        } while (retval != NULL);
}

int get_value(FILE * fo_src) {
        int value;
        int retval;
        if (1 != (retval = fscanf(fo_src, " %d", & value))) {
                fprintf(stderr, "Fail - get_value()/fscanf()==%d, value==%d\n", retval, value);
                value = -1;
        }
        return value;
}

char getLowest(char a, char b){
	return (a < b) ? a : b;
}

char getHighest(char a, char b){
	return (a < b) ? b : a;
}

void getExtremes(pixel8 **image, int height, int width, unsigned char *redLow, unsigned char *redHigh, unsigned char *greenLow, unsigned char *greenHigh, unsigned char *blueLow, unsigned char *blueHigh){

	unsigned char red, green, blue;

	for(int r = 0; r < height; ++r){
		pixel8 *rowp = *(image + r);
		for(int c = 0; c < width; ++c){
			red = ((rowp + c) -> red);
			green = ((rowp + c) -> green);
			blue = ((rowp + c) -> blue);

			if(r != 0 || c != 0){
				*redLow = getLowest(red, *redLow);
				*redHigh = getHighest(red, *redHigh);
				*greenLow = getLowest(green, *greenLow);
				*greenHigh = getHighest(green, *greenHigh);
				*blueLow = getLowest(blue, *blueLow);
				*blueHigh = getHighest(blue, *blueHigh);
			} else{
				*redLow = red;
				*redHigh = red;
				*greenLow = green;
				*greenHigh = green;
				*blueLow = blue;
				*blueHigh = blue;
			}
		}	
	}
}

int getOption(char *option){
                if (strcmp(option, "lRotate") == 0) {
                        return 1;
                } else if (strcmp(option, "rRotate") == 0) {
                        return 2;
                } else if (strcmp(option, "gray") == 0 || strcmp(option, "grey") == 0) {
                        return 3;
                } else if (strcmp(option, "contrast") == 0) {
                        return 4;
                } else if (strcmp(option, "negative") == 0){
			return 5;
		} else if (strcmp(option, "flip") == 0){
			return 6;
		} else if (strcmp(option, "blur") == 0){
			return 7;
		} else if (strcmp(option, "unsharp") == 0){
			return 8;
		} else return 0;
}

void readHeaders(int *width, int *height, int *depth, char *magic, FILE* fo_src, FILE* fo_dst){
        int n = fscanf(fo_src, "%2s", magic);
        if ((1 != n) || strcmp(magic, "P6")) {
                fprintf(stderr, "Bad magic number %s\n", magic);
                exit(2);
        }
        fprintf(stdout, "\n// Magic Value %s\n", magic);
        if (fo_dst != NULL)
                fprintf(fo_dst, "%s\n", magic);

        find_any_comments(fo_src, fo_dst);
        *width = get_value(fo_src);
        fprintf(fo_dst, "%d ", *width);

        find_any_comments(fo_src, fo_dst);
        *height = get_value(fo_src);
        fprintf(fo_dst, "%d\n", *height);

        find_any_comments(fo_src, fo_dst);
        *depth = get_value(fo_src);
        fprintf(fo_dst, "%d\n", *depth);

        char c = fgetc(fo_src);
        if (c == '#')
                find_any_comments(fo_src, fo_dst);
        else if (!isspace(c)) {
                fprintf(stderr, "Bad file structure\n");
                exit (4);
        }
}

void readImageIn(FILE *fo_src, pixel8 **image, int height, int width){
	unsigned char red, green, blue;

	for(int r = 0; r < height; ++r){
		pixel8 *rowp = *(image + r);
		for(int c = 0; c < width; ++c){
			red = fgetc(fo_src);
			green = fgetc(fo_src);
			blue = fgetc(fo_src);	

			((rowp + c) -> red) = red;
			((rowp + c) -> green) = green;
			((rowp + c) -> blue) = blue;
		}	
	}
}

void *classicCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;
	unsigned char red, green, blue;

	for(int r = (argp -> start_row); r < (argp -> end_row); ++r){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + r);
		for(int c = 0; c < (argp -> ncols); ++c){
			red = ((rowp + c) -> red);
			green = ((rowp + c) -> green);
			blue = ((rowp + c) -> blue);

			((rowc + c) -> red) = red;
			((rowc + c) -> green) = green;
			((rowc + c) -> blue) = blue;
		}	
	}
	return NULL;
}

void *lRotateCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;
	unsigned char red, green, blue;

	for(int r = (argp -> start_row); r < (argp -> end_row); ++r){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + r);
		for(int c = 0; c < (argp -> ncols); ++c){
			red = ((rowp + c) -> red);
			green = ((rowp + c) -> green);
			blue = ((rowp + c) -> blue);

			((rowc + c) -> red) = green;
			((rowc + c) -> green) = blue;
			((rowc + c) -> blue) = red;
		}	
	}
	return NULL;
}

void *rRotateCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;
	unsigned char red, green, blue;

	for(int r = (argp -> start_row); r < (argp -> end_row); ++r){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + r);
		for(int c = 0; c < (argp -> ncols); ++c){
			red = ((rowp + c) -> red);
			green = ((rowp + c) -> green);
			blue = ((rowp + c) -> blue);

			((rowc + c) -> red) = blue;
			((rowc + c) -> green) = red;
			((rowc + c) -> blue) = green;
		}	
	}
	return NULL;
}

void *grayCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;
	unsigned char red, green, blue;
	unsigned char gray;

	for(int r = (argp -> start_row); r < (argp -> end_row); ++r){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + r);
		for(int c = 0; c < (argp -> ncols); ++c){
			red = ((rowp + c) -> red);
			green = ((rowp + c) -> green);
			blue = ((rowp + c) -> blue);

			gray = (red + green + blue) / 3;

			((rowc + c) -> red) = gray;
			((rowc + c) -> green) = gray;
			((rowc + c) -> blue) = gray;
		}	
	}
	return NULL;
}

void *contrastCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;
	unsigned char red, green, blue;
	unsigned char redLow, redHigh, greenLow, greenHigh, blueLow, blueHigh;
	unsigned char redScale, greenScale, blueScale;

	// get the lows and highs
	getExtremes((argp -> image), (argp -> nrows), (argp -> ncols), &redLow, &redHigh, &greenLow, &greenHigh, &blueLow, &blueHigh);
	// compute the scales
			
	redScale = 255 / (redHigh - redLow);
	greenScale = 255 / (greenHigh - greenLow);
	blueScale = 255 / (blueHigh - blueLow);	

	// apply the scales
	for(int r = (argp -> start_row); r < (argp -> end_row); ++r){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + r);
		for(int c = 0; c < (argp -> ncols); ++c){
			red = ((rowp + c) -> red);
			green = ((rowp + c) -> green);
			blue = ((rowp + c) -> blue);

			((rowc + c) -> red) = (red - redLow) * redScale;
			((rowc + c) -> green) = (green - greenLow) * greenScale;
			((rowc + c) -> blue) = (blue - blueLow) * blueScale;
		}	
	}
	return NULL;	
}

void *negativeCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;
	unsigned char red, green, blue;

	for(int r = (argp -> start_row); r < (argp -> end_row); ++r){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + r);
		for(int c = 0; c < (argp -> ncols); ++c){
			red = ((rowp + c) -> red);
			green = ((rowp + c) -> green);
			blue = ((rowp + c) -> blue);

			((rowc + c) -> red) = 255 - red;
			((rowc + c) -> green) = 255 - green;
			((rowc + c) -> blue) = 255 - blue;
		}	
	}
	return NULL;
}

void *flipCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;

	unsigned char red, green, blue;
	for(int r = (argp -> start_row); r < (argp -> end_row); ++r){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + ((argp -> nrows) - (r + 1)));
		for(int c = 0; c < (argp -> ncols); ++c){
			red = ((rowp + c) -> red);
			green = ((rowp + c) -> green);
			blue = ((rowp + c) -> blue);

			((rowc + ((argp -> ncols) - (c + 1))) -> red) = red;
			((rowc + ((argp -> ncols) - (c + 1))) -> green) = green;
			((rowc + ((argp -> ncols) - (c + 1))) -> blue) = blue;
		}	
	}
	return NULL;
}

void *blurCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;
	unsigned char red, green, blue;

	int kernelWidth = 7;
	double kernel[7][7] = {
		{0.000215, 0.001217, 0.003450, 0.004882, 0.003450, 0.001217, 0.000215},
		{0.001217, 0.006909, 0.019580, 0.027708, 0.019580, 0.006909, 0.001217},
		{0.003450, 0.019580, 0.055488, 0.078523, 0.055488, 0.019580, 0.003450},
		{0.004882, 0.027708, 0.078523, 0.111120, 0.078523, 0.027708, 0.004882},
		{0.003450, 0.019580, 0.055488, 0.078523, 0.055488, 0.019580, 0.003450},
		{0.001217, 0.006909, 0.019580, 0.027708, 0.019580, 0.006909, 0.001217},
		{0.000215, 0.001217, 0.003450, 0.004882, 0.003450, 0.001217, 0.000215},
	};	

	for(int r = (argp -> start_row); r < (argp -> end_row); r++){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + r);
			
		for(int c = 0; c < (argp -> ncols); c++){
			red = 0;
			green = 0;
			blue = 0;

			for(int k = 0; k < kernelWidth; k++){
				//pixel8 *rowk = *(image + r);
				for(int l = 0; l < kernelWidth; l++){
					int offset = l - 3;
					red += abs(((rowp + c + offset) -> red) * kernel[k][l]);
					green += abs(((rowp + c + offset) -> green) * kernel[k][l]);
					blue += abs(((rowp + c + offset) -> blue) * kernel[k][l]);
				}	
			}

			((rowc + c) -> red) = red;
			((rowc + c) -> green) = green;
			((rowc + c) -> blue) = blue;					
		}
	}
	return NULL;
}

void *unsharpCopy(void *args){
	WorkerArgs *argp = (WorkerArgs *)args;

	unsigned char red, green, blue;
	unsigned char redDiff, greenDiff, blueDiff;	

	int kernelWidth = 7;
	double kernel[7][7] = {
		{0.000215, 0.001217, 0.003450, 0.004882, 0.003450, 0.001217, 0.000215},
		{0.001217, 0.006909, 0.019580, 0.027708, 0.019580, 0.006909, 0.001217},
		{0.003450, 0.019580, 0.055488, 0.078523, 0.055488, 0.019580, 0.003450},
		{0.004882, 0.027708, 0.078523, 0.111120, 0.078523, 0.027708, 0.004882},
		{0.003450, 0.019580, 0.055488, 0.078523, 0.055488, 0.019580, 0.003450},
		{0.001217, 0.006909, 0.019580, 0.027708, 0.019580, 0.006909, 0.001217},
		{0.000215, 0.001217, 0.003450, 0.004882, 0.003450, 0.001217, 0.000215},
	};	
	
	for(int r = (argp -> start_row); r < (argp -> end_row); r++){
		pixel8 *rowp = *((argp -> image) + r);
		pixel8 *rowc = *((argp -> copy) + r);
		
		for(int c = 0; c < (argp -> ncols); c++){
			red = 0;
			green = 0;
			blue = 0;

			for(int k = 0; k < kernelWidth; k++){
				//pixel8 *rowk = *(image + r);
				for(int l = 0; l < kernelWidth; l++){
					int offset = l - 3;
					red += abs(((rowp + c + offset) -> red) * kernel[k][l]);
					green += abs(((rowp + c + offset) -> green) * kernel[k][l]);
					blue += abs(((rowp + c + offset) -> blue) * kernel[k][l]);
				}	
			}
			
			redDiff = abs(((rowp + c) -> red) - red);
			greenDiff = abs(((rowp + c) -> green) - green);
			blueDiff = abs(((rowp + c) -> blue) - blue);

			((rowc + c) -> red) = (((rowp + c) -> red) + redDiff) > 255 ? 255 : (((rowp + c) -> red) + redDiff);
			((rowc + c) -> green) = (((rowp + c) -> green) + greenDiff) > 255 ? 255 : (((rowp + c) -> green) + greenDiff);
			((rowc + c) -> blue) = (((rowp + c) -> blue) + blueDiff) > 255 ? 255 : (((rowp + c) -> blue) + blueDiff);
		}
	}
	return NULL;
}

int main(int argc, char **argv){
		char helpmsg[] = "usage %s <srcfile> <dstfile>\n";

	if(argc < 3){
		fprintf(stderr, helpmsg, argv[0]);
		return 1;
	}

	char *srcfile, *dstfile, *optionsCode;
	FILE *fo_src, *fo_dst;
	int options = 0;
	
	srcfile = argv[1];
	dstfile = argv[2];

	if((fo_src = fopen(srcfile, "r")) == NULL){
		fprintf(stderr, helpmsg, argv[0]);
		return 2;
	}
	
	if((fo_dst = fopen(dstfile, "w")) == NULL){
		fprintf(stderr, "%s failed to open!\n", dstfile);
		fprintf(stderr, helpmsg, argv[0]);
	}

	if(argc == 4){
		optionsCode = argv[3];
		options = getOption(optionsCode);
	}

	char magic[3];
	int width, height, depth;

	readHeaders(&width, &height, &depth, magic, fo_src, fo_dst);	

        const unsigned colors_per_pixel = 3;
        unsigned n_pixels = width * height;
        unsigned bytes_per_color = ((depth <= 255) ? 1 : 2);
        unsigned bytes_per_pixel = colors_per_pixel * bytes_per_color;

	pixel8 **image = malloc(height * sizeof(pixel8 *));
	for(int p = 0; p < height; ++p){
		pixel8 *listp = malloc(width * sizeof(pixel8));
		*(image + p) = listp;
	}

	// read the image in
		readImageIn(fo_src, image, height, width);	

	// create the 2D array for the copy
	pixel8 **copy = malloc(height * sizeof(pixel8 *));
	for(int p = 0; p < height; ++p){
		pixel8 *listp = malloc(width * sizeof(pixel8));
		*(copy + p) = listp;
	}

	// initialize the pthreads
	WorkerArgs args[NUM_THREADS];
	pthread_t threadinfo[NUM_THREADS];

	double rows_per_thread = height / NUM_THREADS;
	int t;

	for(int t = 0; t < NUM_THREADS; t++){
		args[t].image = image;
		args[t].copy = copy;
		args[t].nrows = height;
		args[t].ncols = width;


		args[t].id = t;
		args[t].start_row = (int)(t * rows_per_thread);
		args[t].end_row = (int)((t + 1) * rows_per_thread);
	}

	// apply the option to the image
	switch(options){
		case 1:
			//lRotate
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, lRotateCopy, &(args[t]));
			}
			lRotateCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}
		break;
		
		case 2:
			//rRotate
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, rRotateCopy, &(args[t]));
			}
			rRotateCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}
		break;
		
		case 3:
			//grey
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, grayCopy, &(args[t]));
			}
			grayCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}
		break;
		
		case 4:
			//contrast
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, contrastCopy, &(args[t]));
			}
			contrastCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}		
		break;

		case 5:
			// negative
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, negativeCopy, &(args[t]));
			}
			negativeCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}
		break;

		case 6:
			// flip
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, flipCopy, &(args[t]));
			}
			flipCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}
		break;

		case 7:
			// blur
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, blurCopy, &(args[t]));
			}
			blurCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}
		break;
		
		case 8:
			// unsharp
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, unsharpCopy, &(args[t]));
			}
			unsharpCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}
		break;
	
		default:
			// classic
			for(t = 1; t < NUM_THREADS; ++t){
				void *attributes = NULL;
				pthread_create( &(threadinfo[t]), attributes, classicCopy, &(args[t]));
			}
			classicCopy(&args);

			for(int i = 1; i < NUM_THREADS; i++){
				void **retval = NULL;
				pthread_join(threadinfo[i], retval);
			}
		break;
	}	

	for(int r = 0; r < height; ++r){
		pixel8 *rowp = *(copy + r);
		fwrite(rowp, bytes_per_pixel, (n_pixels / height), fo_dst);
	}

	fclose(fo_src);
	fclose(fo_dst);

	fprintf(stderr, "Done\n");
}