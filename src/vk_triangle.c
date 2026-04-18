#include "x11.h"

#include <stdio.h>
#include <stdlib.h>



int main(){

    int width = 700;
    int heigth = 700;
    const char *title = "Hello Triangle";

    X11Window window = x11_create_window(width, heigth, title);

    x11_set_window_resizable(&window, 0);

    char is_running = 1;

    while(is_running){
        X11Event e = x11_poll_next_event(&window);
        if(e == X11_CLOSE_WINDOW){
            is_running = 0;
        }
    }
    

    return EXIT_SUCCESS;
}