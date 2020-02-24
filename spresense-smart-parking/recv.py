import serial
import time
import base64
import numpy as np
import array as arr
import sys

import matplotlib
import matplotlib.pyplot as plt
import matplotlib.cm as cm

import threading
from matplotlib.widgets import Button

import matplotlib.patches as patches

startmark = b"$$$$$$$$"
startmark_data = b"DDDDDDDD"



#plt.ion()

#fig, ax = plt.subplots()
fig = plt.figure()
ax = fig.add_subplot(111)

parking = []



file_number = 0

def StartApp(ser):
	ser.write(b'\n')
	time.sleep(1)
	ser.write(b'\nsmart_parking\n') # Start the application
	time.sleep(0.5)
	ser.write(b'REQ_IMG\r')
	#ser.write(chr(13))


def WaitForImg(ser):
	global parking
	#im = ax.imshow(rgb_mnx)
	rgb_mnx = np.array([0.5,0.5,0.5])
	rgb_mnx = rgb_mnx.reshape(1, 1, 3)
#	ax.set_ylim(ymin=0, ymax=240)
#	ax.set_xlim(xmin=0, xmax=320)
	#im = ax.imshow(rgb_mnx)

	while True:
		line = ser.readline()
		if startmark in line:
			print("Found keyword")

			im = 1
			header= str(line)
			tokens = header.split(',')
			print(tokens)
			size = int(tokens[2])
			width = int(tokens[3])
			height = int(tokens[4])

			print(size, width, height)
			ReceiveImg(im, ser, size, width, height)
			break
		if startmark_data in line:
			print("Found keyword Data")
			header= str(line)
			tokens = header.split(',')
			size = int(tokens[1])
			print("Receive data: " + str(size))
			parking = ReceiveData( ser, size)
			return
		else:
			print("SPRESENSE$ " + line.decode('utf-8'), end='')

def ReceiveData( ser, size):
	global parking
	pre_configured_parking = np.zeros((size,6))

	for b in range(size):
		rec = ser.readline()
		line = rec.decode("utf-8")
		try:
			tokens = line.split(',')
			pre_configured_parking[b] = [int(token) for token in tokens[0:6:]]

		except Exception:
			print("error at " + str(b))
			continue

	return pre_configured_parking
	time.sleep(1)


def ReceiveImg(im, ser, size, width, height):
	global file_number
	global ax
	filename = "smart_parking_" + format(file_number, '05d') +  ".data"
	imgFile = open(filename, "w+b");

	print('Will receive ' + str(size) + ' pixels -> ' + filename)


	rgb_px = np.zeros([width*height, 3])
#	print(rgb_px)

	val = np.uint16
	px16 = arr.array('H')

	line = 0

	for b in range(size):
		px = ser.readline()
		try:
			val = int(px, 16)
			px16.append(val)
			rgb_px[b,0] = ((val >> (5+6)) & 0x1f)/0x1f
			rgb_px[b,1] = ((val >> (5)) & 0x3f)/0x3f
			rgb_px[b,2] = (val  & 0x1f)/0x1f

		except Exception:
			continue

	imgFile.write(px16)
	imgFile.close()

	print("Done writing file: " + filename)
	rgb_mnx = rgb_px.reshape(height, width, 3)
	# close port
	print(ax)
	im = ax.imshow(rgb_mnx)
	im.set_array(rgb_mnx)
	print(ax)




	file_number = file_number + 1

def get_image(ser):
	WaitForImg(ser)




def onclick(event):
	print('%s click: button=%d, x=%d, y=%d, xdata=%f, ydata=%f' %('double' if event.dblclick else 'single', event.button,		event.x, event.y, event.xdata, event.ydata))

	plt.draw()

def update_cb(event):
	    print('%s click: button=%d, x=%d, y=%d, xdata=%f, ydata=%f' %
          ('double' if event.dblclick else 'single', event.button,
           event.x, event.y, event.xdata, event.ydata))


