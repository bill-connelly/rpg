#include <Python.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <string.h>
#include <time.h>
#include <wiringPi.h>
#include <termios.h>
#include <stropts.h>
#include <stdbool.h>
#include <linux/fb.h>

#define ANGLE_0 -1
#define ANGLE_90 -2
#define ANGLE_180 -3
#define ANGLE_270 -4
#define SINE 		0b0001
#define SQUARE 		0b0000
#define RGB888MODE 	0b0010
#define RGB565MODE 	0b0000
#define FULLSCREEN	0b0000
#define CIRCLE		0b0100
#define GABOR		0b1000
#define INSIDEMASK  	0b0000
#define OUTSIDEMASK	0b0100


#define DEGREES_SUBTENDED 80 //The degrees of visual angle
			     // subtended by the screen

typedef struct {
	int framebuffer;
	void * map;
	unsigned int width;
	unsigned int height;
	unsigned int depth; //bits per pixel,
	unsigned int header; //size of initial header in bytes,
	unsigned int size; //size of buffer in bytes.
	unsigned int orig_width;  //These three values store
	unsigned int orig_height; //the screen settings so they
	unsigned int orig_depth;  //can be reset at program termination.
	int error;
} fb_config;

typedef struct {
	uint16_t frames_per_cycle;
	uint16_t spacial_frequency;
	uint16_t temporal_frequency;
	uint16_t frames_per_second;
	uint16_t n_frames;
	uint16_t _padding;
}fileheader_t;

typedef struct {
	// Unecessarily large variable types so that all
	// variables are 4 bytes long
	long int width;
	long int height;
	long int refresh_per_frame;
	long int n_frames;
} fileheader_raw;


typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} uint24_t;

uint16_t rgb_to_uint(int red, int green, int blue){
	/*Convert an rgb value to a 16bit, RGB565
	value*/
	return  (((31*(red + 4))/255)<<11)|
		(((63*(green+2))/255)<< 5)|
		 ((31*(blue +4))/255);
}

uint24_t rgb_to_uint_24bit(int red, int green, int blue){
	/*Convert an rgb value to a 24bit, RBG888
	value.*/
	uint24_t result;
	result.red = red;
	result.green = green;
	result.blue = blue;
	return result;
}

int gcd(int a, int b){
	/*Helper function to get the greatest
	common denominator of 2 ints*/
	if(a==0||b==0){
		return 0;
	}
	//base case
	if(a==b){
		return a;
	}
	if(a>b){
		return gcd(a-b,b);
	}
	return gcd(a,b-a);
}

struct timespec get_current_time(int* status){
	/*The status argument is passed so we can
	write an error code to it in the event of an
	OS failure*/
	struct timespec t;
	if(clock_gettime(CLOCK_REALTIME,&t)){
		*status = -1;
		PyErr_SetString(PyExc_OSError,"Failed realtime clock_gettime call");
	}else{
		*status = 0;
	}
	return t;
}

double degrees_to_radians(double angle){
	/*Converts angle from degrees to
	 * radians, or to a macro negative
	 * value for multiples of pi/4
	 * radians to avoid graphical artifacts*/
	angle = ((int)(angle)%360 + 360)%360;
	switch((int)(angle)){
		case(0):
			angle = ANGLE_0;
			break;
		case(90):
			angle = ANGLE_90;
			break;
		case(180):
			angle = ANGLE_180;
			break;
		case(270):
			angle = ANGLE_270;
			break;
		default:
			angle = (180-angle)*M_PI/180;
	}
	return angle;
}

long cmp_times(struct timespec time1, struct timespec time2){
	/*Compare the elapsed time between two timespec
	structs, returns as integer number of usecs*/
	long long total_nsec1 = time1.tv_nsec + 1000000000*(long long)(time1.tv_sec);
	long long total_nsec2 = time2.tv_nsec + 1000000000*(long long)(time2.tv_sec);
	if(total_nsec1 > total_nsec2) {
		printf("Compare time error: time 2 occoured before from 1");
		exit(1);
	}
	long delta_usecs = (total_nsec2 - total_nsec1)/1000;
	return delta_usecs;
}

int kbhit(void) {
	static const int STDIN = 0;
	static bool is_init = false;

 	if(!is_init) {
		struct termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON;
		tcsetattr(STDIN, TCSANOW, &term);
		setbuf(stdin, NULL);
		is_init = true;
	}

	int bytesWaiting;
	ioctl(STDIN, FIONREAD, &bytesWaiting);
	return bytesWaiting;
}


int int_round(float x) {
	if (x < 0.0) {
		return (int)(x - 0.5);
	} else {
		return (int)(x + 0.5);
	}
}


float mean_long(long a[], int  n) {
	int i;
	long sum = 0;
	for (i = 0; i < n; i++) {
		sum += a[i];
	}
	return ((float) sum)/n;
}

float std_long(long a[], int n) {
	float mean = mean_long(a, n);
	float error_sum = 0;
	float error;
	int i;
	for (i = 0; i < n; i++) {
		error = mean - a[i];
		error_sum += error * error;
	}
	return  (float) sqrt(error_sum/n);
}

int get_refresh_rate(void) {
	int fb = open("/dev/fb0",O_RDWR);
	int n_reps = 11;
	struct timespec times[n_reps];
	int clock_status;
	__u32 dummy = 0;

	int i;
	for (i = 0; i < n_reps; i++) {
		ioctl(fb, FBIO_WAITFORVSYNC, &dummy);
		times[i] = get_current_time(&clock_status);
	}

	close(fb);

	long delta_usecs[n_reps-1];
	for (i = 0; i < n_reps-1; i++) {
		delta_usecs[i] = cmp_times(times[i], times[i+1]);
	}

	int frame_rate = int_round( 1/( mean_long(delta_usecs, n_reps-1)/1000000 ) );
	return frame_rate;
}

double gaussian(int radius, int sigma) {
	return exp( -( (radius * radius) / (double) ( 2*sigma*sigma ) ) );
}


/*This function, as well as the init() function, both heavily
employ the raspberry pi's mailbox property interface to facilitate
communciation with the videocore. For more information, refer to
github.com/raspberrypi/firmware/wiki/Mailbox-property-interface*/

