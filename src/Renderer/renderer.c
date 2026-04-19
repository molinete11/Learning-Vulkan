#include "renderer.h"

#include "instance.h"

Renderer init_renderer(X11Window *window)
{
    Renderer renderer = { 0 };

    renderer.instance = create_instance();

    return renderer;
}