#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <sys/stat.h>
#include <cmath>

using namespace cv;
using namespace cv::aruco;
using namespace std;

struct Face {
    string label;
    Point2f pos;
    Face(string _label, Point2f _pos) {
        label = _label;
        pos = _pos;
    }
};

int main() {
    string posFile; posFile = "test.xml";
    FileStorage fs(posFile, FileStorage::WRITE);
    if (!fs.isOpened()) {
        cout << "Failed to open " << posFile << '\n';
    }
    Face face("Leo", Point2f(1000, 1000));
    fs << "labels";
    fs << "[";
    fs << face.label;
    fs << "]";
    fs << "label_position";
    fs << "{";
    fs << face.label << face.pos;
    fs << "}";
}