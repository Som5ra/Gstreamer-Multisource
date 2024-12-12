#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

cv::Mat latest_frame;
std::mutex frame_mutex;
std::condition_variable frame_cond;
GstElement *appsrc;
GstClockTime timestamp = 0;
GstClock* _gst_clock;

int TARGET_FPS = 30;

void process_frames() {
    while (true) {
        std::unique_lock<std::mutex> lock(frame_mutex);
        frame_cond.wait(lock);

        // Process the latest frame
        cv::Mat frame = latest_frame.clone();
        lock.unlock();

        // Draw random rectangles on the frame
        int x = std::rand() % frame.cols;
        int y = std::rand() % frame.rows;
        int width = std::rand() % (frame.cols - x);
        int height = std::rand() % (frame.rows - y);
        cv::rectangle(frame, cv::Rect(x, y, width, height), cv::Scalar(0, 255, 0), 2);

        // Convert the frame to a GStreamer buffer
        GstBuffer *buffer = gst_buffer_new_allocate(NULL, frame.total() * frame.elemSize(), NULL);
        gst_buffer_fill(buffer, 0, frame.data, frame.total() * frame.elemSize());

        // Set the timestamp on the buffer
        GST_BUFFER_PTS(buffer) = timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, TARGET_FPS); // Assuming 30 fps
        timestamp += GST_BUFFER_DURATION(buffer);

        SPDLOG_INFO("Pushing buffer with timestamp: {}", timestamp);

        // Push the buffer to appsrc
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
        if (ret != GST_FLOW_OK) {
            SPDLOG_ERROR("Failed to push buffer to appsrc: {}", ret);
        } else {
            SPDLOG_INFO("Successfully pushed buffer to appsrc");
        }
    }
}

static GstFlowReturn new_sample(GstAppSink *sink, gpointer user_data) {
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;
    cv::Mat frame;

    // Pull the sample from appsink
    sample = gst_app_sink_pull_sample(sink);
    if (!sample) {
        SPDLOG_ERROR("Failed to pull sample from appsink");
        return GST_FLOW_ERROR;
    }

    // Get the buffer from the sample
    buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        SPDLOG_ERROR("Failed to get buffer from sample");
        return GST_FLOW_ERROR;
    }

    // Map the buffer memory
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        SPDLOG_ERROR("Failed to map buffer memory");
        return GST_FLOW_ERROR;
    }

    frame = cv::Mat(cv::Size(640, 480), CV_8UC3, (char*)map.data, cv::Mat::AUTO_STEP);

    // Unmap and unref the buffer and sample
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    // Update the latest frame
    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        latest_frame = frame.clone();
    }
    frame_cond.notify_one();

    return GST_FLOW_OK;
}

int main(int argc, char *argv[]) {
    GstElement *pipeline, *appsink, *display_pipeline;
    GstBus *bus;
    GstMessage *msg;

    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    SPDLOG_INFO("[gst_init] Finished");

    /* Build the pipeline */
    pipeline = gst_parse_launch(
        "v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,format=BGR,width=640,height=480,framerate=30/1 ! appsink name=mysink",
        NULL);

    /* Get the appsink element */
    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    if (!appsink) {
        SPDLOG_ERROR("Failed to get appsink element");
        return -1;
    }

    /* Configure appsink */
    gst_app_sink_set_emit_signals((GstAppSink*)appsink, true);
    gst_app_sink_set_drop((GstAppSink*)appsink, true);
    gst_app_sink_set_max_buffers((GstAppSink*)appsink, 1);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(new_sample), NULL);

    /* Create a new pipeline for displaying the processed frames */
    display_pipeline = gst_parse_launch(
        "appsrc name=myappsrc ! videoconvert ! fpsdisplaysink video-sink=glimagesink",
        NULL);

    /* Get the appsrc element */
    appsrc = gst_bin_get_by_name(GST_BIN(display_pipeline), "myappsrc");
    if (!appsrc) {
        SPDLOG_ERROR("Failed to get appsrc element");
        return -1;
    }

    /* Configure appsrc */
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "BGR",
                                        "width", G_TYPE_INT, 640,
                                        "height", G_TYPE_INT, 480,
                                        "framerate", GST_TYPE_FRACTION, TARGET_FPS, 1,
                                        NULL);
    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_caps_unref(caps);

    /* Use the same clock for both pipelines */
    _gst_clock = gst_system_clock_obtain();
    gst_pipeline_use_clock(GST_PIPELINE(pipeline), _gst_clock);
    gst_pipeline_use_clock(GST_PIPELINE(display_pipeline), _gst_clock);
    gst_object_unref(_gst_clock);

    /* Start the frame processing thread */
    std::thread processing_thread(process_frames);

    /* Start playing both pipelines */
    GstStateChangeReturn ret1 = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret1 == GST_STATE_CHANGE_FAILURE) {
        SPDLOG_ERROR("Failed to set pipeline to PLAYING state");
        return -1;
    } else {
        SPDLOG_INFO("Pipeline set to PLAYING state");
    }

    GstStateChangeReturn ret2 = gst_element_set_state(display_pipeline, GST_STATE_PLAYING);
    if (ret2 == GST_STATE_CHANGE_FAILURE) {
        SPDLOG_ERROR("Failed to set display pipeline to PLAYING state");
        return -1;
    } else {
        SPDLOG_INFO("Display pipeline set to PLAYING state");
    }

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != NULL) {
        SPDLOG_ERROR("Error or EOS received: {}", GST_MESSAGE_TYPE_NAME(msg));
        gst_message_unref(msg);
    }

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_set_state(display_pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(display_pipeline);

    /* Join the processing thread */
    processing_thread.join();

    return 0;
}