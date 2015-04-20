/*
* Author: Luis Fererira
* E-mail: luisfferreira@outlook.com
* Date: March 2015
*/

#include "shared_methods.h"

// ED includes
#include <ed/error_context.h>
#include <rgbd/Image.h>
#include <rgbd/View.h>

// ---------------------------------------------------------------------------------------------------

SharedMethods::SharedMethods(){
}


SharedMethods::~SharedMethods(){
}

// ---------------------------------------------------------------------------------------------------

void SharedMethods::prepareMeasurement(const ed::EntityConstPtr& e, cv::Mat& view_color_img, cv::Mat& view_depth_img, cv::Mat& mask, cv::Rect& bouding_box) const{

    // Get the best measurement from the entity
    ed::MeasurementConstPtr msr = e->lastMeasurement();
    if (!msr)
        return;

    int min_x, max_x, min_y, max_y;

    // create a view
    rgbd::View view(*msr->image(), msr->image()->getRGBImage().cols);

    // get depth image
    view_depth_img = msr->image()->getDepthImage();

    // get color image
    const cv::Mat& color_image = msr->image()->getRGBImage();

    // crop it to match the view
    view_color_img = cv::Mat(color_image(cv::Rect(0, 0, view.getWidth(), view.getHeight())));

//    std::cout << "image: " << cropped_image.cols << "x" << cropped_image.rows << std::endl;

    // initialize bounding box points
    max_x = 0;
    max_y = 0;
    min_x = view.getWidth();
    min_y = view.getHeight();

    // initialize mask, all 0s
    mask = cv::Mat::zeros(view.getHeight(), view.getWidth(), CV_8UC1);

    // Iterate over all points in the mask
    for(ed::ImageMask::const_iterator it = msr->imageMask().begin(view.getWidth()); it != msr->imageMask().end(); ++it)
    {
        // mask's (x, y) coordinate in the depth image
        const cv::Point2i& p_2d = *it;

        // paint a mask
        mask.at<unsigned char>(*it) = 255;

        // update the boundary coordinates
        if (min_x > p_2d.x) min_x = p_2d.x;
        if (max_x < p_2d.x) max_x = p_2d.x;
        if (min_y > p_2d.y) min_y = p_2d.y;
        if (max_y < p_2d.y) max_y = p_2d.y;
    }

    bouding_box = cv::Rect(min_x, min_y, max_x - min_x, max_y - min_y);
}


// ----------------------------------------------------------------------------------------------------

float SharedMethods::getAverageDepth(cv::Mat& depth_img) const{

    float median = 0;
    std::vector<float> depths;

    // fill vector with depth values
    for (uint x = 0 ; x < depth_img.cols ; x++){
        for (uint y = 0 ; y < depth_img.rows ; y++){
            if (depth_img.at<float>(y,x) > 0.0){
                depths.push_back(depth_img.at<float>(y,x));
            }
        }
    }

    if (depths.empty()){
        std::cout << "GetAverageDepth: no depth values pushed. Empty image matrix?" << std::endl;
        return 0.0;
    }

    // sort and pick the center value
    std::sort(depths.begin(),depths.end());

    if (depths.size() % 2 == 0){
        median = (depths[depths.size()/2-1] + depths[depths.size()/2]) / 2.0;
    }
    else{
        median = depths[depths.size() / 2];
    }


    return median;
}

// ----------------------------------------------------------------------------------------------------

void SharedMethods::optimizeContourHull(const cv::Mat& mask_orig, cv::Mat& mask_optimized) const{

    std::vector<std::vector<cv::Point> > hull;
    std::vector<std::vector<cv::Point> > contours;

    mask_optimized = cv::Mat::zeros(mask_orig.size(), CV_8UC1);

    cv::findContours(mask_orig, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    for (uint i = 0; i < contours.size(); i++){
        hull.push_back(std::vector<cv::Point>());
        cv::convexHull(cv::Mat(contours[i]), hull.back(), false);

        cv::drawContours(mask_optimized, hull, -1, cv::Scalar(255), CV_FILLED);
    }
}


// ----------------------------------------------------------------------------------------------------


void SharedMethods::optimizeContourBlur(const cv::Mat& mask_orig, cv::Mat& mask_optimized) const{

    mask_orig.copyTo(mask_optimized);

    // blur the contour, also expands it a bit
    for (uint i = 6; i < 18; i = i + 2){
        cv::blur(mask_optimized, mask_optimized, cv::Size( i, i ), cv::Point(-1,-1) );
    }

    cv::threshold(mask_optimized, mask_optimized, 50, 255, CV_THRESH_BINARY);
}
