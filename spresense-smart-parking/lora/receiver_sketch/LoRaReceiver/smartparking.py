#! /usr/bin/python3

from tkinter import *
from serial import Serial
import datetime as t
import numpy as np
from PIL import ImageTk, Image
import os

ser = Serial('/dev/ttyUSB0', 115200, timeout=0.2)
factor = 3
__location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))

rot = Tk()
rect_width = 50 * factor
rect_distance = 20 * factor
rect_height = 80 * factor

watchdog_limit=200

parking_row1_n = 9
parking_row2_n = 9
parking_row3_n = 5

xoffset = 1925

car_position1 = [xoffset + c*185 for c in  range(9)]
car_position2 = [xoffset + c*185 for c in  range(9)]
car_position3 = [xoffset + 30 + c*340 for c in  range(5)]
car_positions = [[a, 0, 0] for a in (car_position1 + car_position2 + car_position3)]
print(car_positions)

for x in car_positions[0:9]:
    x[1] = 600
for x in car_positions[9:18]:
    x[1] = 1130
    x[2] = 180
for x in car_positions[18:23]:
    x[1] = 1630
    x[2] = 90

n_free = 0
    
image_scale_factor_x = 1.0
image_scale_factor_y = 1.0

class DigitalDisplay:
    def __init__(self, c):
        self.c = c
        fname = ["digits/" + str(x) + ".png" for x in range(10)]
        
        #self.digit_raw = [Image.open(os.path.join(__location__, x)) for x in fname]
        
        self.digit_dict = {}
        #self.digit_dict.update([(str(x), Image.open("digits/" + str(x) + ".png")) for x in range(10)])
        self.digit_dict.update([(x, Image.open(os.path.join(__location__ , "digits/" + str(x) + ".png"))) for x in range(10)])
        self.digit_dict.update([(x+9, Image.open(os.path.join(__location__, "digits/" + str(x) + ".png"))) for x in range(1,3)])

        self.photo = []
        self.digit_tk = []
        x = 800
        y = 1400
        
        x = x * image_scale_factor_x
        y = y * image_scale_factor_y
        
        self.t =  []
        for k,v in self.digit_dict.items():
            v = v.resize([int(a * image_scale_factor_x) for a in v.size], Image.ANTIALIAS)
            print(v.size)
            self.t.append(ImageTk.PhotoImage(v))
            xpos = x
            if (k >= 10):
                xpos = x - v.size[0]
                
            print("Digit: " + str(k) + " val: " + str(xpos) + "y: " +  str(y))
            self.digit_dict[k] = self.c.create_image((xpos, y), anchor="center", image=self.t[-1], state="normal")
        
        self.hideAll()
        
    def setValue(self, val):
        print(val)
        if val < 0:
            return
        ones = val % 10
        tens = int(val / 10)
        
        print("Val: " + str(val))
        print("Ones: " + str(ones))
        print("Tens: " + str(tens))
      
        v1 = self.digit_dict[ones]
        v10 = self.digit_dict[tens]
        
        self.hideAll()
        self.c.itemconfig(v1, state="normal")
        if tens > 0:
            self.c.itemconfig(v10+9, state="normal")
        
    def hideAll(self):
        for k,v in self.digit_dict.items():
            self.c.itemconfig(v, state="hidden")

class Space:

    parking_state = {"free": "hidden", "entering": "normal", "leaving": "normal", "occupied": "normal"}
    parking_state_to_text = {2: "free", 3: "entering", 4: "occupied", 5: "leaving"}

    def __init__(self, number, c, x1, y1, angle):
        global n_free
        
        self.number = number
        self.x1 = x1
        self.y1 = y1
        self.c = c
        
        
         
        car = Image.open(os.path.join(__location__, 'car_red.png'));
        self.state = "init"
       
        
        car = car.resize([int(a * image_scale_factor_x) for a in car.size], Image.ANTIALIAS)

        self.bg = ImageTk.PhotoImage(car.rotate(angle))
        self.car = self.c.create_image((x1, y1), anchor="nw", image=self.bg)
        self.id = self.car
        c.create_text(x1 + (car.size[0] / 2), y1 + car.size[1] + 10, text=str(self.number))
        self.set_state_str("free")        
        #self.set_state_str("occupied")        

    def motion(event):
        x, y = event.x, event.y
        print('{}, {}'.format(x, y))

    def set_state(self, state):
        if (state in self.parking_state_to_text.keys()):
            self.set_state_str(self.parking_state_to_text[state])

    def set_state_str(self, state_str):
        global n_free

        if (state_str in self.parking_state.keys()):
            # self.c.itemconfig(self.id, fill=self.parking_state_color[state_str])
            self.c.itemconfig(self.id, state=self.parking_state[state_str])
            if self.state != state_str:
                if state_str == "free":
                    n_free = n_free + 1
                else:
                    if (n_free > 0):
                        n_free = n_free - 1
                self.state = state_str
            
    def is_free(self):
        
        if self.state == "free":
            return 1
        else:
            return 0

