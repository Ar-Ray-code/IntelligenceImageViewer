#ifndef HSV_PLUGIN_H
#define HSV_PLUGIN_H

#include "ai_plugin_interface.h"
#include <QObject>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

class HSVPlugin : public QObject, public AIPlugin
{
    Q_OBJECT
    Q_INTERFACES(AIPlugin)
    Q_PLUGIN_METADATA(IID "com.example.AIPluginInterface")
public:
    HSVPlugin();
    virtual ~HSVPlugin();

    void init(const AIConfig& config) override;
    void update_config(const AIConfig& config) override;
    void deinit() override;
    void fetch(const cv::Mat& image) override;
    void* get(void* param) override;
    void render_result(const cv::Mat& input, cv::Mat& output) override;
    void cleanup() override;
    void status(AIStatus status, const std::string& msg) override;
    std::string getName() const override { return "HSV Plugin"; }

private:
    cv::Mat hsvImage;
    AIStatus currentStatus;
};

#endif // HSV_PLUGIN_H
