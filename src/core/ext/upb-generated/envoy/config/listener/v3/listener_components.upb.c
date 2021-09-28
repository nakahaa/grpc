/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     envoy/config/listener/v3/listener_components.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#include <stddef.h>
#include "upb/msg_internal.h"
#include "envoy/config/listener/v3/listener_components.upb.h"
#include "envoy/config/core/v3/address.upb.h"
#include "envoy/config/core/v3/base.upb.h"
#include "envoy/config/core/v3/extension.upb.h"
#include "envoy/type/v3/range.upb.h"
#include "google/protobuf/any.upb.h"
#include "google/protobuf/duration.upb.h"
#include "google/protobuf/wrappers.upb.h"
#include "envoy/annotations/deprecation.upb.h"
#include "udpa/annotations/status.upb.h"
#include "udpa/annotations/versioning.upb.h"
#include "validate/validate.upb.h"

#include "upb/port_def.inc"

static const upb_msglayout *const envoy_config_listener_v3_Filter_submsgs[2] = {
  &envoy_config_core_v3_ExtensionConfigSource_msginit,
  &google_protobuf_Any_msginit,
};

static const upb_msglayout_field envoy_config_listener_v3_Filter__fields[3] = {
  {1, UPB_SIZE(0, 0), 0, 0, 9, _UPB_MODE_SCALAR},
  {4, UPB_SIZE(8, 16), UPB_SIZE(-13, -25), 1, 11, _UPB_MODE_SCALAR},
  {5, UPB_SIZE(8, 16), UPB_SIZE(-13, -25), 0, 11, _UPB_MODE_SCALAR},
};

const upb_msglayout envoy_config_listener_v3_Filter_msginit = {
  &envoy_config_listener_v3_Filter_submsgs[0],
  &envoy_config_listener_v3_Filter__fields[0],
  UPB_SIZE(16, 32), 3, false, 1, 255,
};

static const upb_msglayout *const envoy_config_listener_v3_FilterChainMatch_submsgs[2] = {
  &envoy_config_core_v3_CidrRange_msginit,
  &google_protobuf_UInt32Value_msginit,
};

static const upb_msglayout_field envoy_config_listener_v3_FilterChainMatch__fields[11] = {
  {3, UPB_SIZE(32, 56), 0, 0, 11, _UPB_MODE_ARRAY},
  {4, UPB_SIZE(8, 8), 0, 0, 9, _UPB_MODE_SCALAR},
  {5, UPB_SIZE(24, 40), 1, 1, 11, _UPB_MODE_SCALAR},
  {6, UPB_SIZE(36, 64), 0, 0, 11, _UPB_MODE_ARRAY},
  {7, UPB_SIZE(40, 72), 0, 0, 13, _UPB_MODE_ARRAY | _UPB_MODE_IS_PACKED},
  {8, UPB_SIZE(28, 48), 2, 1, 11, _UPB_MODE_SCALAR},
  {9, UPB_SIZE(16, 24), 0, 0, 9, _UPB_MODE_SCALAR},
  {10, UPB_SIZE(44, 80), 0, 0, 9, _UPB_MODE_ARRAY},
  {11, UPB_SIZE(48, 88), 0, 0, 9, _UPB_MODE_ARRAY},
  {12, UPB_SIZE(4, 4), 0, 0, 14, _UPB_MODE_SCALAR},
  {13, UPB_SIZE(52, 96), 0, 0, 11, _UPB_MODE_ARRAY},
};

const upb_msglayout envoy_config_listener_v3_FilterChainMatch_msginit = {
  &envoy_config_listener_v3_FilterChainMatch_submsgs[0],
  &envoy_config_listener_v3_FilterChainMatch__fields[0],
  UPB_SIZE(56, 112), 11, false, 0, 255,
};

static const upb_msglayout *const envoy_config_listener_v3_FilterChain_submsgs[7] = {
  &envoy_config_core_v3_Metadata_msginit,
  &envoy_config_core_v3_TransportSocket_msginit,
  &envoy_config_listener_v3_Filter_msginit,
  &envoy_config_listener_v3_FilterChain_OnDemandConfiguration_msginit,
  &envoy_config_listener_v3_FilterChainMatch_msginit,
  &google_protobuf_BoolValue_msginit,
  &google_protobuf_Duration_msginit,
};

