extern "C" {
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/display.h"
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/keyboard.h"
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/rtc.h"
}


#include "src\utils.h"
#include "src\physics.h"
#include "src\raycasting.h"


int key;

/*

int keyPressed(int basic_keycode){
    const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;
    int row, col, word, bit;
    row = basic_keycode%10;
    col = basic_keycode/10-1;
    word = row>>1;
    bit = col + ((row&1)<<3);
    return (0 != (keyboard_register[word] & 1<<bit));
}

*/

int wait(int targetTicks) {
    int startTick = RTC_GetTicks();  // Get the current RTC tick count
    int targetDuration = (targetTicks * 1000) / 8192;  // Convert targetTicks to milliseconds

    // Wait until the target duration has elapsed
    while (RTC_Elapsed_ms(startTick, targetDuration) < targetDuration) {
        GetKey(&key);
        if (key == KEY_CTRL_EXIT) return 1;
    }
    return 0;
}

int main() {
    //SDK Initialisation
    Bdisp_EnableColor(1); //Enable 16b
    Bdisp_AllClr_VRAM();
    Bdisp_PutDisp_DD();


	//const int targetTicks = (8192 * 66.67) / 1000; //15Hz ~= 66.67ms


    //Initialize player and world data
    Player player = Player(vec2(1, -1), 0);
    const Wall wallData[64] = {
        Wall(vec2(-1, -1), vec2(1, 1)),
    };
    const int wallCount = 1;


    while (1) { //Constant loop until exit.
        GetKey(&key);
        //Handle input
        if (key == KEY_CTRL_EXIT) {
            key = 0;
            break; //Exit program immediately.
        }

        if (key == KEY_CTRL_LEFT) player.viewAngle -= 15.0f;
        if (key == KEY_CTRL_RIGHT) player.viewAngle += 15.0f;
        

        //Update physics - Temporarily disabled.
        player = physics::playerMove(player, &key, &wallData, wallCount);

        //Render scene (simplified raycasting)
        raycasting::renderScene(player, &wallData, wallCount);


        Bdisp_PutDisp_DD();
        //Wait for next frame on 15Hz Loop
        /*
        int exit = wait(targetTicks);
        if (exit) {
            exit = 0;
            break;
        }
        */
    }
    return 0;

}

