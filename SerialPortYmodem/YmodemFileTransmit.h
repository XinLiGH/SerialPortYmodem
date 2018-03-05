#ifndef YMODEMFILETRANSMIT_H
#define YMODEMFILETRANSMIT_H

#include <QFile>
#include <QTimer>
#include <QObject>
#include <QSerialPort>
#include "Ymodem.h"

class YmodemFileTransmit : public QObject, public Ymodem
{
    Q_OBJECT

public:
    explicit YmodemFileTransmit(QObject *parent = 0);
    ~YmodemFileTransmit();

    void setFileName(const QString &name);

    void setPortName(const QString &name);
    void setPortBaudRate(qint32 baudrate);

    bool startTransmit();
    void stopTransmit();

    int getTransmitProgress();
    Status getTransmitStatus();

signals:
    void transmitProgress(int progress);
    void transmitStatus(YmodemFileTransmit::Status status);

private slots:
    void readTimeOut();
    void writeTimeOut();

private:
    Code callback(Status status, uint8_t *buff, uint32_t *len);

    uint32_t read(uint8_t *buff, uint32_t len);
    uint32_t write(uint8_t *buff, uint32_t len);

    QFile       *file;
    QTimer      *readTimer;
    QTimer      *writeTimer;
    QSerialPort *serialPort;

    int      progress;
    Status   status;
    uint64_t fileSize;
    uint64_t fileCount;
};

#endif // YMODEMFILETRANSMIT_H
