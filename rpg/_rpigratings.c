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


#define ADJUSTMENT 5.747e-4
#define ANGLE_0 -1
#define ANGLE_90 -2
#define ANGLE_180 -3
#define ANGLE_270 -4
#define SINE 1
#define SQUARE 0
#define FPS 60 //The target FPS for animations
#define DURATION 1 //The duration of an animation.
#define FRAMES DURATION*FPS
#define DEGREES_SUBTENDED 80 //The degrees of visual angle
			     // subtended by the screen

typedef struct {
	int framebuffer;
	uint16_t * map;
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
}fileheader_t;

typedef struct {
	// Unecessarily large variable types so that all
	// variables are 4 bytes long
	long int n_frames;
	long int width;
	long int height;
	float fps;
} fileheader_raw;

uint16_t rgb_to_uint(int red, int green, int blue){
	/*Convert an rgb value to a 16bit, RGB565
	value*/
	return  (((31*(red + 4))/255)<<11)|
		(((63*(green+2))/255)<< 5)|
		 ((31*(blue +4))/255);
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

int cmp_times(struct timespec time1, struct timespec time2){
	/*Compare the elapsed time between two timespec
	structs, returns as integer number of msecs*/
	int delta_secs = time2.tv_sec - time1.tv_sec;
	long delta_nsecs = time2.tv_nsec - time1.tv_nsec;
	int delta_msecs = delta_secs*1000000 + delta_nsecs/1000;
	return delta_msecs;
}

int kbhit() {
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




/*This function, as well as the init() function, both heavily
employ the raspberry pi's mailbox property interface to facilitate
communciation with the videocore. For more information, refer to
github.com/raspberrypi/firmware/wiki/Mailbox-property-interface*/

int flip_buffer(int buffer_num, fb_config fb0){
	/* Flip the front- and back-buffers in the double-buffering
	system */
	int fd = open("/dev/vcio",0);
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
	return 0;
}


uint16_t squarewave(int x, int y, int t, int wavelength, int speed, double angle, double cosine, double sine, double weighting){
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
	brightness = (127 - brightness) * weighting + 127;

	return rgb_to_uint(brightness, brightness, brightness);
}



uint16_t sinewave(int x, int y, int t, int wavelength, int speed, double angle, double cosine, double sine, double weighting){
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

	brightness = weighting * (127*sin(2*M_PI*(x_prime)/wavelength)) + 127;

	return rgb_to_uint(brightness,brightness,brightness);
}

uint16_t * build_frame(int t, double angle, fb_config framebuffer, int wavelength, int speed, int waveform, int center_j, int center_i, int radius, double padding){
	angle = ((int)(angle)%360 + 360)%360;
	if(angle==0){
		angle = ANGLE_0;
	}
	else if(angle==90){
		angle = ANGLE_90;
	}
	else if(angle==180){
		angle = ANGLE_180;
	}
	else if(angle==270){
		angle = ANGLE_270;
	}
	else{
		angle = (180-angle)*M_PI/180;
	}
	double sine = sin(angle);
	double cosine = cos(angle);
	uint16_t * array_start;
	array_start = malloc(framebuffer.size);
	uint16_t* write_location = array_start;
	int i,j;
	for(i=0;i<framebuffer.height;i++){ //for each row of pixels
		for(j=0;j<framebuffer.width;j++){ //for each column of pixels
			//set each pixel's brightness
			if( radius == 0) { //if we have no radius and are doing fullscreen
				if(waveform==SQUARE){
					*write_location = squarewave(j,i,t,wavelength,speed,angle,cosine,sine, 1);
				}else if(waveform==SINE){
					*write_location = sinewave(j,i,t,wavelength,speed,angle,cosine,sine, 1);
				}
			} else { //if we have a radius and hence aren't doing full screen
	                        int point_radius = (int) sqrt( ((j-center_j) * (j-center_j)) + ((i-center_i) * (i-center_i)) );
				if( point_radius > radius + padding) { //if we are outside the circular mask
					*write_location = rgb_to_uint(127,127,127);
				}else if( point_radius <= radius) { //if we are inside the central radius
					if(waveform==SQUARE) {
						*write_location = squarewave(j,i,t,wavelength,speed,angle,cosine,sine, 1);
					}else if(waveform==SINE) {
						*write_location = sinewave(j,i,t,wavelength,speed,angle,cosine,sine, 1);
					}
				} else { //we must be in the padding region
					double weight = (radius + padding - point_radius) / padding;
					if(waveform==SQUARE) {
						*write_location = squarewave(j,i,t,wavelength,speed,angle,cosine,sine, weight);
					}else if (waveform==SINE){
						*write_location = sinewave(j,i,t,wavelength,speed,angle,cosine,sine, weight);
					}
				}
			}
			write_location++;
		}
	}
	//and return a pointer to this pixel data
	return array_start;
}





int build_grating(char * filename, double angle, double sf, double tf, int width, int height, int waveform, int percent_diameter, int percent_center_left, int percent_center_top, int percent_padding, int verbose){
	fb_config fb0;
	fb0.width = width;
	fb0.height = height;
	fb0.depth = 16;
	fb0.size = (fb0.height)*(fb0.depth)*(fb0.width)/8;
	FILE * file = fopen(filename, "wb");
	if(file ==NULL){
		perror("File creation failed\n");
		PyErr_SetString(PyExc_OSError,"File creation failed.");
		return 1;
	}
	printf("Drawing grating %s\n", filename);
	int wavelength = (fb0.width/DEGREES_SUBTENDED)/sf;
	int speed = wavelength*tf/FPS;
	if(speed==0){
		speed = 1;
	}
	double actual_tf = ((double)(speed*FPS)) / wavelength;
	int radius = fb0.width*(percent_diameter) / 200;
	int center_j = fb0.width * percent_center_left / 100;
	int center_i = fb0.height * percent_center_top / 100;
	double padding = radius * percent_padding / 100;
	if(verbose&&actual_tf!=tf){
		printf("Grating %s has a requested temporal frequency of %f,\n" 
			"Actual temporal frequency will be %f (%f% requested)\n"
			"To stop receving these warnings, set verbose arg to False.\n"
			"(You might want to do this if you are building many gratings)\n",filename,tf,actual_tf,100*actual_tf/tf);
	}
	//Calculate the minimum number of frames required for a full cycle
	//(worst case is just FPS*DURATION) and write it, tf, and sf in a header.
	fileheader_t header;
	header.frames_per_cycle = wavelength / gcd(wavelength,speed);
	printf("Frame per cycle is %d\n",header.frames_per_cycle);
	header.spacial_frequency = (uint16_t)(sf);
	header.temporal_frequency = (uint16_t)(tf);
	fwrite(&header,sizeof(fileheader_t),1,file);
	int t, clock_status;
	struct timespec time1, time2;
	time1 = get_current_time(&clock_status);
	if(clock_status){
			return -1;
	}
	for (t=0;t<header.frames_per_cycle;t++){
		uint16_t* frame = build_frame(t,angle,fb0,wavelength, speed, waveform, center_j, center_i, radius, padding);
		fwrite(frame,sizeof(uint16_t),fb0.height*fb0.width,file);
		free(frame);
		if(t==4){
			time2 = get_current_time(&clock_status);
			if(clock_status){
				return -1;
			}
			printf("Expected time to completion: %d seconds\n",header.frames_per_cycle*cmp_times(time1,time2)/1000000/5);
		}
	}
	fclose(file);
	return 0;
}

uint16_t* load_grating(char* filename, fb_config fb0){
	int page_size = getpagesize();
	int bytes_already_read = 0;
	int read_size,frames;
	int filedes;
	filedes = open(filename, O_RDWR);
	if(filedes == -1){
		perror("Failed to open file");
		return NULL;
	}
	//Start by aquiring the header so we can determine the filesize...
	uint16_t* header = mmap(NULL,6,PROT_READ,MAP_PRIVATE,
					filedes,0);
	if(header==MAP_FAILED){
		perror("From mmap for header access");
		exit(1);
	}
	frames = header[0];
	int file_size = frames*fb0.size + sizeof(fileheader_t);
	//clean up the header from the heap
	munmap(header, 6);
	//now copy file_size bytes across using mmap
	uint16_t *frame_data = malloc(file_size);
	while(bytes_already_read < file_size){
		read_size = 20000*page_size;
		if(read_size + bytes_already_read >= file_size){
			read_size = file_size - bytes_already_read;
		}
		uint16_t *mmap_start = mmap(NULL, read_size,PROT_READ,MAP_PRIVATE,
						filedes,bytes_already_read);
		if(mmap_start == MAP_FAILED){
			perror("From MMAP attempt to read");
			exit(1);
		}
		//Copy read_size bytes across
		memcpy(frame_data+(bytes_already_read/2),mmap_start,read_size);
		bytes_already_read += read_size;
		munmap(mmap_start,read_size);
	}
	close(filedes);
	return frame_data;
}


uint16_t* load_raw(char* filename) {
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

	uint16_t *frame_data = malloc(len);
	while(bytes_already_read < len) {
		read_size = 20000*page_size;
		if(read_size + bytes_already_read >= len) {
			read_size = len - bytes_already_read;
		}
		uint16_t *mmap_start = mmap(NULL, read_size, PROT_READ,
					    MAP_PRIVATE, fh, bytes_already_read);
		if(mmap_start == MAP_FAILED) {
			perror("MMAP failed to read");
			return NULL;
		}
		memcpy(frame_data+(bytes_already_read/2),mmap_start,read_size);
		bytes_already_read += read_size;
		munmap(mmap_start,read_size);
	}
	close(fh);
	return frame_data;
}

int convert_raw(char* filename, char* new_filename, int n_frames, int width, int height, float fps) {

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
	header.fps = fps;
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
	uint16_t new_byte;
	while (i < len) {
		r = buffer[i];
		g = buffer[i++];
		b = buffer[i++];
		i++;
		new_byte = rgb_to_uint(r,g,b);
		fwrite(&new_byte,sizeof(uint16_t), 1, new_file);
	}
	munmap(buffer, len);
	fclose(new_file);
	close(fh);
	return 0;
}

double* display_raw(uint16_t *frame_data, fb_config fb0, int trig_pin) {

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
	frame_data += sizeof(fileheader_raw)/sizeof(float);

	uint16_t *write_loc;
	int t, buffer, pixel, frame, clock_status;
	double time;
	write_loc = fb0.map + fb0.size/2;
	double *fastest_frame = malloc(2*sizeof(double));
	double *slowest_frame = fastest_frame+1;
	*slowest_frame = 0;
	*fastest_frame = 1000000;
	struct timespec frame_start = get_current_time(&clock_status);
	struct timespec frame_end;
	if(!clock_status){
		return NULL;
	}
	int n_frames = header -> n_frames;
	float raw_FPS = header -> fps;
	for (t = 0; t < n_frames; t++) {
		frame = t;
		buffer = t%2;
		for(pixel = 0; pixel < fb0.size/2; pixel++) {
			*write_loc = frame_data[(frame*fb0.size/2)+pixel];
			write_loc++;
		}

		frame_end = get_current_time(&clock_status);
		time = cmp_times(frame_start,frame_end);
		if(clock_status){
			return NULL;
		}
		if(time+ADJUSTMENT<(CLOCKS_PER_SEC/raw_FPS)) {
			usleep((CLOCKS_PER_SEC/raw_FPS)-time-ADJUSTMENT);
		} else {
			PyErr_SetString(PyExc_TimeoutError, "A frame was too slow");
			return NULL;
		}
		frame_end = get_current_time(&clock_status);
		if(clock_status){
			return NULL;
		}
		time = cmp_times(frame_start,frame_end);
		if(time>(*slowest_frame) && t != 0) {
			*slowest_frame = (double)(time);
		}
		if(time<(*fastest_frame)){
			*fastest_frame = (double)(time);
		}
		flip_buffer(buffer, fb0);
		frame_start = get_current_time(&clock_status);
		if(clock_status){
			return NULL;
		}
		if(buffer) {
			write_loc = fb0.map + fb0.size/2;
			digitalWrite(1, HIGH);
		} else {
			write_loc = fb0.map;
			digitalWrite(1, LOW);

		}
	}
	*fastest_frame = 1000000/(*fastest_frame);
	*slowest_frame = 1000000/(*slowest_frame);
	return fastest_frame;
}

double* display_grating(uint16_t* frame_data, fb_config fb0, int trig_pin){

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
	frame_data += sizeof(fileheader_t)/sizeof(uint16_t);
	uint16_t *write_loc;
	int t,buffer,pixel,frame, clock_status;
	double time;
	//We want to write to the second buffer
	write_loc = fb0.map + fb0.size/2;
	double* fastest_frame = malloc(2*sizeof(double));
	double* slowest_frame = fastest_frame+1;
	*slowest_frame  = 0;
	*fastest_frame = 1000000;
	struct timespec frame_start = get_current_time(&clock_status);
	if(clock_status) {
		return NULL;
	}
	struct timespec frame_end;
	for (t=0;t<FRAMES;t++){
	//Play each frame by copying data to the memory-mapped area
		frame = t%(header->frames_per_cycle);
		buffer = t%2;
		for(pixel = 0;pixel<fb0.size/2;pixel++){
			*write_loc = frame_data[(frame*fb0.size/2)+pixel];
			write_loc++;
		}
		if(t!=0) {
			frame_end = get_current_time(&clock_status);
			if (clock_status){
				return NULL;
			}
			time = cmp_times(frame_end,frame_start);

			if(time+ADJUSTMENT<(CLOCKS_PER_SEC/FPS)){
				usleep((CLOCKS_PER_SEC/FPS)-time-ADJUSTMENT);
			}else{
				PyErr_SetString(PyExc_TimeoutError,"Frame was too slow");
				return NULL;
			}

			time = cmp_times(frame_start,frame_end);
			if(time>(*slowest_frame) && t!=0){
				*slowest_frame = (double)(time);
			}
                	if(time<(*fastest_frame)){
				*fastest_frame = (double)(time);
			}
		}
		flip_buffer(buffer,fb0);
		frame_start = get_current_time(&clock_status);
		if(clock_status) {
			return NULL;
		}

		if(buffer){
			write_loc = fb0.map + fb0.size/2;
			digitalWrite(1, LOW);
		} else {
			write_loc = fb0.map;
			digitalWrite(1, HIGH);
		}

	}
	*fastest_frame = 1000000 /(*fastest_frame);
	*slowest_frame = 1000000/(*slowest_frame);
	return fastest_frame;
}

int unload_grating(uint16_t* frame_data){
	free(frame_data);
	return 0;
}

int unload_raw(uint16_t* raw_data) {
	free(raw_data);
	return 0;
}

int display_color(fb_config fb0,int buffer, uint16_t color){
	uint16_t *write_loc;
	int pixel;
	if(buffer){
		write_loc = fb0.map + fb0.size/2;
	}else{
		write_loc = fb0.map;
	}
	for(pixel = 0;pixel<fb0.size/2;pixel++){
		*write_loc = color;
		write_loc++;
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


fb_config init(int width, int height){
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
	fb0.depth = 16;
	fb0.size = (fb0.height)*(fb0.depth)*(fb0.width)/8;
	char fbset_str[80];
	sprintf(fbset_str,
		"fbset -xres %d -yres %d -vxres %d -vyres %d -depth 16",
		fb0.width, fb0.height, fb0.width, 2*fb0.height);
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
	fb0.map = (uint16_t *)(mmap(0,2*fb0.size,PROT_READ|PROT_WRITE, MAP_SHARED, fb0.framebuffer, 0));
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
    double angle, sf, tf;
    int width, height, waveform, percent_diameter, percent_center_left,
	percent_center_top, percent_padding, verbose;
    if (!PyArg_ParseTuple(args, "sdddiiiiiiip", &filename, &angle,
                          &sf, &tf, &width, &height, &waveform,
                          &percent_diameter, &percent_center_left,
			  &percent_center_top, &percent_padding,
			  &verbose)){
        return NULL;
    }
    if(build_grating(filename,angle,sf,tf,width,height,waveform,
			percent_diameter,percent_center_left,
			percent_center_top,percent_padding,verbose)){
        return NULL;
    }
    Py_RETURN_NONE;
}



static PyObject* py_init(PyObject *self, PyObject *args) {
    int xres,yres;
    if (!PyArg_ParseTuple(args, "ii", &xres, &yres)) {
        return NULL;
    }
    fb_config* fb0_pointer = malloc(sizeof(fb_config)); 
    *fb0_pointer = init(xres,yres);
    if(fb0_pointer->error){
        return NULL;
    }
    PyObject* fb0_capsule = PyCapsule_New(fb0_pointer,
                                          "framebuffer",NULL);
    Py_INCREF(fb0_capsule);
    return fb0_capsule;
}

static PyObject* py_displaycolor(PyObject* self, PyObject* args){
    PyObject* fb0_capsule;
    int r,g,b;
        if (!PyArg_ParseTuple(args, "Oiii", &fb0_capsule,&r,&g,&b)) {
        return NULL;
    }
    fb_config* fb0_pointer = PyCapsule_GetPointer(fb0_capsule,"framebuffer");
    display_color(*fb0_pointer,1,rgb_to_uint(r,g,b));
    display_color(*fb0_pointer,0,rgb_to_uint(r,g,b));
    Py_RETURN_NONE;
}


static PyObject* py_loadgrating(PyObject* self, PyObject* args){
    PyObject* fb0_capsule;
    char* filename;
        if (!PyArg_ParseTuple(args, "Os", &fb0_capsule,&filename)) {
        return NULL;
    }
    fb_config* fb0_pointer = PyCapsule_GetPointer(fb0_capsule,"framebuffer");
    uint16_t* grating_data = load_grating(filename,*fb0_pointer);
    if (grating_data == NULL) {
        PyErr_SetString(PyExc_FileNotFoundError, "You probably mistyped the file name.	Though stranger things may have happened");
 	return NULL;
    }
    PyObject* grating_capsule = PyCapsule_New(grating_data, "grating_data",NULL);
    Py_INCREF(grating_capsule);
    return grating_capsule;
}

static PyObject* py_loadraw(PyObject* self, PyObject* args){
	char* filename;
	if (!PyArg_ParseTuple(args, "s", &filename)) {
		return NULL;
	}
	uint16_t* raw_data = load_raw(filename);
	if (raw_data == NULL) {
		PyErr_SetString(PyExc_FileNotFoundError, "You probably mistyped the file name.	Though stranger things may have happened");
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
    uint16_t* grating_data = PyCapsule_GetPointer(grating_capsule,"grating_data");
    if(grating_data == NULL){
        return NULL;
    }
    int start_time = time(NULL);
    double* grat_info = display_grating(grating_data,*fb0_pointer,trig_pin);
    if (grat_info == 0) {
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
	uint16_t* raw_data = PyCapsule_GetPointer(raw_capsule, "raw_data");
        if(raw_data == NULL){
                return NULL;
	}
	int start_time = time(NULL);
	double* raw_info = display_raw(raw_data, *fb0_pointer, trig_pin);
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
	int n_frames, width, height;
	float fps;
	if (!PyArg_ParseTuple(args, "ssiiif", &filename, &new_filename,
				&n_frames, &width, &height, &fps)) {
		return NULL;
	}
	if(convert_raw(filename, new_filename, n_frames, width, height, fps)) {
		return NULL;
	}
	Py_RETURN_NONE;
}


static PyMethodDef _rpigratings_methods[] = { 
    {   
        "init", py_init, METH_VARARGS,
        "Initialise the display and return a framebuffer object.\n"
	":Param xres: the virtual width of the display\n"
	":Param yres: the virtual height of the display\n"
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