class ParkingLot:
    """Class of a full Parking lot with parking spaces"""
    def __init__(self, rot, spaces):
        self.rot = rot
        global image_scale_factor_x
        global image_scale_factor_y
        global n_free

        self.img = Image.open(os.path.join(__location__, "parking.jpg"));
        screen_width = rot.winfo_screenwidth()
        screen_height = rot.winfo_screenheight()
        
        #screen_width = 800
        #screen_height = 600
        
        if self.img.width != screen_width or self.img.height != screen_height:
            image_scale_factor_x = screen_width / self.img.width 
            image_scale_factor_y = screen_height / self.img.height  
            self.img = self.img.resize((screen_width, screen_height), Image.ANTIALIAS)
        
        print("width: " + str(screen_width) + " height: " + str(screen_height) + " scale factor: " + str(image_scale_factor_x) + " and " + str(image_scale_factor_y))
        
            
        self.c = Canvas(self.rot, width=screen_width, height=screen_height)
        self.bg = ImageTk.PhotoImage(self.img)

        a = self.c.create_image((0, 0), anchor="nw", image=self.bg)
        self.c.configure(background="white")
        self.c.pack()
        self.parking_lot = {}
        space = 1
        self.disp = DigitalDisplay(self.c)

        for x in spaces:
            #print(x)
            x[0] = x[0] * image_scale_factor_x
            x[1] = x[1] * image_scale_factor_y
            self.parking_lot[space] = Space(space, self.c, x[0], x[1], x[2])
            space = space + 1

        r=10
        pt=15
        self.circle = self.c.create_oval(pt - r, pt - r, pt + r, pt + r)
        self.c.itemconfig(self.circle, fill="yellow")
        self.fill = 0

        self.disp.setValue(n_free)
        

    def update_parking_state(self, space, state):
        if self.fill == 0:
            self.fill = 1
            c = ""
        else:
            self.fill = 0
            c = "yellow"
        self.c.itemconfig(self.circle, fill=c)
        if (space in self.parking_lot.keys()):
            self.parking_lot[space].set_state(state)
            self.disp.setValue(n_free)

    def wd_timedout(self, ):
        self.c.itemconfig(self.circle, fill="red")

def down(e):
    global ser

    print("Key down event:" + e.char)
    if e.char == 'q':
        ser.close()
        exit(0)
    elif e.char == 'f':
        rot.attributes("-fullscreen", False)
    elif e.char == 'h':
        rot.attributes("-fullscreen", True)

rot.attributes("-fullscreen", True)
rot.bind('<KeyPress>', down)

lot = ParkingLot(rot, car_positions)

watchdog=0

def SerialReceive():
    global watchdog
    try:
        raw = ser.readline()
        line = raw.decode("utf-8").strip()
        spaces = str(line).split(':')
    except Exception:
        print("Error decoding:")
        print(raw)
        rot.after(200, SerialReceive)
        return 
    

    if len(spaces) > 1:
        d = t.datetime.now()
        print(d)
        print( spaces)
        watchdog = 0
        for space in spaces[0:-1:]:
            id, state = space.split(',')
            lot.update_parking_state(int(id), int(state))
    else:

        watchdog = watchdog + 1
        if watchdog > watchdog_limit:
            print("watchdog timed out exiting...")
            lot.wd_timedout()
    rot.after(200, SerialReceive)

SerialReceive()

print("Smart Parking")

rot.mainloop()
ser.close()
