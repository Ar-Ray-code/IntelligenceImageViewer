#ifndef AI_PLUGIN_MANAGER_H
#define AI_PLUGIN_MANAGER_H

#include "ai_plugin_interface.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <opencv2/opencv.hpp>
#include <vector>

class AIPluginManager : public QObject
{
    Q_OBJECT
public:
    explicit AIPluginManager(QObject* parent = nullptr);
    ~AIPluginManager();

    void loadModels(const QString& configPath);
    const std::vector<AIPlugin*>& getPlugins() const { return plugins; }

    void startTask(int modelIndex, const cv::Mat3b& image, int timeoutSeconds = 10);
    void cancelTask(int modelIndex);
    void cancelAll();
    bool isTaskRunning() const;
    void addPlugin(AIPlugin* plugin);

signals:
    void taskStarted(int modelIndex);
    void taskFinished(int modelIndex, const cv::Mat3b& result);
    void taskStatusChanged(int modelIndex, int status, const QString& msg);

private:
    struct Task
    {
        int modelIndex;
        QFuture<void> future;
        QFutureWatcher<void>* watcher;
        bool running;
    };

    std::vector<AIPlugin*> plugins;
    std::vector<Task> tasks;
    mutable QMutex mutex;

    void runTask(int modelIndex, cv::Mat3b image, int timeoutSeconds);
};

#endif // AI_PLUGIN_MANAGER_H
