// Utilisation Event UART en emission-reception

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions

#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "GLCD_Config.h"                // Keil.MCB1700::Board Support:Graphic LCD
#include "LPC17xx.h"                    // Device header
#include "cmsis_os.h"                   // CMSIS RTOS header file
#include "stdio.h"
#include "GPIO.h"
#include "string.h"
#include "stdlib.h"

extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;
extern ARM_DRIVER_USART Driver_USART1;
																												//*******************PROTOTYPES**********************
void Thread_T (void const *argument);                             // thread function Transmit
void Thread_R (void const *argument);                             // thread function Receive
void automatique (void const *argument);                          // thread function
void gant (void const *argument);                             
void joystick (void const *argument);                             
void reception (void const *argument);

void Init_UART(void);																							// prototypes fonctions																			
void sendCommand(char * command, int tempo_ms);
void Init_WiFi(void);
void PWM_Init();
void PWM_Droite(int rapport_cyclique);
void PWM_Gauche(int rapport_cyclique);                                    
																												 //*******************ID**********************
osThreadId tid_Thread_T,tid_Thread_R,tid_automatique, tid_gant, tid_joystick, tid_reception;   

osMutexId ID_mut_MODE;
																												 //*******************DEFINITIONS**********************
osThreadDef (Thread_R, osPriorityNormal, 1, 0);                   // thread object
osThreadDef (Thread_T, osPriorityNormal, 1, 0); 
osThreadDef (automatique, osPriorityNormal, 1, 0);                   
osThreadDef (gant, osPriorityNormal, 1, 0);                   
osThreadDef (joystick, osPriorityNormal, 1, 0);                   	
osThreadDef (reception, osPriorityNormal, 1, 0);

osMutexDef(mut_MODE);

char tab[1], mode = 0, JoystickX[3],JoystickY[3];
	
//fonction de CB lancee si Event T ou R
void event_UART(uint32_t event)
{
	switch (event) {
		
		case ARM_USART_EVENT_RECEIVE_COMPLETE : 	osSignalSet(tid_Thread_R, 0x01);
																							break;
		
		case ARM_USART_EVENT_SEND_COMPLETE  : 	osSignalSet(tid_Thread_T, 0x02);
																							break;
		
		default : break;
	}
}
																													//***********************INITIALISATIONS***********************
int main (void){

	char  qualite,  angle1,  angle2,  distance1 ,  distance2, trame[5], a, RxBt;// Chaine de caractères (53 max en petite police / 20 max en grande police)
	int somme, etat=0, k;
	long int i;
	short angle, distance;
	
	osKernelInitialize ();                    // initialize CMSIS-RTOS
	
	// initialize peripherals here
	Init_UART();
	Initialise_GPIO(); // init GPIO
	GLCD_Initialize()
	;
	PWM_Init();
	GLCD_ClearScreen();
	GLCD_SetFont(&GLCD_Font_6x8);
	NVIC_SetPriority(UART1_IRQn,2);
	
	LPC_GPIO2->FIODIR1=0x01;
	LPC_GPIO2->FIODIR0|=0x04;

//	Driver_USART1.Receive(tab,1);
//	while(Driver_USART1.GetRxCount()<1);
	
	
	
	ID_mut_MODE = osMutexCreate(osMutex(mut_MODE));
		
		
	//Creation des 2 taches
	tid_Thread_T = osThreadCreate(osThread(Thread_T),NULL);
	tid_Thread_R = osThreadCreate(osThread(Thread_R),NULL);
	tid_automatique = osThreadCreate (osThread(automatique), NULL);
	tid_gant = osThreadCreate (osThread(gant), NULL);
	tid_joystick = osThreadCreate (osThread(joystick), NULL);
	tid_reception = osThreadCreate (osThread(reception), NULL);
	
	osKernelStart ();                         // start thread execution 
	osDelay(osWaitForever);
	
	return 0;
}

void Thread_T (void const *argument) {
	
	char Cmd[30];
	char ReqHTTP[90];
	osMutexWait(ID_mut_MODE, osWaitForever);
	Init_WiFi();
	osMutexRelease(ID_mut_MODE);
	while(1){
  //sendCommand("AT+CIPSEND=4\r\n",7000); 
	//sendCommand(">448",7000);
	}
	
	
}

// Tache de réception et d'affichage

