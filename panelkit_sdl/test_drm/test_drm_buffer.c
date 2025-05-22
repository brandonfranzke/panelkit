// test_drm_buffer.c - Test DRM dumb buffer creation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <libdrm/drm.h>
#include <libdrm/drm_mode.h>
#include <stdint.h>

int main(int argc, char* argv[]) {
    printf("=== DRM Dumb Buffer Test ===\n\n");
    
    // Try to open DRM devices - start with card1 (vc4) which supports dumb buffers
    int fd = -1;
    const char* device_path = NULL;
    
    // Try card1 first (vc4 driver)
    fd = open("/dev/dri/card1", O_RDWR);
    if (fd >= 0) {
        device_path = "/dev/dri/card1";
        printf("Opened %s successfully\n", device_path);
    } else {
        // Fall back to card0
        fd = open("/dev/dri/card0", O_RDWR);
        if (fd >= 0) {
            device_path = "/dev/dri/card0";
            printf("Opened %s successfully\n", device_path);
        } else {
            printf("Failed to open any DRM device: %s\n", strerror(errno));
            printf("Try: sudo %s\n", argc > 0 ? argv[0] : "test_drm_buffer");
            return 1;
        }
    }
    
    // Create dumb buffer
    struct drm_mode_create_dumb create = {
        .width = 640,
        .height = 480,
        .bpp = 32
    };
    
    if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
        printf("Failed to create dumb buffer: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("Created dumb buffer:\n");
    printf("  Size: %dx%d @ %d bpp\n", create.width, create.height, create.bpp);
    printf("  Handle: %u\n", create.handle);
    printf("  Size: %llu bytes\n", create.size);
    printf("  Pitch: %u bytes\n", create.pitch);
    
    // Map the buffer
    struct drm_mode_map_dumb map = {
        .handle = create.handle
    };
    
    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
        printf("Failed to map dumb buffer: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("\nBuffer mapped, offset: 0x%llx\n", map.offset);
    
    // mmap it
    void* addr = mmap(0, create.size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, map.offset);
    
    if (addr == MAP_FAILED) {
        printf("Failed to mmap: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("Buffer mmap'd to userspace at %p\n", addr);
    
    // Write test pattern
    uint32_t* pixels = (uint32_t*)addr;
    printf("\nWriting test pattern...\n");
    
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            // Create gradient: R=x, G=y, B=255
            pixels[y * (create.pitch/4) + x] = (x * 255 / 640) << 16 | 
                                                (y * 255 / 480) << 8 | 
                                                0xFF;
        }
    }
    
    printf("Test pattern written\n");
    
    // Cleanup
    munmap(addr, create.size);
    
    struct drm_mode_destroy_dumb destroy = {
        .handle = create.handle
    };
    ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    
    close(fd);
    
    printf("\nTest completed successfully!\n");
    return 0;
}