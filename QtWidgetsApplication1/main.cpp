#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QListView>
#include <QAbstractListModel>
#include <QTimer>
#include <QPainter>
#include <QRectF>
#include <QGraphicsItem>
#include <QRandomGenerator>
#include <QShortcut>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QtMath>
#include <QLabel>
#include <QSpinBox>
#include <QColorDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QColor>
#include <stdio.h>

class SettingsDialog : public QDialog {

public:
    SettingsDialog(QWidget* parent, QGraphicsView* radarView, QListView* listView)
        : QDialog(parent), radarView(radarView), listView(listView) {
        QVBoxLayout* layout = new QVBoxLayout(this);

        QPushButton* colorButton = new QPushButton("Change Background Color", this);
        connect(colorButton, &QPushButton::clicked, this, &SettingsDialog::changeBackgroundColor);
        layout->addWidget(colorButton);

        QLabel* widthLabel = new QLabel("Width:", this);
        QSpinBox* widthSpinBox = new QSpinBox(this);
        widthSpinBox->setRange(400, 2000);
        widthSpinBox->setValue(parent->width());

        QLabel* heightLabel = new QLabel("Height:", this);
        QSpinBox* heightSpinBox = new QSpinBox(this);
        heightSpinBox->setRange(300, 1500);
        heightSpinBox->setValue(parent->height());

        QPushButton* radarColorButton = new QPushButton("Change Radar Background Color", this);
        connect(radarColorButton, &QPushButton::clicked, this, &SettingsDialog::changeRadarBackgroundColor);
        
        QPushButton* listColorButton = new QPushButton("Change List Background Color", this);
        connect(listColorButton, &QPushButton::clicked, this, &SettingsDialog::changeListBackgroundColor);

        layout->addWidget(widthLabel);
        layout->addWidget(widthSpinBox);
        layout->addWidget(heightLabel);
        layout->addWidget(heightSpinBox);
        layout->addWidget(radarColorButton);
        layout->addWidget(listColorButton);

        connect(widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [parent](int newWidth) { parent->resize(newWidth, parent->height()); });
        connect(heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            [parent](int newHeight) { parent->resize(parent->width(), newHeight); });
    }

private slots:
    void changeBackgroundColor() {
        QColor color = QColorDialog::getColor(Qt::white, this, "Select Color");
        if (color.isValid()) {
            QPalette pal = palette();
            pal.setColor(QPalette::Window, color);
            parentWidget()->setPalette(pal);
            parentWidget()->update();
        }
    }

    void changeRadarBackgroundColor() {
        QColor color = QColorDialog::getColor(radarView->backgroundBrush().color(), this, "Select Radar Color");
        if (color.isValid()) {
            radarView->setBackgroundBrush(QBrush(color));
        }
    }

    void changeListBackgroundColor() {
        QColor color = QColorDialog::getColor(listView->palette().color(QPalette::Base), this, "Select List Color");
        if (color.isValid()) {
            QPalette pal = listView->palette();
            pal.setColor(QPalette::Base, color);
            listView->setPalette(pal);
        }
    }

private:
    QGraphicsView* radarView;
    QListView* listView;
};

class AirplaneItem : public QGraphicsItem {
public:
    AirplaneItem(int i, qreal sceneWidth, qreal sceneHeight) {
        xPos = QRandomGenerator::global()->bounded(sceneWidth);
        yPos = QRandomGenerator::global()->bounded(sceneHeight);

        angle = QRandomGenerator::global()->bounded(360);
        speed = 2;

        flightName = "Airplain";
        flightName.append(QString::number(i));
    }

    QRectF boundingRect() const override {
        return QRectF(-10, -10, 20, 20);
    }

    QString getFlightName() const {
        return flightName;
    }

    void updatePosition() {
        advance(0);
    }

    void setSpeed(int newSpeed) {
        speed = newSpeed;
    }

