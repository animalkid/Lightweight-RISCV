//Include needed Kendryte SDK libraries.
#include <fpioa.h>
#include <gpio.h>
#include <uarths.h>
#include <sysctl.h>
#include <uart.h>

//Include needed FreeRTOS libraries.
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

//Include standard libraries.
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

//Include the library that defines the compiled algorithm.
#include "algorithm_selection.h"

#include <fcntl.h>
#include <unistd.h>

//Include needed libraries for the LCD use and board configuration library.
#include "lcd.h"
#include "nt35310.h"
#include "board_config.h"

//Include library that implements the compiled algorithm.
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

//In the case of ASCON algorithms, include the api library.
#if AEAD || HASH || AUTH
#include "api.h"
#endif

#define FRAME_LEN   512

//Define the number of blocks to be ciphered.
#define BLOCK_NUM 1000

//Define the block size depending on the algorithm. Only the block size for AEAD, HASH, or AUTH algorithms can be edited.
#if AES
#define BLOCK_SIZE 16
#elif AEAD || HASH || AUTH
#define BLOCK_SIZE 8
#else
#define BLOCK_SIZE 8
#endif

//Define the UART number and the BAUDRATE
#define UART_NUM UART_DEVICE_3
#define BAUDRATE 115200

QueueHandle_t queue;
QueueHandle_t encrypt_queue;
QueueHandle_t decrypt_queue;
QueueHandle_t LCD_queue;
QueueHandle_t encrypt_time;
QueueHandle_t decrypt_time;

#if AEAD
unsigned long long clen = CRYPTO_ABYTES;
unsigned char a[] = "ASCON";
unsigned char npub[CRYPTO_NPUBBYTES] = {0};
unsigned char k[CRYPTO_KEYBYTES] = {0};

unsigned long long alen=sizeof(const char)*5;

struct encryptinfo {
    unsigned char nsec[CRYPTO_NSECBYTES];
    unsigned char c[sizeof(const char)*BLOCK_SIZE + CRYPTO_ABYTES];
    unsigned char m[sizeof(const char)*BLOCK_SIZE];
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


unsigned char message[sizeof(const char)*BLOCK_SIZE]={0};
unsigned long long mlen=sizeof(const char)*BLOCK_SIZE;


uint32_t g_lcd_gram[LCD_X_MAX * LCD_Y_MAX / 2] __attribute__((aligned(128)));

//This Task is the one in charge of the Task execution control
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

//Set the needed power for the LCD
static void io_set_power(void)
{
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
}
//Initialize the LCD pins
static void io_mux_init(void)
{
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
    sysctl_set_spi0_dvp_data(1);
}

//This task is the one in charge of printing the results in the LCD
void LCDTask(void* p){

//Initialize all LCD pins and power supplies, then initialize the LCD
	io_mux_init();
	io_set_power();
	lcd_init();

//Set the direction of the LCD printing
  lcd_set_direction(DIR_XY_LRDU);

//Clear the LCD and start drawing characters
  lcd_clear(RED);
  lcd_draw_picture(0, 0, 240, 320, g_lcd_gram);
  lcd_draw_string(16, 0, "         BENCHMARK         ", RED);

//Initialize all the variables, the message size depends on the used algorithm
  unsigned char mes[sizeof(const char)*BLOCK_SIZE];
	bool state;
  int pos=20;
  TickType_t time;
  char str[6];
	while(true){
    xQueueReceive(queue, &state, portMAX_DELAY);
    xQueueReceive(encrypt_time, &time, portMAX_DELAY);

//If the printing position is higher than the LCD max size, then refresh the LCD and start printing again.
    if(pos>=320){
      lcd_clear(RED);
      lcd_draw_picture(0, 0, 240, 320, g_lcd_gram);
      lcd_draw_string(16, 0, "         BENCHMARK         ", RED);
      pos=20;
    }
//Print the elapsed encryption time
    sprintf(str, "%lu", time);
    lcd_draw_string(16, pos, str, RED);
    pos+=20;
    strcat(str, "\n");
    uart_send_data(UART_NUM, str, strlen(str));

//In the case of AEAD or AES algorithms, also print the elapsed decryption time.
#if AEAD || AES
		xQueueReceive(LCD_queue, &mes, portMAX_DELAY);
    lcd_draw_string(16, pos, mes, RED);
    pos+=20;
    //strcat(mes, "\n");
    //uart_send_data(UART_NUM, mes, strlen(mes));

    xQueueReceive(decrypt_time, &time, portMAX_DELAY);
    sprintf(str, "%lu", time);
    lcd_draw_string(16, pos, str, RED);
    pos+=20;
    strcat(str, "\n###### \n");
    uart_send_data(UART_NUM, str, strlen(str));
#endif
  }
}

//This task is the one in charge of delivering the messages that have to be ciphered to the encryption task.
void crypto_handle(){
  bool state;
  //int len,kant;

  while(true){
    xQueueReceive(queue, &state, portMAX_DELAY);

//A possible implementation for message splitting, in order to encrypt long messages dividing them in 8-byte blocks.
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
    }
*/
//Send the message to cipher to the encryption task.
        xQueueSend(encrypt_queue, &message, portMAX_DELAY);
    }
}

