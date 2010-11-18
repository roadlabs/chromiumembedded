// Copyright (c) 2010 The Chromium Embedded Framework Authors.
// Portions copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cef_process_ui_thread.h"
#include "browser_webkit_glue.h"
#include "browser_webkit_init.h"
#include "cef_context.h"

#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/metrics/stats_table.h"
#include "base/rand_util.h"
#include "base/string_number_conversions.h"
#include "build/build_config.h"
#include "app/gfx/gl/gl_implementation.h"
#include "net/base/net_module.h"
#if defined(OS_WIN)
#include "net/socket/ssl_client_socket_nss_factory.h"
#endif
#include "webkit/blob/blob_storage_controller.h"
#include "webkit/blob/blob_url_request_job.h"
#include "webkit/glue/plugins/plugin_list.h"
#include "webkit/extensions/v8/gc_extension.h"
#include "net/url_request/url_request.h"

#if defined(OS_WIN)
#include <commctrl.h>
#include <Objbase.h>
#endif

static const char* kStatsFilePrefix = "libcef_";
static int kStatsFileThreads = 20;
static int kStatsFileCounters = 200;

namespace {

URLRequestJob* BlobURLRequestJobFactory(URLRequest* request,
                                        const std::string& scheme) {
  webkit_blob::BlobStorageController* blob_storage_controller =
      static_cast<BrowserRequestContext*>(request->context())->
          blob_storage_controller();
  return new webkit_blob::BlobURLRequestJob(
      request,
      blob_storage_controller->GetBlobDataFromUrl(request->url()),
      NULL);
}

} // namespace


CefProcessUIThread::CefProcessUIThread()
      : CefThread(CefThread::UI), statstable_(NULL), webkit_init_(NULL) {}

CefProcessUIThread::CefProcessUIThread(MessageLoop* message_loop)
      : CefThread(CefThread::UI, message_loop), statstable_(NULL),
        webkit_init_(NULL) {}

CefProcessUIThread::~CefProcessUIThread() {
  // We cannot rely on our base class to stop the thread since we want our
  // CleanUp function to run.
  Stop();
}

void CefProcessUIThread::Init() {
  PlatformInit();

#ifndef _DEBUG
  // Only log error messages and above in release build.
  logging::SetMinLogLevel(logging::LOG_ERROR);
#endif

  // Initialize the global CommandLine object.
  CommandLine::Init(0, NULL);

  // Initialize WebKit.
  webkit_init_ = new BrowserWebKitInit();

  // Initialize WebKit encodings
  webkit_glue::InitializeTextEncoding();

  // Load ICU data tables.
  bool ret = icu_util::Initialize();
  if(!ret) {
#if defined(OS_WIN)
    MessageBox(NULL, L"Failed to load the required icudt38 library",
      L"CEF Initialization Error", MB_ICONERROR | MB_OK);
#endif
    return;
  }

  // Config the network module so it has access to a limited set of resources.
  net::NetModule::SetResourceProvider(webkit_glue::NetResourceProvider);

  // Load and initialize the stats table.  Attempt to construct a somewhat
  // unique name to isolate separate instances from each other.
  statstable_ = new base::StatsTable(
      kStatsFilePrefix + base::Uint64ToString(base::RandUint64()),
      kStatsFileThreads,
      kStatsFileCounters);
  base::StatsTable::set_current(statstable_);

  // CEF always exposes the GC.
  webkit_glue::SetJavaScriptFlags("--expose-gc");
  // Expose GCController to JavaScript.
  WebKit::WebScriptController::registerExtension(
      extensions_v8::GCExtension::Get());

#if defined(OS_WIN)
  // Use NSS for SSL on Windows.  TODO(wtc): this should eventually be hidden
  // inside DefaultClientSocketFactory::CreateSSLClientSocket.
  net::ClientSocketFactory::SetSSLClientSocketFactory(
      net::SSLClientSocketNSSFactory);
#endif

  gfx::InitializeGLBindings(gfx::kGLImplementationDesktopGL);

  URLRequest::RegisterProtocolFactory("blob", &BlobURLRequestJobFactory);

  if (!_Context->cache_path().empty()) {
    // Create the storage context object.
    _Context->set_storage_context(new DOMStorageContext());
  }

  const CefSettings& settings = _Context->settings();
  
  if (settings.user_agent)
    webkit_glue::SetUserAgent(WideToUTF8(settings.user_agent));
  
  if (settings.extra_plugin_paths) {
    cef_string_t str;
    FilePath path;
    int size = cef_string_list_size(settings.extra_plugin_paths);
    for(int i = 0; i < size; ++i) {
      str = cef_string_list_value(settings.extra_plugin_paths, i);
#if defined(OS_WIN)
      path = FilePath(str);
#else
      path = FilePath(WideToUTF8(str));
#endif
      NPAPI::PluginList::Singleton()->AddExtraPluginPath(path);
    }
  }
}

void CefProcessUIThread::CleanUp() {
  // Flush any remaining messages.  This ensures that any accumulated
  // Task objects get destroyed before we exit, which avoids noise in
  // purify leak-test results.
  MessageLoop::current()->RunAllPending();

  // Destroy the storage context object.
  _Context->set_storage_context(NULL);

  // Tear down the shared StatsTable.
  base::StatsTable::set_current(NULL);
  delete statstable_;
  statstable_ = NULL;

  // Shut down WebKit.
  delete webkit_init_;
  webkit_init_ = NULL;

  PlatformCleanUp();
}
