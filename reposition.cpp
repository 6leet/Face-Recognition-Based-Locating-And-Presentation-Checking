// input: classroom size (n x m), 




#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>

using namespace cv;
using namespace cv::aruco;
using namespace std;
#define markerCnt 4

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

void buildRoom(int width, int height, Size &roomSize, vector<Point2f> &lookDownPoint2d) {
    roomSize.width = width; roomSize.height = height;
    lookDownPoint2d.push_back(Point2f(0, height - 1));
    lookDownPoint2d.push_back(Point2f(width - 1, height - 1));
    lookDownPoint2d.push_back(Point2f(width - 1, 0));
    lookDownPoint2d.push_back(Point2f(0, 0));

}

Point2f getMidPoint(const vector<Point2f> &point2d) { // input: point2d, output: midpoint
    Point2f mp;
    for (int i = 0; i < point2d.size(); i++) {
        mp.x += point2d[i].x;
        mp.y += point2d[i].y;
    }
    mp.x /= 4; mp.y /= 4;
    return mp;
}

void allocate(const vector<Point2f> &point2d, vector<Point2f> &allocPoint) { // input: point2d, output: alloc_point
    Point2f midPoint = getMidPoint(point2d);

    allocPoint.resize(markerCnt);
    for (int i = 0; i < point2d.size(); i++) {
        if (point2d[i].x <= midPoint.x && point2d[i].y >= midPoint.y) { // down-left
            allocPoint[0] = point2d[i];
        } else if (point2d[i].x >= midPoint.x && point2d[i].y >= midPoint.y) { // down-right
            allocPoint[1] = point2d[i];
        } else if (point2d[i].x >= midPoint.x && point2d[i].y <= midPoint.y) { // up-right
            allocPoint[2] = point2d[i];
        } else if (point2d[i].x <= midPoint.x && point2d[i].y <= midPoint.y) { // up-left
            allocPoint[3] = point2d[i];
        }
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

// photo version
int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "without filename: <fullFileName>\n";
        return 0;
    }

    Mat inputImg; // image of classroom
    string imgFile(argv[1]);
    inputImg = imread(imgFile);

    vector<int> markerIds;
    vector<vector<Point2f> > markerCorners;
    findMarkers(inputImg, markerIds, markerCorners);

    // Mat outputImage = inputImg.clone();
    // drawDetectedMarkers(outputImage, markerCorners, markerIds);

    vector<Point2f> point2d; // centers of each marker
    for (int i = 0; i < markerCorners.size(); i++) { // four markers
        cout << markerCorners[i] << '\n';
        Point2f mp = getMidPoint(markerCorners[i]);
        point2d.push_back(mp);
    }

    map<int, int> id_to_index;
    for (int i = 0; i < markerIds.size(); i++) {
        id_to_index[markerIds[i]] = i;
    }

    for (int i = 0; i < markerIds.size(); i++) {
        cout << "markerId: " << markerIds[i] << '\n';
        cout << "point2d: " << point2d[i] << '\n';
        cout << '\n';
    }

    vector<Edge> edges;
    solveEdge(point2d, edges);
    for (int i = 0; i < edges.size(); i++) {
        cout << "s: " << edges[i].s.x << ' ' << edges[i].s.y << '\n';
        cout << "t: " << edges[i].t.x << ' ' << edges[i].t.y << '\n';
        cout << "len: " << edges[i].len << '\n';
        cout << "vec: " << edges[i].unitVec << '\n';
    }

    vector<Point2f> allocPoint2d;
    allocate(point2d, allocPoint2d);
    Mat H, homoImg;
    getH(inputImg, 360, 770, allocPoint2d, H, homoImg);

    Point2f tmp = getMidPoint(point2d);

    // visualize 
    for (int i = 0; i < edges.size(); i++) {
        line(inputImg, edges[i].s, edges[i].t, Scalar(90, 90, 90), 3);
    }
    float n[4] = {6, 7, 6, 7};
    float alpha[4] = {1.2, 0.7, 0.8, 1.5};
    for (int i = 0; i < edges.size(); i++) {
        float x = solveGeometric(edges[i], alpha[i], n[i]);
        line(inputImg, edges[i].s, edges[i].s + x * edges[i].unitVec, Scalar(0, 0, 255), 6);
    }

    rectangle(inputImg, Rect(tmp.x - 100, tmp.y - 100, 200, 200), Scalar(255, 255, 0), 1, 1, 0);
    imshow("test", inputImg);
    waitKey();

    Point2f homotmp;
    transformH(tmp, H, homotmp);
    rectangle(homoImg, Rect(homotmp.x - 10, homotmp.y - 10, 20, 20), Scalar(255, 255, 0), 1, 1, 0);
    float iy = 770 / 178.0;
    for (int i = 1; i <= 3; i++) {
        line(homoImg, Point2f(0, 770 - i * 18 * iy), Point2f(360, 770 - i * 18 * iy), Scalar(0, 0, 255), 6);
    }
    float c = 3 * 18 * iy;
    for (int i = 1; i <= 4; i++) {
        line(homoImg, Point2f(0, 770 - i * 31 * iy - c), Point2f(360, 770 - i * 31 * iy - c), Scalar(0, 0, 255), 6);
    }
    imshow("homoImg", homoImg);
    waitKey();

    // while (true) {
    //     int x, y, z;
    //     cout << "x = ";
    //     cin >> x;
    //     cout << "y = ";
    //     cin >> y;
    //     cout << "z = ";
    //     cin >> z;
    //     if (x == 0 && y == 0 && z == 0) break;
    //     vector<Point3f> pj;
    //     pj.push_back(Point3f(x, y, z));
    //     projectPoints(pj, rvec, tvec, cameraMatrix, distMatrix, projectedPoints);
    //     int px = projectedPoints[0].x, py = projectedPoints[0].y;
    //     rectangle(inputImg, Rect(px - 100, py - 100, 200, 200), Scalar(255, 0, 0), 1, 1, 0);
    //     imshow("classroom", inputImg);
    //     waitKey();
    // }
}