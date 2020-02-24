# Smart meter reader

Simple demo of how Spresense together with the Spresense camera can be used to
do real time image analysis to digitize a mechanical manometer.

The demo fetches an image from the camera. A set of predefined pixel areas around the center with of the needle are extracted and parsed via a set of filters.
Two pixel area groups exist, one is U shaped and one is like up side down shaped U centered around the rotating point of the gauge needle.
 
The ASCII image below visualizes the shape. "O" stands for outer shape and "I" for inner. "C" is needle rotating point.
```
  OOOOOOOOOOOOOOOOOO
  O                O
  O   I    C    I  O
  O   I         I  O
      IIIIIIIIIII
```
After filtering the extracted scan areas, the result buffer is scanned for a peak value. If a peak value is found, the inner buffer of the opposite side of the needles rotating point is scanned.
If no needle is found on the opposite side, the scanning continues to the next pixel scan area. This continues until the needle is located.

Once the needle has been found, the needle angle can be calculated. Once the needle angle is known, a angle-to-value table used with several points to convert the angle to an actual meter value. This angle-to-value table could be dynamically loaded.

Using a neural network should make it possible to both find what type of gauge that is being used and also a few anchor points. This could be used find the needle rotating point and hopefully enough information to be able to align the angle-to-value table.

Tests using nnabla with an object detection network have worked very well to localize the needle rotating point. 
But in the current version this have not been implemented.




