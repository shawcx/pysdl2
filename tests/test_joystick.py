'''Phase 4: Joystick and GameController, driven by a virtual joystick.'''

import pytest


@pytest.fixture
def joystick_index(sdl):
    if not hasattr(sdl, 'JoystickAttachVirtual'):
        pytest.skip('virtual joysticks need SDL >= 2.0.14')
    sdl.InitSubSystem(sdl.INIT_JOYSTICK | sdl.INIT_GAMECONTROLLER)
    index = sdl.JoystickAttachVirtual(sdl.JOYSTICK_TYPE_GAMECONTROLLER, 4, 6, 1)
    yield index
    try:
        sdl.JoystickDetachVirtual(index)
    except sdl.error:
        pass


@pytest.fixture
def joystick(sdl, joystick_index):
    js = sdl.Joystick(joystick_index)
    yield js
    js.Close()


def test_module_functions(sdl, joystick_index):
    assert sdl.NumJoysticks() >= 1
    assert sdl.JoystickIsVirtual(joystick_index) is True
    assert sdl.JoystickNameForIndex(joystick_index)
    assert isinstance(sdl.JoystickEventState(), int)
    sdl.JoystickUpdate()


def test_joystick_metadata(joystick):
    assert joystick.Name()
    assert len(joystick.GetGUID()) == 32
    assert isinstance(joystick.InstanceID(), int)
    assert joystick.Attached() is True
    assert (joystick.NumAxes(), joystick.NumButtons(), joystick.NumHats()) == (4, 6, 1)


def test_virtual_input_reflects_in_getters(sdl, joystick):
    joystick.SetVirtualAxis(1, -20000)
    joystick.SetVirtualButton(3, 1)
    joystick.SetVirtualHat(0, sdl.HAT_LEFT)
    sdl.JoystickUpdate()

    assert joystick.GetAxis(1) == -20000
    assert joystick.GetButton(3) is True
    assert joystick.GetButton(0) is False
    assert joystick.GetHat(0) == sdl.HAT_LEFT


def test_rumble_and_led_do_not_raise(joystick):
    assert isinstance(joystick.Rumble(0xFFFF, 0x8000, 50), bool)
    if hasattr(joystick, 'SetLED'):
        assert isinstance(joystick.SetLED(10, 20, 30), bool)
        assert isinstance(joystick.HasLED(), bool)


def test_uninitialised_joystick_is_guarded(sdl):
    with pytest.raises(sdl.error):
        sdl.Joystick().Name()


def test_close_is_idempotent(sdl, joystick_index):
    js = sdl.Joystick(joystick_index)
    js.Close()
    js.Close()
    with pytest.raises(sdl.error):
        js.NumAxes()


# --- game controller --------------------------------------------------------

@pytest.fixture
def controller(sdl, joystick_index):
    if not sdl.IsGameController(joystick_index):
        pytest.skip('virtual joystick has no game-controller mapping here')
    gc = sdl.GameController(joystick_index)
    yield gc
    gc.Close()


def test_controller_basics(sdl, controller):
    assert controller.Name()
    assert controller.Attached() is True
    assert isinstance(controller.GetAxis(sdl.CONTROLLER_AXIS_LEFTX), int)
    assert isinstance(controller.GetButton(sdl.CONTROLLER_BUTTON_A), bool)
    assert isinstance(controller.Mapping(), str)


def test_controller_get_joystick_is_borrowed(sdl, controller, joystick_index):
    js = controller.GetJoystick()
    assert type(js).__name__ == 'Joystick'
    del js  # borrowed: must not close the controller's joystick
    assert controller.GetButton(sdl.CONTROLLER_BUTTON_B) is False


def test_controller_add_mapping(sdl):
    guid = '03000000f.0000000000000000000000'.replace('.', '0')
    rc = sdl.GameControllerAddMapping(f'{guid},pysdl2 test pad,a:b0,b:b1,')
    assert rc in (0, 1)


# --- phase 10: joystick / controller completeness -------------------------

BUTTONS = ('CONTROLLER_BUTTON_A', 'CONTROLLER_BUTTON_B')
AXES = ('CONTROLLER_AXIS_LEFTX', 'CONTROLLER_AXIS_LEFTY')


