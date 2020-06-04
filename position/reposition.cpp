// input: classroom size (n x m), 



#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>

using namespace cv;
using namespace cv::aruco;
using namespace std;
#define markerCnt 4

vector<int> ids;
vector<vector<int> > pnpMarker, allMarker;

vector<int> width, height;
Mat cameraMatrix, distMatrix;

struct Edge {
    Point2f s, t;
    float len;
    Point2f unitVec;
    Edge(Point2f _s, Point2f _t) {
        s = _s;
        t = _t;
        len = sqrt(pow(s.x - t.x, 2) + pow(s.y - t.y, 2));
        unitVec.x = (t.x - s.x) / len;
        unitVec.y = (t.y - s.y) / len;
    }
};

struct myVec {
    int id;
    Point2f unit;
    myVec(int _i, Point2f _s, Point2f _t) {
        id = _i;
        // cout << _s << ' ' << _t << '\n';
        float deltax = _t.x - _s.x, deltay = _t.y - _s.y;
        // cout << deltax << ' ' << deltay << '\n';
        float len = sqrt(pow(deltax, 2) + pow(deltay, 2));
        // cout << len << '\n';
        unit.x = deltax / len;
        unit.y = deltay / len;
        // cout << unit.x << ' ' << unit.y << '\n';
    }
    bool operator<(const myVec &r) const {
        if (unit.y >= 0 && r.unit.y < 0) {
            return false;
        } else if (unit.y >= 0 && r.unit.y >= 0) {
            return unit.x < r.unit.x;
        } else if (unit.y < 0 && r.unit.y >= 0) {
            return true;
        } else if (unit.y < 0 && r.unit.y < 0) {
            return unit.x > r.unit.x;
        }
    }
};

void calibrate(const Mat &src, Mat &dst) {
    Size imgSize = src.size();
    Mat mapx, mapy;
    initUndistortRectifyMap(cameraMatrix, distMatrix, Mat(), cameraMatrix, imgSize, CV_32F, mapx, mapy);
    remap(src, dst, mapx, mapy, INTER_LINEAR);
}

void RotateZ(double x, double y, double thetaz, double& outx, double& outy)
{
	double x1 = x;
	double y1 = y;
	double rz = thetaz * CV_PI / 180;
	outx = cos(rz) * x1 - sin(rz) * y1;
	outy = sin(rz) * x1 + cos(rz) * y1;
}

void RotateY(double x, double z, double thetay, double& outx, double& outz)
{
	double x1 = x;
	double z1 = z;
	double ry = thetay * CV_PI / 180;
	outx = cos(ry) * x1 + sin(ry) * z1;
	outz = cos(ry) * z1 - sin(ry) * x1;
}

void RotateX(double y, double z, double thetax, double& outy, double& outz)
{
	double y1 = y;
	double z1 = z;
	double rx = thetax * CV_PI / 180;
	outy = cos(rx) * y1 - sin(rx) * z1;
	outz = cos(rx) * z1 + sin(rx) * y1;
}

void build() {
    int _ids[4] = {1, 2, 3, 4};
    // should set properly
    int _pnpMarker[4][3] = { {300, 0, 0},
                            {300, 54, 330},
                            {0, 54, 330},
                            {0, 0, 0} };
    int _allMarker[6][3] = { {300, 54, 330},
                            {300, 178, 770},
                            {0, 178, 770},
                            {0, 54, 330},
                            {0, 0, 0},
                            {300, 0, 0}};
    ids.resize(4);
    for (int i = 0; i < markerCnt; i++) {
        ids[i] = _ids[i];
    }

    pnpMarker.resize(4);
    for (int i = 0; i < markerCnt; i++) {
        pnpMarker[i].resize(3);
        for (int j = 0; j < 3; j++) {
            pnpMarker[i][j] = _pnpMarker[i][j];
        }
    }
    allMarker.resize(6);
    for (int i = 0; i < 6; i++) {
        allMarker[i].resize(3);
        for (int j = 0; j < 3; j++) {
            allMarker[i][j] = _allMarker[i][j];
        }
    }
    string calFile = "/Users/leolee/documents/108_2/lab_project/calibration/iphoneCal.xml";
    FileStorage fs(calFile, FileStorage::READ);
    if (!fs.isOpened()) {
        cout << "Failed to open " << calFile << '\n';
    }
    fs["intrinsic"] >> cameraMatrix;
    fs["distortion"] >> distMatrix;
}

