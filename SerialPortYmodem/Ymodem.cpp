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
/* Function definitions ------------------------------------------------------*/

/**
  * @brief  Ymodem constructor.
  * @param  [in] timeDivide: The fractional factor of the time the ymodem is called.
  * @param  [in] timeMax:    The maximum time when calling the ymodem.
  * @param  [in] errorMax:   The maximum error count when calling the ymodem.
  * @note   The longest waiting time = call time / (@timeDivide + 1) * (@timeMax + 1).
  * @return None.
  */
Ymodem::Ymodem(uint32_t timeDivide, uint32_t timeMax, uint32_t errorMax)
{
  this->timeDivide = timeDivide;
  this->timeMax    = timeMax;
  this->errorMax   = errorMax;

  this->timeCount  = 0;
  this->errorCount = 0;
  this->dataCount  = 0;

  this->code       = CodeNone;
  this->stage      = StageNone;
}

/**
  * @brief  Set the fractional factor of the time the ymodem is called.
  * @param  [in] timeDivide: The fractional factor of the time the ymodem is called.
  * @return None.
  */
void Ymodem::setTimeDivide(uint32_t timeDivide)
{
  this->timeDivide = timeDivide;
}

/**
  * @brief  Get the fractional factor of the time the ymodem is called.
  * @param  None.
  * @return The fractional factor of the time the ymodem is called.
  */
uint32_t Ymodem::getTimeDivide()
{
  return timeDivide;
}

/**
  * @brief  Set the maximum time when calling the ymodem.
  * @param  [in] timeMax: The maximum time when calling the ymodem.
  * @return None.
  */
void Ymodem::setTimeMax(uint32_t timeMax)
{
  this->timeMax = timeMax;
}

/**
  * @brief  Get the maximum time when calling the ymodem.
  * @param  None.
  * @return The maximum time when calling the ymodem.
  */
uint32_t Ymodem::getTimeMax()
{
  return timeMax;
}

/**
  * @brief  Set the maximum error count when calling the ymodem.
  * @param  [in] errorMax: The maximum error count when calling the ymodem.
  * @return None.
  */
void Ymodem::setErrorMax(uint32_t errorMax)
{
  this->errorMax = errorMax;
}

/**
  * @brief  Get the maximum error count when calling the ymodem.
  * @param  None.
  * @return The maximum error count when calling the ymodem.
  */
uint32_t Ymodem::getErrorMax()
{
  return errorMax;
}

/**
  * @brief  Ymodem receive.
  * @param  None.
  * @return None.
  */
void Ymodem::receive()
{
  switch(stage)
  {
    case StageNone:
    {
      receiveStageNone();

      break;
    }

    case StageEstablishing:
    {
      receiveStageEstablishing();

      break;
    }

    case StageEstablished:
    {
      receiveStageEstablished();

      break;
    }

    case StageTransmitting:
    {
      receiveStageTransmitting();

      break;
    }

    case StageFinishing:
    {
      receiveStageFinishing();

      break;
    }

    default:
    {
      receiveStageFinished();
    }
  }
}

/**
  * @brief  Ymodem transmit.
  * @param  None.
  * @return None.
  */
void Ymodem::transmit()
{
  switch(stage)
  {
    case StageNone:
    {
      transmitStageNone();

      break;
    }

    case StageEstablishing:
    {
      transmitStageEstablishing();

      break;
    }

    case StageEstablished:
    {
      transmitStageEstablished();

      break;
    }

    case StageTransmitting:
    {
      transmitStageTransmitting();

      break;
    }

    case StageFinishing:
    {
      transmitStageFinishing();

      break;
    }

    default:
    {
      transmitStageFinished();
    }
  }
}

/**
  * @brief  Ymodem abort.
  * @param  None.
  * @return None.
  */
void Ymodem::abort()
{
  timeCount  = 0;
  errorCount = 0;
  dataCount  = 0;
  code       = CodeNone;
  stage      = StageNone;

  for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
  {
    txBuffer[txLength] = CodeCan;
  }

  write(txBuffer, txLength);
}

/**
  * @brief  Receives a packet of data.
  * @param  None.
  * @return Packet type.
  */