void flip_buffer(int buffer_num, fb_config fb0){
	/* Flip the front- and back-buffers in the double-buffering
	system */

	int fd = open("/dev/vcio",O_RDWR|O_SYNC);
	if(fd == -1){
		perror("VCIO OPEN ERROR: ");
		return 1;
	}

	volatile uint32_t property[32] __attribute__((aligned(16))) = 
	{
	0x00000000,//Buffer size in bytes
	0x00000000,//Request / Response code
	0x48009, //TAG: set vir offset
	8,
	8,
	0,//x offset
	0,//y offset
	0
	};
	property[0] = 8*sizeof(property[0]);
	if(buffer_num != 0){
		property[6] = fb0.height;
	}
	//send request via property interface using ioctl

	if(ioctl(fd, _IOWR(100, 0, char *), property) == -1){
		perror("BUFFER FLIP IOCTL ERROR");
	}
	close(fd);
}

int* get_current_offset(fb_config fb0){
	int fd = open("/dev/vcio",O_RDWR|O_SYNC);
	if(fd == -1){
		perror("VCIO OPEN ERROR: ");
		return NULL;
	}

	volatile uint32_t property[32] __attribute__((aligned(16))) = 
	{
	0x00000000,//Buffer size in bytes
	0x00000000,//Request / Response code
	0x00040009, //TAG: get vir offset
	8,
	8,
	0,//x offset
	0,//y offset
	0
	};
	property[0] = 8*sizeof(property[0]);
	//send request via property interface using ioctl

	if(ioctl(fd, _IOWR(100, 0, char *), property) == -1){
		perror("BUFFER FLIP IOCTL ERROR");
	}
	close(fd);
	int *result = malloc(2*sizeof(int));
	result[0] = property[5];
	result[1] = property[6];
	return result;
}	

void* squarewave(int x, int y, int t, int wavelength, int speed, double angle, double cosine, double sine, double weight, double contrast, int background, int colormode){
	//Returns a (x,y) pixel's brightness for a squarewave
	unsigned short black = 0;
	unsigned short white = 255;
	double brightness;
	double x_prime, int_part, frac_part;
	if(angle == ANGLE_0){
		x_prime = -x + speed*t;
	}else if(angle == ANGLE_90){
		x_prime = y + speed*t;
	}else if(angle==ANGLE_180){
		x_prime = x + speed*t;
	}else if(angle==ANGLE_270){
		x_prime = -y + speed*t;
	}else{
		x_prime = (cosine*x + sine*y) + speed*t;
	}
	frac_part = modf(x_prime,&int_part);
	brightness = ( ((double) ((((int)(int_part))%wavelength + wavelength)%wavelength)+frac_part) / wavelength);
	if(brightness < 0.5){
		brightness = white;
	} else {
		brightness = black;
	}

	brightness = contrast * weight * (brightness-127) + 127;
	void* pixel_ptr;
	if(colormode==RGB888MODE){
		uint24_t* pixel_ptr_24 = malloc(sizeof(uint24_t));
		*pixel_ptr_24 = rgb_to_uint_24bit(brightness, brightness, brightness);
		pixel_ptr = pixel_ptr_24;
	}
	else{
		uint16_t* pixel_ptr_16 = malloc(sizeof(uint16_t));
		*pixel_ptr_16 = rgb_to_uint(brightness, brightness, brightness);
		pixel_ptr = pixel_ptr_16;
	}
	return pixel_ptr;
}





void* sinewave(int x, int y, int t, int wavelength, int speed, double angle, double cosine, double sine, double weight, double contrast, int background, int colormode){
	//Returns a (x,y) pixel's brightness for a sine wave
	double brightness;
	double x_prime;
	if(angle == ANGLE_0){
		x_prime = -x + speed*t;
	}else if(angle == ANGLE_90){
		x_prime = y + speed*t;
	}else if(angle==ANGLE_180){
		x_prime = x + speed*t;
	}else if(angle==ANGLE_270){
		x_prime = -y + speed*t;
	}else{
		x_prime = (cosine*x + sine*y)+(speed*t);
	}

	brightness = contrast * weight * 127 * sin(2*M_PI*(x_prime)/wavelength) + 127;
	int bright_int = brightness;
	void* pixel_ptr;
	if(colormode==RGB888MODE){
		uint24_t* pixel_ptr_24 = malloc(sizeof(uint24_t));

		*pixel_ptr_24 = rgb_to_uint_24bit(bright_int, bright_int, bright_int);
		pixel_ptr = pixel_ptr_24;
	}
	else{
		uint16_t* pixel_ptr_16 = malloc(sizeof(uint16_t));
		*pixel_ptr_16 = rgb_to_uint(bright_int, bright_int, bright_int);
		pixel_ptr = pixel_ptr_16;
	}
	return pixel_ptr;
}


void* gabor(int x, int y, int t, int wavelength, int speed, double angle, double cosine, double sine, double weight, double contrast, int background, int colormode) {
        //Returns a (x,y) pixel's brightness for a gabor patch
        double brightness, x_prime, amplitude;


        if(angle == ANGLE_0){
                x_prime = -x + speed*t;
        }else if(angle == ANGLE_90){
                x_prime = y + speed*t;
        }else if(angle==ANGLE_180){
                x_prime = x + speed*t;
        }else if(angle==ANGLE_270){
                x_prime = -y + speed*t;
        }else{
                x_prime = (cosine*x + sine*y)+(speed*t);
        }
        if (background < 128) {
          amplitude = contrast * weight * background;
        } else {
          amplitude = contrast * weight * (255 - background);
        }
        brightness = amplitude * sin(2*M_PI*(x_prime)/wavelength) + background;
	void* pixel_ptr;
	if(colormode==RGB888MODE){
		uint24_t* pixel_ptr_24 = malloc(sizeof(uint24_t));
		*pixel_ptr_24 = rgb_to_uint_24bit(brightness, brightness, brightness);
		pixel_ptr = pixel_ptr_24;
	}
	else{
		uint16_t* pixel_ptr_16 = malloc(sizeof(uint16_t));
		*pixel_ptr_16 = rgb_to_uint(brightness, brightness, brightness);
		pixel_ptr = pixel_ptr_16;
	}
	return pixel_ptr;
}

