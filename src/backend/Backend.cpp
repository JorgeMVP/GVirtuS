#include "gvirtus/backend/Backend.h"

#include <gvirtus/communicators/CommunicatorFactory.h>
#include <gvirtus/communicators/EndpointFactory.h>

#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>

using gvirtus::backend::Backend;

Backend::Backend(const fs::path &path) {
    // logger setup
    this->logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("Backend"));

    char *logLevel_envVar = getenv("GVIRTUS_LOGLEVEL");
    std::string logLevelString = (logLevel_envVar == nullptr ? std::string("") : std::string(logLevel_envVar));

    log4cplus::LogLevel logLevel = logLevelString.empty() ? log4cplus::INFO_LOG_LEVEL : std::stoi(logLevelString);
    this->logger.setLogLevel(logLevel);

    // json setup
    if (not (fs::exists(path) and fs::is_regular_file(path) and path.extension() == ".json")) {
        LOG4CPLUS_ERROR(logger, "✖ - " << fs::path(__FILE__).filename() << ":" << __LINE__ << ":" << " json path error: no such file.");
        exit(EXIT_FAILURE);
    }

    LOG4CPLUS_DEBUG(logger, "✓ - " << fs::path(__FILE__).filename() << ":" << __LINE__ << ":" << " Json file has been loaded.");

    // endpoints setup
    _properties = common::JSON<Property>(path).parser();
    _children.reserve(_properties.endpoints());

    if (_properties.endpoints() > 1) LOG4CPLUS_INFO(logger, "🛈  - Application serves on " << _properties.endpoints() << " several endpoint");

    for (int i = 0; i < _properties.endpoints(); i++) {
        _children.push_back(std::make_unique<Process>(
                communicators::CommunicatorFactory::get_communicator(
                        communicators::EndpointFactory::get_endpoint(path),
                        _properties.secure()),
                _properties.plugins().at(i)));
    }
}

void Backend::Start() {
    // std::function<void(std::unique_ptr<gvirtus::Thread> & children)> task =
    // [this](std::unique_ptr<gvirtus::Thread> &children) {
    //   LOG4CPLUS_DEBUG(logger, "✓ - [Thread " << std::this_thread::get_id() <<
    //   "]: Started."); children->Start(); LOG4CPLUS_DEBUG(logger, "✓ - [Thread "
    //   << std::this_thread::get_id() << "]: Finished.");
    // };
    LOG4CPLUS_DEBUG(logger, "✓ - [Process " << getpid() << "] " << "Backend::Start() called.");

    int pid = 0;

    // _children definition: "std::vector<std::unique_ptr<Process>> _children"
    for (int i = 0; i < _children.size(); i++) {
        activeChilds++;
        if ((pid = fork()) == 0) {
            _children[i]->Start();
            LOG4CPLUS_TRACE(logger, "Child exited.");
            break;
        }
    }

    pid_t pid_wait = 0;
    int stat_loc;
    if (pid != 0) {
        signal(SIGINT, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        LOG4CPLUS_TRACE(logger, "Active childs: %d" << activeChilds);

        int status;
        do {
            LOG4CPLUS_DEBUG(logger, "✓ - [Process " << getpid() << "] " << "Waiting for childs to terminate. Current active childs: " << activeChilds);
            int waitres = wait(&status);
            activeChilds--;

            LOG4CPLUS_TRACE(logger, "Active childs: %d" << activeChilds);

            if (waitres < 0) {
                LOG4CPLUS_TRACE(logger, "Error " << strerror(errno) << " on wait.");
                //throw "Backend: Error on wait: " + std::string(strerror(errno));
            }
            else {
                LOG4CPLUS_TRACE(logger, "Process " << waitres << " returned successfully.");
                break;
            }
        } while (not WIFEXITED(status) and not WIFSIGNALED(status));
    }

    LOG4CPLUS_INFO(logger, "✓ - No child processes are running. Use CTRL + C to terminate the backend.");

    signal(SIGINT, sigint_handler);
    pause();

    LOG4CPLUS_DEBUG(logger, "✓ - [Process " << getpid() << "] " << "Backend::Start() returned.");

}

void Backend::EventOccurred(std::string &event, void *object) {
    LOG4CPLUS_DEBUG(logger, "✓ - EventOccurred: " << event);
}


