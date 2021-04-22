#include "objects/gamepad/gamepad.h"
#include "driver/hidrv.h"

using namespace love;

#include "modules/event/event.h"

#define EVENT_MODULE()         (Module::GetInstance<love::Event>(Module::M_EVENT))
#define INVALID_GAMEPAD_BUTTON static_cast<GamepadButton>(-1)

Gamepad::Gamepad(size_t id) : common::Gamepad(id), buttonStates()
{}

Gamepad::Gamepad(size_t id, size_t index) : common::Gamepad(id)
{
    this->Open(index);
}

Gamepad::~Gamepad()
{
    this->Close();
}

bool Gamepad::Open(size_t index)
{
    this->Close();

    this->name = "Nintendo 3DS";

    return this->IsConnected();
}

void Gamepad::Close()
{
    this->instanceID = -1;
    this->vibration  = Vibration();
}

bool Gamepad::IsConnected() const
{
    return true;
}

const char* Gamepad::GetName() const
{
    return this->name.c_str();
}

size_t Gamepad::GetAxisCount() const
{
    bool isN3DS = false;
    APT_CheckNew3DS(&isN3DS);

    /* total axes */
    size_t axes = 12;

    if (!isN3DS)
        return axes - 2;

    return axes;
}

size_t Gamepad::GetButtonCount() const
{
    /* constant value anyways */
    return Gamepad::MAX_BUTTONS - 1;
}

float Gamepad::GetAxis(size_t axis) const
{
    if (axis < 0 || axis >= this->GetAxisCount())
        return 0.0f;

    float value = 0.0f;

    if (axis == 1 || axis == 2)
    {
        circlePosition leftStick;
        hidCircleRead(&leftStick);

        if (axis == 1)
            value = leftStick.dx;
        else
            value = -leftStick.dy;

        return Gamepad::ClampValue(value / JOYSTICK_MAX);
    }
    else if (axis == 3 || axis == 4)
    {
        circlePosition rightStick;
        irrstCstickRead(&rightStick);

        if (axis == 3)
            value = rightStick.dx;
        else
            value = -rightStick.dy;

        return Gamepad::ClampValue(value / JOYSTICK_MAX);
    }
    else if (axis == 5)
    {
        if (hidKeysHeld() & KEY_ZL)
            return 1.0f;

        return 0.0f;
    }
    else if (axis == 6)
    {
        if (hidKeysHeld() & KEY_ZR)
            return 1.0f;

        return 0.0f;
    }
    else
    {
        if (axis >= 7 && axis < 10)
        {
            angularRate gyroscope;
            hidGyroRead(&gyroscope);

            if (axis == 7)
                return gyroscope.x;
            else if (axis == 8)
                return gyroscope.y;

            return gyroscope.z;
        }
        else if (axis >= 10 && axis < 13)
        {
            accelVector accelerometer;
            hidAccelRead(&accelerometer);

            if (axis == 10)
                return accelerometer.x;
            else if (axis == 11)
                return accelerometer.y;

            return accelerometer.z;
        }
    }

    return 0.0f;
}

std::vector<float> Gamepad::GetAxes() const
{
    std::vector<float> axes;
    size_t count = this->GetAxisCount();

    if (count <= 0)
        return axes;

    axes.reserve(count);

    for (size_t index = 0; index < count; index++)
        axes.push_back(this->GetAxis(index));

    return axes;
}

/* helper functions */
bool Gamepad::IsDown(std::pair<const char*, size_t>& button)
{
    uint32_t pressedSet = hidKeysDown();
    uint32_t hidButton;

    if (this->buttonStates.pressed != pressedSet)
    {
        auto recordPair = buttons.GetEntries();
        auto records    = recordPair.first;

        for (size_t i = 0; i < recordPair.second; i++)
        {
            if ((hidButton = static_cast<uint32_t>(records[i].value)) & pressedSet)
            {
                button = { records[i].key, i };
                break;
            }
        }
    }

    this->buttonStates.pressed = pressedSet;
    return (pressedSet & hidButton);
}

bool Gamepad::IsUp(std::pair<const char*, size_t>& button)
{
    uint32_t releasedSet = hidKeysUp();
    uint32_t hidButton;

    if (this->buttonStates.released != releasedSet)
    {
        auto recordPair = buttons.GetEntries();
        auto records    = recordPair.first;

        for (size_t i = 0; i < recordPair.second; i++)
        {
            if ((hidButton = static_cast<uint32_t>(records[i].value)) & releasedSet)
            {
                button = { records[i].key, i };
                break;
            }
        }
    }

    this->buttonStates.released = releasedSet;
    return (releasedSet & hidButton);
}

bool Gamepad::IsHeld(std::pair<const char*, size_t>& button) const
{
    uint32_t heldSet = hidKeysHeld();
    uint32_t hidButton;

    auto recordPair = buttons.GetEntries();
    auto records    = recordPair.first;

    for (size_t i = 0; i < recordPair.second; i++)
    {
        if ((hidButton = static_cast<uint32_t>(records[i].value)) & heldSet)
        {
            button = { records[i].key, i };
            break;
        }
    }

    return (heldSet & hidButton);
}

