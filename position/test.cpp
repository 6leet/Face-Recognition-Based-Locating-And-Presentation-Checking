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

Mat cameraMatrix, distMatrix; 

Point2f getMidPoint(const vector<Point2f> &point2d);
void allocate(const vector<Point2f> &point2d, vector<Point2f> &allocPoint);

struct Marker {
    int id;
    vector<Point2f> corners;
    Point2f midPoint;
    Point3f worldPos;
    Marker(int _id, vector<Point2f> &_corners) {
        id = _id;

        corners.resize(_corners.size());
        for (int i = 0; i < _corners.size(); i++) {
            corners[i] = _corners[i];
        }
        midPoint = getMidPoint(corners);
        worldPos = Point3f(-1, -1, -1);
    }
    void setWorldPos(float x, float y, float z) {
        worldPos = Point3f(x, y, z);
    }
    Marker() {
        id = -1;
    }
};

struct Area {
    int id;
    vector<Marker> corners;
    float width, height;
    Area(int _id, vector<Marker> &_rectangle, float _width, float _height) {
        id = _id;

        corners.resize(_rectangle.size());
        for (int i = 0; i < corners.size(); i++) {
            corners[i] = _rectangle[i];
        }
        width = _width;
        height = _height;
    }
};

struct Face {
    string label;
    Point2f pos, homoPos;
    Face(string _label, Point2f _pos) {
        label = _label;
        pos = _pos;
    }
};

struct Seat {
    int row, col;
    Point2f pos, homoPos;
    int areaIndex;
    Seat(Point2f _pos, Point2f _homoPos, int _areaIndex) {
        pos = _pos;
        homoPos = _homoPos;
        areaIndex = _areaIndex;
    }
};

void build() {
    string calFile = "/Users/leolee/documents/108_2/lab_project/calibration/calibration.xml";
    FileStorage fs(calFile, FileStorage::READ);
    if (!fs.isOpened()) {
        cout << "Failed to open " << calFile << '\n';
    }
    fs["intrinsic"] >> cameraMatrix;
    fs["distortion"] >> distMatrix;
    fs.release();
}

inline bool fileExists(const string& name) {
    struct stat buffer;   
    return (stat (name.c_str(), &buffer) == 0); 
}

void calibrate(const Mat &src, Mat &dst) {
    Size imgSize = src.size();
    Mat mapx, mapy;
    initUndistortRectifyMap(cameraMatrix, distMatrix, Mat(), cameraMatrix, imgSize, CV_32F, mapx, mapy);
    remap(src, dst, mapx, mapy, INTER_LINEAR);
}

