#ifndef VIDEO_FRAME_OBSERVER_H_
#define VIDEO_FRAME_OBSERVER_H_

#include <memory>
#include <stdint.h>

namespace tcrsdk {
	enum class __declspec(dllexport) VideoRotation {
		kVideoRotation_0 = 0,
		kVideoRotation_90 = 90,
		kVideoRotation_180 = 180,
		kVideoRotation_270 = 270
	};

	class __declspec(dllexport) I420Buffer {
	public:
		I420Buffer(const uint8_t*  data_y, const uint8_t* data_u, const uint8_t* data_v,
			int32_t stride_y, int32_t stride_u, int32_t stride_v, int32_t width, int32_t height);
		~I420Buffer();
		const uint8_t* DataY();
		const uint8_t* DataU();
		const uint8_t* DataV();

		int32_t StrideY();
		int32_t StrideU();
		int32_t StrideV();

		int32_t width();
		int32_t height();

	private:
		const uint8_t* data_y_;
		const uint8_t* data_u_;
		const uint8_t* data_v_;
		int32_t stride_y_;
		int32_t stride_u_;
		int32_t stride_v_;
		int32_t width_;
		int32_t height_;
	};

	class __declspec(dllexport) VideoFrame {
	public:
		VideoFrame(){}
		VideoFrame(const std::shared_ptr<I420Buffer> buffer,
			int64_t timestamp_us,
			uint32_t timestamp_rtp,
			int64_t ntp_time_ms,
			VideoRotation rotation);

		~VideoFrame();

		// Get frame width.
		int32_t width() const;
		// Get frame height.
		int32_t height() const;
		// Get frame size in pixels.
		uint32_t size() const;
		// System monotonic clock, same timebase as rtc::TimeMicros().
		int64_t timestamp_us() const;
		// Get frame timestamp (90kHz).
		uint32_t timestamp() const;
		// Get capture ntp time in milliseconds.
		int64_t ntp_time_ms() const;
		// Get frame rotation
		VideoRotation rotation() const;
		// Get I420Buffer
		std::shared_ptr<I420Buffer> i420_buffer() const;

	private:
		std::shared_ptr<I420Buffer> i420_buffer_;
		int32_t width_;
		int32_t height_;
		uint32_t size_;
		int64_t timestamp_us_;
		uint32_t timestamp_;
		int64_t ntp_time_ms_;
		VideoRotation rotation_;
	};

	class __declspec(dllexport) VideoFrameObserver
	{
	public:
		virtual void OnFrame(const VideoFrame& videoFrame) = 0;
	};
}

#endif // !VIDEO_FRAME_OBSERVER_H_


