#include "YmodemFileTransmit.h"
#include <QFileInfo>

#define READ_TIME_OUT   (10)
#define WRITE_TIME_OUT  (100)

YmodemFileTransmit::YmodemFileTransmit(QObject *parent) :
    QObject(parent),
    file(new QFile),
    readTimer(new QTimer),
    writeTimer(new QTimer),
    serialPort(new QSerialPort)
{
    canNum     = 5;
    timeDivide = 499;
    timeMax    = 12;
    errorMax   = 999;
    stage      = YmodemStageNone;

    serialPort->setPortName("COM1");
    serialPort->setBaudRate(115200);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    connect(readTimer, SIGNAL(timeout()), this, SLOT(readTimeOut()));
    connect(writeTimer, SIGNAL(timeout()), this, SLOT(writeTimeOut()));
}

YmodemFileTransmit::~YmodemFileTransmit()
{
    delete file;
    delete readTimer;
    delete writeTimer;
    delete serialPort;
}

void YmodemFileTransmit::setFileName(const QString &name)
{
    file->setFileName(name);
}

void YmodemFileTransmit::setPortName(const QString &name)
{
    serialPort->setPortName(name);
}

void YmodemFileTransmit::setPortBaudRate(qint32 baudrate)
{
    serialPort->setBaudRate(baudrate);
}

bool YmodemFileTransmit::startTransmit()
{
    progress = 0;
    status   = YmodemStatusEstablish;

    if(serialPort->open(QSerialPort::ReadWrite) == true)
    {
        readTimer->start(READ_TIME_OUT);

        return true;
    }
    else
    {
        return false;
    }
}

void YmodemFileTransmit::stopTransmit()
{
    file->close();
    YmodemAbort(this);
    status = YmodemStatusAbort;
    writeTimer->start(WRITE_TIME_OUT);
}

int YmodemFileTransmit::getTransmitProgress()
{
    return progress;
}

YmodemStatus YmodemFileTransmit::getTransmitStatus()
{
    return status;
}

void YmodemFileTransmit::readTimeOut()
{
    readTimer->stop();

    YmodemTransmit(this);

    if((status == YmodemStatusEstablish) || (status == YmodemStatusTransmit))
    {
        readTimer->start(READ_TIME_OUT);
    }
}

void YmodemFileTransmit::writeTimeOut()
{
    writeTimer->stop();

    serialPort->close();
    serialPort->clear();

    transmitStatus(status);
}

YmodemCode YmodemFileTransmit::callback(YmodemStatus status, uint8_t *buff, uint32_t *len)
{
    switch(status)
    {
        case YmodemStatusEstablish:
        {
            if(file->open(QFile::ReadOnly) == true)
            {
                QFileInfo fileInfo(*file);

                fileSize  = fileInfo.size();
                fileCount = 0;

                strcpy((char *)buff, fileInfo.fileName().toLocal8Bit().data());
                strcpy((char *)buff + fileInfo.fileName().toLocal8Bit().size() + 1, QByteArray::number(fileInfo.size()).data());

                *len = YMODEM_PACKET_SIZE;

                YmodemFileTransmit::status = YmodemStatusEstablish;

                transmitStatus(YmodemStatusEstablish);

                return YmodemCodeAck;
            }
            else
            {
                YmodemFileTransmit::status = YmodemStatusError;

                writeTimer->start(WRITE_TIME_OUT);

                return YmodemCodeCan;
            }
        }

        case YmodemStatusTransmit:
        {
            if(fileSize != fileCount)
            {
                if((fileSize - fileCount) > YMODEM_PACKET_SIZE)
                {
                    fileCount += file->read((char *)buff, YMODEM_PACKET_1K_SIZE);

                    *len = YMODEM_PACKET_1K_SIZE;
                }
                else
                {
                    fileCount += file->read((char *)buff, YMODEM_PACKET_SIZE);

                    *len = YMODEM_PACKET_SIZE;
                }

                progress = (int)(fileCount * 100 / fileSize);

                YmodemFileTransmit::status = YmodemStatusTransmit;

                transmitProgress(progress);
                transmitStatus(YmodemStatusTransmit);

                return YmodemCodeAck;
            }
            else
            {
                YmodemFileTransmit::status = YmodemStatusTransmit;

                transmitStatus(YmodemStatusTransmit);

                return YmodemCodeEot;
            }
        }

        case YmodemStatusFinish:
        {
            file->close();

            YmodemFileTransmit::status = YmodemStatusFinish;

            writeTimer->start(WRITE_TIME_OUT);

            return YmodemCodeAck;
        }

        case YmodemStatusAbort:
        {
            file->close();

            YmodemFileTransmit::status = YmodemStatusAbort;

            writeTimer->start(WRITE_TIME_OUT);

            return YmodemCodeCan;
        }

        case YmodemStatusTimeout:
        {
            YmodemFileTransmit::status = YmodemStatusTimeout;

            writeTimer->start(WRITE_TIME_OUT);

            return YmodemCodeCan;
        }

        default:
        {
            file->close();

            YmodemFileTransmit::status = YmodemStatusError;

            writeTimer->start(WRITE_TIME_OUT);

            return YmodemCodeCan;
        }
    }
}

uint32_t YmodemFileTransmit::read(uint8_t *buff, uint32_t len)
{
    return serialPort->read((char *)buff, len);
}

uint32_t YmodemFileTransmit::write(uint8_t *buff, uint32_t len)
{
    return serialPort->write((char *)buff, len);
}
