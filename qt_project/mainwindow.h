#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QDebug>
#include <QColor>
#include <QBitmap>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>
#include <QFileDialog>
#include <QPainter>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class workThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    workThread *thread;

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_checkBox_stateChanged(int arg1);

    void on_comboBox_activated(const QString &arg1);

private:
    Ui::MainWindow *ui;
};



class workThread : public QThread
{
    Q_OBJECT
public:
    void run();
    void exit();
    QColor dither_xy(
    quint32 x,
    quint32 y,
    QColor color
    );
    void Dither1(QString file_name);
    quint8 closest_rb(quint8 c);
    quint8 closest_g(quint8 c);
    int MIN(int a,int b);
    bool alpha_enable = false;

    QString path;
    QString fileType = ".png";
    void setDir(QString Dir);
    void setAlpha(bool alpha);
    void changeFileType(QString type);


signals:
    void disp_msg(QString str);
    void disp_btn();
};




#endif // MAINWINDOW_H
