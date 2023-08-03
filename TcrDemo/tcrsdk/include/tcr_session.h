#ifndef TCRSDK_TCR_SESSION_H_
#define TCRSDK_TCR_SESSION_H_

#include <string>
#include <stdint.h>
#include <memory>

#include "custom_data_channel.h"
#include "keyboard.h"
#include "mouse.h"
#include "gamepad.h"
#include "tcr_logger.h"
#include "video_frame_observer.h"


namespace tcrsdk {

/**
 * This interface represents a session between the client and the TCR server.
 */
 class __declspec(dllexport) TcrSession {
 public:
   /**
     * Session event definitions.
     *
     * @see Observer#onEvent(Event, Object)
     */
    enum class Event {
        /**
         * This event indicates that the session has been initialized.
         *
         * The associated event data is of type String and represents the 'ClientSession' which is further
         * used as a parameter to call the Cloud API 'CreateSession' @see https://cloud.tencent.com/document/api/1162/40740
         */
        STATE_INITED,
        /**
         * This event indicates that the session is connected.
         *
         * This event has no associated data.
         */
        STATE_CONNECTED,
        /**
         * This event indicates that the session is reconnecting. You can do some interaction to prompt your user.
         *
         * This event has no associated data.
         */
        STATE_RECONNECTING,
        /**
         * This event indicates that the session is closed, and thus it can not be used any more.
         *
         * The associated event data is of type String and represents the reason for the close.
         */
        STATE_CLOSED,

        /**
         * This event indicates that the performance data is updated. 
         *
         * The associated event data is of type string in json format:
         * {
         *      "bitrate":number, // The calculated bitrate of the audio and video bytes received over the network.
         *      "cpu":string, // CPU usage of the remote device
         *      "fps":number, // The frame rate of the decoded video
         *      "gpu":string, // GPU usage of the remote device
         *      "packet_lost":number, // The number of discarded packets
         *      "rtt":number // The round-trip event to STUN
         * }
         */
        CLIENT_STATS,

        /**
         * This event indicates that the status of the game process on the server has been changed.
         *
         * The associated event data is of type string in json format:
         * {
         *     user_id: string,      // The user ID
         *     game_id: string,      // The game ID
         *     app_id: number,
         *     request_id: string,
         *     status: number,       // The game start error code. Valid values:
         *                          // `0`: Started the game successfully;
         *                          // `1`: Failed to start the game;
         *                          // `2`: The game status changed from not running to running;
         *                          // `3`: The game status changed from running to not running.
         * }
         */
        GAME_START_COMPLETE,
        /**
         * This event indicates the status of the archive loading in the server.
         *
         * The associated event data is of type String in json format:
         * {
         *     user_id: string,      // The user ID
         *     game_id: string,      // The game ID
         *     name: string,         // The eventual filename of the archive
         *     url: string,          // The archive download address
         *     status: number,       // The error code of the archive loading status.
         *                          // Valid values:
         *                          // `0`: loaded the archive successfully;
         *                          // `1`: Failed to download the archive;
         *                          // `2`: Failed to verify the archive;
         *                          // `3`: Failed to extract the archive;
         *                          // `4`: Other error;
         *                          // `5`: The archive was being downloaded.
         *     save_type: string,    // "`Auto` or `Normal`",
         *     category_id: string,  // The archive category ID
         *     archive_size: number, // The archive size
         *     loaded_size: number  // The size of the downloaded archive
         * }
         */
        ARCHIVE_LOAD_STATUS,
        /**
         * This event indicates the status of the archive saving in the server.
         *
         * The associated event data is of type String in json format:
         * {
         *     user_id: String      // The user ID
         *     game_id: String      // The game ID
         *     name: String         // The eventual filename of the archive
         *     md5: String          // The MD5 value of the archive
         *    status: number        // The error code of the archive saving status.
         *                          // Valid values:
         *                          // `0`: saved the archive successfully;
         *                          // `1`: Failed to save the archive;
         *                          // `2`: Failed to compress the archive;
         *                          // `3`: Other error;
         *                          // `4`: The archive was being uploaded;
         *                          // `5`: The archive could not be found;
         *                          // `6`: The archive operations were too frequent.
         *   save_type: String      // "`Auto` or `Normal`",
         *   category_id: String    // The archive category ID
         *   archive_size: number   // The archive size
         *   saved_size: number     // The size of the saved archive
         * }
         */
        ARCHIVE_SAVE_STATUS,

