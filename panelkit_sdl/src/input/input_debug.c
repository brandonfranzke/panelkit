/**
 * @file input_debug.c
 * @brief Input system debugging implementation
 */

#include "input_debug.h"
#include "../core/logger.h"
#include "../core/sdl_includes.h"
#include <string.h>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>

/* Helper macros for bit operations - must be defined before use */
#ifndef NBITS
#define NBITS(x) (((x)/BITS_PER_LONG)+1)
#endif
#ifndef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(long)*8)
#endif
#ifndef test_bit
#define test_bit(bit, array) ((array[(bit)/BITS_PER_LONG] >> ((bit)%BITS_PER_LONG)) & 1)
#endif

#endif /* __linux__ */

/* Log input handler state */
void input_debug_log_state(InputHandler* handler) {
    if (!handler) {
        log_warn("Input handler is NULL");
        return;
    }
    
    log_info("=== Input Handler State ===");
    log_info("Handler address: %p", (void*)handler);
    log_info("Running: %s", handler->running ? "Yes" : "No");
    
    if (handler->source) {
        log_info("Input source:");
        log_info("  Type: %s", 
                 handler->source->type == INPUT_SOURCE_SDL_NATIVE ? "SDL Native" :
                 handler->source->type == INPUT_SOURCE_LINUX_EVDEV ? "Linux evdev" :
                 handler->source->type == INPUT_SOURCE_MOCK ? "Mock" : "Unknown");
        log_info("  Name: %s", handler->source->name ? handler->source->name : "Unknown");
        
        /* Get capabilities */
        InputCapabilities caps;
        if (handler->source->get_capabilities && 
            handler->source->get_capabilities(handler->source, &caps)) {
            log_info("  Capabilities:");
            log_info("    Touch: %s", caps.has_touch ? "Yes" : "No");
            if (caps.has_touch) {
                log_info("    Max touch points: %d", caps.max_touch_points);
                log_info("    Touch range: X[%d-%d] Y[%d-%d]",
                         caps.touch_x_min, caps.touch_x_max,
                         caps.touch_y_min, caps.touch_y_max);
            }
            log_info("    Mouse: %s", caps.has_mouse ? "Yes" : "No");
            log_info("    Keyboard: %s", caps.has_keyboard ? "Yes" : "No");
        }
    } else {
        log_info("Input source: NULL");
    }
    
    /* Statistics */
    log_info("Statistics:");
    log_info("  Total events: %llu", (unsigned long long)handler->stats.events_processed);
    log_info("  Touch events: %llu", (unsigned long long)handler->stats.touch_events);
    log_info("  Mouse events: %llu", (unsigned long long)handler->stats.mouse_events);
    log_info("  Keyboard events: %llu", (unsigned long long)handler->stats.keyboard_events);
    
    /* Configuration */
    log_info("Configuration:");
    const char* source_name = "Unknown";
    switch (handler->config.source_type) {
        case INPUT_SOURCE_SDL_NATIVE: source_name = "SDL Native"; break;
        case INPUT_SOURCE_LINUX_EVDEV: source_name = "Linux evdev"; break;
        case INPUT_SOURCE_MOCK: source_name = "Mock"; break;
        default: break;
    }
    log_info("  Source type: %s", source_name);
    log_info("  Device path: %s", 
             handler->config.device_path ? handler->config.device_path : "Auto-detect");
    log_info("  Auto-detect devices: %s", 
             handler->config.auto_detect_devices ? "Yes" : "No");
    log_info("  Mouse emulation: %s",
             handler->config.enable_mouse_emulation ? "Yes" : "No");
    
    log_info("==========================");
}

