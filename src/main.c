#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fileioc.h>
#include <graphx.h>
#include <fileioc.h>


/* Include the graphics */
#include "gfx/tiles_gfx.h"


/* Include the external map data */
extern uint8_t tilemap_map[];

/* Tilemap defines */
#define TILE_WIDTH          16
#define TILE_HEIGHT         16

#define TILEMAP_WIDTH       100
#define TILEMAP_HEIGHT      15

#define TILEMAP_DRAW_WIDTH  20
#define TILEMAP_DRAW_HEIGHT 15

#define Y_OFFSET            0
#define X_OFFSET            0

#define DEFAULT_BIRD_Y      30
#define SPACE_BETWEEN_PIPES 6


void WriteToTilePos(unsigned int x, unsigned int y, unsigned int tileId, gfx_tilemap_t *tm) {
    unsigned int pos = (y*tm->width)+x;
    tm->map[pos]=tileId;
}

void DrawDisplayBufferMask() {
    unsigned int y=0;
    //gfx_FillRectangle(320-16, 0, 16, 240);
    while(y<240) {
        gfx_TransparentSprite(sprite_dither, 294, y);
        y+=6;
    }
}

void ScreenPosDebugger() {
    uint32_t *lcd_Ram_i, *lcd_Ram_end = (uint32_t *) ((int) lcd_Ram + LCD_SIZE);
    unsigned int x = 0;
    unsigned int y = 0;
    sk_key_t key;
    while ((key = os_GetCSC()) != sk_Graph) {
        gfx_SetPixel(x,y);
        gfx_SwapDraw(); //Swap buffer
        switch (key) {
            case sk_Right:
				x+=1;
                break;
            case sk_Left:
				x-=1;
                break;
            case sk_Up:
				y+=1;
                break;
            case sk_Down:
				y-=1;
                break;
            default:
                break;
        }
    }
}

int GetTilePos(unsigned int x, unsigned int y, gfx_tilemap_t *tm) {
    unsigned int pos = (y*tm->width)+x;
    unsigned int tileLength = tm->width*tm->height;
    if(pos>tileLength || pos<0) {
        //Out of bounds. Return 0
        return 22;
    }
    return tm->map[pos];
}

void WriteToVerticalStrip(unsigned int x, unsigned int tileId, gfx_tilemap_t *tm) {
    unsigned int y = 0;
    while(y<tm->height) {
        WriteToTilePos(x,y,tileId,tm);
        y+=1;
    }
}

void WriteFourByFourTileAtPos(unsigned int x, unsigned int y, unsigned int tileId, gfx_tilemap_t *tm) {
    //Revise this later.
    unsigned int editorTile = tileId;

    WriteToTilePos(x+0,y+0,editorTile+0,tm);
    WriteToTilePos(x+1,y+0,editorTile+1,tm);

    WriteToTilePos(x+0,y+1,editorTile+0+2,tm);
    WriteToTilePos(x+1,y+1,editorTile+1+2,tm);

}

void FillPrettyVerticalStrip(gfx_tilemap_t *tm, unsigned int x) {
    //First, fill it with blue.
    WriteToVerticalStrip(x,0,tm);
    //Fill the bottom
    WriteFourByFourTileAtPos(x,15-4,4,tm);
    //Determine if this should be grass or a building.
    if(x%4==0) {
        //Building
        WriteFourByFourTileAtPos(x,15-2,16,tm);
    } else {
        //Grass
        WriteFourByFourTileAtPos(x,15-2,12,tm);
    }
}

void FillPrettyTilemap(gfx_tilemap_t *tm) {
    unsigned int x=0;
    while(x<tm->width) {
        //60 above changes the width.
        FillPrettyVerticalStrip(tm,x);
        x+=1;
        
    }
    
}

void FillPrettyTilemapAfterViewpoint(gfx_tilemap_t *tm) {
    
    unsigned int x=TILEMAP_DRAW_WIDTH+2+TILEMAP_DRAW_WIDTH;
    while(x<tm->width) {
        //60 above changes the width.
        FillPrettyVerticalStrip(tm,x);
        x+=1;
        
    }
    
}