//This task is the one in charge of the message encryption process.
void encryption(void* p){

//Initialize all needed variables
#if AEAD
  struct encryptinfo info;
#elif AES
  unsigned char m[sizeof(const char)*BLOCK_SIZE];
  unsigned char c[sizeof(const char)*BLOCK_SIZE] __attribute__ ((aligned (BLOCK_SIZE)));
#else
  unsigned char m[sizeof(const char)*BLOCK_SIZE];
#endif

  TickType_t time;
  while(true){

#if AEAD
    xQueueReceive(encrypt_queue, &info.m, portMAX_DELAY);
#else
    xQueueReceive(encrypt_queue, &m, portMAX_DELAY);
#endif
//Get the time before doing the encryption process
    time=xTaskGetTickCount();

//Do the encryption process for the specified amount of blocks.
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
//Get the time after doing the encryption process and calculate the elapsed time during encryption.
    time=xTaskGetTickCount()-time;
//Send the results to the LCD task. In the case of AEAD or AES also send the resulting ciphertext to the decryption task, in order to perform decryption.
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

//This task is the one in charge of the decryption process. This task is only available for AEAD or AES algorithms.
#if (AEAD || AES)
void decryption(void* p){

//Initialize all needed variables
  #if AEAD
  struct encryptinfo info;
  #elif AES
  unsigned char m[sizeof(const char)*BLOCK_SIZE];
  unsigned char c[sizeof(const char)*BLOCK_SIZE] __attribute__ ((aligned (BLOCK_SIZE)));
  #endif

  TickType_t time;

  while(true){

  #if AEAD
    xQueueReceive(decrypt_queue, &info, portMAX_DELAY);
  #elif AES
    xQueueReceive(decrypt_queue, &c, portMAX_DELAY);
  #endif
//Get the time before doing the decryption process.
    time=xTaskGetTickCount();
//Do the decryption process for the specified amount of blocks.
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
//Get the time after doing the encryption process and calculate the elapsed time during encryption.
    time=xTaskGetTickCount()-time;
//Send the results to the LCD task.
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
//Set the CPU Clock frequency
	sysctl_pll_set_freq(SYSCTL_PLL0, configCPU_CLOCK_HZ * 2);
//Initialize all queues. AEAD and AES algorithms also include decryption process related queues.
  queue = xQueueCreate( 10, sizeof(bool) );
  encrypt_queue = xQueueCreate( 10, sizeof(const char)*BLOCK_SIZE);
  encrypt_time = xQueueCreate( 10, sizeof(TickType_t));
  LCD_queue = xQueueCreate( 10, sizeof(const char)*BLOCK_SIZE);

#if AES
  decrypt_queue = xQueueCreate( 10, sizeof(const char)*BLOCK_SIZE);
  decrypt_time = xQueueCreate( 10, sizeof(TickType_t));
#elif AEAD
  decrypt_queue = xQueueCreate( 10, sizeof(encryptinfo));
  decrypt_time = xQueueCreate( 10, sizeof(TickType_t));
#endif
//Create all the needed tasks with their respective priority. Only AEAD and AES algorithms include decryption task.
	  printf("Create tasks...\r\n");
	  xTaskCreate(ControlTask, "ControlTask", 256, NULL, 3, NULL);
	  xTaskCreate(LCDTask, "LCDTask", 256, NULL, 4, NULL);
    xTaskCreate(crypto_handle, "crypto_handle", 256, NULL, 3, NULL);
    xTaskCreate(encryption, "encryption", 256, NULL, 3, NULL);
#if (AEAD || AES)
    xTaskCreate(decryption, "decryption", 256, NULL, 3, NULL);
#endif

//Initialize UART pins and initialize and configurate UART.
    fpioa_set_function(4, FUNC_UART1_RX + UART_NUM * 2);
    fpioa_set_function(5, FUNC_UART1_TX + UART_NUM * 2);

    uart_init(UART_NUM);
    uart_config(UART_NUM, BAUDRATE, UART_BITWIDTH_8BIT, UART_STOP_1, UART_PARITY_NONE);

//Initialize the scheduler.
	  printf("Start scheduler...\r\n");
	  vTaskStartScheduler();

}
