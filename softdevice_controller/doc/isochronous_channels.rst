.. _softdevice_controller_iso:

LE Isochronous Channels
#######################

.. contents::
   :local:
   :depth: 2

.. note::
   LE Isochronous Channels and the corresponding documentation are currently :ref:`experimental <nrf:software_maturity>` and subject to changes.

LE Isochronous Channels is a feature defined in the `Bluetooth Core Specification`_.
It allows for unreliable transport of data in one-to-one, one-to-many and many-to-one topologies.

In LE Isochronous Channels, data is transmitted in units called SDUs.
The source of the data provides one SDU every SDU interval and these SDUs have a fixed timeout for their delivery.
SDUs are transmitted at a fixed schedule, which allows to time-synchronize multiple receiving devices with microsecond-scale accuracy.

The LE Isochronous Channels feature is split into a broadcast and a unicast part.

For broadcast mode, different broadcast isochronous streams (BIS) are grouped into broadcast isochronous groups (BIG).
In this mode, data is sent in a unidirectional stream from a source device to an arbitrary number of sink devices.

For unicast mode, different connected isochronous streams (CIS) are grouped into connected isochronous groups (CIG).
In this mode, data is sent on a unidirectional or bidirectional connection.


Tested configurations
*********************

The LE Isochronous Channels feature is highly configurable and it is not feasible to test all possible configurations and topologies.
Testing of the |controller| implementation of LE Isochronous Channels focuses on assumed common use-cases.
Among others, the following configurations are tested:

* Configurations for the audio use-case that are described in the Basic Audio Profile specification that is available for download from the `Bluetooth Specifications and Documents`_ page.
* Parallel use of CIS and BIS (1 CIS, 1 BIS)
* 4 CISes to the same peer (CIS central and CIS peripheral)
* 4 CISes to different peers (CIS central)
* 4 CISes distributed in 4 CIGs (CIS central and CIS peripheral)
* 4 CISes to different peers using the same CIG
* BIS source sending on 4 BIGs with 2 streams each
* BIS source sending on 2 BIS in a single BIG
* BIS sink connected to 2 BIS in a single BIG
* BIS sink connected on 4 BIGs with 2 streams each
* BIS source together with the channel survey feature, see :c:func:`sdc_hci_cmd_vs_qos_channel_survey_enable`

There is no absolute maximum of BISes, CISes and ACLs that can be used concurrently.
Instead, the amount of roles that can be used at the same time is limited by available memory and the on-air timings.
Except where otherwise mentioned, the |controller| supports the whole range of the allowed parameters.
​

.. _iso_providing_data:

Providing data
**************

Data is provided using SDUs using the HCI format for ISO data described in the `Bluetooth Core Specification`_ Version 5.4, Vol 4, Part E, Section 5.4.5.
Data should be provided in intervals of SDU interval.
The SDU interval is configured when the CIS or BIS is created and is a constant during the lifetime of the stream.

There are 3 modes that determine when the SDUs provided to the |controller| are sent:

Timestamps
   In this mode, timestamps are added to the HCI ISO data.
   This is the preferred way of providing data to the |controller| and guarantees the highest degree of control.

   The timestamp must be based on the controller's timings.
   The timestamp of a previous SDU can be retrieved using the HCI LE Read ISO TX sync command.
   The next timestamp should be incremented by a multiple of the SDU interval.
   This means that, in the audio use case where SDUs are provided every SDU interval, the next timestamp should be incremented by one SDU interval.
   SDUs must be provided to the |controller| at least :c:macro:`HCI_ISO_SDU_PROCESSING_TIME_US` before the time indicated in the added timestamp.

   In ISO, the timing information is based on the central's clock.
   This means that for the CIS central and Broadcaster roles, it is sufficient to retrieve the timestamp from the controller only once.
   The CIS peripheral needs to compensate for drift between its clock and the central's clock.
   When running the CIS peripheral, the HCI LE Read ISO TX sync needs to be called periodically, and should be called every time before new data is provided.

   When a timestamp is added to the HCI data, the |controller| ignores the SDU sequence numbers.

Time of arrival
   In the time of arrival mode, the |controller| records the time when the data is being processed inside the controller.
   The controller then tries to send the data in the next available CIS or BIS event where it does not yet have data to send.
   By doing this, the application does not need to keep track of the exact time, which leads to a higher probability that the SDU is sent and not dropped before being sent.
   The latency between when an SDU is provided and when it is sent depend on the configuration.
   There is a minimum of :c:macro:`HCI_ISO_SDU_PROCESSING_TIME_US` latency due to the processing overhead of the |controller| before sending the SDU.
   While the controller tries to minimize latency, there is some inherent jitter due to the asynchronous nature of the HCI interface.

   Expect a larger latency if data is not provided every SDU interval and the stream is configured with retransmissions.
   This is due to the fact that the controller first needs to send empty data packets for the data that was not provided.
   In case data is missing, the controller sends NULL data every ISO event.
   This also ensures that the data provided with the time of arrival mode is retransmitted the configured amount of times.

   Use this mode if the exact time when an SDU is sent does not matter or if SDUs are only produced at a rate much smaller than the SDU interval.
   To use this mode, set the sequence number to 0 and do not add a timestamp to the HCI ISO data.

Sequence numbers
   In the sequence number mode, an SDU should be provided every SDU interval, and the SDU sequence number must be increased by one for each SDU.
   If SDUs are provided more than one SDU interval apart, the SDU sequence number must be increased by a matching amount.
   It is not recommended to use the sequence number mode if SDUs are provided more than one SDU interval apart.

   The controller learns the initial sequence number, so there is no need to align the sequence number each time with the one that is returned when calling the HCI LE Read ISO TX sync command.

   Pay special attention on the CIS peripheral side, because the timings of ISO are based on the central's clock.
   This means that you need to account for drift between the central's and the peripheral's clocks for the the generation of SDUs.
   To do this, use the HCI LE Read ISO TX sync command.
   The command provides a timestamp corresponding to the last possible point in time that the previous SDU could have been provided.
   When combined with the SDU interval, this gives an indication of the last possible time when an SDU can be provided.

   Due to the asynchronous nature of the HCI interface, even small jitter or drift can lead to an SDU being provided too late.
   In that case, the data might be dropped or only transmitted as a retransmission.

   If the provided sequence number does not make sense, the |controller| falls back to the time of arrival mode.

   To use this mode, set the sequence number field and do not add a timestamp to the HCI ISO data.


Synchronize data sent on multiple CISes or BISes
************************************************

The LE Isochronous Channels feature allows SDUs to be sent in a way that multiple receivers can process this data synchronously.
An example use case of this is playback of music that needs to be time-synchronized between a left and a right channel.
The application needs to inform the |controller| which SDUs should be time-synchronized on the receivers.

The recommended way to provide this information is using the timestamps mode.
Using the same timestamp for multiple SDUs guarantees that the SDUs are time-synchronized.
Synchronization can not be reliably achieved using the time of arrival method.
See the :ref:`iso_providing_data` section for more information.

The following logical flow demonstrates how to send time-synchronized SDUs on multiple CISes or BISes:

1. Provide the controller with an SDU for one of the CISes or BISes using the time of arrival method.
#. Issue the HCI LE Read ISO TX sync command on the CIS or BIS where the SDU was sent.
   The command obtains the timestamp that was assigned to that SDU.
#. Provide the controller with the SDUs for the remaining CISes or BISes using the timestamp method with the obtained timestamp.

.. note::
   Providing the same sequence number to different CISes or BISes does not time-synchronize the provided SDUs.

Only SDUs sent in the same CIG or BIG can be time-synchronized.
