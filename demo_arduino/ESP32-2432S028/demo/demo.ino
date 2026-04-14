#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "esp_panel_board_custom_conf.h"

#include <demos/lv_demos.h>

using namespace esp_panel::drivers;
using namespace esp_panel::board;

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing board"); 
    Board *board = new Board();
    board->init();
    assert(board->begin());

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());
    
    Serial.println("Creating UI");

    if(lvgl_port_lock(-1)) {
        lv_demo_widgets();
        lvgl_port_unlock();
    }

}


void loop() {
    Serial.println("IDLE loop");
    delay(1000);
}