void* circle(int waveform, int radius, int padding, int point_radius, int j,int i,int t,int wavelength,int speed, 
		double angle,double cosine, double sine, double weight, double contrast, int background,
		int colormode){
	void* pixel_ptr;
	uint16_t* pixel_ptr_16;
	uint24_t* pixel_ptr_24;
	int circle_case;
//	printf("Weight passed to circle function as %d\n",weight);
	if( point_radius > radius + padding) { 	//outside the circular mask
		circle_case = OUTSIDEMASK;
	}
	else if( point_radius <= radius) { 	//inside the central radius
		circle_case = INSIDEMASK;
		weight = 1;
	} else { 				//In the padding region
//		printf("In padding region, weight is %f\n",weight);
		circle_case = INSIDEMASK;
	}
	switch(circle_case|colormode|waveform){
		case(OUTSIDEMASK|RGB888MODE|SQUARE):
		case(OUTSIDEMASK|RGB888MODE|SINE):
			pixel_ptr_24 = malloc(sizeof(uint24_t));
			*pixel_ptr_24 = rgb_to_uint_24bit(background,background,background);
			break;
		case(OUTSIDEMASK|RGB565MODE|SQUARE):
		case(OUTSIDEMASK|RGB565MODE|SINE):
			pixel_ptr_16 = malloc(sizeof(uint16_t));
			*pixel_ptr_16 = rgb_to_uint(background,background,background);
			break;
		case(INSIDEMASK|RGB888MODE|SQUARE):
			pixel_ptr_24 = squarewave(j,i,t,wavelength,speed,angle,cosine,sine,weight,contrast,background,colormode);
			break;
		case(INSIDEMASK|RGB888MODE|SINE):
			pixel_ptr_24 = sinewave(j,i,t,wavelength,speed,angle,cosine,sine,weight,contrast,background,colormode);
			break;
		case(INSIDEMASK|RGB565MODE|SQUARE):
			pixel_ptr_16 = squarewave(j,i,t,wavelength,speed,angle,cosine,sine,weight,contrast,background,colormode);
			break;
		case(INSIDEMASK|RGB565MODE|SINE):
			pixel_ptr_16 = sinewave(j,i,t,wavelength,speed,angle,cosine,sine,weight,contrast,background,colormode);
	}
	if(colormode==RGB888MODE){
		pixel_ptr = pixel_ptr_24;
	}else{
		pixel_ptr = pixel_ptr_16;
	}
	return pixel_ptr;
}

void * build_frame(int t, double angle, fb_config framebuffer, int wavelength, int speed, int waveform, 
			double contrast, int background, int center_j, int center_i, int sigma, int radius, int padding,
			int colormode){

	angle = degrees_to_radians(angle);
	//Set frame-level flags...
	int grating_type;
	if (radius==0 && sigma==0){
		grating_type = FULLSCREEN;
	}else if(sigma==0){
		grating_type = CIRCLE;
	}else{
		grating_type = GABOR;
	}
	double sine = sin(angle);
	double cosine = cos(angle);
	void* array_start = malloc(framebuffer.size);
	uint24_t *write_location_24, *read_location_24;
	uint16_t *write_location_16, *read_location_16;
	write_location_24 = write_location_16 = array_start;
	int i,j;
	for(i=0;i<framebuffer.height;i++){ //for each row of pixels
		for(j=0;j<framebuffer.width;j++){ //for each column of pixels
			int point_radius = (int) sqrt( ((j-center_j) * (j-center_j)) + ((i-center_i) * (i-center_i)) );
			double gauss_weight = gaussian(point_radius, sigma);
			double circle_weight = padding==0?1:((double)(radius + padding - point_radius)) / padding;
			switch(grating_type|colormode|waveform){
				case(FULLSCREEN|SQUARE|RGB888MODE):
					read_location_24 = squarewave(j,i,t,wavelength,speed,angle,cosine,sine, 1, contrast, background,colormode);
					break;
				case(FULLSCREEN|SQUARE|RGB565MODE):
					read_location_16 = squarewave(j,i,t,wavelength,speed,angle,cosine,sine, 1, contrast, background,colormode);
					break;
				case(FULLSCREEN|SINE|RGB888MODE):
					read_location_24 = sinewave(j,i,t,wavelength,speed,angle,cosine,sine, 1, contrast, background,colormode);
					break;
				case(FULLSCREEN|SINE|RGB565MODE):
					read_location_16 = sinewave(j,i,t,wavelength,speed,angle,cosine,sine, 1, contrast, background,colormode);
					break;
				//Squarewave gabor gratings are not supported
				case(GABOR|SINE|RGB888MODE):
					read_location_24 = gabor(j,i,t,wavelength,speed,angle,cosine,sine, gauss_weight, contrast, background,colormode);
					break;
				case(GABOR|SINE|RGB565MODE):
					read_location_16 = gabor(j,i,t,wavelength,speed,angle,cosine,sine, gauss_weight, contrast, background,colormode);
					break;
				case(CIRCLE|SQUARE|RGB888MODE):
				case(CIRCLE|SINE  |RGB888MODE):
//					printf("Circle_weight in build_frame scope is %f; padding is %d\n",circle_weight,padding);
					read_location_24 = circle(
					waveform, radius, padding, point_radius,j,i,t,wavelength,speed,angle,cosine,sine,circle_weight,contrast,background,colormode);
					break;
				case(CIRCLE|SQUARE|RGB565MODE):
				case(CIRCLE|SINE  |RGB565MODE):
					read_location_16 = circle(waveform, radius, padding, point_radius,j,i,t,wavelength,speed,angle,cosine,sine,circle_weight,contrast,background,colormode);
					break;
				default:
					printf("ERROR:Invalid tags encountered in build_frame funnction.\n");
					return NULL;
			}
			if(colormode == RGB888MODE){
				*write_location_24 = *read_location_24;
				free(read_location_24);
			}else{
				*write_location_16 = *read_location_16;
				free(read_location_16);
			}
			write_location_24++;
			write_location_16++;
			
		}
	}
	//and return a pointer to this pixel data
	return (void *)array_start;
}

