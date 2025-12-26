// test.cpp: 定义应用程序的入口点。
//

#include "av_base.h"
#include "test.h"
//#include "httplib/httplib.h"
//#include "cron.h"
#include "av_log.h"
#include "config.h"
#include "logger.h"
#include "error_code.h"
//#include "av_async.h"
//#include "av_queue.h"
//#include "av_base64.h"
#include "publish.h"
#include "av_path.h"
#include "ffmpeg.h"

using namespace std;

int main()
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	if (!Logger::instance().open()) {
		return ErrorCode::ErrOpenLogFailed;
	}

	logi("中文测试");

	if (!Config::instance().parse(av::str::toT("config.toml"))) {
		loge("parse config.toml failed");
		return ErrorCode::ErrParseConfigFileFailed;
	}
	auto& config = Config::instance();
	
	logi("server start ==================================");

	av::path::create_dir(TEXT("中文目录"));
	av::path::create_dir(TEXT("中文目录1"));
	av::path::remove_dir(TEXT("中文目录1"));

	screenshot("/home/marcello/tmp/1153734.mp4");

	return 0;

#ifdef _WIN32
	Publish p(av::str::toT("D:\\Downloads\\tmp - 复制"));
#else
	Publish p(av::str::toT("/home/marcello/tmp"));
#endif
	p.start();

	//Queue<int> q;
	//Cron a("1", "*/1 * * * * *", [&q] {
	//	av::async::Exit exit_cron([&q] {
	//		logi("test aa end base64: {}", av::base64::encode("1234567890"));
	//		q.push(2);
	//		});
	//	q.push(1);
	//	});
	//a.start();

	//auto tmp_thread = std::thread([&a, &q] {
	//	std::this_thread::sleep_for(std::chrono::seconds(10));
	//	a.stop();
	//	q.close();
	//});

	//auto tmp_thread2 = std::thread([&q] {
	//	int a = 0;
	//	while (q.pop(a)) {
	//		logi("THREAD2 q val {}", a);
	//	}
	//	logi("q closed pop thread exit");
	//});

	//auto tmp_thread3 = std::thread([&q] {
	//	int a = 0;
	//	while (q.pop(a)) {
	//		logi("THREAD3 q val {}", a);
	//	}
	//	logi("q closed pop thread exit");
	//	});

	//httplib::Server svr;

	//svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
	//	res.set_content("123", "text/plain");
	//});

	//// check
	//if (config.server.host == "" || config.server.port == 0) {
	//	loge("server param error {}:{}", config.server.host, config.server.port);
	//	return ErrorCode::ErrServerParam;
	//}

	//// bind
	//int port = svr.bind_to_port(config.server.host, config.server.port);
	//if (port < 0) {
	//	loge("bind_to_port {}:{} failed", config.server.host, config.server.port);
	//	return ErrorCode::ErrServerBind;
	//}

	//// listen
	//if (!svr.listen_after_bind()) {
	//	loge("listen_after_bind {}:{} failed", config.server.host, config.server.port);
	//	return ErrorCode::ErrServerListen;
	//}
	return ErrorCode::Success;
}