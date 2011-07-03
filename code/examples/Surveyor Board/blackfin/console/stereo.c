/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  stereo.c - SDL-based console for Surveyor SVS (stereo vision system)
 *      Based on SRV-SLAM.cpp : version 0.02
 *      Original Author: Mosalam Ebrahimi (m.ebrahimi@ieee.org)
 *      Copyright (C) 2008  Surveyor Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details (www.gnu.org/licenses)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include "SDL.h"
#include "SDL_net.h"
#include "SDL_image.h"

#define MTU 2048
#define TIMEOUT 500

int main(int argc, char* argv[])
{
    FILE *fp;
    int archive_flag = 0;
    int framecount = 0;
    char filename[20];
    int index1 = 0, index2 = 0;
    int i1, i2, size1, size2;
    int result, ready;
    int imageReady1 = 0, imageReady2 = 0;
    int nrSocketsReady;
    char imageBuf1[MTU*10], imageBuf2[MTU*10];
    char buf1[MTU], buf2[MTU], *cp, *cp1, *cp2;
    char msg[2] = {'I', 0};
    char msg1[5] = { 'M', 0x01, 0x01, 0x01, 0};
    IPaddress ipaddress1, ipaddress2;
    TCPsocket tcpsock1, tcpsock2;
    SDLNet_SocketSet socSet1, socSet2;

    Uint32 flags = SDL_DOUBLEBUF|SDL_HWSURFACE;
    SDL_Surface *screen;
    SDL_Surface *image1, *image2;

    // initialize SDL and SDL_net
    if((SDL_Init(SDL_INIT_VIDEO) == -1)) { 
        printf("Could not initialize SDL: %s.\n", SDL_GetError());
        return 1;
    }
    if(SDLNet_Init() == -1) {
        printf("SDLNet_Init: %s\n", SDLNet_GetError());
        return 2;
    }

    // initialize the screens
    screen = SDL_SetVideoMode(320, 240, 24, flags);


    SDLNet_ResolveHost(&ipaddress1, "192.168.0.15", 10001);
    tcpsock1 = SDLNet_TCP_Open(&ipaddress1);
    if(!tcpsock1) {
        printf("SDLNet_TCP_Open 1: %s\n", SDLNet_GetError());
        return 2;
    }
    socSet1 = SDLNet_AllocSocketSet(1);
    SDLNet_TCP_AddSocket(socSet1, tcpsock1);

    SDLNet_ResolveHost(&ipaddress2, "192.168.0.15", 10002);
    tcpsock2 = SDLNet_TCP_Open(&ipaddress2);
    if(!tcpsock2) {
        printf("SDLNet_TCP_Open 2: %s\n", SDLNet_GetError());
        return 2;
    }
    socSet2 = SDLNet_AllocSocketSet(1);
    SDLNet_TCP_AddSocket(socSet2, tcpsock2);

    // init motors
    result = SDLNet_TCP_Send(tcpsock1, msg1, 4);
    if(result < 1)
        printf("init motors: %s\n", SDLNet_GetError());
    SDLNet_TCP_Recv(tcpsock1, buf1, 2);
    msg1[0] = msg1[1] = msg1[2] = msg1[3] = 0;
    
    SDL_Event event;
    int quit = 0;

    imageReady1 = 0;
    imageReady2 = 0;

    // main loop
    for (; !quit;) {
        // check keyboard events
        *msg1 = 0;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_MOUSEBUTTONUP:
                if(event.button.button == SDL_BUTTON_LEFT){
                    printf("X: %d Y: %d\n", event.button.x, event.button.y);
                }
                break;
            case SDL_KEYDOWN:
                switch( event.key.keysym.sym ) {
                    case SDLK_ESCAPE:
                        quit = 1;
                        break;
                    case SDLK_l:
                        *msg1 = 'l';
                        break;
                    case SDLK_o:
                        *msg1 = 'L';
                        break;
                    case SDLK_a:
                        archive_flag = 1;
                        framecount = 0;
                        printf(" archiving enabled - files saved in ./archive/ directory\n");
                        break;
                    case SDLK_UP:
                        *msg1 = '8';
                        break;
                    case SDLK_DOWN:
                        *msg1 = '2';
                        break;
                    case SDLK_SPACE:
                        *msg1 = '5';
                        break;
                    case SDLK_LEFT:
                        *msg1 = '4';
                        break;
                    case SDLK_RIGHT:
                        *msg1 = '6';
                        break;
                }
                if (*msg1) {
                    printf("message = %s\n", msg1);
                    result = SDLNet_TCP_Send(tcpsock1, msg1, 1);
                    if(result < 1)
                        printf("command %s\n", SDLNet_GetError());
                    SDLNet_TCP_Recv(tcpsock1, buf1, 2);
                    *msg1 = 0;
                }
            }
        }
        if (*msg1) {
            result = SDLNet_TCP_Send(tcpsock1, msg1, 1);
            if(result < 1)
                printf("command %s\n", SDLNet_GetError());
            *msg1 = 0;
        }

        index1 = index2 = 0;
        imageBuf1[0] = imageBuf2[0] = 0;
        imageReady1 = imageReady2 = 0;

        // send 'I' command
        result = SDLNet_TCP_Send(tcpsock1, msg, 1);
        if(result < 1)
            printf("SDLNet_TCP_Send 1: %s\n", SDLNet_GetError());
        result = SDLNet_TCP_Send(tcpsock2, msg, 1);
        if(result < 1)
            printf("SDLNet_TCP_Send 2: %s\n", SDLNet_GetError());

        // look for frames
        while (!imageReady1) {	
            if (!imageReady1) {
                nrSocketsReady = SDLNet_CheckSockets(socSet1, TIMEOUT);
                if (nrSocketsReady == -1) {
                    printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
                    perror("SDLNet_CheckSockets");
                } else if (nrSocketsReady > 0) {
                    if (SDLNet_CheckSockets(socSet1, TIMEOUT)) {
                        if (SDLNet_SocketReady(socSet1)) {
                            result = SDLNet_TCP_Recv(tcpsock1, buf1, MTU);
                            memcpy(imageBuf1+index1, buf1, result);
                            index1 += result;
                            if ((buf1[result-2] == -1) && (buf1[result-1] == -39))
                                imageReady1 = 1;
                        }
                    }
                } else {
                    printf("\n\rNo sockets ready.\n\r");
                    break;
                }
            }
        } 

        while (!imageReady2) {	
            if (!imageReady2) {
                nrSocketsReady = SDLNet_CheckSockets(socSet2, TIMEOUT);
                if (nrSocketsReady == -1) {
                    printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
                    perror("SDLNet_CheckSockets");
                } else if (nrSocketsReady > 0) {
                    if (SDLNet_CheckSockets(socSet2, TIMEOUT)) {
                        if (SDLNet_SocketReady(socSet2)) {
                            result = SDLNet_TCP_Recv(tcpsock2, buf2, MTU);
                            memcpy(imageBuf2+index2, buf2, result);
                            index2 += result;
                            if ((buf2[result-2] == -1) && (buf2[result-1] == -39))
                                imageReady2 = 1;
                        }
                    }
                } else {
                    printf("\n\rNo sockets ready.\n\r");
                    break;
                }
            }
        } 

        // make certain that captured frames are valid
        if (!imageReady1 && !imageReady2 && 
                imageBuf1[0]!='#' && imageBuf1[1]!='#' && imageBuf1[2]!='I' && 
                    imageBuf2[0]!='#' && imageBuf2[1]!='#' && imageBuf2[2]!='I' )
            continue;
        size1 = (unsigned int)imageBuf1[6] + (unsigned int)imageBuf1[7]*256 + (unsigned int)imageBuf1[8]*65536;
        if (size1 > index1-10) {
            printf("bad image size #1  %d %d %d\n", size1, index1, framecount);
            imageReady1 = 0;
            imageReady2 = 0;
            continue;
        }
        size2 = (unsigned int)imageBuf2[6] + (unsigned int)imageBuf2[7]*256 + (unsigned int)imageBuf2[8]*65536;
        if (size2 > index2-10) {
            printf("bad image size #2  %d %d %d\n", size2, index2, framecount);
            imageReady1 = 0;
            imageReady2 = 0;
            continue;
        }
        imageReady1 = 0;
        imageReady2 = 0;
        SDL_RWops *rwop1, *rwop2;
        rwop1 = SDL_RWFromMem(&(imageBuf1[10]), index1-10);
        image1  = IMG_LoadJPG_RW(rwop1);
        rwop2 = SDL_RWFromMem(&(imageBuf2[10]), index2-10);
        image2  = IMG_LoadJPG_RW(rwop2);
        cp1 = (char *)image1->pixels;
        if (!cp1) {
            printf("bad image after JPEG decode\n\r");
            continue;
        }
        cp2 = (char *)image2->pixels;
        if (!cp2) {
            printf("bad image after JPEG decode\n\r");
            continue;
        }
        for (i1=0; i1 < 230400; i1+=3) {
            i2 = (i1 + 30);
            if (i2 > 230400) 
                i2 = 230397;
            *(cp2+i1) = *(cp1+i2);
        }

        if (archive_flag) {
            sprintf(filename, "archive/%.4d.ppm", framecount);
            fp = fopen(filename, "w");
            fprintf(fp,"P6\n320\n240\n255\n", 15);
            i1 = fwrite(cp2, 1, 230400, fp);
            if (i1 != 230400)
                printf("bad write to archive\n\r");
            fclose(fp);
        }
        framecount++;
        SDL_BlitSurface(image2, NULL, screen, NULL);
        SDL_Flip(screen);
        SDL_FreeRW(rwop1);
        SDL_FreeRW(rwop2);
        SDL_FreeSurface(image1);
        SDL_FreeSurface(image2);
    }

    SDLNet_TCP_Close(tcpsock1);
    SDLNet_TCP_Close(tcpsock2);

    SDLNet_Quit();
    SDL_Quit();
    return 0;
}
