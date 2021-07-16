#include <pico/stdio.h>
#include <math.h>
#include <pico/stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"

#define BIT_LENGTH 32
const uint LED = 25;
const uint IR = 22;

uint cont_zero = 0;
uint cont_one = 0;
int cont_decode = BIT_LENGTH - 1;
long decode = 0;
long last_result = 0;
bool flag_decode = 0;
bool flag_readydecode = 0;

struct repeating_timer read_timer;

void hid_task(void);

bool read_pulse_cb(struct repeating_timer *t);

/*------------- MAIN -------------*/
int main(void){
  gpio_init(LED);
  gpio_init(IR);
  gpio_set_dir(LED, GPIO_OUT);
  gpio_set_dir(IR, GPIO_IN);
  add_repeating_timer_us(-526, read_pulse_cb, NULL, &read_timer);
  board_init();
  tusb_init();
  gpio_put(LED, 1);
   while (1)
  {
    tud_task(); // tinyusb device task
    hid_task();
  }
  while(1);
  return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, bool key_ready)
{
  // skip if hid is not ready yet
  if ( !tud_hid_ready() ) return;

  switch(report_id)
  {
    case REPORT_ID_KEYBOARD:
    {
      // use to avoid send multiple consecutive zero report for keyboard
      static bool has_keyboard_key = false;

      if ( key_ready )
      {
        bool decode_deu_certo = 1;
        uint8_t keycode[6] = { 0 };
        switch(last_result){
          case 0xFFA25D: //Power
            break;
          case 0xFFE21D: //Raio
            keycode[0] = HID_KEY_F12;
            break;
          case 0xFFA857: //OK
            keycode[0] = HID_KEY_F11;
            break;
          case 0xFF02FD: //Cima
            break;
          case 0xFF9867: //Baixo
            break;
          case 0xFFE01F: //Esquerda+
            break;
          case 0xFF906F: //Direita
            break;
          case 0xFF30CF: //A
            keycode[0] = HID_KEY_A;
            break;
          case 0xFF18E7: //B
            keycode[0] = HID_KEY_B;
            break;
          case 0xFF7A85: //C
            keycode[0] = HID_KEY_C;
            break;
          case 0xFF10EF: //D
            keycode[0] = HID_KEY_D;
            break;
          case 0xFF38C7: //E
            keycode[0] = HID_KEY_E;
            break;
          case 0xFF5AA5: //F
            keycode[0] = HID_KEY_F;
            break;
          case 0xFF42BD: //G
            keycode[0] = HID_KEY_G;
            break;
          case 0xFF4AB5: //H
            keycode[0] = HID_KEY_H;
            break;
          case 0xFF52AD: //I
            keycode[0] = HID_KEY_I;
            break;
          default:
            decode_deu_certo = 0;
            break;
        }
        flag_readydecode = 0;
        if(decode_deu_certo){
          tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
          has_keyboard_key = true;
        }
        /*else{
          if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
          has_keyboard_key = false;
        }
        */
      }else
      {
        // send empty key report if previously has key pressed
        if (has_keyboard_key) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        has_keyboard_key = false;
      }
    }
    break;

    default: break;
  }
}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  // Remote wakeup
  if ( tud_suspended() && flag_readydecode )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_KEYBOARD, flag_readydecode);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t itf, uint8_t const* report, uint8_t len)
{
  (void) itf;
  (void) len;

  uint8_t next_report_id = report[0] + 1;

  if (next_report_id < REPORT_ID_COUNT)
  {
    send_hid_report(next_report_id, board_button_read());
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  // TODO set LED based on CAPLOCK, NUMLOCK etc...
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;
}

bool read_pulse_cb(struct repeating_timer *t){
    bool IR_READ = gpio_get(IR);
    if(IR_READ){
        if(!flag_decode && cont_zero > 15 && cont_decode == BIT_LENGTH - 1) flag_decode = 1;
        cont_zero = 0;
        if(cont_one <= 21) cont_one++;
    }else{
        if(flag_decode){
            if(cont_one == 1){
              cont_decode--;
            }else if(cont_one >= 2 && cont_decode != BIT_LENGTH - 1){
                if(decode == 0) cont_decode = 23;
                if(cont_decode < 24) decode += pow(2, cont_decode);
                cont_decode--;
            }
            //printf("%d\n", cont_decode);
        }
        cont_zero++;
        cont_one = 0;
    }
    if(cont_decode < 0 || cont_one > 20 || cont_zero > 20){
        flag_decode = 0;
        cont_decode = BIT_LENGTH - 1;
        last_result = decode;
        flag_readydecode = 1;
        cont_zero = cont_one = 0;
        decode = 0;
    }
}