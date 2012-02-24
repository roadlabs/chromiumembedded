// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//

#ifndef _PERMISSIONHANDLER_CTOCPP_H
#define _PERMISSIONHANDLER_CTOCPP_H

#ifndef BUILDING_CEF_SHARED
#pragma message("Warning: "__FILE__" may be accessed DLL-side only")
#else // BUILDING_CEF_SHARED

#include "include/cef.h"
#include "include/cef_capi.h"
#include "libcef_dll/ctocpp/ctocpp.h"

// Wrap a C structure with a C++ class.
// This class may be instantiated and accessed DLL-side only.
class CefPermissionHandlerCToCpp
    : public CefCToCpp<CefPermissionHandlerCToCpp, CefPermissionHandler,
        cef_permission_handler_t>
{
public:
  CefPermissionHandlerCToCpp(cef_permission_handler_t* str)
      : CefCToCpp<CefPermissionHandlerCToCpp, CefPermissionHandler,
          cef_permission_handler_t>(str) {}
  virtual ~CefPermissionHandlerCToCpp() {}

  // CefPermissionHandler methods
  virtual bool OnBeforeScriptExtensionLoad(CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame, const CefString& extensionName) OVERRIDE;
};

#endif // BUILDING_CEF_SHARED
#endif // _PERMISSIONHANDLER_CTOCPP_H