int GetPipeTypeAtPos(unsigned int test) {
    if(test==1) {
        //Bottom grass
        return 24;
    }
    if(test==2) {
        //Clound endings
        return 32;
    }
    //Default sky
    return 36;
}

void AddRawPipe(unsigned int x,gfx_tilemap_t *tm, unsigned int bottomHeight, unsigned int topHeight) {
    //Draw bottom pipe.
    unsigned int i = 1;
    //Clear out this area first.
    FillPrettyVerticalStrip(tm,x);
    while(i<bottomHeight) {
        int tileId = GetPipeTypeAtPos(i); //Get the tile type for correct background rendering.
        WriteFourByFourTileAtPos(x,(tm->height)-(2*i),tileId,tm);
        i+=1;
    }
    WriteFourByFourTileAtPos(x,(tm->height)-(2*i),20,tm); //Draw cap

    //Draw top pipe. This one is easier because the background is always the same.
    i=0;
    while(i<topHeight) {
        WriteFourByFourTileAtPos(x,2*i,36,tm);
        i+=1;
    }
    WriteFourByFourTileAtPos(x,2*i,40,tm); //Draw cap
    
}

void AddPipe(unsigned int x,gfx_tilemap_t *tm) {
    //Randomize.
    unsigned int range = tm->height-4-4-3;
    unsigned int bottomHeight = randInt(3,range);
    //unsigned int bottomHeight = tm->height-topHeight-3-4;
    unsigned int topHeight = 2 + (range-bottomHeight);
    AddRawPipe(x,tm,bottomHeight,topHeight);
    //If this is nearing the edge, place the same pipe on the beginning.
    if(x>30) {
        AddRawPipe(x-30,tm,bottomHeight,topHeight);
    }
}

bool CheckCollision(signed int posX, signed int posYScreen,unsigned int posYOffset, gfx_tilemap_t *tilemap) {
    int tileId;
    tileId=GetTilePos((posX/16)+3, (posYScreen/16)+posYOffset, tilemap);
    //Check this against the blacklist.
    return (tileId>=22 && tileId <= 41);
}

void FadeDisplay(signed int fadeLevel) {
    //Loop through the screen and set every other pixel or every third pixel.
    //This isn't a true fade, but it should be fast and easy.
    unsigned int posX=0;
    unsigned int posY=0;
    while(posY<240) {
        gfx_SetPixel(posX,posY);
        posX+=fadeLevel;
        if(posX>320) {
            //Reset X and add to Y;
            posX = posY%fadeLevel;
            posY+=1;
        }
    }
}

void FadeDisplayRedraw(signed int flashLevel, signed int posX, gfx_tilemap_t *tilemap) {
    //Redraw background.
    gfx_Tilemap(tilemap, posX, 0);
    //Do fade and fade logic
    FadeDisplay(flashLevel);
    //Set right bit
    DrawDisplayBufferMask();
    //Flush buffer
    gfx_SwapDraw();
}

int CalculateScore(signed int posX, signed int loopCount) {
    signed int tile = posX/16;
    signed int score = ((tile-10)/SPACE_BETWEEN_PIPES)+((20/SPACE_BETWEEN_PIPES)*2*loopCount)-1; //The 5 here is how many points per wrap there is. This might have to be changed.
    if(score<0) {
        score=0;
    }
    return score;
}