bool Gamepad::IsDown(const std::vector<size_t>& buttonsVector) const
{
    auto recordPair = buttons.GetEntries();

    uint32_t heldSet = hidKeysHeld();
    auto records     = recordPair.first;

    for (size_t button : buttonsVector)
    {
        if (button < 0 || button >= recordPair.second)
            continue;

        if (heldSet & static_cast<uint32_t>(records[button].value))
            return true;
    }

    return false;
}

bool Gamepad::IsGamepadDown(const std::vector<GamepadButton>& buttonsVector) const
{
    auto recordPair = buttons.GetEntries();

    uint32_t heldSet = hidKeysHeld();
    auto records     = recordPair.first;

    GamepadButton consoleButton = INVALID_GAMEPAD_BUTTON;
    const char* name            = nullptr;

    for (GamepadButton button : buttonsVector)
    {
        /* make sure our out button isn't invalid */
        if (!GetConstant(button, name))
            continue;

        /* convert to the proper button */
        if (!GetConstant(name, consoleButton))
            continue;

        if (heldSet & static_cast<uint32_t>(consoleButton))
            return true;
    }

    return false;
}

float Gamepad::GetGamepadAxis(Gamepad::GamepadAxis axis) const
{
    const char* name = nullptr;
    if (!Gamepad::GetConstant(axis, name))
        return 0.0f;

    switch (axis)
    {
        case GamepadAxis::GAMEPAD_AXIS_LEFTX:
            return this->GetAxis(1);
        case GamepadAxis::GAMEPAD_AXIS_LEFTY:
            return this->GetAxis(2);
        case GamepadAxis::GAMEPAD_AXIS_RIGHTX:
            return this->GetAxis(3);
        case GamepadAxis::GAMEPAD_AXIS_RIGHTY:
            return this->GetAxis(4);
        case GamepadAxis::GAMEPAD_AXIS_TRIGGERLEFT:
            return this->GetAxis(5);
        case GamepadAxis::GAMEPAD_AXIS_TRIGGERRIGHT:
            return this->GetAxis(6);
        default:
            break;
    }

    return 0.0f;
}

bool Gamepad::IsVibrationSupported()
{
    return false;
}

bool Gamepad::SetVibration(float left, float right, float duration)
{
    return false;
}

bool Gamepad::SetVibration()
{
    return false;
}

void Gamepad::GetVibration(float& left, float& right)
{
    left  = 0.0f;
    right = 0.0f;
}

bool Gamepad::GetConstant(const char* in, Gamepad::GamepadAxis& out)
{
    return axes.Find(in, out);
}

bool Gamepad::GetConstant(Gamepad::GamepadAxis in, const char*& out)
{
    return axes.Find(in, out);
}

bool Gamepad::GetConstant(const char* in, GamepadButton& out)
{
    return buttons.Find(in, out);
}

bool Gamepad::GetConstant(GamepadButton in, const char*& out)
{
    return buttons.Find(in, out);
}

// clang-format off
constexpr StringMap<Gamepad::GamepadAxis, Gamepad::MAX_AXES>::Entry axisEntries[] =
{
    { "leftx",        Gamepad::GamepadAxis::GAMEPAD_AXIS_LEFTX        },
    { "lefty",        Gamepad::GamepadAxis::GAMEPAD_AXIS_LEFTY        },
    { "rightx",       Gamepad::GamepadAxis::GAMEPAD_AXIS_RIGHTX       },
    { "righty",       Gamepad::GamepadAxis::GAMEPAD_AXIS_RIGHTY       },
    { "triggerleft",  Gamepad::GamepadAxis::GAMEPAD_AXIS_TRIGGERLEFT  },
    { "triggerright", Gamepad::GamepadAxis::GAMEPAD_AXIS_TRIGGERRIGHT }
};

constinit const StringMap<Gamepad::GamepadAxis, Gamepad::MAX_AXES> Gamepad::axes(axisEntries);

constexpr StringMap<Gamepad::GamepadButton, Gamepad::MAX_BUTTONS>::Entry buttonEntries[] =
{
    { "a",             Gamepad::GamepadButton::GAMEPAD_BUTTON_A              },
    { "b",             Gamepad::GamepadButton::GAMEPAD_BUTTON_B              },
    { "x",             Gamepad::GamepadButton::GAMEPAD_BUTTON_X              },
    { "y",             Gamepad::GamepadButton::GAMEPAD_BUTTON_Y              },
    { "back",          Gamepad::GamepadButton::GAMEPAD_BUTTON_BACK           },
    { "start",         Gamepad::GamepadButton::GAMEPAD_BUTTON_START          },
    { "leftshoulder",  Gamepad::GamepadButton::GAMEPAD_BUTTON_LEFT_SHOULDER  },
    { "rightshoulder", Gamepad::GamepadButton::GAMEPAD_BUTTON_RIGHT_SHOULDER },
    { "dpup",          Gamepad::GamepadButton::GAMEPAD_BUTTON_DPAD_UP        },
    { "dpdown",        Gamepad::GamepadButton::GAMEPAD_BUTTON_DPAD_DOWN      },
    { "dpleft",        Gamepad::GamepadButton::GAMEPAD_BUTTON_DPAD_LEFT      },
    { "dpright",       Gamepad::GamepadButton::GAMEPAD_BUTTON_DPAD_RIGHT     }
};

constinit const StringMap<Gamepad::GamepadButton, Gamepad::MAX_BUTTONS> Gamepad::buttons(buttonEntries);
// clang-format on