void findMarkers(Mat &img, vector<int> &markerIds, vector<vector<Point2f> > &markerCorners) { // input: img, output: markerIds, markerCorners
    vector<vector<Point2f> > rejectedCandidates;
    Ptr<DetectorParameters> parameters = DetectorParameters::create();
    Ptr<Dictionary> dictionary = getPredefinedDictionary(DICT_6X6_250); // dictionary that map's to marker's id.
    detectMarkers(img, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
}

void markersInfo(Mat &img, vector<Marker> &markers, bool ifDraw = true) {
    vector<int> markerIds;
    vector<vector<Point2f> > markerCorners;
    findMarkers(img, markerIds, markerCorners);
    for (int i = 0; i < markerIds.size(); i++) {
        Marker marker(markerIds[i], markerCorners[i]);
        markers.push_back(marker);
    }
    for (int i = 0; i < markers.size(); i++) {
        cout << "id: " << markers[i].id << '\n';
        cout << "corners: " << markers[i].corners << '\n';
        cout << "midpoint: " << markers[i].midPoint << '\n';
    }
    if (ifDraw) {
        drawDetectedMarkers(img, markerCorners, markerIds);
        imshow("draw markers", img);
        waitKey(0);
    }
}

Point2f getMidPoint(const vector<Point2f> &point2d) { // input: point2d, output: midpoint
    Point2f mp;
    for (int i = 0; i < point2d.size(); i++) {
        mp.x += point2d[i].x;
        mp.y += point2d[i].y;
    }
    mp.x /= point2d.size(); mp.y /= point2d.size();
    return mp;
}

float findArea(const Point2f &pointA, const Point2f &pointB, const Point2f &pointC) { // get triangle area
    return abs((pointA.x * (pointB.y - pointC.y) + pointB.x * (pointC.y - pointA.y) + pointC.x * (pointA.y - pointB.y)) / 2.0);
}

bool inArea(const Point2f &point2d, const vector<Point2f> &rectanglePoint2d, float eps = 1) { // check if a point is in a rectangle
    // input: point2d, rectanglePoint2d
    float recArea = findArea(rectanglePoint2d[0], rectanglePoint2d[1], rectanglePoint2d[2]) +
                    findArea(rectanglePoint2d[0], rectanglePoint2d[2], rectanglePoint2d[3]);
    float area, areaSum = 0;
    for (int i = 0; i < 4; i++) {
        area = findArea(point2d, rectanglePoint2d[i], rectanglePoint2d[(i + 1) % 4]);
        areaSum += area;
    }
    return abs(recArea - areaSum) < eps;
}

void setWorldPos_callback(int event, int x, int y, int flags, void* userdata) {
    if  (event == EVENT_LBUTTONDOWN) {
        vector<Marker> *markers = (vector<Marker>*)userdata;
        Point2f clickPoint;
        clickPoint.x = x, clickPoint.y = y;
        int markerId = -1, it = -1;
        for (int i = 0; i < markers->size(); i++) {
            if (inArea(clickPoint, markers->at(i).corners)) {
                markerId = markers->at(i).id;
                it = i;
                break;
            }
        }
        if (markerId == -1) {
            cout << "Not a marker, nigga\n";
        } else {
            cout << "Set marker " << markerId << "'s 3d-world position.\n";
            float x, y, z;
            cout << "input (x, y, z): "; 
            cin >> x >> y >> z;
            markers->at(it).setWorldPos(x, y, z);
        }
    }
}

void clickPoints_callback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        vector<Point2f> *clickPoints = (vector<Point2f>*)userdata;
        clickPoints->push_back(Point2f(x, y));
        cout << "picked " << x << ' ' << y << '\n';
    } else if (event == EVENT_RBUTTONDOWN) {
        vector<Point2f> *clickPoints = (vector<Point2f>*)userdata;
        if (clickPoints->size()) {
            Point2f last = clickPoints->back();
            clickPoints->pop_back();
            cout << "unpick " << last.x << ' ' << last.y << '\n';
        } else {
            cout << "no point picked yet.\n";
        }
    } else if (event == EVENT_MBUTTONDOWN) {
        vector<Point2f> *clickPoints = (vector<Point2f>*)userdata;
        clickPoints->push_back(Point2f(-1, -1));
        cout << "Leaving.\n";
    }
}

void setWorldPos(const Mat &img, vector<Marker> &markers) {
    string winName = "Setting markers 3d-world position";
    while (true) {
        cout << "Please complete all marker's 3d-world position.\n";
        cout << "Where button-left should be (0, 0, 0)\n";
        namedWindow(winName, 1);
        setMouseCallback(winName, setWorldPos_callback, &markers);
        imshow(winName, img);
        // imshow("markers", inputImg);
        waitKey(0);
        bool flag = true;
        for (int i = 0; i < markers.size(); i++) {
            if (markers[i].worldPos.x == -1) {
                flag = false;
                break;
            }
        }
        if (flag) {
            break;
        }
    }
    destroyWindow(winName);
    cout << "Save world position data to file?\n";
    while (true) {
        cout << "Please type (y/n): ";
        string s; cin >> s;
        if (s == "y") {
            cout << "Please input your filename (without extension, will default with .xml): ";
            string filename; cin >> filename;
            FileStorage fs(filename + ".xml", FileStorage::WRITE);
            for (int i = 0; i < markers.size(); i++) {
                string key = "marker" + to_string(markers[i].id);
                fs << key << markers[i].worldPos;
            }
            fs.release();
            break;
        } else if (s == "n") {
            break;
        }
    }
}

bool getWorldPos(string filename, vector<Marker> &markers) {
    filename = filename + ".xml";
    FileStorage fs(filename, FileStorage::READ);
    if (!fileExists(filename)) {
        cout << filename << " does not exists.\n";
        return false;
    }
    int cnt = 0;
    for (int i = 0; i < markers.size(); i++) {
        string key = "marker" + to_string(markers[i].id);
        fs[key] >> markers[i].worldPos;
        if (markers[i].worldPos == Point3f(0, 0, 0)) {
            cnt++;
        }
    }
    fs.release();
    if (cnt > 1) {
        cout << "Data can't match with current image.\n";
        return false;
    }
    return true;
}

