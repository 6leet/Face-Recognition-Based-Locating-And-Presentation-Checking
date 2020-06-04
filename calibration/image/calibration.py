import numpy as np
import cv2
import glob

# imageCount = 5
# cam = cv2.VideoCapture(0)
# cnt = 0
# while cnt < imageCount:

#     ret, ori = cam.read()
#     cv2.imshow("Video", ori)
#     k = cv2.waitKey(1)
#     if k & 0xFF == 27:
#         break
#     elif k & 0xFF == ord("c"):
#         cv2.imwrite("image" + str(cnt) + ".jpg", ori)
#         cnt = cnt + 1
# cam.release()

# cv2.destroyAllWindows()
cw = 8
ch = 6

objp = np.zeros((cw*ch,3), np.float32)
objp[:,:2] = np.mgrid[0:cw,0:ch].T.reshape(-1,2)
criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
op = []
ip = []

for file in glob.glob("IMG_*.JPG"):
    print(file)
    img = cv2.imread(file)
    gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
    
    ret, corners = cv2.findChessboardCorners(gray, (cw, ch), None)

    print(ret)
    if ret == True:
        op.append(objp)

        imgp = cv2.cornerSubPix(gray, corners, (11,11), (-1,-1), criteria)
        ip.append(imgp)

        img = cv2.drawChessboardCorners(img, (cw,ch), imgp, ret)
        cv2.imshow('img',img)


cv2.waitKey(0)
cv2.destroyAllWindows()

ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(op, ip, gray.shape[::-1], None, None)

f = cv2.FileStorage("calibration.xml", cv2.FILE_STORAGE_WRITE)
f.write("intrinsic", mtx)
f.write("distortion", dist)
f.release()