static const upb_msglayout_field envoy_config_listener_v3_FilterChain__fields[8] = {
  {1, UPB_SIZE(12, 24), 1, 4, 11, _UPB_MODE_SCALAR},
  {3, UPB_SIZE(36, 72), 0, 2, 11, _UPB_MODE_ARRAY},
  {4, UPB_SIZE(16, 32), 2, 5, 11, _UPB_MODE_SCALAR},
  {5, UPB_SIZE(20, 40), 3, 0, 11, _UPB_MODE_SCALAR},
  {6, UPB_SIZE(24, 48), 4, 1, 11, _UPB_MODE_SCALAR},
  {7, UPB_SIZE(4, 8), 0, 0, 9, _UPB_MODE_SCALAR},
  {8, UPB_SIZE(28, 56), 5, 3, 11, _UPB_MODE_SCALAR},
  {9, UPB_SIZE(32, 64), 6, 6, 11, _UPB_MODE_SCALAR},
};

const upb_msglayout envoy_config_listener_v3_FilterChain_msginit = {
  &envoy_config_listener_v3_FilterChain_submsgs[0],
  &envoy_config_listener_v3_FilterChain__fields[0],
  UPB_SIZE(40, 80), 8, false, 1, 255,
};

static const upb_msglayout *const envoy_config_listener_v3_FilterChain_OnDemandConfiguration_submsgs[1] = {
  &google_protobuf_Duration_msginit,
};

static const upb_msglayout_field envoy_config_listener_v3_FilterChain_OnDemandConfiguration__fields[1] = {
  {1, UPB_SIZE(4, 8), 1, 0, 11, _UPB_MODE_SCALAR},
};

const upb_msglayout envoy_config_listener_v3_FilterChain_OnDemandConfiguration_msginit = {
  &envoy_config_listener_v3_FilterChain_OnDemandConfiguration_submsgs[0],
  &envoy_config_listener_v3_FilterChain_OnDemandConfiguration__fields[0],
  UPB_SIZE(8, 16), 1, false, 1, 255,
};

static const upb_msglayout *const envoy_config_listener_v3_ListenerFilterChainMatchPredicate_submsgs[3] = {
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit,
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_msginit,
  &envoy_type_v3_Int32Range_msginit,
};

static const upb_msglayout_field envoy_config_listener_v3_ListenerFilterChainMatchPredicate__fields[5] = {
  {1, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 1, 11, _UPB_MODE_SCALAR},
  {2, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 1, 11, _UPB_MODE_SCALAR},
  {3, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 0, 11, _UPB_MODE_SCALAR},
  {4, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 0, 8, _UPB_MODE_SCALAR},
  {5, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 2, 11, _UPB_MODE_SCALAR},
};

const upb_msglayout envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit = {
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_submsgs[0],
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate__fields[0],
  UPB_SIZE(8, 16), 5, false, 5, 255,
};

static const upb_msglayout *const envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_submsgs[1] = {
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit,
};

static const upb_msglayout_field envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet__fields[1] = {
  {1, UPB_SIZE(0, 0), 0, 0, 11, _UPB_MODE_ARRAY},
};

const upb_msglayout envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_msginit = {
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_submsgs[0],
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet__fields[0],
  UPB_SIZE(8, 8), 1, false, 1, 255,
};

static const upb_msglayout *const envoy_config_listener_v3_ListenerFilter_submsgs[2] = {
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit,
  &google_protobuf_Any_msginit,
};

static const upb_msglayout_field envoy_config_listener_v3_ListenerFilter__fields[3] = {
  {1, UPB_SIZE(4, 8), 0, 0, 9, _UPB_MODE_SCALAR},
  {3, UPB_SIZE(16, 32), UPB_SIZE(-21, -41), 1, 11, _UPB_MODE_SCALAR},
  {4, UPB_SIZE(12, 24), 1, 0, 11, _UPB_MODE_SCALAR},
};

const upb_msglayout envoy_config_listener_v3_ListenerFilter_msginit = {
  &envoy_config_listener_v3_ListenerFilter_submsgs[0],
  &envoy_config_listener_v3_ListenerFilter__fields[0],
  UPB_SIZE(24, 48), 3, false, 1, 255,
};

#include "upb/port_undef.inc"

