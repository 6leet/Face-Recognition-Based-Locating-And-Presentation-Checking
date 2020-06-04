# USAGE
# python recognize_faces_image.py --encodings encodings.pickle --image examples/example_01.png 

# import the necessary packages
import face_recognition
import argparse
import pickle
import cv2

def outputPosition(labels, positions):
	if (len(labels) % 2 == 0):
		labels.append('_NULL')
		positions.append((-1, -1))
	label_position = dict(zip(labels, positions))
    
    # label list 
	fs = cv2.FileStorage("test2.xml", cv2.FILE_STORAGE_WRITE)
	fs.write('labels', '[')
	for i in range(int(len(labels) / 2)):
		fs.write(labels[i * 2], labels[i * 2+ 1])
	fs.write(labels[-1], ']')

	# mapping
	fs.write('label_position', '{')
	for label, position in label_position.items():
		if (label == ''): continue
		fs.write(label, position)
	fs.write('label_position', '}')

	fs.write('cnt_map', '{')
	for name, cnt in count.items():
		fs.write(name, cnt)
	fs.write('cnt_map', '}')

	fs.release()

# construct the argument parser and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-e", "--encodings", required=True,
	help="path to serialized db of facial encodings")
ap.add_argument("-i", "--image", required=True,
	help="path to input image")
ap.add_argument("-d", "--detection-method", type=str, default="cnn",
	help="face detection model to use: either `hog` or `cnn`")
args = vars(ap.parse_args())

# load the known faces and embeddings
print("[INFO] loading encodings...")
data = pickle.loads(open(args["encodings"], "rb").read())

# load the input image and convert it from BGR to RGB
image = cv2.imread(args["image"])
rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

# detect the (x, y)-coordinates of the bounding boxes corresponding
# to each face in the input image, then compute the facial embeddings
# for each face
print("[INFO] recognizing faces...")
boxes = face_recognition.face_locations(rgb,
	model=args["detection_method"])
encodings = face_recognition.face_encodings(rgb, boxes)

# initialize the list of names for each face detected
names = []

# loop over the facial embeddings
for encoding in encodings:
	# attempt to match each face in the input image to our known
	# encodings
	matches = face_recognition.compare_faces(data["encodings"],
		encoding, tolerance=0.48)
	name = "Unknown"

	# check to see if we have found a match
	if True in matches:
		# find the indexes of all matched faces then initialize a
		# dictionary to count the total number of times each face
		# was matched
		matchedIdxs = [i for (i, b) in enumerate(matches) if b]
		counts = {}

		# loop over the matched indexes and maintain a count for
		# each recognized face face
		count_dataset = {}
		for i in range(len(data["names"])):
			n = data["names"][i]
			count_dataset[n] = count_dataset.get(n, 0) + 1
			

		for i in matchedIdxs:
			name = data["names"][i]
			counts[name] = counts.get(name, 0) + 1/count_dataset[name]
			
		# determine the recognized face with the largest number of
		# votes (note: in the event of an unlikely tie Python will
		# select first entry in the dictionary)
		name = max(counts, key=counts.get)
	
	# update the list of names
	names.append(name)

count = {}
for i in range(len(names)):
	name = names[i]
	count[name] = count.get(name, 0) + 1

# loop over the recognized faces
labels = []
positions = []
for ((top, right, bottom, left), name) in zip(boxes, names):
	# draw the predicted face name on the image
	# print(left, top, right, bottom)
	cv2.rectangle(image, (left, top), (right, bottom), (0, 255, 0), 2)
	y = top - 15 if top - 15 > 15 else top + 15
	cv2.putText(image, name, (left, y), cv2.FONT_HERSHEY_SIMPLEX,
		0.75, (0, 255, 0), 2)
	labels.append(name)
	midx = (left + right) / 2
	midy = (top + bottom) / 2
	positions.append((midx, midy))

outputPosition(labels, positions)

# show the output image
# pyrDown( image, dst, Size( image.cols/2, image.rows/2 )
# height, width, channels = image.shape 
# print(height, width)
# res = cv2.resize(image, (width/5, height/5), interpolation = cv2.INTER_AREA)
cv2.namedWindow("Image", cv2.WINDOW_NORMAL)
cv2.imshow("Image", image)
cv2.waitKey(0)