@pytest.fixture
def need_ex(sdl):
    if not hasattr(sdl, 'JoystickAttachVirtualEx'):
        pytest.skip('JoystickAttachVirtualEx needs SDL >= 2.24')
    sdl.InitSubSystem(sdl.INIT_JOYSTICK | sdl.INIT_GAMECONTROLLER)


@pytest.fixture
def calls():
    return []


@pytest.fixture
def vpad(sdl, need_ex, calls):
    '''A virtual game controller with A/B, LeftX/LeftY and recording callbacks.'''
    index = sdl.JoystickAttachVirtualEx(
        sdl.JOYSTICK_TYPE_GAMECONTROLLER, 2, 2, 0,
        vendor_id=0x1234, product_id=0x5678, name='pysdl2 vpad',
        button_mask=sum(1 << getattr(sdl, b) for b in BUTTONS),
        axis_mask=sum(1 << getattr(sdl, a) for a in AXES),
        rumble=lambda low, high: calls.append(('rumble', low, high)),
        set_led=lambda r, g, b: calls.append(('led', r, g, b)),
        send_effect=lambda data: calls.append(('effect', data)),
        set_player_index=lambda player: calls.append(('player', player)),
    )
    yield index
    try:
        sdl.JoystickDetachVirtual(index)
    except sdl.error:
        pass


def test_device_index_queries(sdl, vpad):
    js = sdl.Joystick(vpad)
    try:
        assert sdl.JoystickGetDeviceGUID(vpad) == js.GetGUID()
        assert sdl.JoystickGetDeviceInstanceID(vpad) == js.InstanceID()
        assert sdl.JoystickGetDeviceVendor(vpad) == 0x1234
        assert sdl.JoystickGetDeviceProduct(vpad) == 0x5678
        assert isinstance(sdl.JoystickGetDeviceProductVersion(vpad), int)
        assert sdl.JoystickGetDeviceType(vpad) == sdl.JOYSTICK_TYPE_GAMECONTROLLER
        assert isinstance(sdl.JoystickGetDevicePlayerIndex(vpad), int)
        assert sdl.JoystickPathForIndex(vpad) is None  # virtual devices have no path
    finally:
        js.Close()


def test_joystick_info_methods(sdl, vpad):
    js = sdl.Joystick(vpad)
    try:
        assert (js.GetVendor(), js.GetProduct()) == (0x1234, 0x5678)
        assert isinstance(js.GetProductVersion(), int)
        assert js.GetType() == sdl.JOYSTICK_TYPE_GAMECONTROLLER
        assert js.GetSerial() is None
        assert js.Path() is None
        assert isinstance(js.GetFirmwareVersion(), int)
        assert js.GetAxisInitialState(0) in (0, None)
    finally:
        js.Close()


def test_player_index_and_lookup(sdl, vpad, calls):
    js = sdl.Joystick(vpad)
    try:
        js.SetPlayerIndex(3)
        assert js.GetPlayerIndex() == 3
        assert ('player', 3) in calls

        found = sdl.JoystickFromPlayerIndex(3)
        assert found.InstanceID() == js.InstanceID()
        found.Close()  # owned reference: must not close `js`
        assert js.Attached() is True

        again = sdl.JoystickFromInstanceID(js.InstanceID())
        assert again.Name() == 'pysdl2 vpad'
        again.Close()
        assert sdl.JoystickFromInstanceID(0x7FFFFFF0) is None
    finally:
        js.Close()


def test_virtual_callbacks(sdl, vpad, calls):
    js = sdl.Joystick(vpad)
    try:
        assert js.HasRumble() is True
        assert js.HasRumbleTriggers() is False  # no rumble_triggers callback
        assert js.Rumble(100, 200, 10) is True
        assert js.SetLED(1, 2, 3) is True
        assert js.SendEffect(b'\x01\x02') is True
        assert js.RumbleTriggers(1, 1, 10) is False
    finally:
        js.Close()
    # Closing a rumbling joystick sends a final rumble(0, 0) to stop it.
    assert calls[-4:] == [('rumble', 100, 200), ('led', 1, 2, 3), ('effect', b'\x01\x02'),
                          ('rumble', 0, 0)]


def test_virtual_callback_failure_and_exception(sdl, need_ex, capfd):
    def boom(low, high):
        raise RuntimeError('rumble exploded')

    index = sdl.JoystickAttachVirtualEx(rumble=boom, set_led=lambda r, g, b: False)
    js = sdl.Joystick(index)
    try:
        assert js.Rumble(1, 1, 10) is False
        assert 'rumble exploded' in capfd.readouterr().err
        assert js.SetLED(0, 0, 0) is False
    finally:
        js.Close()
        sdl.JoystickDetachVirtual(index)