int build_grating(char * filename, double duration, double angle, double sf, double tf, double contrast, int background, int width, int height, int waveform, double 
	percent_sigma, double percent_diameter, double percent_center_left, double percent_center_top, double percent_padding, int colormode){ 
	int fps = get_refresh_rate();
        printf("Refresh rate measured as: %d hz\n", fps);
	fb_config fb0;
	fb0.width = width;
	fb0.height = height;
	fb0.depth = (colormode==RGB888MODE)?24:16;
	fb0.size = (fb0.height)*(fb0.depth)*(fb0.width)/8; //8 bits/byte
	FILE * file = fopen(filename, "wb");
	if(file == NULL){
		perror("File creation failed\n");
		PyErr_SetString(PyExc_OSError,"File creation failed.");
		return 1;
	}
	int wavelength = (fb0.width/DEGREES_SUBTENDED)/sf;

	int speed = wavelength*tf/fps;
	if(speed==0){
		speed = 1;
	}
	double actual_tf = ((double)(speed*fps)) / wavelength;
	int sigma = fb0.width * percent_sigma / 100;
	int radius = fb0.width * percent_diameter / 200;
	int center_j = fb0.width * percent_center_left / 100;
	int center_i = fb0.height * percent_center_top / 100;
	double padding = radius * percent_padding / 100;
	if(actual_tf!=tf){
		printf("Grating %s has a requested temporal frequency of %f, actual temporal frequency will be %f\n",filename,tf,actual_tf);
	}
	//Calculate the minimum number of frames required for a full cycle
	//(worst case is just FPS*DURATION) and write it, tf, and sf in a header.
	fileheader_t header;
	header.frames_per_second = fps;
	header.frames_per_cycle = wavelength / gcd(wavelength,speed);
	if(header.frames_per_cycle > fps * duration) {
		header.frames_per_cycle = fps * duration;
	}
	header.n_frames = fps * duration;
	header.spacial_frequency = (uint16_t)(sf);
	header.temporal_frequency = (uint16_t)(tf);
	fwrite(&header,sizeof(fileheader_t),1,file);
	int t, clock_status;
	struct timespec time1, time2;
	time1 = get_current_time(&clock_status);
	if(clock_status){
		return -1;
	}
	uint24_t * frame_24;
	uint16_t * frame_16;
	for (t=0;t<header.frames_per_cycle;t++){
		if(colormode == RGB888MODE){
			frame_24 = build_frame(t,angle,fb0,wavelength, speed, waveform, contrast, background, center_j, center_i, sigma, radius, padding, colormode);
			if(frame_24==NULL){return -1;}
			fwrite(frame_24,sizeof(*frame_24),fb0.height*fb0.width,file);
			free(frame_24);
		}else{
			frame_16 = build_frame(t,angle,fb0,wavelength, speed, waveform, contrast, background, center_j, center_i, sigma, radius, padding, colormode);
			if(frame_16==NULL){return -1;}
			fwrite(frame_16,sizeof(*frame_16),fb0.height*fb0.width,file);		
			free(frame_16);
		}
		if(t==4){
			time2 = get_current_time(&clock_status);
			if(clock_status){
				return -1;
			}
			printf("Expected time to completion: %ld seconds\n",header.frames_per_cycle*cmp_times(time1,time2)/1000000/5);
		}
	}
	fclose(file);
	return 0;
}
void* load_grating(char* filename, fb_config fb0){
	int page_size = getpagesize();
	int bytes_already_read = 0;
	int read_size,frames;
	int colormode = (fb0.depth==24)?RGB888MODE:RGB565MODE;
	int filedes = open(filename, O_RDWR);
	if(filedes == -1){
		perror("Failed to open file");
		return NULL;
	}
	//Start by aquiring the header so we can determine the filesize...
	fileheader_t* header = mmap(NULL,sizeof(fileheader_t),PROT_READ,MAP_PRIVATE,
					filedes,0);
	if(header==MAP_FAILED){
		perror("From mmap for header access");
		exit(1);
	}
	frames = header->frames_per_cycle;
	int file_fps = header->frames_per_second;
	int refresh_rate = get_refresh_rate();
	if (refresh_rate != file_fps) {
		printf("File generated at %d FPS, but monitor running at %d HZ. This will cause inaccurate timing \n", file_fps, refresh_rate);
	}
	int file_size = frames*fb0.size + sizeof(fileheader_t);
	//clean up the header from the heap
	munmap(header, sizeof(fileheader_t));
	//now copy file_size bytes across using mmap
	uint8_t *frame_data;
	frame_data = malloc(file_size);
	while(bytes_already_read < file_size){
		read_size = 20000*page_size;
		if(read_size + bytes_already_read >= file_size){
			read_size = file_size - bytes_already_read;
		}
		printf("Bytes read: %d of %d\nNow reading next %d bytes\n",bytes_already_read,file_size,read_size);
		void* mmap_start = mmap(NULL, read_size,PROT_READ,MAP_PRIVATE,
						filedes,bytes_already_read);
		if(mmap_start == MAP_FAILED){
			perror("From MMAP attempt to read");
			exit(1);
		}
		//Copy read_size bytes across
		memcpy(frame_data+bytes_already_read,mmap_start,read_size);
		bytes_already_read += read_size;
		munmap(mmap_start,read_size);
	}
	printf("Whole file read, closing filedescriptor\n");
	close(filedes);
	return (void*) frame_data;
}

int debug_dump_grating(void* frame_data, fb_config fb0, char* filename){
	/*This is a debugging function that just dumps the first 60
	 * frames of a loaded grating (passed as a void pointer, or from 
	 * python as a grating.capsule object)*/
	uint24_t* frame_data_24 = frame_data;
	FILE * file = fopen(filename, "wb");
	if(file == NULL){
		perror("File creation failed\n");
		PyErr_SetString(PyExc_OSError,"File creation failed.");
		return 1;
	}
	fwrite(frame_data_24,1,fb0.size*60 + sizeof(fileheader_t),file);
	fclose(file);
	return 0;
}


void* load_raw(char* filename) {
	int page_size = getpagesize();
	int bytes_already_read = 0;
	int read_size;
	int fh = open(filename, O_RDWR);
	if(fh == -1) {
		perror("Failed to open file");
		return NULL;
	}
	off_t len = lseek(fh, 0, SEEK_END);
	if (len == -1) {
		printf("Checking File Length Failed.\n");
		return NULL;
	}

	uint8_t *frame_data = malloc(len);
	while(bytes_already_read < len) {
		read_size = 20000*page_size;
		if(read_size + bytes_already_read >= len) {
			read_size = len - bytes_already_read;
		}
		void *mmap_start = mmap(NULL, read_size, PROT_READ,
					    MAP_PRIVATE, fh, bytes_already_read);
		if(mmap_start == MAP_FAILED) {
			perror("MMAP failed to read");
			return NULL;
		}
		memcpy(frame_data+bytes_already_read,mmap_start,read_size);
		bytes_already_read += read_size;
		munmap(mmap_start,read_size);
	}
	close(fh);
	return frame_data;
}

