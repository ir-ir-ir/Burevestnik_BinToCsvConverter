#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("BinToCsvConverter");

    ui->progressBar->setVisible(false);

    connect(ui->pushButton,SIGNAL(clicked()), this, SLOT(selectFileClicked()));
    connect(ui->pushButton_2,SIGNAL(clicked()), this, SLOT(selectOutputFileClicked()));
    connect(ui->pushButton_3,SIGNAL(clicked()), this, SLOT(convertClicked()));

    connect(ui->inputFileLine, &QLineEdit::editingFinished, this,[this](){onLineEditingFinished("input");});
    connect(ui->outputFileLine, &QLineEdit::editingFinished, this,[this](){onLineEditingFinished("output");});

    ui ->pushButton_3->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onLineEditingFinished(const QString lineType){

    if (lineType == "input"){
        QString path = ui ->inputFileLine->text().trimmed();
        if (path.isEmpty()) {
            ui->pushButton_3->setEnabled(false);
            return;
        }
        QFileInfo info(path);

        if (!info.exists() || !info.isFile()){
            QMessageBox::warning(this,
                          "Ошибка",
                          "Файл не найден");
            currentFilePath = "";
            ui->inputFileLine->clear();
            ui->outputFileLine->clear();
            ui -> pushButton_3->setEnabled(false);
            return;
        }
        QString ext = info.suffix().toLower();
        if (ext != "bin" && ext != "log" && ext != "txt"){
            QMessageBox::warning(this,
                          "Ошибка",
                          "Допустимые форматы: .bin .txt .log");
            currentFilePath = "";
            ui->inputFileLine->clear();
            ui->outputFileLine->clear();
            ui -> pushButton_3->setEnabled(false);
            return;
        }

        currentFilePath = path;

        QFileInfo info_(path);
        QString outputFilePathAvtomatic = info_.absolutePath()+ "/" + info_.completeBaseName()+ ".csv";
        ui->outputFileLine->setText(outputFilePathAvtomatic);
        outputFilePath = outputFilePathAvtomatic;
    }
    else if (lineType == "output"){
        QString path = ui ->outputFileLine->text().trimmed();
        if (path.isEmpty()) {
            ui->pushButton_3->setEnabled(false);
            return;
        }
        QFileInfo info(path);

        if (info.absolutePath().isEmpty() || !QDir(info.absolutePath()).exists()){
            QMessageBox::warning(this,
                          "Ошибка",
                          "Папка не найдена");
            outputFilePath = "";
            ui->inputFileLine->clear();
            ui->outputFileLine->clear();
            ui -> pushButton_3->setEnabled(false);
            return;
        }
        QString ext = info.suffix().toLower();
        if (ext != "csv"){
            QMessageBox::warning(this,
                          "Ошибка",
                          "Допустимый формат выходного файла: .csv");
            outputFilePath = "";
            ui->inputFileLine->clear();
            ui->outputFileLine->clear();
            ui -> pushButton_3->setEnabled(false);
            return;
        }

        outputFilePath = path;
    }

    bool enable = !ui->inputFileLine->text().isEmpty() && !ui->outputFileLine->text().isEmpty();
    ui -> pushButton_3->setEnabled(enable);
}

void MainWindow::selectFileClicked(){

    // сохранение последней директории
    QSettings settings("BinTools", "BinToCsvConverter");
    QString defaultPath = QCoreApplication::applicationDirPath();
    QString lastPath = settings.value("lastDirectory",defaultPath).toString();

    if (!QDir(lastPath).exists()){
        lastPath = defaultPath;
    }

    //диалог выбора файла
    QString filePath = QFileDialog::getOpenFileName(
            this,
            "Выберите бинарный файл",
            lastPath,
            "Бинарные файлы(*.bin *.txt *.log)"  // фильтр
            );

    if (filePath.isEmpty()){
        return;
    }

    QFile file(filePath);
    if (!file.exists()){
        QMessageBox::critical(this,"Ошибка", "Файл не найден\n");
        return;
    }

    if ((!filePath.endsWith(".bin", Qt::CaseInsensitive)) &&
            (!filePath.endsWith(".log", Qt::CaseInsensitive))&&
            (!filePath.endsWith(".txt", Qt::CaseInsensitive))) {
        QMessageBox::critical (this, "Ошибка", "Доступные расширения входного файла: .bin .log .txt\n");
        return;
    }

    //чтение
    if (!file.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл\n");
        return;
    }

    currentFilePath = filePath;

    //устанавливаем путь в ui
    ui->inputFileLine->setText(filePath);
    QFileInfo info(filePath);
    QString outputFilePathAvtomatic = info.absolutePath()+ "/" + info.completeBaseName()+ ".csv";
    ui->outputFileLine->setText(outputFilePathAvtomatic);
    outputFilePath = outputFilePathAvtomatic;

    if (!ui->inputFileLine->text().isEmpty() && !ui->outputFileLine->text().isEmpty()){
        ui -> pushButton_3->setEnabled(true);
    }

    //сохранение в реестр
    QFileInfo fileInfo(filePath);
    QString directory = fileInfo.absolutePath();
    settings.setValue("lastDirectory", directory);
}

