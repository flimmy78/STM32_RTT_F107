#ifndef __FLASH_IF_H__
#define __FLASH_IF_H__
/*****************************************************************************/
/* File      : flash_if.h                                                    */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-03-10 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include "stm32f10x.h"

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/

//擦除APP2所在的区域
uint32_t FLASH_If_Erase_APP2(void);
//将文件拷贝到APP2区域
uint32_t FLASH_IF_FILE_COPY_TO_APP2(char * updateFileName);

#endif /*__FLASH_IF_H__*/