void buildRoom(int width, int height, Size &roomSize, vector<Point2f> &lookDownPoint2d) {
    roomSize.width = width; roomSize.height = height;
    
    lookDownPoint2d.push_back(Point2f(width - 1, height - 1));
    lookDownPoint2d.push_back(Point2f(width - 1, 0));
    lookDownPoint2d.push_back(Point2f(0, 0));
    lookDownPoint2d.push_back(Point2f(0, height - 1));

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

void allocate(const vector<Point2f> &point2d, vector<Point2f> &allocPoint) {
    Point2f midPoint = getMidPoint(point2d);

    allocPoint.resize(point2d.size());
    vector<myVec> vecs;
    for (int i = 0; i < point2d.size(); i++) {
        myVec vec(i, midPoint, point2d[i]);
        vecs.push_back(vec);
    }
    sort(vecs.begin(), vecs.end());
    for (int i = 0; i < vecs.size(); i++) {
        allocPoint[i] = point2d[vecs[i].id];
    }
}

void solveEdge(const vector<Point2f> &point2d, vector<Edge> &edges) { // input: point2d, output: edges (buttom, right, top, left)
    vector<Point2f> allocPoint;
    allocate(point2d, allocPoint);
    for (int i = 0; i < allocPoint.size(); i++) {
        Edge e(allocPoint[i], allocPoint[(i + 1) % 4]);
        edges.push_back(e);
    }
}

float solveGeometric(const Edge edge, const float alpha, const int n) {
    if (alpha == 1) {
        return edge.len / n;
    } else {
        return edge.len * (1 - alpha) / (1 - pow(alpha, n));
    }
}

void findMarkers(Mat &img, vector<int> &markerIds, vector<vector<Point2f> > &markerCorners) { // input: img, output: markerIds, markerCorners
    vector<vector<Point2f> > rejectedCandidates;
    Ptr<DetectorParameters> parameters = DetectorParameters::create();
    Ptr<Dictionary> dictionary = getPredefinedDictionary(DICT_6X6_250); // dictionary that map's to marker's id.
    detectMarkers(img, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
}

void getRectangles(const vector<Point2f> &allocPoint2d, vector<vector<Point2f> > &rectanglePoint2d) { // break up into rectangles
    // input: allocPoint2d, output: rectanglePoint2d
    if (allocPoint2d.size() == 4) {
        rectanglePoint2d.resize(1);
        for (int i = 0; i < 4; i++) {
            rectanglePoint2d[0].push_back(allocPoint2d[i]);
        }
    } else if (allocPoint2d.size() == 6) {
        rectanglePoint2d.resize(2);
        int seq[2][4] = {{0, 1, 2, 3}, 
                        {5, 0, 3, 4}};
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                rectanglePoint2d[i].push_back(allocPoint2d[seq[i][j]]);
            }
        }
    }
}

