#include "av_time.h"

namespace av {
	namespace time {

		std::chrono::steady_clock::time_point now() {
			return std::chrono::steady_clock::now();
		}

		int64_t seconds() {
			return std::chrono::duration_cast<std::chrono::seconds>(now().time_since_epoch());
		}

		int64_t milliseconds() {
			std::chrono::duration_cast<std::chrono::milliseconds>(now().time_since_epoch());
		}

		int64_t microseconds() {
			return std::chrono::duration_cast<std::chrono::microseconds>(now().time_since_epoch());
		}
	}
}

