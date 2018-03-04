/**
  ******************************************************************************
  * @file    Ymodem.cpp
  * @author  XinLi
  * @version v1.0
  * @date    21-January-2018
  * @brief   Ymodem protocol module source file.
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

/* Header includes -----------------------------------------------------------*/
#include "Ymodem.h"
#include <string.h>

/* Macro definitions ---------------------------------------------------------*/
/* Type definitions ----------------------------------------------------------*/
/* Variable declarations -----------------------------------------------------*/
/* Variable definitions ------------------------------------------------------*/
/* Function declarations -----------------------------------------------------*/
static YmodemCode YmodemReceivePacket(Ymodem *ymodem);

static void YmodemReceiveStageNone(Ymodem *ymodem);
static void YmodemReceiveStageEstablishing(Ymodem *ymodem);
static void YmodemReceiveStageEstablished(Ymodem *ymodem);
static void YmodemReceiveStageTransmitting(Ymodem *ymodem);
static void YmodemReceiveStageFinishing(Ymodem *ymodem);
static void YmodemReceiveStageFinished(Ymodem *ymodem);

static void YmodemTransmitStageNone(Ymodem *ymodem);
static void YmodemTransmitStageEstablishing(Ymodem *ymodem);
static void YmodemTransmitStageEstablished(Ymodem *ymodem);
static void YmodemTransmitStageTransmitting(Ymodem *ymodem);
static void YmodemTransmitStageFinishing(Ymodem *ymodem);
static void YmodemTransmitStageFinished(Ymodem *ymodem);

static uint16_t YmodemCRC16(uint8_t *buff, uint32_t len);

/* Function definitions ------------------------------------------------------*/

/**
  * @brief  Ymodem receives a packet of data.
  * @param  [in] ymodem: The ymodem to be transmissioned.
  * @return Packet type.
  */
static YmodemCode YmodemReceivePacket(Ymodem *ymodem)
{
  if(ymodem->code == YmodemCodeNone)
  {
    if(ymodem->read(&(ymodem->rxBuffer[0]), 1) > 0)
    {
      if(ymodem->rxBuffer[0] == YmodemCodeSoh)
      {
        uint32_t len = ymodem->read(&(ymodem->rxBuffer[1]), YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1);

        if(len < (YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1))
        {
          ymodem->rxLength = len + 1;
          ymodem->code     = YmodemCodeSoh;

          return YmodemCodeNone;
        }
        else
        {
          return YmodemCodeSoh;
        }
      }
      else if(ymodem->rxBuffer[0] == YmodemCodeStx)
      {
        uint32_t len = ymodem->read(&(ymodem->rxBuffer[1]), YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 1);

        if(len < (YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 1))
        {
          ymodem->rxLength = len + 1;
          ymodem->code     = YmodemCodeStx;

          return YmodemCodeNone;
        }
        else
        {
          return YmodemCodeStx;
        }
      }
      else
      {
        return (YmodemCode)(ymodem->rxBuffer[0]);
      }
    }
    else
    {
      return YmodemCodeNone;
    }
  }
  else
  {
    if(ymodem->code == YmodemCodeSoh)
    {
      uint32_t len = ymodem->read(&(ymodem->rxBuffer[ymodem->rxLength]), YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - ymodem->rxLength);

      if(len < (YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - ymodem->rxLength))
      {
        ymodem->rxLength += len;

        return YmodemCodeNone;
      }
      else
      {
        ymodem->code = YmodemCodeNone;

        return YmodemCodeSoh;
      }
    }
    else if(ymodem->code == YmodemCodeStx)
    {
      uint32_t len = ymodem->read(&(ymodem->rxBuffer[ymodem->rxLength]), YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - ymodem->rxLength);

      if(len < (YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - ymodem->rxLength))
      {
        ymodem->rxLength += len;

        return YmodemCodeNone;
      }
      else
      {
        ymodem->code = YmodemCodeNone;

        return YmodemCodeStx;
      }
    }
    else
    {
      ymodem->code = YmodemCodeNone;

      return YmodemCodeNone;
    }
  }
}

/**
  * @brief  Ymodem receive none stage.
  * @param  [in] ymodem: The ymodem to be received.
  * @return None.
  */
