#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    thread = new workThread;

    connect(thread,&workThread::disp_msg,this,[=](QString str){
        ui->textEdit->insertPlainText(str);
    });

    connect(thread,&workThread::disp_btn,this,[=](){
        ui->pushButton->setEnabled(1);
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    ui->textEdit->clear();

    thread->start();
    ui->pushButton->setEnabled(0);
}


void MainWindow::on_pushButton_2_clicked()
{
    QMessageBox::information(this, "使用方法 Usage",
                                    "1. 本工具用于将现有图片处理为RGB565带抖动的图片\n"
                                    "This tool is used to convert existing images into RGB565 images with dithering.\n"
                                    "2. 处理后的图片将放置于同目录下.output文件夹内\n"
                                    "The processed images will be placed in the .output folder in the same directory.\n"
                                    "3. 这样的处理只适用于渐变的场景，纯色部分需要手动修复\n"
                                    "This kind of processing is only suitable for gradient scenes, and solid color parts need to be manually repaired.",
                                    QMessageBox::Ok);
}


void MainWindow::on_pushButton_3_clicked()
{
    QString path = QFileDialog::getExistingDirectory(
                    this,
                    "Select Folder",
                    QDir::currentPath());

    if(path.isEmpty())path = QDir::currentPath();

    thread->setDir(path);
    ui->textEdit->insertPlainText("\nPath:"+path+"\n");
}


void MainWindow::on_checkBox_stateChanged(int arg1)
{
    Q_UNUSED(arg1)

    if(ui->checkBox->isChecked()){
        thread->setAlpha(true);
    }
    else{
        thread->setAlpha(false);
    }
}


void MainWindow::on_comboBox_activated(const QString &arg1)
{
    qDebug()<<arg1;
    thread->changeFileType(arg1);
    if(arg1 == ".bmp"){
        ui->checkBox->setCheckState(Qt::Unchecked);
        ui->checkBox->setEnabled(0);
    }
    else{
        ui->checkBox->setEnabled(1);
    }
}


void workThread::exit(){

}

void workThread::run(){

    if(path.isEmpty())path = QDir::currentPath();

    emit disp_msg(path+"\n");
    QDir dir(path);
    QStringList filename ;
    filename << "*.png" << "*.jpg" << "*.bmp" << "*.jpeg" << "*.gif";
    QStringList results;
    results = dir.entryList(filename,QDir::Files | QDir::Readable,QDir::Name);

    emit disp_msg("Search for images.\n\n");

    QDir dir2(path+"/.output/");
    if(dir2.exists())
    {
        emit disp_msg("The previously outputted file has been deleted.\n");
        dir2.removeRecursively();
    }
    bool result = dir2.mkdir(path+"/.output");
    if(!result){
        emit disp_msg("Folder creation failed.\n");
        emit disp_btn();
        return;
    }

    foreach(QString a,results){
        emit disp_msg(a+"...");
        Dither1(a);
    }

    if(results.isEmpty()){
        emit disp_msg("No image");
    }

    emit disp_msg("\n\nCompleted ! ");

    emit disp_btn();
}


void workThread::setDir(QString Dir){
    path = Dir;
}

void workThread::setAlpha(bool alpha){
    alpha_enable = alpha;
}



/* Dither Tresshold for Red Channel */
static const int dither_tresshold_r[64] = {
  1, 7, 3, 5, 0, 8, 2, 6,
  7, 1, 5, 3, 8, 0, 6, 2,
  3, 5, 0, 8, 2, 6, 1, 7,
  5, 3, 8, 0, 6, 2, 7, 1,

  0, 8, 2, 6, 1, 7, 3, 5,
  8, 0, 6, 2, 7, 1, 5, 3,
  2, 6, 1, 7, 3, 5, 0, 8,
  6, 2, 7, 1, 5, 3, 8, 0
};

/* Dither Tresshold for Green Channel */
static const int dither_tresshold_g[64] = {
  1, 3, 2, 2, 3, 1, 2, 2,
  2, 2, 0, 4, 2, 2, 4, 0,
  3, 1, 2, 2, 1, 3, 2, 2,
  2, 2, 4, 0, 2, 2, 0, 4,

  1, 3, 2, 2, 3, 1, 2, 2,
  2, 2, 0, 4, 2, 2, 4, 0,
  3, 1, 2, 2, 1, 3, 2, 2,
  2, 2, 4, 0, 2, 2, 0, 4
};

/* Dither Tresshold for Blue Channel */
static const int dither_tresshold_b[64] = {
  5, 3, 8, 0, 6, 2, 7, 1,
  3, 5, 0, 8, 2, 6, 1, 7,
  8, 0, 6, 2, 7, 1, 5, 3,
  0, 8, 2, 6, 1, 7, 3, 5,

  6, 2, 7, 1, 5, 3, 8, 0,
  2, 6, 1, 7, 3, 5, 0, 8,
  7, 1, 5, 3, 8, 0, 6, 2,
  1, 7, 3, 5, 0, 8, 2, 6
};

/* Get 16bit closest color */
quint8 workThread::closest_rb(quint8 c) {
  return quint8(c >> 3 << 3); /* red & blue */
}
quint8 workThread::closest_g(quint8 c) {
  return quint8(c >> 2 << 2); /* green */
}

int workThread::MIN(int a,int b){
    if(b<a)return b;
    return a;
}

/* Dithering by individual subpixel */
QColor workThread::dither_xy(
  quint32 x,
  quint32 y,
  QColor color
){
    int r = color.red();
    int g = color.green();
    int b = color.blue();

    /* Get Tresshold Index */
    quint8 tresshold_id = quint8((y & 7) << 3) + (x & 7);

    r = closest_rb(
        quint8(MIN(r + dither_tresshold_r[tresshold_id], 0xff))
    );
    g = closest_g(
        quint8(MIN(g + dither_tresshold_g[tresshold_id], 0xff))
    );
    b = closest_rb(
        quint8(MIN(b + dither_tresshold_b[tresshold_id], 0xff))
    );

    color.setRgb(r&0xF8,g&0xFC,b&0xF8,color.alpha());
    return color;
}

void workThread::Dither1(QString file_name){
    QImage *image = new QImage;

    qDebug()<<path+file_name<<image->load(path+'/'+file_name);

    if(image->isNull()){
        emit disp_msg("打开失败，检查后缀是否与文件类型相符\nFailed to open the file, please check if the file extension matches its type.\n");
        delete image;
        return;
    }

    if(alpha_enable){
        *image = image->convertToFormat(QImage::Format_ARGB32);
    }
    else{
        QImage resultImg(image->size(), QImage::Format_ARGB32);
        resultImg.fill(Qt::white);

        QPainter painter(&resultImg);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, *image);
        painter.end();
        *image = resultImg.convertToFormat(QImage::Format_RGB32);
    }

    int x, y;
    for (y=0; y<image->height(); y++){
        for (x=0; x<image->width(); x++){
            QColor color = image->pixelColor(x,y);
            color = dither_xy(quint32(x),quint32(y),color);
            image->setPixelColor(x,y,color);
        }
    }

    int lastPoint = file_name.lastIndexOf(".");
    file_name = file_name.left(lastPoint);
    //获得短名

    qDebug()<<path+"/.output/"+file_name+fileType<<image->save(path+"/.output/"+file_name+fileType);
    emit disp_msg("OK\n");
    delete image;
}


void workThread::changeFileType(QString type){
    fileType = type;
}