    void setColor(const QColor& newColor) {
        brushColor = newColor;
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override {
        QPainterPath path;

        path.moveTo(0, -20);
        path.lineTo(0, -3);
        path.lineTo(-10, -3);
        path.lineTo(-10, 3);
        path.lineTo(0, 3);
        path.lineTo(0, 20);
        path.lineTo(5, 3);
        path.lineTo(15, 3);
        path.lineTo(17, 0);
        path.lineTo(15, -3);
        path.lineTo(5, -3);
        path.lineTo(0, -20);

        painter->setBrush(brushColor);
        painter->setPen(QPen(Qt::black, 1));

        painter->save();

        QRectF rect = boundingRect();
        painter->translate(rect.center());
        painter->rotate(angle);
        painter->translate(-rect.center());

        painter->drawPath(path);

        painter->restore();
    }

protected:
    void advance(int phase) override {
        if (phase == 0 && scene()) {
            qreal dx = speed * qCos(qDegreesToRadians(angle));
            qreal dy = speed * qSin(qDegreesToRadians(angle));

            xPos += dx;
            yPos += dy;

            QRectF sceneBounds = scene()->sceneRect();
            qreal halfSize = boundingRect().width() / 2; 

            if (xPos - halfSize < sceneBounds.left() || xPos + halfSize > sceneBounds.right()) {
                angle = 180 - angle;
                xPos = qBound(sceneBounds.left() + halfSize, xPos, sceneBounds.right() - halfSize);
            }
            if (yPos - halfSize < sceneBounds.top() || yPos + halfSize > sceneBounds.bottom()) {
                angle = -angle;
                yPos = qBound(sceneBounds.top() + halfSize, yPos, sceneBounds.bottom() - halfSize);
            }

            setPos(xPos, yPos);
            update();
        }
    }

private:
    qreal xPos, yPos;
    qreal angle;
    int speed;
    QString flightName;
    QColor brushColor = Qt::blue;
};

class FlightListModel : public QAbstractListModel {

public:
    FlightListModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

    void updateData(const QMap<QString, QPointF>& newData) {
        beginResetModel();
        airplanePositions = newData;
        endResetModel();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return airplanePositions.count();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || role != Qt::DisplayRole)
            return QVariant();

        QString flightName = airplanePositions.keys().at(index.row());
        QPointF position = airplanePositions.values().at(index.row());
        return QString("%1: (%2, %3)").arg(flightName).arg(position.x()).arg(position.y());
    }

private:
    QMap<QString, QPointF> airplanePositions;
};


class CustomGraphicsView : public QGraphicsView {
public:
    CustomGraphicsView(QWidget* parent = nullptr) : QGraphicsView(parent) {
        setRenderHint(QPainter::Antialiasing);
        setRenderHint(QPainter::SmoothPixmapTransform);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        setInteractive(true);

        currentZoomLevel = 1.0;
        maxZoomIn = 7.0;
        minZoomOut = 1.0;
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QGraphicsView::resizeEvent(event);
        if (scene()) {
            scene()->setSceneRect(QRectF(QPointF(0, 0), size()));
        }
    }

    void wheelEvent(QWheelEvent* event) override {
        qreal zoomFactor = 1.12;
        qreal newZoomLevel = currentZoomLevel;

        if (event->angleDelta().y()> 0) {
            newZoomLevel *= zoomFactor;
            if (newZoomLevel > maxZoomIn)
                newZoomLevel = maxZoomIn;
        }
        else {
            newZoomLevel /= zoomFactor;
            if (newZoomLevel < minZoomOut)
                newZoomLevel = minZoomOut;
        }

        setTransform(QTransform::fromScale(newZoomLevel, newZoomLevel));
        currentZoomLevel = newZoomLevel;
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
        QGraphicsView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (event->buttons() & Qt::LeftButton) {
            QPoint delta = event->pos() - lastMousePos;
            lastMousePos = event->pos();
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        }
        QGraphicsView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            setCursor(Qt::ArrowCursor);
        }
        QGraphicsView::mouseReleaseEvent(event);
    }