int convert_raw(char* filename, char* new_filename, int n_frames, int width, int height, int refresh_per_frame, int colormode) {

	int fh = open(filename, O_RDWR);
	if (fh == -1) {
		perror("Failed to open file");
		return 1;
	}

	FILE * new_file = fopen(new_filename, "wb");
	if (new_file == NULL) {
		perror("Failed to open new file");
		return 1;
	}

	fileheader_raw header;
	header.n_frames = n_frames;
	header.width = width;
	header.height = height;
	header.refresh_per_frame = refresh_per_frame;
	fwrite(&header, sizeof(fileheader_raw),1,new_file);

	off_t len = lseek(fh, 0, SEEK_END);
	if (len == -1) {
		printf("Checking File Length Failed.\n");
		return 1;
	}
	char *buffer = mmap(0, len, PROT_READ, MAP_PRIVATE, fh, 0);

	if (buffer == MAP_FAILED){
		PyErr_SetString(PyExc_OSError,"MMAP failed");
		return 1;
	}
	int i = 0;
	char r, g, b;
	uint16_t new_pixel_16;
	uint24_t new_pixel_24;
	while (i < len) {
		r = buffer[i];
		g = buffer[i+1];
		b = buffer[i+2];
		i += 3;
		if(colormode==RGB888MODE){
			new_pixel_24 = rgb_to_uint_24bit(r,g,b);
			fwrite(&new_pixel_24,sizeof(uint24_t), 1, new_file);
		}else{
			new_pixel_16 = rgb_to_uint(r,g,b);
			fwrite(&new_pixel_16,sizeof(uint16_t), 1, new_file);
               }
	}
	munmap(buffer, len);
	fclose(new_file);
	close(fh);
	return 0;
}

double* display_raw(void *frame_data, fb_config fb0, int trig_pin, int colormode) {

	pinMode(1, OUTPUT);
	digitalWrite(1, LOW);
	if (trig_pin > 0) {
		pinMode(trig_pin, INPUT);
		while (digitalRead(trig_pin) == 0) {
			if (kbhit()) {
				return 0;
			}
		}
	}
	fileheader_raw* header = frame_data;
	frame_data = header + 1;
	uint24_t * frame_data_24 = frame_data;
	uint16_t * frame_data_16 = frame_data;
	
	uint24_t * write_loc_24 = (uint24_t *)(fb0.map) + fb0.width*fb0.height;
	uint16_t * write_loc_16 = (uint16_t *)(fb0.map) + fb0.width*fb0.height;
	int t, buffer, pixel, clock_status, waits, pixel_size;
	if (colormode == RGB888MODE){pixel_size = sizeof(uint24_t);    }
	else                        {pixel_size = sizeof(uint16_t);    }
	float *frame_duration_mean = malloc(2*sizeof(float));
	float *frame_duration_std = frame_duration_mean+1;
	struct timespec frame_start, frame_end;
	__u32 dummy = 0;

	int n_frames = header -> n_frames;
	int refresh_per_frame = header -> refresh_per_frame;
        long timings[n_frames-1];
	for (t = 0; t < n_frames; t++) {
		frame_end = frame_start;
		frame_start = get_current_time(&clock_status);
		if(clock_status) {
			return NULL;
		}

		buffer = (t+1)%2;
		for(pixel = 0; pixel < fb0.width*fb0.height; pixel++) {
			if(colormode == RGB888MODE){
				*write_loc_24 = frame_data_24[(t*fb0.size/pixel_size)+pixel];
				write_loc_24++;
			}else{
				*write_loc_16 = frame_data_16[(t*fb0.size/pixel_size)+pixel];
				write_loc_16++;
			}
		}
		flip_buffer(buffer, fb0);
		for (waits = 0; waits < refresh_per_frame; waits++) {
			ioctl(fb0.framebuffer, FBIO_WAITFORVSYNC, &dummy);
		}
		if (t != 0) {
			timings[t-1] = cmp_times(frame_end, frame_start);
		}
		if(!buffer){
			digitalWrite(1, LOW);
			write_loc_24 = fb0.map + 3*fb0.width*fb0.height;
			write_loc_16 = fb0.map + 2*fb0.width*fb0.height;
		} else {
			write_loc_24 = fb0.map;
			write_loc_16 = fb0.map;
			digitalWrite(1, HIGH);
		}
	}
	*frame_duration_mean = mean_long(timings, n_frames-1);
	*frame_duration_std = std_long(timings, n_frames-1);
	return frame_duration_mean;
}

double* display_grating(void* frame_data, fb_config fb0, int trig_pin, int colormode){

	pinMode(1, OUTPUT);
	digitalWrite(1, LOW);
	if (trig_pin > 0) {
		pinMode(trig_pin, INPUT);
		while (digitalRead(trig_pin) == 0) {
			if (kbhit()) {
				return 0;
			}
		}
	}

	fileheader_t* header = frame_data;
	frame_data = (header + 1);
	uint24_t * frame_data_24 = frame_data;
	uint16_t * frame_data_16 = frame_data;

	uint24_t * write_loc_24 = (uint24_t *)(fb0.map) + fb0.width*fb0.height;
	uint16_t * write_loc_16 = (uint16_t *)(fb0.map) + fb0.width*fb0.height;

	int t, buffer, pixel, frame, clock_status, pixel_size;
	if (colormode == RGB888MODE){pixel_size = sizeof(uint24_t);    }
	else                        {pixel_size = sizeof(uint16_t);    }

	double* frame_duration_mean = malloc(2*sizeof(double));
	double* frame_duration_std = frame_duration_mean+1;
	struct timespec frame_start, frame_end;
	__u32 dummy = 0;

	int n_frames = header->n_frames;
	long timings[n_frames-1];
	for (t=0; t < n_frames; t++){
                frame_end = frame_start;
                frame_start = get_current_time(&clock_status);
		if(clock_status) {
			return NULL;
		}

		frame = t%(header->frames_per_cycle);
		buffer = (t+1)%2;
		for(pixel = 0; pixel<fb0.width*fb0.height; pixel++){
			if(colormode == RGB888MODE){
				*write_loc_24 = frame_data_24[(frame*fb0.size/pixel_size)+pixel];
				write_loc_24++;
			}else{
				*write_loc_16 = frame_data_16[(frame*fb0.size/pixel_size)+pixel];
				write_loc_16++;
			}
		}
		flip_buffer(buffer, fb0);
		ioctl(fb0.framebuffer, FBIO_WAITFORVSYNC, &dummy);

		if (t != 0) {
			timings[t-1] = cmp_times(frame_end, frame_start);
		}

		if(!buffer){
			digitalWrite(1, LOW);
			write_loc_24 = fb0.map + 3*fb0.width*fb0.height;
			write_loc_16 = fb0.map + 2*fb0.width*fb0.height;
		} else {
			write_loc_24 = fb0.map;
			write_loc_16 = fb0.map;
			digitalWrite(1, HIGH);
		}
	}
	*frame_duration_mean = mean_long(timings, n_frames-1);
	*frame_duration_std = std_long(timings, n_frames-1);
	return frame_duration_mean;
}

int unload_grating(void* frame_data){
	free(frame_data);
	return 0;
}

int unload_raw(uint16_t* raw_data) {
	free(raw_data);
	return 0;
}