void findRectangles(const Mat &img, const vector<Marker> &markers, vector<vector<Marker> > &rectangles) {
    string winName = "Find segmentation area";
    int need_cnt = (markers.size() - 4) / 2 + 1;
    while (true) {
        cout << "Please select " << need_cnt << " rectangle(s).\n";
        cout << "Got " << rectangles.size() << '\n';
        while (true) {
            cout << "Please pick 4 markers to form a rectangle\n";
            vector<Point2f> clickPoints; 
            vector<Marker> rectangle;
            namedWindow(winName, 1);
            setMouseCallback(winName, clickPoints_callback, &clickPoints);
            imshow(winName, img);
            waitKey(0);
            if (clickPoints.size() >= 4) {
                vector<bool> picked; picked.resize(markers.size() + 1);
                for (int i = 0; i < picked.size(); i++) {
                    picked[i] = false;
                }
                int cnt = 0;
                for (int i = 0; i < 4; i++) {
                    for (int j = 0; j < markers.size(); j++) {
                        if (inArea(clickPoints[i], markers[j].corners) && !picked[markers[j].id]) {
                            rectangle.push_back(markers[j]);
                            picked[markers[j].id] = true;
                            cnt++;
                            break;
                        }
                    }
                }
                if (cnt == 4) {
                    rectangles.push_back(rectangle);
                    break;
                }
            }
        }
        if (rectangles.size() >= need_cnt) {
            break;
        }
    }
    destroyWindow(winName);
}

void markersWorldPos(const Mat &img, vector<Marker> &markers) {
    cout << "Read world position data from file?\n";
    while (true) {
        cout << "Please type (y/n): ";
        string s; cin >> s;
        if (s == "y") {
            cout << "Please input your filename (without extension, will default with .xml): ";
            string filename; cin >> filename;
            if (getWorldPos(filename, markers)) {
                break;
            }
        } else if (s == "n") {
            setWorldPos(img, markers);
            break;
        }
    }
}

void pnpSolver(const vector<vector<Marker> > &rectangles, Mat &rvec, Mat &tvec) {
    int it; 
    while (true) {
        cout << "Which rectangle to solve PNP?\n";
        cout << "Input (index): ";
        cin >> it;
        if (it >= rectangles.size() || it < 0) {
            cout << "Rectangle " << it << " does not exist.\n";
        } else {
            break;
        }
    }
    vector<Point2f> point2d; point2d.resize(rectangles[it].size());
    vector<Point3f> point3d; point3d.resize(rectangles[it].size());
    for (int i = 0; i < rectangles[it].size(); i++) {
        point2d[i] = rectangles[it][i].midPoint;
        point3d[i] = rectangles[it][i].worldPos;
    }
    rvec = Mat::zeros(3, 1, CV_64FC1);
    tvec = Mat::zeros(3, 1, CV_64FC1);
    solvePnP(point3d, point2d, cameraMatrix, distMatrix, rvec, tvec, false, CV_ITERATIVE);
}

void getH(const Mat &img, const vector<Marker> &rectangle, Mat &H, Mat &homoImg) {
    vector<Point2f> point2d; point2d.resize(rectangle.size());
    vector<Point2f> lookDownPoint2d; lookDownPoint2d.resize(rectangle.size());
    float maxz = -1, maxy = -1, maxx = -1;
    for (int i = 0; i < point2d.size(); i++) {
        point2d[i] = rectangle[i].midPoint;
        if (rectangle[i].worldPos.x > maxx) {
            maxx = rectangle[i].worldPos.x;
        }
        if (rectangle[i].worldPos.z > maxz) {
            maxz = rectangle[i].worldPos.z;
            maxy = rectangle[i].worldPos.y;
        }
    }
    float width = maxx, height;
    for (int i = 0; i < point2d.size(); i++) {
        float x, y, z, h = 0;
        x = rectangle[i].worldPos.x; y = rectangle[i].worldPos.y; z = rectangle[i].worldPos.z;
        if (int(maxz - z) != 0) {
            h = sqrt(pow(maxy - y, 2) + pow(maxz - z, 2));
            height = h;
        }
        lookDownPoint2d[i] = Point2f(x, h);
    }
    cout << "point2d: \n" << point2d << "\nlookDownPoint2d: \n" << lookDownPoint2d << '\n';

    Size roomSize; roomSize.width = width; roomSize.height = height;
    homoImg = Mat::zeros(roomSize, CV_8UC3);
    H = findHomography(point2d, lookDownPoint2d);
    warpPerspective(img, homoImg, H, roomSize);
}

