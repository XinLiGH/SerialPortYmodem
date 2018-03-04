#ifndef YMODEMFILERECEIVE_H
#define YMODEMFILERECEIVE_H

#include <QFile>
#include <QTimer>
#include <QObject>
#include <QSerialPort>
#include "Ymodem.h"

class YmodemFileReceive : public QObject, public Ymodem
{
    Q_OBJECT

public:
    explicit YmodemFileReceive(QObject *parent = 0);
    ~YmodemFileReceive();

    void setFilePath(const QString &path);

    void setPortName(const QString &name);
    void setPortBaudRate(qint32 baudrate);

    bool startReceive();
    void stopReceive();

    int getReceiveProgress();
    YmodemStatus getReceiveStatus();

signals:
    void receiveProgress(int progress);
    void receiveStatus(YmodemStatus status);

private slots:
    void readTimeOut();
    void writeTimeOut();

private:
    YmodemCode callback(YmodemStatus status, uint8_t *buff, uint32_t *len);

    uint32_t read(uint8_t *buff, uint32_t len);
    uint32_t write(uint8_t *buff, uint32_t len);

    QFile       *file;
    QTimer      *readTimer;
    QTimer      *writeTimer;
    QSerialPort *serialPort;

    int          progress;
    YmodemStatus status;
    QString      filePath;
    QString      fileName;
    uint64_t     fileSize;
    uint64_t     fileCount;
};

#endif // YMODEMFILERECEIVE_H
