#ifndef AI_PLUGIN_INTERFACE_H
#define AI_PLUGIN_INTERFACE_H

#include <QObject>

#include <opencv2/opencv.hpp>
#include <string>

enum class AIStatus
{
    Ready = 0,
    Processing,
    Done,
    Error,
    Fatal,
    Timeout
};

struct AIConfig
{
    std::string param1;
    int param2;
};

class AIPlugin
{
public:
    virtual ~AIPlugin() {}
    virtual void init(const AIConfig& config) = 0;
    virtual void update_config(const AIConfig& config) = 0;
    virtual void deinit() = 0;
    virtual void fetch(const cv::Mat& image) = 0;
    virtual void* get(void* param) = 0;
    virtual void render_result(const cv::Mat& input, cv::Mat& output) = 0;
    virtual void cleanup() = 0;
    virtual void status(AIStatus status, const std::string& msg) = 0;
    virtual std::string getName() const = 0;
};

#define AI_PLUGIN_IID "com.example.AIPluginInterface"
Q_DECLARE_INTERFACE(AIPlugin, AI_PLUGIN_IID)

#endif // AI_PLUGIN_INTERFACE_H
