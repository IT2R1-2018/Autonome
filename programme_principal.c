#include "LPC17xx.h"
#include "uart.h"
#include <stdio.h>
#include "GLCD.h"
#include "GPIO.h"
#include "cmsis_os.h"                       // CMSIS RTOS header file
#include "Driver_SPI.h"
#include "Driver_USART.h"               // ::CMSIS Driver:USART

	extern ARM_DRIVER_USART Driver_USART1;

	void PWM_Init();
	void PWM_Droite(int rapport_cyclique);
	void PWM_Gauche(int rapport_cyclique);
	void LidarInit();
	void affichage(int a);
	void Init_UART(void);
	void GetBTString(char *ptab,char delimFin);
	
	void automatique (void const *argument);                             // thread function
	void gant (void const *argument);                             
	void joystick (void const *argument);                             
	
	osThreadId tid_automatique, tid_gant, tid_joystick;                                          // thread id
	
	osThreadDef (automatique, osPriorityNormal, 1, 0);                   // thread object	
	osThreadDef (gant, osPriorityNormal, 1, 0);                   
	osThreadDef (joystick, osPriorityNormal, 1, 0);                   	

void GetBTString(char *ptab,char delimFin)
{
	char RxBt;
	
	int i=0;

	//On remplit le tableau avec les caractères qui suivent...
	do
	{
		//Attente et réception d'un caractère
		
		Driver_USART1.Receive(&RxBt,1);
		while(Driver_USART1.GetRxCount()<1);


		//Si on vient de recevoir de délimiteur de FIN  ...
		if(RxBt == delimFin)
		{
			ptab[i] = 'NULL';		//... on place le caractère NULL dans le tableau => on "cloture" la chaine de caractères...
		}
			            
		else
			{
			ptab[i]=RxBt;		//... sinon on copie le caractère reçu dans le tableau
		  }
			           
		i++;//on fait évoluer l'index du tableau
	}
	while(RxBt!=delimFin);//... Tant que le caractère reçu n'est pas le délimiteur de FIN
}
int main (void) 
{		
		char  qualite,  angle1,  angle2,  distance1 ,  distance2, trame[5], a, tab[1], mode = 0;// Chaine de caractères (53 max en petite police / 20 max en grande police)
		int somme, etat=0, k;
		long int i;
		short angle, distance;
	
		osKernelInitialize ();  
		
		Init_UART();
	  Initialise_GPIO(); // init GPIO
		PWM_Init();
		//LidarInit();
		GLCD_Init();						// Initialisation LCD
		GLCD_Clear(White);			// Fond d'ecran en blanc
		
		LPC_GPIO2->FIODIR1=0x01;
		
		//LPC_UART2->THR = 0xA5; // Envoi de la donnée (valeur 0xA5)
			
		//LPC_UART2->THR = 0x20; // Envoi de la donnée (valeur 0x20)
		
		LPC_GPIO2->FIODIR0|=0x04;
		
		/*Driver_USART1.Receive(tab,1);
		while(Driver_USART1.GetRxCount()<1);*/
		
		if(tab[0] == 0) 
		{
			mode = 0; //mode auto
			tid_automatique = osThreadCreate (osThread(automatique), NULL);
		}
		else if(tab[0] == 1) 
		{
			mode = 1; //mode gant
			tid_gant = osThreadCreate (osThread(gant), NULL);
		}
		else if(tab[0] == 2)
		{
			mode = 2; //mode nunchuck
			tid_joystick = osThreadCreate (osThread(joystick), NULL);
		}
		
		osKernelStart ();                         // start thread execution 
		osDelay(osWaitForever);
}

void automatique (void const *argument)
{
		int etat = 0;
	
		while(1)
		{
			UART_PutChar(0xA5);
			
			switch (etat)
		{
         case 0: 
					 PWM_Droite(70);
					 PWM_Gauche(70);
					 affichage(0);
					 osDelay(1600);
					 etat = 1;
					 
					 break;
				 
				 case 1: 
					 PWM_Droite(70);
					 PWM_Gauche(50);
				   affichage(1);
					 osDelay(700);
				   etat = 2;
					 
					 break;
				 
				 case 2: 
					 PWM_Droite(65);
					 PWM_Gauche(65);
				   affichage(2);
					 osDelay(3000);
				   etat = 3;
					 
				   break;
					 
				case 3: 
					 PWM_Droite(70);
					 PWM_Gauche(50);
				   affichage(3);
					 osDelay(600);
				   etat = 0;
				 
					 break;	 
    }
		}
}
void gant (void const *argument)
{
	osDelay(osWaitForever);
}
void joystick (void const *argument)
{
	osDelay(osWaitForever);
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


void affichage(int a)
{
	char Chaine_char[53] = "";
	
	sprintf(Chaine_char, "Etat = %d", a);	// creation chaine texte
	GLCD_DisplayString(1,0,1,Chaine_char);		// affichage chaine ligne 0, colonne 0, petite police
}