/* Log SDL input state */
void input_debug_log_sdl_state(void) {
    log_info("=== SDL Input State ===");
    
    /* Check if event subsystem is initialized */
    if (!SDL_WasInit(SDL_INIT_EVENTS)) {
        log_warn("SDL event subsystem not initialized");
        return;
    }
    
    /* Event queue state */
    SDL_PumpEvents();
    int num_events = SDL_PeepEvents(NULL, 0, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
    log_info("Events in queue: %d", num_events);
    
    /* Touch devices */
    int num_touch = SDL_GetNumTouchDevices();
    log_info("Touch devices: %d", num_touch);
    for (int i = 0; i < num_touch; i++) {
        SDL_TouchID id = SDL_GetTouchDevice(i);
        log_info("  Device %d: ID=%lld", i, (long long)id);
        
        /* Get number of active fingers */
        int num_fingers = SDL_GetNumTouchFingers(id);
        if (num_fingers > 0) {
            log_info("    Active fingers: %d", num_fingers);
            for (int f = 0; f < num_fingers; f++) {
                SDL_Finger* finger = SDL_GetTouchFinger(id, f);
                if (finger) {
                    log_info("      Finger %d: id=%lld pos=(%.3f,%.3f) pressure=%.3f",
                             f, (long long)finger->id, finger->x, finger->y, finger->pressure);
                }
            }
        }
    }
    
    /* Mouse state */
    int mouse_x, mouse_y;
    Uint32 mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
    log_info("Mouse state:");
    log_info("  Position: (%d, %d)", mouse_x, mouse_y);
    log_info("  Buttons: %s%s%s",
             (mouse_state & SDL_BUTTON_LMASK) ? "LEFT " : "",
             (mouse_state & SDL_BUTTON_MMASK) ? "MIDDLE " : "",
             (mouse_state & SDL_BUTTON_RMASK) ? "RIGHT " : "");
    
    /* Keyboard state */
    log_info("Keyboard modifiers:");
    SDL_Keymod mod = SDL_GetModState();
    if (mod & KMOD_LSHIFT) log_info("  LSHIFT");
    if (mod & KMOD_RSHIFT) log_info("  RSHIFT");
    if (mod & KMOD_LCTRL) log_info("  LCTRL");
    if (mod & KMOD_RCTRL) log_info("  RCTRL");
    if (mod & KMOD_LALT) log_info("  LALT");
    if (mod & KMOD_RALT) log_info("  RALT");
    
    log_info("=======================");
}

#ifdef __linux__
/* Log evdev device capabilities */
void input_debug_log_device_caps(const char* device_path) {
    log_info("=== Device Capabilities: %s ===", device_path);
    
    int fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        log_error("Cannot open device: %s", strerror(errno));
        return;
    }
    
    /* Get device name */
    char name[256] = "Unknown";
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
        log_info("Device name: %s", name);
    }
    
    /* Get device info */
    struct input_id id;
    if (ioctl(fd, EVIOCGID, &id) >= 0) {
        log_info("Device ID:");
        log_info("  Bus: 0x%04x", id.bustype);
        log_info("  Vendor: 0x%04x", id.vendor);
        log_info("  Product: 0x%04x", id.product);
        log_info("  Version: 0x%04x", id.version);
    }
    
    /* Get event types */
    unsigned long evbit[NBITS(EV_MAX)];
    memset(evbit, 0, sizeof(evbit));
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) >= 0) {
        log_info("Supported event types:");
        if (test_bit(EV_SYN, evbit)) log_info("  EV_SYN (Sync events)");
        if (test_bit(EV_KEY, evbit)) log_info("  EV_KEY (Keys/Buttons)");
        if (test_bit(EV_REL, evbit)) log_info("  EV_REL (Relative axes)");
        if (test_bit(EV_ABS, evbit)) log_info("  EV_ABS (Absolute axes)");
        if (test_bit(EV_MSC, evbit)) log_info("  EV_MSC (Misc events)");
        if (test_bit(EV_SW, evbit)) log_info("  EV_SW (Switch events)");
        if (test_bit(EV_LED, evbit)) log_info("  EV_LED (LEDs)");
        if (test_bit(EV_SND, evbit)) log_info("  EV_SND (Sounds)");
        if (test_bit(EV_REP, evbit)) log_info("  EV_REP (Repeat)");
        if (test_bit(EV_FF, evbit)) log_info("  EV_FF (Force feedback)");
    }
    
    /* Check for touch capabilities */
    unsigned long absbit[NBITS(ABS_MAX)];
    memset(absbit, 0, sizeof(absbit));
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit) >= 0) {
        log_info("Absolute axes:");
        
        /* Single touch */
        if (test_bit(ABS_X, absbit) && test_bit(ABS_Y, absbit)) {
            log_info("  Single touch: Yes");
            struct input_absinfo abs_x, abs_y;
            if (ioctl(fd, EVIOCGABS(ABS_X), &abs_x) >= 0 &&
                ioctl(fd, EVIOCGABS(ABS_Y), &abs_y) >= 0) {
                log_info("    X range: %d - %d", abs_x.minimum, abs_x.maximum);
                log_info("    Y range: %d - %d", abs_y.minimum, abs_y.maximum);
            }
        }
        
        /* Multi-touch */
        if (test_bit(ABS_MT_POSITION_X, absbit) && test_bit(ABS_MT_POSITION_Y, absbit)) {
            log_info("  Multi-touch: Yes");
            struct input_absinfo abs_mt_x, abs_mt_y;
            if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_mt_x) >= 0 &&
                ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs_mt_y) >= 0) {
                log_info("    MT X range: %d - %d", abs_mt_x.minimum, abs_mt_x.maximum);
                log_info("    MT Y range: %d - %d", abs_mt_y.minimum, abs_mt_y.maximum);
            }
            
            if (test_bit(ABS_MT_SLOT, absbit)) {
                struct input_absinfo abs_slot;
                if (ioctl(fd, EVIOCGABS(ABS_MT_SLOT), &abs_slot) >= 0) {
                    log_info("    Max slots: %d", abs_slot.maximum + 1);
                }
            }
            
            if (test_bit(ABS_MT_TRACKING_ID, absbit)) {
                log_info("    Tracking ID: Yes");
            }
            
            if (test_bit(ABS_MT_PRESSURE, absbit)) {
                log_info("    Pressure: Yes");
            }
        }
    }
    
    /* Check for key/button capabilities */
    unsigned long keybit[NBITS(KEY_MAX)];
    memset(keybit, 0, sizeof(keybit));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) {
        if (test_bit(BTN_TOUCH, keybit)) {
            log_info("BTN_TOUCH: Yes");
        }
        if (test_bit(BTN_LEFT, keybit)) {
            log_info("BTN_LEFT (mouse): Yes");
        }
    }
    
    close(fd);
    log_info("=================================");
}
#else
void input_debug_log_device_caps(const char* device_path) {
    (void)device_path; /* Unused on non-Linux */
    log_info("Device capability logging not available on this platform");
}
#endif