int display_color(fb_config fb0,int buffer, uint16_t color_16, uint24_t color_24, int colormode){
	uint16_t *write_loc_16;
	uint24_t *write_loc_24;
	int pixel;
	if(!buffer){
		write_loc_24 = fb0.map + 3*fb0.width*fb0.height;
		write_loc_16 = fb0.map + 2*fb0.width*fb0.height;
	} else {
		write_loc_24 = fb0.map;
		write_loc_16 = fb0.map;
	}
	for(pixel = 0; pixel<fb0.width*fb0.height; pixel++){
		if(colormode == RGB888MODE){
			*write_loc_24 = color_24;
			write_loc_24++;
		}else{
			*write_loc_16 = color_16;
			write_loc_16++;
		}
	}
	flip_buffer(buffer,fb0);
	return 0;
}

int is_current_resolution(int xres, int yres){
	int fd = open("/dev/vcio",0);
	if(fd == -1){
		PyErr_SetString(PyExc_OSError,"Could not open /dev/vcio device");
		return -1;
	}
	volatile uint32_t property[32] __attribute__((aligned(16))) = 
	{
	0x00000000,
	0x00000000,
	0x00040003,
	0x00000008,
	0x00000000,
	0x00000000, //Width response read from/written to here
	0x00000000, //Height response read from/written to here
	0x00000000 //terminal null element
	};
	property[0] = 8*sizeof(property[0]);
	if(ioctl(fd, _IOWR(100,0,char*), property) == -1){
		PyErr_SetString(PyExc_OSError,"IOCTL call failed when attempting to check resolution");
		return -1;
	}
	return ((property[5] == xres)&&(property[6]==yres));
}

fb_config init(int width, int height, int colormode){
	wiringPiSetup();

	fb_config fb0;
	//To determine original width and height
	//a mailbox property interface request is
	//performed.
	int fd = open("/dev/vcio",0);
	if(fd == -1){
		perror("From open() call on /dev/vcio device");
		exit(1);
	}
	volatile uint32_t property[32] __attribute__((aligned(16))) = 
	{
	0x00000000,//size of request
	0x00000000,//Buffer request/response code
	0x00040003,//code for get virtual buffer width and height
	0x00000008,//(size of requst argument in bytes)
	0x00000000,//tag's request code
	0x00000000,//width response will be written here by videocore
	0x00000000,//height response will be written here by videocore
	0x00040005,//code for get depth
	0x00000004,//size of request
	0x00000000,//tag's request code
	0x00000000,//depth response will be written here by videocore
	0x00000000,
	0x00000000 //terminal null element
	};
	property[0] = 12*sizeof(property[0]);
	if(ioctl(fd, _IOWR(100, 0, char *), property) == -1){
		PyErr_SetString(PyExc_OSError,"Error from call to ioctl\n");
		fb0.error = 1;
		return fb0;
	}
	close(fd);
	fb0.orig_width = (int)(property[5]);
	fb0.orig_height = (int)(property[6]);
	fb0.orig_depth = (int)(property[10]);
	fb0.width = width;
	fb0.height = height;
	if(colormode == RGB888MODE){
		fb0.depth = 24;
	}else{
		fb0.depth = 16;
	}
	fb0.size = (fb0.height)*(fb0.depth)*(fb0.width)/8;
	char fbset_str[80];
	sprintf(fbset_str,
		"fbset -xres %d -yres %d -vxres %d -vyres %d -depth %d",
		fb0.width, fb0.height, fb0.width, 2*fb0.height, fb0.depth);
	if(system(fbset_str)){
		PyErr_SetString(PyExc_OSError,"Call to fbset subroutine failed.");
		fb0.error = 1;
		return fb0;
	}
	int resolution_status = is_current_resolution(width,height);
	if(resolution_status == 0){
		printf("The linux framebuffer does not support the requested resolution\n"
			"Attepting to reset resolution settings...\n");
		sprintf(fbset_str,
			"fbset -xres %d -yres %d -vxres %d -vyres %d -depth %d",
			fb0.orig_width, fb0.orig_height, fb0.orig_width, 
			fb0.orig_height, fb0.orig_depth);
		if(system(fbset_str)){
			perror("Attempt failed, message from fbset");
		}
		else{
			printf("Attempt successful.\n");
		}
		PyErr_SetString(PyExc_OSError,"Requested resolution not supported");
		fb0.error = 1;
		return fb0;
	}else if(resolution_status == -1){
		fb0.error = 1;
		return fb0;
	}
	fb0.framebuffer = open("/dev/fb0",O_RDWR);
	if (fb0.framebuffer == -1){
		PyErr_SetString(PyExc_OSError,"Attempt to open /dev/fb0 (framebuffer 0) device failed");
		fb0.error = 1;
		return fb0;
	}
	fb0.map = mmap(0,2*fb0.size,PROT_READ|PROT_WRITE, MAP_SHARED, fb0.framebuffer, 0);
	if (fb0.map == MAP_FAILED){
		PyErr_SetString(PyExc_OSError,"Attempt to mmap /dev/fb0 device failed");
		fb0.error = 1;
		return fb0;
	}
	fb0.error = 0;
	return fb0;
}

int close_display(fb_config fb0){
	munmap(fb0.map,2*fb0.size);
	char fbset_str[80];
	sprintf(fbset_str,
		"fbset -xres %d -yres %d -vxres %d -vyres %d -depth %d",
		fb0.orig_width, fb0.orig_height, fb0.orig_width, fb0.orig_height,
		fb0.orig_depth);
	if(system(fbset_str)){
		PyErr_SetString(PyExc_OSError,"System call to reset resolution (via fbset subroutine) failed");
		return 1;
	}
	return 0;
}




/*----------------------------------------------------*/
/* Python Module Implementation                       */
/*----------------------------------------------------*/

static PyObject* py_buildgrating(PyObject *self, PyObject *args) {
    char* filename;
    double duration, angle, sf, tf, contrast, percent_sigma, percent_diameter,
           percent_center_left, percent_center_top, percent_padding;
    int width, height, waveform, background, colormode;
    if (!PyArg_ParseTuple(args, "sdddddiiiidddddi", &filename, &duration, &angle,
                          &sf, &tf, &contrast, &background, &width, &height, &waveform,
                          &percent_sigma, &percent_diameter, &percent_center_left,
			  &percent_center_top, &percent_padding,&colormode)){
        return NULL;
    }
    if(build_grating(filename,duration,angle,sf,tf,contrast,background,width,height,waveform,
			percent_sigma, percent_diameter,percent_center_left,
			percent_center_top, percent_padding,colormode)){
        return NULL;
    }
    Py_RETURN_NONE; 
}