void solveHomo(const Mat &img, const vector<vector<Marker> > &rectangles, vector<Mat> &Hs, vector<Mat> &homoImgs) {
    Hs.resize(rectangles.size()); homoImgs.resize(rectangles.size());
    for (int i = 0; i < rectangles.size(); i++) {
        getH(img, rectangles[i], Hs[i], homoImgs[i]);
    }
}

void transformH(Point2f point2d, const Mat &H, Point2f &homoPoint2d) {
    Mat matp(3, 1, CV_64FC1), homomatp(3, 1, CV_64FC1); // note: matrices should have the same type(CV_...) during multiply
    matp.at<double>(0, 0) = point2d.x;
    matp.at<double>(0, 1) = point2d.y;
    matp.at<double>(0, 2) = 1;
    homomatp = H * matp;
    homomatp /= homomatp.at<double>(2);
    homoPoint2d.x = homomatp.at<double>(0); homoPoint2d.y = homomatp.at<double>(1);
}

void getFaces(string posFile, vector<Face> &faces) {
    FileStorage fs(posFile, FileStorage::READ);
    if (!fs.isOpened()) {
        cout << "Failed to open " << posFile << '\n';
    }
    vector<string> labels;
    FileNode node = fs["labels"];
    FileNodeIterator it = node.begin(), it_end = node.end();
    for (; it != it_end; ++it) {
        // cout << (string)*it << '\n';
        labels.push_back((string)*it);
    }
    node = fs["label_position"];
    for (int i = 0; i < labels.size(); i++) {
        if (labels[i] == ".NULL") continue;
        Mat position;
        node[labels[i]] >> position;
        Face face(labels[i], Point2f(position));
        faces.push_back(face);
    }
}

void getEmptySeat(const Mat &img, const vector<vector<Marker> > &rectangles, const vector<Mat> &Hs, vector<Mat> &homoImgs, vector<Seat> &seats) {
    vector<vector<Point2f> > rectanglesPoints; rectanglesPoints.resize(rectangles.size());
    for (int i = 0; i < rectanglesPoints.size(); i++) {
        rectanglesPoints[i].resize(rectangles[i].size());
        for (int j = 0; j < rectanglesPoints[i].size(); j++) {
            rectanglesPoints[i][j] = rectangles[i][j].midPoint;
        } 
    }
    while (true) {
        string winName = "Target points";
        vector<Point2f> clickPoints;
        namedWindow(winName, 1);
        setMouseCallback(winName, clickPoints_callback, &clickPoints);
        imshow(winName, img);
        waitKey(0);
        destroyWindow(winName);
        if (clickPoints.size()) {
            if (clickPoints.back() == Point2f(-1, -1)) {
                return;
            }
            Point2f point = clickPoints[0], homoPoint;
            int index = -1;
            for (int i = 0; i < rectanglesPoints.size(); i++) {
                if (inArea(point, rectanglesPoints[i])) {
                    index = i;
                    transformH(point, Hs[i], homoPoint);
                    // rectangle(homoImgs[i], Rect(homoPoint.x - 10, homoPoint.y - 10, 20, 20), Scalar(255, 255, 0), 1, 1, 0);
                    // imshow("homoImg", homoImgs[i]);
                    // waitKey(0);
                    // destroyWindow("homoImg");
                }
            }
            if (index != -1) {
                Seat seat(point, homoPoint, index);
                seats.push_back(seat);
            } else {
                Seat seat(point, Point2f(-1, -1), index);
                seats.push_back(seat);
            }
        }
    }
}

void getFacesSeat(const vector<vector<Point2f> > &rectanglesPoints, const vector<Mat> &Hs, vector<Mat> &homoImgs, vector<Face> &faces) {
    for (int i = 0; i < faces.size(); i++) {
        Point2f point = faces[i].pos;
        for (int j = 0; j < rectanglesPoints.size(); j++) {
            if (inArea(point, rectanglesPoints[j])) {
                Point2f homoPoint;
                transformH(point, Hs[j], homoPoint);
                faces[i].homoPos = homoPoint;
                rectangle(homoImgs[j], Rect(homoPoint.x - 10, homoPoint.y - 10, 20, 20), Scalar(255, 255, 0), 1, 1, 0);
                imshow("homoImg", homoImgs[j]);
                waitKey(0);
                destroyWindow("homoImg");
            }
        }
    }
}

