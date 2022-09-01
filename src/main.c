#include <fpioa.h>
#include <gpio.h>
#include <uarths.h>
#include <sysctl.h>
//#include <i2s.h>


#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "algorithm_selection.h"


#include <fcntl.h>
#include <unistd.h>


//#include "led.h"
#include "lcd.h"
#include "nt35310.h"
#include "board_config.h"
//#include "image.h"

#if AEAD
#include "crypto_aead.h"
#elif HASH
#include "crypto_hash.h"
#elif AUTH
#include "crypto_auth.h"
#elif AES
  #if SOFTORHARD
#include "aes.h"
  #else
#include "aes_soft.h"
  #endif
#elif SHA256
#include "sha256.h"
#endif

#if AEAD || HASH || AUTH
#include "api.h"
#endif

#define FRAME_LEN   512

#define BLOCK_NUM 1000

QueueHandle_t queue;
QueueHandle_t encrypt_queue;
QueueHandle_t decrypt_queue;
QueueHandle_t LCD_queue;
QueueHandle_t encrypt_time;
QueueHandle_t decrypt_time;

#if AEAD
unsigned long long clen = CRYPTO_ABYTES;
unsigned char a[] = "ASCON";
//unsigned char npub[CRYPTO_NPUBBYTES] = "7683950492347567";
//unsigned char k[CRYPTO_KEYBYTES] = "4356245245435342";
unsigned char npub[CRYPTO_NPUBBYTES] = {0};
unsigned char k[CRYPTO_KEYBYTES] = {0};

unsigned long long alen=sizeof(const char)*5;

struct encryptinfo {
    unsigned char nsec[CRYPTO_NSECBYTES];
    unsigned char c[sizeof(const char)*8 + CRYPTO_ABYTES];
    unsigned char m[sizeof(const char)*8];
    int result;
}encryptinfo;

#elif HASH
unsigned char h[CRYPTO_BYTES] = {0};

#elif AUTH
unsigned char k[CRYPTO_KEYBYTES] = {0};
unsigned char t[CRYPTO_BYTES] = {0};

#elif AES

  #if BIT128
unsigned char k[sizeof(const char)*16] = {0};
unsigned long long klen=sizeof(const char)*16;
  #elif BIT192
unsigned char k[sizeof(const char)*24] = {0};
unsigned long long klen=sizeof(const char)*24;
  #elif BIT256
unsigned char k[sizeof(const char)*32] = {0};
unsigned long long klen=sizeof(const char)*32;
  #endif

  #if CBC
unsigned char iv[sizeof(const char)*16] = {0};
    #if SOFTORHARD
cbc_context_t cont={
  .iv=iv,
  .input_key=k
};
    #endif

  #elif GCM
unsigned char iv[sizeof(const char)*12] = {0};
gcm_context_t cont={
  .iv=iv,
  .input_key=k,
  .gcm_aad="AUTH",
  .gcm_aad_len=4
};
unsigned char tag[sizeof(const char)*16];

  #elif CTR
unsigned char iv[sizeof(const char)*16] = {0};

    #endif

  #if !SOFTORHARD
struct AES_ctx cont;
  #endif

#elif SHA256
unsigned char c[sizeof(const char)*32];

#endif


#if AES
unsigned char message[sizeof(const char)*16]="1234567890123456";
unsigned long long mlen=sizeof(const char)*16;
#else
unsigned char message[sizeof(const char)*8]="12345678";
unsigned long long mlen=sizeof(const char)*8;
#endif


uint32_t g_lcd_gram[LCD_X_MAX * LCD_Y_MAX / 2] __attribute__((aligned(128)));


void ControlTask(void* p)
{
	bool state = false;
	while(true)
	{
		xQueueSend(queue, &state, portMAX_DELAY);

		state = !state;
		vTaskDelay(500);
	}
}


static void io_set_power(void)
{
#if BOARD_LICHEEDAN
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
#else
    sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
#endif
}

static void io_mux_init(void)
{
#if BOARD_LICHEEDAN
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
    sysctl_set_spi0_dvp_data(1);
#else
    fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(6, FUNC_SPI0_SS3);
    fpioa_set_function(7, FUNC_SPI0_SCLK);
    sysctl_set_spi0_dvp_data(1);
#endif
}

