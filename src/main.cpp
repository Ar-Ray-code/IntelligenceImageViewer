/*
 * Copyright (c) 2024 Ar-Ray-code
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGesture>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPinchGesture>
#include <QPluginLoader>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMath>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "ai_plugin_interface.h"
#include "ai_plugin_manager.h"

QImage cvMatToQImage(const cv::Mat& mat)
{
    if (mat.type() == CV_8UC3)
    {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
    }
    else if (mat.type() == CV_8UC1)
    {
        return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8).copy();
    }
    return QImage();
}

class ImageGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ImageGraphicsView(QWidget* parent = nullptr) : QGraphicsView(parent), zoomFactor(1.0)
    {
        setAlignment(Qt::AlignCenter);
        setRenderHint(QPainter::SmoothPixmapTransform);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setFocusPolicy(Qt::StrongFocus);
        grabGesture(Qt::PinchGesture);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        setResizeAnchor(QGraphicsView::AnchorViewCenter);
    }
    void setZoomFactor(double factor)
    {
        zoomFactor = factor;
        updateTransform();
    }
    void resetZoom()
    {
        zoomFactor = 1.0;
        updateTransform();
    }
signals:
    void nextImageRequested();
    void previousImageRequested();

protected:
    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::Gesture)
        {
            return gestureEvent(static_cast<QGestureEvent*>(event));
        }
        return QGraphicsView::event(event);
    }
    bool gestureEvent(QGestureEvent* event)
    {
        if (QGesture* pinch = event->gesture(Qt::PinchGesture))
        {
            pinchTriggered(static_cast<QPinchGesture*>(pinch));
        }
        return true;
    }
    void pinchTriggered(QPinchGesture* gesture)
    {
        if (gesture->state() == Qt::GestureUpdated)
        {
            qreal changeFactor = gesture->scaleFactor();
            zoomFactor *= changeFactor;
            updateTransform();
        }
    }
    void wheelEvent(QWheelEvent* event) override
    {
        double numDegrees = event->angleDelta().y() / 8.0;
        double numSteps = numDegrees / 15.0;
        double factor = qPow(1.1, numSteps);
        zoomFactor *= factor;
        updateTransform();
        event->accept();
    }
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Right)
        {
            emit nextImageRequested();
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Left)
        {
            emit previousImageRequested();
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
    }
    void resizeEvent(QResizeEvent* event) override
    {
        QGraphicsView::resizeEvent(event);
        updateTransform();
    }

private:
    double zoomFactor;
    void updateTransform()
    {
        if (!scene() || scene()->items().isEmpty())
            return;
        QGraphicsItem* item = scene()->items().first();
        QRectF itemRect = item->boundingRect();
        double baseFactor;
        if (itemRect.width() >= itemRect.height())
        {
            baseFactor = viewport()->width() / itemRect.width();
        }
        else
        {
            baseFactor = viewport()->height() / itemRect.height();
        }
        QTransform transform;
        transform.scale(baseFactor * zoomFactor, baseFactor * zoomFactor);
        setTransform(transform);
    }
};

class ImageViewerWidget : public QWidget
{
    Q_OBJECT
public:
    ImageViewerWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        scene = new QGraphicsScene(this);
        view = new ImageGraphicsView(this);
        view->setScene(scene);
        view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(view);
        setLayout(layout);
    }
    ImageGraphicsView* getView() const { return view; }

    bool loadImage(const QString& filePath)
    {
        cv::Mat img = cv::imread(filePath.toStdString(), cv::IMREAD_COLOR);
        if (img.empty())
            return false;
        currentImage = img;
        updateImage(currentImage);
        return true;
    }

    cv::Mat getOriginalImage() const { return currentImage; }

    bool updateImage(const cv::Mat& img)
    {
        QImage qimg = cvMatToQImage(img);
        if (qimg.isNull())
            return false;
        if (pixmapItem)
        {
            scene->removeItem(pixmapItem);
            delete pixmapItem;
        }
        pixmapItem = new QGraphicsPixmapItem(QPixmap::fromImage(qimg));
        scene->addItem(pixmapItem);
        view->resetZoom();
        return true;
    }

private:
    ImageGraphicsView* view;
    QGraphicsScene* scene;
    QGraphicsPixmapItem* pixmapItem = nullptr;
    cv::Mat currentImage;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(AIPluginManager* manager, QWidget* parent = nullptr) : QMainWindow(parent), aiManager(manager), currentIndex(0)
    {
        viewer = new ImageViewerWidget(this);
        setCentralWidget(viewer);

        connect(viewer->getView(), &ImageGraphicsView::nextImageRequested, this, &MainWindow::loadNextImage);
        connect(viewer->getView(), &ImageGraphicsView::previousImageRequested, this, &MainWindow::loadPreviousImage);

        createMenus();
        loadImageDirectory();
    }
private slots:
    void loadNextImage()
    {
        if (imageFiles.isEmpty())
            return;
        currentIndex = (currentIndex + 1) % imageFiles.size();
        if (viewer->loadImage(imageFiles[currentIndex]))
            updateRenderedImage();
    }
    void loadPreviousImage()
    {
        if (imageFiles.isEmpty())
            return;
        currentIndex = (currentIndex - 1 + imageFiles.size()) % imageFiles.size();
        if (viewer->loadImage(imageFiles[currentIndex]))
            updateRenderedImage();
    }

    void updateRenderedImage()
    {
        cv::Mat original = viewer->getOriginalImage();
        if (original.empty())
            return;
        cv::Mat rendered = original.clone();

        for (const auto& p : pluginActions)
        {
            if (p.second->isChecked())
            {
                cv::Mat output;
                p.first->render_result(rendered, output);
                rendered = output.clone();
            }
        }
        viewer->updateImage(rendered);
    }

private:
    void createMenus()
    {
        QMenuBar* menuBarPtr = menuBar();
        QMenu* aiMenu = menuBarPtr->addMenu("AI");

        QMenu* tasksMenu = aiMenu->addMenu("Tasks");
        QAction* startTaskAction = new QAction("Start/Cancel Task", this);
        connect(startTaskAction,
                &QAction::triggered,
                [this]()
                {
                    if (aiManager->isTaskRunning())
                    {
                        aiManager->cancelTask(0);
                    }
                    else
                    {
                        if (!imageFiles.isEmpty() && currentIndex < imageFiles.size())
                        {
                            cv::Mat img = cv::imread(imageFiles[currentIndex].toStdString(), cv::IMREAD_COLOR);
                            if (!img.empty())
                                aiManager->startTask(0, img, 5);
                        }
                    }
                });
        tasksMenu->addAction(startTaskAction);

        QMenu* modelsMenu = aiMenu->addMenu("Models");
        const auto& plugins = aiManager->getPlugins();
        for (auto plugin : plugins)
        {
            QString pluginName = QString::fromStdString(plugin->getName());
            QAction* pluginAction = new QAction(pluginName, this);
            pluginAction->setCheckable(true);
            connect(pluginAction, &QAction::toggled, this, &MainWindow::updateRenderedImage);
            modelsMenu->addAction(pluginAction);
            pluginActions.push_back(std::make_pair(plugin, pluginAction));
        }

        QAction* cancelAllAction = new QAction("cancel_all", this);
        connect(cancelAllAction,
                &QAction::triggered,
                [this, modelsMenu]()
                {
                    aiManager->cancelAll();
                    foreach (QAction* action, modelsMenu->actions())
                    {
                        action->setChecked(false);
                    }
                    updateRenderedImage();
                });
        aiMenu->addAction(cancelAllAction);
    }
    void loadImageDirectory()
    {
        QString dirPath = QFileDialog::getExistingDirectory(this, "Select Image Directory");
        if (dirPath.isEmpty())
            return;
        QDir dir(dirPath);
        QStringList filters;
        filters << "*.png"
                << "*.jpg"
                << "*.jpeg"
                << "*.bmp";
        imageFiles = dir.entryList(filters, QDir::Files, QDir::Name);
        for (int i = 0; i < imageFiles.size(); i++)
        {
            imageFiles[i] = dir.absoluteFilePath(imageFiles[i]);
        }
        if (!imageFiles.isEmpty())
        {
            currentIndex = 0;
            if (viewer->loadImage(imageFiles[currentIndex]))
                updateRenderedImage();
        }
    }
    ImageViewerWidget* viewer;
    AIPluginManager* aiManager;
    QStringList imageFiles;
    int currentIndex;
    std::vector<std::pair<AIPlugin*, QAction*>> pluginActions;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    AIPluginManager* aiManager = new AIPluginManager();

    // tmp yaml file path
    QString configFilePath = QDir::currentPath() + "/../config/config.yaml";
    QFile configFile(configFilePath);
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open config file:" << configFilePath;
    }
    else
    {
        QString pluginPath;
        QTextStream in(&configFile);
        while (!in.atEnd())
        {
            QString line = in.readLine().trimmed();
            if (line.startsWith("- pluginpath:"))
            {
                QStringList parts = line.split(":");
                if (parts.size() == 2)
                {
                    qDebug() << "Plugin path found:" << parts[1];
                    pluginPath = parts[1].trimmed();
                }

                break;
            }
        }
        configFile.close();
        if (!pluginPath.isEmpty())
        {
            if (QDir::isRelativePath(pluginPath))
                pluginPath = QDir::current().absoluteFilePath(pluginPath);
            QPluginLoader loader(pluginPath);
            QObject* pluginInstance = loader.instance();
            qDebug() << "Plugin loaded:" << pluginInstance;
            if (!pluginInstance)
            {
                qWarning() << "Failed to load plugin:" << loader.errorString();
            }
            else
            {
                AIPlugin* plugin = qobject_cast<AIPlugin*>(pluginInstance);
                if (plugin)
                {
                    aiManager->addPlugin(plugin);
                    qDebug() << "Plugin loaded successfully:" << pluginPath;
                }
                else
                {
                    qWarning() << "Failed to cast plugin to AIPlugin.";
                }
            }
        }
    }

    MainWindow mainWindow(aiManager);
    mainWindow.setWindowTitle("AI Plugin Viewer");
    mainWindow.show();
    return app.exec();
}

#include "main.moc"
