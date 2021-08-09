#define INIT_C
#include "equates.h"

void megaPEG_init(void)
    {
      //*** init IO ***
      outp(0x00, DDRA);                       // D0:7 & A0:7 (input,pullup)
      outp(0xff, PORTA);

      outp(0x00, DDRB);                       // D8:15 & A8:15 (input,pullup)
      outp(0xff, PORTB);

      //outp(0xFF, PORTC);                  // A8:15 & A8:15 (output,high)
      outp((1<<PC7)|                        // ideSEL, lcdSEL
          (0<<PC6)|                        // /     , lcdENABLE
          (1<<PC5)|                        // DIOW  ,  /
          (1<<PC4)|                        // DIOR  ,  /
          (1<<PC3)|                        // DA1   , lcdDI
          (1<<PC2)|                        // DA0   , lcdRW
          (1<<PC1)|                        // DA2   , lcdCS1
          (1<<PC0), PORTC);                // CS1   , lcdCS2

      outp((0<<ideIRQ)|                      // input
          (0<<i2cIRQ)|                       // input
          (0<<PD2)|                          // input
          (0<<PD3)|                          // input
          (0<<PD4)|                          // input
          (1<<LED), DDRD);                   // output
      outp((1<<ideIRQ)|                      // pulup
          (1<<i2cIRQ)|                       // pulup
          (1<<PD2)|                          // pulup
          (1<<PD3)|                          // pulup
          (1<<PD4)|                          // pulup
          (1<<LED), PORTD);                  // out high

      outp((0<<RXD)|                         // input
          (0<<TXD)|                          // output
          (0<<masDEM)|                       // input
          (0<<rswIRQ)|                       // input
          (0<<rswSW)|                        // input
          (0<<IR), DDRE);                    // input
      outp((0<<RXD)|                         // pulup
          (0<<TXD)|                          // out low
          (1<<masDEM)|                       // pulup
          (1<<rswIRQ)|                       // pulup
          (1<<rswSW)|                        // pulup
          (1<<IR), PORTE);                   // pulup

      // port F all input

      //*** init uart ***
      uart_init();


      //*** init irq ***
      if(bit_is_set(PINE,rswIRQ))
        outp((1<<ISC51)|(0<<ISC50)|
            (1<<ISC61)|(0<<ISC60)|
            (1<<ISC71)|(0<<ISC70), EICR);     // next -\_
      else
        outp((1<<ISC41)|(0<<ISC40)|
            (1<<ISC51)|(1<<ISC50)|
            (1<<ISC61)|(0<<ISC60)|
            (1<<ISC71)|(0<<ISC70), EICR);     // next _/-

      //outp((1<<s165), EIMSK);   // enable external s16 5,6,7    (1<<s164)|
}
