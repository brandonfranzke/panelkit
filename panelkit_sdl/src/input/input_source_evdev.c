/**
 * @file input_source_evdev.c
 * @brief Linux evdev input source implementation
 * 
 * Reads input events directly from Linux /dev/input/event* devices
 * and converts them to SDL events. This is necessary when SDL's
 * video driver (like offscreen) doesn't receive input events.
 */

#include "input_handler.h"
#include "../core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/ioctl.h>

/* Define BTN_TOUCH if not available */
#ifndef BTN_TOUCH
#define BTN_TOUCH 0x14a
#endif

/* Maximum number of simultaneous touch points to track */
#define MAX_TOUCH_POINTS 10

/* Helper macros for bit operations */
#define NBITS(x) (((x)/BITS_PER_LONG)+1)
#define BITS_PER_LONG (sizeof(long)*8)
#define test_bit(bit, array) ((array[(bit)/BITS_PER_LONG] >> ((bit)%BITS_PER_LONG)) & 1)

/* Touch point tracking */
typedef struct {
    int id;           /* Tracking ID from kernel */
    bool active;      /* Currently touching */
    bool sent_down;   /* Have we sent SDL_FINGERDOWN for this touch */
    int x, y;         /* Current position */
    float pressure;   /* Pressure if available */
} TouchPoint;

/* Private implementation data */
typedef struct {
    /* Device handling */
    int fd;                          /* Device file descriptor */
    char device_path[256];           /* Device path */
    pthread_t read_thread;           /* Input reading thread */
    bool thread_running;             /* Thread control flag */
    
    /* Device information */
    struct input_absinfo abs_x;      /* X axis info */
    struct input_absinfo abs_y;      /* Y axis info */
    struct input_absinfo abs_pressure; /* Pressure info (optional) */
    bool has_pressure;               /* Pressure support */
    
    /* Touch tracking */
    TouchPoint touches[MAX_TOUCH_POINTS];
    int current_slot;                /* Current MT slot */
    
    /* Parent handler for event delivery */
    InputHandler* handler;
    
    /* Configuration */
    bool auto_detect;                /* Auto-detect device */
    bool mouse_emulation;            /* Emulate mouse from touch */
} EvdevData;

/* Forward declarations */
static void* evdev_read_thread(void* arg);
static bool find_touch_device(char* path, size_t path_size);
static bool open_device(EvdevData* data, const char* path);
static void close_device(EvdevData* data);
static void process_touch_event(EvdevData* data, const struct input_event* ev);
static void inject_sdl_touch_event(EvdevData* data, Uint32 type, int id, float x, float y, float pressure);

/* Initialize the input source */
static bool evdev_initialize(InputSource* source, const InputConfig* config) {
    EvdevData* data = (EvdevData*)source->impl;
    
    /* Store configuration */
    data->auto_detect = config->auto_detect_devices;
    data->mouse_emulation = config->enable_mouse_emulation;
    
    /* Initialize touch tracking */
    memset(data->touches, 0, sizeof(data->touches));
    data->current_slot = 0;
    
    /* Determine device path */
    if (config->device_path) {
        /* Use specified device */
        strncpy(data->device_path, config->device_path, sizeof(data->device_path) - 1);
        data->device_path[sizeof(data->device_path) - 1] = '\0';
    } else if (config->auto_detect_devices) {
        /* Auto-detect touch device */
        if (!find_touch_device(data->device_path, sizeof(data->device_path))) {
            log_error("No touch device found");
            return false;
        }
        log_info("Auto-detected touch device: %s", data->device_path);
    } else {
        log_error("No device path specified and auto-detect disabled");
        return false;
    }
    
    /* Open the device */
    if (!open_device(data, data->device_path)) {
        return false;
    }
    
    return true;
}

/* Start input processing */
static bool evdev_start(InputSource* source, InputHandler* handler) {
    EvdevData* data = (EvdevData*)source->impl;
    
    data->handler = handler;
    data->thread_running = true;
    
    log_info("Starting evdev input source with device: %s", data->device_path);
    log_info("Mouse emulation: %s", data->mouse_emulation ? "enabled" : "disabled");
    
    /* Create reading thread */
    if (pthread_create(&data->read_thread, NULL, evdev_read_thread, source) != 0) {
        log_error("Failed to create evdev read thread: %s", strerror(errno));
        data->thread_running = false;
        return false;
    }
    
    log_info("Evdev input source started successfully");
    return true;
}

/* Stop input processing */
static void evdev_stop(InputSource* source) {
    EvdevData* data = (EvdevData*)source->impl;
    
    if (data->thread_running) {
        data->thread_running = false;
        pthread_join(data->read_thread, NULL);
        log_info("Evdev input source stopped");
    }
}

