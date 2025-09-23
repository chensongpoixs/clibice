/******************************************************************************
 *  Copyright (c) 2025 The CRTC project authors . All Rights Reserved.
 *
 *  Please visit https://chensongpoixs.github.io for detail
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 ******************************************************************************/
 /*****************************************************************************
				   Author: chensong
				   date:  2025-09-21



 ******************************************************************************/


#include "libice/transport_description.h"

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "libice/p2p_constants.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/logging.h"
#include "rtc_base/strings/string_builder.h"

using webrtc::RTCError;
using webrtc::RTCErrorOr;
using webrtc::RTCErrorType;

namespace libice {
namespace {
	static const unsigned char _kPropertyBits[256] = {
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0x00
	0x40, 0x68, 0x48, 0x48, 0x48, 0x48, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // 0x10
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x28, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,  // 0x20
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84,  // 0x30
	0x84, 0x84, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x05,  // 0x40
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,  // 0x50
	0x05, 0x05, 0x05, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x85, 0x85, 0x85, 0x85, 0x85, 0x85, 0x05,  // 0x60
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,  // 0x70
	0x05, 0x05, 0x05, 0x10, 0x10, 0x10, 0x10, 0x40,
	};

	inline bool ascii_isalnum(unsigned char c) {
		return (_kPropertyBits[c] & 0x04) != 0;
	}
bool IsIceChar(char c) {
  // Note: '-', '=', '#' and '_' are *not* valid ice-chars but temporarily
  // permitted in order to allow external software to upgrade.
  if (c == '-' || c == '=' || c == '#' || c == '_') {
    RTC_LOG(LS_WARNING)
        << "'-', '=', '#' and '-' are not valid ice-char and thus not "
        << "permitted in ufrag or pwd. This is a protocol violation that "
        << "is permitted to allow upgrading but will be rejected in "
        << "the future. See https://crbug.com/1053756";
    return true;
  }
 // return false;
#if OPEN_DEPS
  return ascii_isalnum(c) || c == '+' || c == '/';
#else 
  return absl::ascii_isalnum(c) || c == '+' || c == '/';

#endif 
}

RTCError ValidateIceUfrag(absl::string_view raw_ufrag) {
  if (!(ICE_UFRAG_MIN_LENGTH <= raw_ufrag.size() &&
        raw_ufrag.size() <= ICE_UFRAG_MAX_LENGTH)) {
    rtc::StringBuilder sb;
    sb << "ICE ufrag must be between " << ICE_UFRAG_MIN_LENGTH << " and "
       << ICE_UFRAG_MAX_LENGTH << " characters long.";
    return RTCError(RTCErrorType::SYNTAX_ERROR, sb.Release());
  }

  if (!absl::c_all_of(raw_ufrag, IsIceChar)) {
    return RTCError(
        RTCErrorType::SYNTAX_ERROR,
        "ICE ufrag must contain only alphanumeric characters, '+', and '/'.");
  }

  return RTCError::OK();
}

RTCError ValidateIcePwd(absl::string_view raw_pwd) {
  if (!(ICE_PWD_MIN_LENGTH <= raw_pwd.size() &&
        raw_pwd.size() <= ICE_PWD_MAX_LENGTH)) {
    rtc::StringBuilder sb;
    sb << "ICE pwd must be between " << ICE_PWD_MIN_LENGTH << " and "
       << ICE_PWD_MAX_LENGTH << " characters long.";
    return RTCError(RTCErrorType::SYNTAX_ERROR, sb.Release());
  }

  if (!absl::c_all_of(raw_pwd, IsIceChar)) {
    return RTCError(
        RTCErrorType::SYNTAX_ERROR,
        "ICE pwd must contain only alphanumeric characters, '+', and '/'.");
  }

  return RTCError::OK();
}

}  // namespace

RTCErrorOr<IceParameters> IceParameters::Parse(absl::string_view raw_ufrag,
                                               absl::string_view raw_pwd) {
  IceParameters parameters(std::string(raw_ufrag), std::string(raw_pwd),
                           /* renomination= */ false);
  auto result = parameters.Validate();
  if (!result.ok()) {
    return result;
  }
  return parameters;
}

RTCError IceParameters::Validate() const {
  // For legacy protocols.
  // TODO(zhihuang): Remove this once the legacy protocol is no longer
  // supported.
  if (ufrag.empty() && pwd.empty()) {
    return RTCError::OK();
  }

  auto ufrag_result = ValidateIceUfrag(ufrag);
  if (!ufrag_result.ok()) {
    return ufrag_result;
  }

  auto pwd_result = ValidateIcePwd(pwd);
  if (!pwd_result.ok()) {
    return pwd_result;
  }

  return RTCError::OK();
}

bool StringToConnectionRole(const std::string& role_str, ConnectionRole* role) {
  const char* const roles[] = {
      CONNECTIONROLE_ACTIVE_STR, CONNECTIONROLE_PASSIVE_STR,
      CONNECTIONROLE_ACTPASS_STR, CONNECTIONROLE_HOLDCONN_STR};

  for (size_t i = 0; i < arraysize(roles); ++i) {
    if (absl::EqualsIgnoreCase(roles[i], role_str)) {
      *role = static_cast<ConnectionRole>(CONNECTIONROLE_ACTIVE + i);
      return true;
    }
  }
  return false;
}

bool ConnectionRoleToString(const ConnectionRole& role, std::string* role_str) {
  switch (role) {
    case libice::CONNECTIONROLE_ACTIVE:
      *role_str = libice::CONNECTIONROLE_ACTIVE_STR;
      break;
    case libice::CONNECTIONROLE_ACTPASS:
      *role_str = libice::CONNECTIONROLE_ACTPASS_STR;
      break;
    case libice::CONNECTIONROLE_PASSIVE:
      *role_str = libice::CONNECTIONROLE_PASSIVE_STR;
      break;
    case libice::CONNECTIONROLE_HOLDCONN:
      *role_str = libice::CONNECTIONROLE_HOLDCONN_STR;
      break;
    default:
		//*role_str = libice::CONNECTIONROLE_ACTIVE_STR;
      return false;
  }
  return true;
}

TransportDescription::TransportDescription()
    : ice_mode(ICEMODE_FULL), connection_role(CONNECTIONROLE_NONE) {}

TransportDescription::TransportDescription(
    const std::vector<std::string>& transport_options,
    const std::string& ice_ufrag,
    const std::string& ice_pwd,
    IceMode ice_mode,
    ConnectionRole role,
    const rtc::SSLFingerprint* identity_fingerprint)
    : transport_options(transport_options),
      ice_ufrag(ice_ufrag),
      ice_pwd(ice_pwd),
      ice_mode(ice_mode),
      connection_role(role),
      identity_fingerprint(CopyFingerprint(identity_fingerprint)) {}

TransportDescription::TransportDescription(const std::string& ice_ufrag,
                                           const std::string& ice_pwd)
    : ice_ufrag(ice_ufrag),
      ice_pwd(ice_pwd),
      ice_mode(ICEMODE_FULL),
      connection_role(CONNECTIONROLE_NONE) {}

TransportDescription::TransportDescription(const TransportDescription& from)
    : transport_options(from.transport_options),
      ice_ufrag(from.ice_ufrag),
      ice_pwd(from.ice_pwd),
      ice_mode(from.ice_mode),
      connection_role(from.connection_role),
      identity_fingerprint(CopyFingerprint(from.identity_fingerprint.get())) {}

TransportDescription::~TransportDescription() = default;

TransportDescription& TransportDescription::operator=(
    const TransportDescription& from) {
  // Self-assignment
  if (this == &from)
    return *this;

  transport_options = from.transport_options;
  ice_ufrag = from.ice_ufrag;
  ice_pwd = from.ice_pwd;
  ice_mode = from.ice_mode;
  connection_role = from.connection_role;

  identity_fingerprint.reset(CopyFingerprint(from.identity_fingerprint.get()));
  return *this;
}

}  // namespace cricket
