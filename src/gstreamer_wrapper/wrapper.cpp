#include <gst/gst.h>
#include <glib.h>
#include <wrapper.hpp>
#include "spdlog/spdlog.h"




GstPadProbeReturn cb_have_data(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{

    VideoStreamElement* video_stream_element = (VideoStreamElement*) user_data;

    // // if (probe_user_data_ptr->video_stream_element->index == 3)
    // //     SPDLOG_INFO("cb_have_data");
    // return GST_PAD_PROBE_DROP;

    GstMapInfo map;
    GstBuffer *buffer;

    buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_make_writable(buffer);


    if (buffer == NULL)
    {
        SPDLOG_WARN("[VideoStreamElement {}] Cannot open buffer for video stream element", video_stream_element->config.name);
        return GST_PAD_PROBE_DROP;
        // spdlog::warn("Cannot open buffer\n");
        // return GST_PAD_PROBE_OK;
    }
    

    GstCaps *caps = gst_pad_get_current_caps(pad);

    auto structure = gst_caps_get_structure(caps, 0);
    int width = -1;
    int height = -1;
    int framerate_numerator = -1;
    int framerate_denominator = -1;

    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);
    gst_structure_get_fraction(structure, "framerate", &framerate_numerator, &framerate_denominator);

    video_stream_element->buffer_mutex->lock();
    // SPDLOG_INFO("[{}] Accuired lock", probe_user_data_ptr->video_stream_element->index);
    
    if (video_stream_element->last_buffer != nullptr )
    {
        gst_buffer_unref(video_stream_element->last_buffer);
    }
    gst_buffer_ref(buffer);
    video_stream_element->last_buffer = buffer;
    // video_stream_element->last_buffer_received_at = get_timestamp();
    video_stream_element->last_buffer_width = width;
    video_stream_element->last_buffer_height = height;
    video_stream_element->last_buffer_framerate_numerator = framerate_numerator;
    video_stream_element->last_buffer_framerate_denominator = framerate_denominator;

    SPDLOG_INFO("({}x{})", width, height);

    // GstFlowReturn ret;
    // g_signal_emit_by_name (probe_user_data_ptr->appsrc, "push-buffer", buffer, &ret);
    // SPDLOG_INFO("Buffer pushed: {} ({}x{}) (src: {})", ret, width, height, probe_user_data_ptr->video_stream_element->index);


    video_stream_element->buffer_mutex->unlock();

    
    // /* Mapping a buffer can fail (non-writable) */
    // guint8 *ptr, t;
    // if (gst_buffer_map(buffer, &map, GST_MAP_WRITE))
    // {
    //     SPDLOG_INFO("Got frame. {}x{}", width, height);
    //     ptr = (guint8 *)map.data;
    //     gst_buffer_unmap(buffer, &map);
    // }
    // else
    // {
    //     spdlog::warn("Buffer not writable\n");
    // }

    // GST_PAD_PROBE_INFO_DATA(info) = buffer;


    // ProbeCallbackUserData* probe_user_data_ptr = (ProbeCallbackUserData*) user_data;
    // GstFlowReturn ret;
    // g_object_set (probe_user_data_ptr->appsrc, "caps", caps, "format", GST_FORMAT_TIME, NULL);

    // g_signal_emit_by_name (probe_user_data_ptr->appsrc, "push-buffer", buffer, &ret);
    // SPDLOG_INFO("Buffer pushed: {}", ret);

    GST_PAD_PROBE_INFO_DATA(info) = buffer;
    return GST_PAD_PROBE_DROP;
    return GST_PAD_PROBE_OK;
}