void MainWindow::selectOutputFileClicked(){

    QString savePath = currentFilePath;
    savePath.chop(4);
    savePath += ".csv";
    //диалог выбора папки для сохранения
    QString actualSavePath = QFileDialog::getSaveFileName(
         this,
         "Создать csv",
          savePath,
          "CSV файлы (*.csv)"
         );
    if (actualSavePath.isEmpty()) return;

    outputFilePath = actualSavePath;

    QFile outFile(actualSavePath);
    if (!outFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл\n");
        return;
    }

    //устанавливаем путь в ui
    ui->outputFileLine->setText(actualSavePath);

    if (!ui->inputFileLine->text().isEmpty() && !ui->outputFileLine->text().isEmpty()){
        ui -> pushButton_3->setEnabled(true);
    }
}

void MainWindow::convertClicked(){

    if (currentFilePath == "" || outputFilePath == " ") return;

    ui ->pushButton_3->setEnabled(false);

    QFile file(currentFilePath);
    QFile outFile(outputFilePath);

    file.open(QIODevice::ReadOnly|QIODevice::Text);
    outFile.open(QIODevice::WriteOnly|QIODevice::Text);

    ui->progressBar->setVisible(true);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(file.size());
    ui->progressBar->setValue(0);

    // побайтовое чтение
    if (convertToCsvByByte(file,outFile) == 0) {

        ui->progressBar->setVisible(false);

        file.close();
        outFile.close();

        QMessageBox endBox;
        endBox.setWindowTitle("BinToCsvConverter");
        endBox.setInformativeText("Конвертация завершена");
        int result = endBox.exec();
        if (result != QMessageBox::Ok) {
            return;
        }

        ui->inputFileLine->clear();
        ui->outputFileLine->clear();
        ui ->pushButton_3->setEnabled(false);
    }
}

int MainWindow::convertToCsv (const QByteArray &data, QFile &outFile){

    QString csvHeader = "Time;ID;Format;Data;\n";
    outFile.write(csvHeader.toUtf8());

    int pos = 0; // текущая позиция в массиве байтов
    int msgIndex = 0; // номер сообщения
    int dataSize = data.size();

    while (pos + 20 <= dataSize) {

        ui->progressBar->setValue(pos);

        //заголовок
        quint32 header = (quint32)(unsigned char)data[pos] |
                ((quint32)(unsigned char)data[pos+1] << 8) |
                ((quint32)(unsigned char)data[pos+2] << 16)|
                ((quint32)(unsigned char)data[pos+3] << 24);
        if (header != 0x41414141){
            pos++;
            continue;
        }

        //тип сообщения
        quint8 flags = (quint8)data[pos+9];
        bool isEid = (flags & 0x01) != 0; // проверка 7 бита
        int msgSize = isEid ? 22:20; // размер сообщения в байтах

        if (pos + msgSize > dataSize) {
            break;
        }

        //время
        quint32 time = (quint32)(unsigned char)data[pos+5] |
                ((quint32)(unsigned char)data[pos+6] << 8) |
                ((quint32)(unsigned char)data[pos+7] << 16)|
                ((quint32)(unsigned char)data[pos+8] << 24);
        QString timeCorr = QTime::fromMSecsSinceStartOfDay(time).toString("hh:mm:ss.zzz");

        //флаг расширенного идентификатора
        QString ext = isEid ? "Ext":"Std";

        //идентификатор
        quint32 id = 0;

        if (!isEid){
            //sid: 5 бит в байте 9 (биты 3-7) + 6 бит в байте 10 (биты 7-2)
            id = ((flags >> 3) & 0x1F ) | (((data[pos+10]& 0x3F)) << 5);
        }
        else{
            //eid: 29 бит в байтах 9,10,11,12
            id = (quint32)(flags >> 3) & 0x1F |
                    ((quint32)(unsigned char)data[pos+10] << 5) |
                    ((quint32)(unsigned char)data[pos+11] << 13)|
                    ((quint32)(unsigned char)data[pos+12] << 21);
            id &= 0x1FFFFFFF;

        }

        //данные
        int dataOffset = isEid ? 13:11; // байт, где начинаются данные
        quint8 d0 = (quint8)data[pos + dataOffset];
        quint8 d1 = (quint8)data[pos + dataOffset + 1];
        quint8 d2 = (quint8)data[pos + dataOffset + 2];
        quint8 d3 = (quint8)data[pos + dataOffset + 3];
        quint8 d4 = (quint8)data[pos + dataOffset + 4];
        quint8 d5 = (quint8)data[pos + dataOffset + 5];
        quint8 d6 = (quint8)data[pos + dataOffset + 6];
        quint8 d7 = (quint8)data[pos + dataOffset + 7];

       QString allData = QString("%1 %2 %3 %4 %5 %6 %7 %8")
               .arg(d0,2,16,QChar('0'))
               .arg(d1,2,16,QChar('0'))
               .arg(d2,2,16,QChar('0'))
               .arg(d3,2,16,QChar('0'))
               .arg(d4,2,16,QChar('0'))
               .arg(d5,2,16,QChar('0'))
               .arg(d6,2,16,QChar('0'))
               .arg(d7,2,16,QChar('0')).toUpper();

        //добавление строки
        QString csv = QString ("\"\%1\"\;\"\%2\"\;\"\%3\"\;%4;\"\%5\"\;\n")
                .arg(timeCorr)
                .arg(QString::number(id,16))
                .arg(ext)
                .arg("\"\"")
                .arg(allData);

        //запись строки в файл
        outFile.write(csv.toUtf8());

        msgIndex ++;
        pos += msgSize;
    }

    return 0;
}

