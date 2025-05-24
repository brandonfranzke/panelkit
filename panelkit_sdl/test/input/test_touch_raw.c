/**
 * @file test_touch_raw.c
 * @brief Raw touch input test using Linux input event system
 * 
 * This utility tests touch input directly from Linux input devices
 * without SDL, helping isolate driver vs application issues.
 * 
 * Usage: ./test_touch_raw [device_path]
 * Example: ./test_touch_raw /dev/input/event0
 * 
 * If no device is specified, it will scan for touch devices automatically.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>

/* Bit manipulation macros for input events */
#define BITS_PER_LONG (sizeof(long) * 8)
#define NLONGS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define test_bit(bit, array) ((array)[(bit)/BITS_PER_LONG] & (1UL << ((bit) % BITS_PER_LONG)))

static volatile int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
    printf("\nShutting down...\n");
}

void print_device_info(int fd, const char* device_path) {
    char name[256] = "Unknown";
    struct input_id device_id;
    
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        strcpy(name, "Unknown");
    }
    
    printf("Device: %s\n", device_path);
    printf("Name: %s\n", name);
    
    if (ioctl(fd, EVIOCGID, &device_id) >= 0) {
        printf("ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n",
               device_id.bustype, device_id.vendor, 
               device_id.product, device_id.version);
    }
    printf("\n");
}

int is_touch_device(int fd) {
    unsigned long evbit[NLONGS(EV_MAX)] = {0};
    unsigned long absbit[NLONGS(ABS_MAX)] = {0};
    
    // Check if device supports EV_ABS (absolute events)
    if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evbit) < 0) {
        return 0;
    }
    
    if (!test_bit(EV_ABS, evbit)) {
        return 0;
    }
    
    // Check if device supports touch events (ABS_X, ABS_Y)
    if (ioctl(fd, EVIOCGBIT(EV_ABS, ABS_MAX), absbit) < 0) {
        return 0;
    }
    
    return test_bit(ABS_X, absbit) && test_bit(ABS_Y, absbit);
}