########################################################################################################################


class DraggableRectangle:
    def __init__(self, rect, num):
        self.rect = rect
        self.press = None
        self.number = int(num)

    def connect(self):
        'connect to all the events we need'
        self.cidpress = self.rect.figure.canvas.mpl_connect(
            'button_press_event', self.on_press)
        self.cidrelease = self.rect.figure.canvas.mpl_connect(
            'button_release_event', self.on_release)
        self.cidmotion = self.rect.figure.canvas.mpl_connect(
            'motion_notify_event', self.on_motion)

    def on_press(self, event):
        'on button press we will see if the mouse is over us and store some data'
        if event.inaxes != self.rect.axes: return

        contains, attrd = self.rect.contains(event)
        if not contains: return
        #print('event contains', self.rect.xy)
        x0, y0 = self.rect.xy
        self.press = x0, y0, event.xdata, event.ydata
        print("\n")
        if event.dblclick:
                height = self.rect.get_height()
                width = self.rect.get_width()
                self.rect.set_height(width)
                self.rect.set_width(height)



    def on_motion(self, event):
        'on motion we will move the rect if the mouse is over us'
        if self.press is None: return
        if event.inaxes != self.rect.axes: return
        x0, y0, xpress, ypress = self.press
        dx = event.xdata - xpress
        dy = event.ydata - ypress
        #print("x:%f y%f, d(%f, %f)"% (x0+dx, y0+dy, dx, dy) )

        #print('x0=%f, xpress=%f, event.xdata=%f, dx=%f, x0+dx=%f' %
        #      (x0, xpress, event.xdata, dx, x0+dx))
        self.rect.set_x(x0+dx)
        self.rect.set_y(y0+dy)

        self.rect.figure.canvas.draw()

    def on_release(self, event):
        'on release we reset the press data'
        self.press = None
        #print('event contains', self.rect.xy)
        self.rect.figure.canvas.draw()

        contains, attrd = self.rect.contains(event)
        #if not contains: return
        if event.inaxes != self.rect.axes: return

        self.printCstructInit()

    def disconnect(self):
        'disconnect all the stored connection ids'
        self.rect.figure.canvas.mpl_disconnect(self.cidpress)
        self.rect.figure.canvas.mpl_disconnect(self.cidrelease)
        self.rect.figure.canvas.mpl_disconnect(self.cidmotion)

    def printCstructInit(self):
        x,y = self.rect.xy

        print("\t\t{ .number = %3d, .coords = { {%4d,%4d},{%4d,%4d} } }, /* Autogen by python recv.py */"% (self.number, int(x),int(y), self.rect.get_width() + x ,self.rect.get_height() +y))

########################################################################################################################

if __name__ == '__main__':

	#exit()


	ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=20)
	StartApp(ser)
	get_image(ser)
	print (parking)
	get_image(ser)

	n_parkings = np.shape(parking)[0]

	park_number = [x[1] for x in parking]
	x_coord = [x[2] for x in parking]
	bottom_coord = [x[3] for x in parking]
	rect_width = [x[4] for x in parking]
	rect_heigth = [x[5] for x in parking]

	#cid = fig.canvas.mpl_connect('button_press_event', onclick)
	#cid = fig.canvas.mpl_connect('button_release_event', onclick)
	npar = np.random.random_sample((n_parkings,3))

	rects = ax.bar(x_coord, rect_heigth, bottom=bottom_coord, width = rect_width,align = 'edge', label = park_number ,fill = False, edgecolor = npar)

	drs = []
	for rect, n in zip(rects, park_number):
		dr = DraggableRectangle(rect, n)
		dr.connect()
		drs.append(dr)



	axnext = plt.axes([0.0, 0.0, 1, 0.1])
	button_update_img = Button(axnext, "Update img")
	button_update_img.on_clicked(update_cb)



	#WaitForImg()

	#plt.show()
	ser.close()
	plt.show()
	exit()