/* Get input capabilities */
static bool evdev_get_capabilities(InputSource* source, InputCapabilities* caps) {
    EvdevData* data = (EvdevData*)source->impl;
    
    if (!caps || data->fd < 0) {
        return false;
    }
    
    caps->has_touch = true;
    caps->has_mouse = data->mouse_emulation;
    caps->has_keyboard = false;
    caps->max_touch_points = MAX_TOUCH_POINTS;
    caps->touch_x_min = data->abs_x.minimum;
    caps->touch_x_max = data->abs_x.maximum;
    caps->touch_y_min = data->abs_y.minimum;
    caps->touch_y_max = data->abs_y.maximum;
    
    return true;
}

/* Cleanup and destroy */
static void evdev_cleanup(InputSource* source) {
    EvdevData* data = (EvdevData*)source->impl;
    
    /* Stop thread if running */
    if (data->thread_running) {
        evdev_stop(source);
    }
    
    /* Close device */
    close_device(data);
    
    /* Free implementation data */
    free(data);
    source->impl = NULL;
}

/* Input reading thread */
static void* evdev_read_thread(void* arg) {
    InputSource* source = (InputSource*)arg;
    EvdevData* data = (EvdevData*)source->impl;
    struct input_event ev;
    
    log_info("Evdev read thread started for %s", data->device_path);
    
    while (data->thread_running) {
        ssize_t bytes = read(data->fd, &ev, sizeof(ev));
        
        if (bytes == sizeof(ev)) {
            process_touch_event(data, &ev);
        } else if (bytes < 0) {
            if (errno == EINTR) {
                continue; /* Interrupted, retry */
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* No data available (non-blocking mode) */
                usleep(1000); /* Sleep 1ms to avoid busy loop */
                continue;
            } else if (errno == ENODEV) {
                log_error("Input device disconnected");
                break;
            } else {
                log_error("Error reading input device: %s", strerror(errno));
                break;
            }
        }
        
        /* Small sleep to avoid consuming too much CPU */
        usleep(100); /* 0.1ms */
    }
    
    log_info("Evdev read thread exiting");
    return NULL;
}