char* find_touch_device() {
    DIR* input_dir;
    struct dirent* entry;
    char device_path[256];
    int fd;
    static char found_device[256];
    
    input_dir = opendir("/dev/input");
    if (!input_dir) {
        printf("Error: Cannot open /dev/input directory\n");
        return NULL;
    }
    
    printf("Scanning for touch devices...\n");
    
    while ((entry = readdir(input_dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }
        
        snprintf(device_path, sizeof(device_path), "/dev/input/%s", entry->d_name);
        
        fd = open(device_path, O_RDONLY);
        if (fd < 0) {
            continue;
        }
        
        char name[256] = "Unknown";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        
        if (is_touch_device(fd)) {
            printf("Found touch device: %s (%s)\n", device_path, name);
            strcpy(found_device, device_path);
            close(fd);
            closedir(input_dir);
            return found_device;
        }
        
        close(fd);
    }
    
    closedir(input_dir);
    printf("No touch devices found\n");
    return NULL;
}

void print_touch_event(struct input_event* ev, int* current_x, int* current_y, int* touch_active) {
    static struct timespec start_time = {0};
    struct timespec current_time;
    double elapsed;
    
    // Initialize start time on first event
    if (start_time.tv_sec == 0) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    elapsed = (current_time.tv_sec - start_time.tv_sec) + 
              (current_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    
    switch (ev->type) {
        case EV_ABS:
            switch (ev->code) {
                case ABS_X:
                    *current_x = ev->value;
                    printf("[%8.3f] ABS_X: %d\n", elapsed, ev->value);
                    break;
                case ABS_Y:
                    *current_y = ev->value;
                    printf("[%8.3f] ABS_Y: %d\n", elapsed, ev->value);
                    break;
                case ABS_PRESSURE:
                    printf("[%8.3f] PRESSURE: %d\n", elapsed, ev->value);
                    break;
                case ABS_MT_SLOT:
                    printf("[%8.3f] MT_SLOT: %d\n", elapsed, ev->value);
                    break;
                case ABS_MT_TRACKING_ID:
                    if (ev->value == -1) {
                        printf("[%8.3f] TOUCH_UP (tracking_id: %d)\n", elapsed, ev->value);
                        *touch_active = 0;
                    } else {
                        printf("[%8.3f] TOUCH_DOWN (tracking_id: %d)\n", elapsed, ev->value);
                        *touch_active = 1;
                    }
                    break;
                case ABS_MT_POSITION_X:
                    *current_x = ev->value;
                    printf("[%8.3f] MT_X: %d\n", elapsed, ev->value);
                    break;
                case ABS_MT_POSITION_Y:
                    *current_y = ev->value;
                    printf("[%8.3f] MT_Y: %d\n", elapsed, ev->value);
                    break;
                default:
                    printf("[%8.3f] ABS_%d: %d\n", elapsed, ev->code, ev->value);
                    break;
            }
            break;
            
        case EV_KEY:
            switch (ev->code) {
                case BTN_TOUCH:
                    printf("[%8.3f] BTN_TOUCH: %s\n", elapsed, ev->value ? "PRESS" : "RELEASE");
                    *touch_active = ev->value;
                    break;
                case BTN_TOOL_FINGER:
                    printf("[%8.3f] BTN_TOOL_FINGER: %s\n", elapsed, ev->value ? "PRESS" : "RELEASE");
                    break;
                default:
                    printf("[%8.3f] KEY_%d: %s\n", elapsed, ev->code, ev->value ? "PRESS" : "RELEASE");
                    break;
            }
            break;
            
        case EV_SYN:
            if (ev->code == SYN_REPORT) {
                printf("[%8.3f] --- SYNC --- Position: (%d, %d) %s\n", 
                       elapsed, *current_x, *current_y, 
                       *touch_active ? "TOUCHING" : "NOT_TOUCHING");
            }
            break;
            
        default:
            printf("[%8.3f] TYPE_%d CODE_%d VALUE_%d\n", elapsed, ev->type, ev->code, ev->value);
            break;
    }
}

int main(int argc, char* argv[]) {
    const char* device_path;
    int fd;
    struct input_event ev;
    int current_x = 0, current_y = 0, touch_active = 0;
    
    printf("PanelKit Raw Touch Input Test\n");
    printf("=============================\n\n");
    
    // Setup signal handler for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Determine device path
    if (argc > 1) {
        device_path = argv[1];
        printf("Using specified device: %s\n\n", device_path);
    } else {
        device_path = find_touch_device();
        if (!device_path) {
            printf("Usage: %s [device_path]\n", argv[0]);
            printf("Example: %s /dev/input/event0\n", argv[0]);
            printf("\nAvailable input devices:\n");
            system("ls -la /dev/input/event* 2>/dev/null || echo 'No event devices found'");
            return 1;
        }
        printf("\n");
    }
    
    // Open device
    fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        printf("Error: Cannot open %s: %s\n", device_path, strerror(errno));
        printf("Try running as root: sudo %s\n", argv[0]);
        return 1;
    }
    
    // Print device information
    print_device_info(fd, device_path);
    
    // Verify it's a touch device
    if (!is_touch_device(fd)) {
        printf("Warning: %s may not be a touch device\n\n", device_path);
    }
    
    printf("Touch the screen to see input events. Press Ctrl+C to exit.\n");
    printf("Events will show coordinates and timing information.\n\n");
    
    // Main event loop
    while (running) {
        ssize_t bytes = read(fd, &ev, sizeof(ev));
        
        if (bytes < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by signal
            }
            printf("Error reading from device: %s\n", strerror(errno));
            break;
        }
        
        if (bytes != sizeof(ev)) {
            printf("Warning: Partial event read\n");
            continue;
        }
        
        print_touch_event(&ev, &current_x, &current_y, &touch_active);
    }
    
    close(fd);
    printf("Touch test completed.\n");
    return 0;
}