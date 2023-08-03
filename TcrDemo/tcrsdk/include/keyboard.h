#ifndef TCRSDK_KEYBOARD_H_
#define TCRSDK_KEYBOARD_H_

#include<stdint.h>
#include "async_callback.h"

namespace tcrsdk{
    /**
     * This interface is used to interact with the virtual Keyboard device on the server.
     */
    class __declspec(dllexport) Keyboard
    {
    public:
        /**
         * Trigger a key event of the cloud keyboard.
         *
         * @param keycode The keycode
         * @param down Valid values: `true`: Press; `false`: Release.
         */
        virtual void OnKeyboard(int32_t keycode, bool down) const = 0;

        /**
         * Trigger a key event of the cloud keyboard. Differentiate between the left and right keys of the
         * keyboard.
         *
         * @param keycode The Windows keycode. 
         * @param down Valid values: `true`: Press; `false`: Release.
         * @param isLeft Used to distinguish between left and right keys of the keyboard: `true`: Left; `false` :
         *         Right.
         */
        virtual void OnKeyboard(int32_t keycode, bool down, bool isLeft) const = 0;

        /**
         * Query the capitalization status of the cloud virtual keyboard.
         *
         * @param callback Asynchronous callback object for returning results.
         */
        virtual void CheckKeyboardCapsLock(std::shared_ptr<AsyncCallback> callback) const = 0;

        /**
         * Reset the capitalization status (to lowercase) of the cloud virtual keyboard.
         */
        virtual void ResetKeyboardCapsLock() const = 0;

        /**
         * Reset the key status of the cloud keyboard.
         * This API is used to release all keys when there are stuck keys on the keyboard.
         */
        virtual void ResetKeyboard() const = 0;
    };
}
#endif