void LCDTask(void* p){

	io_mux_init();
	io_set_power();
	lcd_init();

#if BOARD_LICHEEDAN
    	lcd_set_direction(DIR_XY_LRDU);
#endif

    lcd_clear(RED);
    lcd_draw_picture(0, 0, 240, 320, g_lcd_gram);
    lcd_draw_string(16, 0, "         BENCHMARK         ", RED);
#if AES
    unsigned char mes[sizeof(const char)*16];
#else
    unsigned char mes[sizeof(const char)*8];
#endif
	  bool state;
    int pos=20;
    TickType_t time;
    char str[6];
	  while(true){
        xQueueReceive(queue, &state, portMAX_DELAY);
        xQueueReceive(encrypt_time, &time, portMAX_DELAY);

        if(pos>=320){
            lcd_clear(RED);
            lcd_draw_picture(0, 0, 240, 320, g_lcd_gram);
            lcd_draw_string(16, 0, "         BENCHMARK         ", RED);
            pos=20;

        }

        sprintf(str, "%lu", time);
        lcd_draw_string(16, pos, str, RED);
        pos+=20;

#if AEAD || AES
		    xQueueReceive(LCD_queue, &mes, portMAX_DELAY);
        lcd_draw_string(16, pos, mes, RED);
        pos+=20;

        xQueueReceive(decrypt_time, &time, portMAX_DELAY);
        sprintf(str, "%lu", time);
        lcd_draw_string(16, pos, str, RED);
        pos+=20;
#endif
	  }
}


void crypto_handle(){
	  bool state;
    //int len,kant;
    /*char *senddata="Kaizo\n";
    uarths_init();
    uarths_config(115200, UARTHS_STOP_1);
    uarths_send_data(senddata, sizeof(senddata));*/
  
    char *portname = "/dev/ttyUSB0";
    int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    write (fd, "Kaixo\n", 6);
    printf("Kaixo\n");

    while(true){
        xQueueReceive(queue, &state, portMAX_DELAY);
          
        /*
        len=strlen((const char*)message);
        kant=len/8;
        if(len%8!=0){
            kant+=1;
        }
        for (int i=0;i<kant;i++){
            for (int j=0;j<8;j++){
                mes[j]=message[j+(i*8)];
            }

            xQueueSend(encrypt_queue, &mes, portMAX_DELAY);
        }*/
        xQueueSend(encrypt_queue, &message, portMAX_DELAY);
    }
}

void encryption(void* p){

#if AEAD
    struct encryptinfo info;
#elif AES
    unsigned char m[sizeof(const char)*16];
    unsigned char c[sizeof(const char)*16] __attribute__ ((aligned (16)));
#else
    unsigned char m[sizeof(const char)*8];
#endif

    TickType_t time;
    while(true){

#if AEAD
        xQueueReceive(encrypt_queue, &info.m, portMAX_DELAY);
#else
        xQueueReceive(encrypt_queue, &m, portMAX_DELAY);
#endif

        time=xTaskGetTickCount();
        for (int i=0; i<BLOCK_NUM;i++){

#if AEAD
            crypto_aead_encrypt(info.c, &clen, info.m, mlen, a, alen, info.nsec, npub, k);
#elif HASH
            crypto_hash(h, m, mlen);
#elif AUTH
            crypto_auth(t, m, mlen, k);
            //crypto_auth_verify();
#elif AES
  #if ECB
    #if SOFTORHARD
      #if BIT128
            aes_ecb128_hard_encrypt(k, m, mlen, c);
      #elif BIT192
            aes_ecb192_hard_encrypt(k, m, mlen, c);
      #elif BIT256
            aes_ecb256_hard_encrypt(k, m, mlen, c);
      #endif
    #else
            AES_init_ctx(&cont, k);
            AES_ECB_encrypt(&cont, m);
    #endif

  #elif CBC
    #if SOFTORHARD
      #if BIT128
            aes_cbc128_hard_encrypt(&cont, m, mlen, c);
      #elif BIT192
            aes_cbc192_hard_encrypt(&cont, m, mlen, c);
      #elif BIT256
            aes_cbc256_hard_encrypt(&cont, m, mlen, c);
      #endif
    #else
            AES_init_ctx_iv(&cont, k, iv);
            AES_CBC_encrypt_buffer(&cont, m, 16);
    #endif

  #elif GCM
    #if BIT128
            aes_gcm128_hard_encrypt(&cont, m, mlen, c, tag);
    #elif BIT192
            aes_gcm192_hard_encrypt(&cont, m, mlen, c, tag);
    #elif BIT256
            aes_gcm256_hard_encrypt(&cont, m, mlen, c, tag);
    #endif

  #elif CTR
            AES_init_ctx_iv(&cont, k, iv);
            AES_CTR_xcrypt_buffer(&cont, m, 16);
  #endif
#elif SHA256
            sha256_hard_calculate(m, mlen, c);
#endif
        }
        time=xTaskGetTickCount()-time;
#if AEAD
        xQueueSend(decrypt_queue, &info, portMAX_DELAY);
#elif AES
  #if SOFTORHARD
        xQueueSend(decrypt_queue, &c, portMAX_DELAY);
  #else
        xQueueSend(decrypt_queue, &m, portMAX_DELAY);
  #endif
#endif
        xQueueSend(encrypt_time, &time, portMAX_DELAY);
    }
}


