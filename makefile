
build:
	gcc src/Renderer/*.c src/x11.c src/vk_triangle.c -o test -lxcb -lxcb-util -lxcb-icccm -lvulkan


clean:
	rm -r test