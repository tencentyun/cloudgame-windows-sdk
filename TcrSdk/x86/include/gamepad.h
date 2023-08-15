#ifndef TCRSDK_GAMEPAD_H_
#define TCRSDK_GAMEPAD_H_

#include<stdint.h>

namespace tcrsdk {
   
    /**
     * @file gamepad.h
     * 
     * @brief This interface is used to interact with the virtual Gamepad device on the server.
     */
    class __declspec(dllexport) Gamepad
    {
    public:
        /**
         * @brief The type of a general controller button
         */
        enum class KeyType {
            /**
             * @brief The left stick
             */
            LS,
            /**
             * @brief The right stick
             */
            RS,
            /**
             * @brief The left trigger
             */
            LT,
            /**
             * @brief The right trigger
             */
            RT
        };

        /**
         * @brief Trigger the cloud machine to insert the virtual Gamepad.
         */
        virtual void ConnectGamepad() const = 0;

        /**
         * @brief Trigger the cloud machine to pull out the virtual Gamepad.
         */
        virtual void DisconnectGamepad() const = 0;

        /**
         * @brief Trigger a key event of the cloud Gamepad.
         *
         * @param keycode The keycode. 
         * @param down true if the button is pressed, false if the button is released.
         */
        virtual void OnGamepadKey(int32_t keycode, bool down) const = 0;

        /**
         * @brief Trigger a stick event of the cloud Gamepad.
         *
         * @param type The stick type. See {@see KeyType#LS}, {@see KeyType#RS}.
         * @param x The X coordinate of the stick
         * @param y The Y coordinate of the stick
         */
        virtual void OnGamepadStick(const Gamepad::KeyType& type, int32_t x, int32_t y) const = 0;


        /**
         * @brief Trigger a trigger event of the cloud Gamepad.
         *
         * @param type The trigger type. See {@see KeyType#LT}, {@see KeyType#RT}.
         * @param value The absolute position of the trigger control. The value is normalized to a range from 0
         *         (released) to 255 (fully pressed).
         * @param down true if the trigger is pressed, false if the trigger is released.
         */
        virtual void OnGamepadTrigger(const Gamepad::KeyType& type, int32_t value, bool down) const = 0;
    };
}

#endif

