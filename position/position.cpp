#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <cstring>
#include <vector>
#include <map>

using namespace cv;
using namespace cv::aruco;
using namespace std;
#define markerCnt 4

vector<int> ids;
vector<vector<int> > worldPos;

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
    int _ids[markerCnt] = {1, 2, 3, 4};
    // should set properly
    int _worldPos[markerCnt][3] = { {45, 78, 240}, 
                                    {345, 78, 240},
                                    {45, 246, 1010},
                                    {345, 246, 1010} };

    ids.resize(markerCnt);
    for (int i = 0; i < markerCnt; i++) {
        ids[i] = _ids[i];
    }

    worldPos.resize(markerCnt);
    for (int i = 0; i < markerCnt; i++) {
        worldPos[i].resize(3);
        for (int j = 0; j < 3; j++) {
            worldPos[i][j] = _worldPos[i][j];
        }
    }
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

    // Mat mapx, mapy;
    // calibrateCamera(dstPoints, srcPoints, imgSize, cameraMatrix, distMatrix, rvecs, tvecs);
    // initUndistortRectifyMap(cameraMatrix, distMatrix, Mat(), cameraMatrix, imgSize, CV_32F, mapx, mapy);
    // remap(inputImg, calImg, mapx, mapy, INTER_LINEAR);

    vector<int> markerIds;
    vector<vector<Point2f> > markerCorners, rejectedCandidates;
    Ptr<DetectorParameters> parameters = DetectorParameters::create();
    Ptr<Dictionary> dictionary = getPredefinedDictionary(DICT_6X6_250); // map to marker's id.
    detectMarkers(inputImg, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

    Mat outputImage = inputImg.clone();
    drawDetectedMarkers(outputImage, markerCorners, markerIds);
    // imshow("ids", outputImage);
    // waitKey();

    // drawDetectedMarkers(outputImage, rejectedCandidates, markerIds);
    // imshow("ids", outputImage);

    vector<Point2f> point2d; // centers of each marker
    // point2d.resize(markerCnt);
    for (int i = 0; i < markerCorners.size(); i++) { // four markers
        cout << markerCorners[i] << '\n';
        int sumx = 0, sumy = 0;
        for (int j = 0; j < 4; j++) { // four corners
            sumx += markerCorners[i][j].x;
            sumy += markerCorners[i][j].y;
        }
        point2d.push_back(Point2f(sumx / 4, sumy / 4));
    }
    // point2d[0] = Point2f(103, 717);
    // point2d[2] = Point2f(1187, 185);
    // point2d[1] = Point2f(1747, 153);
    // point2d[3] = Point2f(1428, 838);

    map<int, int> id_to_index;
    for (int i = 0; i < markerIds.size(); i++) {
        id_to_index[markerIds[i]] = i;
    }

    build();

    cout << "check\n";
    vector<Point3f> point3d;
    point3d.resize(markerCnt);
    for (int i = 0; i < point3d.size(); i++) {
        point3d[id_to_index[ids[i]]] = Point3f(worldPos[i][0], worldPos[i][1], worldPos[i][2]);
    }
    // point3d[0] = Point3f(20000, 0, 30000);
    // point3d[2] = Point3f(20000, 80000, 130000);
    // point3d[1] = Point3f(70000, 80000, 130000);
    // point3d[3] = Point3f(70000, 0, 30000);

    for (int i = 0; i < markerIds.size(); i++) {
        cout << "markerId: " << markerIds[i] << '\n';
        cout << "point2d: " << point2d[i] << '\n';
        cout << "point3d: " << point3d[i] << '\n';
        cout << '\n';
    }

    string calFile = "iphoneCal.xml";
    FileStorage fs(calFile, FileStorage::READ);
    if (!fs.isOpened()) {
        cout << "Failed to open " << calFile << '\n';
    }

    Mat cameraMatrix, distMatrix;
    fs["intrinsic"] >> cameraMatrix;
    fs["distortion"] >> distMatrix;

    // vector<Vec3d> rvecs, tvecs;
    // estimatePoseSingleMarkers(markerCorners, 0.05, cameraMatrix, distMatrix, rvecs, tvecs);
    // for (int i = 0; i < rvecs.size(); ++i) {
    //     auto rvec = rvecs[i];
    //     auto tvec = tvecs[i];
    //     cout << "markerId: " << markerIds[i] << '\n';
    //     cout << "rvec: " << rvecs[i] << '\n';
    //     cout << "tvec: " << tvecs[i] << '\n';
    //     cout << '\n';
    //     // drawAxis(outputImage, cameraMatrix, distMatrix, rvec, tvec, 0.1); // X: red, Y: green, Z: blue
    // }

    Mat rvec = Mat::zeros(3, 1, CV_64FC1);
    Mat tvec = Mat::zeros(3, 1, CV_64FC1);
    solvePnP(point3d, point2d, cameraMatrix, distMatrix, rvec, tvec, false, CV_ITERATIVE);

    cout << rvec << '\n';
    cout << tvec << '\n';

    double r[3][3];
    Mat rmat(3, 3, CV_64FC1, r);
    Rodrigues(rvec, rmat);
    
    double thetaz, thetay, thetax;
    thetaz = atan2(r[1][0], r[0][0]) / CV_PI * 180;
    thetay = atan2(-r[2][0], sqrt(r[2][1] * r[2][1] + r[2][2] * r[2][2])) / CV_PI * 180;
    thetax = atan2(r[2][1], r[2][2]) / CV_PI * 180;

    FileStorage rot_fs("pnp_theta.xml", FileStorage::WRITE);
    rot_fs << "thetax" << thetax;
    rot_fs << "thetay" << thetay;
    rot_fs << "thetaz" << thetaz;

    double x, y, z;
    x = tvec.ptr<double>(0)[0];
	y = tvec.ptr<double>(0)[1];
	z = tvec.ptr<double>(0)[2];

    RotateZ(x, y, -thetaz, x, y);
	RotateY(x, z, -thetay, x, z);
	RotateX(y, z, -thetax, y, z);

    cout << "camera position: " << -x << ", " << -y << ", " << -z << '\n';
    imshow("classroom", inputImg);

    vector<cv::Point2f> projectedPoints;
	// point3d.push_back(Point3f(285, 120, 500));
	// projectPoints(point3d, rvec, tvec, cameraMatrix, distMatrix, projectedPoints);
    // cout << projectedPoints << '\n';
    // waitKey();

    while (true) {
        int x, y, z;
        cout << "x = ";
        cin >> x;
        cout << "y = ";
        cin >> y;
        cout << "z = ";
        cin >> z;
        if (x == 0 && y == 0 && z == 0) break;
        vector<Point3f> pj;
        pj.push_back(Point3f(x, y, z));
        projectPoints(pj, rvec, tvec, cameraMatrix, distMatrix, projectedPoints);
        int px = projectedPoints[0].x, py = projectedPoints[0].y;
        rectangle(inputImg, Rect(px - 100, py - 100, 200, 200), Scalar(255, 0, 0), 1, 1, 0);
        imshow("classroom", inputImg);
        waitKey();
    }

    // cout << projectedPoints[4].x << ' ' << projectedPoints[4].y << '\n';
    // int px = projectedPoints[4].x, py = projectedPoints[4].y;
    // rectangle(inputImg, Rect(px - 200, py - 200, 400, 400), Scalar(255, 0, 0), 1, 1, 0);
    // imshow("classroom", inputImg);
    // Mat subImg = inputImg(Range(py - 200, py + 200), Range(px - 200, px + 200));
    // imshow("subImg", subImg);
    // waitKey();
}