static PyObject* py_init(PyObject *self, PyObject *args) {
    int xres,yres,colormode;
    if (!PyArg_ParseTuple(args, "iii", &xres, &yres,&colormode)) {
        return NULL;
    }
    fb_config* fb0_pointer = malloc(sizeof(fb_config)); 
    *fb0_pointer = init(xres,yres,colormode);
    if(fb0_pointer->error){
        return NULL;
    }
    PyObject* fb0_capsule = PyCapsule_New(fb0_pointer, "framebuffer",NULL);
    Py_INCREF(fb0_capsule);
    return fb0_capsule;
}

static PyObject* py_displaycolor(PyObject* self, PyObject* args){
    PyObject* fb0_capsule;
    int r,g,b,colormode;
        if (!PyArg_ParseTuple(args, "Oiiii", &fb0_capsule,&r,&g,&b,&colormode)) {
        return NULL;
    }
    uint16_t color_16 = rgb_to_uint(r,g,b);
    uint24_t color_24 = rgb_to_uint_24bit(r,g,b);
    fb_config* fb0_pointer = PyCapsule_GetPointer(fb0_capsule,"framebuffer");
    display_color(*fb0_pointer,1,color_16,color_24,colormode);
    display_color(*fb0_pointer,0,color_16,color_24,colormode);
    Py_RETURN_NONE;
}


static PyObject* py_loadgrating(PyObject* self, PyObject* args){
    PyObject* fb0_capsule;
    char* filename;
        if (!PyArg_ParseTuple(args, "Os", &fb0_capsule,&filename)) {
        return NULL;
    }
    fb_config* fb0_pointer = PyCapsule_GetPointer(fb0_capsule,"framebuffer");
    void* grating_data = load_grating(filename,*fb0_pointer);
    if (grating_data == NULL) {
        PyErr_Format(PyExc_FileNotFoundError, "You probably mistyped the file name. Parsed as %s", filename);
 	return NULL;
    }
    PyObject* grating_capsule = PyCapsule_New(grating_data, "grating_data",NULL);
    Py_INCREF(grating_capsule);
    return grating_capsule;
}

static PyObject* py_debugdumpgrating(PyObject* self, PyObject* args){
    PyObject* fb0_capsule;
    PyObject* grating_capsule;
    char* filename;
    if (!PyArg_ParseTuple(args, "OOs", &fb0_capsule,&grating_capsule,&filename)) {
        return NULL;
    }
    fb_config* fb0_pointer = PyCapsule_GetPointer(fb0_capsule,"framebuffer");
    void* grating_data = PyCapsule_GetPointer(grating_capsule,"grating_data");
    if(grating_data == NULL){
        return NULL;
    }
    if (debug_dump_grating(grating_data,*fb0_pointer,filename)){
	    return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject* py_loadraw(PyObject* self, PyObject* args){
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename)) {
        return NULL;
    }
    void* raw_data = load_raw(filename);
    if (raw_data == NULL) {
        PyErr_Format(PyExc_FileNotFoundError, "You probably mistyped the file name. Parsed as %s", filename);
        return NULL;
    }
    PyObject* raw_capsule = PyCapsule_New(raw_data, "raw_data", NULL);
    Py_INCREF(raw_capsule);
    return raw_capsule;
}

static PyObject* py_unloadgrating(PyObject* self, PyObject* args){
    PyObject* grating_capsule;
    void* grating_pointer;
    if (!PyArg_ParseTuple(args, "O", &grating_capsule)) {
        return NULL;
    }
    grating_pointer = PyCapsule_GetPointer(grating_capsule,"grating_data");
    unload_grating(grating_pointer);
    Py_DECREF(grating_capsule);
    Py_RETURN_NONE;
}

static PyObject* py_unloadraw(PyObject* self, PyObject* args) {
    PyObject* raw_capsule;
    void* raw_pointer;
    if (!PyArg_ParseTuple(args, "O", &raw_capsule)) {
        return NULL;
    }
    raw_pointer = PyCapsule_GetPointer(raw_capsule, "raw_data");
    unload_raw(raw_pointer);
    Py_DECREF(raw_capsule);
    Py_RETURN_NONE;
}

static PyObject* py_displaygrating(PyObject* self, PyObject* args){
    PyObject* fb0_capsule;
    PyObject* grating_capsule;
    int trig_pin;
    if (!PyArg_ParseTuple(args, "OOi", &fb0_capsule,&grating_capsule,&trig_pin)) {
        return NULL;
    }
    fb_config* fb0_pointer = PyCapsule_GetPointer(fb0_capsule,"framebuffer");
    void* grating_data = PyCapsule_GetPointer(grating_capsule,"grating_data");
    int colormode;
    if(fb0_pointer->depth==24){
//	printf("DEBUG: Displaying 24-bit grating");
        colormode = RGB888MODE;
    }else{
        colormode = RGB565MODE;
    }
    if(grating_data == NULL){
        return NULL;
    }
    int start_time = time(NULL);
    double* grat_info = display_grating(grating_data,*fb0_pointer,trig_pin,colormode);
    if (grat_info == NULL) {
        free(grat_info);
        Py_RETURN_NONE;
    } else {
        PyObject* return_tuple = Py_BuildValue("(ddi)",*grat_info,*(grat_info+1),start_time);
        free(grat_info);
        return return_tuple;
    }
}

static PyObject* py_displayraw(PyObject* self, PyObject* args){
    PyObject* fb0_capsule;
    PyObject* raw_capsule;
    int trig_pin;
    if (!PyArg_ParseTuple(args, "OOi", &fb0_capsule, &raw_capsule, &trig_pin)) {
        return NULL;
    }
    fb_config* fb0_pointer = PyCapsule_GetPointer(fb0_capsule, "framebuffer");
    void* raw_data = PyCapsule_GetPointer(raw_capsule, "raw_data");
    if(raw_data == NULL){
        return NULL;
    }
    int colormode;    
    if(fb0_pointer->depth==24){
//	printf("DEBUG: Displaying 24-bit grating");
        colormode = RGB888MODE;
    }else{
        colormode = RGB565MODE;
    }
    int start_time = time(NULL);
    float* raw_info = display_raw(raw_data, *fb0_pointer, trig_pin, colormode);
    if (raw_info == 0) {
        free(raw_info);
        Py_RETURN_NONE;
    } else {
        PyObject* return_tuple = Py_BuildValue("(ddi)", *raw_info, *(raw_info+1), start_time);
        free(raw_info);
        return return_tuple;
    }
}

