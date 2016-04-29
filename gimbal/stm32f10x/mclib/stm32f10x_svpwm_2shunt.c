/******************************************************************************/
/*                                                                            */
/*                         stm32f10x_svpwm_2shunt.c                           */
/*                     Dreamer Rongfei.Deng 2015.9.24                         */
/*                                                                            */
/******************************************************************************/


/*============================================================================*/
/*                               Header include                               */
/*============================================================================*/
#include "stm32f10x.h"
#include "stm32f10x_svpwm_3shunt.h"
#include "MC_Globals.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define NB_CONVERSIONS 16

#define SQRT_3		1.732051
#define T		    (PWM_PERIOD * 4)
#define T_SQRT3     (rt_uint16_t)(T * SQRT_3)

#define SECTOR_1	(rt_uint32_t)1
#define SECTOR_2	(rt_uint32_t)2
#define SECTOR_3	(rt_uint32_t)3
#define SECTOR_4	(rt_uint32_t)4
#define SECTOR_5	(rt_uint32_t)5
#define SECTOR_6	(rt_uint32_t)6

#define PHASE_A_ADC_CHANNEL     ADC_Channel_0
#define PHASE_B_ADC_CHANNEL     ADC_Channel_1
#define PHASE_C_ADC_CHANNEL     ADC_Channel_2


// Settings for current sampling only
#define PHASE_A_MSK       (rt_uint32_t)((rt_uint32_t)(PHASE_A_ADC_CHANNEL) << 15)
#define PHASE_B_MSK       (rt_uint32_t)((rt_uint32_t)(PHASE_B_ADC_CHANNEL) << 15)
#define PHASE_C_MSK       (rt_uint32_t)((rt_uint32_t)(PHASE_C_ADC_CHANNEL) << 15)


// Settings for current sampling only
#define TEMP_FDBK_MSK     (rt_uint32_t)(0)
#define BUS_VOLT_FDBK_MSK (rt_uint32_t)(0)


// Settings for current sampling only
#define SEQUENCE_LENGHT    0x00000000

#define ADC_PRE_EMPTION_PRIORITY 1
#define ADC_SUB_PRIORITY 0

#define BRK_PRE_EMPTION_PRIORITY 0
#define BRK_SUB_PRIORITY 0

#define TIM1_UP_PRE_EMPTION_PRIORITY 1
#define TIM1_UP_SUB_PRIORITY 0

#define LOW_SIDE_POLARITY  TIM_OCIdleState_Reset

#define PWM2_MODE 0
#define PWM1_MODE 1

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
rt_uint8_t  bSector;  

rt_uint16_t hPhaseAOffset;
rt_uint16_t hPhaseBOffset;
//rt_uint16_t hPhaseCOffset;



rt_uint8_t PWM4Direction=PWM2_MODE;

/* Private function prototypes -----------------------------------------------*/
void SVPWM_InjectedConvConfig(void);

/*******************************************************************************
* Function Name  : SVPWM_3ShuntInit
* Description    : It initializes PWM and ADC peripherals
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWM_3ShuntInit(void)
{ 
  ADC_InitTypeDef ADC_InitStructure;
  TIM_TimeBaseInitTypeDef TIM1_TimeBaseStructure;
  TIM_OCInitTypeDef TIM1_OCInitStructure;
  TIM_BDTRInitTypeDef TIM1_BDTRInitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* ADC1, ADC2, DMA, GPIO, TIM1 clocks enabling -----------------------------*/
  
  /* ADCCLK = PCLK2/6 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);

  /* Enable DMA clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  
  /* Enable GPIOA, GPIOB, AFIO clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
  /* Enable ADC1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

  /* Enable ADC2 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE); 
   
  /* Enable TIM1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
     
  /* ADC1, ADC2, PWM pins configurations -------------------------------------*/
     
  GPIO_StructInit(&GPIO_InitStructure);
  /****** Configure PA.00 01 02 (ADC Channels [3]) as analog input ******/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* TIM1 Peripheral Configuration -------------------------------------------*/
  /* TIM1 Registers reset */
  TIM_DeInit(TIM1);
  TIM_TimeBaseStructInit(&TIM1_TimeBaseStructure);
  /* Time Base configuration */
  TIM1_TimeBaseStructure.TIM_Prescaler = 0x0;
  TIM1_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
  TIM1_TimeBaseStructure.TIM_Period = PWM_PERIOD;
  TIM1_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV2;
  
  // Initial condition is REP=1 to set the UPDATE only on the underflow
  TIM1_TimeBaseStructure.TIM_RepetitionCounter = REP_RATE;
  TIM_TimeBaseInit(TIM1, &TIM1_TimeBaseStructure);
  
  TIM_OCStructInit(&TIM1_OCInitStructure);
  /* Channel 1, 2,3 in PWM mode */
  TIM1_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 
  TIM1_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 
  TIM1_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;                  
  TIM1_OCInitStructure.TIM_Pulse = 0x505; //dummy value
  TIM1_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 
  TIM1_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;         
  TIM1_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  TIM1_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;          
  
  TIM_OC1Init(TIM1, &TIM1_OCInitStructure); 
  TIM_OC2Init(TIM1, &TIM1_OCInitStructure);
  TIM_OC3Init(TIM1, &TIM1_OCInitStructure);
  
  GPIO_StructInit(&GPIO_InitStructure);
  /* GPIOA Configuration: Channel 1, 2, 3 Output */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure); 
  
  /* GPIOB Configuration: Channel 1N, 2N, 3N and 4 Output */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure); 
	
  /* Lock GPIOA Pin9 and Pin11 Pin 13 (High sides) */
  GPIO_PinLockConfig(GPIOA, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10);

