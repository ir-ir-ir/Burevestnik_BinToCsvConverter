#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void selectFileClicked();
    void selectOutputFileClicked();
    void convertClicked();

private:
    Ui::MainWindow *ui;
    QString currentFilePath; // путь к файлу
    QString outputFilePath; // путь к выходному файлу
    int convertToCsv (const QByteArray &data, QFile &outFile);//функция конвертации, чтение всего файла
    int convertToCsvByByte(QFile &inputFile, QFile &outFile); // функция конвертации, чтение по 1 байту
    void onLineEditingFinished(const QString lineType); //редактирование lineEdit

};

#endif // MAINWINDOW_H