def test_virtual_update_callback(sdl, need_ex):
    ticks = []
    index = sdl.JoystickAttachVirtualEx(naxes=1, update=lambda: ticks.append(1))
    js = sdl.Joystick(index)
    try:
        sdl.JoystickUpdate()
        assert ticks
    finally:
        js.Close()
        sdl.JoystickDetachVirtual(index)


def test_detach_releases_callbacks(sdl, need_ex):
    import gc
    import weakref

    class Callback:
        def __call__(self, low, high):
            pass

    cb = Callback()
    ref = weakref.ref(cb)
    index = sdl.JoystickAttachVirtualEx(rumble=cb)
    del cb
    gc.collect()
    assert ref() is not None  # SDL may still call it
    sdl.JoystickDetachVirtual(index)
    gc.collect()
    assert ref() is None


def test_attach_virtual_ex_rejects_non_callable(sdl, need_ex):
    with pytest.raises(TypeError):
        sdl.JoystickAttachVirtualEx(rumble=42)


def test_guid_conversions(sdl, vpad):
    guid = sdl.JoystickGetDeviceGUID(vpad)
    raw = sdl.JoystickGetGUIDFromString(guid)
    assert isinstance(raw, bytes) and len(raw) == 16
    assert sdl.JoystickGetGUIDString(raw) == guid
    assert sdl.GUIDFromString(guid) == raw
    assert sdl.GUIDToString(raw) == guid
    with pytest.raises(ValueError):
        sdl.JoystickGetGUIDString(b'short')
    with pytest.raises(ValueError):
        sdl.JoystickGetGUIDFromString('abc')
    if hasattr(sdl, 'GetJoystickGUIDInfo'):
        vendor, product, version, crc16 = sdl.GetJoystickGUIDInfo(guid)
        assert (vendor, product) == (0x1234, 0x5678)


def test_lock_joysticks(sdl, need_ex):
    sdl.LockJoysticks()
    sdl.UnlockJoysticks()


@pytest.fixture
def vcontroller(sdl, vpad):
    gc = sdl.GameController(vpad)
    yield gc
    gc.Close()


def test_controller_capabilities(sdl, vcontroller):
    for name in BUTTONS:
        assert vcontroller.HasButton(getattr(sdl, name)) is True
    for name in AXES:
        assert vcontroller.HasAxis(getattr(sdl, name)) is True
    assert vcontroller.HasButton(sdl.CONTROLLER_BUTTON_X) is False
    assert vcontroller.HasAxis(sdl.CONTROLLER_AXIS_RIGHTX) is False
    assert vcontroller.HasLED() is True
    assert vcontroller.HasRumble() is True
    assert vcontroller.HasRumbleTriggers() is False


def test_controller_binds(sdl, vcontroller):
    kind, button = vcontroller.GetBindForButton(sdl.CONTROLLER_BUTTON_A)
    assert kind == sdl.CONTROLLER_BINDTYPE_BUTTON and button == 0
    kind, axis = vcontroller.GetBindForAxis(sdl.CONTROLLER_AXIS_LEFTY)
    assert kind == sdl.CONTROLLER_BINDTYPE_AXIS and axis == 1
    assert vcontroller.GetBindForButton(sdl.CONTROLLER_BUTTON_X) is None


def test_controller_info(sdl, vcontroller, calls):
    assert (vcontroller.GetVendor(), vcontroller.GetProduct()) == (0x1234, 0x5678)
    assert isinstance(vcontroller.GetProductVersion(), int)
    assert vcontroller.GetType() == sdl.CONTROLLER_TYPE_VIRTUAL
    assert vcontroller.GetSerial() is None
    assert vcontroller.Path() is None
    assert isinstance(vcontroller.GetFirmwareVersion(), int)
    if hasattr(vcontroller, 'GetSteamHandle'):
        assert vcontroller.GetSteamHandle() == 0
    symbol = vcontroller.GetAppleSFSymbolsNameForButton(sdl.CONTROLLER_BUTTON_A)
    assert symbol is None or isinstance(symbol, str)  # str only on Apple platforms

    vcontroller.SetPlayerIndex(2)
    assert vcontroller.GetPlayerIndex() == 2
    assert vcontroller.Rumble(7, 8, 10) is True
    assert vcontroller.SendEffect(b'z') is True
    assert ('rumble', 7, 8) in calls and ('effect', b'z') in calls


