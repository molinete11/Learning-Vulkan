#include <xcb/xcb.h>
#include <xcb/xcb_util.h>
#include <xcb/xcb_icccm.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xcb.h>

typedef struct {
    xcb_connection_t *connection;
    xcb_window_t wid;
    xcb_screen_t *screen;
    int w, h;
    char resizable;
    xcb_atom_t close_window;
}X11Window;

typedef enum {
    X11_NONE,
    X11_CLOSE_WINDOW,
}X11Event;

X11Window x11_create_window(const int width, const int height, const char * title);
void x11_set_window_resizable(X11Window *window, char is_resizable);


X11Event x11_poll_next_event(X11Window *window);

VkResult vkw_create_xcb_surface(
                                VkInstance instance, 
                                const VkAllocationCallbacks * pAllocator,
                                VkSurfaceKHR * pSurface,
                                X11Window * window
                            );