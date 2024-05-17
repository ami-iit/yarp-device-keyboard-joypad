# yarp-device-keyboard-joypad
Repository containing the implementation of a yarp device to emulate a joypad using the keyboard, the touchscreen and/or an actual joypad.

## Example usage
By running the following command,
```
yarpdev --device keyboardJoypad --name /keyboard --buttons "(none, test, c:prova, c:prova, v, b, test, c:prova, 8, g-h:Multi)" --axes "(ws, ad, ad, none, -left_right, up_down)"
```
a GUI will be opened with a similar layout:

https://github.com/ami-iit/yarp-device-keyboard-joypad/assets/18591940/7717159e-8eef-4a5b-9273-814fa55f5560



## Dependencies
- [``YARP``](https://github.com/robotology/yarp) (>= 3.4.0) 
- [``YCM``](https://github.com/robotology/ycm)
- ``glfw3``
- ``GLEW``
- [``imgui``](https://github.com/ocornut/imgui) (Will be downloaded automatically if not found)

The dependencies can also be installed via [``mamba``](https://github.com/mamba-org/mamba) using the following command:
```bash
mamba install cmake compilers make ninja pkg-config glew glfw yarp imgui
```

## Configuration parameters
The device can be configured using the following parameters. They are all optional and have default values.
- ``button_size``: size of the buttons in pixels (default: 100)
- ``min_button_size``: minimum size of the buttons in pixels in the corresponding slider (default: 50)
- ``max_button_size``: maximum size of the buttons in pixels in the corresponding slider (default: 200)
- ``font_multiplier``: multiplier for the font size (default: 1)
- ``min_font_multiplier``: minimum value for the font multiplier in the corresponding slider (default: 0.5)
- ``max_font_multiplier``: maximum value for the font multiplier in the corresponding slider (default: 4.0)
- ``gui_period``: period in seconds for the GUI (default: 0.033)
- ``window_width``: width of the window in pixels (default: 1280)
- ``window_height``: height of the window in pixels (default: 720)
- ``buttons_per_row``: number of buttons per row in the "Buttons" widget (default: 4)
- ``padding``: padding in pixels for the buttons (default: 10)
- ``allow_window_closing``: when specified or set to true, the window can be closed by pressing the "X" button in the title bar. Note: when using this as device, the parent might keep running anyway (default: false)
- ``no_gui_thread``: when specified or set to true, the GUI will run in the same thread as the device. The GUI will be updated when calling ``updateService`` or when getting the values of axis/buttons (default: false, true on macOS)
- ``axes``: definition of the list of axes. The allowed values are "ws", "ad", "up_down" and "left_right". It is possible to select the default sign for an axis prepending a "+" or a "-" to the axis name. For example, "+ws" will set the "ws" axis with the default sign, while "-ws" will set the "ws" axis with the inverted sign. It is also possible to repeat some axis, and use "none" or "" to have dummy axes with always zero value. The order matters. (default: ("ad", "ws", "left_right", "up_down"))
- ``wasd_label``: label for the "WASD" widget (default: "WASD")
- ``arrows_label``: label for the "Arrows" widget (default: "Arrows")
- ``buttons``: definition of the list of buttons. The allowed values are all the letters from A to Z, all the numbers from 0 to 9, "SPACE", "ENTER", "ESCAPE", "BACKSPACE", "DELETE", "LEFT", "RIGHT", "UP", "DOWN". With "J" followed by a number it is possible to map a joypad button, when connected. It is possible to repeat some button. It is possible to specify an alias after a ":". For example "A:Some Text" will create a button with the label "Some Text" that can be activated by pressing "A". It is possible to use "none" or "" to indicate a dummy button always zero. It is possible to specify multiple keys using the "-" delimiter. For example, "A-B-J5:Some Text" creates a button named "Some Text" that can be activated pressing either A, or B, or the joypad button with index 5. It is possible to repeat buttons. The order matters. (default: ())
- ``joypad_indices``: definition of the joypads to consider in case multiple joypads are connected. The value can be a single integer or a list of integers. The indices are 0-based. In case a joypad is not found, it is ignored. The axis and buttons values are stack together in the order provided. (default: 0)
- ``joypad_deadzone``: deadzone for the joypad axes (default: 0.1)
- ``ad_joypad_axis_index``: index of the axis for the "ad" axis in the joypad (default: 0)
- ``ws_joypad_axis_index``: index of the axis for the "ws" axis in the joypad (default: 1)
- ``left_right_joypad_axis_index``: index of the axis for the "left_right" axis in the joypad (default: 2)
- ``up_down_joypad_axis_index``: index of the axis for the "up_down" axis in the joypad (default: 3)

## Maintainers
* Stefano Dafarra ([@S-Dafarra](https://github.com/S-Dafarra))