/*
 * Copyright (c) 2024 Ar-Ray-code
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ai_plugin_manager.h"
#include <QDebug>
#include <QThread>
#include <QtConcurrent>

AIPluginManager::AIPluginManager(QObject* parent) : QObject(parent)
{
}

AIPluginManager::~AIPluginManager()
{
    cancelAll();
    for (auto plugin : plugins)
    {
        plugin->deinit();
        delete plugin;
    }
    plugins.clear();
}

void AIPluginManager::loadModels(const QString& configPath)
{
    qDebug() << "Loading models from" << configPath;
    for (auto plugin : plugins)
    {
        plugin->deinit();
        delete plugin;
    }
    plugins.clear();
}

void AIPluginManager::startTask(int modelIndex, const cv::Mat3b& image, int timeoutSeconds)
{
    QMutexLocker locker(&mutex);
    if (isTaskRunning())
    {
        qDebug() << "A task is already running. Ignoring new task.";
        return;
    }
    if (modelIndex < 0 || modelIndex >= static_cast<int>(plugins.size()))
    {
        qDebug() << "Invalid model index";
        return;
    }

    Task task;
    task.modelIndex = modelIndex;
    task.running = true;
    task.watcher = new QFutureWatcher<void>(this);
    connect(task.watcher,
            &QFutureWatcher<void>::finished,
            [this, modelIndex, taskWatcher = task.watcher]()
            {
                QMutexLocker locker(&mutex);
                cv::Mat3b result;
                emit taskFinished(modelIndex, result);
                for (auto& t : tasks)
                {
                    if (t.modelIndex == modelIndex && t.watcher == taskWatcher)
                    {
                        t.running = false;
                        break;
                    }
                }
                taskWatcher->deleteLater();
            });

    QFuture<void> future = QtConcurrent::run([=]() { runTask(modelIndex, image, timeoutSeconds); });
    task.future = future;
    task.watcher->setFuture(task.future);
    tasks.push_back(task);
    emit taskStarted(modelIndex);
}

void AIPluginManager::cancelTask(int modelIndex)
{
    QMutexLocker locker(&mutex);
    for (auto& t : tasks)
    {
        if (t.modelIndex == modelIndex && t.running)
        {
            t.running = false;
            qDebug() << "Task" << modelIndex << "canceled.";
            cv::Mat3b result;
            emit taskFinished(modelIndex, result);
        }
    }
}

void AIPluginManager::cancelAll()
{
    QMutexLocker locker(&mutex);
    for (auto& t : tasks)
    {
        if (t.running)
        {
            t.running = false;
            qDebug() << "Canceling task" << t.modelIndex;
            cv::Mat3b result;
            emit taskFinished(t.modelIndex, result);
        }
    }
    tasks.clear();
}

bool AIPluginManager::isTaskRunning() const
{
    for (const auto& t : tasks)
    {
        if (t.running)
            return true;
    }
    return false;
}

void AIPluginManager::runTask(int modelIndex, cv::Mat3b image, int timeoutSeconds)
{
    qDebug() << "Running AI task for model" << modelIndex;
    QThread::sleep(timeoutSeconds);
    emit taskStatusChanged(modelIndex, static_cast<int>(AIStatus::Done), "Task completed");
    qDebug() << "AI task for model" << modelIndex << "completed.";
}

void AIPluginManager::addPlugin(AIPlugin* plugin)
{
    QMutexLocker locker(&mutex);
    if (plugin)
    {
        plugins.push_back(plugin);
        AIConfig defaultConfig;
        defaultConfig.param1 = "";
        defaultConfig.param2 = 0;
        plugin->init(defaultConfig);
        qDebug() << "Plugin added and initialized.";
    }
}