/* Process touch events */
static void process_touch_event(EvdevData* data, const struct input_event* ev) {
    switch (ev->type) {
        case EV_ABS:
            switch (ev->code) {
                case ABS_MT_SLOT:
                    /* Switch to different touch slot */
                    data->current_slot = ev->value;
                    if (data->current_slot >= MAX_TOUCH_POINTS) {
                        data->current_slot = MAX_TOUCH_POINTS - 1;
                    }
                    log_debug("Touch slot changed to %d", data->current_slot);
                    break;
                    
                case ABS_MT_TRACKING_ID:
                    /* Touch up/down */
                    if (ev->value == -1) {
                        /* Touch up */
                        TouchPoint* touch = &data->touches[data->current_slot];
                        if (touch->active) {
                            float norm_x = (float)(touch->x - data->abs_x.minimum) / 
                                         (data->abs_x.maximum - data->abs_x.minimum);
                            float norm_y = (float)(touch->y - data->abs_y.minimum) / 
                                         (data->abs_y.maximum - data->abs_y.minimum);
                            
                            log_info("Touch UP: slot=%d id=%d pos=(%.3f,%.3f)", 
                                    data->current_slot, touch->id, norm_x, norm_y);
                            
                            inject_sdl_touch_event(data, SDL_FINGERUP, touch->id, 
                                                 norm_x, norm_y, 0.0f);
                            
                            touch->active = false;
                            touch->id = -1;
                            touch->sent_down = false;  // Reset for next touch
                        }
                    } else {
                        /* Touch down */
                        TouchPoint* touch = &data->touches[data->current_slot];
                        touch->id = ev->value;
                        touch->active = true;
                        touch->pressure = 1.0f;
                        touch->sent_down = false;  // Will send on SYN_REPORT
                        
                        log_info("Touch DOWN detected: slot=%d id=%d", 
                                data->current_slot, touch->id);
                    }
                    break;
                    
                case ABS_MT_POSITION_X:
                    data->touches[data->current_slot].x = ev->value;
                    break;
                    
                case ABS_MT_POSITION_Y:
                    data->touches[data->current_slot].y = ev->value;
                    break;
                    
                case ABS_MT_PRESSURE:
                    if (data->has_pressure) {
                        float pressure = (float)(ev->value - data->abs_pressure.minimum) /
                                       (data->abs_pressure.maximum - data->abs_pressure.minimum);
                        data->touches[data->current_slot].pressure = pressure;
                    }
                    break;
                    
                /* Handle simple touch protocol (non-MT) for compatibility */
                case ABS_X:
                    data->touches[0].x = ev->value;
                    if (!data->touches[0].active) {
                        data->touches[0].active = true;
                        data->touches[0].id = 0;
                        data->touches[0].sent_down = false;
                        log_info("Simple touch protocol: touch detected");
                    }
                    break;
                    
                case ABS_Y:
                    data->touches[0].y = ev->value;
                    break;
            }
            break;
            
        case EV_KEY:
            /* Handle BTN_TOUCH for simple touch protocol */
            if (ev->code == BTN_TOUCH) {
                if (ev->value == 0 && data->touches[0].active) {
                    /* Touch released */
                    float norm_x = (float)(data->touches[0].x - data->abs_x.minimum) / 
                                 (data->abs_x.maximum - data->abs_x.minimum);
                    float norm_y = (float)(data->touches[0].y - data->abs_y.minimum) / 
                                 (data->abs_y.maximum - data->abs_y.minimum);
                    
                    log_info("Simple touch UP: pos=(%.3f,%.3f)", norm_x, norm_y);
                    inject_sdl_touch_event(data, SDL_FINGERUP, 0, norm_x, norm_y, 0.0f);
                    
                    data->touches[0].active = false;
                    data->touches[0].sent_down = false;
                }
            }
            break;
            
        case EV_SYN:
            if (ev->code == SYN_REPORT) {
                /* End of event group - send any pending events */
                for (int i = 0; i < MAX_TOUCH_POINTS; i++) {
                    TouchPoint* touch = &data->touches[i];
                    if (touch->active && touch->id >= 0) {
                        float norm_x = (float)(touch->x - data->abs_x.minimum) / 
                                     (data->abs_x.maximum - data->abs_x.minimum);
                        float norm_y = (float)(touch->y - data->abs_y.minimum) / 
                                     (data->abs_y.maximum - data->abs_y.minimum);
                        
                        /* Send motion or down event */
                        if (!touch->sent_down) {
                            log_info("Sending FINGERDOWN: slot=%d id=%d pos=(%.3f,%.3f)", 
                                    i, touch->id, norm_x, norm_y);
                            inject_sdl_touch_event(data, SDL_FINGERDOWN, touch->id,
                                                 norm_x, norm_y, touch->pressure);
                            touch->sent_down = true;
                        } else {
                            log_debug("Sending FINGERMOTION: slot=%d id=%d pos=(%.3f,%.3f)", 
                                     i, touch->id, norm_x, norm_y);
                            inject_sdl_touch_event(data, SDL_FINGERMOTION, touch->id,
                                                 norm_x, norm_y, touch->pressure);
                        }
                    }
                }
            }
            break;
    }
}

/* Inject SDL touch event */
static void inject_sdl_touch_event(EvdevData* data, Uint32 type, int id, 
                                  float x, float y, float pressure) {
    SDL_Event event;
    event.type = type;
    event.tfinger.timestamp = SDL_GetTicks();
    event.tfinger.touchId = 0; /* We only have one touch device */
    event.tfinger.fingerId = id;
    event.tfinger.x = x;
    event.tfinger.y = y;
    event.tfinger.dx = 0; /* TODO: Track deltas */
    event.tfinger.dy = 0;
    event.tfinger.pressure = pressure;
    
    if (input_handler_push_event(data->handler, &event)) {
        data->handler->stats.touch_events++;
        log_debug("Injected %s event: id=%d pos=(%.3f,%.3f) pressure=%.2f",
                  type == SDL_FINGERDOWN ? "FINGERDOWN" :
                  type == SDL_FINGERUP ? "FINGERUP" : "FINGERMOTION",
                  id, x, y, pressure);
        
        /* Mouse emulation if enabled */
        if (data->mouse_emulation && data->current_slot == 0) {
            /* For mouse emulation, we need window dimensions but can't rely on window ID
               Just use a reasonable default for now */
            const int screen_w = 800;  // Default screen width
            const int screen_h = 480;  // Default screen height
            
            SDL_Event mouse_event;
            
            switch (type) {
                case SDL_FINGERDOWN:
                    mouse_event.type = SDL_MOUSEBUTTONDOWN;
                    mouse_event.button.button = SDL_BUTTON_LEFT;
                    mouse_event.button.state = SDL_PRESSED;
                    mouse_event.button.x = x * screen_w;
                    mouse_event.button.y = y * screen_h;
                    break;
                    
                case SDL_FINGERUP:
                    mouse_event.type = SDL_MOUSEBUTTONUP;
                    mouse_event.button.button = SDL_BUTTON_LEFT;
                    mouse_event.button.state = SDL_RELEASED;
                    mouse_event.button.x = x * screen_w;
                    mouse_event.button.y = y * screen_h;
                    break;
                    
                case SDL_FINGERMOTION:
                    mouse_event.type = SDL_MOUSEMOTION;
                    mouse_event.motion.x = x * screen_w;
                    mouse_event.motion.y = y * screen_h;
                    break;
                    
                default:
                    return;
            }
            
            if (input_handler_push_event(data->handler, &mouse_event)) {
                data->handler->stats.mouse_events++;
            }
        }
    }
}