//  GPIO_StructInit(&GPIO_InitStructure);
//  /* GPIOE Configuration: BKIN pin */   
//  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
//  GPIO_Init(GPIOE, &GPIO_InitStructure);  
  
  TIM_OCStructInit(&TIM1_OCInitStructure);
  /* Channel 4 Configuration in OC */
  TIM1_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;  
//  TIM1_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM1_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 
  TIM1_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;                  
  //TIM1_OCInitStructure.TIM_Pulse = PWM_PERIOD - 120; 
	TIM1_OCInitStructure.TIM_Pulse = PWM_PERIOD - PWM_PERIOD * 1/100; 
  
  TIM1_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 
  TIM1_OCInitStructure.TIM_OCNPolarity =TIM_OCNPolarity_Low;         
  TIM1_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  TIM1_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;            
  
  TIM_OC4Init(TIM1, &TIM1_OCInitStructure);
  
  /* Enables the TIM1 Preload on CC1 Register */
  TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
  /* Enables the TIM1 Preload on CC2 Register */
  TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
  /* Enables the TIM1 Preload on CC3 Register */
  TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
  /* Enables the TIM1 Preload on CC4 Register */
  TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);

  /* Automatic Output enable, Break, dead time and lock configuration*/
  TIM1_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
  TIM1_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
  TIM1_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1; 
  TIM1_BDTRInitStructure.TIM_DeadTime = DEADTIME;
  TIM1_BDTRInitStructure.TIM_Break = TIM_Break_Disable;
  TIM1_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;
  TIM1_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;

  TIM_BDTRConfig(TIM1, &TIM1_BDTRInitStructure);

  TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);
  
  TIM_ClearITPendingBit(TIM1, TIM_IT_Break);
  TIM_ITConfig(TIM1, TIM_IT_Break,DISABLE);
  
  /* TIM1 counter enable */
  TIM_Cmd(TIM1, ENABLE);
  
  // Resynch to have the Update evend during Undeflow
  TIM_GenerateEvent(TIM1, TIM_EventSource_Update);
  
  // Clear Update Flag
  TIM_ClearFlag(TIM1, TIM_FLAG_Update);
  
  TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);
  
  TIM_ITConfig(TIM1, TIM_IT_CC4,DISABLE);
     
  /* ADC1 registers reset ----------------------------------------------------*/
  ADC_DeInit(ADC1);
  /* ADC2 registers reset ----------------------------------------------------*/
  ADC_DeInit(ADC2);
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
  /* Enable ADC2 */
  ADC_Cmd(ADC2, ENABLE);
  
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_Mode = ADC_Mode_InjecSimult;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);
   
  /* ADC2 Configuration ------------------------------------------------------*/
  ADC_StructInit(&ADC_InitStructure);  
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC2, &ADC_InitStructure);
  
  // Start calibration of ADC1
  ADC_StartCalibration(ADC1);
  // Start calibration of ADC2
  ADC_StartCalibration(ADC2);
  
  // Wait for the end of ADCs calibration 
  while (ADC_GetCalibrationStatus(ADC1) & ADC_GetCalibrationStatus(ADC2))
  {
  }
  
  SVPWM_3ShuntCurrentReadingCalibration();
  
  //NVIC_StructInit(&NVIC_InitStructure);
  /* Enable the ADC Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ADC_PRE_EMPTION_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = ADC_SUB_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable the Update Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIM1_UP_PRE_EMPTION_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = TIM1_UP_SUB_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
    
  /* Enable the TIM1 BRK Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BRK_PRE_EMPTION_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = BRK_SUB_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 
} 


/*******************************************************************************
* Function Name  : SVPWM_3ShuntCurrentReadingCalibration
* Description    : Store zero current converted values for current reading 
                   network offset compensation in case of 3 shunt resistors 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void SVPWM_3ShuntCurrentReadingCalibration(void)
{
  static rt_uint16_t bIndex;
  
  /* ADC1 Injected group of conversions end interrupt disabling */
  ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);
  
  hPhaseAOffset=0;
  hPhaseBOffset=0;
