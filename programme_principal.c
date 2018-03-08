#include "LPC17xx.h"
#include "uart.h"
#include <stdio.h>
#include "GPIO.h"
#include "GLCD.h"
  
	
	void PWM_Init();
	void PWM_Droite(int rapport_cyclique);
	void PWM_Gauche(int rapport_cyclique);
	void LidarInit();
	void Tempo_Init(unsigned int prescaler, unsigned int valeur);
	void affichage(int a);
	
	int etat = 0;
	int test = 0;
void TIMER0_IRQHandler(void)
{
	LPC_TIM0->IR |= 1<<0;   // drapeau IT Timer0

	test = 1;
}
	int main (void) 
  {
		char  qualite,  angle1,  angle2,  distance1 ,  distance2, trame[5], a;// Chaine de caractères (53 max en petite police / 20 max en grande police)
		int somme, etat=0, k;
		long int i;
		short angle, distance;
	
		Init_UART();
	  Initialise_GPIO (); // init GPIO
		PWM_Init();
		LidarInit();
		GLCD_Init();						// Initialisation LCD
		GLCD_Clear(White);			// Fond d'ecran en blanc
		Tempo_Init(1,6);
		
		NVIC_EnableIRQ(TIMER0_IRQn);
		
		LPC_GPIO2->FIODIR1=0x01;
		
		//LPC_UART2->THR = 0xA5; // Envoi de la donnée (valeur 0xA5)
			
		//LPC_UART2->THR = 0x20; // Envoi de la donnée (valeur 0x20)
		
		LPC_GPIO2->FIODIR0|=0x04;
		
		while(1)
		{
			UART_PutChar(0xA5);
			
			switch (etat)
		{
         case 0: 
					 PWM_Droite(70);
					 PWM_Gauche(70);
					 affichage(0);
				 
					 if(test == 1)
					 {
						 test = 0;
						 Tempo_Init(0,7);
				     etat = 1;
					 }
				 break;
				 
				 case 1: 
					 PWM_Droite(70);
					 PWM_Gauche(50);
				   affichage(1);
				 
				 if(test == 1)
					 {
						 test = 0;
						 Tempo_Init(3,0);
				     etat = 2;
					 }
				 break;
				 
				 case 2: 
					 PWM_Droite(65);
					 PWM_Gauche(65);
				   affichage(2);
				 if(test == 1)
					 {
						 test = 0;
						 Tempo_Init(0,6);
				     etat = 3;
					 }
				 break;
					 
				case 3: 
					 PWM_Droite(70);
					 PWM_Gauche(50);
				   affichage(3);
				  
				 if(test == 1)
					 {
						 test = 0;
						 Tempo_Init(1,7);
				     etat = 0;
					 }
				 
				 break;	 
    }
		}
		
		/*while(1)
		{	
			
			
			a=UART_GetChar();
			LPC_GPIO2->FIOPIN0|=0x04;
			
		if(a == 0xA5)
		{	
			
			a=UART_GetChar();
			if(a == 0x5A)
			{
				for(k=0; k<5 ; k++)
				{
					trame[k] = UART_GetChar();
					
				}
			}
		}
		
			if(trame[0]!=0x00)
			{
				qualite=(trame[0] >> 2);
				angle1=(trame[1] >> 1);
				angle2=trame[2];
				distance1=trame[3];
				distance2=trame[4];
			
				angle=((angle2<<7) | angle1) / 64.00;
				distance=((distance2<<8) | distance1) /4.00;
			}
		
		}*/
	}
void PWM_Init()
{
	  LPC_PINCON->PINSEL4 = 0x00000005;
		LPC_SC->PCONP = LPC_SC->PCONP | 0x00000040;	// enable PWM1
		
		LPC_PWM1->PR = 11;  // prescaler
		LPC_PWM1->MR0 = 99;
		
	  LPC_PWM1->MCR = LPC_PWM1->MCR | 0x00000002; // Timer relancé quand MR0 repasse à 0
		LPC_PWM1->LER = LPC_PWM1->LER | 0x0000000F;  // ceci donne le droit de modifier dynamiquement la valeur du rapport cyclique
	                                             // bit 0 = MR0    bit3 = MR3
		LPC_PWM1->PCR = LPC_PWM1->PCR | (7<<9);  // autorise les sorties PWM1.1 et PWM1.2  
		
		LPC_PWM1->MR1 = 0; //remise à 0 des compteurs
		LPC_PWM1->MR2 = 0;
		LPC_PWM1->MR3 = 39;
		
		LPC_PWM1->TCR = 1;  /*validation de timer 1 et reset counter */
}


void PWM_Droite(int rapport_cyclique)
{
		LPC_PWM1->MR1 = rapport_cyclique;    // ceci ajuste la duree de l'état haut pour le moteur droit
}

void PWM_Gauche(int rapport_cyclique)
{
		LPC_PWM1->MR2 = rapport_cyclique;    // ceci ajuste la duree de l'état haut pour le moteur gauche
}

void LidarInit(int rapport_cyclique)
{
		LPC_PINCON->PINSEL7 = LPC_PINCON->PINSEL7 | 0x00300000;
		LPC_PWM1->MR3 = rapport_cyclique;
}

void Tempo_Init(unsigned int secondes, unsigned int decisecondes)
{
	float calcul = 0;
	int valeur = 0;
	
	calcul = decisecondes/10.0;
	if(decisecondes == 0) calcul = 1.0;
	
	calcul = 25000000*calcul;
	
	valeur = (int)calcul;
	
LPC_SC->PCONP |= (1<<1); //allume le timer 0 (facultatif, déjà allumé après un reset)

LPC_TIM0->PR =  secondes;
LPC_TIM0->MR0 = valeur; 

LPC_TIM0->MCR = 3;		/*reset compteur si MR0=COUNTER + interruption*/

LPC_TIM0->TCR = 1; 		//démarre le comptage
} 

void affichage(int a)
{
	char Chaine_char[53] = "";
	
	sprintf(Chaine_char, "Etat = %d", a);	// creation chaine texte
	GLCD_DisplayString (1,0,1,Chaine_char);		// affichage chaine ligne 0, colonne 0, petite police
}