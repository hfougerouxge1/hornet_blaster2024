#include "mainwindow.h"
#include <QtSerialPort/QSerialPort>
#include <QApplication>
#include <QCoreApplication>
#include <QSerialPortInfo>
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QTimer>
#include <QLabel>
#include <QDir>
#include <QRandomGenerator>

class MouseTracker : public QWidget
{
public:
    MouseTracker(QWidget *parent = nullptr) : QWidget(parent), serialPort(nullptr)
    {
        setMouseTracking(true);
        setFixedSize(553, 463 );
        serialPort = new QSerialPort(this);
        connect(serialPort, &QSerialPort::errorOccurred, this, &MouseTracker::handleError);

        imageRemovalTimer = new QTimer(this);
        connect(imageRemovalTimer, &QTimer::timeout, this, &MouseTracker::removeOldestImage);
        imageRemovalTimer->start(3000);

        // Find highest COM port available
        QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
        QString highestPort;
        for (const QSerialPortInfo &portInfo : ports) {
            QString portName = portInfo.portName();
            if (portName.startsWith("COM")) {
                if (highestPort.isEmpty() || portName.mid(3).toInt() > highestPort.mid(3).toInt())
                    highestPort = portName;
            }
        }
        if (!highestPort.isEmpty()) {
            serialPort->setPortName(highestPort);
            serialPort->setBaudRate(QSerialPort::Baud115200); // You may adjust the baud rate as per your requirements
            if (serialPort->open(QIODevice::WriteOnly))
                qDebug() << "Serial port opened: " << highestPort;
            else
                qDebug() << "Failed to open serial port: " << highestPort;
        } else {
            qDebug() << "No COM port available";
        }

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MouseTracker::sendYData);
        QTimer *imageTimer = new QTimer(this);
        connect(imageTimer, &QTimer::timeout, this, &MouseTracker::displayRandomImage);
        imageTimer->start(3000);

        imageRemovalTimer->start(3000);

    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
            QPoint pos = event->pos();
            x_data = pos.x();
            y_data = pos.y();
            qDebug() << "Mouse clicked at position: " << pos;
            sendXData();
        }
        for (int i = 0; i < imageRects.size(); ++i) {
            if (imageRects[i].contains(event->pos())) {
                // Supprimez le rectangle de la liste
                imageRects.remove(i);
                // Mettez à jour la fenêtre pour refléter les changements
                update();
                return; // Sortez de la boucle car vous avez trouvé l'image cliquée
            }
        }

    }
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        QPixmap backgroundImage("C:/travail/Serine_belhadj/dossier_sae/sae_Hornet_Blaster-2024/image_Frelon/jardin.jpg");
        painter.drawPixmap(rect(), backgroundImage, backgroundImage.rect());

        // Dessiner les images
        for (const QRect &imageRect : imageRects) {
            QPixmap image("C:/travail/Serine_belhadj/dossier_sae/sae_Hornet_Blaster-2024/image_Frelon/frelon2.png");
            painter.drawPixmap(imageRect, image);
        }
    }


private slots:
    void handleError(QSerialPort::SerialPortError error)
    {
        if (error != QSerialPort::NoError)
            qDebug() << "Serial port error: " << serialPort->errorString();
    }

    void sendXData()
    {
        QString data = QString::number(x_data) + "\n";
        if (serialPort && serialPort->isOpen()) {
            serialPort->write(data.toUtf8());
            serialPort->waitForBytesWritten(100); // Wait for data to be sent
            qDebug() << "Sent X coordinate: " << x_data;
            timer->start(200); // Start timer to send Y data after 200 ms
        } else {
            qDebug() << "Serial port not open";
        }
    }

    void sendYData()
    {
        QString data = QString::number(y_data) + "\n";
        if (serialPort && serialPort->isOpen()) {
            serialPort->write(data.toUtf8());
            serialPort->waitForBytesWritten(100); // Wait for data to be sent
            qDebug() << "Sent Y coordinate: " << y_data;
            timer->stop(); // Stop timer after sending Y data
        } else {
            qDebug() << "Serial port not open";
        }
    }
    void displayRandomImage()
    {
        // Générer une position aléatoire pour l'image
        int x = QRandomGenerator::global()->bounded(width() - 100); // Largeur de la fenêtre - largeur de l'image
        int y = QRandomGenerator::global()->bounded(height() - 100); // Hauteur de la fenêtre - hauteur de l'image

        // Charger l'image à partir du fichier sur le disque
        QPixmap image("C:/travail/Serine_belhadj/dossier_sae/sae_Hornet_Blaster-2024/image_Frelon/frelon2.png");

        if (x >= 450 || y >= 350) {
            // Régler les coordonnées à l'intérieur de la limite
            x = QRandomGenerator::global()->bounded(400 - 100); // Limite de largeur - largeur de l'image
            y = QRandomGenerator::global()->bounded(300 - 100); // Limite de hauteur - hauteur de l'image
        }
        // Vérifier si le chargement de l'image a réussi
        if (image.isNull()) {
            qDebug() << "Erreur : Impossible de charger l'image.";
            return;
        }


        imageIds.append(imageIdCounter);
        imageIdCounter++;

        imageRects.insert(imageIdCounter, QRect(x, y, 80, 80));

        // Mettre à jour la fenêtre
        update();
    }
    void removeOldestImage()
    {
        if (!imageIds.isEmpty()) {
            // Get the ID of the oldest image
            int oldestImageId = imageIds.takeFirst();

            // Remove the image rectangle associated with the oldest ID
            imageRects.remove(oldestImageId);

            // Update the window
            update();
        }
    }


private:
    QSerialPort *serialPort; // Serial port object
    int x_data; // Variable to store X coordinate
    int y_data; // Variable to store Y coordinate
    QTimer *timer; // Timer for sending Y data after X data
    QTimer *imageRemovalTimer; // Timer for removing images
    QMap<int, QRect> imageRects; // Map to store image rectangles with associated IDs
    QList<int> imageIds; // List to store IDs of displayed images
    int imageIdCounter = 0;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MouseTracker tracker;
    tracker.setCursor(Qt::CrossCursor);
    tracker.show();
    return app.exec();
}

