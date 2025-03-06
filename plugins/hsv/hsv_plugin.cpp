#include "hsv_plugin.h"
#include <QtPlugin>

HSVPlugin::HSVPlugin() : currentStatus(AIStatus::Ready)
{
    std::cout << "HSVPlugin created." << std::endl;
}

HSVPlugin::~HSVPlugin()
{
    deinit();
}

void HSVPlugin::init(const AIConfig& config)
{
    std::cout << "HSVPlugin init called." << std::endl;
    currentStatus = AIStatus::Ready;
}

void HSVPlugin::update_config(const AIConfig& config)
{
    std::cout << "HSVPlugin update_config called." << std::endl;
}

void HSVPlugin::deinit()
{
    std::cout << "HSVPlugin deinit called." << std::endl;
    hsvImage.release();
    currentStatus = AIStatus::Ready;
}

void HSVPlugin::fetch(const cv::Mat& image)
{
    std::cout << "HSVPlugin fetch called." << std::endl;
    if (image.empty())
    {
        std::cerr << "Input image is empty!" << std::endl;
        currentStatus = AIStatus::Error;
        return;
    }
    cv::cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);
    currentStatus = AIStatus::Done;
}

void* HSVPlugin::get(void* param)
{
    std::cout << "HSVPlugin get called." << std::endl;
    return static_cast<void*>(&hsvImage);
}

void HSVPlugin::render_result(const cv::Mat& input, cv::Mat& output)
{
    std::cout << "HSVPlugin render_result called." << std::endl;
    if (input.empty())
    {
        std::cerr << "Input image is empty!" << std::endl;
        currentStatus = AIStatus::Error;
        return;
    }
    cv::cvtColor(input, output, cv::COLOR_BGR2HSV);
    currentStatus = AIStatus::Done;
}

void HSVPlugin::cleanup()
{
    std::cout << "HSVPlugin cleanup called." << std::endl;
    hsvImage.release();
}

void HSVPlugin::status(AIStatus status, const std::string& msg)
{
    std::cout << "HSVPlugin status update: " << msg << std::endl;
    currentStatus = status;
}
