#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>

using namespace std;
using namespace cv;

#define n_corner_cols 8
#define n_corner_rows 6
#define corner_size 25


void fprintMatrix(Mat matrix, string name)
{
 FILE * fp;
 fp = fopen(name.c_str(), "w");
 int i, j;
 printf("%s size %d %d\n", name.c_str(), matrix.cols, matrix.rows);
 for (i = 0; i< matrix.rows; ++i)
 {
  for (j = 0; j< matrix.cols; ++j)
  {
   fprintf(fp, "%lf ", matrix.at<  double >(i, j));
  }
  fprintf(fp, "\n");
 }

 fclose(fp);
}

void calibration(string folder_colorImg, string folder_calibration)
{
 vector <string> fileNames;
 for (int i = 0; i < 12; i++) {
    string fname = "IMG_" + to_string(4533 + i) + ".JPG";
    fileNames.push_back(fname);
}

 vector<vector<Point2f>> corner_all_2D;
 vector<vector<Point3f>> corner_all_3D;

 Size image_size = imread(folder_colorImg + fileNames[0], IMREAD_COLOR).size();

 for (int index_file = 0; index_file < fileNames.size(); index_file++)
 {
  cout << fileNames[index_file] << endl;

  //read files
  Mat img_gray = imread(folder_colorImg + fileNames[index_file], IMREAD_GRAYSCALE);
  Mat img_color = imread(folder_colorImg + fileNames[index_file], IMREAD_COLOR);

  //find corners 
  vector <Point2f> corner_per_2D;
  vector <Point3f> corner_per_3D;

  bool isFindCorner = findChessboardCorners(img_gray, Size(n_corner_cols, n_corner_rows), corner_per_2D, CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE);
  cout << "hihi\n";
  if (isFindCorner && (corner_per_2D.size() == n_corner_rows*n_corner_cols))
  {

   //2D corners
   cv::TermCriteria param(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 30, 0.1);
   cornerSubPix(img_gray, corner_per_2D, cv::Size(5, 5), cv::Size(-1, -1), param);

   drawChessboardCorners(img_color, Size(n_corner_cols, n_corner_rows), corner_per_2D, true);

   imshow("img color show", img_color);
   corner_all_2D.push_back(corner_per_2D);


   //3D corners
   for (int r = 0; r < n_corner_rows; r++)
   {
    for (int c = 0; c < n_corner_cols; c++)
    {
     corner_per_3D.push_back(Point3f(c*corner_size, r*corner_size, 0));
    }
   }
   corner_all_3D.push_back(corner_per_3D);

  }
  else
  {
   putText(img_color, "NO FOUND", Point(0, img_color.rows / 2), FONT_HERSHEY_DUPLEX, 3, Scalar(0, 0, 255));
   imshow("img color show", img_color);
  }
 }

 //camera calibration
 vector< Mat> rvecs, tvecs;
 Mat intrinsic_Matrix(3, 3, CV_64F);
 Mat distortion_coeffs(8, 1, CV_64F);

 calibrateCamera(corner_all_3D, corner_all_2D, image_size, intrinsic_Matrix, distortion_coeffs, rvecs, tvecs);
 cout << "Finished IMAGE Camera calibration" << endl;

 //save part
 fprintMatrix(intrinsic_Matrix, folder_calibration + "intrinsic.txt");
 fprintMatrix(distortion_coeffs, folder_calibration + "distortion.txt");

}


int main()
{             
    string folder_colorImg = "image/";

    string folder_calibration = "cal/";

    calibration(folder_colorImg, folder_calibration);

    std::system("pause");
    return 0;
}