// photo version
int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "wrong form: <fullFileName>\n";
        return 0;
    }

    build();

    Mat rawInputImg, inputImg; // image of classroom
    // Mat inputImg;
    string imgFile(argv[1]);
    rawInputImg = imread("../classroom/" + imgFile);
    // inputImg = imread("../classroom/" + imgFile);

    calibrate(rawInputImg, inputImg);

    // inputImg.convertTo(inputImg, -1, 2.5, 0);

    vector<Marker> markers;
    markersInfo(inputImg, markers, true);
    markersWorldPos(inputImg, markers);
    vector<vector<Marker> > rectangles;
    findRectangles(inputImg, markers, rectangles);

    /*
    Mat rvec, tvec;
    pnpSolver(rectangles, rvec, tvec);

    Mat rmat(3, 3, CV_64FC1);
    Rodrigues(rvec, rmat);

    vector<Point2f> projectedPoints;
    vector<Point3f> pj;
    for (int i = 0; i < markers.size(); i++) {
        pj.push_back(markers[i].worldPos);
    }
    projectPoints(pj, rmat, tvec, cameraMatrix, distMatrix, projectedPoints);
    for (int i = 0; i < markers.size(); i++) {
        int px = projectedPoints[i].x, py = projectedPoints[i].y;
        rectangle(inputImg, Rect(px - 100, py - 100, 200, 200), Scalar(255, 0, 0), 3);
    }
    imshow("classroom", inputImg);
    waitKey();
    */

    // cout << "Input image: ";
    // cin >> imgFile;
    // Mat img = imread("../classroom/" + imgFile);


    vector<Mat> Hs, homoImgs;
    vector<Seat> seats;
    solveHomo(inputImg, rectangles, Hs, homoImgs);
    getEmptySeat(inputImg, rectangles, Hs, homoImgs, seats);


    // seat color
    for (int i = 0; i < seats.size(); i++) {
        int index = seats[i].areaIndex;
        if (index == -1)
            continue;
        int mx = seats[i].homoPos.x, my = seats[i].homoPos.y, cnt = 0;
        int c1 = 0, c2 = 0, c3 = 0;
        for (int x = mx - 10; x <= mx + 10 && 0 <= x && x <= homoImgs[index].cols; x++) {
            for (int y = my - 10; y <= my + 10 && 0 <= y && y <= homoImgs[index].rows; y++) {
                if (homoImgs[index].at<Vec3b>(x, y) == Vec3b(255, 255, 0)) {
                    cout << x << ' ' << y << '\n';
                }
                // cout << homoImgs[index].at<Vec3b>(x, y) << '\n';
                // cout << int(homoImgs[index].at<Vec3b>(x, y)[0]) << ' ' << int(homoImgs[index].at<Vec3b>(x, y)[1]) << ' ' << int(homoImgs[index].at<Vec3b>(x, y)[2]) << '\n';
                c1 += homoImgs[index].at<Vec3b>(x, y)[0];
                c2 += homoImgs[index].at<Vec3b>(x, y)[1];
                c3 += homoImgs[index].at<Vec3b>(x, y)[2];
                cnt++;
                // cout << x << ' ' << y;
            }
        }
        c1 /= cnt; c2 /= cnt; c3 /= cnt;
        Vec3b sum(c1, c2, c3);
        cout << "sum: " << sum << '\n';
    }

    // float dx = 35, dy = 30;
    // float ix = homoImgs[0].cols / 5, iy = homoImgs[0].rows / 3;
    // for (int i = 1; i <= 3; i++) {
    //     line(homoImgs[0], Point2f(0, i * iy - dy), Point2f(homoImgs[0].cols, i * iy - dy), Scalar(0, 0, 255), 6);
    // }
    // for (int i = 1; i <= 5; i++) {
    //     line(homoImgs[0], Point2f(i * ix - dx, 0), Point2f(i * ix - dx, homoImgs[0].rows), Scalar(0, 0, 255), 6);
    // }
    // imshow("test", homoImgs[0]);
    // waitKey(0);

    // getEmptySeat(inputImg, rectangles, Hs, homoImgs, seats);

    // vector<vector<Point2f> > rectanglesPoints; rectanglesPoints.resize(rectangles.size());
    // for (int i = 0; i < rectanglesPoints.size(); i++) {
    //     rectanglesPoints[i].resize(rectangles[i].size());
    //     for (int j = 0; j < rectanglesPoints[i].size(); j++) {
    //         rectanglesPoints[i][j] = rectangles[i][j].midPoint;
    //     } 
    // }
    // vector<Face> faces;
    // getFaces("test2.xml", faces);
    // getFacesSeat(rectanglesPoints, Hs, homoImgs, faces);
}