static void YmodemReceiveStageNone(Ymodem *ymodem)
{
  ymodem->timeCount   = 0;
  ymodem->errorCount  = 0;
  ymodem->dataCount   = 0;
  ymodem->code        = YmodemCodeNone;
  ymodem->stage       = YmodemStageEstablishing;
  ymodem->txBuffer[0] = YmodemCodeC;
  ymodem->txLength    = 1;
  ymodem->write(ymodem->txBuffer, ymodem->txLength);
}

/**
  * @brief  Ymodem receive establishing stage.
  * @param  [in] ymodem: The ymodem to be received.
  * @return None.
  */
static void YmodemReceiveStageEstablishing(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeSoh:
    {
      uint16_t crc = ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((ymodem->rxBuffer[1] == 0x00) && (ymodem->rxBuffer[2] == 0xFF) &&
         (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_SIZE;

        if(ymodem->callback(YmodemStatusEstablish, &(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == YmodemCodeAck)
        {
          ymodem->timeCount   = 0;
          ymodem->errorCount  = 0;
          ymodem->dataCount   = 0;
          ymodem->code        = YmodemCodeNone;
          ymodem->stage       = YmodemStageEstablished;
          ymodem->txBuffer[0] = YmodemCodeAck;
          ymodem->txBuffer[1] = YmodemCodeC;
          ymodem->txLength    = 2;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
        else
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }
      else
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeC;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusTimeout, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->txBuffer[0] = YmodemCodeC;
        ymodem->txLength    = 1;
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Ymodem receive established stage.
  * @param  [in] ymodem: The ymodem to be received.
  * @return None.
  */
static void YmodemReceiveStageEstablished(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeSoh:
    {
      uint16_t crc = ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((ymodem->rxBuffer[1] == 0x00) && (ymodem->rxBuffer[2] == 0xFF) &&
         (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeAck;
          ymodem->txBuffer[1] = YmodemCodeC;
          ymodem->txLength    = 2;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }
      else if((ymodem->rxBuffer[1] == 0x01) && (ymodem->rxBuffer[2] == 0xFE) &&
              (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_SIZE;

        if(ymodem->callback(YmodemStatusTransmit, &(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == YmodemCodeAck)
        {
          ymodem->timeCount   = 0;
          ymodem->errorCount  = 0;
          ymodem->dataCount   = 1;
          ymodem->code        = YmodemCodeNone;
          ymodem->stage       = YmodemStageTransmitting;
          ymodem->txBuffer[0] = YmodemCodeAck;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
        else
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }
      else
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeNak;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }

      break;
    }

    case YmodemCodeStx:
    {
      uint16_t crc = ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((ymodem->rxBuffer[1] == 0x01) && (ymodem->rxBuffer[2] == 0xFE) &&
         (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_1K_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_1K_SIZE;

        if(ymodem->callback(YmodemStatusTransmit, &(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == YmodemCodeAck)
        {
          ymodem->timeCount   = 0;
          ymodem->errorCount  = 0;
          ymodem->dataCount   = 1;
          ymodem->code        = YmodemCodeNone;
          ymodem->stage       = YmodemStageTransmitting;
          ymodem->txBuffer[0] = YmodemCodeAck;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
        else
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }
      else
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeNak;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }

      break;
    }

    case YmodemCodeEot:
    {
      ymodem->timeCount   = 0;
      ymodem->errorCount  = 0;
      ymodem->dataCount   = 0;
      ymodem->code        = YmodemCodeNone;
      ymodem->stage       = YmodemStageFinishing;
      ymodem->txBuffer[0] = YmodemCodeNak;
      ymodem->txLength    = 1;
      ymodem->write(ymodem->txBuffer, ymodem->txLength);

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->txBuffer[0] = YmodemCodeNak;
        ymodem->txLength    = 1;
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Ymodem receive transmitting stage.
  * @param  [in] ymodem: The ymodem to be received.
  * @return None.
  */
static void YmodemReceiveStageTransmitting(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeSoh:
    {
      uint16_t crc = ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((ymodem->rxBuffer[1] == (uint8_t)(ymodem->dataCount)) && (ymodem->rxBuffer[2] == (uint8_t)(0xFF - ymodem->dataCount)) &&
         (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeAck;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }
      else if((ymodem->rxBuffer[1] == (uint8_t)(ymodem->dataCount + 1)) && (ymodem->rxBuffer[2] == (uint8_t)(0xFE - ymodem->dataCount)) &&
              (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_SIZE;

        if(ymodem->callback(YmodemStatusTransmit, &(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == YmodemCodeAck)
        {
          ymodem->timeCount   = 0;
          ymodem->errorCount  = 0;
          ymodem->dataCount   = ymodem->dataCount + 1;
          ymodem->code        = YmodemCodeNone;
          ymodem->stage       = YmodemStageTransmitting;
          ymodem->txBuffer[0] = YmodemCodeAck;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
        else
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }
      else
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeNak;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }

      break;
    }

    case YmodemCodeStx:
    {
      uint16_t crc = ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((ymodem->rxBuffer[1] == (uint8_t)(ymodem->dataCount)) && (ymodem->rxBuffer[2] == (uint8_t)(0xFF - ymodem->dataCount)) &&
         (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_1K_SIZE)))
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeAck;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }
      else if((ymodem->rxBuffer[1] == (uint8_t)(ymodem->dataCount + 1)) && (ymodem->rxBuffer[2] == (uint8_t)(0xFE - ymodem->dataCount)) &&
              (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_1K_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_1K_SIZE;

        if(ymodem->callback(YmodemStatusTransmit, &(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == YmodemCodeAck)
        {
          ymodem->timeCount   = 0;
          ymodem->errorCount  = 0;
          ymodem->dataCount   = ymodem->dataCount + 1;
          ymodem->code        = YmodemCodeNone;
          ymodem->stage       = YmodemStageTransmitting;
          ymodem->txBuffer[0] = YmodemCodeAck;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
        else
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }
      else
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeNak;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }

      break;
    }

    case YmodemCodeEot:
    {
      ymodem->timeCount   = 0;
      ymodem->errorCount  = 0;
      ymodem->dataCount   = 0;
      ymodem->code        = YmodemCodeNone;
      ymodem->stage       = YmodemStageFinishing;
      ymodem->txBuffer[0] = YmodemCodeNak;
      ymodem->txLength    = 1;
      ymodem->write(ymodem->txBuffer, ymodem->txLength);

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->txBuffer[0] = YmodemCodeNak;
        ymodem->txLength    = 1;
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Ymodem receive finishing stage.
  * @param  [in] ymodem: The ymodem to be received.
  * @return None.
  */
static void YmodemReceiveStageFinishing(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeEot:
    {
      ymodem->timeCount   = 0;
      ymodem->errorCount  = 0;
      ymodem->dataCount   = 0;
      ymodem->code        = YmodemCodeNone;
      ymodem->stage       = YmodemStageFinished;
      ymodem->txBuffer[0] = YmodemCodeAck;
      ymodem->txBuffer[1] = YmodemCodeC;
      ymodem->txLength    = 2;
      ymodem->write(ymodem->txBuffer, ymodem->txLength);

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->txBuffer[0] = YmodemCodeNak;
        ymodem->txLength    = 1;
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Ymodem receive finished stage.
  * @param  [in] ymodem: The ymodem to be received.
  * @return None.
  */
static void YmodemReceiveStageFinished(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeSoh:
    {
      uint16_t crc = ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(ymodem->rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((ymodem->rxBuffer[1] == 0x00) && (ymodem->rxBuffer[2] == 0xFF) &&
         (crc == YmodemCRC16(&(ymodem->rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        ymodem->timeCount   = 0;
        ymodem->errorCount  = 0;
        ymodem->dataCount   = 0;
        ymodem->code        = YmodemCodeNone;
        ymodem->stage       = YmodemStageNone;
        ymodem->txBuffer[0] = YmodemCodeAck;
        ymodem->txLength    = 1;
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusFinish, NULL, NULL);
      }
      else
      {
        ymodem->errorCount++;

        if(ymodem->errorCount > ymodem->errorMax)
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
          ymodem->callback(YmodemStatusError, NULL, NULL);
        }
        else
        {
          ymodem->txBuffer[0] = YmodemCodeNak;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }

      break;
    }

    case YmodemCodeEot:
    {
      ymodem->errorCount++;

      if(ymodem->errorCount > ymodem->errorMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else
      {
        ymodem->txBuffer[0] = YmodemCodeAck;
        ymodem->txBuffer[1] = YmodemCodeC;
        ymodem->txLength    = 2;
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->txBuffer[0] = YmodemCodeNak;
        ymodem->txLength    = 1;
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Ymodem transmit none stage.
  * @param  [in] ymodem: The ymodem to be transmitted.
  * @return None.
  */
static void YmodemTransmitStageNone(Ymodem *ymodem)
{
  ymodem->timeCount   = 0;
  ymodem->errorCount  = 0;
  ymodem->dataCount   = 0;
  ymodem->code        = YmodemCodeNone;
  ymodem->stage       = YmodemStageEstablishing;
}

/**
  * @brief  Ymodem transmit establishing stage.
  * @param  [in] ymodem: The ymodem to be transmitted.
  * @return None.
  */
static void YmodemTransmitStageEstablishing(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeC:
    {
      memset(&(ymodem->txBuffer[YMODEM_PACKET_HEADER]), NULL, YMODEM_PACKET_SIZE);

      if(ymodem->callback(YmodemStatusEstablish, &(ymodem->txBuffer[YMODEM_PACKET_HEADER]), &(ymodem->txLength)) == YmodemCodeAck)
      {
        uint16_t crc = YmodemCRC16(&(ymodem->txBuffer[YMODEM_PACKET_HEADER]), ymodem->txLength);

        ymodem->timeCount                                               = 0;
        ymodem->errorCount                                              = 0;
        ymodem->dataCount                                               = 0;
        ymodem->code                                                    = YmodemCodeNone;
        ymodem->stage                                                   = YmodemStageEstablished;
        ymodem->txBuffer[0]                                             = YmodemCodeSoh;
        ymodem->txBuffer[1]                                             = 0x00;
        ymodem->txBuffer[2]                                             = 0xFF;
        ymodem->txBuffer[ymodem->txLength + YMODEM_PACKET_OVERHEAD - 2] = (uint8_t)(crc >> 8);
        ymodem->txBuffer[ymodem->txLength + YMODEM_PACKET_OVERHEAD - 1] = (uint8_t)(crc >> 0);
        ymodem->txLength                                                = ymodem->txLength + YMODEM_PACKET_OVERHEAD;
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
      else
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusTimeout, NULL, NULL);
      }
    }
  }
}

/**
  * @brief  Ymodem transmit established stage.
  * @param  [in] ymodem: The ymodem to be transmitted.
  * @return None.
  */
static void YmodemTransmitStageEstablished(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeNak:
    {
      ymodem->errorCount++;

      if(ymodem->errorCount > ymodem->errorMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else
      {
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }

      break;
    }

    case YmodemCodeC:
    {
      ymodem->errorCount++;

      if(ymodem->errorCount > ymodem->errorMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = ymodem->dataCount;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = (YmodemStage)(ymodem->stage + ymodem->dataCount);
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }

      break;
    }

    case YmodemCodeAck:
    {
      memset(&(ymodem->txBuffer[YMODEM_PACKET_HEADER]), NULL, YMODEM_PACKET_1K_SIZE);

      switch(ymodem->callback(YmodemStatusTransmit, &(ymodem->txBuffer[YMODEM_PACKET_HEADER]), &(ymodem->txLength)))
      {
        case YmodemCodeAck:
        {
          uint16_t crc = YmodemCRC16(&(ymodem->txBuffer[YMODEM_PACKET_HEADER]), ymodem->txLength);

          ymodem->timeCount                                               = 0;
          ymodem->errorCount                                              = 0;
          ymodem->dataCount                                               = 1;
          ymodem->code                                                    = YmodemCodeNone;
          ymodem->stage                                                   = YmodemStageEstablished;
          ymodem->txBuffer[0]                                             = ymodem->txLength > YMODEM_PACKET_SIZE ? YmodemCodeStx : YmodemCodeSoh;
          ymodem->txBuffer[1]                                             = 0x01;
          ymodem->txBuffer[2]                                             = 0xFE;
          ymodem->txBuffer[ymodem->txLength + YMODEM_PACKET_OVERHEAD - 2] = (uint8_t)(crc >> 8);
          ymodem->txBuffer[ymodem->txLength + YMODEM_PACKET_OVERHEAD - 1] = (uint8_t)(crc >> 0);
          ymodem->txLength                                                = ymodem->txLength + YMODEM_PACKET_OVERHEAD;

          break;
        }

        case YmodemCodeEot:
        {
          ymodem->timeCount   = 0;
          ymodem->errorCount  = 0;
          ymodem->dataCount   = 2;
          ymodem->code        = YmodemCodeNone;
          ymodem->stage       = YmodemStageEstablished;
          ymodem->txBuffer[0] = YmodemCodeEot;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);

          break;
        }

        default:
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Ymodem transmit transmitting stage.
  * @param  [in] ymodem: The ymodem to be transmitted.
  * @return None.
  */
static void YmodemTransmitStageTransmitting(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeNak:
    {
      ymodem->errorCount++;

      if(ymodem->errorCount > ymodem->errorMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else
      {
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }

      break;
    }

    case YmodemCodeAck:
    {
      memset(&(ymodem->txBuffer[YMODEM_PACKET_HEADER]), NULL, YMODEM_PACKET_1K_SIZE);

      switch(ymodem->callback(YmodemStatusTransmit, &(ymodem->txBuffer[YMODEM_PACKET_HEADER]), &(ymodem->txLength)))
      {
        case YmodemCodeAck:
        {
          uint16_t crc = YmodemCRC16(&(ymodem->txBuffer[YMODEM_PACKET_HEADER]), ymodem->txLength);

          ymodem->timeCount                                               = 0;
          ymodem->errorCount                                              = 0;
          ymodem->dataCount                                               = ymodem->dataCount + 1;
          ymodem->code                                                    = YmodemCodeNone;
          ymodem->stage                                                   = YmodemStageTransmitting;
          ymodem->txBuffer[0]                                             = ymodem->txLength > YMODEM_PACKET_SIZE ? YmodemCodeStx : YmodemCodeSoh;
          ymodem->txBuffer[1]                                             = ymodem->dataCount;
          ymodem->txBuffer[2]                                             = 0xFF - ymodem->dataCount;
          ymodem->txBuffer[ymodem->txLength + YMODEM_PACKET_OVERHEAD - 2] = (uint8_t)(crc >> 8);
          ymodem->txBuffer[ymodem->txLength + YMODEM_PACKET_OVERHEAD - 1] = (uint8_t)(crc >> 0);
          ymodem->txLength                                                = ymodem->txLength + YMODEM_PACKET_OVERHEAD;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);

          break;
        }

        case YmodemCodeEot:
        {
          ymodem->timeCount   = 0;
          ymodem->errorCount  = 0;
          ymodem->dataCount   = 0;
          ymodem->code        = YmodemCodeNone;
          ymodem->stage       = YmodemStageFinishing;
          ymodem->txBuffer[0] = YmodemCodeEot;
          ymodem->txLength    = 1;
          ymodem->write(ymodem->txBuffer, ymodem->txLength);

          break;
        }

        default:
        {
          ymodem->timeCount  = 0;
          ymodem->errorCount = 0;
          ymodem->dataCount  = 0;
          ymodem->code       = YmodemCodeNone;
          ymodem->stage      = YmodemStageNone;

          for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
          {
            ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
          }

          ymodem->write(ymodem->txBuffer, ymodem->txLength);
        }
      }

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Ymodem transmit finishing stage.
  * @param  [in] ymodem: The ymodem to be transmitted.
  * @return None.
  */
static void YmodemTransmitStageFinishing(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeNak:
    {
      ymodem->timeCount   = 0;
      ymodem->errorCount  = 0;
      ymodem->dataCount   = 0;
      ymodem->code        = YmodemCodeNone;
      ymodem->stage       = YmodemStageFinishing;
      ymodem->txBuffer[0] = YmodemCodeEot;
      ymodem->txLength    = 1;
      ymodem->write(ymodem->txBuffer, ymodem->txLength);

      break;
    }

    case YmodemCodeC:
    {
      memset(&(ymodem->txBuffer[YMODEM_PACKET_HEADER]), NULL, YMODEM_PACKET_SIZE);
      uint16_t crc = YmodemCRC16(&(ymodem->txBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE);

      ymodem->timeCount                                                 = 0;
      ymodem->errorCount                                                = 0;
      ymodem->dataCount                                                 = 0;
      ymodem->code                                                      = YmodemCodeNone;
      ymodem->stage                                                     = YmodemStageFinished;
      ymodem->txBuffer[0]                                               = YmodemCodeSoh;
      ymodem->txBuffer[1]                                               = 0x00;
      ymodem->txBuffer[2]                                               = 0xFF;
      ymodem->txBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2] = (uint8_t)(crc >> 8);
      ymodem->txBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1] = (uint8_t)(crc >> 0);
      ymodem->txLength                                                  = YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD;
      ymodem->write(ymodem->txBuffer, ymodem->txLength);

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Ymodem transmit finished stage.
  * @param  [in] ymodem: The ymodem to be transmitted.
  * @return None.
  */
static void YmodemTransmitStageFinished(Ymodem *ymodem)
{
  switch(YmodemReceivePacket(ymodem))
  {
    case YmodemCodeC:
    case YmodemCodeNak:
    {
      ymodem->errorCount++;

      if(ymodem->errorCount > ymodem->errorMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else
      {
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }

      break;
    }

    case YmodemCodeAck:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusFinish, NULL, NULL);

      break;
    }

    case YmodemCodeA1:
    case YmodemCodeA2:
    case YmodemCodeCan:
    {
      ymodem->timeCount  = 0;
      ymodem->errorCount = 0;
      ymodem->dataCount  = 0;
      ymodem->code       = YmodemCodeNone;
      ymodem->stage      = YmodemStageNone;
      ymodem->callback(YmodemStatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      ymodem->timeCount++;

      if((ymodem->timeCount / (ymodem->timeDivide + 1)) > ymodem->timeMax)
      {
        ymodem->timeCount  = 0;
        ymodem->errorCount = 0;
        ymodem->dataCount  = 0;
        ymodem->code       = YmodemCodeNone;
        ymodem->stage      = YmodemStageNone;

        for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
        {
          ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
        }

        ymodem->write(ymodem->txBuffer, ymodem->txLength);
        ymodem->callback(YmodemStatusError, NULL, NULL);
      }
      else if((ymodem->timeCount % (ymodem->timeDivide + 1)) == 0)
      {
        ymodem->write(ymodem->txBuffer, ymodem->txLength);
      }
    }
  }
}

/**
  * @brief  Calculate ymodem CRC16 checksum.
  * @param  [in] buff: The data to be calculated.
  * @param  [in] len:  The length of the data to be calculated.
  * @return Calculated CRC16 checksum.
  */
static uint16_t YmodemCRC16(uint8_t *buff, uint32_t len)
{
  uint16_t crc = 0;

  while(len--)
  {
    crc ^= (uint16_t)(*(buff++)) << 8;

    for(int i = 0; i < 8; i++)
    {
      if(crc & 0x8000)
      {
        crc = (crc << 1) ^ 0x1021;
      }
      else
      {
        crc = crc << 1;
      }
    }
  }

  return crc;
}

/**
  * @brief  Ymodem receive.
  * @param  [in] ymodem: The ymodem to be receive.
  * @return None.
  */
void YmodemReceive(Ymodem *ymodem)
{
  switch(ymodem->stage)
  {
    case YmodemStageNone:
    {
      YmodemReceiveStageNone(ymodem);

      break;
    }

    case YmodemStageEstablishing:
    {
      YmodemReceiveStageEstablishing(ymodem);

      break;
    }

    case YmodemStageEstablished:
    {
      YmodemReceiveStageEstablished(ymodem);

      break;
    }

    case YmodemStageTransmitting:
    {
      YmodemReceiveStageTransmitting(ymodem);

      break;
    }

    case YmodemStageFinishing:
    {
      YmodemReceiveStageFinishing(ymodem);

      break;
    }

    default:
    {
      YmodemReceiveStageFinished(ymodem);
    }
  }
}

/**
  * @brief  Ymodem transmit.
  * @param  [in] ymodem: The ymodem to be transmit.
  * @return None.
  */
void YmodemTransmit(Ymodem *ymodem)
{
  switch(ymodem->stage)
  {
    case YmodemStageNone:
    {
      YmodemTransmitStageNone(ymodem);

      break;
    }

    case YmodemStageEstablishing:
    {
      YmodemTransmitStageEstablishing(ymodem);

      break;
    }

    case YmodemStageEstablished:
    {
      YmodemTransmitStageEstablished(ymodem);

      break;
    }

    case YmodemStageTransmitting:
    {
      YmodemTransmitStageTransmitting(ymodem);

      break;
    }

    case YmodemStageFinishing:
    {
      YmodemTransmitStageFinishing(ymodem);

      break;
    }

    default:
    {
      YmodemTransmitStageFinished(ymodem);
    }
  }
}

/**
  * @brief  Abort ymodem transmission.
  * @param  [in] ymodem: The ymodem to be abort.
  * @return None.
  */
void YmodemAbort(Ymodem *ymodem)
{
  ymodem->timeCount  = 0;
  ymodem->errorCount = 0;
  ymodem->dataCount  = 0;
  ymodem->code       = YmodemCodeNone;
  ymodem->stage      = YmodemStageNone;

  for(ymodem->txLength = 0; ymodem->txLength < ymodem->canNum; ymodem->txLength++)
  {
    ymodem->txBuffer[ymodem->txLength] = YmodemCodeCan;
  }

  ymodem->write(ymodem->txBuffer, ymodem->txLength);
}
