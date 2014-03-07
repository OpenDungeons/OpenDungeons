#include <cmath>
#include <SDL.h>
#include "HermiteCatmullSpline.h"
#include <iostream>

 
void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    Uint8 *target_pixel = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
    *(Uint32 *)target_pixel = pixel;
}
 


int main(int argc, char *argv[])
{

    
    int nn = 10;
    int put_ii;
    int put_jj;
    double localxCord[]  = {1,2,5,3,4,8,21,1,17,4};
    double localyCord[]  = {300,-300,-5,33,-1214,-463, 21 ,20,7,-124};

    double localSegments[] = {0,100,200,300,400,500,600,700,800,900};

    HermiteCatmullSpline xHCS(nn,localxCord,localSegments);
    HermiteCatmullSpline yHCS(nn,localyCord,localSegments);

    static const int width = 2000;
    static const int height = 2000;

 
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    	return 1;
 
    atexit(SDL_Quit);
 
    SDL_Surface *screen = SDL_SetVideoMode(width, height, 0, SDL_DOUBLEBUF);
 
    if (screen == NULL)
        return 2;
 
    while(true)
    {
        SDL_Event event; 
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
                return 0;
        }
 




        // int x = rand() % (width - 4) + 2;
        // int y = rand() % (height - 4) + 2;
        // int r = rand() % (max_radius - 10) + 10;
        // int c = ((rand() % 0xff) << 16) +
        //         ((rand() % 0xff) << 8) +
        //         (rand() % 0xff);
 
        // if (r >= 4)
        // {
        //     if (x < r + 2)
        //         x = r + 2;
        //     else if (x > width - r - 2)
        //         x = width - r - 2;
 
        //     if (y < r + 2)
        //         y = r + 2;
        //     else if (y > height - r - 2)
        //         y = height - r - 2;
        // }
 
	
        SDL_LockSurface(screen);
	for (double ii = 0 ; ii < 900; ii+= 0.01){

	put_ii = xHCS.evaluate(ii) + width/2;	   
	put_jj = yHCS.evaluate(ii) + height/2 ;
	if(put_ii > 0 && put_ii < width &&  put_jj > 0 && put_jj < height)
	set_pixel(screen , put_ii, put_jj ,0xff0000ff);
	std::cerr << ii << "  " << put_jj << std::endl;
	    // else{}



	}


        // fill_circle(screen, x, y, r, 0xff000000 + c);
        // draw_circle(screen, x, y, r, 0xffffffff);
 
        SDL_FreeSurface(screen);
 
        SDL_Flip(screen);
    }
 
    return 0;
}
