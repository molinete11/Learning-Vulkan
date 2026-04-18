#include "x11.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>



X11Window x11_create_window(const int width, const int height, const char *title)
{
    X11Window window = { 0 };

    int screen_number;
    xcb_connection_t *connection = xcb_connect(NULL, &screen_number);
    assert(!xcb_connection_has_error(connection));

    xcb_intern_atom_cookie_t cookie;
    xcb_intern_atom_reply_t *reply;

    cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
    reply = xcb_intern_atom_reply(connection, cookie, NULL);
    xcb_atom_t wm_protocol_property_atom = reply->atom;
    free(reply);

    cookie = xcb_intern_atom(connection, 1, 16, "WM_DELETE_WINDOW");
    reply = xcb_intern_atom_reply(connection, cookie, NULL);
    xcb_atom_t close_event = reply->atom;
    free(reply);

    xcb_window_t wid = xcb_generate_id(connection);

    xcb_screen_t *screen = xcb_aux_get_screen(connection, screen_number);

    xcb_create_window_aux(
        connection,
        screen->root_depth,
        wid,
        screen->root,
        0, 0,
        width, height,
        1,
        XCB_WINDOW_CLASS_COPY_FROM_PARENT,
        screen->root_visual,
        XCB_CW_EVENT_MASK | XCB_CW_BACK_PIXEL,
        &(xcb_create_window_value_list_t){
            .background_pixel = screen->black_pixel,
            .event_mask =  XCB_EVENT_MASK_EXPOSURE
        }
    );    

    xcb_icccm_set_wm_name(connection, wid, XCB_ATOM_STRING, 8, strlen(title), title);
    xcb_icccm_set_wm_protocols(connection, wid, wm_protocol_property_atom, 1, &close_event);

    xcb_map_window(connection, wid);

    assert(xcb_flush(connection));

    window.w = width;
    window.h = height;
    window.wid = wid;
    window.connection = connection;
    window.resizable = 1;
    window.close_window = close_event;
    window.screen = screen;

    return window;
}

void x11_set_window_resizable(X11Window *window, char is_resizable)
{
    if(!is_resizable && window->resizable){
        xcb_size_hints_t w_size_hints;
        xcb_icccm_size_hints_set_min_size(&w_size_hints, window->w, window->h);
        xcb_icccm_size_hints_set_max_size(&w_size_hints, window->w, window->h);
        xcb_icccm_set_wm_size_hints(window->connection, window->wid, XCB_ATOM_WM_NORMAL_HINTS, &w_size_hints);
        window->resizable = 0;
        
    }else if(is_resizable && !window->resizable){
        xcb_size_hints_t w_size_hints;
        xcb_icccm_size_hints_set_min_size(&w_size_hints, 10, 10);
        xcb_icccm_size_hints_set_max_size(&w_size_hints, INT32_MAX, INT32_MAX);
        xcb_icccm_set_wm_size_hints(window->connection, window->wid, XCB_ATOM_WM_NORMAL_HINTS, &w_size_hints);
        window->resizable = 1;
    }

    assert(xcb_flush(window->connection));
}

X11Event x11_poll_next_event(X11Window *window)
{
    xcb_generic_event_t *e = xcb_poll_for_event(window->connection);

    if(e == NULL){
        return X11_NONE;
    }

    switch (XCB_EVENT_RESPONSE_TYPE(e))
    {
        case XCB_CLIENT_MESSAGE:
            xcb_client_message_event_t *message = (xcb_client_message_event_t*)(e);
            xcb_client_message_data_t m_data = message->data;

            if(m_data.data32[0] == window->close_window){
                return X11_CLOSE_WINDOW;
            }
            break;
        
        default:
            return X11_NONE;
            break;
    }
}

VkResult vkw_create_xcb_surface(
                                VkInstance instance, 
                                const VkAllocationCallbacks * pAllocator, 
                                VkSurfaceKHR * pSurface,
                                X11Window * window
                            )
{
    VkXcbSurfaceCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .connection = window->connection,
        .window = window->wid,
    };

    return vkCreateXcbSurfaceKHR(instance, &info, pAllocator, pSurface);
}
