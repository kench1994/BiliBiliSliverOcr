#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDir>
#include <QVector>
#include <QTime>

#define MAX(x,y) ((x>y)?x:y)
#define MIN(x,y,z) (((x>y?y:x)>z)?z:(x>y?y:x))

QTime time;
/* 求解最大公共子序列,用于字符串相似度比较
 * 2018-02-16,发现如果只使用该算法将存在bug,比如4,+,-这几个符号与-相比较所得相似度一样
 * 蛤铪^_^
*/
int LongestCommonSubsequence(const QString& strFirst, const  QString& strSecond) {
    int nLenOfFirstStr = strFirst.length();    int nLenOfSecondStr = strSecond.length();
    QVector<QVector<int>> matrix(nLenOfFirstStr+1, QVector<int>(nLenOfSecondStr+1));
    for (int i = 0; i <= nLenOfFirstStr; i++) {
        for( int j = 0; j <= nLenOfSecondStr; j++) {
            if(i == 0 || j == 0) {
                matrix[i][j] = 0;
            } else if (strFirst.at(i-1) == strSecond.at(j-1)) {
                matrix[i][j] = matrix[i-1][j-1] + 1;
            } else {
                matrix[i][j] = MAX(matrix[i-1][j], matrix[i][j-1]);
            }
        }
    }
    return matrix[nLenOfFirstStr][nLenOfSecondStr];
}

int LevenshteinDistance(const QString& strFirst, const QString& strSecond){
    int n = strFirst.length();
    int m = strSecond.length();
    if (n == 0)
       return n;
    if (m == 0)
       return m;
    int above,left,diag,cost;
    QChar si,dj;
    QVector<QVector<int>> matrix(n+1, QVector<int>(m+1));
    for (int i = 1; i <= n; i++){
        si = strFirst[i - 1];
        for (int j = 1; j <= m; j++){
            dj = strSecond[j - 1];
            if (si == dj) cost = 0;
            else cost = 1;
            above  = matrix[i - 1][j] + 1;
            left = matrix[i][j - 1] + 1;
            diag = matrix[i - 1][j - 1] + cost;
            matrix[i][j] = MIN(above, left, diag);
        }
    }
    return matrix[n][m];
}

float SimilarTextRate(const QString &strFirst, const QString &strSecond)
{
    int lcs = LongestCommonSubsequence(strFirst, strSecond);
    int ld = LevenshteinDistance(strFirst, strSecond);
    return (float)lcs/(float)(ld+lcs);
}

//生成字模
inline QString GenCharacterMatrix(const QString &strFilePath){
    QString strCharacterMatrix = "";
    QImage img(strFilePath);
    for(int i = 0; i < img.height(); i++)
        for(int j = 0; j < img.width(); j++){
            strCharacterMatrix += QString::number((!img.pixelColor(j,i).value()) ? 0 :1,10);
        }
    //qDebug()<<strCharacterMatrix;
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
        //qDebug()<<rect;
        listWorldEdge.push_back(rect);
        img->copy(rect).save(QString("d:/"+QString::number(nSliceCount,10)+".png"));
        if(nTryTimes==0) break;//search end
        nSliceCount++;
    }
    return nSliceCount;
}


int HistogramHorizon(QImage* binImg, QVector<int>& hist){
    int nHigest = 0;
    for (int i=0;i!=binImg->width();i++) {
        for (int j=0;j!=binImg->height();j++){
            if(!binImg->pixelColor(i,j).value()){
                hist[i]++;
            }
        }
        if(hist[i] > nHigest)
            nHigest = hist[i];
        //qDebug()<<hist[i];
    }
    return nHigest;
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    ui->textEdit->setEnabled(false);
    QStringList listCharacterMatrix;
    for(int i = 0; i <= 11 ;i++){
        //读取字模加载到listCharacterMatrix
        listCharacterMatrix<<GenCharacterMatrix("../bili2SliverOcr/CharacterMatrix/"+QString::number(i,10)+".png");
    }

    QImage oriImg("../bili2SliverOcr/getCaptcha.jpg");
    //ui->originalImg->show();
    QImage binaryImage(oriImg.size(), QImage::Format_ARGB32);
    QGraphicsScene* sceneOriImg;
    time.start();
    BinProcessing(&oriImg,&binaryImage);
    //显示二值化后的图片
    sceneOriImg = new QGraphicsScene();
    sceneOriImg->addPixmap(QPixmap::fromImage(binaryImage));
    ui->originalImg->setScene(sceneOriImg);
    ui->originalImg->show();


    QVector<int> vectHist(binaryImage.width(),0);
    int nHigest = HistogramHorizon(&binaryImage,vectHist);
    QImage imgHist(binaryImage.width(),nHigest,QImage::Format_ARGB32);
    for(int x=0;x<vectHist.length();x++){
        for(int y=0;y<vectHist[x];y++)
            imgHist.setPixel(x,y,qRgb(0, 0, 0));
    }
    sceneOriImg = new QGraphicsScene();
    sceneOriImg->addPixmap(QPixmap::fromImage(imgHist));
    ui->histogramVertical->setScene(sceneOriImg);
    ui->histogramVertical->show();




    //提取图像边缘,并保存到list
    QList<QRect> listWorldEdge;
    int nSliceCount = EdgeExtracting(&binaryImage,listWorldEdge);
    //qDebug()<<"SliceCount:"<<nSliceCount;
    QHash<int,int> hashSimilarity;
    int nSignPos = 0;
    for(int i=0;i<nSliceCount;i++){
        int nMaxSimilarity = 0;
        //读取切片数据
        QString strCompare = "";
        for(int q = listWorldEdge.at(i).y();q<listWorldEdge.at(i).height()+listWorldEdge.at(i).y();q++)
            for(int p=listWorldEdge.at(i).x();p<listWorldEdge.at(i).width()+listWorldEdge.at(i).x();p++)
                strCompare+=(!binaryImage.pixelColor(p,q).value())?"0":"1";


        //相似度比较
        for(int j=0; j<listCharacterMatrix.length(); j++){
            float nTempSimilarity = SimilarTextRate(listCharacterMatrix.at(j),strCompare);
            //qDebug()<< j << "->"<<nTempSimilarity;
            if(nTempSimilarity>nMaxSimilarity) {
                nMaxSimilarity=nTempSimilarity;
                hashSimilarity[i] = j;
                if(j>9) nSignPos = i;//符号位
            }
        }
    }
    //qDebug()<<nSignPos;
    int res1 = 0,res2 = 0,res = 0;
    for(int i = 0;i<nSignPos;i++){
        res1*=10;
        res1+=hashSimilarity[i];
    }
    qDebug()<<"First num:"<<res1;

    for(int i = nSignPos+1;i<nSliceCount;i++){
        res2*=10;
        res2+=hashSimilarity[i];
    }
    qDebug()<<"Second num:"<<res2;

    if(hashSimilarity[nSignPos]==10)
        res = res1+res2;
    else if(hashSimilarity[nSignPos]==11)
        res = res1-res2;
    else
        qDebug()<<"none find sign symbol";
    qDebug()<<time.elapsed()/1000.0<<"s";
    ui->textEdit->setText(QString::number(res,10));

}

MainWindow::~MainWindow()
{
    delete ui;
}
