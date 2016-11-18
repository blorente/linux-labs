#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define DRIVER_PATH "/dev/usb/blinkstick0"
#define NR_LEDS 8
#define INTERVAL 8000000
#define BRIGHTNESS_COEFFICIENT 10 // Have the intensity of the colors
#define PI 3.14159265
#define MAX_MESSAGE_LEN 100

void print_usage( void ) {
	printf("Usage: ./blink <led> <repo>\nExample: ./ledctl 0 /path/to/my_repo\n");
}

int compose_message(unsigned int *colors, char *target) {
	sprintf(target, "0:0x%.6x,1:0x%.6x,2:0x%.6x,3:0x%.6x,4:0x%.6x,5:0x%.6x,6:0x%.6x,7:0x%.6x", 
		colors[0],
		colors[1],
		colors[2],
		colors[3],
		colors[4],
		colors[5],
		colors[6],
		colors[7]);
	return strlen(target) + 1;
}

int write_to_stick(int driver_file, char *message, int len) {	
	if (lseek(driver_file, 1, SEEK_SET) < 0) {
		printf("Seek operation failed\n");
		return -1;
	}

	if (write(driver_file, message, len) < 0) {
		printf("Write operation failed\n");
		return -1;
	}
	return 0;
}

void init_starts(double step, double *starts) {
	int i;
	for(i = 0; i < NR_LEDS; i++) {
		starts[i] = step * i;
	}
}

void update_intensities(double elapsed, double *starts, double *intensities) {
	int i;
	for(i = 0; i < NR_LEDS; i++) {
		double raw = sin((elapsed - starts[i]));
		intensities[i] = raw * raw;
	}
}

unsigned int get_color(unsigned int base_shift, double intensity) {
	unsigned int color = 0xFF * (intensity / BRIGHTNESS_COEFFICIENT);
	return (0xFFFFFF & (color << base_shift));
}

void update_colors(int base_shift, double *intensities, unsigned int *colors) {
	int i;
	for(i = 0; i < NR_LEDS; i++) {
		colors[i] = get_color(base_shift, intensities[i]);
	}
}

int main(int argc, char** argv) {
	double step = INTERVAL / NR_LEDS;
	double elapsed;
	double intensities[NR_LEDS];
	double starts[NR_LEDS];
	unsigned int colors[NR_LEDS];
	int base_shift = 0; // BLUE color for now

	char message[MAX_MESSAGE_LEN];
	int message_len;

	int driver_file;
	driver_file = open(DRIVER_PATH, O_RDWR);
	if (driver_file <= 0) {
		printf("Open operation failed\n");
		return -1;
	}

	printf("Step: %fms\n", step);

	init_starts(step, starts);

	while(1) {
		update_intensities(elapsed, starts, intensities);

		printf("Intensities: %f %f %f %f %f %f %f %f\n", 
			intensities[0],
			intensities[1],
			intensities[2],
			intensities[3],
			intensities[4],
			intensities[5],
			intensities[6],
			intensities[7]);

		update_colors(base_shift, intensities, colors);

		printf("Colors: %x %x %x %x %x %x %x %x\n", 
			colors[0],
			colors[1],
			colors[2],
			colors[3],
			colors[4],
			colors[5],
			colors[6],
			colors[7]);

		message_len = compose_message(colors, message);

		printf("Composed Message: %s\n", message);

		if (write_to_stick(driver_file, message, message_len) != 0) {
			perror("Something went wrong");
			close(driver_file);
			return 1;
		}

		elapsed += step;
		usleep(step);
	}

	close(driver_file);
		
	return 0;
}