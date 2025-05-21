#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>

// External API response struct and renderer
extern SDL_Renderer* renderer;
extern TTF_Font* font;

// Forward declaration of text rendering functions
extern void draw_text(const char* text, int x, int y, SDL_Color color);
extern void draw_text_left(const char* text, int x, int y, SDL_Color color);
extern void draw_small_text_left(const char* text, int x, int y, SDL_Color color, int max_width);

#define API_REFRESH_INTERVAL 5000  // 5 seconds in ms (for testing)
extern Uint32 last_api_call_time; // Defined in pages_app.c

// API data
typedef struct {
    char* data;
    size_t size;
    bool is_ready;
    bool is_loading;
    pthread_mutex_t mutex;
    
    // User data fields
    char name[128];
    char email[128];
    char location[128];
    char phone[64];
    char picture_url[256];
    char nationality[32];
    int age;
} ApiResponse;

extern ApiResponse api_response;

// cURL callback function to handle the API response
size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t real_size = size * nmemb;
    ApiResponse* response = (ApiResponse*)userdata;
    
    // Reallocate memory to hold the new data
    char* new_data = realloc(response->data, response->size + real_size + 1);
    if (!new_data) {
        // Handle memory allocation failure
        return 0;
    }
    
    // Copy the new data
    response->data = new_data;
    memcpy(&(response->data[response->size]), ptr, real_size);
    response->size += real_size;
    response->data[response->size] = '\0';
    
    return real_size;
}

// Parse the JSON API response
void parse_api_response() {
    // This is a very simplified JSON parsing approach without a proper library
    // In a real application, you should use a proper JSON parser like cJSON or jansson
    
    if (!api_response.data || api_response.size == 0) {
        return;
    }
    
    // Clear previous data
    memset(api_response.name, 0, sizeof(api_response.name));
    memset(api_response.email, 0, sizeof(api_response.email));
    memset(api_response.location, 0, sizeof(api_response.location));
    memset(api_response.phone, 0, sizeof(api_response.phone));
    memset(api_response.picture_url, 0, sizeof(api_response.picture_url));
    memset(api_response.nationality, 0, sizeof(api_response.nationality));
    api_response.age = 0;
    
    // Extract info using simple string search (not robust, but works for demo)
    char* ptr;
    
    // Extract name
    ptr = strstr(api_response.data, "\"name\":{");
    if (ptr) {
        ptr = strstr(ptr, "\"first\":\"");
        if (ptr) {
            ptr += 9; // Skip "\"first\":\""
            char first[64] = {0};
            int i = 0;
            while (*ptr != '\"' && i < 63) {
                first[i++] = *ptr++;
            }
            first[i] = '\0';
            
            ptr = strstr(ptr, "\"last\":\"");
            if (ptr) {
                ptr += 8; // Skip "\"last\":\""
                char last[64] = {0};
                i = 0;
                while (*ptr != '\"' && i < 63) {
                    last[i++] = *ptr++;
                }
                last[i] = '\0';
                
                snprintf(api_response.name, sizeof(api_response.name), "%s %s", first, last);
            }
        }
    }
    
    // Extract email
    ptr = strstr(api_response.data, "\"email\":\"");
    if (ptr) {
        ptr += 9; // Skip "\"email\":\""
        int i = 0;
        while (*ptr != '\"' && i < (int)sizeof(api_response.email) - 1) {
            api_response.email[i++] = *ptr++;
        }
        api_response.email[i] = '\0';
    }
    
    // Extract location
    ptr = strstr(api_response.data, "\"location\":{");
    if (ptr) {
        ptr = strstr(ptr, "\"city\":\"");
        if (ptr) {
            ptr += 8; // Skip "\"city\":\""
            char city[64] = {0};
            int i = 0;
            while (*ptr != '\"' && i < 63) {
                city[i++] = *ptr++;
            }
            city[i] = '\0';
            
            ptr = strstr(ptr, "\"country\":\"");
            if (ptr) {
                ptr += 11; // Skip "\"country\":\""
                char country[64] = {0};
                i = 0;
                while (*ptr != '\"' && i < 63) {
                    country[i++] = *ptr++;
                }
                country[i] = '\0';
                
                snprintf(api_response.location, sizeof(api_response.location), "%s, %s", city, country);
            }
        }
    }
    
    // Extract nationality
    ptr = strstr(api_response.data, "\"nat\":\"");
    if (ptr) {
        ptr += 7; // Skip "\"nat\":\""
        int i = 0;
        while (*ptr != '\"' && i < (int)sizeof(api_response.nationality) - 1) {
            api_response.nationality[i++] = *ptr++;
        }
        api_response.nationality[i] = '\0';
    }
    
    // Extract age
    ptr = strstr(api_response.data, "\"age\":");
    if (ptr) {
        ptr += 6; // Skip "\"age\":"
        api_response.age = atoi(ptr);
    }
    
    // Extract phone
    ptr = strstr(api_response.data, "\"phone\":\"");
    if (ptr) {
        ptr += 9; // Skip "\"phone\":\""
        int i = 0;
        while (*ptr != '\"' && i < (int)sizeof(api_response.phone) - 1) {
            api_response.phone[i++] = *ptr++;
        }
        api_response.phone[i] = '\0';
    }
    
    // Extract picture url
    ptr = strstr(api_response.data, "\"picture\":{");
    if (ptr) {
        ptr = strstr(ptr, "\"large\":\"");
        if (ptr) {
            ptr += 9; // Skip "\"large\":\""
            int i = 0;
            while (*ptr != '\"' && i < (int)sizeof(api_response.picture_url) - 1) {
                api_response.picture_url[i++] = *ptr++;
            }
            api_response.picture_url[i] = '\0';
        }
    }
    
    api_response.is_ready = true;
    api_response.is_loading = false;
    
    printf("Loaded user: %s, %d, %s\n", api_response.name, api_response.age, api_response.location);
}