def test_controller_sensors_and_touchpads(sdl, vcontroller):
    assert vcontroller.HasSensor(sdl.SENSOR_ACCEL) is False
    assert vcontroller.IsSensorEnabled(sdl.SENSOR_ACCEL) is False
    assert vcontroller.GetSensorDataRate(sdl.SENSOR_ACCEL) == 0.0
    with pytest.raises(sdl.error):
        vcontroller.SetSensorEnabled(sdl.SENSOR_ACCEL, True)
    with pytest.raises(sdl.error):
        vcontroller.GetSensorData(sdl.SENSOR_ACCEL)
    with pytest.raises(ValueError):
        vcontroller.GetSensorData(sdl.SENSOR_ACCEL, count=0)
    if hasattr(vcontroller, 'GetSensorDataWithTimestamp'):
        with pytest.raises(sdl.error):
            vcontroller.GetSensorDataWithTimestamp(sdl.SENSOR_ACCEL)

    assert vcontroller.GetNumTouchpads() == 0
    with pytest.raises(sdl.error):
        vcontroller.GetNumTouchpadFingers(0)
    with pytest.raises(sdl.error):
        vcontroller.GetTouchpadFinger(0, 0)


def test_controller_lookup(sdl, vcontroller):
    iid = vcontroller.GetJoystick().InstanceID()
    found = sdl.GameControllerFromInstanceID(iid)
    assert found.Name() == vcontroller.Name()
    found.Close()  # owned reference: the original stays open
    assert vcontroller.Attached() is True

    vcontroller.SetPlayerIndex(1)
    by_player = sdl.GameControllerFromPlayerIndex(1)
    assert by_player.Name() == vcontroller.Name()
    by_player.Close()
    assert sdl.GameControllerFromInstanceID(0x7FFFFFF0) is None


def test_controller_mapping_queries(sdl, vpad, vcontroller):
    assert sdl.GameControllerNumMappings() > 0
    assert isinstance(sdl.GameControllerMappingForIndex(0), str)
    assert sdl.GameControllerMappingForIndex(10 ** 6) is None

    guid = vcontroller.GetJoystick().GetGUID()
    assert sdl.GameControllerMappingForGUID(guid).startswith(guid)
    assert sdl.GameControllerMappingForGUID(sdl.JoystickGetGUIDFromString(guid)).startswith(guid)
    assert 'a:b0' in sdl.GameControllerMappingForDeviceIndex(vpad)
    assert sdl.GameControllerTypeForIndex(vpad) == sdl.CONTROLLER_TYPE_VIRTUAL
    assert sdl.GameControllerPathForIndex(vpad) is None


def test_controller_string_names(sdl):
    a = sdl.CONTROLLER_BUTTON_A
    leftx = sdl.CONTROLLER_AXIS_LEFTX
    assert sdl.GameControllerGetStringForButton(a) == 'a'
    assert sdl.GameControllerGetButtonFromString('a') == a
    assert sdl.GameControllerGetStringForAxis(leftx) == 'leftx'
    assert sdl.GameControllerGetAxisFromString('leftx') == leftx
    assert sdl.GameControllerGetAxisFromString('nope') == sdl.CONTROLLER_AXIS_INVALID
    assert sdl.GameControllerGetStringForButton(999) is None


def test_add_mappings_from_bytes(sdl):
    sdl.InitSubSystem(sdl.INIT_GAMECONTROLLER)
    data = b'03000000ab0000000000000000000000,pysdl2 bytes pad,a:b0,b:b1,platform:Linux,\n'
    assert sdl.GameControllerAddMappingsFromFile(data) in (0, 1)
    assert sdl.GameControllerMappingForGUID('03000000ab0000000000000000000000') is not None


def test_new_event_constants(sdl):
    for name in ('CONTROLLERTOUCHPADDOWN', 'CONTROLLERTOUCHPADMOTION', 'CONTROLLERTOUCHPADUP',
                 'CONTROLLERSENSORUPDATE', 'CONTROLLER_BINDTYPE_HAT', 'JOYSTICK_AXIS_MAX'):
        assert isinstance(getattr(sdl, name), int)
