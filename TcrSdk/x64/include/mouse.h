#ifndef TCRSDK_MOUSE_H_
#define TCRSDK_MOUSE_H_

#include<stdint.h>

namespace tcrsdk {
    /**
     * @brief This interface is used to interact with the virtual Mouse device on the server.
     */
    class __declspec(dllexport) Mouse
    {
    public:
        /**
         * @brief General mouse button types
         */
        enum class MouseKeyType {
            /**
             * @brief The left mouse button
             */
            LEFT,
            /**
             * @brief The right mouse button
             */
            RIGHT,
            /**
             * @brief The middle mouse button (scroll wheel)
             */
            MIDDLE,
            /**
             * @brief The forward side button
             */
            FORWARD,
            /**
             * @brief The back side button
             */
            BACKWARD
        };

        /**
         * @brief The style of the cloud cursor
         */
        enum class CursorStyle {
            /**
             * @brief The default cursor style
             **/
            NORMAL,

            /**
             * @brief The big cursor style
             **/
            HUGE
        };

        /**
         * @brief Set the cursor style.
         * 
         * The cursor style {@see CursorStyle} will be updated immediately: the new cursor style will be distributed
         * from the cloud through the data channel. You can get the latest mouse style image from Observer.
         *
         * @param cursorStyle The new cursor style, which will overwrite the original style value.
         */
        virtual void SetMouseCursorStyle(const CursorStyle& cursor_style) const = 0;

        /**
         * @brief Trigger a click event of the cloud mouse.
         *
         * @param key The mouse button. Valid values: `left`: The left button; `middle`: The scroll wheel; `right`:
         *         The right button; `forward`: The forward side button; `backward`: The back side button.
         * @param down Valid values: `true`: Press; `false`: Release.
         */
        virtual void OnMouseKey(const MouseKeyType& key, bool down) const = 0;

        /**
         * @brief Rotate the scroll wheel of the cloud mouse.
         *
         * @param delta Valid values: -1.0~1.0
         */
        virtual void OnMouseScroll(float delta) const = 0;

        /**
         * @brief After you get the coordinates of the local View, you need to convert the coordinates to the coordinates of the
         * remote end before calling this method.Before calculation, you need to get the width and height of the gameView
         * [localViewWidth, localViewHeight] and the position of the click position in the game screen [localX, localY].
         * Then calculate the coordinates through the following sample code to get [remoteX,remoteY].<br>
         * Finally, call the interface to send the mouse movement event.
         * You can refer to the sample code below to complete your own calculations.
         * 
         *      // localX/localY: click coordinates on gameView
         *      // localViewWidth/localViewHeight: gameView width&height
         *      remoteX = localX * 8192 / localViewWidth;
         *      remoteY = localY * 8192 / localViewHeight;
         *      // send mouse move message
         *      onMoveTo(remoteX,remoteY);
         *
         * @param x The X coordinate on the remote device
         * @param y The Y coordinate on the remote device
         */
        virtual void OnMouseMoveTo(int32_t x, int32_t y) const = 0;
    };
}

#endif