private:
    qreal currentZoomLevel;
    qreal maxZoomIn;
    qreal minZoomOut;
    QPoint lastMousePos;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QList<AirplaneItem*> airplanes;
    int num_airplains = 20;

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Airplane Radar Simulator");
    mainWindow.setMinimumSize(1024, 768);

    QWidget centralWidget;
    QHBoxLayout centralLayout(&centralWidget);
    mainWindow.setCentralWidget(&centralWidget);

    CustomGraphicsView radarView;
    QGraphicsScene radarScene;
    radarScene.setSceneRect(0, 0, radarView.width(), radarView.height());
    radarView.setScene(&radarScene);
    centralLayout.addWidget(&radarView, 4);

    for (int i = 0; i < num_airplains; ++i) {
        AirplaneItem* airplane = new AirplaneItem(i, radarScene.width(), radarScene.height());
        radarScene.addItem(airplane);
        airplanes.append(airplane);
    }

    QVBoxLayout* rightSideLayout = new QVBoxLayout();

    QListView flightListView;
    rightSideLayout->addWidget(&flightListView);

    FlightListModel* model = new FlightListModel(&mainWindow);
    flightListView.setModel(model);

    QWidget settingsWidget;
    QVBoxLayout settingsLayout(&settingsWidget);

    QLabel* numPlanesLabel = new QLabel("Number of Planes:");
    QSpinBox* numPlanesSpinBox = new QSpinBox;
    numPlanesSpinBox->setRange(1, 100);
    numPlanesSpinBox->setValue(num_airplains);
    settingsLayout.addWidget(numPlanesLabel);
    settingsLayout.addWidget(numPlanesSpinBox);

    QLabel* speedLabel = new QLabel("Airplane Speed:");
    QSlider* speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setRange(1, 10);
    speedSlider->setValue(2);
    settingsLayout.addWidget(speedLabel);
    settingsLayout.addWidget(speedSlider);

    QPushButton* colorButton = new QPushButton("Choose Airplane Color");
    settingsLayout.addWidget(colorButton);

    QPushButton* settingsButton = new QPushButton("Open Settings Window");
    settingsLayout.addWidget(settingsButton);

    rightSideLayout->addWidget(&settingsWidget);
    centralLayout.addLayout(rightSideLayout, 1);

    SettingsDialog* settingsDialog = nullptr;

    auto updateAirplaneCount = [&](int newCount) {
        while (airplanes.size() < newCount) {
            int id = airplanes.size();
            AirplaneItem* airplane = new AirplaneItem(id, radarScene.width(), radarScene.height());
            radarScene.addItem(airplane);
            airplanes.append(airplane);
        }
        while (airplanes.size() > newCount) {
            AirplaneItem* airplane = airplanes.takeLast();
            radarScene.removeItem(airplane);
            delete airplane;
        }
        num_airplains = newCount;
    };

    QObject::connect(settingsButton, &QPushButton::clicked, [&]() {
        if (!settingsDialog) {
            settingsDialog = new SettingsDialog(&mainWindow, &radarView, &flightListView);
            settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
            QObject::connect(settingsDialog, &QObject::destroyed, [&]() {
                settingsDialog = nullptr;
                });
        }

        if (settingsDialog->isHidden()) {
            settingsDialog->show();
        }
        else {
            settingsDialog->raise();
            settingsDialog->activateWindow();
        }
    });

    QObject::connect(numPlanesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), updateAirplaneCount);

    QObject::connect(speedSlider, &QSlider::valueChanged, [&](int newSpeed) {
        for (AirplaneItem* airplane : airplanes) {
            airplane->setSpeed(newSpeed);
        }
    });

    QObject::connect(colorButton, &QPushButton::clicked, [&]() {
        QColor color = QColorDialog::getColor(Qt::blue, &mainWindow, "Choose color");
        if (color.isValid()) {
            for (AirplaneItem* airplane : airplanes) {
                airplane->setColor(color);
            }
        }
    });
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        QMap<QString, QPointF> airplanePositions;
        QRectF viewportRect = radarView.mapToScene(radarView.viewport()->rect()).boundingRect();

        QList<QGraphicsItem*> visible_items = radarScene.items();
        for (QGraphicsItem* item : visible_items) {
            if (AirplaneItem* airplane = dynamic_cast<AirplaneItem*>(item)) {
                airplane->updatePosition();

                if (viewportRect.intersects(airplane->boundingRect().translated(airplane->pos()))) {
                    airplanePositions[airplane->getFlightName()] = airplane->pos();
                }
            }
        }

        model->updateData(airplanePositions);
        });
    timer.start(100);


    mainWindow.show();
    return app.exec();
}