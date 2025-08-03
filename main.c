#include "MKL05Z4.h"
#include "lcd1602.h"
#include "frdm_bsp.h"
#include "klaw.h"
#include "TPM.h"
#include "tsi.h"
#include "leds.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

volatile uint8_t S2_press=0;
volatile uint8_t S3_press=0;
volatile uint8_t S4_press=0;
uint8_t ust_zegar=0;
uint8_t ust_alarm=0;
uint8_t ust_drzm=0;
uint8_t alarm = 0;
uint8_t minutyAlarm, godzinaAlarm; 
uint8_t sekundy, minuty, godzina;
uint8_t iloscDrzemki = 0;
void PORTA_IRQHandler(void)	// Podprogram obslugi przerwania od klawiszy S2, S3 i S4
{
	uint32_t buf;
	buf=PORTA->ISFR & (S2_MASK | S3_MASK | S4_MASK);

	switch(buf)
	{
		case S2_MASK:	if(!S2_press)
									if(!(PTA->PDIR&S2_MASK))		// Minimalizacja drgan zestyków
									{
										if(!(PTA->PDIR&S2_MASK))	
										{
											if(!S2_press)
											{
												S2_press=1;
											}
										}
									}
									break;
		case S3_MASK:	DELAY(10)
									if(!(PTA->PDIR&S3_MASK))		// Minimalizacja drgan zestyków
									{
										if(!(PTA->PDIR&S3_MASK))	
										{
											if(!S3_press)
											{
												S3_press=1;
											}
										}
									}
									break;
		case S4_MASK:	DELAY(10)
									if(!(PTA->PDIR&S4_MASK))		// Minimalizacja drgan zestyków
									{
										if(!(PTA->PDIR&S4_MASK))	
										{
											if(!S4_press)
											{
												S4_press=1;
											}
										}
									}
									break;
		default:			break;
	}	
	PORTA->ISFR |=  S2_MASK | S3_MASK | S4_MASK;	// Kasowanie wszystkich bitów ISF
	NVIC_ClearPendingIRQ(PORTA_IRQn);
}
void rtc_init(void) { 												// Inicjalizacja RTC
    int i;
    SIM->SCGC6 |= SIM_SCGC6_RTC_MASK;					// Wlaczenie sygnalu zegarowego RTC (Real-Time Clock)

    SIM->SOPT1 = SIM_SOPT1_OSC32KSEL(0);			// Ustawienie zródla zegara 32 kHz do RTC

    NVIC_DisableIRQ(RTC_IRQn);								// Wylaczenie przerwan RTC - wylaczamy obsluge przerwan

    RTC->CR = RTC_CR_SWR_MASK;								// Soft reset rejestru kontrolnego RTC, czekanie na zakonczenie resetu
    RTC->CR &= ~RTC_CR_SWR_MASK;  

    if (RTC->SR & RTC_SR_TIF_MASK) {					// Sprawdzenie czy wystapilo przepelnienie czasu (Time Invalid Flag)
        
        RTC->TSR = 0x00000000;
    }

    RTC->CR |= RTC_CR_OSCE_MASK | RTC_CR_SC16P_MASK;		// Wlaczenie oscylatora RTC

    for (i = 0; i < 0x600000; i++);											// Oczekiwanie na ustabilizowanie oscylatora

    RTC->SR |= RTC_SR_TCE_MASK;													// Wlaczenie Time Counter

    RTC->TCR = RTC_TCR_CIR(0) | RTC_TCR_TCR(0);					// Kompensacja czasu - ustawienie wartosci kompensacji

    RTC->SR &= ~(RTC_SR_TCE_MASK);											// Wylaczenie RTC, zapisanie 1 do TSR, a nastepnie wlaczenie RTC
    RTC->TSR = 1;
    RTC->SR |= RTC_SR_TCE_MASK;

    RTC->IER |= RTC_IER_TAIE_MASK;											// Konfiguracja przerwania alarmu
		
    NVIC_ClearPendingIRQ(RTC_IRQn);											// Wyzerowanie oczekujacych przerwan dla RTC
		NVIC_EnableIRQ(RTC_IRQn);														// Wlaczenie przerwan dla RTC
}
void ustawCzasZegara() { // funkcja ustawiajaca czas zegara
    uint8_t ustMinutGodz = 0;

    while (1) {
        if (S2_press) {
            S2_press = 0;
            if (!ustMinutGodz) {
                ustMinutGodz = 1; // Przechodzimy do ustawiania minut
            } else {
                ust_zegar = 1;
                break;
            }
        } else if (ustMinutGodz && S3_press) {
            minuty++;
            if (minuty >= 60) {
                minuty = 0;
            }
            S3_press = 0;
        } else if (ustMinutGodz && S4_press) {
            if (minuty > 0) {
                minuty--;
            } else {
                minuty = 59;
            }
            S4_press = 0;
        } else if (!ustMinutGodz && S3_press) {
            godzina++;
            if (godzina >= 24) {
                godzina = 0;
            }
            S3_press = 0;
        } else if (!ustMinutGodz && S4_press) {
            if (godzina > 0) {
                godzina--;
            } else {
                godzina = 23;
            }
            S4_press = 0;
        }

        char display[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
        if (!ustMinutGodz) {
            sprintf(display, "G:%02d:%02d", godzina, minuty);
        } else {
            sprintf(display, "G:%02d:%02d", godzina, minuty); // Podkreslenie miejsca ustawiania minut
        }

        LCD1602_SetCursor(0, 0);
        LCD1602_Print(display);
    }
}
void ustawCzasAlarmu() { // funkcja ustawiajaca czas alarmu
    uint8_t ustMinutAlarm = 0;

    while (1) {
        if (S2_press) {
            S2_press = 0;
            if (!ustMinutAlarm) {
                ustMinutAlarm = 1; // Przechodzimy do ustawiania minut
            } else {
                ust_alarm = 1;
                break;
            }
        } else if (ustMinutAlarm && S3_press) {
            minutyAlarm++;
            if (minutyAlarm >= 60) {
                minutyAlarm = 0;
            }
            S3_press = 0;
        } else if (ustMinutAlarm && S4_press) {
            if (minutyAlarm > 0) {
                minutyAlarm--;
            } else {
                minutyAlarm = 59;
            }
            S4_press = 0;
        } else if (!ustMinutAlarm && S3_press) {
            godzinaAlarm++;
            if (godzinaAlarm >= 24) {
                godzinaAlarm = 0;
            }
            S3_press = 0;
        } else if (!ustMinutAlarm && S4_press) {
            if (godzinaAlarm > 0) {
                godzinaAlarm--;
            } else {
                godzinaAlarm = 23;
            }
            S4_press = 0;
        }

        char display[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
        if (!ustMinutAlarm) {
            sprintf(display, "A:%02d:%02d", godzinaAlarm, minutyAlarm);
        } else {
            sprintf(display, "A:%02d:%02d", godzinaAlarm, minutyAlarm); // Podkreslenie miejsca ustawiania minut
        }

        LCD1602_SetCursor(0, 0);
        LCD1602_Print(display);
    }
}
void ustawIloscDrzemki() {				//funkcja ustawiajaca drzemki
    while (1) {
        if (S2_press) {
            S2_press = 0;
						ust_drzm=1;
            break;
        }
				else if (S3_press) {
						iloscDrzemki++;
            S3_press = 0;
				}
				if (iloscDrzemki >= 61) {
            iloscDrzemki = 0;
        }
        char display[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
        sprintf(display, "Drzemka: %02d", iloscDrzemki);
        LCD1602_SetCursor(0, 0);
        LCD1602_Print(display);
    }
}
int main() {
		char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
		LED_Init();
		TSI_Init();
		Klaw_Init();								// Inicjalizacja klawiatury
		Klaw_S2_4_Int();
		LCD1602_Init();
		LCD1602_Print("BUDZIK(S1-start)");
		while(PTA->PDIR&S1_MASK);  	// Czy klawisz S1 wcisniety?
		LCD1602_ClearAll();
		uint8_t w=0;
		while(1){
		if (!ust_zegar) {												//ustawienie czasu zegara i alarmu
            ustawCzasZegara();
        } else if (!ust_alarm) {
            ustawCzasAlarmu();
        }else if (!ust_drzm) {
            ustawIloscDrzemki();
        } 
			else {
			rtc_init();
			uint32_t startTimestamp = RTC->TSR - ((godzina * 3600) + (minuty * 60) + sekundy);
			while(1){
				w=TSI_ReadSlider();
        uint32_t aktualnyCzas = RTC->TSR;
        uint32_t uplSekundy = aktualnyCzas - startTimestamp;
				godzina = (uplSekundy / 3600) % 24;
        minuty = (uplSekundy / 60) % 60;
        sekundy = uplSekundy % 60;
				if(godzina == godzinaAlarm && minuty == minutyAlarm){
							alarm = 1;
				}
				if(alarm){
				PTB->PTOR|=LED_R_MASK;			// uruchomienie alarmu
				}
				if(alarm && S2_press){		//ustawianie drzemki
				PTB->PCOR|=LED_R_MASK; 		//sygnalizacja drzemki
				alarm = 0;
				godzinaAlarm = godzina;
				minutyAlarm = minuty + iloscDrzemki;
				if (minutyAlarm >= 60) {
            minutyAlarm = minutyAlarm-60;
						godzinaAlarm++;
        }
				S2_press=0;
				}
				if(w!=0 && alarm){						//wylaczenie alarmu sliderem
				godzinaAlarm=NULL;
				minutyAlarm=NULL;
				alarm = 0;
				PTB->PSOR|=LED_R_MASK; 			//wylaczenie diody
				}
				sprintf(display,"Budzik:%02d:%02d:%02d",godzina,minuty,sekundy);  //wyswietlanie budzika
				LCD1602_SetCursor(0,0);
				LCD1602_Print(display);
				for (int i = 0; i < 1000000; i++) {								// Opóznienie dla widocznosci zmiany czasu
            __NOP();
        }
			}
			}
		}		
}