Ymodem::Code Ymodem::receivePacket()
{
  if(code == CodeNone)
  {
    if(read(&(rxBuffer[0]), 1) > 0)
    {
      if(rxBuffer[0] == CodeSoh)
      {
        uint32_t len = read(&(rxBuffer[1]), YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1);

        if(len < (YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1))
        {
          rxLength = len + 1;
          code     = CodeSoh;

          return CodeNone;
        }
        else
        {
          return CodeSoh;
        }
      }
      else if(rxBuffer[0] == CodeStx)
      {
        uint32_t len = read(&(rxBuffer[1]), YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 1);

        if(len < (YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 1))
        {
          rxLength = len + 1;
          code     = CodeStx;

          return CodeNone;
        }
        else
        {
          return CodeStx;
        }
      }
      else
      {
        return (Code)(rxBuffer[0]);
      }
    }
    else
    {
      return CodeNone;
    }
  }
  else
  {
    if(code == CodeSoh)
    {
      uint32_t len = read(&(rxBuffer[rxLength]), YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - rxLength);

      if(len < (YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - rxLength))
      {
        rxLength += len;

        return CodeNone;
      }
      else
      {
        code = CodeNone;

        return CodeSoh;
      }
    }
    else if(code == CodeStx)
    {
      uint32_t len = read(&(rxBuffer[rxLength]), YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - rxLength);

      if(len < (YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - rxLength))
      {
        rxLength += len;

        return CodeNone;
      }
      else
      {
        code = CodeNone;

        return CodeStx;
      }
    }
    else
    {
      code = CodeNone;

      return CodeNone;
    }
  }
}

/**
  * @brief  Receive none stage.
  * @param  None.
  * @return None.
  */
void Ymodem::receiveStageNone()
{
  timeCount   = 0;
  errorCount  = 0;
  dataCount   = 0;
  code        = CodeNone;
  stage       = StageEstablishing;
  txBuffer[0] = CodeC;
  txLength    = 1;
  write(txBuffer, txLength);
}

/**
  * @brief  Receive establishing stage.
  * @param  None.
  * @return None.
  */
