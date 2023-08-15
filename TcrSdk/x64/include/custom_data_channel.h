#ifndef TCRSDK_CUSTOM_DATA_CHANNEL_H_
#define TCRSDK_CUSTOM_DATA_CHANNEL_H_

#include <string>
#include <stdint.h>
#include "tcr_code.h"

namespace tcrsdk {
    /**
     * @file custom_data_channel.h
     * 
     * @brief This interface represents a data channel between the client and the cloud application.
     *
     * The client can send a message to the cloud application via the {@see #send(char*)} method, and receive the
     * reply message from the cloud application via {@see Observer#onMessage(int, char*)}.
     *
     * In terms of implementation, the {@see #send(char*)} method in fact sends the message to a UDP port of the
     * cloud machine, which is defined when creating this CustomDataChannel. The cloud application listens to this UDP port
     * to receive messages from the client. When the cloud application replies a message, the Cloud Rendering SDK will call
     * back the message to the client application through {@see Observer#OnMessage(int, char*)}. Note that the
     * associated UDP port identifies a data channel uniquely.
     *
     * @see TcrSession#createCustomDataChannel(int, Observer)
     */
	class __declspec(dllexport) CustomDataChannel
	{
	public:
        /**
         * @brief A class can implement the Observer interface when it wants to be informed of events in CustomDataChannel objects.
         */
        class Observer {
        public:

            /**
             * @brief This method is called when the listened 'CustomDataChannel' is connected successfully.
             *
             * @param port The identify of the listened 'CustomDataChannel'.
             */
            virtual void OnConnected(const int32_t port) = 0;

            /**
             * @brief This method is called whenever some error is happened in the listened 'CustomDataChannel'.
             *
             * @param port The identify of the listened 'CustomDataChannel'.
             * @param code The error code.
             * @param msg The error message.
             */
            virtual void OnError(const int32_t port, const TcrCode& code, const std::string& msg) = 0;

            /**
             * @brief This method is called whenever the listened 'CustomDataChannel' receives cloud message.
             *
             * NOTE: |buffer| will be freed once this function returns so callers who want to use the data
             * asynchronously must make sure to copy it first.
             *
             * @param port The identify of the listened 'CustomDataChannel'.
             * @param buffer The message sent from the cloud.
             */
            virtual void OnMessage(const int32_t port, const char* buffer) = 0;
        };

        /**
         * @brief Send data to the cloud Application.
         *
         * @param data The data to be sent, which cannot exceed 1,200 bytes.
         * @return true if success, false otherwise.
         */
        virtual bool Send(const char* data) = 0;

        /**
         * @brief Close this data channel.
         */
        virtual void Close() = 0;

    
	};
} // namespace tcrsdk

#endif // TCRSDK_CUSTOM_DATA_CHANNEL_H_

