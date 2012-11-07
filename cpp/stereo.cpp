#include "camera.h"
#include <stdio.h>

#include <SDL/SDL.h>
#include <jpeglib.h>

SDL_Surface *screen;

const int WIDTH = 640;
const int HEIGHT = 480;
SDL_Color colors[256];

void blit_image(CameraImage *img, int x, int y)
{
    int bpp = img->bits_per_pixel;
    SDL_Surface *image = NULL;
    if (bpp == 8) {
        image = SDL_CreateRGBSurfaceFrom((void*)img->ptr, WIDTH, HEIGHT, 8, WIDTH, 0, 0, 0, 0);
        SDL_SetPalette(image, SDL_LOGPAL|SDL_PHYSPAL, colors, 0, 256);
    } else {
        image = SDL_CreateRGBSurfaceFrom((void*)img->ptr, WIDTH, HEIGHT, bpp, WIDTH*bpp/8, 0x0000ff, 0x00ff00, 0xff0000, 0);
    }
    SDL_Rect offset;
    offset.x = x;
    offset.y = y;
    SDL_BlitSurface(image, NULL, screen, &offset);
    SDL_Flip(screen);
    SDL_FreeSurface(image);
}

void save_image(CameraImage *img)
{
    FILE *f = fopen("sample.jpg", "w");
    fwrite(img->ptr, 1, img->size, f);
    fclose(f);
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_EVERYTHING);
    for (int i = 0 ; i < 256 ; i++) {
        colors[i].r = i;
        colors[i].g = i;
        colors[i].b = i;
    }
    screen = SDL_SetVideoMode(1400, 600, 32, SDL_SWSURFACE);
    SDL_WM_SetCaption("Image test", NULL);

    GreyCamera left_cam("/dev/video0", HEIGHT, WIDTH);
    left_cam.start();

    JpegCamera right_cam("/dev/video1", HEIGHT, WIDTH);
    right_cam.start();

    SDL_Event event;
    CameraImage left_img, right_img;
    for (;;) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            break;
        }
        left_cam.capture(&left_img);
        right_cam.capture(&right_img);

        blit_image(&left_img, 0, 0);
        blit_image(&right_img, 650, 0);

        //save_image(&left_img);
    }
    left_cam.stop();
    right_cam.stop();
    SDL_Quit();
    return 0;
}
