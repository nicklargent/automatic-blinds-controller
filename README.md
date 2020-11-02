# Automatic Blinds Controller

Ardunio code to control custom rig for my mini blinds.

You can find the printed parts over on [thingiverse](https://www.thingiverse.com/thing:4370318)

## Intro

My goal was to automatically open and close my office blinds in sync with daylight outside.  I was inspired by a design on thingiverse and decided to make my own controller.
I wanted to be as compact as possible which meant eliminating the control rod for the blinds.  I built a rig that attached directly to the blinds base with a minimal profile.

The design is powered by an ardunio nano powered by wall USB and triggered by a light sensor taped to the window behind the blinds.

I intended to minimize the design further with a custom board and 3D enclosure, but my reality is that this has operated flawlessly for 6 months and has been hidded behind curtains.

## Usage

1. Upon powering on, the device is in calibration mode.  The buttons are used to rotate the blinds in one direction or the other.  
2. Use the buttons to position the blinds to your preferred 'open' position.  Press both buttons together to set the position.  
3. Now use the buttons to rotate the blinds into your prefered 'closed' position and then press both buttons again.  
4. The system is now calibrated and will open / close the blinds based on the light level detected (make sure the light sensor is positioned outside of the blinds.)  
5. While in this mode, holding either button for 4 seconds will toggle the current state (open/closed).


## Images

![Project Photo](/images/rev2_electronics.jpg)

![Closeup](/images/rev2_electronics_close.jpg)

![Fritz](/images/rev2_breadboard.png)
