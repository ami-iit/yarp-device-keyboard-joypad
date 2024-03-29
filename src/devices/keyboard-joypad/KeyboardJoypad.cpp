/*
 * Copyright (C) 2021 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-2-Clause license. See the accompanying LICENSE file for details.
 */

#include <yarp/os/LogStream.h>

#include <KeyboardJoypad.h>
#include <KeyboardJoypadLogComponent.h>

yarp::dev::KeyboardJoypad::KeyboardJoypad()
    : yarp::dev::DeviceDriver(),
    yarp::os::PeriodicThread(0.01, yarp::os::ShouldUseSystemClock::Yes)
{
}

yarp::dev::KeyboardJoypad::~KeyboardJoypad()
{
}

bool yarp::dev::KeyboardJoypad::open(yarp::os::Searchable& cfg)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::close()
{
    return false;
}

bool yarp::dev::KeyboardJoypad::threadInit()
{
    return false;
}

void yarp::dev::KeyboardJoypad::threadRelease()
{
}

void yarp::dev::KeyboardJoypad::run()
{
}

bool yarp::dev::KeyboardJoypad::startService()
{
    return false;
}

bool yarp::dev::KeyboardJoypad::updateService()
{
    return false;
}

bool yarp::dev::KeyboardJoypad::stopService()
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getAxisCount(unsigned int& axis_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getButtonCount(unsigned int& button_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getTrackballCount(unsigned int& trackball_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getHatCount(unsigned int& hat_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getTouchSurfaceCount(unsigned int& touch_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getStickCount(unsigned int& stick_count)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getStickDoF(unsigned int stick_id, unsigned int& dof)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getButton(unsigned int button_id, float& value)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getTrackball(unsigned int trackball_id, yarp::sig::Vector& value)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getHat(unsigned int hat_id, unsigned char& value)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getAxis(unsigned int axis_id, double& value)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getStick(unsigned int stick_id, yarp::sig::Vector& value, JoypadCtrl_coordinateMode coordinate_mode)
{
    return false;
}

bool yarp::dev::KeyboardJoypad::getTouch(unsigned int touch_id, yarp::sig::Vector& value)
{
    return false;
}
