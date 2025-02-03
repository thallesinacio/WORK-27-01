#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
 
//ALUNO: Thalles Inácio Araújo
// MATRÍCULA: tic370101531



#define led_red 13
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b);

//arquivo .pio
#include "pio_matrix.pio.h"

//número de LEDs
#define NUM_PIXELS 25

//pino de saída
#define OUT_PIN 7

//botão de interupção
const uint button_0 = 5;
const uint button_1 = 6;
int idx = 0;

//Variáveis declaradas globalmente pra facilitar a comunicação entre funções
PIO pio;
uint sm;
uint32_t VALOR_LED;
unsigned char R, G, B;
static volatile uint32_t last_time = 0;

//matriz contendo a intensidade dos leds de cada algarismo 
double matrix[10][25] = {
    {0.0, 0.2, 0.2, 0.2, 0.0, 0.2, 0.0, 0.0, 0.0, 0.2, 0.2, 0.0, 0.0, 0.0, 0.2, 0.2, 0.0, 0.0, 0.0, 0.2, 0.0, 0.2, 0.2, 0.2, 0.0},
    {0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.2, 0.0, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0},
    {0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0},
    {0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0},
    {0.0, 0.2, 0.0, 0.2, 0.0, 0.0, 0.2, 0.0, 0.2, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.2, 0.0},
    {0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0},
    {0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.2, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0},
    {0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.2, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.2, 0.0},
    {0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.2, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.2, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0},
    {0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.2, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0, 0.0, 0.2, 0.0, 0.0, 0.0, 0.0, 0.2, 0.2, 0.2, 0.0}
};


//rotina da interrupção
static void gpio_irq_handler(uint gpio, uint32_t events){
    
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    
        // Verifica se passou tempo suficiente desde o último evento (debouncing)
        if (current_time - last_time > 600000){ // 600 ms 
            last_time = current_time; // Atualiza o tempo do último evento
            
            if (gpio == button_1) {  // Avança
                idx++;  
                if (idx > 9) {  // Se passou de 9, volta para 0
                    idx = 0;
                }
            } else if (gpio == button_0) {  // Retrocede
                if (idx == 0) {  // Se estava em 0, volta para 9
                    idx = 9;
                } else {
                    idx--;  
                }
            }
        }
    desenho_pio(matrix[idx], VALOR_LED, pio, sm, R, G, B);
}

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g)
{
  //unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b){

    for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(desenho[24-i], r=0.0, g=0.0);
            pio_sm_put_blocking(pio, sm, valor_led);
    }
}

//função principal
int main()
{

    double r = 0.0, b = 0.0 , g = 0.0;
    bool ok;
    //coloca a frequência de clock para 128 MHz, facilitando a divisão pelo clock
    ok = set_sys_clock_khz(128000, false);
    pio = pio0;
    // Inicializa todos os códigos stdio padrão que estão ligados ao binário.
    stdio_init_all();

    printf("iniciando a transmissão PIO");
    if (ok) printf("clock set to %ld\n", clock_get_hz(clk_sys));

    //configurações da PIO
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    //inicializar o botão de interrupção - GPIO5
    gpio_init(button_0);
    gpio_set_dir(button_0, GPIO_IN);
    gpio_pull_up(button_0);

    //inicializar o botão de interrupção - GPIO5
    gpio_init(button_1);
    gpio_set_dir(button_1, GPIO_IN);
    gpio_pull_up(button_1);


    gpio_init(led_red);
    gpio_set_dir(led_red,GPIO_OUT);
    desenho_pio(matrix[idx], VALOR_LED, pio, sm, R, G, B);
    
    //interrupção da gpio habilitada
    gpio_set_irq_enabled_with_callback(button_0, GPIO_IRQ_EDGE_FALL, true, & gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_1, GPIO_IRQ_EDGE_FALL, true, & gpio_irq_handler);

    while (true) {
        gpio_put(led_red,true);
        sleep_ms(100);
        gpio_put(led_red,false);
        sleep_ms(100);
    }
    
}