#include <gst/gst.h>

int main ()
{
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    /* Initialize GStreamer */
    gst_init (NULL, NULL);

    /* Build the pipeline */
    pipeline =
        gst_parse_launch
        // ("filesrc location=1.mp4 ! decodebin ! videoconvert ! capsfilter caps=\"video/x-raw, format=BGR\" ! videoconvert ! identity sync=true",
        ("tee ! filesrc location=1.mp4 ! decodebin ! videoconvert ! capsfilter caps=\"video/x-raw, format=BGR\" ! videoconvert ! fpsdisplaysink sync=true",
        NULL);

    /* Start playing */
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    
    /* Wait until error or EOS */
    bus = gst_element_get_bus (pipeline);
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR);

    /* Free resources */
    if (msg != NULL)
    gst_message_unref (msg);
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    return 0;
}