//  hPhaseCOffset=0;
  
  /* ADC1 Injected conversions trigger is given by software and enabled */ 
  ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);  
  ADC_ExternalTrigInjectedConvCmd(ADC1,ENABLE); 
  
  /* ADC1 Injected conversions configuration */ 
  ADC_InjectedSequencerLengthConfig(ADC1,2);
  ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,SAMPLING_TIME_CK);
  ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,2,SAMPLING_TIME_CK);
//  ADC_InjectedChannelConfig(ADC1, PHASE_C_ADC_CHANNEL,3,SAMPLING_TIME_CK);
  
  /* Clear the ADC1 JEOC pending flag */
  ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);  
  ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
   
  /* ADC Channel used for current reading are read 
     in order to get zero currents ADC values*/ 
  for(bIndex=0; bIndex <NB_CONVERSIONS; bIndex++)
  {
    while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_JEOC)) { }
    
    hPhaseAOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_1)>>3);
    hPhaseBOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_2)>>3);
//    hPhaseCOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_3)>>3);    
        
    /* Clear the ADC1 JEOC pending flag */
    ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);    
    ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
  }
  
  SVPWM_InjectedConvConfig();  
}



/*******************************************************************************
* Function Name  : SVPWM_InjectedConvConfig
* Description    : This function configure ADC1 for 3 shunt current 
*                  reading and temperature and voltage feedbcak after a 
*                  calibration of the three utilized ADC Channels
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWM_InjectedConvConfig(void)
{
  ADC_InjectedSequencerLengthConfig(ADC1,1);
  
  ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL, 1, SAMPLING_TIME_CK);
	
	ADC_InjectedSequencerLengthConfig(ADC2,1);
  
  ADC_InjectedChannelConfig(ADC2, PHASE_B_ADC_CHANNEL, 1, SAMPLING_TIME_CK);

    
  /* ADC1 Injected conversions trigger is TIM1 TRGO */ 
  ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO); 
  
  ADC_ExternalTrigInjectedConvCmd(ADC2,ENABLE);
  
  /* ADC1 Injected group of conversions end and Analog Watchdog interrupts enabling */
	ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
}

/*******************************************************************************
* Function Name  : SVPWM_3ShuntGetPhaseCurrentValues
* Description    : This function computes current values of Phase A and Phase B 
*                 in q1.15 format starting from values acquired from the A/D 
*                 Converter peripheral.
* Input          : None
* Output         : Stat_Curr_a_b
* Return         : None
*******************************************************************************/
Curr_Components SVPWM_3ShuntGetPhaseCurrentValues(void)
{
  Curr_Components Local_Stator_Currents;
  rt_int32_t wAux;

 // Ia = (hPhaseAOffset)-(ADC Channel 11 value)    
//	wAux = (rt_int32_t)(hPhaseAOffset)- ((ADC1->JDR1)<<1);  
	wAux = ((ADC1->JDR1)<<1) - (rt_int32_t)(hPhaseAOffset) ;	
	//wAux = ( ((ADC1->JDR1)<<1)&0xFFFFFFCF ) -  (rt_int32_t)(hPhaseAOffset) ;	
 //Saturation of Ia 
	if (wAux < S16_MIN)
	{
		Local_Stator_Currents.qI_Component1= S16_MIN;
	}  
	else  if (wAux > S16_MAX)
	{ 
		Local_Stator_Currents.qI_Component1= S16_MAX;
	}
	else
	{
		Local_Stator_Currents.qI_Component1= wAux;
	}
					 
 // Ib = (hPhaseBOffset)-(ADC Channel 12 value)
//	wAux = (rt_int32_t)(hPhaseBOffset)-((ADC2->JDR1)<<1);
	wAux = ((ADC2->JDR1)<<1) - (rt_int32_t)(hPhaseBOffset);
	//wAux = ( ((ADC2->JDR1)<<1)&0xFFFFFFCF ) -  (rt_int32_t)(hPhaseBOffset) ;	
 // Saturation of Ib
	if (wAux < S16_MIN)
	{
		Local_Stator_Currents.qI_Component2= S16_MIN;
	}  
	else  if (wAux > S16_MAX)
	{ 
		Local_Stator_Currents.qI_Component2= S16_MAX;
	}
	else
	{
		Local_Stator_Currents.qI_Component2= wAux;
	}
  
  return(Local_Stator_Currents); 
}

