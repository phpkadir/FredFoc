/******************************************************************************/
/*                                                                            */
/*                                 TLE5012.h                                  */
/*                     Dreamer Rongfei.Deng 2015.9.24                         */
/*                                                                            */
/******************************************************************************/

#ifndef __TLE5012_H
#define __TLE5012_H
/*============================================================================*/
/*                               Header include                               */
/*============================================================================*/
#include <rtthread.h>

/*============================================================================*/
/*                              Macro Definition                              */
/*============================================================================*/


/*============================================================================*/
/*                            Structure definition                            */
/*============================================================================*/
#pragma pack(push)
#pragma pack(4)

// here Structure definition...


#pragma pack(pop)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */									
/*============================================================================*/
/*                              Global variables                              */
/*============================================================================*/



/*============================================================================*/
/*                             Function definition                            */
/*============================================================================*/
void TLE5012_init(void);
rt_int16_t TLE5012_Read(void);
//void TLE5012_Aligned(rt_uint16_t* Encoder,bool *Align_state);
void TLE5012_ResetEncoder(void);
rt_int16_t TLE5012_Aligned(void);
rt_int16_t TLE5012_Get_Mechanical_Speed(void);
rt_int16_t TLE5012_Postion(void);
rt_int16_t SensorGetAngle(rt_uint8_t bMotorNbr,rt_bool_t *Error_Flag);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