        /**
         * This event indicates that the input status of the server has changed. 
         *
         * The associated event data is of type string in json format:
         * {
         *   status: String  // If the status distributed by the cloud is `disabled`, input is disabled.
         * }
         */
        INPUT_STATUS_CHANGED,
        /**
         * This event indicates that the remote desktop information is updated. 
         *
         * The associated event data is of type string in json format:
         * {
         *      "app_window_left_offset":number, // Offset of the horizontal axis of the remote desktop.
         *                                       // The picture received locally is an offset picture, so the coordinates to be sent need to be converted
         *                                       // according to the width of the remote desktop and an offset needs to be added.
         * 
         *      "app_window_top_offset":number,  // Vertical offset of the remote desktop.
         *                                       // The image received locally is an offset image, so the coordinates to be sent need to be converted
         *                                       // according to the height of the remote desktop and then offset.
         *      "cursor_showing":bool, // cursor show status true:showing false:hidden.
         *      "screen_height":number, // The width of the remote desktop, which is used to calculate the ratio of the local and remote coordinate transformations.
         *      "screen_width":number // The height of the remote desktop, which is used to calculate the ratio of the local and remote coordinate transformations.
         *  }
         */
        REMOTE_DESKTOP_INFO,
        /**
         * This event indicates that the configuration of the cloud screen configuration has been changed.<br>
         *
         * The associated event data is of type {@link ScreenConfig}:
         * {
         *      "orientation":string, // Activity direction of cloud application
         *      "width":number, // The width of the video screen resolution
         *      "height":number // The height of the video screen resolution
         * }
         */
        SCREEN_CONFIG_CHANGE,
        /**
         * This event indicates that the remote cursor image information is updated.
         *
         * The associated event data is of type string in json format:
         * {
         *      "cursor_bitmap":string, // cursorBitmap mouse pointer picture, is a base64 encoded bitmap.
         *      "hot_spot_x":number, // hot_spot_x mouse x coordinate offset, default value is 0 (The mouse pointer points to the upper left vertex)
         *      "hot_spot_y":number // hot_spot_y mouse y coordinate offset, default value is 0 (The mouse pointer points to the upper left vertex)
         * }
         */
        CURSOR_IMAGE_INFO,
        /**
         * This event indicates that the showing status of cloud cursor is changed. 
         *
         * The associated event data is of type string in json format:
         * {
         *     "cursor_show_state":bool // Cloud cursor showing state
         * }
         */
        CURSOR_STATE_CHANGE,
    };

  class Observer {
  public:
      /**
        * This will be called when some event happened in the session.
        *
        * @param event @see Event.
        * @param eventData The data relative to the event.
        */
      virtual void onEvent(Event event, const char* eventData) = 0;
  };

  TcrSession(std::shared_ptr<TcrSession::Observer> observer);
  ~TcrSession();

 /*
  * @brief Starts the session. This method should only be called once.
  */
  void Init();

 /**
  * @brief Starts the session. This method should only be called once.
  *
  * @param serverSession The ServerSession returned from CreateSession API. Check out the https://cloud.tencent.com/document/api/1162/40740 website for details.
  *         
  * @return true if success, false otherwise.
  */
  bool Start(const char* serverSession);

 /**
  * @brief Resume the media stream.
  */
  void PauseStreaming();

 /**
  * @brief Resume the media stream.
  */
  void ResumeStreaming();

 /**
  * @brief Set the local video profile.
  *
  * @param width The frame width. Suggested value: 640.
  * @param height The frame height. Suggested value: 480.
  * @param fps The frame rate. Suggested value: 30.
  * @param minBitrate The minimum bitrate in kbps. Suggested value: 512.
  * @param maxBitrate The maximum bitrate in kbps. Suggested value: 1024.
  * @param isFrontCamera whether to use the front camera. Suggested value: true.
  */
  void SetRemoteVideoProfile(int32_t fps, int32_t minBitrate, int32_t maxBitrate, std::shared_ptr<AsyncCallback> callback);

 /**
  * @brief Set the resolution of Cloud Desktop.
  *
  * If the cloud PC application is in full-screen mode, the resolution of the downstream video stream will also change accordingly.
  * 
  * This method is currently only valid for scenarios where the cloud is a PC application, and the scenario where the
  * cloud is a mobile application is to be supported.
  *
  * @param width Width of cloud desktop.
  * @param height Height of cloud desktop.
  */
  void SetRemoteDesktopResolution(int32_t width, int32_t height);

 /**
  * @brief Set the callback of the video frame.
  * 
  * This observer will callback the decoded frame, which contains I420Buffer frame data and some frame information.
  * 
  * @param video_frame_observer This observer will callback the decoded frame.
  * @see VideoFrameObserver
  */
  void SetVideoFrameObserver(std::shared_ptr<VideoFrameObserver> video_frame_observer);

 /**
  * @brief Restart the cloud application process.
  */
  void RestartCloudApp();

 /**
  * @brief Copy the text to the input box in the cloud application.
  * 
  * The input box of the cloud application must have obtained the focus and allow the user to paste text from the
  * clipboard.
  *
  * @param msg The text to be pasted
  */
  void PasteText(const char* msg);

 /**
  * @brief Creates a custom data channel.
  *
  * @param port The cloud port number uniquely identifying the data channel.
  * @param CustomDataChannel.Observer.
  * @return the created data channel.
  * @see CustomDataChannel
  */
  std::shared_ptr<CustomDataChannel> CreateCustomDataChannel(int32_t port, std::shared_ptr<CustomDataChannel::Observer>observer);

 /**
  * @brief Return the interface to interact with the cloud keyboard in this session.
  *
  * @return The interface to interact with the cloud keyboard in this session.
  */
  const Keyboard* GetKeyboard();

 /**
  * @brief Return the interface to interact with the cloud Mouse in this session.
  *
  * @return The interface to interact with the cloud Mouse in this session.
  */
  const Mouse* GetMouse();

 /**
  * @brief Return the interface to interact with the cloud Gamepad in this session.
  *
  * @return The interface to interact with the cloud Gamepad in this session.
  */
  const Gamepad* GetGamepad();

 private:
     class TcrSessionImpl;
     std::shared_ptr<TcrSessionImpl>impl_;
};
}



#endif
