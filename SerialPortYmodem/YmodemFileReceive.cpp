#include "YmodemFileReceive.h"

#define READ_TIME_OUT   (10)
#define WRITE_TIME_OUT  (100)

YmodemFileReceive::YmodemFileReceive(QObject *parent) :
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

YmodemFileReceive::~YmodemFileReceive()
{
    delete file;
    delete readTimer;
    delete writeTimer;
    delete serialPort;
}

void YmodemFileReceive::setFilePath(const QString &path)
{
    filePath = path + "/";
}

void YmodemFileReceive::setPortName(const QString &name)
{
    serialPort->setPortName(name);
}

void YmodemFileReceive::setPortBaudRate(qint32 baudrate)
{
    serialPort->setBaudRate(baudrate);
}

bool YmodemFileReceive::startReceive()
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

void YmodemFileReceive::stopReceive()
{
    file->close();
    YmodemAbort(this);
    status = YmodemStatusAbort;
    writeTimer->start(WRITE_TIME_OUT);
}

int YmodemFileReceive::getReceiveProgress()
{
    return progress;
}

YmodemStatus YmodemFileReceive::getReceiveStatus()
{
    return status;
}

void YmodemFileReceive::readTimeOut()
{
    readTimer->stop();

    YmodemReceive(this);

    if((status == YmodemStatusEstablish) || (status == YmodemStatusTransmit))
    {
        readTimer->start(READ_TIME_OUT);
    }
}

void YmodemFileReceive::writeTimeOut()
{
    writeTimer->stop();

    serialPort->close();
    serialPort->clear();

    receiveStatus(status);
}

YmodemCode YmodemFileReceive::callback(YmodemStatus status, uint8_t *buff, uint32_t *len)
{
    switch(status)
    {
        case YmodemStatusEstablish:
        {
            if(buff[0] != 0)
            {
                int  i         =  0;
                char name[128] = {0};
                char size[128] = {0};

                for(int j = 0; buff[i] != 0; i++, j++)
                {
                    name[j] = buff[i];
                }

                i++;

                for(int j = 0; buff[i] != 0; i++, j++)
                {
                    size[j] = buff[i];
                }

                fileName  = QString::fromLocal8Bit(name);
                fileSize  = QString(size).toULongLong();
                fileCount = 0;

                file->setFileName(filePath + fileName);

                if(file->open(QFile::WriteOnly) == true)
                {
                    YmodemFileReceive::status = YmodemStatusEstablish;

                    receiveStatus(YmodemStatusEstablish);

                    return YmodemCodeAck;
                }
                else
                {
                    YmodemFileReceive::status = YmodemStatusError;

                    writeTimer->start(WRITE_TIME_OUT);

                    return YmodemCodeCan;
                }
            }
            else
            {
                YmodemFileReceive::status = YmodemStatusError;

                writeTimer->start(WRITE_TIME_OUT);

                return YmodemCodeCan;
            }
        }

        case YmodemStatusTransmit:
        {
            if((fileSize - fileCount) > *len)
            {
                file->write((char *)buff, *len);

                fileCount += *len;
            }
            else
            {
                file->write((char *)buff, fileSize - fileCount);

                fileCount += fileSize - fileCount;
            }

            progress = (int)(fileCount * 100 / fileSize);

            YmodemFileReceive::status = YmodemStatusTransmit;

            receiveProgress(progress);
            receiveStatus(YmodemStatusTransmit);

            return YmodemCodeAck;
        }

        case YmodemStatusFinish:
        {
            file->close();

            YmodemFileReceive::status = YmodemStatusFinish;

            writeTimer->start(WRITE_TIME_OUT);

            return YmodemCodeAck;
        }

        case YmodemStatusAbort:
        {
            file->close();

            YmodemFileReceive::status = YmodemStatusAbort;

            writeTimer->start(WRITE_TIME_OUT);

            return YmodemCodeCan;
        }

        case YmodemStatusTimeout:
        {
            YmodemFileReceive::status = YmodemStatusTimeout;

            writeTimer->start(WRITE_TIME_OUT);

            return YmodemCodeCan;
        }

        default:
        {
            file->close();

            YmodemFileReceive::status = YmodemStatusError;

            writeTimer->start(WRITE_TIME_OUT);

            return YmodemCodeCan;
        }
    }
}

uint32_t YmodemFileReceive::read(uint8_t *buff, uint32_t len)
{
    return serialPort->read((char *)buff, len);
}

uint32_t YmodemFileReceive::write(uint8_t *buff, uint32_t len)
{
    return serialPort->write((char *)buff, len);
}
