#include <stdio.h>
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"

unsigned char img[640*480];

void load_image() {
    FILE *f = fopen("img.raw", "r");
    fread(img, sizeof(img), 1, f);
    fclose(f);
}

void dump_image() {
    for (int i = 0 ; i < 40 ; i++) {
        for (int j = 0 ; j < 16 ; j++) {
            printf("%3X", img[i*16+j]);
        }
        printf("\n");
    }
}

SDL_Surface* create_surface() {
    SDL_Surface *image = NULL;
    for (int i = 0 ; i < 1000 ; i++) {
        SDL_RWops *file = SDL_RWFromFile("left.jpg", "r");
        //image = IMG_LoadJPG_RW(file);
    }
    /*
    SDL_Surface *image = SDL_CreateRGBSurfaceFrom(img, 640, 480, 8, 640, 0, 0, 0, 0);
    SDL_Color colors[256];
    for (int i = 0 ; i < 256 ; i++) {
        colors[i].r = i;
        colors[i].g = i;
        colors[i].b = i;
    }
    SDL_SetColors(image, colors, 0, 256);
    */
    return image;
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Surface *screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE);
    SDL_WM_SetCaption("Image test", NULL);

    load_image();
    //dump_image();
    SDL_Surface *image = create_surface();
    SDL_Rect offset;
    offset.x = 90;
    offset.y = 80;
    SDL_BlitSurface(image, NULL, screen, &offset);
    SDL_Flip(screen);
    
    /*
    SDL_Event event;
    bool isRunning = true;
    while (isRunning) {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            isRunning = false;
            break;
        }
    }
    SDL_FreeSurface(image);
    SDL_Quit();
    */
    return 0;
}
