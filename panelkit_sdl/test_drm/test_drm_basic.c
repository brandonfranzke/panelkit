// test_drm_basic.c - Test basic DRM access
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

void print_drm_info(int fd) {
    // Get DRM capabilities
    uint64_t has_dumb;
    if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) >= 0) {
        printf("  Dumb buffer support: %s\n", has_dumb ? "YES" : "NO");
    }
    
    // Get resources
    drmModeRes *resources = drmModeGetResources(fd);
    if (resources) {
        printf("  CRTCs: %d\n", resources->count_crtcs);
        printf("  Connectors: %d\n", resources->count_connectors);
        printf("  Encoders: %d\n", resources->count_encoders);
        
        // Check connectors
        for (int i = 0; i < resources->count_connectors; i++) {
            drmModeConnector *conn = drmModeGetConnector(fd, resources->connectors[i]);
            if (conn) {
                printf("  Connector %d: %s\n", conn->connector_id,
                       conn->connection == DRM_MODE_CONNECTED ? "connected" : "disconnected");
                drmModeFreeConnector(conn);
            }
        }
        
        drmModeFreeResources(resources);
    }
}

int main() {
    printf("=== DRM Basic Test ===\n\n");
    
    // Try different DRM devices
    const char* devices[] = {"/dev/dri/card0", "/dev/dri/card1", NULL};
    
    for (int i = 0; devices[i]; i++) {
        printf("Testing %s:\n", devices[i]);
        
        int fd = open(devices[i], O_RDWR);
        if (fd < 0) {
            printf("  Failed to open: %s\n", strerror(errno));
            continue;
        }
        
        // Check if it's a DRM device
        drmVersionPtr version = drmGetVersion(fd);
        if (version) {
            printf("  DRM driver: %s v%d.%d.%d\n", 
                   version->name, 
                   version->version_major,
                   version->version_minor,
                   version->version_patchlevel);
            drmFreeVersion(version);
            
            // Get more info
            print_drm_info(fd);
        } else {
            printf("  Not a DRM device\n");
        }
        
        close(fd);
        printf("\n");
    }
    
    return 0;
}