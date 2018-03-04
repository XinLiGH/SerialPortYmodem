/**
  ******************************************************************************
  * @file    Ymodem.h
  * @author  XinLi
  * @version v1.0
  * @date    21-January-2018
  * @brief   Header file for Ymodem.cpp module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>Copyright &copy; 2018 XinLi</center></h2>
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <https://www.gnu.org/licenses/>.
  *
  ******************************************************************************
  */

#ifndef __YMODEM_H
#define __YMODEM_H

/* Header includes -----------------------------------------------------------*/
#include <stdint.h>

/* Macro definitions ---------------------------------------------------------*/
#define YMODEM_PACKET_HEADER    (3)
#define YMODEM_PACKET_TRAILER   (2)
#define YMODEM_PACKET_OVERHEAD  (YMODEM_PACKET_HEADER + YMODEM_PACKET_TRAILER)
#define YMODEM_PACKET_SIZE      (128)
#define YMODEM_PACKET_1K_SIZE   (1024)

/* Type definitions ----------------------------------------------------------*/
enum YmodemCode
{
  YmodemCodeNone = 0x00,
  YmodemCodeSoh  = 0x01,
  YmodemCodeStx  = 0x02,
  YmodemCodeEot  = 0x04,
  YmodemCodeAck  = 0x06,
  YmodemCodeNak  = 0x15,
  YmodemCodeCan  = 0x18,
  YmodemCodeC    = 0x43,
  YmodemCodeA1   = 0x41,
  YmodemCodeA2   = 0x61
};

enum YmodemStage
{
  YmodemStageNone,
  YmodemStageEstablishing,
  YmodemStageEstablished,
  YmodemStageTransmitting,
  YmodemStageFinishing,
  YmodemStageFinished
};

enum YmodemStatus
{
  YmodemStatusEstablish,
  YmodemStatusTransmit,
  YmodemStatusFinish,
  YmodemStatusAbort,
  YmodemStatusTimeout,
  YmodemStatusError
};

class Ymodem
{
public:
  virtual YmodemCode callback(YmodemStatus status, uint8_t *buff, uint32_t *len) = 0;

  virtual uint32_t read(uint8_t *buff, uint32_t len)  = 0;
  virtual uint32_t write(uint8_t *buff, uint32_t len) = 0;

  uint32_t canNum;
  uint32_t timeDivide;
  uint32_t timeMax;
  uint32_t errorMax;

  uint32_t timeCount;
  uint32_t errorCount;
  uint8_t  dataCount;

  YmodemCode  code;
  YmodemStage stage;

  uint8_t  rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD];
  uint8_t  txBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD];
  uint32_t rxLength;
  uint32_t txLength;
};

/* Variable declarations -----------------------------------------------------*/
/* Variable definitions ------------------------------------------------------*/
/* Function declarations -----------------------------------------------------*/
void YmodemReceive(Ymodem *ymodem);
void YmodemTransmit(Ymodem *ymodem);
void YmodemAbort(Ymodem *ymodem);

/* Function definitions ------------------------------------------------------*/

#endif /* __YMODEM_H */