void Thread_R (void const *argument) {

	char RxChar;
	int ligne;
	int i=0,j=0;	// i pour position colonne caractère
	char RxBuf[200];
	
  while (1) {
		Driver_USART1.Receive(&RxChar,1);		// A mettre ds boucle pour recevoir 
		osSignalWait(0x01, osWaitForever);	// sommeil attente reception
		
		RxBuf[i]=RxChar;
		i++;
		//Suivant le caractère récupéré
		switch(RxChar)
		{
			case 0x0D: 		//Un retour chariot? On ne le conserve pas...
				i--;
				break;
			case 0x0A:										//Un saut de ligne?
				RxBuf[i-1]=0;											//=> Fin de ligne, donc, on "cloture" la chaine de caractères
				GLCD_DrawString(1,ligne,RxBuf);	//On l'affiche (peut etre trop long, donc perte des caractères suivants??)
				ligne+=10;								//On "saute" une ligne de l'afficheur LCD
			  
			if(RxBuf[9] == 'N')              //*****Réception des données du nunchuck*****
			{
				mode = 3;
				osSignalSet(tid_joystick,0x0004);
				for(j=0;j<3;j++)
				{
					JoystickX[j] = RxBuf[10+j];
					if(JoystickX[j] == ' ') JoystickX[j] = 0;
				}
				for(j=0;j<3;j++)
				{
					JoystickY[j] = RxBuf[13+j];
					if(JoystickY[j] == ' ') JoystickY[j] = 0;
				}
			}
			
		 if(RxBuf[9] == 'G')              //*****Réception des données du gant*****
		 {
		 }
		 
		 if(RxBuf[9] == 'C')              //*****Réception des données des capteurs*****
		 {
		 }
		 
		 if(RxBuf[9] == 'M')              //*****Réception du mode*****
		 {
					if(RxBuf[1] == '1') 
				{
					mode = 1; //mode auto
					GLCD_DrawString(1,0,"***Mode Automatique***                           ");
				}
				else if(RxBuf[1] == '2') 
				{
					mode = 2; //mode gant
					//GLCD_DrawString(1,0,"***Mode Force***                           ");
				}
				else if(RxBuf[1] == '3')
				{
					mode = 3; //mode nunchuck
					//GLCD_DrawString(1,0,"***Mode Wii***                           ");
				}
	   }		
			
			if(ligne>240)
				{
					ligne=1;
					GLCD_ClearScreen();
					osDelay(2000);
				}
				i=0;													//On se remet au début du buffer de réception pour la prochaine ligne à recevoir
				break;
		}
  }
}
void automatique (void const *argument)
{
	int etat = 0;
	
		while(1)
		{
			if(mode != 1) 
			{
				osSignalWait(0x0001,osWaitForever);
				PWM_Droite(0);
				PWM_Gauche(0);
			}
			//GLCD_DisplayString(5,0,0,"               o                   ");
//			UART_PutChar(0xA5);
			 
			switch (etat)
		{	 
				case 0:
					
					 PWM_Droite(70); //gauche
					 PWM_Gauche(0);
				   osDelay(450);
					
					 PWM_Droite(0);
					 PWM_Gauche(0);
					 osDelay(400);
				 
					 PWM_Droite(65); //tout droit
					 PWM_Gauche(65);
					 osDelay(150);
				 
					 PWM_Droite(0); 
					 PWM_Gauche(0);
					 osDelay(400);
				 
					 PWM_Droite(0); //droite
					 PWM_Gauche(70);
				   osDelay(400);
					
					 PWM_Droite(0);
					 PWM_Gauche(0);
					 osDelay(400);
				 
					 PWM_Droite(60); //tout droit
					 PWM_Gauche(60);
					 osDelay(150);
				 
					 PWM_Droite(0); 
					 PWM_Gauche(0);
					 osDelay(400);
				 
				   etat = 1;
				
					 break;
				 
				 case 1: //arret total
					 PWM_Droite(0);
					 PWM_Gauche(0);
				   //affichage(1);
//					 osDelay(700);
//				   etat = 2;
					 
					 break;
				 
				 case 2: //cas tout droit
					 PWM_Droite(65);
					 PWM_Gauche(65);
					 //affichage(0);
					 osDelay(450);
					 etat = 1;
					 
					 break;
				 
				 case 3: //cas place libre a gauche
					 PWM_Droite(70); //gauche
					 PWM_Gauche(0);
				   osDelay(450);
					
					 PWM_Droite(0);
					 PWM_Gauche(0);
					 osDelay(400);
				 
					 PWM_Droite(65); //tout droit
					 PWM_Gauche(65);
					 osDelay(150);
				 
					 PWM_Droite(0); 
					 PWM_Gauche(0);
					 osDelay(400);
				 
				   PWM_Droite(0); //droite
					 PWM_Gauche(70);
					 osDelay(350);
				 
					 PWM_Droite(0); 
					 PWM_Gauche(0);
					 osDelay(400);
				 
					 PWM_Droite(65); //tout droit
					 PWM_Gauche(65);
					 osDelay(200);
				 
				   etat = 1;
				 
				   break;
			 }
	}
}
void gant (void const *argument)
{
//char v_moy,delta,v_moteurD,v_moteurG,texte[50];
//	
//	while(1)
//	{
//	if(mode != 2) 
//	{
//		osSignalWait(0x0002,osWaitForever);
//		PWM_Droite(0);
//	  PWM_Gauche(0);
//	}
//	
//	PWM_Droite(0);
//	PWM_Gauche(0);
//	
//	osMutexWait(ID_mut_MODE, osWaitForever);
//	reception_gant(v_moy,delta);
//	osMutexRelease(ID_mut_MODE);
//	
//	v_moteurD = v_moy - delta;
//	v_moteurG = v_moy + delta;
//	
//	sprintf(texte,"MoteurD = %c      MoteurG = %c",v_moteurD,v_moteurG);
//	GLCD_DrawString(5,0,texte);
//	}
}	
void joystick (void const *argument)
{
//	char 
	int X, Y;
	char texte[20],texte2[20];
  int valeurX,valeurY;
	
	while(1)
	{
	if(mode != 3) 
	{
		osSignalWait(0x0004,osWaitForever);
		PWM_Droite(0);
	  PWM_Gauche(0);
	}
	
	
	X = 100*(JoystickX[0]-0x30)+10*(JoystickX[1]-0x30)+(JoystickX[2]-0x30);
	Y = (100*(JoystickY[0]-0x30)+10*(JoystickY[1]-0x30)+(JoystickY[2]-0x30))+50;
	
//	valeurX = (int)X;
//	valeurY = (int)Y;
	
//	sprintf(texte,"%3d",X);
//	sprintf(texte2,"%3d",Y);

	
	if(X>0)GLCD_DrawString(300,400,texte);
	if(X>0)GLCD_DrawString(300,350,texte2);
	
	
	if((X<135)&&(X > 120)&&(Y > 245))
	{
//		if(X>0)GLCD_DrawString(250,300,"TOUT DROIT");
		PWM_Droite(70);
	  PWM_Gauche(70);
	}
	if((X < 10)&&(Y > 125)&&(Y < 140))
	{
//		if(X>0)GLCD_DrawString(250,250,"GAUCHE");
		PWM_Droite(70);
	  PWM_Gauche(0);
	}
  if((X > 245)&&(Y > 125)&&(Y < 140))
	{
//		if(X>0)GLCD_DrawString(250,200,"DROITE");
		PWM_Droite(0);
	  PWM_Gauche(70);
	}
	if((X > 120)&&(X < 135)&&(Y > 125)&&(Y < 140))
	{
//		if(X>0)GLCD_DrawString(250,150,"ARRET");
		PWM_Droite(0);
	  PWM_Gauche(0);
	}

	
	}
}	
void reception (void const *argument)
{
	while(1);
}
void Init_UART(void){
	Driver_USART1.Initialize(event_UART);
	Driver_USART1.PowerControl(ARM_POWER_FULL);
	Driver_USART1.Control(	ARM_USART_MODE_ASYNCHRONOUS |
							ARM_USART_FLOW_CONTROL_NONE   |
							ARM_USART_DATA_BITS_8		|
							ARM_USART_STOP_BITS_1		|
							ARM_USART_PARITY_NONE		,							
							115200);
	Driver_USART1.Control(ARM_USART_CONTROL_TX,1);
	Driver_USART1.Control(ARM_USART_CONTROL_RX,1);
}
void PWM_Init()
{
	LPC_PINCON->PINSEL4 |= 0x00000050;
	LPC_SC->PCONP = LPC_SC->PCONP | 0x00000040;	// enable PWM1
		
	LPC_PWM1->PR = 11;  // prescaler
	LPC_PWM1->MR0 = 99;
	
	LPC_PWM1->MCR = LPC_PWM1->MCR | 0x00000002; // Timer relancé quand MR0 repasse à 0
	LPC_PWM1->LER = LPC_PWM1->LER | 0x0000003F;  // ceci donne le droit de modifier dynamiquement la valeur du rapport cyclique
	                                             // bit 0 = MR0    bit3 = MR3
	LPC_PWM1->PCR = LPC_PWM1->PCR | (7<<11);  // autorise les sorties PWM1.1 et PWM1.2  
		
	LPC_PWM1->MR3 = 0; //remise à 0 des compteurs
	LPC_PWM1->MR4 = 0;
	LPC_PWM1->MR5 = 39;
		
		LPC_PWM1->TCR = 1;  /*validation de timer 1 et reset counter */
}
void PWM_Droite(int rapport_cyclique)
{
	LPC_PWM1->MR3 = rapport_cyclique;    // ceci ajuste la duree de l'état haut pour le moteur droit
}
void PWM_Gauche(int rapport_cyclique)
{
	LPC_PWM1->MR4 = rapport_cyclique;    // ceci ajuste la duree de l'état haut pour le moteur gauche
}
void sendCommand(char * command, int tempo_ms)
{
	int len;
	len = strlen (command);
	Driver_USART1.Send(command,len); // send the read character to the esp8266
	osSignalWait(0x02, osWaitForever);		// sommeil fin emission
	osDelay(tempo_ms);		// attente traitement retour
}

void Init_WiFi(void)
{
	// reset module
	sendCommand("AT+RST\r\n",7000); 
	
	// disconnect from any Access Point
	sendCommand("AT+CWQAP\r\n",2000); 
	
	sendCommand("AT+CWMODE=3\r\n",2000);
	
  // configure as Station 
	sendCommand("AT+CWJAP=\"IT2R1\",\"It2r2018\"\r\n",7000);
	
	sendCommand("AT+CIPMUX=1\r\n",2000);
	
	sendCommand("AT+CIPSERVER=1\r\n",2000);
	
	sendCommand("AT+CIFSR\r\n",2000);
	
	//Connect to YOUR Access Point
	//sendCommand("AT+CIPSTART=\"TCP\",\"192.168.0.100\",333\r\n",7000); 
			
	
}