int MainWindow::convertToCsvByByte(QFile &inputFile,QFile &outFile){

   QString csvHeader = "Time;ID;Format;Data;\n";
   outFile.write(csvHeader.toUtf8());

   QByteArray buffer; // для накопления байтов
   char byte;
   int byteCounter = 0;

   inputFile.seek(0); // курсор в начало

   while (inputFile.getChar(&byte)){

       ui->progressBar->setValue(byteCounter);
       buffer.append(byte);
       byteCounter++;

       while (buffer.size() >= 20){
           // проверка заголовка

           quint32 header = (quint32)(unsigned char)buffer[0] |
                   ((quint32)(unsigned char)buffer[1] << 8) |
                   ((quint32)(unsigned char)buffer[2] << 16)|
                   ((quint32)(unsigned char)buffer[3] << 24);
           if (header != 0x41414141){
               buffer.remove(0,1); // удаление первого байта
               continue;
           }

           //тип сообщения
           quint8 flags = (quint8)buffer[9];
           bool isEid = (flags & 0x01) != 0; // проверка 7 бита
           int msgSize = isEid ? 22:20; // размер сообщения в байтах

           if (buffer.size() < msgSize) {
               break;
           }

           //время
           quint32 time = (quint32)(unsigned char)buffer[5] |
                   ((quint32)(unsigned char)buffer[6] << 8) |
                   ((quint32)(unsigned char)buffer[7] << 16)|
                   ((quint32)(unsigned char)buffer[8] << 24);
           QString timeCorr = QTime::fromMSecsSinceStartOfDay(time).toString("hh:mm:ss.zzz");

           //флаг расширенного идентификатора
           QString ext = isEid ? "Ext":"Std";

           //идентификатор
           quint32 id = 0;

           if (!isEid){
               //sid: 5 бит в байте 9 (биты 3-7) + 6 бит в байте 10 (биты 7-2)
               id = ((flags >> 3) & 0x1F ) | (((buffer[10]& 0x3F)) << 5);
           }
           else{
               //eid: 29 бит в байтах 9,10,11,12
               id = (quint32)(flags >> 3) & 0x1F |
                       ((quint32)(unsigned char)buffer[10] << 5) |
                       ((quint32)(unsigned char)buffer[11] << 13)|
                       ((quint32)(unsigned char)buffer[12] << 21);
               id &= 0x1FFFFFFF;

           }

           //данные
           int dataOffset = isEid ? 13:11; // байт, где начинаются данные
           quint8 d0 = (quint8)buffer[dataOffset];
           quint8 d1 = (quint8)buffer[dataOffset + 1];
           quint8 d2 = (quint8)buffer[dataOffset + 2];
           quint8 d3 = (quint8)buffer[dataOffset + 3];
           quint8 d4 = (quint8)buffer[dataOffset + 4];
           quint8 d5 = (quint8)buffer[dataOffset + 5];
           quint8 d6 = (quint8)buffer[dataOffset + 6];
           quint8 d7 = (quint8)buffer[dataOffset + 7];

          QString allData = QString("%1 %2 %3 %4 %5 %6 %7 %8")
                  .arg(d0,2,16,QChar('0'))
                  .arg(d1,2,16,QChar('0'))
                  .arg(d2,2,16,QChar('0'))
                  .arg(d3,2,16,QChar('0'))
                  .arg(d4,2,16,QChar('0'))
                  .arg(d5,2,16,QChar('0'))
                  .arg(d6,2,16,QChar('0'))
                  .arg(d7,2,16,QChar('0')).toUpper();

           //добавление строки
           QString csv = QString ("\"\%1\"\;\"\%2\"\;\"\%3\"\;%4;\"\%5\"\;\n")
                   .arg(timeCorr)
                   .arg(QString::number(id,16))
                   .arg(ext)
                   .arg("\"\"")
                   .arg(allData);

           //запись строки в файл
           outFile.write(csv.toUtf8());

           buffer.remove(0, msgSize);
       }
   }
   return 0;
}