/*******************************************************************************
* Function Name  : SVPWM_3ShuntCalcDutyCycles
* Description    : Computes duty cycle values corresponding to the input value
		   and configures the AD converter and TIM0 for next period 
		   current reading conversion synchronization
* Input          : Stat_Volt_alfa_beta
* Output         : None
* Return         : None
*******************************************************************************/

void SVPWM_3ShuntCalcDutyCycles (Volt_Components Stat_Volt_Input)
{
   rt_int32_t wX, wY, wZ, wUAlpha, wUBeta;
   rt_uint16_t  hTimePhA=0, hTimePhB=0, hTimePhC=0;
    
   wUAlpha = Stat_Volt_Input.qV_Component1 * T_SQRT3 ;
   wUBeta = -(Stat_Volt_Input.qV_Component2 * T);

   wX = wUBeta;
   wY = (wUBeta + wUAlpha)/2;
   wZ = (wUBeta - wUAlpha)/2;
   
  // Sector calculation from wX, wY, wZ
   if (wY<0)
   {
      if (wZ<0)
      {
        bSector = SECTOR_5;
      }
      else // wZ >= 0
        if (wX<=0)
        {
          bSector = SECTOR_4;
        }
        else // wX > 0
        {
          bSector = SECTOR_3;
        }
   }
   else // wY > 0
   {
     if (wZ>=0)
     {
       bSector = SECTOR_2;
     }
     else // wZ < 0
       if (wX<=0)
       {  
         bSector = SECTOR_6;
       }
       else // wX > 0
       {
         bSector = SECTOR_1;
       }
    }
   
   /* Duty cycles computation */
  PWM4Direction=PWM2_MODE;
    
  switch(bSector)
  {  
    case SECTOR_1:
        hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhB - wX/131072;
        break;
    case SECTOR_2:
        hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhA - wY/131072;
        break;
    case SECTOR_3:
        hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
				hTimePhC = hTimePhA - wY/131072;
				hTimePhB = hTimePhC + wX/131072;
        break;
    
    case SECTOR_4:
				hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhB - wX/131072;
        break;  
    case SECTOR_5:
        hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhA - wY/131072;
				break;        
    case SECTOR_6:
        hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
				hTimePhC = hTimePhA - wY/131072;
				hTimePhB = hTimePhC + wX/131072;
        break;
    default:
		break;
   }
  
  /* Load compare registers values */ 
  TIM1->CCR1 = hTimePhA;
  TIM1->CCR2 = hTimePhB;
  TIM1->CCR3 = hTimePhC;
}

/*******************************************************************************
* Function Name  : SVPWM_3ShuntAdvCurrentReading
* Description    :  It is used to enable or disable the advanced current reading.
			if disabled the current readign will be performed after update event
* Input          : cmd (ENABLE or DISABLE)
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWM_3ShuntAdvCurrentReading(FunctionalState cmd)
{
  if (cmd == ENABLE)
  {
    // Enable ADC trigger sync with CC4
    ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_CC4);  
//    ADC1->CR2 |= 0x00001000;
    
    // Enable UPDATE ISR
    // Clear Update Flag
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
  }
  else
  {
    // Disable UPDATE ISR
    TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);

    // Sync ADC trigger with Update
    ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO);
//    ADC1->CR2 &=0xFFFFEFFF;
    
    // ReEnable EXT. ADC Triggering
//    ADC1->CR2 |=0x00008000;    
  }
}

/*******************************************************************************
* Function Name  : SVPWMUpdateEvent
* Description    :  Routine to be performed inside the update event ISR  it reenable the ext adc. triggering
		        It must be assigned to pSVPWM_UpdateEvent pointer.	
* Input           : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWMUpdateEvent(void)
{
  // ReEnable EXT. ADC Triggering
  ADC1->CR2 |= 0x00008000;
  
  // Clear unwanted current sampling
  ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
}

/*******************************************************************************
* Function Name  : SVPWMEOCEvent
* Description    :  Routine to be performed inside the end of conversion ISR
		         It computes the bus voltage and temperature sensor sampling 
		        and disable the ext. adc triggering.	
* Input           : None
* Output         : None
* Return         : None
*******************************************************************************/
rt_uint8_t SVPWMEOCEvent(void)
{
  // Store the Bus Voltage and temperature sampled values
//  h_ADCTemp = ADC_GetInjectedConversionValue(ADC2,ADC_InjectedChannel_2);
//  h_ADCBusvolt = ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_2);
    
  if ((State == START) || (State == RUN))
  {          
    // Disable EXT. ADC Triggering
    ADC1->CR2 = ADC1->CR2 & 0xFFFF7FFF;
  }
  return ((rt_uint8_t)(1));
}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/  
