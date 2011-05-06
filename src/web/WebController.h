// This may look like C code, but it's really -*- C++ -*-
/*
 * Copyright (C) 2008 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#ifndef WEBCONTROLLER_H_
#define WEBCONTROLLER_H_

#include <string>
#include <vector>
#include <set>
#include <map>

#include <Wt/WDllDefs.h>
#include <Wt/WSocketNotifier>

#include "SocketNotifier.h"

#if defined(WT_THREADED) && !defined(WT_TARGET_JAVA)
#include <boost/thread.hpp>
#include "threadpool/threadpool.hpp"
#endif

namespace Wt {

class Configuration;
class EntryPoint;

class WebRequest;
class WebSession;
class WebStream;

class WApplication;
class WAbstractServer;

#ifndef WT_CNOR
/*
 * An event to be delivered to a session which is not caused by a web
 * request (or, probably not one for that session).
 */
struct ApplicationEvent {
  ApplicationEvent(const std::string& aSessionId,
		   const boost::function<void ()>& aFunction,
                   const boost::function<void ()>& aFallbackFunction
		     = boost::function<void ()>())
    : sessionId(aSessionId),
      function(aFunction),
      fallbackFunction(aFallbackFunction)
  { }

  std::string sessionId;
  boost::function<void ()> function;
  boost::function<void ()> fallbackFunction;
};
#endif

/*
 * The controller is a singleton class
 *
 * It either listens for incoming requests from a webstream, using run(),
 * or it may be used to handle an incoming request, using handleRequest().
 * In the latter case, sessions will only expire with a delay -- at the
 * next request. Seems harmless to me (but causes confusion to others).
 *
 * There is a method forceShutDown() to quit the controller.
 *
 * It has the following tasks:
 *  - handle session life-cycle: create new sessions, delete quit()ed
 *    sessions, expire sessions on timeout
 *  - handles web requests and application events
 */
class WT_API WebController
{
public:
  static bool isAsyncSupported() { return true; }

  WApplication *doCreateApplication(WebSession *session);
  Configuration& configuration();

  void removeSession(const std::string& sessionId);

#ifndef WT_TARGET_JAVA
  /*
   * Construct the WebController and let it read requests from the given
   * streams.
   */
  WebController(Configuration& configuration, WAbstractServer *server,
		WebStream *stream, std::string singleSessionId = std::string());

  ~WebController();

  void run();
  int sessionCount() const;

  // Returns whether we should continue receiving data.
  bool requestDataReceived(WebRequest *request, boost::uintmax_t current,
			   boost::uintmax_t total);

  void handleRequest(WebRequest *request);

#ifndef WT_CNOR
  bool handleApplicationEvent(const ApplicationEvent& event);
  void post(const boost::function<void()>& function);
#endif // WT_CNOR

  bool expireSessions();
  void forceShutdown();

  static std::string sessionFromCookie(std::string cookies,
				       std::string scriptName,
				       int sessionIdLength);

  typedef std::map<int, WSocketNotifier *> SocketNotifierMap;

  void addSocketNotifier(WSocketNotifier *notifier);
  void removeSocketNotifier(WSocketNotifier *notifier);

  void addUploadProgressUrl(const std::string& url);
  void removeUploadProgressUrl(const std::string& url);

  // returns false if removeSocketNotifier was called while processing
  void socketSelected(int descriptor, WSocketNotifier::Type type);

  std::string switchSession(WebSession *session, const std::string& newSessionId);
  std::string generateNewSessionId(boost::shared_ptr<WebSession> session);

  WAbstractServer *server_;

private:
  Configuration&   conf_;
  WebStream       *stream_;
  std::string      singleSessionId_;
  bool             running_;

#ifdef WT_THREADED
  boost::mutex uploadProgressUrlsMutex_;
#endif // WT_THREADED
  std::set<std::string> uploadProgressUrls_;

  typedef std::map<std::string, boost::shared_ptr<WebSession> > SessionMap;
  SessionMap sessions_;

  bool shutdown_;

#ifdef WT_THREADED
  SocketNotifier socketNotifier_;
  // mutex to protect access to notifier maps. This cannot be protected
  // by mutex_ as this lock is grabbed while the application lock is
  // being held, which would potentially deadlock if we took mutex_.
  boost::recursive_mutex notifierMutex_;
  SocketNotifierMap socketNotifiersRead_;
  SocketNotifierMap socketNotifiersWrite_;
  SocketNotifierMap socketNotifiersExcept_;
  // assumes that you did grab the notifierMutex_
  SocketNotifierMap& socketNotifiers(WSocketNotifier::Type type);
  void socketNotify(int descriptor, WSocketNotifier::Type type);

  // mutex to protect access to the sessions map.
  boost::recursive_mutex mutex_;

  boost::threadpool::pool threadPool_;
#endif

  void handleAsyncRequest(WebRequest *request);
  void handleRequestThreaded(WebRequest *request);
  void updateResourceProgress(WebRequest *request,
			      boost::uintmax_t current,
			      boost::uintmax_t total);

  const EntryPoint *getEntryPoint(WebRequest *request);

  static std::string appSessionCookie(std::string url);

#endif // WT_TARGET_JAVA
};

}

#endif // WEBCONTROLLER_H_
