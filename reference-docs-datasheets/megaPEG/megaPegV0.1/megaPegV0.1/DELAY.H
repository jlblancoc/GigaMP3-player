#ifndef DELAY_H
	#define DELAY_H

	#define K_DELAY_100us	F_CPU/61349
	#define K_DELAY_1ms		F_CPU/6013
	#define K_DELAY_10ms	F_CPU/600

	void Delay_100us(u08 t);
	void Delay_1ms(u08 t);
	void Delay_10ms(u08 t);
#endif

