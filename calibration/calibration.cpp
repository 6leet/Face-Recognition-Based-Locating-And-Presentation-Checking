#include <iostream>
#include <opencv2/opencv.hpp>
#include <cstring>
#include <vector>

using namespace cv;
using namespace std;

class CameraCalibrator {
private:
    int cal_cnt, img_cnt;
    vector<string> filenames;
    Size borderSize;
    vector<vector<Point3f> > dstPoints;
    vector<vector<Point2f> > srcPoints;
public:
    void setFileNames(int ccnt, int icnt, int firstFile) {
        cal_cnt = ccnt;
        img_cnt = icnt;
        filenames.clear();
        for (int i = 0; i < img_cnt; i++) {
            string fname = "IMG_" + to_string(firstFile + i) + ".JPG";
            filenames.push_back(fname);
            cout << fname << '\n';
        }
        cout << "setFileNames done\n";
    }
    void setBorderSizeAndCount(const Size &bsize) {
        borderSize = bsize;
    }
    void addBoardPoints() {
        vector<Point3f> dstCorners;
        vector<Point2f> srcCorners;
        for (int i = 0; i < borderSize.height; i++) {
            for (int j = 0; j < borderSize.width; j++) {
                dstCorners.push_back(Point3f(i, j, 0.0));
                cout << dstCorners.back() << '\n';
            }
        }
        cout << "hihi\n";
        for (int i = 0; i < filenames.size(); i++) {
            Mat img = imread(filenames[i], CV_LOAD_IMAGE_GRAYSCALE);
            cout << filenames[i] << " finding\n";
            bool found = findChessboardCorners(img, borderSize, srcCorners, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);
            if (!found) {
                cout << filenames[i] << " not found\n";
                continue;
            }
            cout << "checkpoint\n";
            drawChessboardCorners(img, borderSize, srcCorners, true);
            imshow("img color show", img);
            waitKey();
            TermCriteria param(TermCriteria::MAX_ITER + TermCriteria::EPS, 30, 0.1);
            cornerSubPix(img, srcCorners, Size(5, 5), Size(-1, -1), param);
            if (srcCorners.size() == borderSize.area()) {
                srcPoints.push_back(srcCorners);
                dstPoints.push_back(dstCorners);
            }
        }
    }
    void calibrate(const Mat &src, Mat &dst, string calFileName) {
        Size imgSize = src.size();
        Mat cameraMatrix; // 內部參數
        Mat distMatrix; //畸變參數
        Mat mapx, mapy;
        vector<Mat> rvecs, tvecs; // rotation, translation of each image.
        calibrateCamera(dstPoints, srcPoints, imgSize, cameraMatrix, distMatrix, rvecs, tvecs);
        initUndistortRectifyMap(cameraMatrix, distMatrix, Mat(), cameraMatrix, imgSize, CV_32F, mapx, mapy);
        remap(src, dst, mapx, mapy, INTER_LINEAR);

        // string filename = "calibration_" + to_string(cal_cnt) + ".xml";
        FileStorage fs(calFileName, FileStorage::WRITE);
        fs << "intrinsic" << cameraMatrix;
        fs << "distortion" << distMatrix;
    }
    void fsWrite(const Mat &src) {
        Size imgSize = src.size();
        Mat cameraMatrix, distMatrix;
        vector<Mat> rvecs, tvecs;
        calibrateCamera(dstPoints, srcPoints, imgSize, cameraMatrix, distMatrix, rvecs, tvecs);

        string filename = "calibration_" + to_string(cal_cnt) + ".xml";
        FileStorage fs(filename, FileStorage::WRITE);
        fs << "intrinsic" << cameraMatrix;
        fs << "distortion" << distMatrix;
    }
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "without image info: <firstFileNo> <size>\n";
        return 0;
    }
    int cal_cnt = 1, img_cnt = atoi(argv[2]), firstFile = atoi(argv[1]);

    // FileStorage fs("counter.xml", FileStorage::READ);
    // if (!fs.isOpened()) {
    //     cout << "Can't calibrate without initial image.\n";
    //     return -1;
    // } else {
    //     fs["calibration_count"] >> cal_cnt;
    //     fs["image_count"] >> img_cnt;
    //     // fs << "calibration_count" << cal_cnt + 1;
    // }

    CameraCalibrator cc;
    cc.setFileNames(cal_cnt, img_cnt, firstFile);
    cc.setBorderSizeAndCount(Size(8, 6)); // Size(col, row)
    cout << "setbordersizeandcount\n";
    cc.addBoardPoints();

    cout << "check\n";
    for (int i = 0; i < img_cnt; i++) {
        Mat src, dst;
        // src = ardrone.getImage();
        string filename = "IMG_" + to_string(firstFile + i) + ".JPG";
        src = imread(filename);
        cc.calibrate(src, dst, "iphoneCal.xml");
        // cc.fsWrite(src);

        // imshow("src", src);
        // imshow("dst", dst);
        // waitKey();
    }
}