GstWrapper::GstWrapper(WrapperConfig config)
{
    this->_init();


    SPDLOG_INFO("Creating main_stream_elements...");
    this->main_stream_elements = this->_create_appsrc_tee_elements(config.main_stream_config);
    assert(this->main_stream_elements != nullptr);
    SPDLOG_INFO("Created main_stream_elements (ptr: {})", fmt::ptr(this->main_stream_elements));


    SPDLOG_INFO("Creating gst clock...");
    this->gst_clock = gst_system_clock_obtain();
    assert(this->gst_clock != nullptr);
    SPDLOG_INFO("Created gst clock (ptr: {})", fmt::ptr(this->gst_clock));



    g_object_set (G_OBJECT (this->main_stream_elements->appsrc), "caps",
        gst_caps_new_simple ("video/x-raw",
            "format", G_TYPE_STRING, "BGR",
            "width", G_TYPE_INT, 1920,
            "height", G_TYPE_INT, 1080,
            "framerate", GST_TYPE_FRACTION, 30, 1,
        NULL),
    NULL);
    this->main_stream_elements->cap_set = true;

    
    for(auto video_stream_config : config.video_streams_config)
    {
        this->multi_streams_elements.push_back(this->_create_branch_stream_elements(video_stream_config));
    }

    SPDLOG_INFO("Setting playing state...");
    gst_element_set_state(this->main_pipeline, GST_STATE_PLAYING);

    this->bus = gst_element_get_bus(this->main_pipeline);
    this->msg = gst_bus_timed_pop_filtered(this->bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    if (this->msg != NULL) {
        SPDLOG_ERROR("Error or EOS received: {}", GST_MESSAGE_TYPE_NAME(this->msg));
        gst_message_unref(this->msg);
    }
    gst_object_unref(this->bus);
    gst_element_set_state(this->main_pipeline, GST_STATE_NULL);
}

GstWrapper::~GstWrapper()
{
    // free(this->pipeline_head);

    // this->_configure();
}

// define public method: start()
void GstWrapper::start()
{
    return ;
}




// define private method: _init()
void GstWrapper::_init()
{
    spdlog::info("Initialize Gstreamer Pipeline");
    gst_init (NULL, NULL);

    spdlog::info("Configure main stream pipeline");
    this->main_pipeline = gst_pipeline_new("main_pipeline");
    if (!this->main_pipeline){
        spdlog::info(fmt::format("main_pipeline: {}", fmt::ptr(this->main_pipeline)));
    }

}




// define private method: _create_appsrc_tee_elements()
MultiSinksElement* GstWrapper::_create_appsrc_tee_elements(MultiSinksConfig config)
{

    auto name = config.name;
    auto gst_sink_pipeline_strs = config.gst_sink_pipeline_strs;


    auto ret = new MultiSinksElement {};
    ret->config = config;

    if (config.gst_sink_pipeline_strs.size() == 0)
    {
        SPDLOG_INFO("Created empty MultiSinksElement [{}]", name);
        return ret;
    }


    // create appsrc -> tee -> others
    ret->appsrc = gst_element_factory_make("appsrc", NULL);
    ret->tee = gst_element_factory_make("tee", NULL);
    assert(ret->appsrc != nullptr);
    assert(ret->tee != nullptr);
    gst_bin_add_many(GST_BIN(this->main_pipeline), ret->appsrc, ret->tee, NULL);

    // create appsrc -> tee -> others
    ret->appsrc = gst_element_factory_make("appsrc", NULL);
    ret->tee = gst_element_factory_make("tee", NULL);
    assert(ret->appsrc != nullptr);
    assert(ret->tee != nullptr);
    gst_bin_add_many(GST_BIN(this->main_pipeline), ret->appsrc, ret->tee, NULL);
    auto link_status = gst_element_link(ret->appsrc, ret->tee);
    assert(link_status);

    // add caps to appsrc
    g_object_set (G_OBJECT (ret->appsrc), "caps",
            gst_caps_new_simple ("video/x-raw",
                "format", G_TYPE_STRING, "BGR",
                "width", G_TYPE_INT, 1920,
                "height", G_TYPE_INT, 1080,
                "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL),
        NULL);



    for (int i = 0; i < gst_sink_pipeline_strs.size(); i ++)
    {
        auto pipeline_str = gst_sink_pipeline_strs[i];
        SPDLOG_INFO("[MultiSinksElement \"{}\"] Creating sink element: [{}]", name, pipeline_str);
        GstElement* pipeline = gst_parse_bin_from_description(pipeline_str.c_str(), false, nullptr);
        assert(pipeline != nullptr);
        
        // create virtual pad
        GstPad* unlinked_pad = gst_bin_find_unlinked_pad((GstBin*) pipeline, GST_PAD_SINK);
        gst_element_add_pad(pipeline, gst_ghost_pad_new ("sink", unlinked_pad));
        gst_object_unref(unlinked_pad);
        gst_bin_add(GST_BIN(this->main_pipeline), pipeline);
        

        // link tee -> sink element
        link_status = gst_element_link(ret->tee, pipeline);
        assert(link_status);
        
        ret->sink_elements.push_back(pipeline);
        SPDLOG_INFO("[MultiSinksElement \"{}\"] Created sink element #{}: {}", name, i, fmt::ptr(pipeline));

    }

    g_object_set(G_OBJECT (ret->appsrc), 
        "stream-type", 0,
        "is-live", TRUE,
        "block", FALSE,
        "format", GST_FORMAT_TIME,
        // "leaky-type", 2,
        // "max-buffers", 1,
        NULL
    );

    // // set to some initial values to prevent memory leak
    // g_object_set (G_OBJECT (this->pipeline_head->appsrc), "caps",
    //     gst_caps_new_simple ("video/x-raw",
    //         "format", G_TYPE_STRING, "BGR",
    //         "width", G_TYPE_INT, 1920,
    //         "height", G_TYPE_INT, 1080,
    //         "framerate", GST_TYPE_FRACTION, 1, 1,
    //     NULL),
    // NULL);
    return ret;
}

VideoStreamElement* GstWrapper::_create_branch_stream_elements(VideoStreamConfig config)
{
    SPDLOG_INFO("Creating branch stream...");

    auto name = config.name;
    auto* ret = new VideoStreamElement();
    auto gst_pipeline_str = config.gst_pipeline_str;
    if (gst_pipeline_str.size() == 0){
        SPDLOG_INFO("Created an empty branch stream");
        return ret;
    }

    SPDLOG_INFO("Creating branch stream [{}]", gst_pipeline_str);
    GstElement* pipeline = gst_parse_bin_from_description(gst_pipeline_str.c_str(), false, nullptr);
    GstPad* unlinked_pad = gst_bin_find_unlinked_pad((GstBin*) pipeline, GST_PAD_SRC);
    gst_element_add_pad(pipeline, gst_ghost_pad_new ("src", unlinked_pad));
    gst_object_unref(unlinked_pad);

    gst_bin_add_many(GST_BIN(this->main_pipeline), pipeline, NULL);
    ret->element = pipeline;

    // push to main stream
    ret->main_stream_source_sinks = this->main_stream_elements;
    // ret->stream_sinks = this->_create_multi_sink_element(args.multi_sinks_config);
    // SPDLOG_INFO("[VideoStreamElement {}] Created stream sinks (ptr: {})...", name, fmt::ptr(ret->stream_sinks));

    // probe add
    ret->self_src_pad = unlinked_pad;
    ret->probe_id = gst_pad_add_probe(ret->self_src_pad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_have_data, ret, NULL);
    SPDLOG_INFO("[VideoStreamElement {}] Added probe to {} (probe: {})...", name, GST_PAD_NAME(ret->self_src_pad),ret->probe_id);


    return ret;
}