static PyObject* py_closedisplay(PyObject* self, PyObject* args){
    PyObject* fb0_capsule;
        if (!PyArg_ParseTuple(args, "O", &fb0_capsule)) {
        return NULL;
    }
    fb_config* fb0_pointer = PyCapsule_GetPointer(fb0_capsule,"framebuffer");
    if(close_display(*fb0_pointer)){
        return NULL;
    }
    Py_DECREF(fb0_capsule);
    Py_RETURN_NONE;
}


static PyObject* py_convertraw(PyObject* self, PyObject* args){
	char *filename, *new_filename;
	int n_frames, width, height, refresh_per_frame, colormode;
	if (!PyArg_ParseTuple(args, "ssiiiii", &filename, &new_filename,
				&n_frames, &width, &height, &refresh_per_frame,
				&colormode)) {
		return NULL;
	}
	if(convert_raw(filename, new_filename, n_frames, width, height, refresh_per_frame, colormode)) {
		return NULL;
	}
	Py_RETURN_NONE;
}


static PyMethodDef _rpigratings_methods[] = { 
    {   
        "init", py_init, METH_VARARGS,
        "Initialise the display and return a framebuffer object.\n"
	":Param xres:  the virtual width of the display\n"
	":Param yres:  the virtual height of the display\n"
	":Param depth: the number of bits per pixel - 16 or 24\n"
	":rtype framebuffer capsule: a framebuffer object for use\n"
	"with other functions in this module.\n"
	"WARNING: only one instance of this object should\n"
	"exist at any one time. Framebuffer objects should\n"
	"be cleaned up by being passed to close_display().\n\n"
	"Since init modifies the screen settings, it is a good\n"
	"idea to try and catch all exceptions, call \n"
	"close_display, and then re-raise the exeption like this:\n\n"
        " >>> root = init(1680,1050) \n"
        " >>> for file in os.listdir():\n"
        " >>>     try:\n"
        " >>>         load_grating(root, file)\n"
        " >>>         display_grating(root, file)\n"
        " >>>         unload_grating(root, file)\n"
        " >>>     except:\n"
        " >>>         close_grating(root)\n"
        " >>>         raise\n",
        " >>> close_grating(root)\n",
    },  
    {   
        "display_color", py_displaycolor, METH_VARARGS,
        "Display a rbg color to the framebuffer.\n"
	":Param fb0: a framebuffer object returned from init()\n"
	":Param r: the red component of the color (int)\n"
	":Param b: the blue component of the color (int)\n"
	":Param g: the green component of the color (int)\n"
	":rtype None:"
    },  
{   
        "display_grating", py_displaygrating, METH_VARARGS,
        "Displays data that has been loaded into memory to the screen.\n"
	":Param fb0: a framebuffer object created from an init() call\n"
	":Param data: a raw data object created from a load_grating() call\n"
	":rtype None:"
    },
{   
        "debug_dump_grating", py_debugdumpgrating, METH_VARARGS,
        "Dumps a loaded grating in filename.\n"
	":Param fb0: a framebuffer object created from an init() call\n"
	":Param data: a raw data object created from a load_grating() call\n"
	":Param filename: file to dump the grating in.\n"
	":rtype None:"
    },
{
	"display_raw", py_displayraw, METH_VARARGS,
	":rtype None:"
},  
    {   
        "build_grating", py_buildgrating, METH_VARARGS,
        "Creates a raw animation file of a drifting grating.\n"
	":Param filename:\n"
    	":Param angle: angle of propogation of the drifting grating (in\n"
	"      degrees anticlockwise from the x-axis).\n"
    	":Param sf: Spacial frequency in cycles per degree of visual angle\n"
	":Param tf: Temporal frequency in cycles per second\n"
    	":Param width: X component of the desired resolution\n"
	":Param height: Y component of the desired resolution\n"
	":Param waveform: SINE or SQUARE\n"
	":Param percent_diameter: 0 for full screen or width of circlular mask\n"
	":rtype None:\n\n"
	"NOTE: the resolution of this file must match the resolution used\n"
	"in init() calls that are used to display this file."
    },  
    {   
        "load_grating", py_loadgrating, METH_VARARGS,
        "Loads a raw animation file into memory for use with\n"
	"The display_grating function\n"
	":Param fb0: a framebuffer object returned from init()\n"
	":Param filename: (string) the raw data file to be loaded\,\n"
	"      typically created with a draw_grating call.\n"
	":rtype grating_data capsule: The raw data object."
    },
    {
	"load_raw", py_loadraw, METH_VARARGS,
	":rtype raw_data capsule"
    },  
    {   
        "unload_grating", py_unloadgrating, METH_VARARGS,
        "Unloads raw animation data, freeing the assosiated memory\n"
	":Param data: a grating_data object returned from load_grating()\n"
	":rtype None:"
    },  
   {
	"unload_raw", py_unloadraw, METH_VARARGS,
	":rtype None:"
   }, 
   {   
        "close_display", py_closedisplay, METH_VARARGS,
        "Destroy/uninitialise a framebuffer object.\n"
	":Param fb0: an initialised framebuffer object\n"
	":rtype None:"
    },  
    {   
	"convertraw", py_convertraw, METH_VARARGS,
	"fillertext\n"
	":type None:"
    },
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef _rpigratings_definition = { 
    PyModuleDef_HEAD_INIT,
    "_rpigratings",
    "A Python module that displays drifting gratings\n"
    "on Raspberry Pis. This module should be used from\n"
    "the terminal rather than a windowing system for\n"
    "intended behaviour. The terminal can be accessed\n"
    "with Ctrl+Alt+F1 on raspbian (Ctrl+Alt+F7 will return\n"
    "to the windowing system).\n\n"
    "Typical usage:\n"
    " >>> import _rpigratings as rg\n"
    " >>> #First create a grating file...\n"
    " >>> rg.draw_grating(\"grat_file\",60,0.5,3,1680,1050)\n"
    " >>> #Then initialise the display...\n"
    " >>> root = rg.init(1680,1050)\n"
    " >>> #Then display midgrey while loading our grating file\n"
    " >>> rg.display_color(root,127,127,127)\n"
    " >>> my_grating = rg.load_grating(root,\"grat_file\")\n"
    " >>> #Now display the loaded grating...\n"
    " >>> rg.diplay_grating(root, my_grating)\n"
    " >>> #Free the memory assosiated with the grating...\n"
    " >>> rg.unload_grating(my_grating)\n"
    " >>> #Remember: uninitialise module to restore screen settings\n"
    " >>> rg.close_display(root)\n",
    -1, 
    _rpigratings_methods
};


PyMODINIT_FUNC PyInit__rpigratings(void) {
    Py_Initialize();
    return PyModule_Create(&_rpigratings_definition);
}
