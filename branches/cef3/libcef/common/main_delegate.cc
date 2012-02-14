// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libcef/common/main_delegate.h"
#include "libcef/browser/content_browser_client.h"
#include "libcef/browser/context.h"
#include "libcef/common/cef_switches.h"
#include "libcef/plugin/content_plugin_client.h"
#include "libcef/renderer/content_renderer_client.h"
#include "libcef/utility/content_utility_client.h"

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "content/public/browser/browser_main_runner.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

CefMainDelegate::CefMainDelegate() {
}

CefMainDelegate::~CefMainDelegate() {
}

bool CefMainDelegate::BasicStartupComplete(int* exit_code) {
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);

  if (process_type.empty()) {
    // In the browser process. Populate the global command-line object.
    const CefSettings& settings = _Context->settings();

    if (settings.user_agent.length > 0) {
      command_line->AppendSwitchASCII(switches::kUserAgent,
          CefString(&settings.user_agent));
    } else if (settings.product_version.length > 0) {
      command_line->AppendSwitchASCII(switches::kProductVersion,
          CefString(&settings.product_version));
    }

    if (settings.locale.length > 0) {
      command_line->AppendSwitchASCII(switches::kLocale,
          CefString(&settings.locale));
    }

    if (settings.pack_file_path.length > 0) {
      FilePath file_path = FilePath(CefString(&settings.pack_file_path));
      if (!file_path.empty())
        command_line->AppendSwitchPath(switches::kPackFilePath, file_path);
    }

    if (settings.locales_dir_path.length > 0) {
      FilePath file_path = FilePath(CefString(&settings.locales_dir_path));
      if (!file_path.empty())
        command_line->AppendSwitchPath(switches::kLocalesDirPath, file_path);
    }

    // TODO(cef): Figure out how to support the sandbox.
    if (!command_line->HasSwitch(switches::kNoSandbox))
      command_line->AppendSwitch(switches::kNoSandbox);
  }

  return false;
}

void CefMainDelegate::PreSandboxStartup() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);

  content::SetContentClient(&content_client_);
  InitializeContentClient(process_type);

  InitializeResourceBundle();
}

void CefMainDelegate::SandboxInitialized(const std::string& process_type) {
}

int CefMainDelegate::RunProcess(
    const std::string& process_type,
    const content::MainFunctionParams& main_function_params) {
  if (process_type.empty()) {
    // Use our own browser process runner.
    browser_runner_.reset(content::BrowserMainRunner::Create());

    // Initialize browser process state. Results in a call to
    // CefBrowserMain::GetMainMessageLoop().
    int exit_code = browser_runner_->Initialize(main_function_params);
    if (exit_code >= 0)
      return exit_code;

    return 0;
  }

  return -1;
}

void CefMainDelegate::ProcessExiting(const std::string& process_type) {
  ResourceBundle::CleanupSharedInstance();
}

#if defined(OS_MACOSX)
bool CefMainDelegate::ProcessRegistersWithSystemProcess(
    const std::string& process_type) {
  return false;
}

bool CefMainDelegate::ShouldSendMachPort(const std::string& process_type) {
  return false;
}

bool CefMainDelegate::DelaySandboxInitialization(
    const std::string& process_type) {
  return false;
}

#elif defined(OS_POSIX)
content::ZygoteForkDelegate* CefMainDelegate::ZygoteStarting() {
  return NULL;
}

void CefMainDelegate::ZygoteForked() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);
  InitializeContentClient(process_type);
}
#endif  // OS_MACOSX

void CefMainDelegate::ShutdownBrowser() {
  if (browser_runner_.get()) {
    browser_runner_->Shutdown();
    browser_runner_.reset(NULL);
  }
}

void CefMainDelegate::InitializeContentClient(
    const std::string& process_type) {
  if (process_type.empty()) {
    browser_client_.reset(new CefContentBrowserClient);
    content::GetContentClient()->set_browser(browser_client_.get());

    // Single-process is an unsupported and not fully tested mode.
    const CommandLine& command_line = *CommandLine::ForCurrentProcess();
    if (command_line.HasSwitch(switches::kSingleProcess)) {
      content::RenderProcessHost::set_run_renderer_in_process(true);
      renderer_client_.reset(new CefContentRendererClient);
      content::GetContentClient()->set_renderer(renderer_client_.get());
    }
  } else if (process_type == switches::kRendererProcess) {
    renderer_client_.reset(new CefContentRendererClient);
    content::GetContentClient()->set_renderer(renderer_client_.get());
  } else if (process_type == switches::kPluginProcess) {
    plugin_client_.reset(new CefContentPluginClient);
    content::GetContentClient()->set_plugin(plugin_client_.get());
  } else if (process_type == switches::kUtilityProcess) {
    utility_client_.reset(new CefContentUtilityClient);
    content::GetContentClient()->set_utility(utility_client_.get());
  }
}

void CefMainDelegate::InitializeResourceBundle() {
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();

  FilePath pak_file, locales_dir;

  if (command_line.HasSwitch(switches::kPackFilePath))
    pak_file = command_line.GetSwitchValuePath(switches::kPackFilePath);

  if (pak_file.empty()) {
    FilePath pak_dir;
    PathService::Get(base::DIR_MODULE, &pak_dir);
    pak_file = pak_dir.Append(FILE_PATH_LITERAL("cef.pak"));
  }

  if (!pak_file.empty())
    PathService::Override(ui::FILE_RESOURCES_PAK, pak_file);

  if (command_line.HasSwitch(switches::kLocalesDirPath))
    locales_dir = command_line.GetSwitchValuePath(switches::kLocalesDirPath);

  if (!locales_dir.empty())
    PathService::Override(ui::DIR_LOCALES, locales_dir);

  std::string locale = command_line.GetSwitchValueASCII(switches::kLocale);
  if (locale.empty())
    locale = "en-US";

  const std::string loaded_locale =
      ui::ResourceBundle::InitSharedInstanceWithLocale(locale);
  CHECK(!loaded_locale.empty()) << "Locale could not be found for " << locale;

#if defined(OS_WIN)
  // Explicitly load cef.pak on Windows.
  if (file_util::PathExists(pak_file))
    ResourceBundle::AddDataPackToSharedInstance(pak_file);
  else
    NOTREACHED() << "Could not load cef.pak";
#endif
}