void Ymodem::receiveStageEstablishing()
{
  switch(receivePacket())
  {
    case CodeSoh:
    {
      uint16_t crc = ((uint16_t)(rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((rxBuffer[1] == 0x00) && (rxBuffer[2] == 0xFF) &&
         (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_SIZE;

        if(callback(StatusEstablish, &(rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == CodeAck)
        {
          timeCount   = 0;
          errorCount  = 0;
          dataCount   = 0;
          code        = CodeNone;
          stage       = StageEstablished;
          txBuffer[0] = CodeAck;
          txBuffer[1] = CodeC;
          txLength    = 2;
          write(txBuffer, txLength);
        }
        else
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
        }
      }
      else
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeC;
          txLength    = 1;
          write(txBuffer, txLength);
        }
      }

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusTimeout, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        txBuffer[0] = CodeC;
        txLength    = 1;
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Receive established stage.
  * @param  None.
  * @return None.
  */
void Ymodem::receiveStageEstablished()
{
  switch(receivePacket())
  {
    case CodeSoh:
    {
      uint16_t crc = ((uint16_t)(rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((rxBuffer[1] == 0x00) && (rxBuffer[2] == 0xFF) &&
         (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeAck;
          txBuffer[1] = CodeC;
          txLength    = 2;
          write(txBuffer, txLength);
        }
      }
      else if((rxBuffer[1] == 0x01) && (rxBuffer[2] == 0xFE) &&
              (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_SIZE;

        if(callback(StatusTransmit, &(rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == CodeAck)
        {
          timeCount   = 0;
          errorCount  = 0;
          dataCount   = 1;
          code        = CodeNone;
          stage       = StageTransmitting;
          txBuffer[0] = CodeAck;
          txLength    = 1;
          write(txBuffer, txLength);
        }
        else
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
        }
      }
      else
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeNak;
          txLength    = 1;
          write(txBuffer, txLength);
        }
      }

      break;
    }

    case CodeStx:
    {
      uint16_t crc = ((uint16_t)(rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((rxBuffer[1] == 0x01) && (rxBuffer[2] == 0xFE) &&
         (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_1K_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_1K_SIZE;

        if(callback(StatusTransmit, &(rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == CodeAck)
        {
          timeCount   = 0;
          errorCount  = 0;
          dataCount   = 1;
          code        = CodeNone;
          stage       = StageTransmitting;
          txBuffer[0] = CodeAck;
          txLength    = 1;
          write(txBuffer, txLength);
        }
        else
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
        }
      }
      else
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeNak;
          txLength    = 1;
          write(txBuffer, txLength);
        }
      }

      break;
    }

    case CodeEot:
    {
      timeCount   = 0;
      errorCount  = 0;
      dataCount   = 0;
      code        = CodeNone;
      stage       = StageFinishing;
      txBuffer[0] = CodeNak;
      txLength    = 1;
      write(txBuffer, txLength);

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        txBuffer[0] = CodeNak;
        txLength    = 1;
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Receive transmitting stage.
  * @param  None.
  * @return None.
  */
void Ymodem::receiveStageTransmitting()
{
  switch(receivePacket())
  {
    case CodeSoh:
    {
      uint16_t crc = ((uint16_t)(rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((rxBuffer[1] == (uint8_t)(dataCount)) && (rxBuffer[2] == (uint8_t)(0xFF - dataCount)) &&
         (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeAck;
          txLength    = 1;
          write(txBuffer, txLength);
        }
      }
      else if((rxBuffer[1] == (uint8_t)(dataCount + 1)) && (rxBuffer[2] == (uint8_t)(0xFE - dataCount)) &&
              (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_SIZE;

        if(callback(StatusTransmit, &(rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == CodeAck)
        {
          timeCount   = 0;
          errorCount  = 0;
          dataCount   = dataCount + 1;
          code        = CodeNone;
          stage       = StageTransmitting;
          txBuffer[0] = CodeAck;
          txLength    = 1;
          write(txBuffer, txLength);
        }
        else
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
        }
      }
      else
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeNak;
          txLength    = 1;
          write(txBuffer, txLength);
        }
      }

      break;
    }

    case CodeStx:
    {
      uint16_t crc = ((uint16_t)(rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(rxBuffer[YMODEM_PACKET_1K_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((rxBuffer[1] == (uint8_t)(dataCount)) && (rxBuffer[2] == (uint8_t)(0xFF - dataCount)) &&
         (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_1K_SIZE)))
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeAck;
          txLength    = 1;
          write(txBuffer, txLength);
        }
      }
      else if((rxBuffer[1] == (uint8_t)(dataCount + 1)) && (rxBuffer[2] == (uint8_t)(0xFE - dataCount)) &&
              (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_1K_SIZE)))
      {
        uint32_t dataLength = YMODEM_PACKET_1K_SIZE;

        if(callback(StatusTransmit, &(rxBuffer[YMODEM_PACKET_HEADER]), &dataLength) == CodeAck)
        {
          timeCount   = 0;
          errorCount  = 0;
          dataCount   = dataCount + 1;
          code        = CodeNone;
          stage       = StageTransmitting;
          txBuffer[0] = CodeAck;
          txLength    = 1;
          write(txBuffer, txLength);
        }
        else
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
        }
      }
      else
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeNak;
          txLength    = 1;
          write(txBuffer, txLength);
        }
      }

      break;
    }

    case CodeEot:
    {
      timeCount   = 0;
      errorCount  = 0;
      dataCount   = 0;
      code        = CodeNone;
      stage       = StageFinishing;
      txBuffer[0] = CodeNak;
      txLength    = 1;
      write(txBuffer, txLength);

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        txBuffer[0] = CodeNak;
        txLength    = 1;
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Receive finishing stage.
  * @param  None.
  * @return None.
  */
void Ymodem::receiveStageFinishing()
{
  switch(receivePacket())
  {
    case CodeEot:
    {
      timeCount   = 0;
      errorCount  = 0;
      dataCount   = 0;
      code        = CodeNone;
      stage       = StageFinished;
      txBuffer[0] = CodeAck;
      txBuffer[1] = CodeC;
      txLength    = 2;
      write(txBuffer, txLength);

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        txBuffer[0] = CodeNak;
        txLength    = 1;
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Receive finished stage.
  * @param  None.
  * @return None.
  */
void Ymodem::receiveStageFinished()
{
  switch(receivePacket())
  {
    case CodeSoh:
    {
      uint16_t crc = ((uint16_t)(rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2]) << 8) |
                     ((uint16_t)(rxBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1]) << 0);

      if((rxBuffer[1] == 0x00) && (rxBuffer[2] == 0xFF) &&
         (crc == crc16(&(rxBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE)))
      {
        timeCount   = 0;
        errorCount  = 0;
        dataCount   = 0;
        code        = CodeNone;
        stage       = StageNone;
        txBuffer[0] = CodeAck;
        txLength    = 1;
        write(txBuffer, txLength);
        callback(StatusFinish, NULL, NULL);
      }
      else
      {
        errorCount++;

        if(errorCount > errorMax)
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
          callback(StatusError, NULL, NULL);
        }
        else
        {
          txBuffer[0] = CodeNak;
          txLength    = 1;
          write(txBuffer, txLength);
        }
      }

      break;
    }

    case CodeEot:
    {
      errorCount++;

      if(errorCount > errorMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else
      {
        txBuffer[0] = CodeAck;
        txBuffer[1] = CodeC;
        txLength    = 2;
        write(txBuffer, txLength);
      }

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        txBuffer[0] = CodeNak;
        txLength    = 1;
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Transmit none stage.
  * @param  None.
  * @return None.
  */
void Ymodem::transmitStageNone()
{
  timeCount   = 0;
  errorCount  = 0;
  dataCount   = 0;
  code        = CodeNone;
  stage       = StageEstablishing;
}

/**
  * @brief  Transmit establishing stage.
  * @param  None.
  * @return None.
  */
void Ymodem::transmitStageEstablishing()
{
  switch(receivePacket())
  {
    case CodeC:
    {
      memset(&(txBuffer[YMODEM_PACKET_HEADER]), NULL, YMODEM_PACKET_SIZE);

      if(callback(StatusEstablish, &(txBuffer[YMODEM_PACKET_HEADER]), &(txLength)) == CodeAck)
      {
        uint16_t crc = crc16(&(txBuffer[YMODEM_PACKET_HEADER]), txLength);

        timeCount                                       = 0;
        errorCount                                      = 0;
        dataCount                                       = 0;
        code                                            = CodeNone;
        stage                                           = StageEstablished;
        txBuffer[0]                                     = CodeSoh;
        txBuffer[1]                                     = 0x00;
        txBuffer[2]                                     = 0xFF;
        txBuffer[txLength + YMODEM_PACKET_OVERHEAD - 2] = (uint8_t)(crc >> 8);
        txBuffer[txLength + YMODEM_PACKET_OVERHEAD - 1] = (uint8_t)(crc >> 0);
        txLength                                        = txLength + YMODEM_PACKET_OVERHEAD;
        write(txBuffer, txLength);
      }
      else
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
      }

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusTimeout, NULL, NULL);
      }
    }
  }
}

/**
  * @brief  Transmit established stage.
  * @param  None.
  * @return None.
  */
void Ymodem::transmitStageEstablished()
{
  switch(receivePacket())
  {
    case CodeNak:
    {
      errorCount++;

      if(errorCount > errorMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else
      {
        write(txBuffer, txLength);
      }

      break;
    }

    case CodeC:
    {
      errorCount++;

      if(errorCount > errorMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = dataCount;
        code       = CodeNone;
        stage      = (Stage)(stage + dataCount);
        write(txBuffer, txLength);
      }

      break;
    }

    case CodeAck:
    {
      memset(&(txBuffer[YMODEM_PACKET_HEADER]), NULL, YMODEM_PACKET_1K_SIZE);

      switch(callback(StatusTransmit, &(txBuffer[YMODEM_PACKET_HEADER]), &(txLength)))
      {
        case CodeAck:
        {
          uint16_t crc = crc16(&(txBuffer[YMODEM_PACKET_HEADER]), txLength);

          timeCount                                       = 0;
          errorCount                                      = 0;
          dataCount                                       = 1;
          code                                            = CodeNone;
          stage                                           = StageEstablished;
          txBuffer[0]                                     = txLength > YMODEM_PACKET_SIZE ? CodeStx : CodeSoh;
          txBuffer[1]                                     = 0x01;
          txBuffer[2]                                     = 0xFE;
          txBuffer[txLength + YMODEM_PACKET_OVERHEAD - 2] = (uint8_t)(crc >> 8);
          txBuffer[txLength + YMODEM_PACKET_OVERHEAD - 1] = (uint8_t)(crc >> 0);
          txLength                                        = txLength + YMODEM_PACKET_OVERHEAD;

          break;
        }

        case CodeEot:
        {
          timeCount   = 0;
          errorCount  = 0;
          dataCount   = 2;
          code        = CodeNone;
          stage       = StageEstablished;
          txBuffer[0] = CodeEot;
          txLength    = 1;
          write(txBuffer, txLength);

          break;
        }

        default:
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
        }
      }

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Transmit transmitting stage.
  * @param  None.
  * @return None.
  */
void Ymodem::transmitStageTransmitting()
{
  switch(receivePacket())
  {
    case CodeNak:
    {
      errorCount++;

      if(errorCount > errorMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else
      {
        write(txBuffer, txLength);
      }

      break;
    }

    case CodeAck:
    {
      memset(&(txBuffer[YMODEM_PACKET_HEADER]), NULL, YMODEM_PACKET_1K_SIZE);

      switch(callback(StatusTransmit, &(txBuffer[YMODEM_PACKET_HEADER]), &(txLength)))
      {
        case CodeAck:
        {
          uint16_t crc = crc16(&(txBuffer[YMODEM_PACKET_HEADER]), txLength);

          timeCount                                       = 0;
          errorCount                                      = 0;
          dataCount                                       = dataCount + 1;
          code                                            = CodeNone;
          stage                                           = StageTransmitting;
          txBuffer[0]                                     = txLength > YMODEM_PACKET_SIZE ? CodeStx : CodeSoh;
          txBuffer[1]                                     = dataCount;
          txBuffer[2]                                     = 0xFF - dataCount;
          txBuffer[txLength + YMODEM_PACKET_OVERHEAD - 2] = (uint8_t)(crc >> 8);
          txBuffer[txLength + YMODEM_PACKET_OVERHEAD - 1] = (uint8_t)(crc >> 0);
          txLength                                        = txLength + YMODEM_PACKET_OVERHEAD;
          write(txBuffer, txLength);

          break;
        }

        case CodeEot:
        {
          timeCount   = 0;
          errorCount  = 0;
          dataCount   = 0;
          code        = CodeNone;
          stage       = StageFinishing;
          txBuffer[0] = CodeEot;
          txLength    = 1;
          write(txBuffer, txLength);

          break;
        }

        default:
        {
          timeCount  = 0;
          errorCount = 0;
          dataCount  = 0;
          code       = CodeNone;
          stage      = StageNone;

          for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
          {
            txBuffer[txLength] = CodeCan;
          }

          write(txBuffer, txLength);
        }
      }

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Transmit finishing stage.
  * @param  None.
  * @return None.
  */
void Ymodem::transmitStageFinishing()
{
  switch(receivePacket())
  {
    case CodeNak:
    {
      timeCount   = 0;
      errorCount  = 0;
      dataCount   = 0;
      code        = CodeNone;
      stage       = StageFinishing;
      txBuffer[0] = CodeEot;
      txLength    = 1;
      write(txBuffer, txLength);

      break;
    }

    case CodeC:
    {
      memset(&(txBuffer[YMODEM_PACKET_HEADER]), NULL, YMODEM_PACKET_SIZE);
      uint16_t crc = crc16(&(txBuffer[YMODEM_PACKET_HEADER]), YMODEM_PACKET_SIZE);

      timeCount                                                 = 0;
      errorCount                                                = 0;
      dataCount                                                 = 0;
      code                                                      = CodeNone;
      stage                                                     = StageFinished;
      txBuffer[0]                                               = CodeSoh;
      txBuffer[1]                                               = 0x00;
      txBuffer[2]                                               = 0xFF;
      txBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 2] = (uint8_t)(crc >> 8);
      txBuffer[YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD - 1] = (uint8_t)(crc >> 0);
      txLength                                                  = YMODEM_PACKET_SIZE + YMODEM_PACKET_OVERHEAD;
      write(txBuffer, txLength);

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Transmit finished stage.
  * @param  None.
  * @return None.
  */
void Ymodem::transmitStageFinished()
{
  switch(receivePacket())
  {
    case CodeC:
    case CodeNak:
    {
      errorCount++;

      if(errorCount > errorMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else
      {
        write(txBuffer, txLength);
      }

      break;
    }

    case CodeAck:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusFinish, NULL, NULL);

      break;
    }

    case CodeA1:
    case CodeA2:
    case CodeCan:
    {
      timeCount  = 0;
      errorCount = 0;
      dataCount  = 0;
      code       = CodeNone;
      stage      = StageNone;
      callback(StatusAbort, NULL, NULL);

      break;
    }

    default:
    {
      timeCount++;

      if((timeCount / (timeDivide + 1)) > timeMax)
      {
        timeCount  = 0;
        errorCount = 0;
        dataCount  = 0;
        code       = CodeNone;
        stage      = StageNone;

        for(txLength = 0; txLength < YMODEM_CODE_CAN_NUMBER; txLength++)
        {
          txBuffer[txLength] = CodeCan;
        }

        write(txBuffer, txLength);
        callback(StatusError, NULL, NULL);
      }
      else if((timeCount % (timeDivide + 1)) == 0)
      {
        write(txBuffer, txLength);
      }
    }
  }
}

/**
  * @brief  Calculate CRC16 checksum.
  * @param  [in] buff: The data to be calculated.
  * @param  [in] len:  The length of the data to be calculated.
  * @return Calculated CRC16 checksum.
  */
uint16_t Ymodem::crc16(uint8_t *buff, uint32_t len)
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