void KillPlayer(signed int *posX, signed int *posYScreen, signed int *lastPipeLocation, signed int *posY,unsigned int score, unsigned int *loopCountMain, signed int *velocityMain,gfx_tilemap_t *tilemap) {
    //Player was killed. Play some animation or something.
    sk_key_t key;
    //while ((key = os_GetCSC()) != sk_Graph);
    signed int posXInt = *posX;
    signed int posYInt = *posYScreen*80;
    signed int velocity = 0;
    signed int animationFrame=0;
    //Play screen flash.
    //FadeDisplay(4);
    //Flush buffer
    gfx_SwapDraw();
    //Animate the bird falling
    while(posYInt/80<300) {
        signed int posScreen = (posYInt/80);
        velocity+=60;
        posYInt+=velocity;
        //Draw tilemap and bird.
        gfx_Tilemap(tilemap, posXInt, 0);
        DrawDisplayBufferMask();
        gfx_TransparentSprite(bird_90, 42, posScreen);
        
        //Flush buffer
        gfx_SwapDraw();
    }
    //Show the menu now.
    //Reset the velocity.
    velocity=0;
    while(true) {
        signed int posScreen = (posYInt/110);
        velocity-=190;
        posYInt+=velocity;
        if(posYInt/80<=40) {
            posYInt=40*80;
            break;
        }
        //Draw tilemap and scoreboard
        gfx_Tilemap(tilemap, posXInt, 0);
        DrawDisplayBufferMask();
        gfx_TransparentSprite(sprite_scoreboard, 54, posScreen);
        if(posScreen+40<240) {
            gfx_PrintStringXY("",74,posScreen+50); //Pritn score
            gfx_PrintUInt(score, 3);
        }
        //Flush buffer
        gfx_SwapDraw();
    }

    //Flash the 2nd key icon.
    while ((key = os_GetCSC()) != sk_2nd) {
        signed int posScreen = (posYInt/110);
        //Draw tilemap and key
        gfx_Tilemap(tilemap, posXInt, 0);
        DrawDisplayBufferMask();
        gfx_TransparentSprite(sprite_scoreboard, 54, posScreen);

        gfx_PrintStringXY("",74,posScreen+50); //Print score
        gfx_PrintUInt(score, 3);

        if(animationFrame>10) {
            gfx_TransparentSprite(sprite_2nd_1, 128, 176);
        } else {
            gfx_TransparentSprite(sprite_2nd_0, 128, 176);
        }
        //Flush buffer
        gfx_SwapDraw();
        animationFrame+=1;
        if(animationFrame>20) {
            animationFrame=0;
        }
    }

    //Play a nice animation.
    velocity=0;
    posYInt=0;
    while(posYInt<480) {
        velocity+=1;
        posYInt+=velocity;
        gfx_SetColor(0x00);
        gfx_FillRectangle(0, 240-(posYInt/2), 320, posYInt/2);
        //Flush buffer
        gfx_SwapDraw();
    }
    //Clean up and get ready to reset.
    FillPrettyTilemap(tilemap);
    *posX=0;
    *posY = DEFAULT_BIRD_Y*80;
    *posYScreen = DEFAULT_BIRD_Y;
    *lastPipeLocation = 0;
    *loopCountMain = 0;
    *velocityMain=0;
    //Play a nice animation out
    velocity=0;
    while(posYInt>0) {
        velocity+=2;
        posYInt-=velocity;
        gfx_SetColor(0x00);
        //Draw
        gfx_Tilemap(tilemap, 0, 0);
        gfx_TransparentSprite(bird, 42, DEFAULT_BIRD_Y);
        DrawDisplayBufferMask();
        //Draw cover
        gfx_FillRectangle(0, 0, 320, posYInt/2);
        //Flush buffer
        gfx_SwapDraw();
    }
}

void CheckIfKillShouldHappen(signed int *posX, signed int *posYScreen, signed int *lastPipeLocation, signed int *posY, unsigned int score, unsigned int *loopCount, signed int *velocity,gfx_tilemap_t *tilemap) {
    //First, depointerize this.
    signed int posXInt;
    signed int posYInt;
    posXInt = *posX;
    posYInt = *posYScreen;
    //If the player hits a pipe, kill them and play the animation.
    if(CheckCollision(posXInt,posYInt,0,tilemap) || CheckCollision(posXInt,posYInt,1,tilemap)) {
        //Killed!
        KillPlayer(posX,posYScreen,lastPipeLocation,posY,score, loopCount, velocity,tilemap);
    }
}