void getH(const Mat &img, int width, int height, const vector<Point2f> &allocPoint2d, Mat &H, Mat &homoImg) {
    // input: img, (width, height of classroom), allocPoint2d (dl, dr, ur, ul)
    // output: homo, homoImg
    Size roomSize;
    vector<Point2f> lookDownPoint2d;
    buildRoom(width, height, roomSize, lookDownPoint2d);
    homoImg = Mat::zeros(roomSize, CV_8UC3);
    H = findHomography(allocPoint2d, lookDownPoint2d);
    warpPerspective(img, homoImg, H, roomSize);
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

float findArea(const Point2f &pointA, const Point2f &pointB, const Point2f &pointC) { // get triangle area
    return abs((pointA.x * (pointB.y - pointC.y) + pointB.x * (pointC.y - pointA.y) + pointC.x * (pointA.y - pointB.y)) / 2.0);
}

bool inArea(const Point2f &point2d, const vector<Point2f> &rectanglePoint2d) { // check if a point is in a rectangle
    // input: point2d, rectanglePoint2d
    float eps = 1;
    float recArea = findArea(rectanglePoint2d[0], rectanglePoint2d[1], rectanglePoint2d[2]) +
                    findArea(rectanglePoint2d[0], rectanglePoint2d[2], rectanglePoint2d[3]);
    float area, areaSum = 0;
    for (int i = 0; i < 4; i++) {
        area = findArea(point2d, rectanglePoint2d[i], rectanglePoint2d[(i + 1) % 4]);
        areaSum += area;
    }
    return abs(recArea - areaSum) < eps;
}

void pnpSolver(const vector<Point2f> &point2d, Mat &rvec, Mat &tvec) {
    vector<Point3f> point3d;
    point3d.resize(4);
    for (int i = 0; i < point3d.size(); i++) {
        point3d[i] = Point3f(pnpMarker[i][0], pnpMarker[i][1], pnpMarker[i][2]);
    }
    rvec = Mat::zeros(3, 1, CV_64FC1);
    tvec = Mat::zeros(3, 1, CV_64FC1);
    solvePnP(point3d, point2d, cameraMatrix, distMatrix, rvec, tvec, false, CV_ITERATIVE);
}

// photo version
int main(int argc, char *argv[]) {
    if (argc <= 4 || argc % 2 != 0) {
        cout << "wrong form: <fullFileName> <width0> <height0> ...\n";
        return 0;
    }
    build();
    for (int i = 2; i < argc; ) {
        width.push_back(atoi(argv[i++]));
        height.push_back(atoi(argv[i++]));
    }

    Mat rawInputImg, inputImg; // image of classroom
    string imgFile(argv[1]);
    rawInputImg = imread("../classroom/" + imgFile);

    calibrate(rawInputImg, inputImg);

    vector<int> markerIds;
    vector<vector<Point2f> > markerCorners;
    findMarkers(inputImg, markerIds, markerCorners);
    drawDetectedMarkers(inputImg, markerCorners, markerIds);
    // imshow("markers", outputImage);
    // waitKey(0);

    vector<Point2f> point2d; // centers of each marker
    cout << "markerCorners: \n";
    for (int i = 0; i < markerCorners.size(); i++) { // four markers
        Point2f mp = getMidPoint(markerCorners[i]);
        point2d.push_back(mp);
        cout << mp << '\n';
    }

    vector<Point2f> allocPoint2d;
    allocate(point2d, allocPoint2d);

    vector<vector<Point2f> > rectanglePoint2d;
    getRectangles(allocPoint2d, rectanglePoint2d);

    Mat rvec, tvec;
    pnpSolver(rectanglePoint2d.back(), rvec, tvec);

    // double r[3][3];
    // Mat rmat(3, 3, CV_64FC1, r);
    // Rodrigues(rvec, rmat);

    vector<Point2f> projectedPoints;
    vector<Point3f> pj;
    for (int i = 0; i < 6; i++) {
        pj.push_back(Point3f(allMarker[i][0], allMarker[i][1] + 20, allMarker[i][2]));
    }
    projectPoints(pj, rvec, tvec, cameraMatrix, distMatrix, projectedPoints);
    for (int i = 0; i < point2d.size(); i++) {
        int px = projectedPoints[i].x, py = projectedPoints[i].y;
        rectangle(inputImg, Rect(px - 100, py - 100, 200, 200), Scalar(255, 0, 0), 3);
    }
    imshow("classroom", inputImg);
    waitKey();

    // vector<vector<Point2f> > rectanglePoint2d;
    rectanglePoint2d.clear();
    getRectangles(projectedPoints, rectanglePoint2d);

    for (int i = 0; i < rectanglePoint2d.size(); i++) {
        cout << rectanglePoint2d[i] << '\n';
    }

    vector<Mat> H, homoImg;
    H.resize(width.size()); homoImg.resize(width.size());
    for (int i = 0; i < width.size(); i++) {
        getH(inputImg, width[i], height[i], rectanglePoint2d[i], H[i], homoImg[i]);
    }

    Point2f tmp = getMidPoint(projectedPoints);
    rectangle(inputImg, Rect(tmp.x - 100, tmp.y - 100, 200, 200), Scalar(255, 255, 0), 1, 1, 0);
    imshow("test", inputImg);
    waitKey();
    for (int i = 0; i < width.size(); i++) {
        if (inArea(tmp, rectanglePoint2d[i])) {
            Point2f homotmp;
            transformH(tmp, H[i], homotmp);
            rectangle(homoImg[i], Rect(homotmp.x - 10, homotmp.y - 10, 20, 20), Scalar(255, 255, 0), 1, 1, 0);
            imshow("homoImg", homoImg[i]);
            waitKey();
            
        }
    }
    // visualize 
    // for (int i = 0; i < edges.size(); i++) {
    //     line(inputImg, edges[i].s, edges[i].t, Scalar(90, 90, 90), 3);
    // }
    // float n[4] = {6, 7, 6, 7};
    // float alpha[4] = {1.2, 0.7, 0.8, 1.5};
    // for (int i = 0; i < edges.size(); i++) {
    //     float x = solveGeometric(edges[i], alpha[i], n[i]);
    //     line(inputImg, edges[i].s, edges[i].s + x * edges[i].unitVec, Scalar(0, 0, 255), 6);
    // }

    // float iy = 770 / 178.0;
    // for (int i = 1; i <= 3; i++) {
    //     line(homoImg, Point2f(0, 770 - i * 18 * iy), Point2f(360, 770 - i * 18 * iy), Scalar(0, 0, 255), 6);
    // }
    // float c = 3 * 18 * iy;
    // for (int i = 1; i <= 4; i++) {
    //     line(homoImg, Point2f(0, 770 - i * 31 * iy - c), Point2f(360, 770 - i * 31 * iy - c), Scalar(0, 0, 255), 6);
    // }
}