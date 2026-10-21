#include "HardwareDecodePolicy.h"
#include "Logger.h"

#ifdef USB_REQUIRE_HARDWARE_H264
#include <gst/gst.h>
#include <cstring>

namespace {
/** @brief Enforce the video allowlist, including factories registered by late plugin loading. */
void enforceFeatureRank(GstRegistry*, GstPluginFeature* feature, gpointer) {
    if (GST_IS_ELEMENT_FACTORY(feature) == FALSE) {
        return;
    }
    auto* factory = GST_ELEMENT_FACTORY(feature);
    const bool decoder = gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER) != FALSE;
    const bool video = gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO) != FALSE;
    const bool image = gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_MEDIA_IMAGE) != FALSE;
    if (decoder == false || (video == false && image == false)) {
        return;
    }
    const char* name = gst_plugin_feature_get_name(feature);
    const bool allowed = name != nullptr && std::strcmp(name, "v4l2h264dec") == 0;
    gst_plugin_feature_set_rank(feature, allowed ? GST_RANK_PRIMARY + 44 : GST_RANK_NONE);
}
}
#endif

void HardwareDecodePolicy::apply() {
#ifdef USB_REQUIRE_HARDWARE_H264
    gst_init(nullptr, nullptr);
    GstRegistry* registry = gst_registry_get();
    // Future factory registrations must not silently restore software fallback ranks.
    g_signal_connect(registry, "feature-added", G_CALLBACK(enforceFeatureRank), nullptr);
    GList* features = gst_registry_get_feature_list(registry, GST_TYPE_ELEMENT_FACTORY);
    for (GList* entry = features; entry != nullptr; entry = entry->next) {
        enforceFeatureRank(registry, GST_PLUGIN_FEATURE(entry->data), nullptr);
    }
    gst_plugin_feature_list_free(features);
    LOG_I("Video decode policy: v4l2h264dec only; software video fallback disabled; audio unchanged");
#endif
}