void DrawLargeDigit(unsigned int digit, unsigned int x, unsigned int y) {
    switch(digit) {
        case 0:
            gfx_TransparentSprite(sprite_number_00, x, y);
            break;
        case 1:
            gfx_TransparentSprite(sprite_number_01, x, y);
            break;
        case 2:
            gfx_TransparentSprite(sprite_number_02, x, y);
            break;
        case 3:
            gfx_TransparentSprite(sprite_number_03, x, y);
            break;
        case 4:
            gfx_TransparentSprite(sprite_number_04, x, y);
            break;
        case 5:
            gfx_TransparentSprite(sprite_number_05, x, y);
            break;
        case 6:
            gfx_TransparentSprite(sprite_number_06, x, y);
            break;
        case 7:
            gfx_TransparentSprite(sprite_number_07, x, y);
            break;
        case 8:
            gfx_TransparentSprite(sprite_number_08, x, y);
            break;
        case 9:
            gfx_TransparentSprite(sprite_number_09, x, y);
            break;
    }
}

void PrintLargeNumber(unsigned int num) {
    unsigned int digitOne = (num/1)%10;
    unsigned int digitTwo = (num/10)%10;
    unsigned int digitThree = (num/100)%10;
    DrawLargeDigit(digitOne,320-32-(22*1),20);
    DrawLargeDigit(digitTwo,320-32-(22*2),20);
    DrawLargeDigit(digitThree,320-32-(22*3),20);
}

bool OnPause(unsigned int score) {
    //If returned true, continue. If false, hault.
    //Animate this.
    unsigned int width=0;
    sk_key_t key;
    unsigned int velocity = 2;
    unsigned int option = 0;
    
    while(width<140) {
        //Draw box.
        gfx_SetColor(gfx_red);
        gfx_FillRectangle(320-width-1, 0, 1, 240);
        gfx_SetColor(gfx_white);
        gfx_FillRectangle(320-width, 0, width, 240);
        PrintLargeNumber(score); //Draw score
        gfx_SwapDraw(); //Swap buffer
        width+=velocity/12;
        velocity+=4;
    }
    velocity-=4;
    width-=velocity/12;
    
    while (true) {
        bool shouldExit = false;
        gfx_TransparentSprite(pause, width+70, 70);
        gfx_TransparentSprite(arrow, width+70, 98+(option*11));
        gfx_SwapDraw();
        while (true) {
            key = os_GetCSC();
            if(key == sk_Up || key == sk_Down || key == sk_2nd) {
                //Changed.
                if(key==sk_Up || key == sk_Down) {
                    if(option==1) {
                        option = 0;
                    } else {
                        option=1;
                    }
                    
                } else {
                    //Should eixt
                    shouldExit=true;
                }
                break;
            }
        }
        if(shouldExit) {
            break;
        }
    }
    gfx_SwapDraw();
    return option==1;
}



