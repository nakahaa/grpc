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
#include "envoy/config/core/v3/config_source.upb.h"
#include "envoy/type/v3/range.upb.h"
#include "google/protobuf/any.upb.h"
#include "google/protobuf/duration.upb.h"
#include "google/protobuf/wrappers.upb.h"
#include "envoy/annotations/deprecation.upb.h"
#include "udpa/annotations/status.upb.h"
#include "udpa/annotations/versioning.upb.h"
#include "validate/validate.upb.h"

#include "upb/port_def.inc"

static const upb_MiniTable_Sub envoy_config_listener_v3_Filter_submsgs[2] = {
  {.submsg = &envoy_config_core_v3_ExtensionConfigSource_msginit},
  {.submsg = &google_protobuf_Any_msginit},
};

static const upb_MiniTable_Field envoy_config_listener_v3_Filter__fields[3] = {
  {1, UPB_SIZE(0, 0), 0, 0, 9, kUpb_FieldMode_Scalar | (upb_FieldRep_StringView << upb_FieldRep_Shift)},
  {4, UPB_SIZE(8, 16), UPB_SIZE(-13, -25), 1, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {5, UPB_SIZE(8, 16), UPB_SIZE(-13, -25), 0, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
};

const upb_MiniTable envoy_config_listener_v3_Filter_msginit = {
  &envoy_config_listener_v3_Filter_submsgs[0],
  &envoy_config_listener_v3_Filter__fields[0],
  UPB_SIZE(16, 32), 3, upb_ExtMode_NonExtendable, 1, 255, 0,
};

static const upb_MiniTable_Sub envoy_config_listener_v3_FilterChainMatch_submsgs[2] = {
  {.submsg = &envoy_config_core_v3_CidrRange_msginit},
  {.submsg = &google_protobuf_UInt32Value_msginit},
};

static const upb_MiniTable_Field envoy_config_listener_v3_FilterChainMatch__fields[11] = {
  {3, UPB_SIZE(32, 56), 0, 0, 11, kUpb_FieldMode_Array | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {4, UPB_SIZE(8, 8), 0, 0, 9, kUpb_FieldMode_Scalar | (upb_FieldRep_StringView << upb_FieldRep_Shift)},
  {5, UPB_SIZE(24, 40), 1, 1, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {6, UPB_SIZE(36, 64), 0, 0, 11, kUpb_FieldMode_Array | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {7, UPB_SIZE(40, 72), 0, 0, 13, kUpb_FieldMode_Array | upb_LabelFlags_IsPacked | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {8, UPB_SIZE(28, 48), 2, 1, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {9, UPB_SIZE(16, 24), 0, 0, 9, kUpb_FieldMode_Scalar | (upb_FieldRep_StringView << upb_FieldRep_Shift)},
  {10, UPB_SIZE(44, 80), 0, 0, 9, kUpb_FieldMode_Array | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {11, UPB_SIZE(48, 88), 0, 0, 9, kUpb_FieldMode_Array | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {12, UPB_SIZE(4, 4), 0, 0, 5, kUpb_FieldMode_Scalar | (upb_FieldRep_4Byte << upb_FieldRep_Shift)},
  {13, UPB_SIZE(52, 96), 0, 0, 11, kUpb_FieldMode_Array | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
};

const upb_MiniTable envoy_config_listener_v3_FilterChainMatch_msginit = {
  &envoy_config_listener_v3_FilterChainMatch_submsgs[0],
  &envoy_config_listener_v3_FilterChainMatch__fields[0],
  UPB_SIZE(56, 112), 11, upb_ExtMode_NonExtendable, 0, 255, 0,
};

static const upb_MiniTable_Sub envoy_config_listener_v3_FilterChain_submsgs[7] = {
  {.submsg = &envoy_config_core_v3_Metadata_msginit},
  {.submsg = &envoy_config_core_v3_TransportSocket_msginit},
  {.submsg = &envoy_config_listener_v3_Filter_msginit},
  {.submsg = &envoy_config_listener_v3_FilterChain_OnDemandConfiguration_msginit},
  {.submsg = &envoy_config_listener_v3_FilterChainMatch_msginit},
  {.submsg = &google_protobuf_BoolValue_msginit},
  {.submsg = &google_protobuf_Duration_msginit},
};

static const upb_MiniTable_Field envoy_config_listener_v3_FilterChain__fields[8] = {
  {1, UPB_SIZE(12, 24), 1, 4, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {3, UPB_SIZE(36, 72), 0, 2, 11, kUpb_FieldMode_Array | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {4, UPB_SIZE(16, 32), 2, 5, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {5, UPB_SIZE(20, 40), 3, 0, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {6, UPB_SIZE(24, 48), 4, 1, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {7, UPB_SIZE(4, 8), 0, 0, 9, kUpb_FieldMode_Scalar | (upb_FieldRep_StringView << upb_FieldRep_Shift)},
  {8, UPB_SIZE(28, 56), 5, 3, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {9, UPB_SIZE(32, 64), 6, 6, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
};

const upb_MiniTable envoy_config_listener_v3_FilterChain_msginit = {
  &envoy_config_listener_v3_FilterChain_submsgs[0],
  &envoy_config_listener_v3_FilterChain__fields[0],
  UPB_SIZE(40, 80), 8, upb_ExtMode_NonExtendable, 1, 255, 0,
};

static const upb_MiniTable_Sub envoy_config_listener_v3_FilterChain_OnDemandConfiguration_submsgs[1] = {
  {.submsg = &google_protobuf_Duration_msginit},
};

static const upb_MiniTable_Field envoy_config_listener_v3_FilterChain_OnDemandConfiguration__fields[1] = {
  {1, UPB_SIZE(4, 8), 1, 0, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
};

const upb_MiniTable envoy_config_listener_v3_FilterChain_OnDemandConfiguration_msginit = {
  &envoy_config_listener_v3_FilterChain_OnDemandConfiguration_submsgs[0],
  &envoy_config_listener_v3_FilterChain_OnDemandConfiguration__fields[0],
  UPB_SIZE(8, 16), 1, upb_ExtMode_NonExtendable, 1, 255, 0,
};

static const upb_MiniTable_Sub envoy_config_listener_v3_ListenerFilterChainMatchPredicate_submsgs[3] = {
  {.submsg = &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit},
  {.submsg = &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_msginit},
  {.submsg = &envoy_type_v3_Int32Range_msginit},
};

static const upb_MiniTable_Field envoy_config_listener_v3_ListenerFilterChainMatchPredicate__fields[5] = {
  {1, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 1, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {2, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 1, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {3, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 0, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {4, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 0, 8, kUpb_FieldMode_Scalar | (upb_FieldRep_1Byte << upb_FieldRep_Shift)},
  {5, UPB_SIZE(0, 0), UPB_SIZE(-5, -9), 2, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
};

const upb_MiniTable envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit = {
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_submsgs[0],
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate__fields[0],
  UPB_SIZE(8, 16), 5, upb_ExtMode_NonExtendable, 5, 255, 0,
};

static const upb_MiniTable_Sub envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_submsgs[1] = {
  {.submsg = &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit},
};

static const upb_MiniTable_Field envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet__fields[1] = {
  {1, UPB_SIZE(0, 0), 0, 0, 11, kUpb_FieldMode_Array | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
};

const upb_MiniTable envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_msginit = {
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_submsgs[0],
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet__fields[0],
  UPB_SIZE(8, 8), 1, upb_ExtMode_NonExtendable, 1, 255, 0,
};

static const upb_MiniTable_Sub envoy_config_listener_v3_ListenerFilter_submsgs[3] = {
  {.submsg = &envoy_config_core_v3_ExtensionConfigSource_msginit},
  {.submsg = &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit},
  {.submsg = &google_protobuf_Any_msginit},
};

static const upb_MiniTable_Field envoy_config_listener_v3_ListenerFilter__fields[4] = {
  {1, UPB_SIZE(4, 8), 0, 0, 9, kUpb_FieldMode_Scalar | (upb_FieldRep_StringView << upb_FieldRep_Shift)},
  {3, UPB_SIZE(16, 32), UPB_SIZE(-21, -41), 2, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {4, UPB_SIZE(12, 24), 1, 1, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
  {5, UPB_SIZE(16, 32), UPB_SIZE(-21, -41), 0, 11, kUpb_FieldMode_Scalar | (upb_FieldRep_Pointer << upb_FieldRep_Shift)},
};

const upb_MiniTable envoy_config_listener_v3_ListenerFilter_msginit = {
  &envoy_config_listener_v3_ListenerFilter_submsgs[0],
  &envoy_config_listener_v3_ListenerFilter__fields[0],
  UPB_SIZE(24, 48), 4, upb_ExtMode_NonExtendable, 1, 255, 0,
};

static const upb_MiniTable *messages_layout[7] = {
  &envoy_config_listener_v3_Filter_msginit,
  &envoy_config_listener_v3_FilterChainMatch_msginit,
  &envoy_config_listener_v3_FilterChain_msginit,
  &envoy_config_listener_v3_FilterChain_OnDemandConfiguration_msginit,
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_msginit,
  &envoy_config_listener_v3_ListenerFilterChainMatchPredicate_MatchSet_msginit,
  &envoy_config_listener_v3_ListenerFilter_msginit,
};

const upb_MiniTable_File envoy_config_listener_v3_listener_components_proto_upb_file_layout = {
  messages_layout,
  NULL,
  NULL,
  7,
  0,
  0,
};

#include "upb/port_undef.inc"

