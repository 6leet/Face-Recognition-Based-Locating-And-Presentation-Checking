import cv2
import numpy as np

def outputPosition(labels, positions):
    if (len(labels) % 2 == 0):
        labels.append('')
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
    fs.release()

labels = ['lee', 'lee', 'test']
positions = [(300, 400), (800, 700), (0, 0)]
outputPosition(labels, positions)