void main(void) {
    sk_key_t key;
	ti_var_t myAppVarRead;

    unsigned int cycleCount = 0;
    unsigned int pipeCount = 0;

    bool shouldExit = false;

    signed int velocityY = 0;
    signed int posY = DEFAULT_BIRD_Y*80;
    signed int posX = 0;
    signed int lastPipeLocation=0;

    unsigned int screenLoopWrapCount=0;
    unsigned int scoreOffset=0;
    
    gfx_tilemap_t tilemap;

    /* Initialize the sprites */
    gfx_UninitedSprite(sprite_buffer, bird_width, bird_height);

    /* Initialize the tilemap structure */
    tilemap.map         = tilemap_map;
    tilemap.tiles       = tileset_tiles;
    tilemap.type_width  = gfx_tile_16_pixel;
    tilemap.type_height = gfx_tile_16_pixel;
    tilemap.tile_height = TILE_HEIGHT;
    tilemap.tile_width  = TILE_WIDTH;
    tilemap.draw_height = TILEMAP_DRAW_HEIGHT;
    tilemap.draw_width  = TILEMAP_DRAW_WIDTH;
    tilemap.height      = TILEMAP_HEIGHT;
    tilemap.width       = TILEMAP_WIDTH;
    tilemap.y_loc       = Y_OFFSET;
    tilemap.x_loc       = X_OFFSET;

    /* Initialize the 8bpp graphics */
    gfx_Begin();

    /* Seed random numbers */
    srand(rtc_Time());

    /* Set up the palette */
    gfx_SetPalette(tiles_gfx_pal, sizeof_tiles_gfx_pal, 0);
    //gfx_SetPalette(logo_gfx_pal, sizeof_logo_gfx_pal, 0);
    gfx_SetColor(gfx_white);

    /* Draw to buffer to avoid tearing */
    gfx_SetDrawBuffer();

    /* Set monospace font with width of 8 */
    gfx_SetMonospaceFont(8);

    /* Create the background. */
    FillPrettyTilemap(&tilemap);
    
	
    while ((key = os_GetCSC()) != sk_Graph) {
		/* Main Game Loop */
        /* Variable init */
        signed int posYScreen = (posY/80);
        signed int currentTile = posX/16;
        unsigned int score = CalculateScore(posX,screenLoopWrapCount);

        /* Physics */

        velocityY+=60;
        posY+=velocityY;
        posX+=3;
        cycleCount+=1;

        /* Pipe creation */

        if((currentTile+TILEMAP_DRAW_WIDTH)%SPACE_BETWEEN_PIPES==0 && lastPipeLocation<(currentTile+TILEMAP_DRAW_WIDTH)) {
            //We should add a pipe.

            //Reseed
            srand(rtc_Time()*cycleCount);
            AddPipe((currentTile+TILEMAP_DRAW_WIDTH),&tilemap);
            lastPipeLocation=(currentTile+TILEMAP_DRAW_WIDTH);
            pipeCount+=1;
        }

        /* Rendering */
        gfx_Tilemap(&tilemap, posX, 0); //Pipes and background
        gfx_TransparentSprite(bird, 42, posYScreen); //Character
        PrintLargeNumber(score); //Score
        
        /* Rendering debugging 
        gfx_SetMonospaceFont(8);
        gfx_SetColor(gfx_black);
        gfx_PrintStringXY("Cycles:    ",30,20);
        gfx_PrintUInt(cycleCount, 8);
        gfx_PrintStringXY("Velocity Y:",30,30);
        gfx_PrintUInt(velocityY, 8);
        gfx_PrintStringXY("Pos Y:     ",30,40);
        gfx_PrintUInt(posYScreen, 8);
        gfx_PrintStringXY("Pos X:     ",30,50);
        gfx_PrintUInt(posX, 8);
        gfx_PrintStringXY("Pos X Tile:",30,60);
        gfx_PrintUInt(currentTile, 8);
        gfx_PrintStringXY("Score:     ",30,70);
        gfx_PrintUInt(CalculateScore(posX,screenLoopWrapCount), 8);*/

        /* Collision detection and game finish */
        CheckIfKillShouldHappen(&posX, &posYScreen, &lastPipeLocation, &posY,score, &screenLoopWrapCount, &velocityY, &tilemap);

        /* Clearing out of range objects */

        if(currentTile-2>=0) {
            //Clear out any pipes beyond here.
            FillPrettyVerticalStrip(&tilemap,currentTile-2);
        }

        FillPrettyVerticalStrip(&tilemap,currentTile+8+TILEMAP_DRAW_WIDTH); //Clear in front of user

       /* Infinite scroll logic */

        if(currentTile>40) {
            unsigned int i = currentTile;
            posX-=(30*16);
            currentTile=0;
            lastPipeLocation=TILEMAP_DRAW_WIDTH;
            //Clear out this area.
            FillPrettyTilemapAfterViewpoint(&tilemap);
            screenLoopWrapCount+=1;
            scoreOffset+=CalculateScore(posX,0);
        }

        /* User input */
		
        switch (key) {
            case sk_2nd:
				//The user has jumped.
                velocityY=-430;
                break;
            case sk_Down:
				//Debugging.
                posX-=8;
                break;
            case sk_Clear:
                shouldExit = !OnPause(score);
                break;
            case sk_Right:
				//Debugging.
                while ((key = os_GetCSC()) != sk_Graph);
                break;
            default:
                break;
        }
		
        /* Finish rendering */
        //Last step: Mask the last 16 pixels on the right side.
        DrawDisplayBufferMask();
		
		//Flush buffer
        gfx_SwapDraw();

        //Check if should exit.
        if(shouldExit) {
            break;
        }
    }

	
    /* Close the graphics and return to the OS */
    gfx_End();
}


