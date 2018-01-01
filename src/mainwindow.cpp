#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#define MAX(x,y) ((x>y)?x:y)
#define MIN(x,y,z) (((x>y?y:x)>z)?z:(x>y?y:x))


//求解最大公共子序列,用于字符串相似度比较
int LCS(QString str1, QString str2) {
    int len1 = str1.length();    int len2 = str2.length();
    int c[len1+1][len2+1];
    for (int i = 0; i <= len1; i++) {
        for( int j = 0; j <= len2; j++) {
            if(i == 0 || j == 0) {
                c[i][j] = 0;
            } else if (str1.at(i-1) == str2.at(j-1)) {
                c[i][j] = c[i-1][j-1] + 1;
            } else {
                c[i][j] = MAX(c[i-1][j], c[i][j-1]);
            }
        }
    }
    return c[len1][len2];
}
//生成字模
inline QString GenCharacterMatrix(const QString &strFilePath){
    QString strCharacterMatrix = "";
    QImage img(strFilePath);
    for(int i = 0; i < img.height(); i++)
        for(int j = 0; j < img.width(); j++){
            strCharacterMatrix += QString::number((!img.pixelColor(j,i).value()) ? 0 :1,10);
        }
    return strCharacterMatrix;
}
//二值化处理
inline void BinProcessing(QImage* oriImg,QImage *binaryImage){
    for(int i = 0; i < oriImg->width(); i++)
        for(int j = 0; j < oriImg->height(); j++){
            QRgb pixel = oriImg->pixel(i, j);
            //通用灰度计算公式
            int r = qRed(pixel) * 0.3; int g = qGreen(pixel) * 0.59; int b = qBlue(pixel) * 0.11;
            int rgb = r + g + b;//先把图像灰度化，转化为灰度图像
            if(rgb > 220) rgb = 255;
            else  rgb = 0;
            binaryImage->setPixel(i, j, qRgb(rgb, rgb, rgb));
        }
}
//边缘提取,普适性不强
inline int EdgeExtracting(const QImage* img,QList<QRect> &listWorldEdge){
    int nRightEdge=0,nLeftEdge=0,max=0,min=0;
    int nTryTimes,nSliceCount =0;
    while(true){
        nTryTimes = 0;
        for(int j=0;j<img->height();j++){
            for(int i=nRightEdge;i<img->width()-1;i++){//左白右黑
                if(img->pixelColor(i,j).value() && !img->pixelColor(i+1,j).value()){
                    nLeftEdge = i+1;
                    if(min == nRightEdge) min = nLeftEdge;
                    if(nLeftEdge<min) min = nLeftEdge;
                    break;
                }
            }
        }
        nLeftEdge = min;

        for(int j=0;j<img->height();j++){
            for(int i=nLeftEdge;i<img->width()-1;/*leftEdge+16*;*/i++){//i<oriImg->width()-1======>>这边针对性太强
                if((i-nLeftEdge)>16) break;//直接做长度判断而没有做了连续性判断，意外可以？
                if(!img->pixelColor(i,j).value() && img->pixelColor(i+1,j).value()){ //左黑右白
                    nRightEdge = i+1;
                    if(nRightEdge>max) max=nRightEdge;
                    nTryTimes++;
                    break;
                }
            }
        }
        nRightEdge = max;
        min = nRightEdge;
        //这边直接做了图像切割，整体来看算法针对性太强，扩展性及性能都略差，亟待改进
        QRect rect(nLeftEdge,4,nRightEdge-nLeftEdge,31);
        listWorldEdge.push_back(rect);
        //img->copy(rect).save(QString("d:/"+QString::number(nSliceCount,10)+".png"));
        if(nTryTimes==0) break;//search end
        nSliceCount++;
    }
    return nSliceCount;
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    ui->textEdit->setEnabled(false);
    QStringList listCharacterMatrix;
    for(int i = 0; i < 11 ;i++){
        //读取字模加载到listCharacterMatrix
        listCharacterMatrix<<GenCharacterMatrix("../bili2SliverOcr/CharacterMatrix/"+QString::number(i,10)+".png");
    }


    QImage oriImg("D:/getCaptcha.jpg");
    ui->originalImg->show();
    QImage binaryImage(oriImg.size(), QImage::Format_RGB32);
    BinProcessing(&oriImg,&binaryImage);

    //显示二值化后的图片
    QGraphicsScene* sceneOriImg = new QGraphicsScene();
    sceneOriImg->addPixmap(QPixmap::fromImage(binaryImage));
    ui->originalImg->setScene(sceneOriImg);
    ui->originalImg->show();

    //提取图像边缘,并保存到list
    QList<QRect> listWorldEdge;
    int nSliceCount = EdgeExtracting(&binaryImage,listWorldEdge);

    QHash<int,int> hashSimilarity;
    int nSignPos = 0;
    for(int i=0;i<nSliceCount;i++){
        int nMaxSimilarity = 0;
        //读取切片数据
        QString strCompare = "";
        for(int q = listWorldEdge.at(i).y();q<listWorldEdge.at(i).height()+listWorldEdge.at(i).y();q++)
            for(int p=listWorldEdge.at(i).x();p<listWorldEdge.at(i).width()+listWorldEdge.at(i).x();p++){
                strCompare+=(0==binaryImage.pixelColor(p,q).value())?"0":"1";
        }
        //相似度比较
        for(int j=0; j<listCharacterMatrix.length(); j++){
            int nTempSimilarity = LCS(listCharacterMatrix.at(j),strCompare);
            if(nTempSimilarity>nMaxSimilarity) {
                nMaxSimilarity=nTempSimilarity;
                hashSimilarity[i] = j;
                if(j>9) nSignPos = i;
            }
        }
    }
    int res1 = 0,res2 = 0,res = 0;
    for(int i = 0;i<nSignPos;i++){
        //int nDigit = nSignPos-i-1;
        //==0时说明当前为个位数
        //res1 += (!nDigit)?hashSimilarity[i]:hashSimilarity[i]*qPow(10,nDigit);
        res1*=10;
        res1+=hashSimilarity[i];
    }
    for(int i = nSignPos+1;i<nSliceCount;i++){
        res2*=10;
        res2+=hashSimilarity[i];
    }
    if(hashSimilarity[nSignPos]==10)
        res = res1+res2;
    else if(hashSimilarity[nSignPos]==11)
        res = res1-res2;
    else
        qDebug()<<"none find sign symbol";
    ui->textEdit->setText(QString::number(res,10));
}

MainWindow::~MainWindow()
{
    delete ui;
}
