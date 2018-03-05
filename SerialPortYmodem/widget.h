#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "YmodemFileTransmit.h"
#include "YmodemFileReceive.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_comButton_clicked();
    void on_transmitBrowse_clicked();
    void on_receiveBrowse_clicked();
    void on_transmitButton_clicked();
    void on_receiveButton_clicked();
    void transmitProgress(int progress);
    void receiveProgress(int progress);
    void transmitStatus(YmodemFileTransmit::Status status);
    void receiveStatus(YmodemFileReceive::Status status);

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;
    YmodemFileTransmit *ymodemFileTransmit;
    YmodemFileReceive *ymodemFileReceive;

    bool transmitButtonStatus;
    bool receiveButtonStatus;
};

#endif // WIDGET_H