/* Event monitor state */
static bool monitor_active = false;
static SDL_Thread* monitor_thread = NULL;
static InputHandler* monitored_handler = NULL;

/* Event monitor thread */
static int event_monitor_thread(void* data) {
    (void)data;
    
    log_info("Event monitor started");
    
    while (monitor_active) {
        SDL_Event event;
        if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0) {
            const char* event_name = "Unknown";
            char details[256] = "";
            
            switch (event.type) {
                case SDL_FINGERDOWN:
                    event_name = "FINGERDOWN";
                    SDL_snprintf(details, sizeof(details), 
                                "id=%lld pos=(%.3f,%.3f) pressure=%.3f",
                                (long long)event.tfinger.fingerId,
                                event.tfinger.x, event.tfinger.y,
                                event.tfinger.pressure);
                    break;
                    
                case SDL_FINGERUP:
                    event_name = "FINGERUP";
                    SDL_snprintf(details, sizeof(details),
                                "id=%lld pos=(%.3f,%.3f)",
                                (long long)event.tfinger.fingerId,
                                event.tfinger.x, event.tfinger.y);
                    break;
                    
                case SDL_FINGERMOTION:
                    event_name = "FINGERMOTION";
                    SDL_snprintf(details, sizeof(details),
                                "id=%lld pos=(%.3f,%.3f) delta=(%.3f,%.3f)",
                                (long long)event.tfinger.fingerId,
                                event.tfinger.x, event.tfinger.y,
                                event.tfinger.dx, event.tfinger.dy);
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    event_name = "MOUSEBUTTONDOWN";
                    SDL_snprintf(details, sizeof(details),
                                "button=%d pos=(%d,%d)",
                                event.button.button,
                                event.button.x, event.button.y);
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    event_name = "MOUSEBUTTONUP";
                    SDL_snprintf(details, sizeof(details),
                                "button=%d pos=(%d,%d)",
                                event.button.button,
                                event.button.x, event.button.y);
                    break;
                    
                case SDL_MOUSEMOTION:
                    event_name = "MOUSEMOTION";
                    SDL_snprintf(details, sizeof(details),
                                "pos=(%d,%d) rel=(%d,%d)",
                                event.motion.x, event.motion.y,
                                event.motion.xrel, event.motion.yrel);
                    break;
                    
                case SDL_KEYDOWN:
                    event_name = "KEYDOWN";
                    SDL_snprintf(details, sizeof(details),
                                "key=%s scancode=%d",
                                SDL_GetKeyName(event.key.keysym.sym),
                                event.key.keysym.scancode);
                    break;
                    
                case SDL_KEYUP:
                    event_name = "KEYUP";
                    SDL_snprintf(details, sizeof(details),
                                "key=%s scancode=%d",
                                SDL_GetKeyName(event.key.keysym.sym),
                                event.key.keysym.scancode);
                    break;
            }
            
            if (strcmp(event_name, "Unknown") != 0) {
                log_debug("[EVENT] %s: %s", event_name, details);
            }
        }
        
        SDL_Delay(10);
    }
    
    log_info("Event monitor stopped");
    return 0;
}

/* Start event monitoring */
void input_debug_start_monitor(InputHandler* handler) {
    if (monitor_active) {
        log_warn("Event monitor already active");
        return;
    }
    
    monitor_active = true;
    monitored_handler = handler;
    monitor_thread = SDL_CreateThread(event_monitor_thread, "EventMonitor", NULL);
    
    if (!monitor_thread) {
        log_error("Failed to create event monitor thread");
        monitor_active = false;
    } else {
        log_info("Event monitor started");
    }
}

/* Stop event monitoring */
void input_debug_stop_monitor(InputHandler* handler) {
    (void)handler;
    
    if (!monitor_active) {
        return;
    }
    
    monitor_active = false;
    
    if (monitor_thread) {
        SDL_WaitThread(monitor_thread, NULL);
        monitor_thread = NULL;
    }
    
    monitored_handler = NULL;
    log_info("Event monitor stopped");
}