#if (AEAD || AES)
void decryption(void* p){

  #if AEAD
    struct encryptinfo info;
  #elif AES
    unsigned char m[sizeof(const char)*16];
    unsigned char c[sizeof(const char)*16] __attribute__ ((aligned (16)));

  #endif
    TickType_t time;
  int pos=80;

    while(true){

  #if AEAD
        xQueueReceive(decrypt_queue, &info, portMAX_DELAY);
  #elif AES
        xQueueReceive(decrypt_queue, &c, portMAX_DELAY);
  #endif
        time=xTaskGetTickCount();
        for (int i=0; i<BLOCK_NUM;i++){
  #if AEAD
            crypto_aead_decrypt(info.m, &mlen, info.nsec, info.c, clen, a, alen, npub, k);
  #elif AES
    #if ECB
      #if SOFTORHARD
        #if BIT128
            aes_ecb128_hard_decrypt(k, c, mlen, m);
        #elif BIT192
            aes_ecb192_hard_decrypt(k, c, mlen, m);
        #elif BIT256
            aes_ecb256_hard_decrypt(k, c, mlen, m);
        #endif
      #else
            AES_init_ctx(&cont, k);
            AES_ECB_decrypt(&cont, c);
      #endif

    #elif CBC
      #if SOFTORHARD
        #if BIT128
            aes_cbc128_hard_decrypt(&cont, c, mlen, m);
        #elif BIT192
            aes_cbc192_hard_decrypt(&cont, c, mlen, m);
        #elif BIT256
            aes_cbc256_hard_decrypt(&cont, c, mlen, m);
        #endif
      #else
            AES_init_ctx_iv(&cont, k, iv);
            AES_CBC_decrypt_buffer(&cont, c, 16);
      #endif

    #elif GCM
      #if BIT128
            aes_gcm128_hard_decrypt(&cont, c, mlen, m, tag);
      #elif BIT192
            aes_gcm192_hard_decrypt(&cont, c, mlen, m, tag);
      #elif BIT256
            aes_gcm256_hard_decrypt(&cont, c, mlen, m, tag);
      #endif

    #elif CTR
            AES_init_ctx_iv(&cont, k, iv);
            AES_CTR_xcrypt_buffer(&cont, c, 16);

    #endif
  #endif
        }
        time=xTaskGetTickCount()-time;
  #if AEAD
        xQueueSend(LCD_queue, &info.m, portMAX_DELAY);
  #elif AES
    #if SOFTORHARD
        xQueueSend(LCD_queue, &m, portMAX_DELAY);
    #else
        xQueueSend(LCD_queue, &c, portMAX_DELAY);
    #endif
  #endif
        xQueueSend(decrypt_time, &time, portMAX_DELAY);
    }
}
#endif


int main()
{
	  sysctl_pll_set_freq(SYSCTL_PLL0, configCPU_CLOCK_HZ * 2);
    queue = xQueueCreate( 10, sizeof(bool) );
#if AES
    encrypt_queue = xQueueCreate( 10, sizeof(const char)*16);
    LCD_queue = xQueueCreate( 10, sizeof(const char)*16);
    decrypt_queue = xQueueCreate( 10, sizeof(const char)*16);
    decrypt_time = xQueueCreate( 10, sizeof(TickType_t));
#else
    encrypt_queue = xQueueCreate( 10, sizeof(const char)*8);
    LCD_queue = xQueueCreate( 10, sizeof(const char)*8);
#endif
    encrypt_time = xQueueCreate( 10, sizeof(TickType_t));

#if AEAD
    decrypt_queue = xQueueCreate( 10, sizeof(encryptinfo));
    decrypt_time = xQueueCreate( 10, sizeof(TickType_t));
#endif
	  printf("Create tasks...\r\n");
	  //xTaskCreate(LedTask, "LedTask", 256, queue, 3, NULL);
	  xTaskCreate(ControlTask, "ControlTask", 256, NULL, 3, NULL);
	  xTaskCreate(LCDTask, "LCDTask", 256, NULL, 4, NULL);
    xTaskCreate(crypto_handle, "crypto_handle", 256, NULL, 3, NULL);
    xTaskCreate(encryption, "encryption", 256, NULL, 3, NULL);
#if (AEAD || AES)
    xTaskCreate(decryption, "decryption", 256, NULL, 3, NULL);
#endif
	  printf("Start scheduler...\r\n");
	  vTaskStartScheduler();

}