// Thread function to fetch API data
void* fetch_api_data(void* arg) {
    (void)arg; // Unused parameter
    
    CURL* curl;
    CURLcode res;
    
    // Init cURL
    curl = curl_easy_init();
    if (curl) {
        // Reset the response structure
        pthread_mutex_lock(&api_response.mutex);
        if (api_response.data) {
            free(api_response.data);
        }
        api_response.data = malloc(1);
        api_response.size = 0;
        api_response.is_ready = false;
        api_response.is_loading = true;
        pthread_mutex_unlock(&api_response.mutex);
        
        // Set up the request
        curl_easy_setopt(curl, CURLOPT_URL, "https://randomuser.me/api/");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&api_response);
        
        // Execute the request
        res = curl_easy_perform(curl);
        
        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            pthread_mutex_lock(&api_response.mutex);
            api_response.is_loading = false;
            pthread_mutex_unlock(&api_response.mutex);
        } else {
            // Success - parse the response
            pthread_mutex_lock(&api_response.mutex);
            parse_api_response();
            pthread_mutex_unlock(&api_response.mutex);
        }
        
        // Clean up
        curl_easy_cleanup(curl);
    }
    
    return NULL;
}

// Function to start an API request if needed
void update_api_data(Uint32 current_time, bool force_refresh) {
    pthread_mutex_lock(&api_response.mutex);
    bool is_loading = api_response.is_loading;
    pthread_mutex_unlock(&api_response.mutex);
    
    if (is_loading) {
        return; // Already loading, don't start another request
    }
    
    if (force_refresh || (current_time - last_api_call_time > API_REFRESH_INTERVAL)) {
        // Start a new thread to fetch data
        pthread_t thread;
        if (pthread_create(&thread, NULL, fetch_api_data, NULL) == 0) {
            pthread_detach(thread);
            last_api_call_time = current_time;
            printf("Starting API request %s\n", force_refresh ? "(forced)" : "(scheduled)");
        }
    }
}

// Render API data to the screen
void render_api_data(SDL_Renderer* renderer_unused, int x, int y) {
    (void)renderer_unused; // Unused parameter - we use the global renderer
    
    pthread_mutex_lock(&api_response.mutex);
    
    SDL_Color text_color = {255, 255, 255, 255};
    
    // Calculate the maximum width for text (with right padding)
    int right_padding = 20; // Padding on the right side
    int max_width = 640 - x - right_padding; // Screen width - x position - padding
    
    // Set line height for smaller font
    int line_height = 25; // Reduced from 30 for smaller font
    
    if (api_response.is_loading) {
        draw_small_text_left("Loading user data...", x, y, text_color, max_width);
    }
    else if (api_response.is_ready) {
        // Render the API data nicely formatted
        char buffer[256];
        
        // Name
        snprintf(buffer, sizeof(buffer), "Name: %s", api_response.name);
        draw_small_text_left(buffer, x, y, text_color, max_width);
        
        // Age
        snprintf(buffer, sizeof(buffer), "Age: %d", api_response.age);
        draw_small_text_left(buffer, x, y + line_height, text_color, max_width);
        
        // Nationality
        snprintf(buffer, sizeof(buffer), "Nationality: %s", api_response.nationality);
        draw_small_text_left(buffer, x, y + line_height * 2, text_color, max_width);
        
        // Location
        snprintf(buffer, sizeof(buffer), "Location: %s", api_response.location);
        draw_small_text_left(buffer, x, y + line_height * 3, text_color, max_width);
        
        // Email 
        snprintf(buffer, sizeof(buffer), "Email: %s", api_response.email);
        draw_small_text_left(buffer, x, y + line_height * 4, text_color, max_width);
        
        // Phone
        snprintf(buffer, sizeof(buffer), "Phone: %s", api_response.phone);
        draw_small_text_left(buffer, x, y + line_height * 5, text_color, max_width);
        
        // Picture URL info (just show that it's available)
        draw_small_text_left("Image URL available", x, y + line_height * 6, text_color, max_width);
    }
    else {
        draw_small_text_left("No user data loaded", x, y, text_color, max_width);
    }
    
    pthread_mutex_unlock(&api_response.mutex);
}

// Initialize API handling
void init_api() {
    pthread_mutex_init(&api_response.mutex, NULL);
    api_response.data = NULL;
    api_response.size = 0;
    api_response.is_ready = false;
    api_response.is_loading = false;
    
    // Initialize curl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Initial API request
    last_api_call_time = SDL_GetTicks();
    update_api_data(last_api_call_time, true);
}

// Cleanup API resources
void cleanup_api() {
    pthread_mutex_lock(&api_response.mutex);
    if (api_response.data) {
        free(api_response.data);
        api_response.data = NULL;
    }
    pthread_mutex_unlock(&api_response.mutex);
    pthread_mutex_destroy(&api_response.mutex);
    
    // Cleanup curl globally
    curl_global_cleanup();
}