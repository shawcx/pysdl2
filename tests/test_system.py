'''Phase 6: power, CPU extras, touch, and the haptic / sensor device wrappers.

No haptic hardware or sensors exist under the headless drivers, so these mostly
check the query surface and the uninitialised-instance guards.'''

import pytest


def test_power_info(sdl):
    state, seconds, percent = sdl.GetPowerInfo()
    assert state in (
        sdl.POWERSTATE_UNKNOWN, sdl.POWERSTATE_ON_BATTERY, sdl.POWERSTATE_NO_BATTERY,
        sdl.POWERSTATE_CHARGING, sdl.POWERSTATE_CHARGED,
    )
    assert isinstance(seconds, int)
    assert isinstance(percent, int)


def test_cpu_extras(sdl):
    assert sdl.GetSystemRAM() > 0
    assert isinstance(sdl.HasNEON(), bool)
    assert isinstance(sdl.HasAVX512F(), bool)
    if hasattr(sdl, 'SIMDGetAlignment'):
        assert sdl.SIMDGetAlignment() >= 1


def test_touch_query(sdl):
    n = sdl.GetNumTouchDevices()
    assert n >= 0
    for i in range(n):
        tid = sdl.GetTouchDevice(i)
        assert sdl.GetNumTouchFingers(tid) >= 0
        assert sdl.GetTouchDeviceType(tid) in (
            sdl.TOUCH_DEVICE_INVALID, sdl.TOUCH_DEVICE_DIRECT,
            sdl.TOUCH_DEVICE_INDIRECT_ABSOLUTE, sdl.TOUCH_DEVICE_INDIRECT_RELATIVE,
        )


def test_touch_finger_out_of_range_is_none(sdl):
    assert sdl.GetTouchFinger(sdl.MOUSE_TOUCHID, 999) is None


# --- haptic -----------------------------------------------------------------

def test_haptic_module_functions(sdl):
    assert isinstance(sdl.NumHaptics(), int)
    assert isinstance(sdl.MouseIsHaptic(), bool)


def test_uninitialised_haptic_is_guarded(sdl):
    with pytest.raises(sdl.error):
        sdl.Haptic().Query()


def test_haptic_open_bad_index_raises(sdl):
    with pytest.raises(sdl.error):
        sdl.Haptic(99)


def test_joystick_is_haptic_type_check(sdl):
    with pytest.raises(TypeError):
        sdl.JoystickIsHaptic(object())


# --- sensor ---------------------------------------------------------------

def test_sensor_module_functions(sdl):
    assert sdl.NumSensors() >= 0
    for i in range(sdl.NumSensors()):
        assert isinstance(sdl.SensorGetDeviceName(i), str)
        assert sdl.SensorGetDeviceType(i) in (
            sdl.SENSOR_INVALID, sdl.SENSOR_UNKNOWN, sdl.SENSOR_ACCEL, sdl.SENSOR_GYRO,
        )
    sdl.SensorUpdate()


def test_uninitialised_sensor_is_guarded(sdl):
    with pytest.raises(sdl.error):
        sdl.Sensor().GetData()


def test_sensor_open_bad_index_raises(sdl):
    with pytest.raises(sdl.error):
        sdl.Sensor(99)
