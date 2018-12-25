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
    setTimeDivide(499);
    setTimeMax(5);
    setErrorMax(999);

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
    status   = StatusEstablish;

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
    abort();
    status = StatusAbort;
    writeTimer->start(WRITE_TIME_OUT);
}

int YmodemFileReceive::getReceiveProgress()
{
    return progress;
}

Ymodem::Status YmodemFileReceive::getReceiveStatus()
{
    return status;
}

void YmodemFileReceive::readTimeOut()
{
    readTimer->stop();

    receive();

    if((status == StatusEstablish) || (status == StatusTransmit))
    {
        readTimer->start(READ_TIME_OUT);
    }
}

void YmodemFileReceive::writeTimeOut()
{
    writeTimer->stop();
    serialPort->close();
    receiveStatus(status);
}

Ymodem::Code YmodemFileReceive::callback(Status status, uint8_t *buff, uint32_t *len)
{
    switch(status)
    {
        case StatusEstablish:
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
                QString file_desc(size);
                QString sizeStr = file_desc.left(file_desc.indexOf(' '));
                fileSize  = sizeStr.toULongLong();
                fileCount = 0;

                file->setFileName(filePath + fileName);

                if(file->open(QFile::WriteOnly) == true)
                {
                    YmodemFileReceive::status = StatusEstablish;

                    receiveStatus(StatusEstablish);

                    return CodeAck;
                }
                else
                {
                    YmodemFileReceive::status = StatusError;

                    writeTimer->start(WRITE_TIME_OUT);

                    return CodeCan;
                }
            }
            else
            {
                YmodemFileReceive::status = StatusError;

                writeTimer->start(WRITE_TIME_OUT);

                return CodeCan;
            }
        }

        case StatusTransmit:
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

            YmodemFileReceive::status = StatusTransmit;

            receiveProgress(progress);
            receiveStatus(StatusTransmit);

            return CodeAck;
        }

        case StatusFinish:
        {
            file->close();

            YmodemFileReceive::status = StatusFinish;

            writeTimer->start(WRITE_TIME_OUT);

            return CodeAck;
        }

        case StatusAbort:
        {
            file->close();

            YmodemFileReceive::status = StatusAbort;

            writeTimer->start(WRITE_TIME_OUT);

            return CodeCan;
        }

        case StatusTimeout:
        {
            YmodemFileReceive::status = StatusTimeout;

            writeTimer->start(WRITE_TIME_OUT);

            return CodeCan;
        }

        default:
        {
            file->close();

            YmodemFileReceive::status = StatusError;

            writeTimer->start(WRITE_TIME_OUT);

            return CodeCan;
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