/* Find touch device by scanning /dev/input */
static bool find_touch_device(char* path, size_t path_size) {
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        log_error("Failed to open /dev/input: %s", strerror(errno));
        return false;
    }
    
    log_info("Scanning for touch devices in /dev/input...");
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }
        
        char device_path[256];
        snprintf(device_path, sizeof(device_path), "/dev/input/%s", entry->d_name);
        
        int fd = open(device_path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            log_debug("  Cannot open %s: %s", device_path, strerror(errno));
            continue;
        }
        
        /* Check if device has touch capabilities */
        unsigned long evbit[NBITS(EV_MAX)];
        unsigned long absbit[NBITS(ABS_MAX)];
        memset(evbit, 0, sizeof(evbit));
        memset(absbit, 0, sizeof(absbit));
        
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) >= 0 &&
            ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) >= 0) {
            
            if (test_bit(EV_ABS, evbit) && 
                test_bit(ABS_MT_POSITION_X, absbit) &&
                test_bit(ABS_MT_POSITION_Y, absbit)) {
                
                /* Found touch device */
                char name[256] = "Unknown";
                ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                log_info("  Found touch device: %s - %s", device_path, name);
                
                close(fd);
                closedir(dir);
                strncpy(path, device_path, path_size - 1);
                path[path_size - 1] = '\0';
                return true;
            } else {
                /* Log what capabilities this device has */
                char name[256] = "Unknown";
                ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                log_debug("  Device %s (%s) - EV_ABS:%d MT_X:%d MT_Y:%d",
                         device_path, name,
                         test_bit(EV_ABS, evbit),
                         test_bit(ABS_MT_POSITION_X, absbit),
                         test_bit(ABS_MT_POSITION_Y, absbit));
            }
        }
        
        close(fd);
    }
    
    closedir(dir);
    log_warn("No touch device found after scanning /dev/input");
    return false;
}

/* Open input device */
static bool open_device(EvdevData* data, const char* path) {
    data->fd = open(path, O_RDONLY | O_NONBLOCK);
    if (data->fd < 0) {
        log_error("Failed to open input device %s: %s", path, strerror(errno));
        return false;
    }
    
    /* Get device name */
    char name[256] = "Unknown";
    ioctl(data->fd, EVIOCGNAME(sizeof(name)), name);
    log_info("Opened input device: %s (%s)", path, name);
    
    /* Get axis information */
    if (ioctl(data->fd, EVIOCGABS(ABS_MT_POSITION_X), &data->abs_x) < 0 ||
        ioctl(data->fd, EVIOCGABS(ABS_MT_POSITION_Y), &data->abs_y) < 0) {
        log_error("Failed to get touch axis info");
        close(data->fd);
        data->fd = -1;
        return false;
    }
    
    /* Check for pressure support (optional) */
    if (ioctl(data->fd, EVIOCGABS(ABS_MT_PRESSURE), &data->abs_pressure) >= 0) {
        data->has_pressure = true;
        log_info("Device supports pressure sensing");
    }
    
    log_info("Touch range: X=%d-%d, Y=%d-%d",
             data->abs_x.minimum, data->abs_x.maximum,
             data->abs_y.minimum, data->abs_y.maximum);
    
    return true;
}

/* Close input device */
static void close_device(EvdevData* data) {
    if (data->fd >= 0) {
        close(data->fd);
        data->fd = -1;
    }
}

/* Create Linux evdev input source */
InputSource* input_source_linux_evdev_create(void) {
    InputSource* source = calloc(1, sizeof(InputSource));
    if (!source) {
        return NULL;
    }
    
    EvdevData* data = calloc(1, sizeof(EvdevData));
    if (!data) {
        free(source);
        return NULL;
    }
    
    /* Initialize structure */
    source->type = INPUT_SOURCE_LINUX_EVDEV;
    source->name = "Linux evdev";
    source->impl = data;
    source->initialize = evdev_initialize;
    source->start = evdev_start;
    source->stop = evdev_stop;
    source->get_capabilities = evdev_get_capabilities;
    source->cleanup = evdev_cleanup;
    
    /* Initialize private data */
    data->fd = -1;
    
    return source;
}