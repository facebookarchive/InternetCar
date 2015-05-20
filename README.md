# Internet Car

InternetCar is an Internet of Things (IoT) sample application built on Parse. It
demonstrates the usage of Parse to enable communication between an remote device
and an IoT device. The IoT device could theoretically be any hardware which runs
an [Embedded SDK](https://www.parse.com/products/iot). Specifically, this
application communicates with the Texas Instruments
[SimpleLink Wi-Fi CC3200 LaunchPad](http://www.ti.com/tool/cc3200-launchxl). We
will refer to this device as simply 'CC3200' throughout the README.  This app is
adapted from the Parse [AnyDevice](https://github.com/parseplatform/AnyDevice)
sample app.

## CC3200 Setup

The device code running in the car is contained in directory cc3200.

Refer to the [Quick Start for the CC3200](https://www.parse.com/apps/quickstart#embedded/ticc3200) to get your device set up.

## Parse Application Setup

Create a new app on [Parse](https://parse.com/apps). Your
Application ID, Master Key, and Client Key will be available to you under
`Settings > Keys`.

## Cloud Code Setup

Before using the iOS application, the Parse Application and associated Cloud
Code must be setup. Full instructions can be found in the [Cloud Code
repository](../cloud).

## iOS Setup

The iOS control app is adapted from Anydevice, an App which we built for universally controlling a CC3200 device.


Anydevice requires Xcode 6 and iOS 8.

#### Setting up your Xcode project

1. Install dependencies via `Cocoapods` by running `pod install` in the `ios` directory.

2. Open the Xcode workspace at `Anydevice.xcworkspace`.

3. Copy your new Parse Application ID and Client Key into `PADConstants.m`.

  ```objective-c
  NSString * const PADApplicationId = @"YOUR_APPLICATION_ID";
  NSString * const PADClientKey = @"YOUR_CLIENT_KEY";
  ```

#### Setting up push notifications

In order to receive events that originate from a CC3200 device, you must
have push notifications set up. This
[tutorial](https://www.parse.com/tutorials/ios-push-notifications) provides full
push notification setup instructions.

## Application Overview

Typically, Anydevice allows you to control an LED on a CC3200 device, and to view the
current state of that LED. The LED has three states: On, Off, and Blinking.
Using Anydevice, you can send a message corresponding to one of these states
from a mobile device in order to change the LED's behavior. The LED state can
also be controlled from the CC3200 itself. When the LED state is updated on the
CC3200, a push notification is sent to the owner's mobile devices via Parse.
This way, Anydevice can correctly reflect the current state of the LED.

In InternetCar example, we adapted Anydevice to car control instead of LED control. You have Drive, Reverse and Flashing available as long as your device name contains a word "car" when you follow the following steps to provisioning your CC3200.

#### Provisioning the CC3200

In order to enable the communication between Anydevice and the CC3200 device,
the CC3200 device must go through a provisioning process whereby the device
registers on Parse and then connects to a wifi network. When you are adding a
new device via Anydevice, you are guided through the steps needed to complete
the device provisioning. An overview of the provisioning process is given below:

1. Once the '+' button on the home screen is tapped, the appropriate device
objects are created and initialized on Parse.

2. After initialization, you must connect to the CC3200's wireless access
point. The first step is to navigate to the Wi-Fi section in the iOS Settings
application.

3. Select the network corresponding to the CC3200 device's access point.

4. Upon connecting to the CC3200 device, navigate back to Anydevice. When you do
this, the connection with the CC3200 device is verified.

5. Confirm the provisioning details. These details include the wifi network SSID
to which the CC3200 device should connect along with the wifi password. Again, make
sure your device name constains a "car", e.g. my-internet-car, before you submit the
provisioning details.

6. Send the network information to the CC3200 device. This will happen when the
'Done' button is tapped on the confirmation screen.

7. After provisioning begins, the CC3200 device will stop broadcasting its
access point. Anydevice will then attempt to reconnect to a wifi network
automatically.

8. Wait for the confirmation event which is sent from the CC3200 device after it
provisions successfully. The provisioning flow will dismiss automatically when
this event is received (the event is sent via a push notification).
